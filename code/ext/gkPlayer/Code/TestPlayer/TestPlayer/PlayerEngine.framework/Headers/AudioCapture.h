/**
 * File : AudioCapture.mh
 * Created on : 2015-12-12
 * Author : kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : AudioCapture.h 
 */

#ifndef AudioCapture_h
#define AudioCapture_h

#include <AudioToolbox/AudioToolbox.h>
#include "FileWrite.h"
#include "AudioEncoder.h"

#define  kNumberBuffers   3                             // 1

class CGKAudioCapture
{
public:
    
    CGKAudioCapture();
    
    virtual ~CGKAudioCapture();
    
public:
    void      setupAudioFormat();
    void      encoder(void* data, int size);
    
    void      start();
    
    void      stop();
    
    void      stoptransfer();
    
    void      setaudioparameter(TTInt SampleRate, TTInt Channels);
    
    void      setPush(void* pushobj);
    
    void     startEncoder();
    
private:
    AudioStreamBasicDescription  mDataFormat;
    AudioQueueRef               mQueue;
    AudioQueueBufferRef         mBuffers[kNumberBuffers];
    UInt32                      bufferByteSize;
    
    FileWrite *                fwc;
    CGKAudioEncoder*            mEncoder;
    
    TTInt                       mSampleRate;
    TTInt                       mChannels;
    RGKCritical                 iCritical;
    
 public:
    bool   mIsRunning;
};

#endif /* AudioCapture_h */
