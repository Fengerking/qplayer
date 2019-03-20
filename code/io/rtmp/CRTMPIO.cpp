/*******************************************************************************
	File:		CRTMPIO.cpp

	Contains:	RTMP io implement code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-04-15		Jun Lin			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <winsock2.h>
#include "Ws2tcpip.h"
#endif // __QC_OS_WIN32__
#include "qcErr.h"
#include "CRTMPIO.h"
#include "CMsgMng.h"

#include "rtmp.h"
#include "log.h"

#include "USystemFunc.h"
#include "USocketFunc.h"
#include "ULogFunc.h"
#include "UUrlParser.h"
#include "CDNSLookUp.h"
#include "CDNSCache.h"

CRTMPIO::CRTMPIO(CBaseInst * pBaseInst)
	: CBaseIO (pBaseInst)
	, m_hHandle(NULL)
	, m_bFirstByte(true)
	, m_nReadTime(0)
	, m_nDownloadSize(0)
	, m_bReconnectting(false)
	, m_bConnected(false)
	, m_llLastAudioMsgTime(0)
	, m_llLastVideoMsgTime(0)
	, m_pDNSLookUp(NULL)
	, m_pAddrInfo(NULL)
{
	SetObjectName ("CRTMPIO");
	qcSocketInit ();
    //qcRTMP_LogSetLevel(qcRTMP_LOGINFO);
    if(pBaseInst)
    	pBaseInst->AddListener(this);
    
	m_pDNSLookUp = m_pBaseInst->m_pDNSLookup;// new CDNSLookup(m_pBaseInst);
    m_pAddrInfo = new qcRTMP_IPAddrInfo;
    memset(m_pAddrInfo, 0, sizeof(qcRTMP_IPAddrInfo));
    m_pAddrInfo->pUserData = this;
    m_pAddrInfo->fGetAddrInfo = CRTMPIO::GetAddrInfo;
    m_pAddrInfo->fFreeAddrInfo = CRTMPIO::FreeAddrInfo;
    m_pAddrInfo->fGetCache = CRTMPIO::GetCache;
    m_pAddrInfo->fAddCache = CRTMPIO::AddCache;

	m_sHostIP = NULL;
    
#ifdef __QC_OS_WIN32__
    m_pInetNtop = NULL;
#endif

	m_llFileSize = 0X7FFFFFFFFFFFFFFF;
}

CRTMPIO::~CRTMPIO(void)
{
#ifdef __QC_OS_WIN32__
    QC_DEL_P(m_pInetNtop);
#endif

	Close ();
	qcSocketUninit ();
    QC_DEL_P(m_pAddrInfo);
	QC_FREE_P(m_sHostIP);
    if(m_pBaseInst)
        m_pBaseInst->RemListener(this);
}

int CRTMPIO::RecvEvent(int nEventID)
{
	if (nEventID == QC_BASEINST_EVENT_FORCECLOE)
    {
        if(m_pBaseInst)
        {
            if(m_pBaseInst->m_bForceClose)
            {
                if (m_hHandle)
                {
                    QCLOGW("[KPI]forceclose");
                    m_hHandle->forceClose = 1;
                }
            }
        }
    }
    
    return QC_ERR_NONE;
}

int CRTMPIO::Open (const char * pURL, long long llOffset, int nFlag)
{
    QCLOG_CHECK_FUNC(NULL, m_pBaseInst, m_pBaseInst->m_bForceClose?1:0);
    if(m_pBaseInst->m_bForceClose)
        return QC_ERR_FAILED;
    //QCLOGI("[KPI]RTMP forceclose %d, %d", m_hHandle?m_hHandle->forceClose:0, m_pBaseInst->m_bForceClose?1:0);
    Close();
	if (pURL != NULL)
    {
		if (m_pURL == NULL && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_DOWNLOAD_SPEED, 0, 0, pURL);

		QC_DEL_A(m_pURL);
		m_pURL = new char[strlen(pURL) + 1];
		strcpy(m_pURL, pURL);
	}
    
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_CONNECT_START, 0, 0, m_pURL);
    
    m_hHandle = qcRTMP_Alloc();
    qcRTMP_Init(m_hHandle);
    m_hHandle->Link.timeoutConnect = m_pBaseInst->m_pSetting->g_qcs_nTimeOutConnect;
    m_hHandle->Link.timeout = m_pBaseInst->m_pSetting->g_qcs_nTimeOutRead;
    m_hHandle->fIPAddrInfo = m_pAddrInfo;
    
    int nUseTime = qcGetSysTime();
	if (!qcRTMP_SetupURL(m_hHandle, (char*)m_pURL))
        return QC_ERR_FAILED;
	char * pHostAddr = strstr(m_pURL, "?domain=");
    if (pHostAddr != NULL && m_hHandle->Link.app.av_val)
    {
        char szProtocal[16];
		qcUrlParseProtocal(m_pURL, szProtocal);
        sprintf(m_szHostAddr, "%s://%s:%d/%s", szProtocal, pHostAddr + 8, m_hHandle->Link.port, m_hHandle->Link.app.av_val);
		AVal opt = {(char *)"tcUrl", 5};
#ifdef __QC_OS_IOS__
        AVal arg = {m_szHostAddr, (int)strlen(m_szHostAddr)};
#else
        AVal arg = {m_szHostAddr, strlen(m_szHostAddr)};
#endif
        qcRTMP_SetOpt(m_hHandle, &opt, &arg);
    }
    
    // Handle case: rtmp://183.146.213.65/live/hks?domain=live.hkstv.hk.lxdns.com
    char szHost[1204];
    AVal* host = NULL;
    if(m_hHandle->Link.socksport)
        host = &m_hHandle->Link.sockshost;
    else
        host = &m_hHandle->Link.hostname;
    if (host->av_val[host->av_len])
    {
        memcpy(szHost, host->av_val, host->av_len);
        szHost[host->av_len] = '\0';
    }
    else
        strcpy(szHost, host->av_val);

    int nRC = qcRTMP_Connect(m_hHandle, NULL);
    int nTryTimes = 0;
    while (nRC == 0 && !m_pBaseInst->m_bCheckReopn)
    {
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_CONNECT_FAILED, qcGetSysTime() - nUseTime, 0);
        if(m_pBaseInst && m_pBaseInst->m_pDNSCache && m_hHandle->Link.sockAddr)
			m_pBaseInst->m_pDNSCache->Del(szHost, m_hHandle->Link.sockAddr, sizeof(struct sockaddr));

        nTryTimes++;
        if (nTryTimes > 5 || m_pBaseInst->m_bForceClose)
            break;
        
        qcSleepEx (100000, &m_pBaseInst->m_bForceClose);
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_CONNECT_START, 0, 0, m_pURL);
        nUseTime = qcGetSysTime();
        QCLOGI("Try to connect server again at %d  times.", nTryTimes);
        nRC = qcRTMP_Connect(m_hHandle, NULL);
    }
    
    if(nRC == 0)
        return QC_ERR_FAILED;

    m_bFirstByte = true;
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_CONNECT_SUCESS, 0, 0);
    //QCLOGI("RTMP connect use time %d, %d", qcGetSysTime()-nUseTime, qcGetSysTime());

	return QC_ERR_NONE;
}

int CRTMPIO::Reconnect (const char * pNewURL, long long llOffset)
{
    m_bReconnectting = true;
	return Open(pNewURL, llOffset, 0);
}

int CRTMPIO::Close (void)
{
    CAutoLock lock(&m_mtLockFunc);
    if (m_hHandle)
    {
        qcRTMP_Close(m_hHandle);
        qcRTMP_Free(m_hHandle);
        m_hHandle = NULL;
    }

	return QC_ERR_NONE;
}

int CRTMPIO::Stop (void)
{
    CAutoLock lock(&m_mtLockFunc);
    CBaseIO::Stop();
    if (m_hHandle)
    {
        QCLOGI("[KPI]forceclose");
        m_hHandle->forceClose = 1;
    }
    
    return QC_ERR_NONE;
}


int CRTMPIO::Read (unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
    CAutoLock lock(&m_mtLockFunc);
    if(!m_hHandle)
        return QC_ERR_STATUS;
    
    int nTime = qcGetSysTime();
    m_hHandle->m_read.nCurrPacketType = 0;
    int nRet = qcRTMP_Read(m_hHandle, (char*)pBuff, nSize);
    
    // for reconnect server, we need consider stream is stopped by push stream side
    // for this case, it can connect server successfully, but will not read any data.
    if(nRet == 0)
	{
        if(m_bReconnectting)
        {
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_RECONNECT_FAILED, 0, 0);
            m_bReconnectting = false;
        }
        else if(m_bConnected)
        {
            if (m_sStatus != QCIO_Stop && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_DISCONNECTED, 0, 0);
            m_bConnected = false;
        }
        nSize = 0;
		return QC_ERR_RETRY;
	}
	else if (nRet < 0)
    {
        nSize = 0;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_CONNECT_FAILED, 0, 0);
        return QC_ERR_RETRY;
    }
    
    //QCLOGI("[TS]type %d: timestamp %d", m_hHandle->m_read.nCurrPacketType, m_hHandle->m_read.timestamp);
    if(m_hHandle->m_read.nCurrPacketType == qcRTMP_PACKET_TYPE_AUDIO)
        m_llLastAudioMsgTime = m_hHandle->m_read.timestamp;
    else if(m_hHandle->m_read.nCurrPacketType == qcRTMP_PACKET_TYPE_VIDEO)
        m_llLastVideoMsgTime = m_hHandle->m_read.timestamp;
    
    m_bConnected = true;
    if(m_bReconnectting)
    {
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_RECONNECT_SUCESS, 0, 0);
        m_bReconnectting = false;
    }
    
    nSize = nRet;
    m_nDownloadSize += nSize;
    m_nReadTime += (qcGetSysTime() - nTime);
    
    if(m_nReadTime > 2000)
    {
        //QCLOGI("[RTMP]Time %d, size %lld, speed %f (MBytes/Sec)", m_nReadTime, m_llDownloadSize, (((int)m_llDownloadSize)*8*1000)/(m_nReadTime*1024.0*1024.0));
        m_llDownPos += m_nDownloadSize;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_DOWNLOAD_SPEED, m_nDownloadSize * 1000 / m_nReadTime, m_llDownPos);
        m_nDownloadSize = 0;
        m_nReadTime = 0;
    }
    
    if(m_bFirstByte)
    {
        m_bFirstByte = false;
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_IO_FIRST_BYTE_DONE, 0, 0);
    }
    
#if 0
    qcDumpFile(pBuff, nSize, "flv");
#endif

	return QC_ERR_NONE;
}

QCIOType CRTMPIO::GetType (void)
{
    return QC_IOTYPE_RTMP;
}


int	CRTMPIO::ReadAt (long long llPos, unsigned char * pBuff, int & nSize, bool bFull, int nFlag)
{
//	if (llPos != m_llReadPos)
//		SetPos (llPos, QCIO_SEEK_BEGIN);
	return Read (pBuff, nSize, bFull, nFlag);
}

int CRTMPIO::GetParam (int nID, void * pParam)
{
    if(nID == QCIO_PID_RTMP_AUDIO_MSG_TIMESTAMP)
    {
        *(long long*)pParam = m_llLastAudioMsgTime;
        return QC_ERR_NONE;
    }
    else if(nID == QCIO_PID_RTMP_VIDEO_MSG_TIMESTAMP)
    {
        *(long long*)pParam = m_llLastVideoMsgTime;
        return QC_ERR_NONE;
    }
    
    return CBaseIO::GetParam(nID, pParam);
}

int CRTMPIO::GetAddrInfo (void* pUserData, const char* pszHostName, const char* pszService,
                                 const struct addrinfo* pHints, struct addrinfo** ppResult)
{
    if(!pUserData)
        return -1;
    
    return ((CRTMPIO*)pUserData)->doGetAddrInfo(pszHostName, pszService, pHints, ppResult);
}

int CRTMPIO::FreeAddrInfo (void* pUserData, struct addrinfo* pResult)
{
    if(!pUserData)
        return -1;

    return ((CRTMPIO*)pUserData)->doFreeAddrInfo(pResult);
}

int CRTMPIO::doGetAddrInfo (const char* pszHostName, const char* pszService,
                          const struct addrinfo* pHints, struct addrinfo** ppResult)
{
    if(IsUseDNSLookup())
    {
        QCLOGI("Use DNS lookup");
        if(m_pDNSLookUp)
        {
            m_pDNSLookUp->GetDNSAddrInfo((char*)pszHostName, (void*)pszService, (void*)pHints, (void**)ppResult);
        }
    }
    else
    {
        ::getaddrinfo(pszHostName, NULL, pHints, ppResult);
    }
    
    struct addrinfo* pAddrInfo = *ppResult;
    if(pAddrInfo && pAddrInfo->ai_addr)
    {
        ResolveIP(pAddrInfo->ai_addr, false);
    }
    
    return QC_ERR_NONE;
}

int CRTMPIO::doFreeAddrInfo (struct addrinfo* pResult)
{
    if(IsUseDNSLookup())
    {
        //QCLOGI("Network is ipv4");
        if(m_pDNSLookUp)
        {
            return m_pDNSLookUp->FreeDNSAddrInfo(pResult);
        }
    }
	else
	{
		::freeaddrinfo(pResult);
	}
    
    return QC_ERR_NONE;
}

void* CRTMPIO::GetCache (void* pUserData, char* pszHostName)
{
    if(!pUserData)
        return NULL;
    
    return ((CRTMPIO*)pUserData)->doGetCache(pszHostName);
}

int CRTMPIO::AddCache (void* pUserData, char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime)
{
    if(!pUserData)
        return -1;
    
    return ((CRTMPIO*)pUserData)->doAddCache(pHostName, pAddress, pAddressSize, nConnectTime);
}

void* CRTMPIO::doGetCache (char* pszHostName)
{
	if (m_sHostIP == NULL) 
		m_sHostIP = (sockaddr *)malloc(sizeof(struct sockaddr_storage));
	memset(m_sHostIP, 0, sizeof(struct sockaddr_storage));
	if (m_pBaseInst && m_pBaseInst->m_pDNSCache)
	{
		if (m_pBaseInst->m_pDNSCache->Get(pszHostName, m_sHostIP) == QC_ERR_NONE)
		{
			ResolveIP(m_sHostIP, true);
			return m_sHostIP;
		}
	}
    return NULL;
}

int CRTMPIO::doAddCache (char* pHostName, void * pAddress, unsigned int pAddressSize, int nConnectTime)
{
    if(m_pBaseInst && m_pBaseInst->m_pDNSCache)
    {
        return m_pBaseInst->m_pDNSCache->Add (pHostName, (void *)pAddress, pAddressSize, nConnectTime);
    }

    return QC_ERR_NONE;
}

bool CRTMPIO::IsUseDNSLookup()
{
    //return false;
    if (!qcIsIPv6() && strcmp(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, "127.0.0.1"))
        return true;
    return false;
}

int CRTMPIO::ResolveIP(void* pAddr, bool bFromCache)
{
    if(!pAddr)
        return QC_ERR_EMPTYPOINTOR;
    
    char pHostIP[INET6_ADDRSTRLEN];
    void* numericAddress = NULL;
    struct sockaddr* cachedHostIp = (struct sockaddr*)pAddr;
    
    if (cachedHostIp != NULL)
    {
        numericAddress = (void*)&(((struct sockaddr_in*)cachedHostIp)->sin_addr);
#ifdef __QC_OS_WIN32__
        if (m_pInetNtop == NULL)
            m_pInetNtop = new CQCInetNtop();
        m_pInetNtop->qcInetNtop(cachedHostIp->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN);
#else
        inet_ntop(cachedHostIp->sa_family, numericAddress, pHostIP, INET6_ADDRSTRLEN);
#endif // __QC_OS_WIN32__
        
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(bFromCache ? QC_MSG_RTMP_DNS_GET_CACHE : QC_MSG_RTMP_DNS_GET_IPADDR, 0, 0, pHostIP);
    }
    
    return QC_ERR_NONE;
}


