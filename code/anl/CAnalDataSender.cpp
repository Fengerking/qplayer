/*******************************************************************************
	File:		CAnalDataSender.h
 
	Contains:	Analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-21		Jun			Create file
 
 *******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <winsock2.h>
#include "Ws2tcpip.h"
#endif  // __QC_OS_WIN32__

#include "CAnalDataSender.h"
#include "qcErr.h"
#include "UUrlParser.h"
#include "ULogFunc.h"
#include "UAVFormatFunc.h"
#include "USystemFunc.h"
#include "CFileIO.h"

//#define _PRINTF_HTTP_DATA_

CAnalDataSender::CAnalDataSender(CBaseInst * pBaseInst, CDNSCache* pCache, char* pszDstServer)
	: CHTTPClient(pBaseInst, pCache)
	, m_bDNSReady(false)
	, m_nReportInterval(120)
	, m_nSampleInterval(60)
	, m_bUpdateSampleTime(false)
{
#ifndef __QC_OS_WIN32__
    //signal(SIGPIPE, SIG_IGN);
#endif
    m_bNotifyMsg = false;
    SetObjectName("AnlSnd");
    
    ReadRawData();

    memset(m_szDstServer, 0, 1024);
    if(pszDstServer)
        sprintf(m_szDstServer, "%s", pszDstServer);
    
    m_nPostStartTime = qcGetSysTime();
    m_pThreadWork = new CThreadWork(pBaseInst);
    m_pThreadWork->SetOwner (m_szObjName);
    m_pThreadWork->SetWorkProc (this, &CThreadFunc::OnWork);
    m_pThreadWork->Start ();
}

CAnalDataSender::~CAnalDataSender()
{
    QCLOGI("+[ANL]Snd destroyed, %d", m_listAnlRaw.GetCount());
    m_pBaseInst->m_bForceClose = true;
    if(m_pThreadWork)
    {
        m_pThreadWork->Stop();
        delete m_pThreadWork;
        m_pThreadWork = NULL;
    }
    
    if(m_nSocketHandle != KInvalidSocketHandler)
        SocketClose(m_nSocketHandle);
    m_nSocketHandle = KInvalidSocketHandler;
    WriteRawData();
    ReleaseAnlRawList();
    QC_DEL_P(m_pBaseInst);
    QCLOGI("-[ANL]Snd destroyed");
}

int CAnalDataSender::Prepare(char* szServer)
{
    if(m_nWSAStartup || !szServer)
        return QC_ERR_CANNOT_CONNECT;
    QCLOGI("[ANL]+Rsv");
    
    int nPort;
	qcUrlParseUrl(szServer, m_szHostAddr, m_szHostFileName, nPort, m_szDomainAddr);
    
    m_nStatusCode = 0;
    m_bCancel = false;
    m_bTransferBlock = false;
    m_bMediaType = false;
    m_llContentLength = -1;
    if (m_sHostIP == NULL)
    {
        m_sHostIP = (QCIPAddr)malloc(sizeof(struct sockaddr_storage));
    }
    else
    {
        memset(m_sHostIP, 0, sizeof(struct sockaddr_storage));
    }
    m_nHostIP = 0;
    
    int nTime = qcGetSysTime();
    int nErr = ResolveDNS(m_szHostAddr, m_sHostIP);
    QCLOGI("[ANL]-Rsv %d, %X", qcGetSysTime()-nTime, nErr);
    if(nErr != QC_ERR_NONE)
        return nErr;
    m_bDNSReady = true;
    
    nTime = qcGetSysTime();
    int nTimeout = 2000;
    nErr = ConnectServer(m_sHostIP, nPort, nTimeout);
    QCLOGI("[ANL][CNT]Done %d, timeout setting %d, forceclose %d", qcGetSysTime()-nTime, nTimeout, m_pBaseInst->m_bForceClose?1:0);
    if( nErr != QC_ERR_NONE)
        return nErr;
    
#ifdef __QC_OS_IOS__
    int set = 1;
    setsockopt(m_nSocketHandle, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    
    //SetSocketCheckForNetException();
    
    struct timeval nTimeOut = {0, 200000};
    setsockopt(m_nSocketHandle, SOL_SOCKET, SO_SNDTIMEO, (char *)&nTimeOut, sizeof(struct timeval));
    setsockopt(m_nSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeOut, sizeof(struct timeval));
    return QC_ERR_NONE;
}

int CAnalDataSender::PostData(const char* pData, int nLen, bool bWait)
{
    int nRet = QC_ERR_RETRY;
    int nTime = qcGetSysTime();
    if(m_sState != CONNECTED)
    {
        nRet = Prepare(m_szDstServer);
        if(nRet != QC_ERR_NONE)
            return nRet;
    }
    
    int nTryCount = 5;
    nRet = QC_ERR_RETRY;
    while (nRet != QC_ERR_NONE && nTryCount>0)
    {
        nTryCount--;
        
        if(m_nSocketHandle > 0)
        {
            if(bWait)
            {
                struct timeval tTimeout = {0, 500000};
                nRet = WaitSocketWriteBuffer(m_nSocketHandle, tTimeout);
                //QCLOGI("[ANL]Wait socket IO use time %d", qcGetSysTime()-nTime);
            }
            if(nRet == QC_ERR_NONE || !bWait)
            	nRet = CHTTPClient::Send(pData, nLen);
            else if(nRet == QC_ERR_TIMEOUT && !m_pBaseInst->m_bForceClose)
                continue;
        }
        
        if(nRet == QC_ERR_NONE)
        {
#ifdef _PRINTF_HTTP_DATA_
            QCLOGI("[ANL][SND]Success, %s", pData);
#endif
        }
        else
        {
#ifdef _PRINTF_HTTP_DATA_
            QCLOGW("[ANL][SND]Failed POST, %d", nRet);
#endif
            
            if(m_pBaseInst->m_bForceClose || IsCancel())
            {
            	QCLOGW("[ANL][SND]Force exit");
                break;
            }
            
            if(m_nSocketHandle > 0)
            {
#ifdef __QC_OS_WIN32__
                closesocket(m_nSocketHandle);
#else
                close(m_nSocketHandle);
#endif
                bWait = true;
                m_nSocketHandle = KInvalidSocketHandler;
            }
            
            Prepare(m_szDstServer);
        }
    }
    
    if(nRet != QC_ERR_NONE || nTryCount<=0)
    {
#ifdef _PRINTF_HTTP_DATA_
        QCLOGW("[ANL][SND]Lost event: %s", pData);
#endif
    }
    
//#ifdef _PRINTF_HTTP_DATA_
    QCLOGI("[ANL][SND]Done %d, force status %d", qcGetSysTime()-nTime, m_pBaseInst->m_bForceClose?1:0);
//#endif
    
    return nRet;
}

int CAnalDataSender::ReadResponse(char* pData, int& nLen)
{
    if(m_nSocketHandle == KInvalidSocketHandler || m_sState != CONNECTED)
    {
        QCLOGW("[ANL][SND]Invalid status while recv, sock %d, status %d", m_nSocketHandle, m_sState);
        return 0;
    }
    
    int nTime = qcGetSysTime();
    int nRet = QC_ERR_RETRY;
    
    //struct timeval tTimeout = {0, 200000};
    struct timeval tTimeout = {1, 0};
    nRet = WaitSocketReadBuffer(m_nSocketHandle, tTimeout);
    
    if(nRet > 0)
    {
        memset(pData, 0, nLen);
        nLen = (int)recv(m_nSocketHandle, pData, nLen, 0);
        if(nLen > 0)
        {
#ifdef _PRINTF_HTTP_DATA_
            QCLOGI("[ANL][SND]RESPONSE: %s", pData);
#endif
            return nLen;
        }
    }
    return 0;
}

int CAnalDataSender::WaitSocketWriteBuffer(int& aSocketHandle, timeval& aTimeOut)
{
    fd_set        fds;
    //timeval        tmWait = { 1, 0 };
    timeval        tmWait = { 0, 200000 };
    if(aTimeOut.tv_sec == 0 && aTimeOut.tv_usec < tmWait.tv_usec)
        tmWait.tv_usec = aTimeOut.tv_usec;

    int            ret = 0;
    int            nStartTime = qcGetSysTime();
    while (ret == 0)
    {
        if (qcGetSysTime() - nStartTime > (aTimeOut.tv_sec * 1000 + aTimeOut.tv_usec/1000))
            break;
        
        FD_ZERO(&fds);
        FD_SET(aSocketHandle, &fds);
        
        ret = select(aSocketHandle + 1, NULL, &fds, NULL, &tmWait);
        
        if (m_pBaseInst->m_bForceClose == true && ret <= 0)
        {
            QCLOGW("[ANL]Force exit wait write buffer, wait time %d", qcGetSysTime() - nStartTime);
            return QC_ERR_CANNOT_CONNECT;
        }
    }
    int err = 0;
    int errLength = sizeof(err);
    
    if (ret > 0 && FD_ISSET(aSocketHandle, &fds))
    {
        getsockopt(aSocketHandle, SOL_SOCKET, SO_ERROR, (char *)&err, (socklen_t*)&errLength);
        if (err != 0)
        {
            //SetStatusCode(err + CONNECT_ERROR_BASE);
            ret = -1;
        }
    }
    else if(ret < 0)
    {
        //SetStatusCode(errno + CONNECT_ERROR_BASE);
    }
    
    return (ret > 0) ? QC_ERR_NONE : ((ret == 0) ? QC_ERR_TIMEOUT : QC_ERR_CANNOT_CONNECT);
}

int CAnalDataSender::Stop()
{
    Interrupt();
    return QC_ERR_NONE;
}

void CAnalDataSender::UpdateServer(const char* pszDstServer)
{
    if(!pszDstServer)
        return;
    if(!strcmp(pszDstServer, m_szDstServer))
        return;
    Disconnect();
    
    m_bDNSReady = false;
    memset(m_szDstServer, 0, 1024);
    sprintf(m_szDstServer, "%s", pszDstServer);
    //QCLOGI("[ANL]Server is %s", m_szDstServer);
}

bool CAnalDataSender::IsDNSParsed()
{
    return m_bDNSReady;
}

void CAnalDataSender::Save(const char* pServerURL, int nURLLen, char* pRawData, int nDataLen)
{
    CAutoLock lock(&m_mtAnlRawList);
    
    AnlRawData* pData = new AnlRawData;
    memset(pData, 0, sizeof(AnlRawData));
    
    pData->server = new char[nURLLen+1];
    memset(pData->server, 0, nURLLen+1);
    memcpy(pData->server, pServerURL, nURLLen);
    
    pData->dataLen = nDataLen;
    pData->data = new char[pData->dataLen + 1];
    memset(pData->data, 0, pData->dataLen + 1);
    memcpy(pData->data, pRawData, nDataLen);
    
    //QCLOGI("[ANL]Save-> server: %s\ndata:%s", pData->server, pData->data);
    
    m_listAnlRaw.AddTail(pData);
}

void CAnalDataSender::Save(const char* pServerURL, char* pHeader, int nHeaderLen, char* pBody, int nBodyLen)
{
    CAutoLock lock(&m_mtAnlRawList);
    
    //QCLOGI("[TEST0]%s\n", pBody);
    AnlRawData* pData = new AnlRawData;
    memset(pData, 0, sizeof(AnlRawData));
    
    pData->server = new char[strlen(pServerURL)+1];
    memset(pData->server, 0, strlen(pServerURL)+1);
    strcpy(pData->server, pServerURL);
    
    pData->dataLen = nHeaderLen + nBodyLen;
    pData->data = new char[pData->dataLen + 1];
    memset(pData->data, 0, pData->dataLen + 1);
    memcpy(pData->data, pHeader, nHeaderLen);
    memcpy(pData->data+nHeaderLen, pBody, nBodyLen);
    
    m_listAnlRaw.AddTail(pData);
}

void CAnalDataSender::ReleaseAnlRawData(AnlRawData* pData)
{
    QC_DEL_A(pData->server);
    QC_DEL_A(pData->data);
    QC_DEL_P(pData);
}

void CAnalDataSender::ReleaseAnlRawList()
{
    CAutoLock lock(&m_mtAnlRawList);
    
    //QCLOGW("[ANL]Left %d", m_listAnlRaw.GetCount());
    
    AnlRawData* pAnal = m_listAnlRaw.RemoveHead ();
    while (pAnal != NULL)
    {
        //QCLOGI("[TEST1]%s\n", pAnal->data);
        ReleaseAnlRawData(pAnal);
        pAnal = m_listAnlRaw.RemoveHead ();
    }
}

int CAnalDataSender::PostData()
{
    int nTryCount = 0;
    int nRC = QC_ERR_NONE;
    AnlRawData* pAnal = NULL;
    
    {
        CAutoLock lock(&m_mtAnlRawList);
        pAnal = m_listAnlRaw.RemoveHead();
    }
    
    while(pAnal != NULL)
    {
        if(m_pBaseInst->m_bForceClose)
        {
            //ReleaseAnlRawData(pAnal);
            m_listAnlRaw.AddHead(pAnal);
            break;
        }
        
        UpdateServer(pAnal->server);
        
        //QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
    	nRC = PostData(pAnal->data, pAnal->dataLen);
        
        if(nRC != QC_ERR_NONE)
        {
            //QCLOGW("[ANL]Snd fail");
            {
//                CAutoLock lock(&m_mtAnlRawList);
//                m_listAnlRaw.AddHead(pAnal);
            }
            
            // HTTP POST failed(connect or send), make sure if network changed or not
            // 1. Network switches between IPv4 and IPv6
            // 2. Server's IP maybe not same on different network operator or CDN
			// The Clear function should be called in framework. Bang 2018-10-11
         //   if(m_pDNSCache)
         //       m_pDNSCache->Clear();
            if(nTryCount < 2)
            {
				qcSleepEx(100000, &m_pBaseInst->m_bForceClose);
                nTryCount++;
                //QCLOGW("[ANL]Snd fail, continue try %d", nTryCount);
                continue;
            }
        }
        else
        {
            if(!m_pBaseInst->m_bForceClose && !m_bUpdateSampleTime)
            {
                unsigned int nStatusCode = 404;
                int nErr = ParseResponseHeader(nStatusCode);
                if (nErr == QC_ERR_NONE)
                {
#ifdef _PRINTF_HTTP_DATA_
                    if(m_pRespBuff && m_nRespSize > 0)
                        QCLOGI("[ANL]Response: %s", m_pRespBuff);
#endif
                    if(nStatusCode == 200)
                    {
                        if(m_pRespBuff && m_nRespSize > 0)
                            UpdateTrackParam(m_pRespBuff, m_nRespSize);
                    }
                }
            }
        }

        //QCLOGI("[ANL]\n-------------------------------------------------------------------------------------------------------------------\n%s\n-------------------------------------------------------------------------------------------------------------------", pAnal->data);
        nTryCount = 0;
        ReleaseAnlRawData(pAnal);
        
        {
            CAutoLock lock(&m_mtAnlRawList);
            pAnal = m_listAnlRaw.RemoveHead();
        }
        qcSleep(5000);
    }
    
    //if((m_sState == CONNECTED || m_sState == CONNECTING) && (m_nSocketHandle != KInvalidSocketHandler))
    	Disconnect();
    
    return nRC;
}

int CAnalDataSender::OnWorkItem (void)
{
    if (qcGetSysTime() - m_nPostStartTime > 5000)
    {
        //QCLOGI("[ANL]I am working!!! %d", m_listAnlRaw.GetCount());
        PostData();
        m_nPostStartTime = qcGetSysTime();
    }
    
    if(!m_pBaseInst->m_bForceClose)
        qcSleep(5*1000);
    return 0;
}

void CAnalDataSender::UpdateTrackParam(char* pData, int nLen)
{
    if(nLen <= 0)
        return;
    
    char*    pBuff = const_cast<char*>(pData);
    int      nLength = nLen;
    int      nLineSize = 0;
    char*    pLine = NULL;
    
    while(GetLine (&pBuff, &nLength, &pLine, &nLineSize) == true)
    {
        //format:    {"reportInterval":120,"sampleInterval":60}
        if(pLine)
        {
            char* pStart = pLine + 2;
            if(!strncmp(pStart, "reportInterval", strlen("reportInterval")))
            {
                char szTmp[12];
                char* end = strchr(pStart, ',');
                char* start = strchr(pStart, ':');
                if(start && end)
                {
                    start++;
                    if((end - start) <= 0)
                        return;
                    memcpy(szTmp, start, end-start);
                    szTmp[end-start] = '\0';
                    m_nReportInterval = atoi(szTmp);
                    
                    start = strchr(end, ':');
                    end = strchr(end, '}');
                    
                    if(start && end)
                    {
                        start++;
                        if((end - start) <= 0)
                            return;
                        memcpy(szTmp, start, end-start);
                        szTmp[end-start] = '\0';
                        m_nSampleInterval = atoi(szTmp);
                        m_bUpdateSampleTime = true;
                    }
                    //QCLOGI("[ANL]Update time, %d, %d", m_nSampleInterval, m_nReportInterval);
                }
                break;
            }
        }
    }
}

bool CAnalDataSender::GetLine (char ** pBuffer, int* nLen, char** pLine, int* nLineSize)
{
    char *    pBegin = *pBuffer;
    char *    pEnd = *pBuffer;
    int        nChars = 0;
    
    *pLine = NULL;
    *nLineSize = 0;
    
    if(*nLen<=0)
        return false;
    
    while(*nLen > 0 && *pBegin != 0 && (*pBegin =='\r' || *pBegin=='\n' || *pBegin==' ' || *pBegin == '\t'))
    {
        pBegin++;
        (*nLen)--;
    }
    
    pEnd = pBegin;
    
    while((*nLen) > 0 && *pEnd!=0 && *pEnd!='\r' && *pEnd!='\n')
    {
        pEnd++;
        (*nLen)--;
        nChars++;
    }
    
    *pBuffer = pEnd;
    if(*nLen > 0)
    {
        (*pBuffer)++;
        (*nLen)--;
    }
    
    if(nChars > 0)
    {
        *pEnd = 0;
        *pLine = pBegin;
        *nLineSize = nChars;
        return true;
    }
    else
    {
        return false;
    }
}

void CAnalDataSender::GetReportParam(int& nSampleInterval, int& nReportInterval)
{
    nSampleInterval = m_nSampleInterval;
    nReportInterval = m_nReportInterval;
}

void CAnalDataSender::UpdateDNSServer(const char* pszServer)
{
    if (pszServer == NULL)
        strcpy(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, "");
    else
        strcpy(m_pBaseInst->m_pSetting->g_qcs_szDNSServerName, pszServer);
    if (m_pBaseInst->m_pDNSLookup != NULL)
        m_pBaseInst->m_pDNSLookup->UpdateDNSServer();
}

void CAnalDataSender::DeleteDumpFile(char* pszFileName)
{
    CFileIO io(m_pBaseInst);
    if(QC_ERR_NONE == io.Open(pszFileName, 0, QCIO_FLAG_WRITE))
    {
        io.Close();
    }
}

int CAnalDataSender::ReadRawData()
{
    //return 0;
    CAutoLock lock(&m_mtAnlRawList);
    
    int nTotalCount = 0;
    char cachePath[2048];
    memset(cachePath, 0, 2048);
    qcGetCachePath(NULL, cachePath, 2048);
    //qcGetAppPath(NULL, cachePath, 2048);
    
    if(strlen(cachePath) > 0)
    {
        strcat(cachePath, "lna.nq");
        CFileIO io(m_pBaseInst);
        //io.SetParam(QCIO_PID_FILE_KEY, (void*)"ANLANL");
        if(QC_ERR_NONE == io.Open(cachePath, 0, QCIO_FLAG_READ|QCIO_FLAG_WRITE))
        {
            int nFileSize = (int)io.GetSize();
            if(nFileSize <= 0)
            {
                io.Close();
                return 0;
            }
            
            char* data = new char[nFileSize];
            int nRet = io.Read((unsigned char*)data, nFileSize, true, 0);
            io.Close();
            if(nRet != QC_ERR_NONE)
            {
                delete []data;
                return 0;
            }
            
            DeleteDumpFile(cachePath);
        
            int 		headBytes = 2;
            int      	nLeft = nFileSize;
            char*    	pBuff = const_cast<char*>(data);
            char*     	pURL = NULL;
            int       	nURLLen = 0;
            char*    	pRawData = NULL;
            int      	nRawDataSize = 0;

            while(nLeft > headBytes)
            {
                //
                nURLLen = *(unsigned short*)pBuff;
                nLeft	-= 2;
                pBuff	+= 2;
                if(nURLLen >= nLeft)
                {
                    delete []data;
                    return nTotalCount;
                }
                pURL	= pBuff;
                nLeft	-= nURLLen;
                pBuff	+= nURLLen;
                pURL 	+= strlen("url=");
                nURLLen	-= strlen("url=");
                
                //
                if(nLeft <= headBytes)
                {
                    //QCLOGW("[ANL]Data error");
                    delete []data;
                    return nTotalCount;
                }
                nRawDataSize = *(unsigned short*)pBuff;
                pRawData	= pBuff + 2;
                nLeft		-= 2;
                pBuff		+= 2;
                if(nRawDataSize > nLeft)
                {
                    //QCLOGW("[ANL]Data error");
                    delete []data;
                    return nTotalCount;
                }
                nLeft    -= nRawDataSize;
                pBuff    += nRawDataSize;
                pRawData		+= strlen("data=");
                nRawDataSize  	-= strlen("data=");

                if(pURL && pRawData)
                {
                    Save(pURL, nURLLen, pRawData, nRawDataSize);
                    pURL = NULL;
                    pRawData = NULL;
                    nTotalCount++;
                    //QCLOGI("[ANL]Read-> url: %d, data: %d, total %d", nURLLen, nRawDataSize, nTotalCount);
                }
            }
            
            delete []data;
        }
    }
    
    return nTotalCount;
}

int CAnalDataSender::WriteRawData()
{
    CAutoLock lock(&m_mtAnlRawList);
    
    if(m_listAnlRaw.GetCount() <= 0)
        return 0;
    
    int nTotalCount = 0;
    char cachePath[2048];
    memset(cachePath, 0, 2048);
    qcGetCachePath(NULL, cachePath, 2048);
    //qcGetAppPath(NULL, cachePath, 2048);
    
    if(strlen(cachePath) > 0)
    {
        strcat(cachePath, "lna.nq");
        CFileIO io(m_pBaseInst);
        //io.SetParam(QCIO_PID_FILE_KEY, (void*)"ANLANL");
        
        if(QC_ERR_NONE == io.Open(cachePath, 0, QCIO_FLAG_WRITE|QCIO_FLAG_READ))
        {
            long long llFileSize = io.GetSize();
            if(llFileSize > 0)
                io.SetPos(llFileSize, QCIO_SEEK_BEGIN);

            int nCurrPos = 0;
            int nTotalSize = GetRawDataSize();
            unsigned char* pDump = new unsigned char[nTotalSize];
            
            AnlRawData* pAnal = m_listAnlRaw.RemoveHead ();
            while (pAnal != NULL)
            {
                unsigned short urlLen = strlen("url=") + (int)strlen(pAnal->server);
                memcpy(pDump+nCurrPos, (unsigned char*)&urlLen, 2);
                nCurrPos += 2;
                
                memcpy(pDump+nCurrPos, (unsigned char*)"url=", strlen("url="));
                nCurrPos += strlen("url=");
                
                memcpy(pDump+nCurrPos, (unsigned char*)pAnal->server, (int)strlen(pAnal->server));
                nCurrPos += (int)strlen(pAnal->server);

                unsigned short dataLen = strlen("data=") + pAnal->dataLen;
                memcpy(pDump+nCurrPos, (unsigned char*)&dataLen, 2);
                nCurrPos += 2;

                memcpy(pDump+nCurrPos, (unsigned char*)"data=", strlen("data="));
                nCurrPos += strlen("data=");

                memcpy(pDump+nCurrPos, (unsigned char*)pAnal->data, pAnal->dataLen);
                nCurrPos += pAnal->dataLen;

                nTotalCount++;
                //QCLOGI("[ANL]Write-> url: %d, data: %d, total %d", (int)strlen(pAnal->server), pAnal->dataLen, nTotalCount);
                ReleaseAnlRawData(pAnal);
                pAnal = m_listAnlRaw.RemoveHead ();
            }
            
            //int time = qcGetSysTime();
            io.Write(pDump, nTotalSize);
            io.Close();
            //QCLOGI("[ANL]Dump time %d, %d", qcGetSysTime() - time, nTotalSize - nCurrPos);
            delete []pDump;
        }
    }
    
    return nTotalCount;
}

int CAnalDataSender::GetRawDataSize()
{
    CAutoLock lock(&m_mtAnlRawList);
    
    int nTotalSize = 0;
    NODEPOS pos = m_listAnlRaw.GetHeadPosition();
    AnlRawData* pAnal = m_listAnlRaw.GetNext(pos);
    while (pAnal)
    {
        nTotalSize += 2;  					// head, 2 bytes
        nTotalSize += strlen("url=");		// flag
        nTotalSize += strlen(pAnal->server);
        
        nTotalSize += 2;                  	// head, 2 bytes
        nTotalSize += strlen("data=");		// flag
        nTotalSize += pAnal->dataLen;
        
        pAnal = m_listAnlRaw.GetNext(pos);
    }

    return nTotalSize;
}

