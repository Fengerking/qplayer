/**
* File : TTRtmpInfoProxy.cpp
* Created on : 2015-9-25
* Author : Kevin
* Copyright : Copyright (c) 2015 GoKu Ltd. All rights reserved.
* Description : CTTRtmpInfoProxy 实现文件
*/

//INCLUDES
#include "TTFLVParser.h"
#include "TTRtmpInfoProxy.h"
#include "TTSysTime.h"
#include "TTLog.h"
#include "TTBufferManager.h"

#define BUFFERING_START 0
#define BUFFERING_DONE  1
#define MAXWAITCNT      600
#define WAITINTERVALMS  100

CTTRtmpInfoProxy::CTTRtmpInfoProxy(TTObserver* aObserver)
: iObserver(aObserver)
, mCurMinBuffer(2000)
, mCurBandWidth(0)
, mFirstTimeOffsetUs(0)
, mCancel(0)
, iBufferStatus(-1)
, iAudioStream(NULL)
, iVideoStream(NULL)
, iCountAMAX(100)
, iCountVMAX(60)
{
	mCriEvent.Create();
    mCriStatus.Create();
	mSemaphore.Create();
	iRtmpDownload = new CTTRtmpDownload(this);
}

CTTRtmpInfoProxy::~CTTRtmpInfoProxy()
{
	Close();
    
    SAFE_DELETE(iAudioStream);
    SAFE_DELETE(iVideoStream);

	mSemaphore.Destroy();
	mCriEvent.Destroy();
    mCriStatus.Destroy();

	SAFE_DELETE(iRtmpDownload);
}

int  CTTRtmpInfoProxy::AddAudioTag(unsigned char *data, unsigned int size, TTInt64 timeUs)
{
    int ret = 0;
    if (iAudioStream) {
        ret = iAudioStream->addTag(data, size, timeUs);
    }
    return ret;
}

int  CTTRtmpInfoProxy::AddVideoTag(unsigned char *data, unsigned int size, TTInt64 timeUs)
{
    int ret = 0;
    if (iVideoStream) {
        ret = iVideoStream->addTag(data, size, timeUs);
    }
    return ret;
}

void CTTRtmpInfoProxy::DiscardUselessBuffer()
{
    TTInt vCnt = -1;
    TTInt aCnt = -1;
    TTBufferManager* sourceA = NULL;
    TTBufferManager* sourceV = NULL;
    
    if(iAudioStream != NULL) {
        sourceA = iAudioStream->getSource();
        if(sourceA != NULL) {
            aCnt = sourceA->getBufferCount();
        }
    }
    
    if(iVideoStream != NULL) {
        sourceV = iVideoStream->getSource();
        if(sourceV != NULL) {
            vCnt = sourceV->getBufferCount();
        }
    }
    
    mCriStatus.Lock();
    TTInt eBufferStatus = iBufferStatus;
    mCriStatus.UnLock();
    
    if (eBufferStatus == BUFFERING_START){
        if((aCnt == -1 || aCnt >= iCountAMAX) && (vCnt == -1 || vCnt >= iCountVMAX)) {
            BufferingReady();
            mCriStatus.Lock();
            iBufferStatus = BUFFERING_DONE;
            mCriStatus.UnLock();
        }
    }
    
    if((aCnt == -1 || aCnt >= 150) && (vCnt == -1 || vCnt >= 100)) {
        TTInt64 nStartTime = 0;
        TTInt64 nSeekTime = 0;
        TTInt	nDuration = 0;
        TTInt   nEOS = 0;
        if(sourceV) {
            sourceV->nextBufferTime(&nStartTime);
            nDuration = sourceV->getBufferedDurationUs(&nEOS);
            nSeekTime = nStartTime + nDuration - 10;
            nSeekTime = sourceV->seek(nSeekTime);
        } else {
            if(sourceA) {
                sourceA->nextBufferTime(&nStartTime);
                nDuration = sourceA->getBufferedDurationUs(&nEOS);
                nSeekTime = nStartTime + nDuration - 200;
            }
        }
        
        if(sourceA) {
            nSeekTime = sourceA->seek(nSeekTime);
        }
    }
}

TTInt CTTRtmpInfoProxy::Open(const TTChar* aUrl, TTInt aFlag)
{
	TTInt ret;
	if(aUrl == NULL) {
		return TTKErrArgument;
	}

	if(!IsRtmpSource(aUrl)) {
		return TTKErrNotSupported;
	}

	mCancel = 0;
    
    SAFE_DELETE(iAudioStream);
    SAFE_DELETE(iVideoStream);
    
    if(iAudioStream == NULL) {
        iAudioStream = new CTTFlvTagStream(FLV_STREAM_TYPE_AUDIO);
    }
        
    if(iVideoStream == NULL) {
        iVideoStream = new CTTFlvTagStream(FLV_STREAM_TYPE_VIDEO);
    }

	iRtmpDownload->SetStreamBufferingObserver(this);
    
	ret = iRtmpDownload->Open(aUrl);
    
    if (ret != 0) {
        SAFE_DELETE(iAudioStream);
        SAFE_DELETE(iVideoStream);
    }

	return ret;
}

TTInt CTTRtmpInfoProxy::Parse()
{
	TTInt nErr = TTKErrNone;
    TTInt nWaitCount = 0;
    TTBufferManager* source = NULL;
    TTBuffer buffer;
    TTBool checkAudio = ETTFalse;
    TTBool checkVideo = ETTFalse;
    while (!mCancel) {
        if (!checkAudio) {
            source = iAudioStream->getSource();
            if(source) {
                nErr = source->dequeueAccessUnit(&buffer);
                if (nErr == TTKErrNotReady) {
                }
                else{
                    if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
                        checkAudio = ETTTrue;
                        TTAudioInfo* pAudioInfo = new TTAudioInfo(*((TTAudioInfo*)buffer.pData));
                        iMediaInfo.iAudioInfoArray.Append(pAudioInfo);
                    }
                }
            }
        }
        
        if (!checkVideo) {
            source = iVideoStream->getSource();
            if(source) {
                nErr = source->dequeueAccessUnit(&buffer);
                if (nErr == TTKErrNotReady) {
                }
                else{
                    if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
                        checkVideo = ETTTrue;
                        TTVideoInfo* pVideoInfo = new TTVideoInfo(*((TTVideoInfo*)buffer.pData));
                        iMediaInfo.iVideoInfo = pVideoInfo;
                    }
                }
            }
        }
        
        if (checkVideo && checkAudio) {
            break;
        }
        
        if(nWaitCount++ < MAXWAITCNT && !mCancel)
        {
            mSemaphore.Wait(WAITINTERVALMS);
        }
        else
        {
            if (!mCancel)
                nErr = TTKErrNotReady;
            else
                nErr = TTKErrCancel;
            break;
        }
    }


	if(nErr != TTKErrNone )
	{
		iMediaInfo.Reset();
	}

	return nErr;
}

void CTTRtmpInfoProxy::Close()
{
	iMediaInfo.Reset();
	if (iRtmpDownload)
	   iRtmpDownload->Close();
}

void CTTRtmpInfoProxy::EOFNotify()
{
    if(iAudioStream) {
        iAudioStream->signalEOS(true);
    }
    if(iVideoStream) {
        iVideoStream->signalEOS(true);
    }
}
const TTMediaInfo& CTTRtmpInfoProxy::GetMediaInfo()
{
	return iMediaInfo;
}

TTUint CTTRtmpInfoProxy::MediaSize()
{
	return  0;
}

TTBool CTTRtmpInfoProxy::IsSeekAble()
{
	return 0;
}

void CTTRtmpInfoProxy::CreateFrameIndex()
{
}

TTInt CTTRtmpInfoProxy::GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer)
{
    TTBuffer buffer;
    
    TTBufferManager* source = NULL;
    
    memset(&buffer, 0, sizeof(TTBuffer));
    buffer.llTime = pMediaBuffer->llTime;
    if(aStreamType == EMediaTypeAudio) {
        if(iAudioStream == NULL) {
            return TTKErrNotReady;
        }
        
        source = iAudioStream->getSource();
        if(source == NULL) {
            return TTKErrNotReady;
        }
    } else if(aStreamType == EMediaTypeVideo) {
        if(iVideoStream == NULL) {
            return TTKErrNotReady;
        }
        
        source = iVideoStream->getSource();
        if(source == NULL) {
            return TTKErrNotReady;
        }
    }
    
    TTInt nErr = TTKErrNotReady;
    if(source) {
        nErr = source->dequeueAccessUnit(&buffer);
        
        //int bufferCount = source->getBufferCount();,LOGI("GetMediaSample: tMediaType %d, bufferCount %d, nErr %d", aStreamType, bufferCount, nErr);
        
        if(nErr ==  TTKErrNone) {
            memcpy(pMediaBuffer, &buffer, sizeof(TTBuffer));
            return TTKErrNone;
        } else if(nErr == TTKErrEof){
            return TTKErrEof;
        } else if(nErr == TTKErrNotReady) {
            SendBufferStartEvent();
        }
    }
    return TTKErrNotFound;
}

void CTTRtmpInfoProxy::SendBufferStartEvent()
{
    mCriStatus.Lock();
    TTInt eBufferStatus = iBufferStatus;
    mCriStatus.UnLock();
    
    if (eBufferStatus != BUFFERING_START)
    {
        BufferingEmtpy(TTKErrNotReady, 0, iRtmpDownload->GetHostIP());
        
        mCriStatus.Lock();
        iBufferStatus = BUFFERING_START;
        mCriStatus.UnLock();
    }
}

TTUint CTTRtmpInfoProxy::MediaDuration()
{
	return 0;
}

TTBool CTTRtmpInfoProxy::IsCreateFrameIdxComplete()
{ 
	return ETTTrue;
}

TTUint CTTRtmpInfoProxy::BufferedSize()
{
	return 0;
}

TTUint CTTRtmpInfoProxy::ProxySize()
{
	return 0;
}

TTUint CTTRtmpInfoProxy::BandWidth()
{
	TTInt nBandWidth = 0;
	nBandWidth = iRtmpDownload->BandWidth();

	return nBandWidth;
}

void CTTRtmpInfoProxy::SetDownSpeed(TTInt aFast)
{
}

TTUint CTTRtmpInfoProxy::BandPercent()
{
	return 0;
}

TTInt CTTRtmpInfoProxy::BufferedPercent(TTInt& aBufferedPercent)
{
	return 0;
}

TTInt CTTRtmpInfoProxy::SelectStream(TTMediaType aType, TTInt aStreamId)
{
	return 0;
}

TTInt64 CTTRtmpInfoProxy::Seek(TTUint64 aPosMS, TTInt aOption)
{
	return 0;
}

TTInt CTTRtmpInfoProxy::SetParam(TTInt aType, TTPtr aParam)
{
	return TTKErrNotSupported;
}

TTInt CTTRtmpInfoProxy::GetParam(TTInt aType, TTPtr aParam)
{
	if(aParam == NULL) {
		return TTKErrArgument;
	}

	if(aType == TT_PID_COMMON_STATUSCODE) {
		*((TTInt *)aParam) = 0;
		return 0;
	} else if(aType == TT_PID_COMMON_HOSTIP) {
		if(iRtmpDownload) {
			*((TTUint *)aParam) = iRtmpDownload->GetHostIP();
		} else {
			*((TTUint *)aParam) = 0;
		}
		return 0;
	} else if(aType == TT_PID_COMMON_LIVEDMOE) {
		*((TTUint *)aParam) = 1;
		return 0;
	}
        return TTKErrNotSupported;
}

int CTTRtmpInfoProxy::IsRtmpSource(const TTChar* aUrl)
{
#ifndef __TT_OS_WINDOWS__
	return (strncasecmp("rtmp://", aUrl, 7) == 0 );
#else
	return (strnicmp("rtmp://", aUrl, 7) == 0);
#endif
}

void CTTRtmpInfoProxy::SetNetWorkProxy(TTBool aNetWorkProxy)
{
}

void CTTRtmpInfoProxy::CancelReader()
{
	mCancel = ETTTrue;
	mSemaphore.Signal();

	if (iRtmpDownload)
		iRtmpDownload->Cancel();
}


void CTTRtmpInfoProxy::SetObserver(TTObserver*	aObserver)
{
	GKCAutoLock lock(&mCriEvent);
	iObserver = aObserver;
}

void CTTRtmpInfoProxy::BufferingReady()
{
    GKCAutoLock lock(&mCriEvent);
    if(iObserver && iObserver->pObserver)
        iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingDone, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::BufferingEmtpy(TTInt nErr, TTInt nStatus, TTUint32 aParam)
{
    GKCAutoLock lock(&mCriEvent);
    if(iObserver && iObserver->pObserver) {
        char *pParam3 = NULL;
        if(aParam)
            pParam3 = inet_ntoa(*(struct in_addr*)&aParam);
        iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingStart, nErr, nStatus, pParam3);
    }
}

void CTTRtmpInfoProxy::BufferingStart(TTInt nErr, TTInt nStatus, TTUint32 aParam)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingStart, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::BufferingDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingDone, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::DNSDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyDNSDone, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::ConnectDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyConnectDone, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::HttpHeaderReceived()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyHttpHeaderReceived, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::PrefetchStart(TTUint32 aParam)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchStart, TTKErrNone, aParam, NULL);
}

void CTTRtmpInfoProxy::PrefetchCompleted()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchCompleted, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::CacheCompleted(const TTChar* pFileName)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyCacheCompleted, TTKErrNone, 0, NULL);
}

void CTTRtmpInfoProxy::DownLoadException(TTInt errorCode, TTInt nParam2, void *pParam3)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyException, errorCode, nParam2, pParam3);
}
