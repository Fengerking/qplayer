/**
* File : TTBaseVideoSink.cpp  
* Created on : 2014-9-1
* Author : yongping.lin
* Description : TTBaseVideoSink实现文件
*/

// INCLUDES
#include "GKOsalConfig.h"
#include "TTBaseVideoSink.h"
#include "TTSysTime.h"
#include "TTSleep.h"
#include "TTLog.h"

TTChar TTCBaseVideoSink::mVideoPath[256] = "";

#ifdef __TT_OS_IOS__
extern int gIsIphone6Up;
extern TTInt gIos8Above;
#endif

TTCBaseVideoSink::TTCBaseVideoSink(CTTSrcDemux* aSrcMux, TTCBaseAudioSink* aAudioSink, GKDecoderType aDecoderType)
: mSrcMux(aSrcMux)
, mAudioSink(aAudioSink)
, mDecoderType(aDecoderType)
, mCurPos(0)
, mCurBuffer(NULL)
, mEOS(false)
, mSeeking(false)
, mStartSeek(false)
, mTimeReset(false)
, mResetNum(0)
, mPlayStatus(EStatusStoped)
, mRenderNum(0)
, mFirstFrame(0)
, mBufferStatus(ETTBufferingInValid)
, mCPUType(6)
, mCPUNum(2)
, mHWDec(0)
, mRenderType(0)
, mMotionEnable(0)
, mTouchEnable(0)
, mObserver(NULL)
, mFrameDuration(0)
, mSeekOption(0)
, mDelayTime(-1)
, mStartSystemTime(0)
, mLastSystemTime(0)
, mLastVideoPlayTime(0)
, mLastVideoTime(-1)
, mRenderThread(NULL)
{
	mCritical.Create();
	mCritStatus.Create();
	mCritTime.Create();

	mVideoDecoder = new CTTVideoDecode(aSrcMux);

	mPlayRange.bEnable = false;
	mPlayRange.nStartTime = 0;
	mPlayRange.nStartTime = 0;

	memset(&mVideoFormat, 0, sizeof(mVideoFormat));
	memset(&mSinkBuffer, 0, sizeof(mSinkBuffer));
}

TTCBaseVideoSink::~TTCBaseVideoSink()
{
	close();
	SAFE_DELETE(mRenderThread);
	SAFE_DELETE(mVideoDecoder);

	mCritTime.Destroy();
	mCritStatus.Destroy();
	mCritical.Destroy();
}

TTInt TTCBaseVideoSink::open(TTVideoInfo* pCurVideoInfo)
{
	if(pCurVideoInfo) {
		mVideoFormat.Width = pCurVideoInfo->iWidth;
		mVideoFormat.Height = pCurVideoInfo->iHeight;
	}
	checkHWEnable();
    
#ifdef __TT_OS_IOS__
    if(!gIsIphone6Up) {
        if(mVideoFormat.Width * mVideoFormat.Height > 1920*1080) {
            return TTKErrNotSupported;
        }
    }
#endif

	GKCAutoLock Lock(&mCritical);
	TTInt nHWDec = mHWDec;

	TTInt nErr = mVideoDecoder->initDecode(pCurVideoInfo, mHWDec);
	if(nErr != TTKErrNone) {
		if(mHWDec != TT_VIDEODEC_SOFTWARE)	{
			mHWDec = nHWDec = TT_VIDEODEC_SOFTWARE;
			nErr = mVideoDecoder->initDecode(pCurVideoInfo, mHWDec);
			if(nErr != TTKErrNone)
				return nErr;
		} else {
			return nErr;
		}
	}

	mVideoDecoder->getParam(TT_PID_VIDEO_FORMAT, &mVideoFormat);

	if(mRenderThread == NULL)
		 mRenderThread =  new TTEventThread("TTVideo Render");

	setPlayStatus(EStatusStarting);

	nErr = newVideoView();
	if(nErr == TTKErrNone)
		setPlayStatus(EStatusPrepared);

	if(nHWDec != TT_VIDEODEC_SOFTWARE && mHWDec == TT_VIDEODEC_SOFTWARE) {
		nErr = mVideoDecoder->initDecode(pCurVideoInfo, mHWDec);	
	}

	mPlayRange.bEnable = false;
	mPlayRange.nStartTime = 0;
	mPlayRange.nStartTime = 0;

	mFirstFrame = 0;

	return nErr;
}

TTInt TTCBaseVideoSink::syncPosition(TTUint64 aPosition, TTInt aOption)
{
	if(mVideoDecoder)
		mVideoDecoder->syncPosition(aPosition, aOption);

	mCritTime.Lock();
	mCurPos = aPosition;
	mSeeking = true;
	mStartSeek = true;
	mRenderNum = 0;
	mStartSystemTime = 0;
	mEOS = false;
	mSeekOption = aOption;
	mCritTime.UnLock();

	LOGI("TTCBaseVideoSink::syncPosition   mCurPos %lld", mCurPos);
	return TTKErrNone;
}

TTInt TTCBaseVideoSink::start(TTBool aPause)
{
	GKCAutoLock Lock(&mCritical);

	if (getPlayStatus() == EStatusPlaying)
		return TTKErrNone;

	if (getPlayStatus() == EStatusStoped)
		return TTKErrGeneral;

	mCritTime.Lock();
	mEOS = false;
	mCurPos = 0;
	mRenderNum = 0;
	mCurBuffer = NULL;
	mStartSystemTime = 0;
	mLastVideoPlayTime = 0;
	mLastVideoTime = -1;
	mCritTime.UnLock();	

	if(mVideoDecoder) {
		TTInt nErr = mVideoDecoder->start();
		if(nErr != TTKErrNone) {
			if(mHWDec != TT_VIDEODEC_SOFTWARE) {
				mHWDec = TT_VIDEODEC_SOFTWARE;
				nErr = mVideoDecoder->initDecode(NULL, mHWDec);
				if(nErr != TTKErrNone)
					return nErr;

				nErr = mVideoDecoder->start();
			} else {
				return nErr;
			}
		}
	}

	if(mRenderThread)
		mRenderThread->start();
	
	if(!aPause) {
		setPlayStatus(EStatusPlaying);
		postVideoRenderEvent(-1);
	} else {
		setPlayStatus(EStatusPaused);
	}

	return TTKErrNone;
}

TTInt TTCBaseVideoSink::pause()
{
	GKCAutoLock Lock(&mCritical);

	if (getPlayStatus() == EStatusPlaying) {
		setPlayStatus(EStatusPaused);

		if(mVideoDecoder)
			mVideoDecoder->pause();
	}

	return TTKErrNone;
}

TTInt TTCBaseVideoSink::resume()
{
	GKCAutoLock Lock(&mCritical);

	if (getPlayStatus() == EStatusPaused) {
		mCritTime.Lock();
		mRenderNum = 0;
		mStartSystemTime = 0;
		mCritTime.UnLock();	

		if(mVideoDecoder)
			mVideoDecoder->resume();

		setPlayStatus(EStatusPlaying);
		postVideoRenderEvent(-1);
	}
    return TTKErrNone;
}

TTInt TTCBaseVideoSink::stop()
{
	GKCAutoLock Lock(&mCritical);

	setPlayStatus(EStatusStoped);

	mPlayRange.bEnable = false;
	mPlayRange.nStartTime = 0;
	mPlayRange.nStartTime = 0;
	mFirstFrame = 0;

	if(mRenderThread) {
		mRenderThread->stop();
	}
		
	if(mVideoDecoder)
		mVideoDecoder->stop();
	return TTKErrNone;
}

TTInt	 TTCBaseVideoSink::close()
{
	if (getPlayStatus() != EStatusStoped)
		stop();

	GKCAutoLock Lock(&mCritical);
	if(mVideoDecoder)
		mVideoDecoder->uninitDecode();

	mPlayRange.bEnable = false;
	mPlayRange.nStartTime = 0;
	mPlayRange.nStartTime = 0;

	SAFE_DELETE(mRenderThread);

	TTInt nErr = closeVideoView();

	return nErr;
}

TTInt TTCBaseVideoSink::setView(void * pView)
{
	GKCAutoLock Lock(&mCritical);
	mView = pView;

	return TTKErrNone;
}

void TTCBaseVideoSink::setPlayRange(TTUint aStartTime, TTUint aEndTime)
{
	GKCAutoLock Lock(&mCritical);
	mPlayRange.bEnable = true;
	mPlayRange.nStartTime = aStartTime;
	mPlayRange.nStopTime = aEndTime;	
}

void TTCBaseVideoSink::setDecoderType(GKDecoderType aDecoderType)
{
	GKCAutoLock Lock(&mCritical);
	if(mDecoderType != aDecoderType) {
		if(mDecoderType == EDecoderDefault && aDecoderType == EDecoderSoft) {
			mHWDec = 0;
			open(NULL);
		} else if(mDecoderType == EDecoderSoft && aDecoderType == EDecoderDefault) {
			checkHWEnable();
			open(NULL);
		}
	}
}

void TTCBaseVideoSink::checkHWEnable(unsigned int nCodecType) 
{
	if(mDecoderType == EDecoderSoft)
		mHWDec = 0;
}

TTInt TTCBaseVideoSink::flush()
{
	GKCAutoLock Lock(&mCritical);
	TTInt nErr = TTKErrNone;
	if(mRenderThread)
		mRenderThread->cancelAllEvent();

	if(mVideoDecoder)
		nErr = mVideoDecoder->flush();

	return nErr;
}

TTInt TTCBaseVideoSink::doRender()
{
//    static TTInt64 nSystemTime = 0;
    
//    if(mCurBuffer) {
//        TTInt64 nPlayingTime = getPlayTime();
//        LOGI("000nSystemTime %lld, FrameTime %lld, RenderTime %lld, mRenderNum %d, mDelayTime %d", GetTimeOfDay(),  mCurBuffer->Time, GetTimeOfDay() - nSystemTime, mRenderNum, mDelayTime);
//    }
    
	mDelayTime = -1;
	TTInt nErr = checkVideoRenderTime();
    TTInt64 nNowTime = GetTimeOfDay();
    if(nErr != TTKErrNone) {
        if(nErr == TTKErrNotReady && (mRenderType&ERenderGlobelView)) {
            if(nNowTime - mLastSystemTime >= 5) {
                redraw();
                mLastSystemTime = nNowTime;
                
//                if(mCurBuffer) {
//                    TTInt64 nPlayingTime = getPlayTime();
//                    LOGI("111nSystemTime %lld, nPlayingTime %lld, RenderTime %lld, mRenderNum %d", GetTimeOfDay(), nPlayingTime, GetTimeOfDay() - nSystemTime, mRenderNum);
//                    nSystemTime = GetTimeOfDay();
//                }
            }
            if(mDelayTime > 5) {
                mDelayTime = 5;
            }
        }
        return nErr;
    }

	mCritTime.Lock();
	TTBool bSeeking = mSeeking;
	TTBool bStartSeek = mStartSeek;
	TTInt SeekOption = mSeekOption;
	mCritTime.UnLock();

	TTInt64 nPlayingTime = getPlayTime();
	mSinkBuffer.Time = nPlayingTime;
	mSinkBuffer.nFlag = 0;
	if(bStartSeek) {
		mSinkBuffer.nFlag |= TT_FLAG_BUFFER_SEEKING;
	}
	if(mVideoDecoder) {
		nErr = mVideoDecoder->getOutputBuffer(&mSinkBuffer);
	}

	mCritTime.Lock();
	if(mStartSeek && bStartSeek)
		mStartSeek = false;
	mCritTime.UnLock();

	if(nErr != TTKErrNone) {
		if(nErr == TTKErrNotReady) {
			mDelayTime = 5;
		}
		if(nErr == TTKErrHardwareNotAvailable) {
			if(mCurBuffer) {
				mCurBuffer->Time = mSinkBuffer.Time;
			}
			mDelayTime = 5;
		}
        if(nErr == TTKErrInUse && (mRenderType&ERenderGlobelView)) {
            if(nNowTime - mLastSystemTime >= 5) {
                redraw();
                mLastSystemTime = nNowTime;
            }
        }
		return nErr;
	}

	mCritTime.Lock();
	if(!bSeeking && mSeeking) {
		mCritTime.UnLock();
		return TTKErrTimedOut;
	}
	mCurBuffer = &mSinkBuffer;
	TTInt64 nCurrentTime = mCurBuffer->Time;
	mCritTime.UnLock();

	nPlayingTime = getPlayTime();
	if(bSeeking && SeekOption) {
		if(nCurrentTime < nPlayingTime) {
			return TTKErrTimedOut;
		}
	}
#ifdef __TT_OS_IOS__
    if (gIos8Above == 0) {
        if(nCurrentTime + 100 < nPlayingTime) {
            if(!mDropFrame)	{
                mDropFrame = true;
                return TTKErrTimedOut;
            }
        }
    }
#else
	if(nCurrentTime + 100 < nPlayingTime) {
		if(!mDropFrame)	{
			mDropFrame = true;
			return TTKErrTimedOut;
		}
	}
#endif

	if(mRenderNum >= 1) {
		if((nCurrentTime == 0 || mLastVideoTime == nCurrentTime) && nPlayingTime - nCurrentTime < 250) {
			return TTKErrTimedOut;
		}
	}

	mLastVideoTime = nCurrentTime;
	mLastVideoPlayTime = nPlayingTime;
    mLastSystemTime = GetTimeOfDay();
	mDropFrame = false;
    mDelayTime = 5;
    
	render();
    
//    LOGI("222nSystemTime %lld, nPlayingTime %lld, RenderTime %lld, mRenderNum %d", GetTimeOfDay(), nPlayingTime, GetTimeOfDay() - nSystemTime, mRenderNum);
//    nSystemTime = GetTimeOfDay();

	mCritTime.Lock();
	if(mRenderNum == 0) {
		if(!bSeeking && mSeeking) {
			mCritTime.UnLock();
			return TTKErrNone;
		}
		mRenderNum++;
		mCritTime.UnLock();
		checkSeekingStatus();

		if(mAudioSink) {
			if(getPlayStatus() == EStatusPlaying) {
				mAudioSink->startOne(-1);
			}
		}
	} else {
		mRenderNum++;
		mCritTime.UnLock();
	}

	if(mFirstFrame == 0) {
		if(mObserver) {
			mVideoDecoder->getParam(TT_PID_VIDEO_FORMAT, &mVideoFormat);
			if(mObserver) {
				mObserver->pObserver(mObserver->pUserData, ENotifyVideoFormatChanged, mVideoFormat.Width, mVideoFormat.Height, NULL);
				mObserver->pObserver(mObserver->pUserData, ENotifyMediaFirstFrame, TTKErrNone, 0, NULL);
			}
		}
		mFirstFrame = 1;
	}

	return TTKErrNone;
}

TTInt TTCBaseVideoSink::render()
{
	return TTKErrNone;
}

TTInt TTCBaseVideoSink::redraw()
{
 	return TTKErrNone;
}

TTInt TTCBaseVideoSink::onRenderVideo (TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3)
{
	TTInt nErr = TTKErrNone;
	if(isEOS()) {
		if(mObserver) {
			mObserver->pObserver(mObserver->pUserData, ENotifyComplete, TTKErrNone, 0, NULL);
		}		
		return TTKErrNone;
	}

	if(mVideoDecoder == NULL) {
		if(getPlayStatus() == EStatusPlaying && !isEOS()) {
			postVideoRenderEvent (10);
		}
		return TTKErrNotReady;
	}

	if(!mSeeking && mFirstFrame && mAudioSink ) {
		if(mAudioSink->getBufferStatus() == ETTBufferingStart) {
            int nDelayTime = 10;
            if(mRenderType&ERenderGlobelView) {
                redraw();
                nDelayTime = 5;
            }
            postVideoRenderEvent (nDelayTime);
			return TTKErrNotReady;
		}
	}

	nErr = doRender();

	if (nErr == TTKErrFormatChanged) {
		videoFormatChanged();
	}else if (nErr == TTKErrEof) {
		setEOS();
	}

	if (mPlayRange.bEnable)	{
		if(mSinkBuffer.Time >= mPlayRange.nStopTime) {
			setEOS();
			nErr = TTKErrEof;
		}
	}

	if(isEOS()) {
		if(mObserver) {
			mObserver->pObserver(mObserver->pUserData, ENotifyComplete, TTKErrNone, 0, NULL);
		}
	} else {
		if (getPlayStatus() == EStatusPlaying || mSeeking || getPlayStatus() == EStatusPaused) {
			postVideoRenderEvent (mDelayTime);
		}
	}

	return nErr;
}

TTInt TTCBaseVideoSink::postVideoRenderEvent (TTInt  nDelayTime)
{
	if (mRenderThread == NULL)
		return TTKErrNotFound;
    
    if( mRenderThread->getFullEventNum(EEventVideoRender) > 0)
        return TTKErrNone;

	TTBaseEventItem * pEvent = mRenderThread->getEventByType(EEventVideoRender);
	if (pEvent == NULL)
		pEvent = new TTCVideoRenderEvent (this, &TTCBaseVideoSink::onRenderVideo, EEventVideoRender);
	mRenderThread->postEventWithDelayTime (pEvent, nDelayTime);

	return TTKErrNone;
}

void TTCBaseVideoSink::setRendType(TTInt aRenderType)
{
    mRenderType = aRenderType;
}

void TTCBaseVideoSink::setMotionEnable(bool aEnable)
{
    mMotionEnable = aEnable;
}

void TTCBaseVideoSink::setTouchEnable(bool aEnable)
{
    mTouchEnable = aEnable;
}

void TTCBaseVideoSink::setPlayStatus(GKPlayStatus aStatus)
{
	LOGI("TTCBaseVideoSink::SetPlayStatus %d", aStatus);
	GKCAutoLock Lock(&mCritStatus);

	mPlayStatus = aStatus;
}

GKPlayStatus TTCBaseVideoSink::getPlayStatus()
{
	GKPlayStatus status;
	mCritStatus.Lock();
	status = mPlayStatus;
	mCritStatus.UnLock();

	return status;
}

TTInt TTCBaseVideoSink::drawBlackFrame()
{
	return TTKErrNone;
}

TTInt TTCBaseVideoSink::setBufferStatus(TTBufferingStatus aBufferStatus)
{
	GKCAutoLock Lock(&mCritStatus);
	mBufferStatus = aBufferStatus;

	return TTKErrNone;
}


TTBufferingStatus TTCBaseVideoSink::getBufferStatus()
{
	GKCAutoLock Lock(&mCritStatus);
	TTBufferingStatus nBufferStatus = mBufferStatus;

	return nBufferStatus;
}


TTBool TTCBaseVideoSink::isEOS()
{
	GKCAutoLock Lock(&mCritStatus);

	TTBool bEOS = mEOS;

	return bEOS;
}

TTInt TTCBaseVideoSink::setParam(TTInt aID, void* pValue)
{
	GKCAutoLock Lock(&mCritical);
	TTInt nErr = TTKErrNotFound;

	if(aID == TT_PID_VIDEO_TIMERESET) {
		mTimeReset = true;
		return TTKErrNone;
	}else if(aID == TT_PID_VIDEO_SEEKOPTION) {
		if(pValue)
			mSeekOption = *((TTInt *)pValue);
		return TTKErrNone;
	}

	if(mVideoDecoder)
		nErr = mVideoDecoder->setParam(aID, pValue);

	return nErr;
}

TTInt TTCBaseVideoSink::getParam(TTInt aID, void* pValue)
{
	GKCAutoLock Lock(&mCritical);
	TTInt nErr = TTKErrNotFound;
	if(mVideoDecoder)
		nErr = mVideoDecoder->getParam(aID, pValue);

	return nErr;
}

TTInt64 TTCBaseVideoSink::getPlayTime()
{
	GKPlayStatus PlayStatus = getPlayStatus();
	TTBufferingStatus nBufferStatus = getBufferStatus();

	GKCAutoLock Lock(&mCritTime);
	TTInt64 nPosition = 0;

	if(mAudioSink){
		nPosition = mAudioSink->getPlayTime();
		return nPosition;
	}

	if(mCurBuffer == NULL) {
		return mCurPos;
	}
	
	if(mEOS) {
		return mCurBuffer->Time;
	}

	if(mStartSystemTime == 0) {
		mStartSystemTime = GetTimeOfDay() - mCurBuffer->Time;
	}

	if(PlayStatus == EStatusStarting || PlayStatus == EStatusStoped || PlayStatus == EStatusPrepared) {
		nPosition = 0;
	} else if(PlayStatus == EStatusPaused || nBufferStatus == ETTBufferingStart) {
		nPosition = mCurBuffer->Time;
	} else {
		nPosition = GetTimeOfDay() - mStartSystemTime;
	}

	return nPosition;
}

TTInt TTCBaseVideoSink::newVideoView()
{
	return TTKErrNone;
}


TTInt TTCBaseVideoSink::closeVideoView()
{
	return TTKErrNone;
}

void TTCBaseVideoSink::setObserver(TTObserver*	aObserver)
{
	GKCAutoLock Lock(&mCritical);
	mObserver = aObserver;
}

void TTCBaseVideoSink::setAudioSink(TTCBaseAudioSink* aAudioSink)
{
   mAudioSink = aAudioSink;
}

void TTCBaseVideoSink::setEOS()
{
	TTInt nRenderNum = 0;
	mCritStatus.Lock();
	mEOS = true;
	nRenderNum = mRenderNum;
	mCritStatus.UnLock();
	
	if(mAudioSink && nRenderNum == 0) {
		if(getPlayStatus() == EStatusPlaying)
			mAudioSink->startOne(-1);
	}

	checkSeekingStatus();
}

void TTCBaseVideoSink::videoFormatChanged()
{
	TTVideoFormat VideoFormat;
	
	memcpy(&VideoFormat, &mVideoFormat, sizeof(VideoFormat));

	if(mVideoDecoder)
		mVideoDecoder->getParam(TT_PID_VIDEO_FORMAT, &VideoFormat);

	if(VideoFormat.Width != mVideoFormat.Width || VideoFormat.Height != mVideoFormat.Height) {
		
		memcpy(&mVideoFormat, &VideoFormat, sizeof(VideoFormat));
		if(mObserver) {
			mObserver->pObserver(mObserver->pUserData, ENotifyVideoFormatChanged, VideoFormat.Width, VideoFormat.Height, NULL);
		}

		if(mHWDec == TT_VIDEODEC_SOFTWARE)
			newVideoView();
	}
}

TTInt TTCBaseVideoSink::checkVideoRenderTime ()
{
	mCritTime.Lock();
	if (mRenderNum > 0)	{
		if (mCurBuffer == NULL) {
			mCritTime.UnLock();
			return TTKErrNotReady;
		}

		TTInt64 nCurFrameTime = mCurBuffer->Time;
		TTInt64 nPlayingTime = getPlayTime();
		mCritTime.UnLock();

		if (nPlayingTime <= 0)	{
			mDelayTime = 2;
			return TTKErrNotReady;
		}

		if (nCurFrameTime > 0 && ((abs((int)(nPlayingTime - nCurFrameTime)) > 120000) || mTimeReset) )	{
			if(mTimeReset) {
				mResetNum++;
				if(mResetNum >= 30) {
					mResetNum = 0;
					mTimeReset = false;
				}
			}

			mDelayTime = 30;
		} else {
			if (nPlayingTime < nCurFrameTime && mRenderNum > 1)	{
				mDelayTime = nCurFrameTime - nPlayingTime;
				if(mDelayTime > 10)
					mDelayTime = 10;
				return TTKErrNotReady;
			}
		}
	} else {
		mCritTime.UnLock();
	}

	if(isEOS())
		return TTKErrEof;

	return TTKErrNone;
}

TTInt TTCBaseVideoSink::startOne(TTInt nDelaytime)
{
	if(mVideoDecoder)
		mVideoDecoder->setParam(TT_PID_VIDEO_PREVIEW, &mSeeking);

	postVideoRenderEvent (nDelaytime);
	return TTKErrNone;
}

void TTCBaseVideoSink::setPluginPath(const TTChar* aPath)
{
	if (aPath != NULL && strlen(aPath) > 0)
	{
		memset(mVideoPath, 0, sizeof(TTChar)*256);
		strcpy(mVideoPath, aPath);
	}
}

void TTCBaseVideoSink::checkCPUFeature()
{
	if(mVideoDecoder)
		mVideoDecoder->setParam(TT_PID_VIDEO_CPU_TYPE, &mCPUType);
}

void TTCBaseVideoSink::checkSeekingStatus()
{
	TTBool bSeeking = false;
	mCritTime.Lock();
	bSeeking = mSeeking;
	mCritTime.UnLock();

	if(bSeeking) {
		mCritTime.Lock();
		mSeeking = false;
		bSeeking = mSeeking;
		mCritTime.UnLock();
		if(mVideoDecoder)
			mVideoDecoder->setParam(TT_PID_VIDEO_PREVIEW, &bSeeking);

		if(mObserver) {
			mObserver->pObserver(mObserver->pUserData, ENotifySeekComplete, TTKErrNone, 0, NULL);
		}
	}
}

#ifdef __TT_OS_IOS__
void TTCBaseVideoSink::SetRotate(){
    redraw();
}
#endif
