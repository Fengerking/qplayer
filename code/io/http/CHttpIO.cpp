/*******************************************************************************
	File:		CHttpIO.cpp
 
	Contains:
 
	Written by:	liangliang
 
	Change History (most recent first):
	12/14/16		liangliang			Create file
 
 *******************************************************************************/

#include <stdlib.h>

#include "qcErr.h"

#include "ULogFunc.h"

#include "CHttpIO.h"
#include "CTcpIO.h"

CHttpIO::CHttpIO (void) : m_pIO(NULL), m_llPos(0), m_nRedicrts(0), m_nBuffRead(0), m_nBuffWrite(0), m_nHttpCode(0), m_llOff(0), m_llFileSize(0), m_bSeekable(false), m_llChunkSize(0)
{
    SetObjectName("CHttpIO");
    m_vBuff.resize(2048);
}

CHttpIO::~CHttpIO (void)
{
    Close();
}

int CHttpIO::Open (const char *pURL, int nFlag)
{
    Close();
    
    QCLOGI("open url : %s", pURL);
    
    if (NULL == pURL)
    {
        return QC_ERR_FAILED;
    }
    
    m_pIO = new CTcpIO();
    int ret = m_pIO->Open(pURL, nFlag);
    if (QC_ERR_NONE != ret)
    {
        Close();
        return ret;
    }
    
    m_url.paserString(pURL);
    m_vHeaders.clear();
    
    AddHeader("GET %s HTTP/1.1\r\n", m_url.path().c_str());
    AddHeader("Host: %s\r\n", m_url.host().c_str());
    AddHeader("User-Agent: qcplayer\r\n");
    AddHeader("Connection: close\r\n");
    AddHeader("Referer: %s\r\n", m_url.host().c_str());
    AddHeader("Accept: */*\r\n");
    AddHeader("Icy-MetaData: 1\r\n");
    if (m_llFileSize > 0)
    {
        AddHeader("Range: %lld-%lld\r\n", m_llOff, m_llFileSize);
    }
    else if (m_llOff > 0)
    {
        AddHeader("Range: %lld-\r\n", m_llOff);
    }
    AddHeader("\r\n");
    
    if (m_vHeaders.size()) {
        char * begin = &*m_vHeaders.begin();
        ret = m_pIO->Write((unsigned char *)begin, (int)m_vHeaders.size());
        if (QC_ERR_FAILED == ret) {
            Close();
            return ret;
        }
    }
    
    Run();
    m_nBuffRead = 0;
    
    std::string line;
    do
    {
        line.clear();
        ret = ReadLine(line);
        if (QC_ERR_NONE == ret)
        {
            if (line.size() == 0)
            {
                // read http header end;
                QCLOGI("read http header end");
                break;
            }
            QCLOGI("response header: %s", line.c_str());
            ParseLine(line);
        }
        else
        {
            Close();
            return QC_ERR_FAILED;
        }
    } while (true);
    
    return CheckHttpCode();
}

int CHttpIO::Reconnect (const char *pNewURL)
{
    Close();
    return Open(pNewURL, 0);
}

int CHttpIO::Close ()
{
    if (m_pIO)
    {
        m_pIO->Close();
        delete m_pIO;
        m_pIO = NULL;
    }
    
    return QC_ERR_NONE;
}

int CHttpIO::Read (unsigned char *pBuff, int nSize)
{
    int ret = QC_ERR_RETRY;
    if (m_nBuffWrite != m_nBuffRead)
    {
        int size = m_nBuffWrite - m_nBuffRead;
        if (size >= nSize)
        {
            char * p = &*m_vBuff.begin() + m_nBuffRead;
            memcpy(pBuff, p, nSize);
            m_nBuffRead += nSize;
            m_llPos += nSize;
            ret = nSize;
        }
        else
        {
            char * p = &*m_vBuff.begin() + m_nBuffRead;
            memcpy(pBuff, p, size);
            ret = m_pIO->ReadAt(m_llPos, pBuff + size, nSize - size);
            if (QC_ERR_NONE == ret)
            {
                m_nBuffRead += size;
                m_llPos += nSize;
            }
        }
        return ret;
    }
    else
    {
        ret = m_pIO->ReadAt(m_llPos, pBuff, nSize);
        if (QC_ERR_NONE == ret)
        {
            m_llPos += nSize;
        }
        return ret;
    }
}

int CHttpIO::ReadAt (long long llPos, unsigned char *pBuff, int nSize)
{
    int ret = QC_ERR_RETRY;
    if (m_nBuffWrite != m_nBuffRead && llPos < m_nBuffWrite - m_nBuffRead + m_llOff)
    {
        int size = m_nBuffWrite - m_nBuffRead;
        if (size >= nSize)
        {
            char * p = &*m_vBuff.begin() + m_nBuffRead;
            memcpy(pBuff, p, nSize);
            m_nBuffRead += nSize;
            m_llPos += nSize;
            ret = nSize;
        }
        else
        {
            char * p = &*m_vBuff.begin() + m_nBuffRead;
            memcpy(pBuff, p, size);
            ret = m_pIO->ReadAt(m_llPos, pBuff + size, nSize - size);
            if (QC_ERR_NONE == ret)
            {
                m_nBuffRead += size;
                m_llPos += nSize;
            }
        }
        return ret;
    }
    else
    {
        ret = m_pIO->ReadAt(llPos, pBuff, nSize);
        if (QC_ERR_NONE == ret)
        {
            m_llPos += nSize;
        }
        return ret;
    }
}

int CHttpIO::Write (unsigned char *pBuff, int nSize)
{
    return QC_ERR_IMPLEMENT;
}

int CHttpIO::GetSpeed ()
{
    if (m_pIO)
    {
        return m_pIO->GetSpeed();
    }
    
    return QC_ERR_STATUS;
}

int CHttpIO::Run ()
{
    if (m_pIO)
    {
        return m_pIO->Run();
    }
    
    return QC_ERR_STATUS;
}

int CHttpIO::Pause ()
{
    if (m_pIO)
    {
        return m_pIO->Pause();
    }
    
    return QC_ERR_STATUS;
}

int CHttpIO::Stop ()
{
    if (m_pIO)
    {
        return m_pIO->Stop();
    }
    
    return QC_ERR_STATUS;
}

long long CHttpIO::GetPos ()
{
    return m_llPos;
}

long long CHttpIO::SetPos (long long llPos, int nFlag)
{
    m_llOff = llPos;
    if (llPos != m_llPos)
    {
        SetPos(llPos, 0);
        Reconnect(m_url.url().c_str());
    }

    return m_llPos;;
}

long long CHttpIO::GetSize ()
{
    return m_llFileSize;
}

QCIOType CHttpIO::GetType ()
{
    return QC_IOTYPE_NONE;
}

int CHttpIO::GetParam (int nID, void *pParam)
{
    return QC_ERR_IMPLEMENT;
}

int CHttpIO::SetParam (int nID, void *pParam)
{
    return QC_ERR_IMPLEMENT;
}

bool CHttpIO::IsSpace (const char c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

int CHttpIO::AddHeader (const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    size_t size = m_vHeaders.size();
    m_vHeaders.resize(m_vHeaders.size() + len);
    char * p = &*m_vHeaders.begin() + size;
    va_start(args, fmt);
    len = vsprintf(p, fmt, args);
    QCLOGI("request header %s", p);
    va_end(args);
    
    return len;
}

int CHttpIO::ReadLine (std::string & line)
{
    int lineBegin = m_nBuffRead;
    do
    {
        // TODO checkout the timeout
        if (m_nBuffWrite != m_nBuffRead)
        {
            char ch = m_vBuff[m_nBuffRead];
            if ('\n' == ch)
            {
                if ('\r' == m_vBuff[m_nBuffRead - 1])
                {
                    break;
                }
            }
            m_nBuffRead++;
        }
        else
        {
            line.append(m_vBuff.begin() + lineBegin, m_vBuff.begin() + m_nBuffRead);
            char * pBuff = &*m_vBuff.begin();
            int ret = m_pIO->Read((unsigned char *)pBuff, (int)m_vBuff.size());
            if (QC_ERR_FAILED == ret)
            {
                return ret;
            }
            else
            {
                m_nBuffRead = 0;
                m_nBuffWrite = ret;
                lineBegin = 0;
            }
            
        }
    } while (true);
    
    int lineEnd = m_nBuffRead - 1;
    line.append(m_vBuff.begin() + lineBegin, m_vBuff.begin() + lineEnd);
    m_nBuffRead++;
    return QC_ERR_NONE;
}

void CHttpIO::ParseLine (std::string & line)
{
#ifdef __QC_OS_WIN32__
	char szLine[1024];
	strcpy (szLine, line.c_str ());
	strlwr (szLine);
	line = szLine;
#else
    std::transform(line.begin(), line.end(), line.begin(), std::tolower);
#endif //__QC_OS_WIN32__
    if (!line.compare(0, 5, "http/"))
    {
        const char * p = line.c_str();
        while (!IsSpace(*p) && *p != '\0')
            p++;
        while (IsSpace(*p))
            p++;
        char * pEnd;
        m_nHttpCode = (int)strtol(p, &pEnd, 10);
    }
    else
    {
        const char *p = line.c_str();
        while (*p != '\0' && *p != ':')
            p++;
        if (*p != ':')
            return;
        
        std::string tag(line.c_str(), p - line.c_str());
        p++;
        while (IsSpace(*p))
            p++;
        if (!tag.compare("location"))
        {
            m_sLocation.assign(p);
            m_bNewLocation = true;
        }
        else if (!tag.compare("content-length") && m_llFileSize == -1)
        {
            m_llFileSize = strtol(p, NULL, 10);
        }
        else if (!tag.compare("content-range"))
        {
            ParseContentRange(p);
        }
        else if (!tag.compare("content-type"))
        {
            m_sContentType.assign(p);
        }
        else if (!tag.compare("accept-ranges") && !strncmp(p, "bytes", 5))
        {
            m_bSeekable = true;
        }
        else if (!tag.compare("transfer-encoding") && !strncmp(p, "chunked", 7))
        {
            m_llFileSize  = -1;
            m_llChunkSize = 0;
        }
        else if (!tag.compare("connection"))
        {
            
        }
        else if (!tag.compare("server"))
        {
            m_sServer.assign(p);
        }
    }
}

void CHttpIO::ParseContentRange (const char * p)
{
    const char * slash;
    
    if (!strncmp(p, "bytes ", 6)) {
        p += 6;
        m_llOff = strtol(p, NULL, 10);
        m_llPos = m_llOff;
        if ((slash = strchr(p, '/')) && strlen(slash) > 0)
            m_llFileSize = strtol(slash + 1, NULL, 10);
    }
}

int CHttpIO::CheckHttpCode ()
{
    if ((301 == m_nHttpCode || 302 == m_nHttpCode || 303 == m_nHttpCode || 307 == m_nHttpCode) &&
        m_bNewLocation)
    {
        if (MAX_REDIRECTS >= m_nRedicrts)
            return QC_ERR_FAILED;
        
        m_bNewLocation = false;
        return Reconnect(m_sLocation.c_str());
    }
    else if (400 <= m_nHttpCode && m_nHttpCode < 600)
    {
        Close();
        return QC_ERR_FAILED;
    }
    
    return QC_ERR_NONE;
}
