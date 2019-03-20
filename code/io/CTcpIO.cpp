/*******************************************************************************
	File:		CTcpIO.cpp
 
	Contains:
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/13/16		liangliang			Create file
 
 *******************************************************************************/
#include "qcErr.h"

#include "CTcpIO.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

#include "USocket.h"

CTcpIO::CTcpIO (void) : m_status(QCIO_Init), m_nError(QC_ERR_NONE), m_nFD(-1), m_nRead(0), m_nWrite(0), m_llPos(0), m_llBytesCount(0), m_nPreTime(0), m_nSpeed(0)
{
    SetObjectName("CTcpIO");
    m_pMutexLock = new CMutexLock();
}

CTcpIO::~CTcpIO (void)
{
    m_ThreadWork.Stop();
    
    if (m_pMutexLock)
    {
        delete m_pMutexLock;
        m_pMutexLock = NULL;
    }
}

int CTcpIO::Open (const char * pURL, int nFlag)
{
    m_url.paserString(pURL);
    if (m_url.host().size()) {
        struct addrinfo hints = { 0 }, * ai, * cur_ai;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
#ifdef __QC_OS_IOS__
        hints.ai_flags = AI_DEFAULT;
#endif
        
        char portstr[10];
        snprintf(portstr, sizeof(portstr), "%d", m_url.port());
        
        QCLOGI("dns begin : %s", m_url.host().c_str());
        int ret = getaddrinfo(m_url.host().c_str(), portstr, &hints, &ai);
        if (ret)
        {
            QCLOGE("dns failed %d %s : %s", ret, gai_strerror(ret), m_url.host().c_str());
            return QC_ERR_IO_DNS_FAILED;
        }
        
        QCLOGI("dns success : %s", m_url.host().c_str());
 
    reconnect:
        cur_ai = ai;
        // 在 ipv6 的环境下，使用 ipv4 的地址， port 需要多设置一次
        if (cur_ai->ai_family == AF_INET6)
        {
            struct sockaddr_in6 * in6 = (struct sockaddr_in6 *)cur_ai->ai_addr;
            in6->sin6_port = htons(m_url.port());
        }
        else
        {
            struct sockaddr_in * in = (struct sockaddr_in *)cur_ai->ai_addr;
            char ip[30];
            QCLOGI("connect ipv4 : %s:%d", inet_ntop(AF_INET, &in->sin_addr, ip, 29), ntohs(in->sin_port));
        }
        
        m_nFD = qcSockSocket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
        if (m_nFD < 0)
        {
            goto error;
        }
        
        ret = qcSockConnect(m_nFD, cur_ai->ai_addr, cur_ai->ai_addrlen);
        if (QC_ERR_FAILED == ret)
        {
            if (cur_ai->ai_next)
            {
                /* connect with the next sockaddr */
                cur_ai = cur_ai->ai_next;
                if (m_nFD >= 0)
                    qcSockClose(m_nFD);
                ret = 0;
                goto reconnect;
            }
            else
            {
                goto error;
            }
        }
        else if (QC_ERR_IO_AGAIN == ret)
        {
            do
            {
                // TODO should check timeout 
                ret = qcSockSelect(m_nFD, 1, 0, 10);
            } while(QC_ERR_IO_AGAIN == ret || QC_ERR_TIMEOUT == ret);
            
            if (QC_ERR_NONE != ret)
            {
                goto error;
            }
        }
        
        freeaddrinfo(ai);
        return QC_ERR_NONE;
        
    error:
        Close();
        freeaddrinfo(ai);
    }
    return QC_ERR_FAILED;
}

int CTcpIO::Reconnect (const char * pNewURL)
{
    Pause();
    Close();
    return Open(pNewURL, 0);
}

int CTcpIO::Close ()
{
    if (m_nFD >= 0)
    {
        qcSockClose(m_nFD);
        m_nFD = -1;
    }
    
    return QC_ERR_NONE;
}

int CTcpIO::Read (unsigned char * pBuff, int nSize)
{
    int ret = 0;
    m_pMutexLock->Lock();
    
    do
    {
        if (QCIO_Run != m_status || NULL == pBuff)
        {
            break;
        }
        
        int size = ReadableSize();
        size = size <= nSize ? size : nSize;
        memcpy(pBuff, m_pBuffer + m_nRead, size);
        m_nRead += size;
        m_llPos += size;
        ret = size;
    } while(false);
    
    m_pMutexLock->Unlock();
    
    return ret;
}

int CTcpIO::ReadAt (long long llPos, unsigned char *pBuff, int nSize)
{
    int ret = QC_ERR_RETRY;
    m_pMutexLock->Lock();
    
    if (llPos >= m_llPos)
    {
        int offset = llPos - m_llPos;
        int size = ReadableSize();
        if (size >= offset + nSize)
        {
            m_nRead += offset;
            memcpy(pBuff, m_pBuffer + m_nRead, nSize);
            m_nRead += nSize;
            m_llPos = llPos;
            ret = nSize;
        }
        else if (QC_ERR_NONE != m_nError)
        {
            ret = m_nError;
        }
    }
    else
    {
        ret = QC_ERR_FAILED;
    }
    
    m_pMutexLock->Unlock();
    return ret;
}

int CTcpIO::Write (unsigned char * pBuff, int nSize)
{
    if (m_nFD < 0)
    {
        return QC_ERR_FAILED;
    }
    
    int ret = 0;
    int nSend = 0;
    while (nSend < nSize)
    {
        // TODO should check timeout
        ret = qcSockSelect(m_nFD, 1, 0, 10);
        if (QC_ERR_NONE == ret)
        {
            ret = qcSockWrite(m_nFD, pBuff + nSend, nSize - nSend);
            if (ret >= 0)
            {
                nSend += ret;
            }
        }
        else if (QC_ERR_FAILED == ret)
        {
            return QC_ERR_FAILED;
        }
    }
    
    return nSend;
}

int CTcpIO::GetSpeed ()
{
    int ret = 0;
    m_pMutexLock->Lock();
    ret = m_nSpeed;
    m_pMutexLock->Unlock();
    return ret;
}

int CTcpIO::Run ()
{
    m_pMutexLock->Lock();
    m_ThreadWork.SetWorkProc(this, (int	(CThreadFunc::*) (void *))&CTcpIO::OnWorkItem, NULL);
    int ret = m_ThreadWork.Start();
    if (QC_ERR_NONE == ret)
    {
        m_status = QCIO_Run;
        m_llBytesCount = 0;
        m_nPreTime = 0;
        m_nSpeed = 0;
    }
    m_pMutexLock->Unlock();
    return ret;
}

int CTcpIO::Pause ()
{
    int ret = m_ThreadWork.Pause();
    if (QC_ERR_NONE == ret)
    {
        m_pMutexLock->Lock();
        m_status = QCIO_Pause;
        m_llBytesCount = 0;
        m_nPreTime = 0;
        m_nSpeed = 0;
        m_pMutexLock->Unlock();
    }
    return ret;
}

int CTcpIO::Stop ()
{
    m_pMutexLock->Lock();
    m_status = QCIO_Stop;
    m_pMutexLock->Unlock();
    return m_ThreadWork.Stop();
}

long long CTcpIO::GetPos ()
{
    return m_llPos;
}

int CTcpIO::GetParam (int nID, void *pParam)
{
    return QC_ERR_IMPLEMENT;
}

int CTcpIO::SetParam (int nID, void *pParam)
{
    return QC_ERR_IMPLEMENT;
}

int CTcpIO::OnWorkItem (void *pParam)
{
    m_pMutexLock->Lock();
    
    do
    {
        if (QCIO_Run != m_status || m_nFD < 0 || QC_ERR_IO_EOF == m_nError)
        {
            break;
        }
        
        int size = WriteableSize();
        int ret = qcSockSelect(m_nFD, 0, 1, 10);
        if (QC_ERR_NONE == ret && size > 0)
        {
            ret = qcSockRead(m_nFD, m_pBuffer + m_nWrite, size);
            if (ret > 0)
            {
                m_nWrite += ret;
                m_llBytesCount += ret;
            }
            else
            {
                m_nError = ret;
            }
        }
        
        int curTime = qcGetSysTime();
        if (0 == m_nPreTime)
        {
            m_nPreTime = curTime;
        }
        else if (curTime - m_nPreTime >= 5000)
        {
            int second = (curTime - m_nPreTime) / 1000;
            m_nSpeed = m_llBytesCount / second;
            QCLOGI("status : speed(%d), bytes(%lld), (curtime)%d, (pretime)%d", m_nSpeed, m_llBytesCount, curTime, m_nPreTime);
            
            m_nPreTime = curTime;
            m_llBytesCount = 0;
        }
        
    } while(false);
    
    m_pMutexLock->Unlock();
    
    return QC_ERR_NONE;
}

int CTcpIO::ReadableSize ()
{
    int size = 0;
    if (m_nRead == m_nWrite)
    {
        if (0 != m_nRead)
        {
            size = kBufferSize - m_nRead - 1;
        }
    }
    else if (m_nRead < m_nWrite)
    {
        size = m_nWrite - m_nRead;
    }
    else if (m_nRead > m_nWrite)
    {
        size = kBufferSize - m_nRead - 1;
    }
    
    return size;
}

int CTcpIO::WriteableSize ()
{
    int size = 0;
    if (m_nWrite == m_nRead)
    {
        if (0 == m_nRead)
        {
            size = kBufferSize - m_nWrite - 1;
        }
    }
    if (m_nWrite > m_nRead)
    {
        size = kBufferSize - m_nWrite - 1;
        if (0 == size && 0 != m_nRead)
        {
            m_nWrite = 0;
            size = m_nRead - m_nWrite - 1;
        }
    }
    else if (m_nWrite < m_nRead)
    {
        size = m_nRead - m_nWrite - 1;
    }
    
    return size;
}
