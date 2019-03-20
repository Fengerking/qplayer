/**
 * File : AudioEncoder.h
 * Created on : 2015-12-12
 * Author : kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : AudioEncoder.h
 */

#ifndef AudioEncoder_h
#define AudioEncoder_h

#include <AudioToolbox/AudioToolbox.h>
#include "TTMediadef.h"
#include "GKOsalConfig.h"
#include "GKTypedef.h"
#include "TTSemaphore.h"
#include "GKPushWrap.h"
//#define DUMP_ENCODER_AAC 1

typedef struct _tagConvertContext {
    AudioConverterRef converter;
    int samplerate;
    int channels;
    
}ConvertContext;

typedef struct
{
    void *source;
    UInt32 sourceSize;
    UInt32 channelCount;
    AudioStreamPacketDescription *packetDescriptions;
}FillComplexInputParam;


class CGKAudioEncoder
{
public:

    CGKAudioEncoder();
    
    virtual ~CGKAudioEncoder();
    
    
public:
    
    void      encoder(void* srcdata, int srclen);

    void      close();

    void      stop();
    
    void      init();
    
    void      setaudioparameter(TTInt SampleRate, TTInt Channels);
    
    void      setPush(void* pushobj);
private:
    //static OSStatus
    static OSStatus inInputDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData);
    
    void        adtsPacketHead(int packetLength);
    
private:
    ConvertContext*     mContext;
    UInt32               mValue;
    bool                mCancle;
    TTBYTE              mpacket[7];
    
    TTBYTE              mAACConfig[2];
    bool                mIsConfigSet;
    
    TTInt               mSampleRate;
    TTInt               mChannels;
    
    CGKPushWrap*        mPush;
    
    void*               moutBuffer;
    TTInt               moutBufferSize;
    
   // RGKCritical                         iCritical;
   // RTTSemaphore                        iSemaphore;
    long                mlast;
    long                mcurrentOrigin;
    
    TTPBYTE              mInputBuffer;
    TTPBYTE              mInputFrame;
    int                  minputpos;
    
};

#endif /* AudioEncoder_h */
