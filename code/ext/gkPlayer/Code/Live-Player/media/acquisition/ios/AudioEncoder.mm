/**
 * File : AudioEncoder.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : CTTAudioEncoder
 */

#import <Foundation/Foundation.h>
#include "AudioEncoder.h"
#include "FileWrite.h"
#include "GKTypedef.h"
#include "TTSysTime.h"
#include "AVTimeStamp.h"
#include "GKCollectCommon.h"

#define ENCODE_SRC_LEN 4096
#define OutBuffer_limit  8192

FileWrite* fwc = NULL;
CGKAudioEncoder::CGKAudioEncoder()
: mValue(0)
, mCancle(false)
, mIsConfigSet(false)
, mContext(NULL)
, mSampleRate(0)
, mChannels(0)
, mcurrentOrigin(0)
, mlast(0)
, mPush(NULL)
{
    
#ifdef DUMP_ENCODER_AAC
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    NSLog(@"doc dir: %@", docDir);
    NSString *filename = @"/tt.AAC";
    NSString *realName = [docDir stringByAppendingString:filename];
    
    fwc = new FileWrite();
    fwc->create([realName UTF8String]);
#endif
    mInputBuffer = (TTPBYTE)malloc(OutBuffer_limit);
    mInputFrame = (TTPBYTE)malloc(ENCODE_SRC_LEN);
    minputpos = 0;
    moutBuffer = malloc(OutBuffer_limit);
    moutBufferSize = OutBuffer_limit;
}

CGKAudioEncoder::~CGKAudioEncoder()
{
    SAFE_FREE(moutBuffer);
    SAFE_FREE(mContext);
    SAFE_FREE(mInputBuffer);
    SAFE_FREE(mInputFrame);
    
}

void CGKAudioEncoder::setaudioparameter(TTInt SampleRate, TTInt Channels)
{
    mSampleRate = SampleRate;
    mChannels = Channels;
    
    //construct aac config
    TTUint16 smpleindx = 4; //44100
    
    int kAACSampleRate[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000};
    
    for (int i=0; i<12; i++) {
        if (kAACSampleRate[i] == mSampleRate) {
            smpleindx = i;
        }
    }

    TTUint16 AACConfig = 0x02;//objectype: aac lc
    AACConfig = AACConfig << 11;

    AACConfig = AACConfig | (smpleindx << 7);
    
    TTUint16 ch = 2;
    if (mChannels == 1) {
        ch = 1;
    }
    
    AACConfig = AACConfig | (ch<< 3);
    
    TTBYTE* strConfig = (TTBYTE*)(&AACConfig);
    mAACConfig[1] = *strConfig;
    mAACConfig[0] = *(strConfig+1);
}

void CGKAudioEncoder::encoder(void* srcdata, int srclen)
{
    if (mCancle) {
        return;
    }
    
    if (minputpos + srclen <= OutBuffer_limit) {
        memcpy(mInputBuffer + minputpos , srcdata, srclen);
        minputpos += srclen;
    }
    
    if (minputpos < ENCODE_SRC_LEN) {
        return;
    }

    if (mContext && mContext->converter) {
        UInt32 packetSize = 1;
        if (srclen > moutBufferSize) {
            SAFE_FREE(moutBuffer);
            moutBuffer = malloc(srclen);
            moutBufferSize = srclen;
        }
        memset(moutBuffer, 0, moutBufferSize);
        
        memcpy(mInputFrame, mInputBuffer, ENCODE_SRC_LEN);
        minputpos -= ENCODE_SRC_LEN;
        if (minputpos > 0 ) {
             memmove(mInputBuffer, mInputBuffer + ENCODE_SRC_LEN, minputpos);
        }
       
        FillComplexInputParam userParam;
        userParam.source = mInputFrame;
        userParam.sourceSize = ENCODE_SRC_LEN;
        userParam.channelCount = mContext->channels;
        userParam.packetDescriptions = NULL;
        
        OSStatus ret = noErr;
        AudioBufferList outputBuffers = {0};
        
        outputBuffers.mNumberBuffers = 1;
        outputBuffers.mBuffers[0].mNumberChannels = mContext->channels;
        outputBuffers.mBuffers[0].mData = moutBuffer;
        outputBuffers.mBuffers[0].mDataByteSize = moutBufferSize;

        ret = AudioConverterFillComplexBuffer(mContext->converter, &inInputDataProc, &userParam, &packetSize, &outputBuffers, NULL);//outputPacketDescriptions);

        if (ret == noErr) {
            if (outputBuffers.mBuffers[0].mDataByteSize > 0) {
 #ifdef DUMP_ENCODER_AAC
                adtsPacketHead(outputBuffers.mBuffers[0].mDataByteSize);
                fwc->write(mpacket, 7);
                fwc->write(outputBuffers.mBuffers[0].mData, outputBuffers.mBuffers[0].mDataByteSize);
                fwc->flush();
#endif
                
                long curTime = CAVTimeStamp::getCurrentTime();
                mcurrentOrigin = curTime;
                
                if (mcurrentOrigin >0 && mlast >0 && (mcurrentOrigin- mlast) > 10) {
                    //printf("--A.pts--\n");
                }
                else{
                    if (curTime - mlast < 5) {
                        curTime = mlast + 22;
                    }else if(curTime - mlast <= 10)
                    {
                        curTime = mlast + 16;
                    }
                }
                
                mlast = curTime;
                
                
                //printf("++A.pts = %ld\n",curTime);
                //printf("aac encode: %d\n",(unsigned int)outputBuffers.mBuffers[0].mDataByteSize);
                
                if (!mIsConfigSet) {
                    mIsConfigSet = true;
                    mPush->audioconfig(mAACConfig, 2);
                }
                //printf("++A.pts = %lld  [%lld]\n",curTime,mcurrentOrigin);
                mPush->sendaudiopacket((TTPBYTE)outputBuffers.mBuffers[0].mData, outputBuffers.mBuffers[0].mDataByteSize, curTime);
                
            }
        }
    }
}

void CGKAudioEncoder::close(){
    AudioConverterDispose(mContext->converter);
    mContext->converter = NULL;
}

void CGKAudioEncoder::init()
{
    AudioStreamBasicDescription sourceDes;
    memset(&sourceDes, 0, sizeof(sourceDes));
    sourceDes.mSampleRate = mSampleRate;
    sourceDes.mFormatID = kAudioFormatLinearPCM;
    sourceDes.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsSignedInteger;
    
    sourceDes.mChannelsPerFrame = mChannels;
    sourceDes.mBitsPerChannel = 16;
    sourceDes.mBytesPerFrame = sizeof(SInt16) * sourceDes.mChannelsPerFrame;
    sourceDes.mBytesPerPacket = sourceDes.mBytesPerFrame;
    sourceDes.mFramesPerPacket = 1;
    sourceDes.mReserved = 0;
    
    
    AudioStreamBasicDescription targetDes;
    memset(&targetDes, 0, sizeof(targetDes));
    targetDes.mFormatID = kAudioFormatMPEG4AAC;
    targetDes.mSampleRate = mSampleRate;
    targetDes.mChannelsPerFrame = mChannels;
    UInt32 size = sizeof(targetDes);
    AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, &targetDes);
    
    AudioClassDescription audioClassDes;
    memset(&audioClassDes, 0, sizeof(AudioClassDescription));
    AudioFormatGetPropertyInfo(kAudioFormatProperty_Encoders, sizeof(targetDes.mFormatID), &targetDes.mFormatID, &size);
    int encoderCount = size / sizeof(AudioClassDescription);
    AudioClassDescription descriptions[encoderCount];
    AudioFormatGetProperty(kAudioFormatProperty_Encoders, sizeof(targetDes.mFormatID), &targetDes.mFormatID, &size, descriptions);
    
    for (int pos = 0; pos < encoderCount; pos ++) {
        if (targetDes.mFormatID == descriptions[pos].mSubType && descriptions[pos].mManufacturer == kAppleSoftwareAudioCodecManufacturer) {
            memcpy(&audioClassDes, &descriptions[pos], sizeof(AudioClassDescription));
            break;
        }
    }
    
    SAFE_FREE(mContext);
    mContext = (ConvertContext*)malloc(sizeof(ConvertContext));
    mContext->channels = mChannels;
    mContext->samplerate = mSampleRate;
    
    //soft
    OSStatus ret = AudioConverterNewSpecific(&sourceDes, &targetDes, 1, &audioClassDes, &mContext->converter);
    
    //hard
    //OSStatus ret =AudioConverterNew(&sourceDes, &targetDes, &mContext->converter);
    
    if (ret == noErr) {
        
        AudioConverterRef converter = mContext->converter;
        UInt32 tmp = kAudioConverterQuality_Medium;
        AudioConverterSetProperty(converter, kAudioConverterCodecQuality, sizeof(tmp), &tmp);
        
        UInt32 bitRate = 64000;
        UInt32 size = sizeof(bitRate);
        ret = AudioConverterSetProperty(converter, kAudioConverterEncodeBitRate, size, &bitRate);
    }
    else {
        SAFE_FREE(mContext);
    }
    
    //查询一下最大编码输出
    mValue = 0;
    size = sizeof(mValue);
    AudioConverterGetProperty(mContext->converter, kAudioConverterPropertyMaximumOutputPacketSize, &size, &mValue);
}

void CGKAudioEncoder::adtsPacketHead(int packetLength)
{
    int adtsLength = 7;
    int profile = 2;  //AAC LC
    //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
    int freqIdx = 4;  //8:16KHz  4:44100
    int chanCfg = 2;  //MPEG-4 Audio Channel Configuration. 1 Channel front-center
    NSUInteger fullLength = adtsLength + packetLength;
    // fill in ADTS data
    mpacket[0] = (char)0xFF;	//syncword
    mpacket[1] = (char)0xF9;	// 1111 1 00 1  = syncword MPEG-2 Layer CRC
    mpacket[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
    mpacket[3] = (char)(((chanCfg&3)<<6) + (fullLength>>11));
    mpacket[4] = (char)((fullLength&0x7FF) >> 3);
    mpacket[5] = (char)(((fullLength&7)<<5) + 0x1F);
    mpacket[6] = (char)0xFC;
}

OSStatus CGKAudioEncoder::inInputDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
    
    FillComplexInputParam* param = (FillComplexInputParam*)inUserData;
    if (param->sourceSize <= 0) {
        *ioNumberDataPackets = 0;
        return -1;
    }
    
    ioData->mBuffers[0].mData = param->source;
    ioData->mBuffers[0].mNumberChannels = param->channelCount;
    ioData->mBuffers[0].mDataByteSize = param->sourceSize;
    *ioNumberDataPackets = 1;
    param->sourceSize = 0;
    param->source = NULL;
    
    return noErr;
}

void CGKAudioEncoder::stop()
{
    mCancle = true;
}

void CGKAudioEncoder::setPush(void* pushobj)
{
    mPush = (CGKPushWrap*)pushobj;
}
