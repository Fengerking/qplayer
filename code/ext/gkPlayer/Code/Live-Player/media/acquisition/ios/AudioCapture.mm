/**
 * File : AudioCapture.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Inc. All rights reserved.
 * Description : CGKAudioCapture
 */
#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>
#import "AudioCapture.h"

//#define  DUMP_PCM 1
#define ONE_FRAME_NUM 4096

CGKAudioCapture::CGKAudioCapture()
: mSampleRate(0)
, mChannels(0)
{
#ifdef DUMP_PCM
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    NSLog(@"doc dir: %@", docDir);
    NSString *filename = @"/tt.pcm";
    NSString *realName = [docDir stringByAppendingString:filename];
    
    fwc = new FileWrite();
    fwc->create([realName UTF8String]);
#endif
    
    iCritical.Create();
    
    mEncoder = new CGKAudioEncoder();
}

CGKAudioCapture::~CGKAudioCapture()
{
    SAFE_DELETE(mEncoder);
    iCritical.Destroy();
}

void CGKAudioCapture::setaudioparameter(TTInt SampleRate, TTInt Channels)
{
    mSampleRate = SampleRate;
    mChannels = Channels;
}

static void HandleInputBuffer(void* aqData, AudioQueueRef inAQ,
                            AudioQueueBufferRef inBuffer,const AudioTimeStamp *inStartTime,
                        UInt32 inNumPackets,const AudioStreamPacketDescription *inPacketDesc)
{
    CGKAudioCapture *ac = (CGKAudioCapture *)aqData;
    
    if (ac->mIsRunning == false){
    }
    else{
        if (inBuffer->mAudioDataByteSize >0) // encoder pcm data
            ac->encoder((char *)inBuffer->mAudioData ,inBuffer->mAudioDataByteSize);
    }
    
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
}

/*void DeriveBufferSize (AudioQueueRef  audioQueue, AudioStreamBasicDescription* StreamDescription,Float64 seconds,  UInt32* outBufferSize)
{
    static const int maxBufferSize = 0x50000;//256k
    int maxPacketSize = StreamDescription->mBytesPerPacket;
    if (maxPacketSize == 0) {
        UInt32 maxVBRPacketSize = sizeof(maxPacketSize);
        AudioQueueGetProperty( audioQueue,kAudioQueueProperty_MaximumOutputPacketSize,
                            &maxPacketSize, &maxVBRPacketSize);
    }
    
    Float64 numBytesForTime = StreamDescription->mSampleRate * maxPacketSize * seconds;
    *outBufferSize=(numBytesForTime<maxBufferSize ? numBytesForTime:maxBufferSize);
}*/

void CGKAudioCapture::setupAudioFormat()
{
    memset(&mDataFormat, 0, sizeof(mDataFormat));
    mDataFormat.mSampleRate = mSampleRate;
    mDataFormat.mChannelsPerFrame = mChannels;
    mDataFormat.mFormatID = kAudioFormatLinearPCM;

    // if we want pcm, default to signed 16-bit little-endian
    mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    mDataFormat.mBitsPerChannel = 16;
    mDataFormat.mBytesPerPacket = mDataFormat.mBytesPerFrame = sizeof(SInt16) * mDataFormat.mChannelsPerFrame;
    mDataFormat.mFramesPerPacket = 1;
}

void CGKAudioCapture::start()
{
    //UInt32 category = kAudioSessionCategory_RecordAudio;
    //AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
    mIsRunning=false;
    
    NSError *sessionError = nil;
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryRecord error:&sessionError];
    
    if(sessionError)
    {
        NSLog(@"%@", sessionError);
    }
    
    // format
    setupAudioFormat();
    
    //DeriveBufferSize (mQueue, &mDataFormat, 0.025, &bufferByteSize);
    
    // 设置回调函数
    AudioQueueNewInput(&mDataFormat, HandleInputBuffer, this, NULL, NULL, 0, &mQueue);
    
    for (int i = 0; i < kNumberBuffers; ++i) {
        AudioQueueAllocateBuffer (mQueue, ONE_FRAME_NUM, &mBuffers[i]);
        AudioQueueEnqueueBuffer (mQueue,mBuffers[i],0,NULL);
    }
    
    mEncoder->setaudioparameter(mSampleRate, mChannels);
    mEncoder->init();
    
    // 开始录音
    AudioQueueStart(mQueue, NULL);
    //mIsRunning= YES;
}

void  CGKAudioCapture::stoptransfer()
{
    iCritical.Lock();
    mIsRunning = false;
    iCritical.UnLock();
}

void CGKAudioCapture::stop()
{
    iCritical.Lock();
    mIsRunning = false;
    iCritical.UnLock();
    
    mEncoder->stop();
    AudioQueueFlush(mQueue);
    AudioQueueStop (mQueue,true);
    mEncoder->close();
    
    for (int i = 0; i < kNumberBuffers; ++i) {
        AudioQueueFreeBuffer (mQueue, mBuffers[i]);
        mBuffers[i] = NULL;
    }
    
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
    SAFE_DELETE(mEncoder);
#ifdef DUMP_PCM
    if (fwc) {
        fwc->close();
        delete fwc;
    }
#endif
}

void CGKAudioCapture::encoder(void* data, int size)
{
#ifdef DUMP_PCM
    fwc->write(data, size);
    fwc->flush();
#endif
    
    iCritical.Lock();
    if (mIsRunning == false) {
        iCritical.UnLock();
        return;
    }
    iCritical.UnLock();
    
    mEncoder->encoder(data, size);
}

void CGKAudioCapture::setPush(void* pushobj)
{
    mEncoder->setPush(pushobj);
}

void CGKAudioCapture::startEncoder()
{
    iCritical.Lock();
    mIsRunning = true;
    iCritical.UnLock();
}


