/**
* File : GKRtmpPush.cpp
* Created on : 2015-11-11
* Copyright : Copyright (c) 2015 All rights reserved.
*/


#include "GKRtmpPush.h"
#include <string.h>
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
#include "TTLog.h"
#include "GKTypedef.h"
#include "TTSysTime.h"
#include "TTSleep.h"
#include "TTLog.h"
#include <stdlib.h>
#include <math.h>

#include "GKCollectCommon.h"

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

#ifdef __TT_OS_IOS__
#define MAX_TRY_CONNECT_NUM 17
#else
#define MAX_TRY_CONNECT_NUM 20
#endif

#define MAX_IGNORED_FRAMES		50
#define KWaitIntervalMs			10
#define MAX_RECONNECT_COUNT		4
#define NET_BADCONDITION        10
#define MAX_BUFFERING_SIZE      50


#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

static const TTChar* KBufDownloadThreadName = "RtmpPushThread";


void AlarmSignalHandler(TTInt avalue)
{
}

CGKRtmpPush::CGKRtmpPush(IGKMsgObserver* aObserver)
:iErrorCode(0)
,iCacheUrl(NULL)
,iUrl(NULL)
,mUrlPath(NULL)
,m_aacBufferConfig(NULL)
,m_aacConfigLength(0)
,m_spsBuffer(NULL)
,m_spsLength(0)
,m_ppsBuffer(NULL)
,mOpenStatus(ETTFalse)
,mClose(ETTFalse)
,mFirstConnect(ETTTrue)
,m_ppsLength(0)
,mReConnectMsg(0)
,mObserver(aObserver)
,iAAcConfigSet(ETTFalse)
,mCollectType(0)
,mCollectStart(0)
,mRecordTime(0)
,mRecordFLV(NULL)
,iRecordPath(NULL)
,mReConnecttimes(0)
,mConnectErrorCode(0)
,mlastMonitor(0)
,mVtimes(0)
,mAtimes(0)
{
	iCritical.Create();	
	mCtritConfig.Create();	
	iSemaphore.Create();
	iMsgCritical.Create();	
	iBufferSize = 64*1024;
	iBuffer = (unsigned char *) malloc(iBufferSize);
	memset(&iConnectionTid, 0, sizeof(iConnectionTid));

#ifndef __TT_OS_IOS__
	mVCapProcess = new CTTVCapProcess();

	mSendVideo.pObserver = OnVideoSend;
	mSendVideo.pUserData = this;

	mVCapProcess->SetObserver(&mSendVideo);

	mACapProcess = new TTAudioCapture(); 
	mSendAideo.pObserver = OnAudioSend;
	mSendAideo.pUserData = this;
	mACapProcess->SetObserver(&mSendAideo);
#endif
	mHandleThread = new TTEventThread("TTHandle Thread");

	mHandleThread->start();

#ifdef __TT_OS_WINDOWS__

#else
	struct sigaction act, oldact;
	act.sa_handler = AlarmSignalHandler;
	act.sa_flags = SA_NODEFER; 
	//sigaddset(&act.sa_mask, SIGALRM);
 	sigaction(SIGALRM, &act, &oldact);
	//signal(SIGPIPE, SIG_IGN);
#endif

}

CGKRtmpPush::~CGKRtmpPush()
{
	Close();
	iSemaphore.Destroy();
	iCritical.Destroy();
	mCtritConfig.Destroy();
	iMsgCritical.Destroy();	
	SAFE_DELETE(mHandleThread);
#ifndef __TT_OS_IOS__
	SAFE_DELETE(mVCapProcess);
	SAFE_DELETE(mACapProcess);
#endif
	mBufferManager.release();

	if(iBuffer) {
		free(iBuffer);
	}
    
    SAFE_FREE(m_spsBuffer);
    SAFE_FREE(m_ppsBuffer);
    SAFE_FREE(mUrlPath);
    SAFE_FREE(m_aacBufferConfig);
    
    if (iRecordPath){
        free(iRecordPath);
        iRecordPath = NULL;
    }
}

void CGKRtmpPush::Setbitrate(int bitrate)
{
    mFlowMonitor.SetBitrate(bitrate);
}

TTInt CGKRtmpPush::postPublishSourceEvent (TTInt  nDelayTime)
{
	if (mHandleThread == NULL)
		return TTKErrNotFound;

	mHandleThread->cancelEventByType(EEventLoad, false);
	TTBaseEventItem * pEvent = mHandleThread->getEventByType(EEventLoad);
	if (pEvent == NULL)
		pEvent = new GKCRtmpEvent(this, &CGKRtmpPush::onPublishSource, EEventLoad);
	mHandleThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}

TTInt CGKRtmpPush::onPublishSource(TTInt nMsg, TTInt nVar1)
{
	return Open(mUrlPath);
}

TTInt CGKRtmpPush::SetPublishSource(const TTChar* aUrl)
{
	SAFE_FREE(mUrlPath);
    mUrlPath = (TTChar*) malloc((strlen(aUrl) + 1) * sizeof(TTChar));
    strcpy(mUrlPath, aUrl);

	postPublishSourceEvent(0);
	return 0;
}

TTInt CGKRtmpPush::Stop()
{
#ifdef __TT_OS_ANDROID__  
	mVCapProcess->Pause();
	mACapProcess->Pause();
	
	if (mCollectType != 0)
    {
        iCritical.Lock();
        if (mRecordFLV){
            fclose(mRecordFLV);
            mRecordFLV = NULL;
        }
        iCritical.UnLock();
        LOGE("CTTRtmpPush::Stop");
        
        return mCollectType;
    }
#endif

	Cancel();

	iMsgCritical.Lock();
	mObserver = NULL;
	iMsgCritical.UnLock();
    
	//postStopEvent(0);
    Close();
    
    return 0;
}

TTInt CGKRtmpPush::postStopEvent(TTInt  nDelayTime)
{
	if (mHandleThread == NULL)
		return TTKErrNotFound;

	TTBaseEventItem * pEvent = mHandleThread->cancelEventByType(EEventClose, false);
	mHandleThread->cancelEventByType(EEventLoad, false);
    mHandleThread->cancelEventByType(EEventFlowMonitor, false);

	pEvent = mHandleThread->getEventByType(EEventClose);
	if (pEvent == NULL)
		pEvent = new GKCRtmpEvent (this, &CGKRtmpPush::onStop, EEventClose);
	mHandleThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}

TTInt CGKRtmpPush::postCongestionEvent(TTInt  nDelayTime)
{
    if (mHandleThread == NULL)
        return TTKErrNotFound;
    
    TTBaseEventItem * pEvent = mHandleThread->cancelEventByType(EEventNetCongestion, false);
    
    pEvent = mHandleThread->getEventByType(EEventNetCongestion);
    if (pEvent == NULL)
        pEvent = new GKCRtmpEvent (this, &CGKRtmpPush::onNetCongestion, EEventNetCongestion);
    mHandleThread->postEventWithDelayTime (pEvent, nDelayTime);
    
    return TTKErrNone;
}

TTInt CGKRtmpPush::postFlowMonitorEvent(TTInt  nDelayTime)
{
   /*
    if (mHandleThread == NULL)
        return TTKErrNotFound;
    
    TTBaseEventItem * pEvent = mHandleThread->cancelEventByType(EEventFlowMonitor, false);
    
    pEvent = mHandleThread->getEventByType(EEventFlowMonitor);
    if (pEvent == NULL)
        pEvent = new TTCRtmpEvent (this, &CTTRtmpPush::onFlowMonitor, EEventFlowMonitor);
    mHandleThread->postEventWithDelayTime (pEvent, nDelayTime);
    */
    return TTKErrNone;
}


TTInt CGKRtmpPush::onStop(TTInt nMsg, TTInt nVar1)
{
	return Close();  
}

TTInt  CGKRtmpPush::onNetCongestion(TTInt nMsg, TTInt nVar1)
{
    NotifyEvent(ENotifyNetBadCondition, 0, 0);
    return 0;
}

TTInt CGKRtmpPush::onFlowMonitor(TTInt nMsg, TTInt nVar1)
{
  /*  int bitrate = 0;
    int FlowValue = 0;
    if (mOpenStatus) {
#ifdef __TT_OS_ANDROID__
        int FlowValue = mObserver->GetMobileTx();
#endif
        bitrate = mFlowMonitor.CheckNetworkflow(FlowValue);
        
        if (bitrate > 0) {
            NotifyEvent(ENotifyResetEncoder,bitrate, 0);
        }
    }
    postFlowMonitorEvent(6000);*/
    return 0;
}

TTInt CGKRtmpPush::ConnectRtmpServer()
{
	int ret = TTKErrNone;
    
    GKPushNotifyMsg NotifyMsg;

	memset(&iRtmp, 0, sizeof(RTMP));

	iConnectionTid = pthread_self();

	RTMP_Init(&iRtmp);

	LOGE("Setup url:%s",iUrl);
    
    /*NotifyMsg = ENotifyOpenStart;
    if (mReConnectMsg) {
        NotifyMsg = ENotifyReconn_OpenStart;
    }

	NotifyEvent(NotifyMsg,0, 0);*/

	if (RTMP_SetupURL(&iRtmp,iUrl) == 0) {
    	   LOGE("RTMP_SetupURL() failed!");
		   NotifyEvent(ENotifyUrlParseError,0, 0);
    	   return TTKErrGeneral;
	}

	RTMP_EnableWrite(&iRtmp);

	ret = RTMP_Connect(&iRtmp, NULL);
	if (TTKErrNone != ret)
	{
		LOGE("RTMP_Connect() failed!");
        mReConnecttimes++;
        mConnectErrorCode = GetErrorCode();
        /*NotifyMsg = ENotifySokcetConnectFailed;
        if (mReConnectMsg) {
            NotifyMsg = ENotifyReconn_SokcetConnectFailed;
        }

        NotifyEvent(NotifyMsg, ret, GetErrorCode());*/

		goto _end;
	}

	if (!RTMP_ConnectStream(&iRtmp, 0))
	{
		LOGE("RTMP_ConnectStream() failed!");
        mReConnecttimes++;
		mConnectErrorCode = 256;
        /*NotifyMsg = ENotifyStreamConnectFailed;
        if (mReConnectMsg) {
            NotifyMsg = ENotifyReconn_StreamConnectFailed;
        }

		NotifyEvent(NotifyMsg,0, 0);*/

		goto _end;
	}
    
    NotifyEvent(ENotifyServerIP, iRtmp.m_ip, 0);

_end:
	if (ret == TTKErrNone)
	{
        NotifyMsg = ENotifyOpenSucess;
        /*if (mReConnectMsg) {
            NotifyMsg = ENotifyReconn_OpenSucess;
         }*/
		
		NotifyEvent(NotifyMsg,mReConnecttimes, mConnectErrorCode);
        
        mReConnecttimes = 0;
        mConnectErrorCode = 0;

	  	LOGE("connect is ok");
	}
	else{
		RTMP_Close(&iRtmp);
	}

	return ret;
}

TTInt CGKRtmpPush::Open(const TTChar* aUrl)
{
	iCancel = ETTFalse;
	iSemaphore.Reset();
	iErrorCode = 0;
	
	SAFE_FREE(iUrl);
	iUrl = (TTChar*) malloc(strlen(aUrl) + 1);
	strcpy(iUrl, aUrl);

	TTInt nErr = TTKErrNone;
	iAAcConfigSet = 0;

	mOpenStatus = ETTFalse;
    mClose = ETTFalse;

	LOGE("CTTRtmpPush::Open and begin to create thread.%s",iUrl);
	
	if (TTKErrNone != (nErr = iThreadHandle.Create(KBufDownloadThreadName, PushThreadProc, this, 0)))
	{
	}

	if (TTKErrNone != nErr) 
	{
		SAFE_FREE(iUrl);
	}
    
    mReConnectMsg = 0;
	LOGI("CTTRtmpPush::Open return: %d", nErr);
    
    //postFlowMonitorEvent(6000);
	return nErr;	
}

TTInt CGKRtmpPush::Close()
{
	if (mClose)
		return TTKErrNone;
    mHandleThread->stop();

	iCritical.Lock();
    iRtmp.m_cancle = ETTTrue;
	iCritical.UnLock();
	LOGI("CTTRtmpPush Close");

	iThreadHandle.Close();
#ifndef __TT_OS_IOS__
	mVCapProcess->Close();
#endif
	SAFE_FREE(iUrl);
	SAFE_FREE(iCacheUrl);

	CloseFLV();

    mOpenStatus = ETTFalse;
    
	mClose = ETTTrue;
    mReConnectMsg = 0;

	return TTKErrNone;
}

void CGKRtmpPush::Cancel()
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

void CGKRtmpPush::TransferVdieoData(CGKBuffer* pbuffer)
{
	if (pbuffer == NULL || iCancel || !mOpenStatus)
		return;

	TTInt ret = 0;
	CGKBuffer*  aBuffer = mBufferManager.dequeue(MODE_VIDEO);
	if(aBuffer == NULL)
		return;
	
	if (aBuffer->nPreAllocSize >= pbuffer->nSize)
	{
		memcpy(aBuffer->pBuffer, pbuffer->pBuffer, pbuffer->nSize);
		aBuffer->Tag = MODE_VIDEO;
		aBuffer->nSize = pbuffer->nSize;
		aBuffer->llTime = pbuffer->llTime;
		aBuffer->nFlag =  pbuffer->nFlag;
		ret = mBufferManager.queue(aBuffer, MODE_MIX);
	}
	else{
		SAFE_FREE(aBuffer->pBuffer);
		aBuffer->pBuffer = (TTPBYTE)malloc(pbuffer->nSize);
		if (aBuffer->pBuffer){
			memcpy(aBuffer->pBuffer, pbuffer->pBuffer, pbuffer->nSize);
			aBuffer->Tag = MODE_VIDEO;
			aBuffer->nSize = pbuffer->nSize;
			aBuffer->llTime = pbuffer->llTime;
			aBuffer->nFlag =  pbuffer->nFlag;
			aBuffer->nPreAllocSize = pbuffer->nSize;
			ret = mBufferManager.queue(aBuffer, MODE_MIX);
		}
	}

    if (ret < 0)
        mBufferManager.queue(aBuffer,MODE_VIDEO);
    else if (ret > 0){
        postCongestionEvent(0);
    }
}

#ifdef __TT_OS_ANDROID__  
//android hardencode use
void CGKRtmpPush::TransferVdieoData(TTPBYTE pdata, TTInt size, TTUint32 pts)
{
	if (pdata == NULL || size == 0 || iCancel || !mOpenStatus)
		return;
	
	TTInt ret = 0;
	CTTBuffer*  aBuffer = mBufferManager.dequeue(MODE_VIDEO);
	if(aBuffer == NULL)
		return;

	TTInt type;
    TTInt nFlag;
	TTPBYTE buf = pdata;
	if (buf[2] == 0x00) { /*00 00 00 01*/
		size -= 4;
		buf += 4;
	} else if (buf[2] == 0x01){ /*00 00 01*/
		size -= 3;
		buf += 3;
	}

	type = buf[0]&0x1f;

	if (type == NAL_SLICE_IDR){
		nFlag = TT_FLAG_BUFFER_KEYFRAME;
	}
	else
		nFlag = 0;

	if (aBuffer->nPreAllocSize >= size+4)
	{
		aBuffer->pBuffer[0] = (size & 0xff000000) >>24;
		aBuffer->pBuffer[1] = (size & 0x00ff0000) >>16;
		aBuffer->pBuffer[2] = (size & 0x0000ff00) >>8;
		aBuffer->pBuffer[3] = (size & 0x000000ff);
		memcpy(aBuffer->pBuffer+4, buf, size);
		aBuffer->Tag = MODE_VIDEO;
		aBuffer->nSize =  size+4;
		aBuffer->llTime = pts;
		aBuffer->nFlag  = nFlag;
		ret = mBufferManager.queue(aBuffer, MODE_MIX);
	}
	else{
		SAFE_FREE(aBuffer->pBuffer);
		aBuffer->pBuffer = (TTPBYTE)malloc(size+4);
		if (aBuffer->pBuffer){
			aBuffer->pBuffer[0] = (size & 0xff000000) >>24;
			aBuffer->pBuffer[1] = (size & 0x00ff0000) >>16;
			aBuffer->pBuffer[2] = (size & 0x0000ff00) >>8;
			aBuffer->pBuffer[3] = (size & 0x000000ff);
			memcpy(aBuffer->pBuffer+4, buf, size);
			aBuffer->Tag = MODE_VIDEO;
			aBuffer->nSize = size+4;
			aBuffer->llTime = pts;
			aBuffer->nFlag  = nFlag;
			aBuffer->nPreAllocSize = size+4;
			ret = mBufferManager.queue(aBuffer, MODE_MIX);
		}
	}

	if (ret < 0)
		mBufferManager.queue(aBuffer,MODE_VIDEO);
	else if (ret > 0){
		postCongestionEvent(0);
	}
}


void CGKRtmpPush::SetVideoConfig(TTPBYTE buf, TTInt len)
{
	if (buf == NULL || len == 0)
		return;

	TTPBYTE sps = NULL;
	TTPBYTE pps = NULL;
	TTPBYTE tmp = NULL;
	TTInt spslen = 0;
	TTInt ppslen = 0;
	TTInt nalLength = 0;
	TTInt ret = 0;

	//handle ?
	if (buf[2] == 0x00) { /*00 00 00 01*/
		nalLength = 4;
	} else if (buf[2] == 0x01){ /*00 00 01*/
		nalLength = 3;
	}

	sps = buf + nalLength;
	tmp = sps;
	for(int i = nalLength; i< (len-nalLength-1); i++)
	{
		tmp = &(buf[i]);
		if (tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 1){
			if ((tmp[3] & 0x1f) == NAL_PPS){
				pps = tmp+3;
				spslen = pps - sps - nalLength;
				break;
			}
		}
		else if(tmp[0] == 0 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 1){
			if ((tmp[4] & 0x1f) == NAL_PPS){
				pps = tmp+4;
				spslen = pps - sps - nalLength;
				break;
			}
		}
	}
	if (pps){
		ppslen = len - spslen - 2*nalLength;
	}

	if (sps && pps && ppslen >0 && spslen >0)
	{
		SAFE_FREE(m_spsBuffer);
		m_spsBuffer = (TTPBYTE)malloc(spslen);
		memcpy(m_spsBuffer,sps,spslen);
		m_spsLength = spslen;

		SAFE_FREE(m_ppsBuffer);
		m_ppsBuffer = (TTPBYTE)malloc(ppslen);
		memcpy(m_ppsBuffer,pps,ppslen);
		m_ppsLength = ppslen;

		//LOGE("SetVideoConfig,buffer len = %d",len);
	}

}

void CGKRtmpPush::GetLastPic(int* prgb,TTInt size)
{
	mVCapProcess->YUV2RGB(prgb, size);
}

#endif


void CGKRtmpPush::TransferVdieoData(TTPBYTE pdata, TTInt size, TTUint32 pts, TTInt nFlag)
{
	if (pdata == NULL || size == 0 || iCancel || !mOpenStatus)
		return;
	
	TTInt ret = 0;
	CGKBuffer*  aBuffer = mBufferManager.dequeue(MODE_VIDEO);
	if(aBuffer == NULL)
		return;
	
	if (aBuffer->nPreAllocSize >= size)
	{
		memcpy(aBuffer->pBuffer, pdata, size);
		aBuffer->Tag = MODE_VIDEO;
		aBuffer->nSize = size;
		aBuffer->llTime = pts;
		aBuffer->nFlag  = nFlag;
		ret = mBufferManager.queue(aBuffer, MODE_MIX);
	}
	else{
		SAFE_FREE(aBuffer->pBuffer);
		aBuffer->pBuffer = (TTPBYTE)malloc(size);
		if (aBuffer->pBuffer){
			memcpy(aBuffer->pBuffer, pdata, size);
			aBuffer->Tag = MODE_VIDEO;
			aBuffer->nSize = size;
			aBuffer->llTime = pts;
			aBuffer->nFlag  = nFlag;
			aBuffer->nPreAllocSize = size;
			ret = mBufferManager.queue(aBuffer, MODE_MIX);
		}
	}

	if (ret < 0)
		mBufferManager.queue(aBuffer,MODE_VIDEO);
    else if (ret > 0){
        postCongestionEvent(0);
    }
}

void CGKRtmpPush::TransferAudioData(TTPBYTE pdata, TTInt size, TTUint32 pts)
{
	if (pdata == NULL || size == 0 || iCancel || !mOpenStatus)
		return;

	TTInt ret = 0;
	CGKBuffer*  aBuffer = mBufferManager.dequeue(MODE_AUDIO);
	if(aBuffer == NULL)
		return;
	
	if (aBuffer->nPreAllocSize >= size)
	{
		memcpy(aBuffer->pBuffer, pdata, size);
		aBuffer->Tag = MODE_AUDIO;
		aBuffer->nSize = size;
		aBuffer->llTime = pts;
		ret = mBufferManager.queue(aBuffer, MODE_MIX);
	}
	else{
		SAFE_FREE(aBuffer->pBuffer);
		aBuffer->pBuffer = (TTPBYTE)malloc(size);
		if (aBuffer->pBuffer){
			memcpy(aBuffer->pBuffer, pdata, size);
			aBuffer->Tag = MODE_AUDIO;
			aBuffer->nSize = size;
			aBuffer->llTime = pts;
			aBuffer->nPreAllocSize = size;
			ret = mBufferManager.queue(aBuffer, MODE_MIX);
		}
	}

	if (ret < 0)
		mBufferManager.queue(aBuffer,MODE_AUDIO);
    else if (ret > 0){
        postCongestionEvent(0);
    }
}

void CGKRtmpPush::SetAudioConfig(TTPBYTE pdata, TTInt size)
{
	if (pdata == NULL || size == 0)
		return;

	mCtritConfig.Lock();
	SAFE_FREE(m_aacBufferConfig);
	m_aacBufferConfig = (TTPBYTE)malloc(size);
	if (m_aacBufferConfig)
	{
		memcpy(m_aacBufferConfig, pdata, size);
		m_aacConfigLength = size;
	}
	mCtritConfig.UnLock();
}

void CGKRtmpPush::SetVideoConfig(TTPBYTE sps, TTInt spslen, TTPBYTE pps, TTInt ppslen)
{
    if (sps == NULL || pps == NULL || spslen<=0 || ppslen<=0 ) {
        return;
    }
	mCtritConfig.Lock();
	SAFE_FREE(m_spsBuffer);
    m_spsBuffer = (TTPBYTE)malloc(spslen);
    memcpy(m_spsBuffer,sps,spslen);
    m_spsLength = spslen;
    
	SAFE_FREE(m_ppsBuffer);
    m_ppsBuffer = (TTPBYTE)malloc(ppslen);
    memcpy(m_ppsBuffer,pps,ppslen);
    m_ppsLength = ppslen;
	mCtritConfig.UnLock();
}

void CGKRtmpPush::SetVideoPps(TTPBYTE buf, TTInt len)
{
	if (buf && len >0 )
	{
		if (buf[2] == 0x00) { /*00 00 00 01*/
			buf += 4;
			len -= 4;
		} else if (buf[2] == 0x01){ /*00 00 01*/
			buf += 3;
			len -= 3;
		}
		mCtritConfig.Lock();
		if (m_ppsLength != len)
		{
			SAFE_FREE(m_ppsBuffer);
			m_ppsBuffer = (TTPBYTE)malloc(len);
			m_ppsLength = len;
		}
		memcpy(m_ppsBuffer,buf,len);
		mCtritConfig.UnLock();

		//LOGE("call SetVideoPps,buffer len = %d",len);
	}
}

void CGKRtmpPush::SetVideoSps(TTPBYTE buf, TTInt len)
{
	if (buf && len >0 )
	{
		if (buf[2] == 0x00) { /*00 00 00 01*/
			buf += 4;
			len -= 4;
		} else if (buf[2] == 0x01){ /*00 00 01*/
			buf += 3;
			len -= 3;
		}

		mCtritConfig.Lock();
		if (m_spsLength != len)
		{
			SAFE_FREE(m_spsBuffer);
			m_spsBuffer = (TTPBYTE)malloc(len);
			m_spsLength = len;
		}
		memcpy(m_spsBuffer,buf,len);
		mCtritConfig.UnLock();

		//LOGE("call SetVideoConfig,buffer len = %d",len);
	}
}

TTInt CGKRtmpPush::ReConnectServer()
{
	TTInt nConnectErr = TTKErrNone;
	TTInt nConnectErrorCnt = 0;
#ifdef NET_SEND
    do 
    {
		nConnectErr = ConnectRtmpServer();
        mReConnectMsg = 1;
		if (nConnectErr == TTKErrNone || iCancel) 
		{
			break;
		}
        nConnectErrorCnt++;
		iSemaphore.Wait(KWaitIntervalMs*25);
    } while(nConnectErrorCnt < MAX_RECONNECT_COUNT);
    
	if (nConnectErr == TTKErrNone)
		iErrorCode = TTKErrNone;
#endif
    return nConnectErr;
}

void* CGKRtmpPush::PushThreadProc(void* aPtr)
{
	CGKRtmpPush* pReaderProxy = reinterpret_cast<CGKRtmpPush*>(aPtr);
	pReaderProxy->PushThreadProcL(NULL);
	return NULL;
}

void CGKRtmpPush::PushThreadProcL(void* aPtr)
{
#ifdef __TT_OS_ANDROID__
	nice(-1);
#endif
    
	TTInt  nReconnectNum = 0;
	//TTInt  nZeroBuffer = 0;
	//TTInt64	nNowTime = GetTimeOfDay();
	CGKBuffer* aBuffer;
	TTInt ret = 0;
	OpenFLV();
     
	while(!iThreadHandle.Terminating())
	{
		if (iCancel){
			break;
		}

		if (!mOpenStatus)
		{
			ret = ReConnectServer();
			if (ret == 0){
				nReconnectNum = 0;
				mOpenStatus = ETTTrue;
				if (mFirstConnect){
					mFirstConnect = ETTFalse;
#ifndef __TT_OS_IOS__
					mVCapProcess->Start();
#endif
				}
                
                mVtimes = 0;
                mAtimes = 0;
			}
			else{
				nReconnectNum++;
				if (nReconnectNum < MAX_TRY_CONNECT_NUM){
					//iSemaphore.Wait(KWaitIntervalMs*50);
				}
				else{
					NotifyEvent(ENotifyNetReconnectUpToMax,0, 0);
					break;
				}
				continue;
			}
		}

		aBuffer = mBufferManager.getBuffer();
		if (aBuffer){
			if (aBuffer->Tag == MODE_AUDIO){					  
				//LOGE("send aac  [%d]",aBuffer->nSize);
				ret = SendAACData(aBuffer);
				if (ret == -1)
				  LOGE("send_audio failed ")
                else
                    mAtimes++;
			}
			else if(aBuffer->Tag == MODE_VIDEO){
				//LOGE("send video , [%d]",aBuffer->nSize);
				ret = SendVideoData(aBuffer);
				if (ret == -1)
				  LOGE("send_video failed ")
                else
                    mVtimes++;
			}

			if (ret != TTKErrNone )
			{
				NotifyEvent(ENotifyTransferError, 0, GetErrorCode());

				//if(mBufferManager.getBufferCount() > MAX_BUFFERING_SIZE)
				//	NotifyEvent(ENotifyNetBadCondition, 0, 0);

				ret = 0;
				mBufferManager.clear();
				RTMP_Close(&iRtmp);
				mOpenStatus = ETTFalse;
			}
			else{
                if (aBuffer->llTime - mlastMonitor > 30000) {
                 
                    if (mVtimes > 16 + mAtimes) {
                        NotifyEvent(ENotifyAVPushException,mVtimes, mAtimes);
                        //LOGE("-- push av error!--");
                    }

                    mlastMonitor = aBuffer->llTime;
                    mVtimes = 0;
                    mAtimes = 0;
                }
				mBufferManager.eraseBuffer(aBuffer->llTime,aBuffer->Tag);
			}
		}
		else{
			//run out of data ,need wait
			//LOGE("--wait 20 !--");
			iSemaphore.Wait(20);
		}
	}
#ifdef NET_SEND
	if(iCancel)
		RTMP_Close(&iRtmp);
#endif
    
}


TTInt CGKRtmpPush::SendVideoConfig(int pts)
{
	RTMPPacket * packet;
	unsigned char * body;
	int i;
	int ret = 0;

	if (m_spsBuffer== NULL || m_ppsBuffer== NULL || m_spsLength<=0 || m_ppsLength<=0)
		return -1;

	mCtritConfig.Lock();
	unsigned char *sps = m_spsBuffer;
	int sps_len = m_spsLength;
	unsigned char *pps = m_ppsBuffer;
	int pps_len = m_ppsLength;

	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	memset(packet,0,RTMP_HEAD_SIZE);

	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;

	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i], pps, pps_len);
	i +=  pps_len;

	mCtritConfig.UnLock();

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = pts;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = iRtmp.m_stream_id;

#ifdef DUMP_FLV
	WriteFLVTag(packet);
#endif
#ifdef NET_SEND
	/*调用发送接口*/
	if(!RTMP_SendPacket(&iRtmp,packet,TRUE))
		ret = -1;
#endif
	free(packet);    

	return ret;
}


TTInt CGKRtmpPush::SendVideoData(CGKBuffer* aBuffer)//界定符不发送
{
	RTMPPacket * packet;
	unsigned char * body;
	int ret = 0;

	unsigned char * buf = aBuffer->pBuffer;
	int len = aBuffer->nSize;
	TTUint32 pts = aBuffer->llTime;

	if (aBuffer->nFlag == TT_FLAG_BUFFER_KEYFRAME ){
		ret = SendVideoConfig(pts);
		//LOGE("send video spec %d",ret);
		if (ret != 0)
			return ret;
	}

	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+9);
	memset(packet,0,RTMP_HEAD_SIZE);
 
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = len + 5; //5 : video tag 5byte ,len contain packetlength(4byte)
 
	/*send video packet*/
	body = (unsigned char *)packet->m_body;
	memset(body,0,len+5);
 
	/*key frame*/
	body[0] = 0x27;
	if (aBuffer->nFlag == TT_FLAG_BUFFER_KEYFRAME)
	{
	    body[0] = 0x17;
	}
 
	body[1] = 0x01;   /*nal unit*/
	body[2] = 0x00;
	body[3] = 0x00;
	body[4] = 0x00;

	/*copy data*/
	memcpy(&body[5],buf,len);
 
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nInfoField2 = iRtmp.m_stream_id;
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = pts;
    
    //cs
    //LOGE("---V.pts = %d",pts);

#ifdef DUMP_FLV
	WriteFLVTag(packet);
#endif
#ifdef NET_SEND
	/*调用发送接口*/
	if(!RTMP_SendPacket(&iRtmp,packet,TRUE))
			ret = -1;
#endif
	free(packet);
	return ret;
}

TTInt CGKRtmpPush::SendAACConfig(int pts)
{
    RTMPPacket * packet;
    unsigned char * body;
    int len;
    int ret = 0;
 
    len = m_aacConfigLength;  /*spec data长度,一般是2*/
 
    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+2);
    memset(packet,0,RTMP_HEAD_SIZE);
 
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
 
    /*AF 00 + AAC RAW data*/
    body[0] = 0xAF;
    body[1] = 0x00;
    memcpy(&body[2],m_aacBufferConfig,len); /*m_aacBufferConfig是AAC sequence header数据*/
 
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = len+2;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = pts;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = iRtmp.m_stream_id;
 
 #ifdef DUMP_FLV
    if (iAAcConfigSet == 0){
	WriteFLVTag(packet);
        iAAcConfigSet = 1;
    }
#endif
#ifdef NET_SEND
    /*调用发送接口*/
    if(!RTMP_SendPacket(&iRtmp,packet,TRUE))
	ret = -1;
#endif
    free(packet);
 
    return ret;
}


TTInt  CGKRtmpPush::SendAACData(CGKBuffer* aBuffer)
{
    int ret = 0;
    unsigned char * buf = aBuffer->pBuffer;
    int len = aBuffer->nSize;
    TTUint32 pts = aBuffer->llTime;

    ret = SendAACConfig(pts);
    
    if (ret != 0)
	return ret;
 
    if (len > 0) {
        RTMPPacket * packet;
        unsigned char * body;
 
        packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+2);
        memset(packet,0,RTMP_HEAD_SIZE);
 
        packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
        body = (unsigned char *)packet->m_body;
 
        /*AF 01 + AAC RAW data*/
        body[0] = 0xAF;
        body[1] = 0x01;
        memcpy(&body[2],buf,len);
 
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = len+2;
        packet->m_nChannel = 0x04;
        packet->m_nTimeStamp = pts;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
        packet->m_nInfoField2 = iRtmp.m_stream_id;
        
        //cs
        //LOGE("+++A.pts = %d",pts);

#ifdef DUMP_FLV
		WriteFLVTag(packet);
#endif

#ifdef NET_SEND
		/*调用发送接口*/
		if(!RTMP_SendPacket(&iRtmp,packet,TRUE))
			ret = -1;
#endif
		free(packet);
	}
	return ret;
}

//size = x264_encoder_encode(cx->hd,&nal,&n,pic,&pout);
// 
//int i,last;
//for (i = 0,last = 0;i < n;i++) {
//    if (nal[i].i_type == NAL_SPS) {
// 
//        sps_len = nal[i].i_payload-4;
//        memcpy(sps,nal[i].p_payload+4,sps_len);
// 
//    } else if (nal[i].i_type == NAL_PPS) {
// 
//        pps_len = nal[i].i_payload-4;
//        memcpy(pps,nal[i].p_payload+4,pps_len);
// 
//        /*发送sps pps*/
//        SendVideoConfig();    
// 
//    } else {
// 
//        /*发送普通帧*/
//        SendVideoData(nal[i].p_payload,nal[i].i_payload);
//    }
//    last += nal[i].i_payload;
//}

//TTUint CTTRtmpPush::BandWidth()
//{
//	return 0;
//}

char * put_byte( char *output, uint8_t nVal )    
{    
	output[0] = nVal;    
	return output+1;    
}   

char * put_be16(char *output, uint16_t nVal )    
{    
	output[1] = nVal & 0xff;    
	output[0] = nVal >> 8;    
	return output+2;    
}  

char * put_be24(char *output,uint32_t nVal )    
{    
	output[2] = nVal & 0xff;    
	output[1] = nVal >> 8;    
	output[0] = nVal >> 16;    
	return output+3;    
}

char * put_be32(char *output, uint32_t nVal )    
{    
	output[3] = nVal & 0xff;    
	output[2] = nVal >> 8;    
	output[1] = nVal >> 16;    
	output[0] = nVal >> 24;    
	return output+4;    
} 

char *  put_be64( char *output, uint64_t nVal )    
{    
    output=put_be32( output, nVal >> 32 );    
    output=put_be32( output, nVal );    
    return output;    
}    

char* put_amf_bool(char *c, int b)
{
    c = put_byte(c, AMF_BOOLEAN);
    c = put_byte(c, !!b);
	return c;
}

char * put_amf_string( char *c, const char *str )    
{    
    unsigned short len = strlen( str );    
    c=put_be16( c, len );    
    memcpy(c,str,len);    
    return c+len;    
}    
char * put_amf_double( char *c, double d )    
{    
    *c++ = AMF_NUMBER;  /* type: Number */    
    {    
        unsigned char *ci, *co;    
        ci = (unsigned char *)&d;    
        co = (unsigned char *)c;    
        co[0] = ci[7];    
        co[1] = ci[6];    
        co[2] = ci[5];    
        co[3] = ci[4];    
        co[4] = ci[3];    
        co[5] = ci[2];    
        co[6] = ci[1];    
        co[7] = ci[0];    
    }    
    return c+8;    
}  

int	CGKRtmpPush::WriteMetaData()
{  
	RTMPPacket * packet;
    char * body;
 
    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
    memset(packet,0,RTMP_HEAD_SIZE);
 
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body =  (char *)packet->m_body;
      
    char * p = (char *)body;    
    p = put_byte(p, AMF_STRING );  
    p = put_amf_string(p , "@setDataFrame" );  
  
    p = put_byte( p, AMF_STRING );  
    p = put_amf_string( p, "onMetaData" );  
  
    p = put_byte(p, AMF_OBJECT );    
    p = put_amf_string( p, "copyright" );    
    p = put_byte(p, AMF_STRING );    
    p = put_amf_string( p, "jackylinda" );    
  
    p =put_amf_string( p, "width");  
    p =put_amf_double( p, 480);  
  
    p =put_amf_string( p, "height");  
    p =put_amf_double( p, 288);  
  
    p =put_amf_string( p, "framerate" );  
    p =put_amf_double( p, 15);   
  
    p =put_amf_string( p, "videocodecid" );  
    p =put_amf_double( p, 7 );  

	p =put_amf_string( p, "audiosamplerate");  
    p =put_amf_double( p, 44100); 

	p =put_amf_string( p, "stereo");  
    p =put_amf_bool( p, 1);
  
    p =put_amf_string( p, "audiocodecid" );  
    p =put_amf_double( p, 10);  
  
    p =put_amf_string( p, "" );  
    p =put_byte( p, AMF_OBJECT_END  );  
  
    int index = p-body;

    packet->m_packetType = RTMP_PACKET_TYPE_INFO;
    packet->m_nBodySize = index;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = 0; //mRtmp->m_stream_id;

    WriteFLVTag(packet);

    free(packet);    
 
	return 0;
}

int	CGKRtmpPush::WriteFLVTag(RTMPPacket * packet)
{
#ifdef DUMP_FLV
	if(mFileFLV) {
		char cTag[16];
		char* buf = cTag;
		buf = put_byte(buf, packet->m_packetType);
		buf = put_be24(buf, packet->m_nBodySize);
		buf = put_be24(buf, packet->m_nTimeStamp);
		buf = put_byte(buf, packet->m_nTimeStamp >> 24);
		buf = put_be24(buf, 0);
		fwrite(cTag, 1, 11, mFileFLV);
		fwrite(packet->m_body, 1, packet->m_nBodySize, mFileFLV);

		put_be32(buf, 11 + packet->m_nBodySize);
  
		fwrite(buf, 1, 4, mFileFLV);
	}
#endif
	return 0;
}

int	CGKRtmpPush::OpenFLV()
{
#ifdef DUMP_FLV
	mFileFLV = fopen(gDumpflv, "wb+");
	if(mFileFLV) {
		unsigned char flvTag[13] = {0x46, 0x4c, 0x56, 0x01, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00};
		fwrite(flvTag, 1, 13, mFileFLV);

		WriteMetaData();
	}
#endif
	return 0;
}

int	CGKRtmpPush::CloseFLV()
{
#ifdef DUMP_FLV
	if(mFileFLV) {
		fclose(mFileFLV);
		mFileFLV = NULL;
	}
#endif
	return 0;
}

void CGKRtmpPush::NotifyEvent(GKPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2)
{
	iMsgCritical.Lock();

	if (mObserver){
		mObserver->NotifyEvent(aMsg, aArg1, aArg2);
	}
	iMsgCritical.UnLock();
}


#ifdef __TT_OS_ANDROID__   

TTInt CGKRtmpPush::OnVideoSend(void * pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	CTTRtmpPush* pThis = (CTTRtmpPush*)pUserData;
	TTInt nRet = pThis->ProcessVSendData(nID, nParam1, nParam2, pParam3);

	return nRet;
}

TTInt CGKRtmpPush::OnAudioSend(void * pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	CTTRtmpPush* pThis = (CTTRtmpPush*)pUserData;
	TTInt nRet = pThis->ProcessASendData(nID, nParam1, nParam2, pParam3);

	return nRet;
}

TTInt CGKRtmpPush::ProcessVSendData(TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	CTTBuffer* pSendBuffer = (CTTBuffer*)pParam3;
	if(nID == 109) {//data
		TransferVdieoData(pSendBuffer);
	} else if(nID == 107) {//sps
		SetVideoSps((unsigned char *)pParam3, nParam1); 
	} else if(nID == 108) {//pps
		SetVideoPps((unsigned char *)pParam3, nParam1); 
	}else if(nID == 110) {//file record
		WriteVdieoData(pSendBuffer);
	}

	return 0;
}

TTInt CGKRtmpPush::ProcessASendData(TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3)
{
	if(nID == 111) {//aac header
		SetAudioConfig((unsigned char *)pParam3, nParam1);
		//SetVideoPps((unsigned char *)pParam3, nParam1); 
	}else if(nID == 112) {//file record
		CTTBuffer* pSendBuffer = (CTTBuffer*)pParam3;
		WriteAudioData(pSendBuffer);
	}

	return 0;
}

#endif

void CGKRtmpPush::SetCollectionType(int type)
{
	mCollectType = type;//Collection_Live Collection_Record
#ifndef __TT_OS_IOS__
	mVCapProcess->SetCollectionType(type);
#endif
}

#ifdef __TT_OS_ANDROID__
void CGKRtmpPush::SetFilePath(const char* path)
{
	LOGE("set file %s %d",path,strlen(path) );
    if (iRecordPath){
        free(iRecordPath);
        iRecordPath = NULL;
    }
	iRecordPath = (TTChar*) malloc(strlen(path) + 1);
	strcpy(iRecordPath, path);

	mRecordFLV = fopen(iRecordPath,"wb+"); 

	if(mRecordFLV) {
		LOGE("File open ok");
		unsigned char flvTag[13] = {0x46, 0x4c, 0x56, 0x01, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00};
		fwrite(flvTag, 1, 13, mRecordFLV);

		WriteMetaData();

		NotifyEvent(ENotifyRecord_FILEOPEN_OK, 0, 0);
	}
}

void CGKRtmpPush::WriteVdieoData(CTTBuffer* aBuffer)
{
	char body[5]; 
	TTUint TimeStamp =  aBuffer->llTime;

	if(mCollectStart == 0){
		mCollectStart++;
		NotifyEvent(ENotifyRecord_START, 0, 0);
	}

	iCritical.Lock();

	mRecordTime = TimeStamp;

	body[0] = 0x27;
	if (aBuffer->nFlag == TT_FLAG_BUFFER_KEYFRAME)
	{
		body[0] = 0x17;
		WriteVdieoConfig(TimeStamp);
	}

	body[1] = 0x01;   /*nal unit*/
	body[2] = 0x00;
	body[3] = 0x00;
	body[4] = 0x00;

	if(mRecordFLV) {
		char cTag[16];
		char* buf = cTag;

		buf = put_byte(buf, RTMP_PACKET_TYPE_VIDEO);
		buf = put_be24(buf, aBuffer->nSize + 5);//5 : video tag 5byte ,len contain packetlength(4byte)
		buf = put_be24(buf, TimeStamp);
		buf = put_byte(buf, TimeStamp >> 24);
		buf = put_be24(buf, 0);
		fwrite(cTag, 1, 11, mRecordFLV);
		fwrite(body, 1, 5, mRecordFLV);
		fwrite( aBuffer->pBuffer, 1, aBuffer->nSize, mRecordFLV);
		put_be32(buf, 11 + aBuffer->nSize + 5);
		fwrite(buf, 1, 4, mRecordFLV);
	}
	iCritical.UnLock();
}

void CGKRtmpPush::WriteVdieoConfig(TTUint32 TimeStamp)
{
	unsigned char * bodystart;
	unsigned char * body;
	int i;

	if (m_spsBuffer == NULL || m_ppsBuffer== NULL || m_spsLength<=0 || m_ppsLength<=0)
		return  ;

	mCtritConfig.Lock();
	unsigned char *sps = m_spsBuffer;
	int sps_len = m_spsLength;
	unsigned char *pps = m_ppsBuffer;
	int pps_len = m_ppsLength;

	body = (unsigned char *)malloc(1024);
	bodystart = body;

	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i], pps, pps_len);
	i +=  pps_len;

	if(mRecordFLV) {
		char cTag[16];
		char* buf = cTag;
		buf = put_byte(buf, RTMP_PACKET_TYPE_VIDEO);
		buf = put_be24(buf, i);//m_nBodySize
		buf = put_be24(buf, TimeStamp);
		buf = put_byte(buf, TimeStamp >> 24);
		buf = put_be24(buf, 0);
		fwrite(cTag, 1, 11, mRecordFLV);
		fwrite(bodystart, 1, i, mRecordFLV);
		put_be32(buf, 11 + i);
		fwrite(buf, 1, 4, mRecordFLV);
	}
	mCtritConfig.UnLock();
	free(bodystart);
}

void CGKRtmpPush::WriteAudioConfig(TTUint32 TimeStamp)
{
	unsigned char body[2];
    int len  = m_aacConfigLength;  /*spec data长度,一般是2*/
 
    /*AF 00 + AAC RAW data*/
    body[0] = 0xAF;
    body[1] = 0x00;
    memcpy(&body[2],m_aacBufferConfig,len); /*m_aacBufferConfig是AAC sequence header数据*/

	if(mRecordFLV) {
		char cTag[16];
		char* buf = cTag;
		buf = put_byte(buf, RTMP_PACKET_TYPE_AUDIO);
		buf = put_be24(buf, len+2);//m_nBodySize
		buf = put_be24(buf, TimeStamp);
		buf = put_byte(buf, TimeStamp >> 24);
		buf = put_be24(buf, 0);
		fwrite(cTag, 1, 11, mRecordFLV);
		fwrite(body, 1, 2, mRecordFLV);
		fwrite(m_aacBufferConfig, 1, len, mRecordFLV);
		put_be32(buf, 11 + len+2);
		fwrite(buf, 1, 4, mRecordFLV);
	}
}

void CGKRtmpPush::WriteAudioData(CTTBuffer* aBuffer)
{
	int len = aBuffer->nSize;
	TTUint32 TimeStamp = aBuffer->llTime;

	if(mCollectStart == 0){
		mCollectStart++;
		NotifyEvent(ENotifyRecord_START, 0, 0);
	}

	iCritical.Lock();

	mRecordTime = TimeStamp;

	WriteAudioConfig(TimeStamp);

	if (len > 0) {//len+2
		unsigned char body[2];
		/*AF 01 + AAC RAW data*/
		body[0] = 0xAF;
		body[1] = 0x01;

		if(mRecordFLV) {
			char cTag[16];
			char* buf = cTag;

			buf = put_byte(buf, RTMP_PACKET_TYPE_AUDIO);
			buf = put_be24(buf, len+2);
			buf = put_be24(buf, TimeStamp);
			buf = put_byte(buf, TimeStamp >> 24);
			buf = put_be24(buf, 0);
			fwrite(cTag, 1, 11, mRecordFLV);
			fwrite(body, 1, 2, mRecordFLV);
			fwrite(aBuffer->pBuffer, 1, aBuffer->nSize, mRecordFLV);
			put_be32(buf, 11 + len+2);
			fwrite(buf, 1, 4, mRecordFLV);
		}
	}
	iCritical.UnLock();
}

long CGKRtmpPush::GetRecordTime()
{
	long tm;
	iCritical.Lock();
	tm = mRecordTime;
	iCritical.UnLock();
	return tm;
}

void CGKRtmpPush::TransferVdieoRawData(TTPBYTE pdata, TTInt size, TTUint32 pts,TTInt rotateType)
{
	mVCapProcess->ProcessVCapData(pdata,size,pts, rotateType);
}

void CGKRtmpPush::TransferAudioRawData(TTPBYTE pdata, TTInt size, TTUint32 pts)
{
	mACapProcess->ProcesACapData(pdata,size,pts);
}

void CGKRtmpPush::VideoEncoderInit()
{
	mVCapProcess->EncoderInit();
}

void CGKRtmpPush::SetVideoFpsBitrate(int fps,int Bitrate)
{
	mVCapProcess->SetFrameRate(fps);
	mVCapProcess->SetVideoBitrate(Bitrate);
}

void CGKRtmpPush::SetVideoWidthHeight(int width,int height)
{
	mVCapProcess->SetWidth(width);
	mVCapProcess->SetHeight(height);
}

void CGKRtmpPush::HandleRawdata(unsigned char* src,unsigned char* dst,int with, int heigh, int rotateType)
{
	mVCapProcess->HandleRawdata(src,dst,with,heigh,rotateType);
}

void CGKRtmpPush::SetColorFormat(int color)
{
	mVCapProcess->SetColorFormat(color);
}

//file record
void CGKRtmpPush::RecordStart(int smaplerate, int channel)
{
	mACapProcess->SetParameter(smaplerate, channel);
	mACapProcess->Start();

	mVCapProcess->Start();
}
void CGKRtmpPush::RecordPause()
{
	mACapProcess->Pause();
	mVCapProcess->Pause();
}
void CGKRtmpPush::RecordResume()
{
	mACapProcess->Resume();
	mVCapProcess->Resume();
}
void CGKRtmpPush::RecordClose()
{
	mACapProcess->Close();
	mVCapProcess->Close();

	iCritical.Lock();
	if(mRecordFLV){
		fclose(mRecordFLV);
		mRecordFLV = NULL;
	}
	iCritical.UnLock();

	iMsgCritical.Lock();
	mObserver = NULL;
	iMsgCritical.UnLock();

	mHandleThread->stop();
}

void CGKRtmpPush::SetRenderProxy(void* proxy)
{
	mVCapProcess->SetRenderProxy(proxy);
}

#endif

#ifdef __TT_OS_IOS__
void CGKRtmpPush::Netdisconnect()
{
    if (mOpenStatus) {
        if (iConnectionTid > 0 && !pthread_equal(iConnectionTid, pthread_self()))
        {
            iRtmp.m_cancle = ETTTrue;
            int pthread_kill_err = pthread_kill(iConnectionTid, 0);
            if((pthread_kill_err != ESRCH) && (pthread_kill_err != EINVAL))
            {
                pthread_kill(iConnectionTid, SIGALRM);
                LOGI("sent interrupt signal");
            }
        }
    }
}
#endif
