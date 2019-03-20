/**
* File : HWDecoder.h
* Created on : 2015-4-8
* Author : Kevin
* Copyright : Copyright (c) 2015 Kevin Software Ltd. All rights reserved.
* Description : CTTHWDecoder
*/

#ifndef __TTH264HWDECODER_H__
#define __TTH264HWDECODER_H__

#import <VideoToolbox/VideoToolbox.h>

#include "TTVideo.h"
#include "AVCDecoderTypes.h"
#include "ttHWVideoDec.h"
#include "TTList.h"
#include "GKCritical.h"

typedef struct _TTInnerDataPacket {
    int64_t             pts;
    CVImageBufferRef   imageBuffer;
}TTInnerDataPacket;


class CTTHWDecoder
{
public:

	CTTHWDecoder(TTUint aCodecType);
	virtual ~CTTHWDecoder();

public:
	virtual TTInt							initDecode();

	virtual TTInt							uninitDecode();

	virtual TTInt							getOutputBuffer(TTVideoBuffer* DstBuffer, TTVideoFormat* pOutInfo);

	virtual TTInt							setParam(TTInt aID, void* pValue);

	virtual TTInt							getParam(TTInt aID, void* pValue);

	//virtual	TTObserver*						getObserver();
    
    virtual	TTInt 						    createSession();
    
    virtual	TTInt32                            SetInputData(TTBuffer *InBuffer);
    
    virtual	void                            InsertToUsedList(TTInnerDataPacket * apk);
    
    virtual	void                            ReleaseVideoBuffer();
    
    virtual	TTInnerDataPacket *             GetDataFromUsedList();
    
    virtual TTInt                           flush();

private:
	TTUint								mVideoCodec;
	TTVideoFormat						mVideoFormat;
    TTBool                              mSeeking;
    TTBool                              mEOS;
    TTBool                              mNewStart;
    
    unsigned char *						mSpsBuffer;
    TTInt								mSpsSize;
    unsigned char *						mPpsBuffer;
    TTInt								mPpsSize;
    unsigned char *						mConfigBuffer;
    TTInt								mConfigSize;
    
    VTDecompressionSessionRef              mDecSession;
    CMVideoFormatDescriptionRef            mVideoFormatDescr;
    
    RGKCritical                         mCritical;
    
    List<TTInnerDataPacket *>	        mListUsed;
    List<TTInnerDataPacket *>	        mListFree;
    
    unsigned char *						mTmpBuffer;
    TTInt								mTmpSize;
    
};

#endif //__TTHWDECODER_H__
