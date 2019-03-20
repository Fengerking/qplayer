#ifndef __TT_BASE_VIDEO_SINK_H__
#define __TT_BASE_VIDEO_SINK_H__

// INCLUDES
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include "GKCritical.h"
#include "TTEventThread.h"
#include "GKMediaPlayerItf.h"
#include "TTMediainfoDef.h"
#include "TTSrcDemux.h"
#include "TTBaseAudioSink.h"
#include "TTVideoDecode.h"

// CLASSES DECLEARATION
class TTCBaseVideoSink
{
public:

	TTCBaseVideoSink(CTTSrcDemux* aSrcMux, TTCBaseAudioSink* aAudioSink, GKDecoderType aDecoderType);

	virtual ~TTCBaseVideoSink();

public:

	virtual TTInt					open(TTVideoInfo* pCurVideoInfo);

	virtual	TTInt					setView(void * pView);

	virtual TTInt					flush();
    
    virtual TTInt                   close();

	virtual	TTInt					start(TTBool aPause = false);

	virtual	TTInt					pause();

	virtual	TTInt 					resume();

	virtual	TTInt					stop();

	virtual	TTInt					syncPosition(TTUint64 aPosition, TTInt aOption = 0);

	virtual TTInt					render();
    
    virtual TTInt					redraw();

	virtual TTBool					isEOS();

	virtual TTInt					doRender();

	virtual TTInt					newVideoView();

	virtual TTInt					closeVideoView();

	virtual TTInt					drawBlackFrame();

	virtual TTInt					setParam(TTInt aID, void* pValue);

	virtual TTInt					getParam(TTInt aID, void* pValue);

	virtual void					setPlayRange(TTUint aStartTime, TTUint aEndTime);

	virtual TTInt					onRenderVideo (TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	virtual TTInt					postVideoRenderEvent (TTInt  nDelayTime);

	virtual TTInt64					getPlayTime();

	virtual TTInt					setBufferStatus(TTBufferingStatus aBufferStatus);

	virtual void					setDecoderType(GKDecoderType aDecoderType);

	virtual TTBufferingStatus		getBufferStatus();

	GKPlayStatus				    getPlayStatus();

	void							setPlayStatus(GKPlayStatus aStatus);

	virtual void					setObserver(TTObserver*	aObserver);
    
    virtual void					setAudioSink(TTCBaseAudioSink* aAudioSink);

	virtual	TTInt					startOne(TTInt nDelaytime);

	virtual void					setEOS();

	virtual void				    checkCPUFeature();
	virtual void					checkHWEnable(unsigned int nCodecType = 0);
	static void						setPluginPath(const TTChar* aPath);
    
    virtual void                    setRendType(TTInt aRenderType);
    
    virtual void                    setMotionEnable(bool aEnable);
    
    virtual void                    setTouchEnable(bool aEnable);
    
#ifdef __TT_OS_IOS__
    virtual void					SetRotate();
#endif
    
protected:	
	virtual void					videoFormatChanged();
	virtual void					checkSeekingStatus();
	virtual TTInt					checkVideoRenderTime();
	

	
protected:
	RGKCritical						mCritical;
	RGKCritical						mCritStatus;
	CTTSrcDemux*					mSrcMux;
	TTUint64						mCurPos;
	TTBool							mEOS;
	TTBool							mDropFrame;
	TTBool							mSeeking;
	TTBool							mStartSeek;
	TTBool							mTimeReset;
	TTInt							mResetNum;
	TTVideoBuffer					mSinkBuffer;
	TTVideoBuffer*					mCurBuffer;
	GKPlayStatus					mPlayStatus;
	TTVideoFormat					mVideoFormat;
	TTInt							mRenderNum;
	TTInt							mFirstFrame;
	TTBufferingStatus				mBufferStatus;
	TTInt32							mFrameDuration;
	TTInt							mSeekOption;
	TTInt							mDelayTime;
	TTPlayRange						mPlayRange;

	TTInt							mCPUType;
	TTInt							mCPUNum;
	TTInt							mHWDec;
	GKDecoderType					mDecoderType;
    
    TTInt                           mRenderType;
    TTBool                          mMotionEnable;
    TTBool                          mTouchEnable;

	TTObserver*						mObserver;
	void*							mView;

	CTTVideoDecode*					mVideoDecoder;
	TTCBaseAudioSink*				mAudioSink;

	RGKCritical						mCritTime;
	TTUint64						mStartSystemTime;
    TTUint64						mLastSystemTime;
	TTUint64						mLastVideoPlayTime;
	TTInt64							mLastVideoTime;

	TTEventThread*					mRenderThread;

	static TTChar					mVideoPath[256];
};

class TTCVideoRenderEvent : public TTBaseEventItem
{
public:
    TTCVideoRenderEvent(TTCBaseVideoSink * pRender, TTInt (TTCBaseVideoSink::* method)(TTInt, TTInt,TTInt,void*),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0, void* nVar3 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2, nVar3)
	{
		mVideoRender = pRender;
		mMethod = method;
    }

    virtual ~TTCVideoRenderEvent()
	{
	}

    virtual void fire (void) 
	{
        (mVideoRender->*mMethod)(mMsg, mVar1, mVar2, mVar3);
    }

protected:
    TTCBaseVideoSink *		mVideoRender;
    int (TTCBaseVideoSink::* mMethod) (TTInt, TTInt, TTInt, void*);
};

#endif  //__TT_BASE_VIDEO_SINK_H__
