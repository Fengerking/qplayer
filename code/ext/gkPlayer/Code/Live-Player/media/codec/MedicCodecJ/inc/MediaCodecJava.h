/**
* File : MediaCodecJava.h 
* Created on : 2015-1-1
* Author : yongping.lin
* Copyright : Copyright (c) 2014 GoKu Software Ltd. All rights reserved.
* Description : MediaCodecJava定义文件
*/

#ifndef __MEDIACODECJAVA_H__
#define __MEDIACODECJAVA_H__

#include "jni.h"
#include "TTVideo.h"
#include "AVCDecoderTypes.h"
#include "M4a.h"

class CMediaCodecJava
{
public:

	CMediaCodecJava(TTUint aCodecType);
	virtual ~CMediaCodecJava();

public:
	virtual TTInt							initDecode(void *object);

	virtual TTInt							uninitDecode();

	virtual	TTInt							start();

	virtual	TTInt							stop();

	virtual TTInt							setInputBuffer(TTBuffer *InBuffer);

	virtual TTInt							getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo);

	virtual TTInt							setParam(TTInt aID, void* pValue);

	virtual TTInt							getParam(TTInt aID, void* pValue);

	virtual TTInt							renderOutputBuffer(TTVideoBuffer* DstBuffer, TTBool bRender);

private:
	virtual TTInt							isSupportAdpater(jstring jstr);
	virtual TTInt							updateMCJFunc();
	virtual TTInt							updateBuffers();
	virtual TTInt							setConfigData();
	virtual TTInt							setCSData();
	virtual TTInt							setCSDataJava(unsigned char *pBuf, int nLen, int index);

private:
	TTUint								mVideoCodec;
	TTBool								mEOS;
	TTBool								mStarted;
	TTBool								mAllocated;
	TTBool								mNewStart;
	TTBool								mAdaptivePlayback;
	TTVideoFormat						mVideoFormat;
	unsigned char *						mHeadBuffer;
	int									mHeadSize;
	unsigned char *						mHeadConfigBuffer;
	int									mHeadConfigSize;
	unsigned char *						mSpsBuffer;
	int									mSpsSize;
	unsigned char *						mPpsBuffer;
	int									mPpsSize;

	TTInt								mVideoIndex;

	JavaVM*								mJVM;
	jobject								mSurfaceObj;
	jobject								mMediaCodec;
    jobject								mBufferInfo;
	jobject								mVideoFormatObj;
    jobjectArray						mInputBuffers;
	jobjectArray						mOutputBuffers;


    jclass								mMediaCodecClass;
    jclass								mMediaFormatClass;
    jclass								mBufferInfoClass; 
	jclass								mByteBufferClass;
    
	jmethodID							mToString;
    jmethodID							mCreateByCodecType;
	jmethodID							mConfigure;
	jmethodID							mStart;
	jmethodID							mStop;
	jmethodID							mFlush;
	jmethodID							mRelease;
    jmethodID							mGetOutputFormat;
    jmethodID							mGetInputBuffers;
    jmethodID							mGetOutputBuffers;
    jmethodID							mDequeueInputBuffer;
	jmethodID							mDequeueOutputBuffer;
	jmethodID							mQueueInputBuffer;
    jmethodID							mReleaseOutputBuffer;
    jmethodID							mCreateVideoFormat;
	jmethodID							mSetInteger;
	jmethodID							mSetByteBuffer;
	jmethodID							mGetInteger;
    jmethodID							mBufferInfoConstructor;
    jfieldID							mSizeField;
	jfieldID							mOffsetField;
	jfieldID							mPtsField;	
};

#endif //__TTHWDECODER_H__
