/**
* File : TTRtmpDownloadProxy.cpp
* Created on : 2015-9-25
* Author : kevin
* Copyright : Copyright (c) 2015 GoKu Ltd. All rights reserved.
* Description : CTTRtmpDownload 实现文件
*/


#include "TTRtmpDownload.h"

#include "GKOsalConfig.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#ifndef __TT_OS_WINDOWS__
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#endif
#include "TTUrlParser.h"
#include "TTBufferReaderProxy.h"
#include "TTHttpClient.h"
#include "TTCacheBuffer.h"
#include "GKNetWorkConfig.h"
#include "TTLog.h"
#include "GKTypedef.h"
#include "TTSysTime.h"
//#include "TTCommandId.h"
#include "TTSleep.h"
#include "TTRtmpInfoProxy.h"

#include <stdlib.h>
#include <math.h>

//#include <signal.h>		// to catch Ctrl-C

#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#include <io.h>
#include <fcntl.h>
#define	SET_BINMODE(f)	setmode(fileno(f), O_BINARY)
#else
#define	SET_BINMODE(f)
#endif

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE	2

#define DEF_TIMEOUT	30	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

//uint32_t nIgnoredFlvFrameCounter = 0;
//uint32_t nIgnoredFrameCounter = 0;
#define MAX_IGNORED_FRAMES	50

//RTMP_ctrlC for ctrl over

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

#ifndef __TT_OS_WINDOWS__
extern TTChar* GetHostMetaData();
#endif

static const TTChar* KBufDownloadThreadName = "RtmpDownloadThread";

extern TTBool  gUseProxy;

void AlarmSignalHandle(TTInt avalue)
{
}

CTTRtmpDownload::CTTRtmpDownload(CTTRtmpInfoProxy* aRtmpProxy)
:iUrl(NULL)
,iRtmpInfoProxy(aRtmpProxy)
,iReadStatus(ETTReadStatusNotReady)
,iStreamBufferingObserver(NULL)
,iBandWidthTimeStart(0)
,iBandWidthSize(0)
,iBandWidthData(0)
,iNetUseProxy(gUseProxy)
,iProxySize(0)
,iWSAStartup(0)
,iErrorCode(0)
,iCacheUrl(NULL)
,iPrefechComplete(ETTFalse)
,iFirstPacket(ETTTrue)
{
	iCritical.Create();	
	iSemaphore.Create();
	iBufferSize = 64*1024;
	iBuffer = (unsigned char *) malloc(iBufferSize);
	memset(&iConnectionTid, 0, sizeof(iConnectionTid));

#ifdef __TT_OS_WINDOWS__
   	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	iWSAStartup = WSAStartup( wVersionRequested, &wsaData );
#else
	struct sigaction act, oldact;
	act.sa_handler = AlarmSignalHandle;
	act.sa_flags = SA_NODEFER; 
	//sigaddset(&act.sa_mask, SIGALRM);
 	sigaction(SIGALRM, &act, &oldact);
	//signal(SIGPIPE, SIG_IGN);
#endif

}

CTTRtmpDownload::~CTTRtmpDownload()
{
	Close();
	iSemaphore.Destroy();
	iCritical.Destroy();

	if(iBuffer) {
		free(iBuffer);
	}

#ifdef __TT_OS_WINDOWS__
	WSACleanup();
#endif

}

TTChar* CTTRtmpDownload::Url()
{
	return iUrl;
}

TTInt CTTRtmpDownload::ConnectRtmpServer()
{
	int nStatus = RD_SUCCESS;

	//int bOverrideBufferTime = FALSE;	// if the user specifies a buffer time override this is true
	uint32_t bufferTime = DEF_BUFTIME;
	AVal parsedHost,parsedApp,parsedPlaypath;

	int port = -1;
	int protocol = RTMP_PROTOCOL_UNDEFINED;
	int bLiveStream = TRUE;	// is it a live stream? then we can't seek/resume
	int timeout = DEF_TIMEOUT;	// timeout connection after 120 seconds

	AVal hostname = { 0, 0 };
	AVal playpath = { 0, 0 };
	AVal subscribepath = { 0, 0 };
	AVal swfUrl = { 0, 0 };
	AVal tcUrl = { 0, 0 };
	AVal pageUrl = { 0, 0 };
	AVal app = { 0, 0 };
	AVal auth = { 0, 0 };
	AVal swfHash = { 0, 0 };
	AVal flashVer = { 0, 0 };
	AVal sockshost = { 0, 0 };

	uint32_t swfSize = 0;
	unsigned int parsedPort = 0;
	int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;
	//char *flvFile = fflv;

#ifndef __TT_OS_IOS__
	memset(&iRtmp, 0, sizeof(RTMP));
#endif

	iConnectionTid = pthread_self();

	RTMP_Init(&iRtmp);

	if (!RTMP_ParseURL(iUrl, &parsedProtocol, &parsedHost, &parsedPort,&parsedPlaypath, &parsedApp))
	{
		nStatus = RD_FAILED;
		goto _end;
	}
	else
	{
		if (!hostname.av_len)
			hostname = parsedHost;
		if (port == -1)
			port = parsedPort;
		if (playpath.av_len == 0 && parsedPlaypath.av_len)
		{
			playpath = parsedPlaypath;
		}
		if (protocol == RTMP_PROTOCOL_UNDEFINED)
			protocol = parsedProtocol;
		if (app.av_len == 0 && parsedApp.av_len)
		{
			app = parsedApp;
		}
	}

	if (hostname.av_len == 0)
	{
		LOGI("hostname is NULL ");
		return RD_FAILED;
	}
	if (playpath.av_len == 0)
	{
		LOGI("playpath is NULL");
		return RD_FAILED;
	}

	if (protocol == RTMP_PROTOCOL_UNDEFINED)
	{
		LOGI("protocol is NULL");
		protocol = RTMP_PROTOCOL_RTMP;
	}
	if (port == -1)
	{
		LOGI("port NULL");
		port = 0;
	}
	if (port == 0)
	{
		if (protocol & RTMP_FEATURE_HTTP)
			port = 80;
		else
			port = 1935;
	}

	if (tcUrl.av_len == 0)
	{
		char str[512] = { 0 };

		tcUrl.av_len = snprintf(str, 511, "%s://%.*s:%d/%.*s",
			RTMPProtocolStringsLower[protocol], hostname.av_len,
			hostname.av_val, port, app.av_len, app.av_val);
		tcUrl.av_val = (char *) malloc(tcUrl.av_len + 1);
		strcpy(tcUrl.av_val, str);
	}

	RTMP_SetupStream(&iRtmp, protocol, &hostname, port, &sockshost, &playpath,
		&tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
		&flashVer, &subscribepath, 0, 0, bLiveStream, timeout);

	RTMP_SetBufferMS(&iRtmp, bufferTime);

	if (!RTMP_Connect(&iRtmp, NULL))
	{
		nStatus = RD_FAILED;
		goto _end;
	}
    
    iRtmpInfoProxy->ConnectDone();

	if (!RTMP_ConnectStream(&iRtmp, 0))
	{
		nStatus = RD_FAILED;
		goto _end;
	}

_end:
	if (nStatus == RD_SUCCESS)
	{
		iRtmp.m_read.timestamp = 0;

		iRtmp.m_read.initialFrameType = 0;
		iRtmp.m_read.nResumeTS = 0;
		iRtmp.m_read.metaHeader = NULL; //use for resume
		iRtmp.m_read.initialFrame = NULL;
		iRtmp.m_read.nMetaHeaderSize = 0;//use for resume
		iRtmp.m_read.nInitialFrameSize = 0;
	}
	else{
		RTMP_Close(&iRtmp);
	}

	return nStatus;
}

TTInt CTTRtmpDownload::ReceiveNetData()
{
	TTInt nRead = 0;
	if(RTMP_IsConnected(&iRtmp))
		nRead = RTMP_Read(&iRtmp, (char*)iBuffer, iBufferSize);
	else{
		if (iErrorCode < 0)
			return iErrorCode;
		else
			return TTKErrDisconnected;
	}

	if (nRead > 0)
	{
        if (iFirstPacket && nRead >3 && iBuffer[0] =='F' && iBuffer[1] =='L' &&  iBuffer[2] =='V') {
            iFirstPacket = ETTFalse;
            nRead = 0;
        }
		return nRead;
	}
    else if (nRead == 0){
        if(iRtmp.m_read.status == RTMP_READ_TIMEOUT)
        {
            iRtmp.m_read.status = 0;
        }
        else if(iRtmp.m_read.status == RTMP_READ_CANCLE)
            nRead = TTKErrCancel;
        else if(iRtmp.m_read.status == RTMP_READ_PEER_DISCONNET)
            nRead = TTKErrServerTerminated;
        else
            nRead = TTKErrSessionClosed;//transmission over
    }
    else {
        if (iRtmp.m_read.status == RTMP_READ_ERROR)
            nRead = TTKErrArgument;
        else if(iRtmp.m_read.status == RTMP_READ_NOMEMORY)
            nRead = TTKErrNoMemory;
        else if(iRtmp.m_read.status == RTMP_READ_NETDATAERROR)
            nRead = TTKErrServerDataError;
        else if(iRtmp.m_read.status == RTMP_READ_CANCLE)
            nRead = TTKErrCancel;
        else if(iRtmp.m_read.status == RTMP_READ_PEER_DISCONNET)
            nRead = TTKErrServerTerminated;
    }
    
    if(iRtmp.m_nServerStreamEof)
        nRead = TTKErrSessionClosed;

	if (nRead < 0){
		iErrorCode = nRead;
		RTMP_Close(&iRtmp);
	}

    return nRead;
}

TTInt CTTRtmpDownload::WriteData(unsigned char * aData, int size)
{
    TTInt32 timeUs;//may has problem
    TTInt datasize;
	TTInt ret = 0;
    if(size < 11 || iRtmpInfoProxy == NULL)
        return TTKErrNotReady;

    datasize = (TTUint32(aData[1]<<16) | TTUint32(aData[2]<<8) | TTUint32(aData[3]));
    timeUs = (TTUint32(aData[4]<<16) | TTUint32(aData[5]<<8) | TTUint32(aData[6]));
    if (aData[0] == 0x08) {
        ret = iRtmpInfoProxy->AddAudioTag((TTUint8*)(aData+11),datasize,timeUs);
        //LOGI("A: t =%d, size =%d \n",timeUs,datasize);
    }
    else if(aData[0] == 0x09){
        ret = iRtmpInfoProxy->AddVideoTag((TTUint8*)(aData+11),datasize,timeUs);
        //LOGI("V: t =%d, size =%d \n",timeUs,datasize);
    }
    
    return size;
}

TTInt CTTRtmpDownload::Open(const TTChar* aUrl)
{
	GKASSERT(GKNetWorkConfig::getInstance() != NULL);
	GKASSERT(GKNetWorkConfig::getInstance()->getActiveNetWorkType() != EActiveNetWorkNone);
	iCancel = ETTFalse;
	iProxySize = 0;
	iSemaphore.Reset();
	iErrorCode = 0;
	iBandWidthData = 0;
	LOGI("CTTRtmpDownload::Open %s, gUseProxy %d", aUrl, gUseProxy);
	
	SAFE_FREE(iUrl);
	iUrl = (TTChar*) malloc(strlen(aUrl) + 1);
	strcpy(iUrl, aUrl);

	TTInt nErr = TTKErrNone;
	TTInt nConnectErr;
	//
	//#ifndef __TT_OS_WINDOWS__
	//    iHttpClient->SetHostMetaData(GetHostMetaData());
	//#endif

	if(iWSAStartup)
		return TTKErrCouldNotConnect;

	nConnectErr = ConnectRtmpServer();

	if (nConnectErr != TTKErrNone) 
	{
		SAFE_FREE(iUrl);
		return nConnectErr;
	}

	iReadStatus = ETTReadStatusReading;

	LOGE("CTTRtmpDownload::Open and begin to create thread.");
	
	if ((TTKErrNone	!= nErr) || (TTKErrNone != (nErr = iThreadHandle.Create(KBufDownloadThreadName, DownloadThreadProc, this, 0))))
	{
		iReadStatus = ETTReadStatusNotReady;
	}

	if (TTKErrNone != nErr) 
	{
		SAFE_FREE(iUrl);
	}

	LOGI("CTTRtmpDownload::Open return: %d", nErr);
	return nErr;	
}

TTInt CTTRtmpDownload::Close()
{
	iCritical.Lock();
	iReadStatus = ETTReadStatusToStopRead;
    iRtmp.m_cancle = ETTTrue;
	iCritical.UnLock();
	LOGI("CTTRtmpDownload Close");

	iThreadHandle.Close();

	SAFE_FREE(iUrl);
	SAFE_FREE(iCacheUrl);

	return TTKErrNone;
}

void CTTRtmpDownload::Cancel()
{
	iSemaphore.Signal();
	if(!iCancel) {
		iCancel = ETTTrue;
		iRtmp.m_cancle = ETTTrue;

#ifndef __TT_OS_WINDOWS__
		if (iConnectionTid > 0 && !pthread_equal(iConnectionTid, pthread_self()))
		{
			int pthread_kill_err = pthread_kill(iConnectionTid, 0);
			if((pthread_kill_err != ESRCH) && (pthread_kill_err != EINVAL))
			{
				pthread_kill(iConnectionTid, SIGALRM);
				LOGI("sent interrupt signal");
			}
		}
#endif
	}
}

void  CTTRtmpDownload::SetNetWorkProxy(TTBool aNetWorkProxy)
{
}

TTInt CTTRtmpDownload::ReConnectServer()
{
    TTInt nConnectErr = TTKErrNone;
	TTInt nConnectErrorCnt = 0;

    do 
    {
		nConnectErr = ConnectRtmpServer();
		if (nConnectErr == TTKErrNone || iCancel) 
		{
			break;
		}
        nConnectErrorCnt++;
		iSemaphore.Wait(KWaitIntervalMs*10);
    } while(nConnectErrorCnt < MAX_RECONNECT_COUNT);
    
	if (nConnectErr == TTKErrNone)
		iErrorCode = TTKErrNone;

    return nConnectErr;
}

TTUint CTTRtmpDownload::ProxySize()
{
	if(gUseProxy)
		return iProxySize;
	else
		return 0;
}

void* CTTRtmpDownload::DownloadThreadProc(void* aPtr)
{
	CTTRtmpDownload* pReaderProxy = reinterpret_cast<CTTRtmpDownload*>(aPtr);
	pReaderProxy->DownloadThreadProcL(NULL);
	return NULL;
}

void CTTRtmpDownload::DownloadThreadProcL(void* aPtr)
{
	//TTChar* pBuffer = new TTChar[KBUFFER_SIZE];
	if(iStreamBufferingObserver)
		iStreamBufferingObserver->PrefetchStart(iRtmp.m_ip);

#ifdef __TT_OS_ANDROID__
	nice(-1);
#endif
    
	//TTInt  nSlowDown = 0;
	TTInt  nReconnectNum = 0;
	TTInt  nZeroBuffer = 0;
	TTInt64	nNowTime = GetTimeOfDay();

	iBandWidthTimeStart = nNowTime;
	iBandWidthSize = 0;
	iBandWidthData = 0;
     
	while(!iThreadHandle.Terminating())
	{
		iCritical.Lock();
		if (iReadStatus == ETTReadStatusToStopRead)
		{
			iReadStatus = ETTReadStatusNotReady;
			iCritical.UnLock();
			break;
		}
		iCritical.UnLock();	

		if (iCancel)	
		{
			iCritical.Lock();
			iReadStatus = ETTReadStatusNotReady;
			iCritical.UnLock();
			break;
		}

		nNowTime = GetTimeOfDay();
		if(nNowTime >= iBandWidthTimeStart + 1000) {
			iBandWidthData = (iBandWidthData + iBandWidthSize*1000/(nNowTime - iBandWidthTimeStart)) >> 1;
			iBandWidthTimeStart = nNowTime;
			iBandWidthSize = 0;			
		}

		TTInt nReadSize = ReceiveNetData();
		if(nReadSize > 0) {
			iBandWidthSize += nReadSize;
		}
        
        if (nReadSize == 0)
        {
			nZeroBuffer++;
			iSemaphore.Wait(5);
			if(nZeroBuffer >= 1000) {
				nReadSize = -1;
			} else {
				continue;
			}
		}

		if (nReadSize < 0)
		{
			if (nReadSize == TTKErrSessionClosed)
			{
                iRtmpInfoProxy->EOFNotify();
				break;
			}
			else if (nReadSize == TTKErrNoMemory)
			{
				if(iStreamBufferingObserver)
					iStreamBufferingObserver->DownLoadException(nReadSize, 0, NULL);
				break;
			}
			else if(nReadSize == TTKErrCancel)
				break;


			if (ReConnectServer() == TTKErrNone)
			{
				continue;
			}

			nReconnectNum++;
			//LOGI("-----------HttpReaderProxy Read Error: %d, nReconnectNum %d", nReadSize, nReconnectNum);

			if(nReconnectNum > 20) {
				if(iStreamBufferingObserver) {
					char *pParam3 = NULL;
					pParam3 = inet_ntoa(*(struct in_addr*)&(iRtmp.m_ip));
					iStreamBufferingObserver->DownLoadException(nReadSize, 0, pParam3);
				}
				break;
			}
			continue;
		}

		iErrorCode = 0;

		iProxySize += nReadSize;

		if (!iPrefechComplete && iProxySize >= KILO*KILO)
		{
			//send PrefetchCompleted msg
			if(iStreamBufferingObserver)
				iStreamBufferingObserver->PrefetchCompleted();

			iPrefechComplete = ETTTrue;
		}

		//LOGI("nOffset %d, nReadSize %d", nOffset, nReadSize);

		nReconnectNum = 0;
		nZeroBuffer = 0;

		TTInt nWriteSize = WriteData(iBuffer,nReadSize);//addtag
        iRtmpInfoProxy->DiscardUselessBuffer();
		if (nWriteSize != nReadSize)
		{
			TTInt errorCode = TTKErrWrite;
			if (nWriteSize >=0)
				errorCode = TTKErrNoMemory;
			else if(errorCode != -1)
				errorCode = nWriteSize;
			//Send exception msg
			if(iStreamBufferingObserver)
					iStreamBufferingObserver->DownLoadException(errorCode, 0, NULL);

			iCritical.Lock();
			iCancel = true;
			iReadStatus = ETTReadStatusNotReady;
			iCritical.UnLock();

			LOGE("Write Buffer Error");
			break;
		}
        
	}
}

void CTTRtmpDownload::SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver)
{
	iStreamBufferingObserver = aObserver;
}

TTUint CTTRtmpDownload::GetHostIP()
{
	return iRtmp.m_ip;
}

TTUint CTTRtmpDownload::BandWidth()
{
	TTUint nBandWidth = 0;
	nBandWidth = iBandWidthData;
	if(nBandWidth < 1024)
		nBandWidth = 0;
	return nBandWidth;
}