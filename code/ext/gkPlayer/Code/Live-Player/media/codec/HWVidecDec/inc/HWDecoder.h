/**
* File : TTHWDecoder.h  
* Created on : 2014-11-5
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : CTTHWDecoder定义文件
*/

#ifndef __TTHWDECODER_H__
#define __TTHWDECODER_H__

#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <new>
#include <android/log.h>

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#include "TTVideo.h"
#include "AVCDecoderTypes.h"
#include "M4a.h"
#include "ttHWVideoDec.h"

using namespace android;

class CTTHWDecoder
{
public:

	CTTHWDecoder(TTUint aCodecType);
	virtual ~CTTHWDecoder();

public:
	virtual TTInt							initDecode();

	virtual TTInt							uninitDecode();

	virtual	TTInt							start();

	virtual	TTInt							stop();

	virtual TTInt							getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo);

	virtual TTInt							setParam(TTInt aID, void* pValue);

	virtual TTInt							getParam(TTInt aID, void* pValue);

	virtual TTInt							renderOutputBuffer(TTVideoBuffer* DstBuffer);

	virtual	TTObserver*						getObserver();
	
	virtual TTInt							getConfigData(TTBuffer* DstBuffer);

private:
	TTUint								mVideoCodec;
	TTBool								mEOS;
	TTBool								mStarted;
	TTBool								mCreated;
	TTBool								mConnected;
	TTObserver							*mObserver;
	TTVideoFormat						mVideoFormat;
	unsigned char *						mHeadBuffer;
	int									mHeadSize;
	unsigned char *						mHeadNalBuffer;
	int									mHeadNalSize;
	int									mSeeking;
	TT_COLORTYPE						mColorType;

	sp<MediaSource>						mSource;
	sp<MediaSource>						mDecoder;
	MediaBuffer							*mBuffer;
	sp<ANativeWindow> 					mNativeWindow;	
	int32_t								mColorFormat;
	int32_t								mUseSurfaceAlloc;
    OMXClient							*mClient;
    const char							*mCompName;
	
};

#endif //__TTHWDECODER_H__
