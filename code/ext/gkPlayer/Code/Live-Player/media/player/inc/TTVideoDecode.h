/**
* File : TTVideoDecode.h
* Created on : 2014-9-1
* Author : yongping.lin
* Description : TTVideoDecode定义文件
*/

#ifndef __TT_TTVIDEODECODER_H__
#define __TT_TTVIDEODECODER_H__

// INCLUDES
#include "TTVideoPlugin.h"
#include "GKMediaPlayerItf.h"
#include "TTMediadef.h"
#include "TTSrcDemux.h"
#include "GKCritical.h"

//#define __DUMP_H264__

class CTTVideoDecode
{
public:

	CTTVideoDecode(CTTSrcDemux*	aSrcMux);
	virtual ~CTTVideoDecode();

public:
	virtual TTInt							initDecode(TTVideoInfo* pCurVideoInfo, TTInt aHwDecoder = 0);

	virtual TTInt							uninitDecode();

	virtual	TTInt							start();

	virtual	TTInt							stop(TTBool aDecoder = false);

	virtual	TTInt							pause();

	virtual	TTInt							resume();

	virtual TTInt							flush();

	virtual	TTInt							syncPosition(TTUint64 aPosition, TTInt aOption = 0);
	
	virtual TTInt							getOutputBuffer(TTVideoBuffer* DstBuffer);

	virtual TTInt							setParam(TTInt aID, void* pValue);

	virtual TTInt							getParam(TTInt aID, void* pValue);

public:
	static	TTInt		 					videoHWCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);

	virtual	TTInt		 					handleHWCallBack (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);

	virtual TTInt							getHWOutputBuffer(TTVideoBuffer* DstBuffer);

private:

	virtual TTInt							initConfig();
	virtual void							setEOS(TTBool bEOS);
	virtual TTBool							getEOS();

	virtual bool							checkRefFrame(TTBuffer*	pBuffer);

private:
	CTTSrcDemux*							mSrcMux;
	CTTVideoPluginManager*					mPluginManager;

	TTInt									mVideoCodec;
	TTInt									mCPUNum;
	TTInt									mCPUType;
	TTInt									mOutputNum;
	TTBuffer*								mCurBuffer;	
	TTBuffer								mSrcBuffer;
	TTInt									mNewCodec;
    TTInt                                   mDropFrame;
	TTBool									mEOS;
	TTVideoFormat							mVideoFormat;
	RGKCritical								mCritical;
	RGKCritical								mCriStatus;
	GKPlayStatus							mStatus;
	TTBool									mPreView;
	TTBool									mStartSeek;
	TTInt64									mDelayTime;
	TTInt									mDeblock;
	TTInt									mFlag;
	TTInt									mHwDecoder;
	TTInt									mHwTimeCount;
    TTInt                                   mLastFailed;

	RTTSemaphore							mSemaphore;

	TTObserver								mVideoDecObserver;

#ifdef __DUMP_H264__
	FILE*									DumpFile;
#endif
};

#endif //__TT_TTCAUDIODECODER_H__
