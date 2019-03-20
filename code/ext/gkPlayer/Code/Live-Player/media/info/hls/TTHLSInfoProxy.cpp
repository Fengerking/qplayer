/**
* File : TTHLSInfoProxy.cpp
* Created on : 2015-5-12
* Author : yongping.lin
* Description : TTHLSInfoProxy 实现文件
*/

//INCLUDES
#include "TTHLSInfoProxy.h"
#include "TTM3UParser.h"
#include "TTSysTime.h"
#include "TTLog.h"

CTTHLSInfoProxy::CTTHLSInfoProxy(TTObserver* aObserver)
: iObserver(aObserver)
, mCurBitrateIndex(0)
, mCurMinBuffer(2000)
, mCurBandWidth(0)
, mAudioStreamID(-1)
, mVideoStreamID(-1)
, mFirstPTSValid(false)
, mFirstPTS(-1)
, mResetAudioPTS(-1)
, mResetVideoPTS(-1)
, mLastAudioPTS(-1)
, mLastVideoPTS(-1)
, mFirstTimeOffsetUs(0)
, mSeekTime(0)
, mSeekOption(0)
, mSeeking(false)
, mPlayListBuffer(NULL)
, mPlayListSize(0)
, mCurSession(NULL)
, mBASession(NULL)
, mCurItem(NULL)
, mBAItem(NULL)
, mBASwitchStatus(EBASwitchNone)
, mSwitchAudioStatus(EBASwitchNone)
, mSwitchVideoStatus(EBASwitchNone)
, mBABitrateIndex(-1)
, mUpCount(0)
, mDownCount(0)
, mStartHLS(1)
, mLiveErrNum(0)
, mCancel(0)
, mBufferStatus(0)
{
	mCriEvent.Create();
	mCriSession.Create();
	mCriBA.Create();
	mCriPlayList.Create();
	mSemaphore.Create();

	mMsgThread	   = new TTEventThread("TTHLSInfo Thread");

	mPlayListManager = new PlaylistManager();

	mPL = new CTTIOClient(this);
}

CTTHLSInfoProxy::~CTTHLSInfoProxy()
{
	Close();
	SAFE_DELETE(mMsgThread);
	freeLiveSession();
	SAFE_DELETE(mPlayListManager);	
	SAFE_DELETE(mPL);
	SAFE_FREE(mPlayListBuffer);

	mSemaphore.Destroy();
	mCriPlayList.Destroy();
	mCriBA.Destroy();
	mCriEvent.Destroy();
	mCriSession.Destroy();
}

TTInt CTTHLSInfoProxy::Open(const TTChar* aUrl, TTInt aFlag)
{
	if(aUrl == NULL) {
		return TTKErrArgument;
	}

	if(!IsHLSSource(aUrl)) {
		return TTKErrNotSupported;
	}

	mCancel = 0;
	mStartHLS = 1;

	mMsgThread->start();
	TTInt nErr = initPlayList(aUrl, aFlag);
	return nErr;
}

TTInt CTTHLSInfoProxy::resetInitPlayList()
{
	int isOK = 0;
	if(mPlayListManager->isVariantPlaylist() && mPlayListManager->getVariantNum() > 1) {
		mCurSession->cancel();
		mCurSession->stop();

		int nIndex = mPlayListManager->getCurBitrateIndex();
		if(nIndex > 0) {
			nIndex = 0;
		} else {
			nIndex++;
		}

		ListItem* pItem = mPlayListManager->getListItem(nIndex, EMediaList, 0);
		if(pItem != NULL && updatePlayList(pItem) == TTKErrNone) {
			isOK = 1;
		}

		if(isOK) {
			mCurBitrateIndex = nIndex;
			mPlayListManager->setCurBitrateIndex(mCurBitrateIndex);

			mCurItem = pItem;
			mCurSession->setUrlListItem(pItem);
			TTInt nseqNum = mPlayListManager->initSeqNum(pItem, 0);
			mCurSession->start(nseqNum);

			TTInt nReady = isHeadReady(mCurSession, 1, 1);
			if(nReady < 2) {
				isOK = 0;
			}
		}
	}

	return isOK;
}

TTInt CTTHLSInfoProxy::Parse()
{
	TTInt nErr = TTKErrNone;

	mCriSession.Lock();
	if(mCurSession == NULL) {
		mCriSession.UnLock();
		return TTKErrNotReady;
	}

	TTInt nReady = isHeadReady(mCurSession, 1, 1);
	if(nReady < 2) {
		int isOK = resetInitPlayList();
		if(isOK == 0) {
			nReady = isHeadReady(mCurSession, 1, 0);
			if(nReady == 0) {
				mCriSession.UnLock();
				return TTKErrNotReady;
			}
		}
	}

	nErr = updateMediaInfo(mCurSession);
	mCriSession.UnLock();

	GKCAutoLock lock(&mCriEvent);
	mCurMinBuffer = 5000;
	postInfoMsgEvent(100, ECheckBufferStatus, mCurMinBuffer, 0, (void *)mCurSession);
	if(mPlayListManager->isVariantPlaylist()) {
		postInfoMsgEvent(2000, ECheckBandWidth, 0, 0, NULL);
	}

	mBufferStatus = 1;
	BufferingStart(TTKErrNotReady, 0, 0);

	return nErr;
}

TTInt CTTHLSInfoProxy::updateMediaInfo(CLiveSession*	aLiveSession)
{
	TTInt nErr = TTKErrNone;
	TTInt nNum = aLiveSession->getProgramStreamNum(1);
	TTBufferManager* source = NULL;
	TTBuffer buffer;
	
	for(TTInt n = 0; n < nNum; n++) {
		source = aLiveSession->getStreamSource(1, n);
		if(source) {
			if(source->isAudio()) {
				nErr = source->dequeueAccessUnit(&buffer);
				if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
					TTAudioInfo* pAudioInfo = new TTAudioInfo(*((TTAudioInfo*)buffer.pData));
					iMediaInfo.iAudioInfoArray.Append(pAudioInfo);
				}
			}

			if(source->isVideo()) {
				nErr = source->dequeueAccessUnit(&buffer);
				if(buffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT) {
					TTVideoInfo* pVideoInfo = new TTVideoInfo(*((TTVideoInfo*)buffer.pData));
					iMediaInfo.iVideoInfo = pVideoInfo;
				}
			}
		}
	}

	return TTKErrNone;
}

void CTTHLSInfoProxy::Close()
{	
	iMediaInfo.Reset();

	mMsgThread->stop();

	mCriSession.Lock();
	if(mCurSession) {
		mCurSession->stop();
		putLiveSession(mCurSession);
		mCurSession = NULL;
	}

	if(mBASession) {
		mBASession->stop();
		putLiveSession(mBASession);
		mBASession = NULL;
	}
	mCurItem = NULL;
	mBAItem = NULL;


	mFirstPTSValid = false;
    mFirstPTS = 0;
	mResetAudioPTS = -1;
	mResetVideoPTS = -1;
	mLastAudioPTS = 0;
	mLastVideoPTS = 0;

	mCriBA.Lock();
	mSwitchAudioStatus = EBASwitchNone;
	mSwitchVideoStatus = EBASwitchNone;
	mBASwitchStatus = EBASwitchNone;
	mCriBA.UnLock();

	mCriSession.UnLock();
	
	mPL->Close();
	mPlayListManager->stop();
}

const TTMediaInfo& CTTHLSInfoProxy::GetMediaInfo() 
{
	return iMediaInfo;
}

TTUint CTTHLSInfoProxy::MediaSize()
{
	return  0;
}

TTBool CTTHLSInfoProxy::IsSeekAble()
{
	if(mPlayListManager->isLive()) {
		return ETTFalse;
	}

	return ETTTrue;
}

void CTTHLSInfoProxy::CreateFrameIndex()
{
}

TTInt CTTHLSInfoProxy::GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer)
{	
	if(mCurSession == NULL) {
		return TTKErrNotReady;
	}

	TTInt nErr = TTKErrNotReady;
	GKCAutoLock lock(&mCriSession);

	mCriBA.Lock();
	TTInt nAudioStatus = mSwitchAudioStatus;
	TTInt nVideoStatus = mSwitchVideoStatus;
	mCriBA.UnLock();

	if(aStreamType == EMediaTypeAudio) {
		if(nAudioStatus == EBASwitched) {
			if(nVideoStatus == EBASwitchDownStart || nVideoStatus == EBASwitchUpStart) {
				nErr = TTKErrNotReady;
			} else {
				nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mAudioStreamID);
			}
		} else if(nAudioStatus == EBASwitching) {
			nErr = GetMediaSamplebyID(mCurSession, aStreamType, pMediaBuffer, mAudioStreamID);
			if(nErr == TTKErrNone) {
				TTInt64 nStartTime = 0;
				TTInt nDuration = 0;
				TTInt nErr1 = mBASession->getBufferStatus(true, &nStartTime, &nDuration); 
				if(nErr1 == TTKErrNone)
				{
					TTInt nDiff = (TTInt)(nStartTime - mFirstPTS - pMediaBuffer->llTime);
					if((pMediaBuffer->nFlag & TT_FLAG_BUFFER_FLUSH) && pMediaBuffer->llTime + mFirstPTS > nStartTime) {
						nDiff = 0;
					}

					if(nDiff < 10 && nDiff > -10) {
						mCriBA.Lock();
						mSwitchAudioStatus = EBASwitched;
						mCriBA.UnLock();
						nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mAudioStreamID); 

						LOGI("onInfoBandWidth: switch audio completed nDiff %d", nDiff);
					}
				}			
			} else if(nErr == TTKErrNotReady) {
				mCriBA.Lock();
				mSwitchAudioStatus = EBASwitched;
				mCriBA.UnLock();
				nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mAudioStreamID); 

				LOGI("onInfoBandWidth: switch audio completed buffer empty");
			}
		} else {
			nErr = GetMediaSamplebyID(mCurSession, aStreamType, pMediaBuffer, mAudioStreamID);
		}
	} else if(aStreamType == EMediaTypeVideo) {
		if(nVideoStatus == EBASwitched) {
			if(nAudioStatus == EBASwitchDownStart || nAudioStatus == EBASwitchUpStart) {
				nErr = TTKErrNotReady;
			} else {
				nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mVideoStreamID);
			}
		} else if(nVideoStatus == EBASwitching) {
			nErr = GetMediaSamplebyID(mCurSession, aStreamType, pMediaBuffer, mVideoStreamID);
			if(nErr == TTKErrNone) {
				TTInt64 nStartTime = 0;
				TTInt nDuration = 0;
				TTInt nErr1 = mBASession->getBufferStatus(false, &nStartTime, &nDuration); 
				if(nErr1 == TTKErrNone)
				{
					TTInt nDiff = (TTInt)(nStartTime - mFirstPTS - pMediaBuffer->llTime);
					if((pMediaBuffer->nFlag & TT_FLAG_BUFFER_FLUSH) && pMediaBuffer->llTime + mFirstPTS > nStartTime) {
						nDiff = 0;
					}

					if(nDiff < 10 && nDiff > -10) {
						mCriBA.Lock();
						mSwitchVideoStatus = EBASwitched;
						mCriBA.UnLock();
						nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mVideoStreamID);

						LOGI("onInfoBandWidth: switch video completed, nDiff %d", nDiff);
					}
				}
			} else if(nErr == TTKErrNotReady) {
				mCriBA.Lock();
				mSwitchVideoStatus = EBASwitched;
				mCriBA.UnLock();
				nErr = GetMediaSamplebyID(mBASession, aStreamType, pMediaBuffer, mVideoStreamID);

				LOGI("onInfoBandWidth: switch video completed buffer empty");
			}
		} else {
			nErr = GetMediaSamplebyID(mCurSession, aStreamType, pMediaBuffer, mVideoStreamID);
		}
	}

	if(nErr == TTKErrNotReady) {
		GKCAutoLock Lock(&mCriEvent);
		if(mBufferStatus == 0) {
			onInfoBufferStart(mCurSession);
			mBufferStatus = 1;
		}
	}

	mCriBA.Lock();
	if(mSwitchAudioStatus == EBASwitched && mSwitchVideoStatus == EBASwitched) {
		mCriBA.Lock();
		mSwitchAudioStatus = EBASwitchNone;
		mSwitchVideoStatus = EBASwitchNone;
		mBASwitchStatus = EBASwitchNone;
		mCriBA.UnLock();

		mCurBitrateIndex = mBABitrateIndex;
		mPlayListManager->setCurBitrateIndex(mCurBitrateIndex);

		GKCAutoLock lock(&mCriEvent);
		postInfoMsgEvent(0, ECheckCloseSession, 0, 0, (void *)mCurSession);

		LOGI("onInfoBandWidth: switch stream completed");

		mCurItem = mBAItem;
		mCurSession = mBASession;
		mBASession = NULL;
	}	
	mCriBA.UnLock();

	return  nErr;
}

void CTTHLSInfoProxy::upDateTimeStamp(TTMediaType aStreamType, TTBuffer* pMediaBuffer)
{
	TTInt64 nCurTime = pMediaBuffer->llTime - mFirstPTS;
	if(aStreamType == EMediaTypeAudio) {
		if(mResetVideoPTS != -1) {
			if(pMediaBuffer->llTime < mResetVideoPTS) {
				pMediaBuffer->llTime = 0; 
			} else {
				pMediaBuffer->llTime -= mResetVideoPTS;
			}
			mFirstPTS = mResetVideoPTS;
			mResetVideoPTS = -1;
			mResetAudioPTS = -1;
		} else {
			if(mResetAudioPTS != -1) {
				pMediaBuffer->llTime -= mResetAudioPTS;
			} else {
				mResetAudioPTS = pMediaBuffer->llTime - (mLastAudioPTS + 20);
				pMediaBuffer->llTime = mLastAudioPTS + 20;
			}
		}
	} else if(aStreamType == EMediaTypeVideo) {
		if(mResetAudioPTS != -1) {
			if(pMediaBuffer->llTime < mResetAudioPTS) {
				pMediaBuffer->llTime = 0; 
			} else {
				pMediaBuffer->llTime -= mResetAudioPTS;
			}
			mFirstPTS = mResetAudioPTS;
			mResetVideoPTS = -1;
			mResetAudioPTS = -1;
		} else {
			if(mResetVideoPTS != -1) {
				pMediaBuffer->llTime -= mResetVideoPTS;
			} else {
				mResetVideoPTS = pMediaBuffer->llTime - (mLastVideoPTS + 30);
				pMediaBuffer->llTime = mLastVideoPTS + 30;
			}
		}
	}
}

void CTTHLSInfoProxy::resetTimeStamp(TTMediaType aStreamType, TTBuffer* pMediaBuffer)
{
	if(mPlayListManager->isLive()) {
		if(aStreamType == EMediaTypeAudio) {
			if(mResetVideoPTS >= 0) {
				if(pMediaBuffer->llTime < mResetVideoPTS) {
					pMediaBuffer->llTime = 0; 
				} else {
					pMediaBuffer->llTime -= mResetVideoPTS;
				}
				pMediaBuffer->nFlag |= TT_FLAG_BUFFER_TIMESTAMP_RESET;
				mFirstPTS = mResetVideoPTS;
				mResetVideoPTS = -1;
				mResetAudioPTS = -1;
			} else {
				if(mResetAudioPTS >= 0) {
					pMediaBuffer->llTime -= mResetAudioPTS;
				} else {
					mResetAudioPTS = pMediaBuffer->llTime;
					pMediaBuffer->nFlag |= TT_FLAG_BUFFER_TIMESTAMP_RESET;
					pMediaBuffer->llTime = 0;
				}
			}
		} else if(aStreamType == EMediaTypeVideo) {
			if(mResetAudioPTS >= 0) {
				if(pMediaBuffer->llTime < mResetAudioPTS) {
					pMediaBuffer->llTime = 0; 
				} else {
					pMediaBuffer->llTime -= mResetAudioPTS;
				}
				pMediaBuffer->nFlag |= TT_FLAG_BUFFER_TIMESTAMP_RESET;
				mFirstPTS = mResetAudioPTS;
				mResetVideoPTS = -1;
				mResetAudioPTS = -1;
			} else {
				if(mResetVideoPTS >= 0) {
					pMediaBuffer->llTime -= mResetVideoPTS;
				} else {
					mResetVideoPTS = pMediaBuffer->llTime;
					pMediaBuffer->nFlag |= TT_FLAG_BUFFER_TIMESTAMP_RESET;
					pMediaBuffer->llTime = 0;
				}
			}
		}
	} else {
		if(aStreamType == EMediaTypeAudio) {
			if(mResetVideoPTS != -1) {
				if(pMediaBuffer->llTime < mResetVideoPTS) {
					pMediaBuffer->llTime = 0; 
				} else {
					pMediaBuffer->llTime -= mResetVideoPTS;
				}
				mFirstPTS = mResetVideoPTS;
				mResetVideoPTS = -1;
				mResetAudioPTS = -1;
			} else {
				if(mResetAudioPTS != -1) {
					pMediaBuffer->llTime -= mResetAudioPTS;
				} else {
					mResetAudioPTS = pMediaBuffer->llTime - (mLastAudioPTS + 20);
					pMediaBuffer->llTime = mLastAudioPTS + 20;
				}
			}
		} else if(aStreamType == EMediaTypeVideo) {
			if(mResetAudioPTS != -1) {
				if(pMediaBuffer->llTime < mResetAudioPTS) {
					pMediaBuffer->llTime = 0; 
				} else {
					pMediaBuffer->llTime -= mResetAudioPTS;
				}
				mFirstPTS = mResetAudioPTS;
				mResetVideoPTS = -1;
				mResetAudioPTS = -1;
			} else {
				if(mResetVideoPTS != -1) {
					pMediaBuffer->llTime -= mResetVideoPTS;
				} else {
					mResetVideoPTS = pMediaBuffer->llTime - (mLastVideoPTS + 30);
					pMediaBuffer->llTime = mLastVideoPTS + 30;
				}
			}
		}
	}
}

int CTTHLSInfoProxy::GetMediaSamplebyID(CLiveSession* aLiveSession, TTMediaType aStreamType, TTBuffer* pMediaBuffer, TTInt elementID)
{
	if(aLiveSession == NULL) {
		return TTKErrNotReady;
	}

	if(mFirstPTSValid) {
		pMediaBuffer->llTime += mFirstPTS;
	}

	TTInt nErr = aLiveSession->GetMediaSample(aStreamType, pMediaBuffer, elementID);
	if(nErr == TTKErrNone) {
		if(!mFirstPTSValid) {
			mFirstPTSValid = true;
			mFirstPTS = pMediaBuffer->llTime;
			if(mFirstTimeOffsetUs && IsSeekAble()) {
				mFirstPTS -= mFirstTimeOffsetUs;
			}
		} 

		if(aStreamType == EMediaTypeAudio) {
			if(pMediaBuffer->llTime - mFirstPTS > mLastAudioPTS + 10000 || pMediaBuffer->llTime - mFirstPTS < mLastAudioPTS - 10000) {
				upDateTimeStamp(aStreamType, pMediaBuffer);
			} else {
				pMediaBuffer->llTime -= mFirstPTS;
				if(pMediaBuffer->llTime < 0) {
					pMediaBuffer->llTime = 0;
				}
			}

			mLastAudioPTS = pMediaBuffer->llTime;
		} else if(aStreamType == EMediaTypeVideo) {
			if(pMediaBuffer->llTime - mFirstPTS > mLastVideoPTS + 10000 || pMediaBuffer->llTime - mFirstPTS < mLastVideoPTS - 10000) {
				upDateTimeStamp(aStreamType, pMediaBuffer);
			} else {
				pMediaBuffer->llTime -= mFirstPTS;
				if(pMediaBuffer->llTime < 0) {
					pMediaBuffer->llTime = 0;
				}
			}

			mLastVideoPTS = pMediaBuffer->llTime;
		}
		return TTKErrNone;
	} 

	return nErr;
}

TTUint CTTHLSInfoProxy::MediaDuration()
{
	return  mPlayListManager->getTotalDuration();
}

TTBool CTTHLSInfoProxy::IsCreateFrameIdxComplete()
{ 
	return ETTTrue;
}

TTUint CTTHLSInfoProxy::BufferedSize()
{
	return 0;
}

TTUint CTTHLSInfoProxy::ProxySize()
{
	return 0;
}

TTUint CTTHLSInfoProxy::BandWidth()
{
	TTInt nBandWidth = 0;
	mCriSession.Lock();
	if(mCurSession) {
		nBandWidth += mCurSession->getBandWidth();
	}

	if(mBASession) {
		nBandWidth += mBASession->getBandWidth();
	}
	mCriSession.UnLock();

	nBandWidth += mPL->GetBandWidth();

	return nBandWidth;
}

void CTTHLSInfoProxy::SetDownSpeed(TTInt aFast)
{
}

TTUint CTTHLSInfoProxy::BandPercent()
{
	if(mCurSession == NULL) {
		return 0;
	}

	return mCurSession->bandPercent(mCurMinBuffer);
}

TTInt CTTHLSInfoProxy::BufferedPercent(TTInt& aBufferedPercent)
{
	if(mPlayListManager->isLive()) {
		return TTKErrNotSupported;
	}

	GKCAutoLock lock(&mCriSession);
	if(mCurSession == NULL) {
		aBufferedPercent = 0;
		return TTKErrNotReady;
	}

	return mCurSession->bufferedPercent(aBufferedPercent);
}

TTInt CTTHLSInfoProxy::SelectStream(TTMediaType aType, TTInt aStreamId)
{
	if(aType == EMediaTypeAudio) {
		mAudioStreamID = aStreamId;
		return TTKErrNone;
	} else if(aType == EMediaTypeVideo){
		mVideoStreamID = aStreamId;
		return TTKErrNone;
	}
	
	return TTKErrNotSupported;
}

TTInt64 CTTHLSInfoProxy::Seek(TTUint64 aPosMS, TTInt aOption)
{
	if(mCurSession == NULL) {
		return TTKErrNotReady;
	}

	GKCAutoLock lock(&mCriSession);
	TTInt64	nSeekTime = aPosMS;
	if(!mPlayListManager->isLive()) {
		if(getBAStatus() == EBASwitchDownStart || getBAStatus() == EBASwitchUpStart) {
			setBAStatus(EBASwitchCancel);
			GKCAutoLock eventlock(&mCriEvent);
			postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
		} else if(getBAStatus() == EBASwitching || getBAStatus() == EBASwitched) {
			mCurBitrateIndex = mBABitrateIndex;
			mPlayListManager->setCurBitrateIndex(mCurBitrateIndex);

			mCriEvent.Lock();
			postInfoMsgEvent(0, ECheckCloseSession, 0, 0, (void *)mCurSession);
			mCriEvent.UnLock();

			mCriBA.Lock();
			mSwitchAudioStatus = EBASwitchNone;
			mSwitchVideoStatus = EBASwitchNone;
			mBASwitchStatus = EBASwitchNone;
			mCurItem = mBAItem;
			mCurSession = mBASession;
			mBASession = NULL;
			mCriBA.UnLock();
		} 
	}

	nSeekTime = mCurSession->seek(aPosMS, aOption, mFirstPTS);

	if(!mFirstPTSValid && nSeekTime > 0) {
		mFirstTimeOffsetUs = nSeekTime;
	}

	mLastAudioPTS = nSeekTime;
	mLastVideoPTS = nSeekTime;
    
    if(aOption) {
        nSeekTime = aPosMS;
    }
    
    mSeekTime = nSeekTime;
    mSeekOption = aOption;
    mSeeking = true;

	GKCAutoLock Lock(&mCriEvent);
	if(mBufferStatus == 0) {
		onInfoBufferStart(mCurSession);
		mBufferStatus = 1;
	}

	return nSeekTime;
}

TTInt CTTHLSInfoProxy::SetParam(TTInt aType, TTPtr aParam)
{
	return TTKErrNotSupported;
}

TTInt CTTHLSInfoProxy::GetParam(TTInt aType, TTPtr aParam)
{
	if(aParam == NULL) {
		return TTKErrArgument;
	}

	if(aType == TT_PID_COMMON_STATUSCODE) {
		if(mPL) {
			*((TTInt *)aParam) = mPL->GetStatusCode();
		} else {
			*((TTInt *)aParam) = 0;
		}
		return 0;
	}else if(aType == TT_PID_COMMON_HOSTIP) {
		if(mPL) {
			*((TTUint *)aParam) = mPL->GetHostIP();
		} else {
			*((TTUint *)aParam) = 0;
		}
		return 0;
	} else if(aType == TT_PID_COMMON_LIVEDMOE) {
		if(mPlayListManager->isLive()) {
			*((TTUint *)aParam) = 1;
		} else {
			*((TTUint *)aParam) = 0;
		}

		return 0;
	}

	return TTKErrNotSupported;
}

TTInt CTTHLSInfoProxy::isHeadReady(CLiveSession* aLiveSession, int checkAudio, int checkVideo)
{
	TTInt nReady = 0;
	TTInt nCount = 0;
	TTInt nMaxCount = 100;

	TTInt nNum = checkAudio + checkVideo;

	if(mPlayListManager->isVariantPlaylist()) {
		nMaxCount = 50;
	}

	do {
		nReady = aLiveSession->isHeaderReady();		

		if(mCancel) {
			break;
		}

		if(nReady < nNum) {
			nCount++;
			if(nCount > nMaxCount) {
				break;
			}
			mSemaphore.Wait(100);
		}
	} while(nReady < nNum);

	return nReady;
}

int CTTHLSInfoProxy::IsHLSSource(const TTChar* aUrl)
{
#ifndef __TT_OS_WINDOWS__
	return (strncasecmp("http://", aUrl, 7) == 0 && strstr(aUrl, ".m3u") != NULL);
#else
	return (strnicmp("http://", aUrl, 7) == 0 && strstr(aUrl, ".m3u") != NULL);
#endif
}

void CTTHLSInfoProxy::SetNetWorkProxy(TTBool aNetWorkProxy)
{
}

int  CTTHLSInfoProxy::doDownLoadList(const char* aUrl, char* actualUrl)
{
	GKCAutoLock Lock(&mCriPlayList);
	TTInt nErr = mPL->Open(aUrl);
	if(nErr < 0) {
		return nErr;
	}

	if(mPL->IsTransferBlock())
	{
		TTInt aPlayListSize;
		unsigned char* aPlayListBuffer = NULL;

		mPlayListSize = 0;
		do{
			aPlayListSize = mPL->RequireContentLength();
			if(aPlayListSize <=0)
				break;

			aPlayListBuffer = (unsigned char*)malloc(mPlayListSize + aPlayListSize);
			if(mPlayListBuffer) {
				memcpy(aPlayListBuffer, mPlayListBuffer, mPlayListSize);
				SAFE_DELETE(mPlayListBuffer);
			}

			nErr = mPL->GetBuffer((TTChar *)aPlayListBuffer + mPlayListSize, aPlayListSize);
			if(nErr != aPlayListSize) {
				return TTKErrAccessDenied;
			}

			mPlayListBuffer = aPlayListBuffer;
			mPlayListSize += aPlayListSize;
		}while(1);
	}
	else
	{
		mPlayListSize = mPL->ContentLength();
		SAFE_DELETE(mPlayListBuffer);
		mPlayListBuffer = (unsigned char*)malloc(mPlayListSize);
		memset(mPlayListBuffer, 0, mPlayListSize);

		nErr = mPL->GetBuffer((TTChar *)mPlayListBuffer, mPlayListSize);
		if(nErr != mPlayListSize) {
			return TTKErrAccessDenied;
		}
	}

	if(actualUrl) {
		strcpy(actualUrl, mPL->GetActualUrl());
	}

	mPL->Close();

	return TTKErrNone;
}

void CTTHLSInfoProxy::CancelReader()
{
	mCancel = ETTTrue;
	mSemaphore.Signal();

	mPL->Cancel();

	if(mCurSession) {
		mCurSession->cancel();
	}
	if(mBASession) {
		mBASession->cancel();
	}
}

TTInt CTTHLSInfoProxy::initPlayList(const TTChar* aUrl, TTInt aFlag)
{
	char url[4096];
	memset(url, 0, sizeof(char)*4096);
	TTInt nErr = doDownLoadList(aUrl, url);
	if(nErr < 0) {
		return nErr;
	}
		
	nErr = mPlayListManager->open(url, mPlayListBuffer, mPlayListSize);
	if(nErr < 0) {
		return nErr;
	}

	ListItem* pItem = NULL;
	if(mPlayListManager->isVariantPlaylist()) {
		int nIndex = mPlayListManager->getCurBitrateIndex();
		int isOK = 0;

		pItem = mPlayListManager->getListItem(nIndex, EMediaList, 0);
		if(pItem != NULL && updatePlayList(pItem) == TTKErrNone) {
			isOK = 1;
		}

		if(isOK <= 0) {
			if(nIndex - 1 >= 0) {
				nIndex -= 1;
			} else {
				nIndex += 1;
				if(nIndex >= mPlayListManager->getVariantNum()) {
					return TTKErrNotSupported;
				}
			}

			pItem = mPlayListManager->getListItem(nIndex, EMediaList, 0);
			nErr = updatePlayList(pItem);
			if(nErr < 0) {
				return nErr;
			}
		}

		mCurBitrateIndex = nIndex;
	} else {
		pItem = mPlayListManager->getListItem(0, EMediaList, 0);
	}

	mCurSession = getLiveSession();
	if(mCurSession == NULL) {
		return TTKErrNoMemory;
	}

	mCurItem = pItem;
	nErr = mCurSession->setUrlListItem(pItem);
	TTInt nseqNum = mPlayListManager->initSeqNum(pItem, 0);
	mCurSession->start(nseqNum);

	postPlayList(pItem, nErr, 0);

	return TTKErrNone;
}

CLiveSession* CTTHLSInfoProxy::getLiveSession()
{
	GKCAutoLock lock(&mCriSession);
	if(mListLSFree.empty()) {		
		return  new CLiveSession(this, mPlayListManager);
	}

	List<CLiveSession *>::iterator it = mListLSFree.begin();
	CLiveSession* aLiveSession = *it;
	mListLSFree.erase(it);

	return aLiveSession;
}

TTInt CTTHLSInfoProxy::putLiveSession(CLiveSession* aLiveSession)
{
	GKCAutoLock lock(&mCriSession);
	if(aLiveSession == NULL) {
		return TTKErrNone;
	}
	
	mListLSFree.push_back(aLiveSession);

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::freeLiveSession()
{
	GKCAutoLock lock(&mCriSession);

	List<CLiveSession *>::iterator it = mListLSFree.begin();
	while (it != mListLSFree.end()) {
		CLiveSession *LiveSession = *it;
		SAFE_DELETE(LiveSession);
		it = mListLSFree.erase(it);
	}

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::updatePlayList(ListItem* pItem)
{
	if(pItem == NULL) {
		return TTKErrArgument;
	}

	if(mPlayListManager->isComplete(pItem)) {
		return TTKErrNone;
	}

	char url[4096];
	memset(url, 0, sizeof(char)*4096);
	TTInt nErr = doDownLoadList(pItem->cUrl, url);
	if(nErr != TTKErrNone) {
		return nErr;
	}

	strcpy(pItem->cUrl, url);

	nErr = mPlayListManager->addPlayList(pItem, mPlayListBuffer, mPlayListSize);

	return nErr;
}

TTInt CTTHLSInfoProxy::postPlayList(ListItem* pItem, TTInt nErr, TTInt isBAItem)
{
	if(pItem == NULL)  {
		return TTKErrNone;
	}

	if(!mPlayListManager->isComplete(pItem)) {
		TTInt nDelayTime = mPlayListManager->getTargetDuration()/2;
		if(nDelayTime == 0) {
			nDelayTime = 1000;
		}

		if(nErr) {
			nDelayTime = 100;
			mLiveErrNum++;

			if(mLiveErrNum > 50) {
				DownLoadException(nErr, 0, 0);
				return nErr;
			}
		} else {
			mLiveErrNum = 0;
		}

		if(!mCancel) {
			GKCAutoLock lock(&mCriEvent);
			postInfoMsgEvent(nDelayTime, EMediaListUpdated, isBAItem, 0, pItem);
		}
	}

	return TTKErrNone;
}


void CTTHLSInfoProxy::SetObserver(TTObserver*	aObserver)
{
	GKCAutoLock lock(&mCriEvent);
	iObserver = aObserver;
}

TTInt CTTHLSInfoProxy::onInfoHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3)
{
	TTInt nErr = TTKErrNone;

	if(mCancel) {
		return TTKErrNone;
	}

	switch(nMsg) {
		case EMediaListUpdated:
			{
				if(nVar1 == 0) {
					nErr = updatePlayList(mCurItem);
					postPlayList(mCurItem, nErr, 0);
				} else if(nVar1 == 1){
					nErr = updatePlayList(mBAItem);
					if(getBAStatus() == EBASwitched) {
						postPlayList(mBAItem, nErr, 1);
					}
				}
			}
			break;
		case ECheckBandWidth:
			{
				nErr = onInfoBandWidth();
			}			
			break;
		case ECheckCPULoading:
			{
				nErr = onInfoCPULoading();
			}			
			break;
		case ECheckBufferStatus:
			{
				nErr = onInfoBufferStatus(nVar1);
			}
			break;
		case ECheckStartBASession:
			{
				nErr = onInfoStartBASession(nVar1, nVar2);
			}
			break;
		case ECheckBAStatus:
			{
				nErr = onInfoCheckBAStatus(nVar1, nVar2);
			}
			break;
		case ECheckCancelBASession:
			{
				nErr = onInfoCancelBASession(nVar1);
			}
			break;
		case ECheckCloseSession:
			{
				nErr = onInfoCloseSession((CLiveSession*)nVar3);
			}
			break;
	}

	return nErr;
}

TTInt CTTHLSInfoProxy::onInfoBufferStart(CLiveSession*	aLiveSession)
{
	BufferingStart(TTKErrNotReady, 0, 0);

	if(mPlayListManager->isLive()) {
		mCurMinBuffer = 4000;
	} else {
		TTInt nTarget = mPlayListManager->getTargetDuration();
		mCurMinBuffer = nTarget > 8000 ? nTarget : 8000;
	}
	GKCAutoLock lock(&mCriEvent);
	postInfoMsgEvent(50, ECheckBufferStatus, mCurMinBuffer, 0, (void *)aLiveSession);

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::onInfoBufferStatus(int nDuration)
{
	GKCAutoLock lock(&mCriSession);
	if(mCurSession == NULL){
		return TTKErrNone;
	}

	TTInt nErr = mCurSession->isBufferReady(nDuration, mAudioStreamID, mVideoStreamID);
	if(nErr == TTKErrNone || (getBAStatus() == EBASwitched && !mStartHLS)) {
		if(mStartHLS) {
			mStartHLS = 0;
		} 

		GKCAutoLock lock(&mCriEvent);
		BufferingDone();
		mBufferStatus = 0;		
	} else {
		//if(mPlayListManager->isVariantPlaylist() && nCheckBandWidth > nDuration/100) {
			//LOGI("onInfoBandWidth: nCheckBandWidth %d, mCurBitrateIndex %d", nCheckBandWidth, mCurBitrateIndex);
			//nErr = resetInitPlayList();
			//LOGI("onInfoBandWidth: nErr %d, nCheckBandWidth %d, mCurBitrateIndex %d", nErr, nCheckBandWidth, mCurBitrateIndex);
			//if(nErr) {
			//	nCheckBandWidth = 1;
			//} else {
			//	nErr = resetInitPlayList();
			//	if(nErr == 0) {
			//		DownLoadException(0, 0, 0);
			//	}
			//}

			//if(getBAStatus() == EBASwitchNone) {
			//	if(mCurBitrateIndex > 0) {
			//		mBABitrateIndex = 0;
			//	} else {
			//		mBABitrateIndex = 1;
			//	}

			//	LOGI("onInfoBandWidth:onInfoBufferStatus mCurBitrateIndex %d, mBABitrateIndex %d", mCurBitrateIndex, mBABitrateIndex);

			//	setBAStatus(EBASwitchDownStart);
			//	GKCAutoLock eventlock(&mCriEvent);
			//	postInfoMsgEvent(0, ECheckStartBASession, mBABitrateIndex, 2, 0);	
			//}
		//}

		//if(nCheckBandWidth > 0) {
		//	nCheckBandWidth++;
		//}

		GKCAutoLock lock(&mCriEvent);
		postInfoMsgEvent(100, ECheckBufferStatus, nDuration, 0, (void *)mCurSession);
	}

	return TTKErrNone;
}

void CTTHLSInfoProxy::setBAStatus(int aBAStatus)
{
	GKCAutoLock lock(&mCriBA);
	mBASwitchStatus = aBAStatus;
}

int CTTHLSInfoProxy::getBAStatus()
{
	GKCAutoLock lock(&mCriBA);
	TTInt nStatus = mBASwitchStatus;
	return nStatus;
}

TTInt CTTHLSInfoProxy::onInfoStartBASession(TTInt nIndex, TTInt upDown)
{
	int isOK = 0;
	ListItem* pItem = mPlayListManager->getListItem(nIndex, EMediaList, 0);
	if(pItem != NULL && updatePlayList(pItem) == TTKErrNone) {
		isOK = 1;
	}

	if(isOK == 0) {
		setBAStatus(EBASwitchNone);
		return TTKErrNone; 
	}

	mBAItem = pItem;

	mCriSession.Lock();
	TTInt nCurSeqNum = mCurSession->getCurrentSeqNum();
	TTInt nPercent = mCurSession->getCurChunkPercent();
	mCriSession.UnLock();

	TTInt nEstimateSeqNum = mPlayListManager->estimateSeqNumFromSeqNum(mBAItem, mCurItem, nCurSeqNum, nPercent);
	if(upDown == 0 && nPercent > 30) {
		nEstimateSeqNum++;
	}

	if(upDown == 1 && (nPercent < 10)) {
		nEstimateSeqNum--;  
	}

	LOGI("onInfoBandWidth: nCurSeqNum %d, nEstimateSeqNum %d, percent %d", nCurSeqNum, nEstimateSeqNum, nPercent);

	mBASession = getLiveSession();
	if(mBASession == NULL) {
		return TTKErrNoMemory;
	}

	mBASession->setUrlListItem(pItem);
	mBASession->start(nEstimateSeqNum);

	mCriBA.Lock();
	if(upDown == 0 || upDown == 2) {
		mSwitchAudioStatus = EBASwitchDownStart;
		mSwitchVideoStatus = EBASwitchDownStart;
	} else {
		mSwitchAudioStatus = EBASwitchUpStart;
		mSwitchVideoStatus = EBASwitchUpStart;
	}
	mCriBA.UnLock();

	GKCAutoLock lock(&mCriEvent);
	postInfoMsgEvent(100, ECheckBAStatus, 0, 0, 0);

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::onInfoCheckBAStatus(TTInt nStatus, TTInt nCount)
{
	GKCAutoLock lock(&mCriSession);
	if(mBASession == NULL) {
		return TTKErrNone;
	}

	if(nStatus == 0) {
		TTInt nReady = mBASession->isHeaderReady();

		if(nReady < 2) {
			GKCAutoLock lock(&mCriEvent);
			postInfoMsgEvent(100, ECheckBAStatus, 0, 0, 0);
		} else { 
			LOGI("onInfoBandWidth: BA check header ready");
			setBAStatus(EBASwitching);
			GKCAutoLock lock(&mCriEvent);
			postInfoMsgEvent(100, ECheckBAStatus, 1, 0, 0);
		}
	} else if(nStatus == 1) {
		TTInt nErr = TTKErrNone;
		TTInt64 nStartTime = -1;
		TTInt	nDuration = 0;
		TTInt64 nNewStartTime = 0;
		TTInt   nNewDuration = 0;

		if(mAudioStreamID > 0 && (mSwitchAudioStatus == EBASwitchDownStart || mSwitchAudioStatus == EBASwitchUpStart)) {
			nErr = mCurSession->getBufferStatus(true, &nStartTime, &nDuration);
			if(nErr == TTKErrNone) {
				nErr = mBASession->getBufferStatus(true, &nNewStartTime, &nNewDuration);
				if(nErr == TTKErrNone) {
					if(nNewStartTime < nStartTime) {
						LOGI("onInfoBandWidth: audio start time too small");
						LOGI("onInfoBandWidth: nNewStartTime %lld, nNewDuration %d, nStartTime %lld, nDuration %d",nNewStartTime, nNewDuration, nStartTime, nDuration);
						
						TTInt64 nTime = -1;
						if(nNewStartTime + nNewDuration > nStartTime + 200) {
							nTime = mBASession->seektoTime(true, nStartTime + 200);
						}

						LOGI("onInfoBandWidth: audio start time too small, seek to right point %lld", nTime);

						if(nTime > nStartTime && nTime < nStartTime + nDuration) {
							mCriBA.Lock();
							mSwitchAudioStatus = EBASwitching;
							mCriBA.UnLock();
						} else {
							if(mSwitchVideoStatus == EBASwitchUpStart) {
								setBAStatus(EBASwitchCancel);
								GKCAutoLock eventlock(&mCriEvent);
								postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
								return nErr;
							} else {
								if(mSwitchVideoStatus == EBASwitchDownStart && nNewStartTime + mPlayListManager->getTargetDuration() < nStartTime) {
									setBAStatus(EBASwitchCancel);
									GKCAutoLock eventlock(&mCriEvent);
									postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
									return nErr;									
								} else {
									nTime = mBASession->seektoTime(true, nStartTime);
								}
							}
						}						
					}

					if(nNewStartTime > nStartTime && nNewStartTime < nStartTime + nDuration) {
						mCriBA.Lock();
						mSwitchAudioStatus = EBASwitching;
						mCriBA.UnLock();
					}
				}				
			} else if(nErr == TTKErrEof) {
				if(mSwitchVideoStatus == EBASwitchDownStart || mSwitchVideoStatus == EBASwitchUpStart) {
					setBAStatus(EBASwitchCancel);
					GKCAutoLock eventlock(&mCriEvent);
					postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
					return nErr;
				}
			}
		}

		if(mVideoStreamID > 0 && (mSwitchVideoStatus == EBASwitchDownStart || mSwitchVideoStatus == EBASwitchUpStart)) {
			nErr = mCurSession->getBufferStatus(false, &nStartTime, &nDuration);
			if(nErr == TTKErrNone) {
				nErr = mBASession->getBufferStatus(false, &nNewStartTime, &nNewDuration);
				if(nErr == TTKErrNone) {
					if(nNewStartTime < nStartTime) {
						LOGI("onInfoBandWidth: video start time too small");
						LOGI("onInfoBandWidth: nNewStartTime %lld, nNewDuration %d, nStartTime %lld, nDuration %d",nNewStartTime, nNewDuration, nStartTime, nDuration);

						TTInt64 nTime = -1;
						TTInt nOffset = nDuration > 5000 ? 5000 : nDuration;
						if(nNewStartTime + nNewDuration > nStartTime + nOffset) {
							nTime = mBASession->seektoTime(false, nStartTime + nOffset);
						} 
						LOGI("onInfoBandWidth: video start time too small, seek to right point %lld", nTime);

						if(nTime > nStartTime && nTime < nStartTime + nDuration) {
							mCriBA.Lock();
							mSwitchVideoStatus = EBASwitching;
							mCriBA.UnLock();
						} else {
							if(mSwitchAudioStatus == EBASwitchUpStart) {
								setBAStatus(EBASwitchCancel);
								GKCAutoLock eventlock(&mCriEvent);
								postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
								return nErr;
							} else {
								if(mSwitchAudioStatus == EBASwitchDownStart && nNewStartTime +  mPlayListManager->getTargetDuration() < nStartTime) {
									setBAStatus(EBASwitchCancel);
									GKCAutoLock eventlock(&mCriEvent);
									postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
									return nErr;									
								} else {
									nTime = mBASession->seektoTime(false, nStartTime + mPlayListManager->getTargetDuration());
								}
							}
						}						
					}

					if(nNewStartTime > nStartTime && nNewStartTime < nStartTime + nDuration) {
						mCriBA.Lock();
						mSwitchVideoStatus = EBASwitching;
						mCriBA.UnLock();
					}
				}	
			} else if(nErr == TTKErrEof) {
				if(mSwitchAudioStatus == EBASwitchDownStart || mSwitchAudioStatus == EBASwitchUpStart) {
					setBAStatus(EBASwitchCancel);
					GKCAutoLock eventlock(&mCriEvent);
					postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
					return nErr;
				}
			}
		}

		if(mSwitchAudioStatus == EBASwitchDownStart || mSwitchVideoStatus == EBASwitchDownStart 
			|| mSwitchAudioStatus == EBASwitchUpStart || mSwitchVideoStatus == EBASwitchUpStart) {
			GKCAutoLock lock(&mCriEvent);
			postInfoMsgEvent(100, ECheckBAStatus, 1, 0, 0);
		} else {
			LOGI("onInfoBandWidth: buffer ready");
			setBAStatus(EBASwitched);
			if(!mStartHLS) {
				mCurSession->pause();
			}

			postPlayList(mBAItem, TTKErrNotReady, 1);

			if(mBABitrateIndex < mCurBitrateIndex) {
				return TTKErrNone;
			}

			TTInt64 nAStartTime = -1;
			TTInt64 nANewStartTime = -1;
			TTInt64 nVStartTime = -1;
			TTInt64 nVNewStartTime = -1;

			nErr = mCurSession->getBufferStatus(true, &nStartTime, &nDuration);
			if(nErr == TTKErrNone) {
				nAStartTime = nStartTime;
				nErr = mBASession->getBufferStatus(true, &nNewStartTime, &nNewDuration);
				if(nErr == TTKErrNone) {
					nANewStartTime = nNewStartTime;
				}
			}

			nErr = mCurSession->getBufferStatus(false, &nStartTime, &nDuration);
			if(nErr == TTKErrNone) {
				nVStartTime = nStartTime;
				nErr = mBASession->getBufferStatus(false, &nNewStartTime, &nNewDuration);
				if(nErr == TTKErrNone) {
					nVNewStartTime = nNewStartTime;
				}
			}

			if(nAStartTime != -1 && nANewStartTime != -1 && nVStartTime != -1 && nVNewStartTime != -1) {
				nStartTime = nAStartTime;
				if(nVStartTime > nStartTime) {
					nStartTime = nVStartTime;
				}
				
				nNewStartTime = nANewStartTime;
				if(nVNewStartTime < nNewStartTime) {
					nNewStartTime = nVNewStartTime;
				}
				
				TTInt nDiff = (TTInt)(nNewStartTime - nStartTime);

				if(nDiff > 8000) {
					TTInt nDiffCount = nDiff/1000;
					if(nDiffCount > 15) {
						nDiffCount -= 5;
					} else {
						nDiffCount -= 3;
					}
					LOGI("onInfoBandWidth: buffer ready nDiffCount %d", nDiffCount);
					GKCAutoLock lock(&mCriEvent);
					postInfoMsgEvent(2000, ECheckBAStatus, 2, nDiffCount, 0);
				}
			}
		}
	} else if(nStatus == 2) {
		if(nCount > 0 && mSwitchAudioStatus == EBASwitching && mSwitchVideoStatus == EBASwitching && mBASession != NULL) {
			TTInt nBandWidthBA = mBASession->getBandWidth()*8;
			TTInt nBandWidth = mCurSession->getBandWidth()*8;
			TTInt nBAIndex = mPlayListManager->checkBitrate(nBandWidthBA);		
			if(nBAIndex < mBABitrateIndex) {
				LOGI("onInfoBandWidth: nBandWidthBA %d, nBandWidth %d, nBAIndex %d, mBABitrateIndex %d", nBandWidthBA, nBandWidth, nBAIndex, mBABitrateIndex);
				if(mDownCount == 0) {
					mDownCount++;
				} else {
					mDownCount++;
					if(mDownCount > 5) {
						onInfoCancelBASession(0);
						mCurSession->resume();
						return TTKErrNone;
					}
				}
			} else {
				mDownCount = 0;
			}

			nCount--;
			GKCAutoLock lock(&mCriEvent);
			postInfoMsgEvent(1000, ECheckBAStatus, 2, nCount, 0);	
		}
	}

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::onInfoCancelBASession(TTInt nStatus)
{
	LOGI("onInfoBandWidth: cancel ba session");
	GKCAutoLock lock(&mCriSession);
	mCriBA.Lock();
	mSwitchAudioStatus = EBASwitchNone;
	mSwitchVideoStatus = EBASwitchNone;
	mBASwitchStatus = EBASwitchNone;
	mCriBA.UnLock();

	if(mBASession == NULL) {
		return TTKErrNone;
	}

	mBASession->cancel();
	mBASession->stop();

	putLiveSession(mBASession);
	mBASession = NULL;

	return TTKErrNone;
}

TTInt CTTHLSInfoProxy::onInfoCloseSession(CLiveSession*	aLiveSession)
{
	if(aLiveSession == NULL) {
		return TTKErrNone;
	}
	
	aLiveSession->cancel();
	aLiveSession->stop();

	putLiveSession(aLiveSession);
	return TTKErrNone;
}


TTInt CTTHLSInfoProxy::onInfoBandWidth()
{
	GKCAutoLock lock(&mCriSession);
	TTInt nErrA = TTKErrNone;
	TTInt nErrV = TTKErrNone;

	if(mCurSession == NULL) {
		GKCAutoLock eventlock(&mCriEvent);
		postInfoMsgEvent(5000, ECheckBandWidth, 0, 0, NULL);
		return TTKErrNone;
	}

	TTInt	nCurSeqNum = 0;
	TTInt   nBandWidth = 0;
	TTInt   nBandWidthBA = 0;
	TTInt64 nStartTimeA = -1;
	TTInt	nDurationA = 0;
	TTInt64 nStartTimeV = -1;
	TTInt	nDurationV = 0;
	TTInt	nTargetDuration = mPlayListManager->getTargetDuration();
	TTInt   nPercent = mCurSession->getCurChunkPercent();


	nBandWidth = mCurSession->getBandWidth();
	nCurSeqNum = mCurSession->getCurrentSeqNum();

	if(nBandWidth > 0) {
		mCurBandWidth = nBandWidth*8;
	}

	if(mBASession) {
		nBandWidthBA = mBASession->getBandWidth()*8;
	}

	//LOGI("onInfoBandWidth: mCurBandWidth %d, nBandWidthBA %d, nCurSeqNum %d, nPercent %d", mCurBandWidth, nBandWidthBA, nCurSeqNum, nPercent);

	if(mAudioStreamID > 0) {
		nErrA = mCurSession->getBufferStatus(true, &nStartTimeA, &nDurationA);
		//if(nErrA == TTKErrNone) {
		//	LOGI("onInfoBandWidth: a startTime %lld, Duration %d", nStartTimeA - mFirstPTS, nDurationA);
		//} else {
		//	LOGI("onInfoBandWidth: mAudioStreamID %d, buffer Duration %d, nErrA %d", mAudioStreamID, nDurationA, nErrA);
		//}
	}

	if(mVideoStreamID > 0) {
		nErrV = mCurSession->getBufferStatus(false, &nStartTimeV, &nDurationV);
		//if(nErrV == TTKErrNone) {
		//	LOGI("onInfoBandWidth: v startTime %lld, Duration %d", nStartTimeV - mFirstPTS, nDurationV);
		//} else {
		//	LOGI("onInfoBandWidth: mVideoStreamID %d, buffer Duration %d, nErrV %d", mVideoStreamID, nDurationV, nErrV);
		//}
	}

	TTInt nBAIndex = 0;
	if((nErrA == TTKErrNotFound || nDurationA < 12000) && ( nErrV == TTKErrNotFound || nDurationV < 12000)) {
		if(getBAStatus() == EBASwitchUpStart) {
			setBAStatus(EBASwitchCancel);
			GKCAutoLock eventlock(&mCriEvent);
			postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
		}
	}

	int limitation = nTargetDuration*2;
	if(limitation > 20000) {
		limitation = 20000;
	}

	if(limitation < 15000) {
		limitation = 15000;
	}

	if(getBAStatus() == EBASwitchNone && (nErrA == TTKErrNotFound || nDurationA < limitation) && ( nErrV == TTKErrNotFound || nDurationV < limitation)){
		nBAIndex = mPlayListManager->switchDown(mCurBandWidth);
		//LOGI("onInfoBandWidth: mCurBitrateIndex %d, nBAIndex %d, mDownCount %d, nPercent %d", mCurBitrateIndex, nBAIndex, mDownCount, nPercent);
		if(nBAIndex < mCurBitrateIndex) {
			if(mDownCount == 0) {
				mBABitrateIndex = nBAIndex;
				mDownCount++;
			} else {
				mDownCount++;
				if(nBAIndex < mBABitrateIndex) {
					mBABitrateIndex = nBAIndex;
				}

				ListItem* pItem = mPlayListManager->getListItem(mBABitrateIndex, EMediaList, 0);
				bool bSwitch = true;
				if(mPlayListManager->isComplete(pItem)) {
					bSwitch = (nPercent >= 80) || (nPercent < 30);
				} else {
					bSwitch = (nPercent > 60) || (nPercent < 5);
				}

				TTInt upDown = 0;
				if(mStartHLS) {
					upDown = 2;
					bSwitch = true;
				}

				if(mDownCount > 2 && (bSwitch)) {
					LOGE("onInfoBandWidth:canSwitchDown  %d, mCurBitrateIndex %d........", mBABitrateIndex, mCurBitrateIndex);
					LOGI("onInfoBandWidth: nDurationA %d, nDurationV %d, mCurBandWidth %d, nPercent %d", nDurationA, nDurationV, mCurBandWidth, nPercent);
					mDownCount = 0;
					setBAStatus(EBASwitchDownStart);
					GKCAutoLock eventlock(&mCriEvent);
					postInfoMsgEvent(0, ECheckStartBASession, mBABitrateIndex, upDown, 0);						
				}
			}
		} 
	}

	if((nErrA == TTKErrNotFound || nDurationA >= limitation) && (nErrV == TTKErrNotFound || nDurationV >= limitation)) {
		mDownCount = 0;
		if(getBAStatus() == EBASwitchDownStart) {
			setBAStatus(EBASwitchCancel);
			GKCAutoLock eventlock(&mCriEvent);
			postInfoMsgEvent(0, ECheckCancelBASession, mBABitrateIndex, 0, NULL);
		} else if(getBAStatus() == EBASwitchNone){
			nBAIndex = mPlayListManager->switchUp(mCurBandWidth);
			if(mUpCount == 0) {
				if(nBAIndex > mCurBitrateIndex) {
					mBABitrateIndex = nBAIndex;
					mUpCount++;
				}
			} else {
				if(nBAIndex > mCurBitrateIndex) {
					++mUpCount;
					if(nBAIndex < mBABitrateIndex) {
						mBABitrateIndex = nBAIndex;
					}
					if(mUpCount >= 10) {
						LOGE("onInfoBandWidth:canSwitchUp  %d, mCurBitrateIndex %d.......", mBABitrateIndex, mCurBitrateIndex);
						LOGI("onInfoBandWidth: nDurationA %d, nDurationV %d, mCurBandWidth %d", nDurationA, nDurationV, mCurBandWidth);
						setBAStatus(EBASwitchUpStart);
						mUpCount = 0;
						GKCAutoLock eventlock(&mCriEvent);
						postInfoMsgEvent(0, ECheckStartBASession, mBABitrateIndex, 1, NULL);
					}
				} else {
					mUpCount = 0;
				}
			}
		}
	} else {
		mUpCount = 0;
	}

	GKCAutoLock eventlock(&mCriEvent);
	postInfoMsgEvent(1000, ECheckBandWidth, 0, 0, NULL);

	return TTKErrNone; 
}

TTInt CTTHLSInfoProxy::onInfoCPULoading()
{
	return TTKErrNone; 
}

TTInt CTTHLSInfoProxy::postInfoMsgEvent(TTInt  nDelayTime, TTInt32 nMsg, int nParam1, int nParam2, void * pParam3)
{
	if (mMsgThread == NULL)
		return TTKErrNotFound;

	TTBaseEventItem * pEvent = mMsgThread->getEventByType(EEventStream);
	if (pEvent == NULL)
		pEvent = new TTCHLSProxyEvent (this, &CTTHLSInfoProxy::onInfoHandle, EEventStream, nMsg, nParam1, nParam2, pParam3);
	else
		pEvent->setEventMsg(nMsg, nParam1, nParam2, pParam3);
	mMsgThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}

void CTTHLSInfoProxy::BufferingStart(TTInt nErr, TTInt nStatus, TTUint32 aParam)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingStart, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::BufferingDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyBufferingDone, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::DNSDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyDNSDone, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::ConnectDone()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyConnectDone, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::HttpHeaderReceived()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyHttpHeaderReceived, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::PrefetchStart(TTUint32 aParam)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchStart, TTKErrNone, aParam, NULL);
}

void CTTHLSInfoProxy::PrefetchCompleted()
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyPrefetchCompleted, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::CacheCompleted(const TTChar* pFileName)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyCacheCompleted, TTKErrNone, 0, NULL);
}

void CTTHLSInfoProxy::DownLoadException(TTInt errorCode, TTInt nParam2, void *pParam3)
{
	GKCAutoLock lock(&mCriEvent);
	if(iObserver && iObserver->pObserver)
		iObserver->pObserver(iObserver->pUserData, ESrcNotifyException, errorCode, nParam2, pParam3);
}
