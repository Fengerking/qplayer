/**
 * File : TTAudioSink.cpp
 * Description : CTTAudioSink
 */

// INCLUDES
#include "TTAudioSink.h"
#import <Foundation/NSThread.h>
#import <AVFoundation/AVAudioSession.h>
#import <UIKit/UIApplication.h>
#ifdef __MEDIA_PLAYER__
#include "TTAudioEffectManager.h"
#endif
#include "TTBackgroundConfig.h"
#include "TTLog.h"
#include "TTSysTime.h"

#define MAX_COUNT       6
#define BUFFER_LIMIT    4

extern TTInt gAudioEffectLowDelay;
extern TTInt gIos8Above;
extern float gAngleX;
extern float gAngleY;
extern float gAngleZ;
#define KAudioProcessAlignFramesBits  2
#define KWaitIntervalUs               20  //20 ms

#define MAXSLICEPERFRAME  4096
#define DATA_28k (1024 * 28)
#define DATA_20k (1024 * 20)
#define DATA_12k (1024 * 12)
#define STATUS_INIT 0
#define STATUS_PAUSE 1
#define STATUS_RESUME 2
#define MAX_ENLARGE_VOLUME 10

CTTAudioSink::CTTAudioSink(CTTSrcDemux* SrcMux, TTInt nCount)
: TTCBaseAudioSink(SrcMux,nCount)
, iStreamStarted(ETTFalse)
, iCurEffectProcessPos(0)
, iCurEffectProcessAlignBits(0)
, iBalanceVolume(0)
, iIsRestoreVolume(ETTFalse)
, iHeadDataBlockSize(0)
, iPauseResumeFlg(STATUS_INIT)
, iEnlargedVolume(0)
, mListBuffer(NULL)
, mListFull(0)
, mListUsing(0)
, mQueuBufferFull(ETTFalse)
, mIOSSeeking(false)
, mIosEOS(false)
, mCurBuffer(NULL)
, mLastBuffer(NULL)
, mLastAudioLen(0)
, mEffectEnable(ETTFalse)
, mResetAudioSystemTime(ETTFalse)
, mAudioCnt(MAX_COUNT)
{

    iCritical.Create();
    iSemaphore.Create();
    
    mListBuffer = new TTBuffer*[mAudioCnt];
    mListUnitSize = new int[mAudioCnt];
    
    for(TTUint i = 0; i < mAudioCnt; i++) {
        mListBuffer[i] = new TTBuffer;
        mListBuffer[i]->nSize = 0;
        mListBuffer[i]->pBuffer = (TTPBYTE)malloc(KSinkBufferSize);
        mListBuffer[i]->lReserve = 0;//lReserve used for effect handle tag
        mListBuffer[i]->nDuration = 0;//nDuration used for buffer render position
        mListUnitSize[i] = KSinkBufferSize;
    }
    
    mLastBuffer = (TTPBYTE)malloc(KSinkBufferSize);
    mLastAudioLen = 0;
    
    mIOSStop = ETTFalse;
    
    /*int AudioStepTime;
    if(nCount <= 2) {
        AudioStepTime = 50;
    } else if(nCount <= 5) {
        AudioStepTime = 100;
    }else {
        AudioStepTime = 200;
    }*/
    
    //mAudioOffSetTime = mAudioCnt * AudioStepTime;
    //LOGE("---Offsesttime = %d----",mAudioOffSetTime);

    //[NSThread setThreadPriority:0.6];
}

CTTAudioSink::~CTTAudioSink()
{
    //TTBackgroundAudioQueueConfig::EnableBackground(ETTFalse);
    iSemaphore.Destroy();
    iCritical.Destroy();
    
    if(mListBuffer != NULL)	{
        for(TTUint i = 0; i < mAudioCnt; i++)
        {
            SAFE_FREE(mListBuffer[i]->pBuffer);
            SAFE_DELETE(mListBuffer[i]);
        }
    }
    SAFE_DELETE_ARRAY(mListBuffer);
    SAFE_DELETE_ARRAY(mListUnitSize);
    
    SAFE_FREE(mLastBuffer);
}

TTInt CTTAudioSink::render()
{
    //TTInt64 nStart = GetTimeOfDay();
    //full
    iCritical.Lock();
    
    if(mListFull - mListUsing >= BUFFER_LIMIT) {
        iCritical.UnLock();
        if(getPlayStatus() != EStatusPaused) {
            AudioUnitStart();
            startOne(100);
        }
        return 0;
    }

    iCritical.UnLock();
    
    TTInt nErr = TTKErrNone;
    if(mAudioProcess == NULL)
        return TTKErrNotFound;
    
    mCritTime.Lock();
    TTBool bSeeking = mSeeking;
    mCritTime.UnLock();
    
    if(mProcessCount > 1) {
        mSinkBuffer.pBuffer = NULL;
        mSinkBuffer.nSize = 0;
    }else {
        mSinkBuffer.pBuffer = mSinkBuf;
        mSinkBuffer.nSize = mSinkBufLen;
    }
    
    mSinkBuffer.llTime = mCurPos;
    mSinkBuffer.nFlag = 0;
    //TTInt64 nEnd = GetTimeOfDay();
    //TTInt64 nBeforeGet = nEnd - nEnd;
    
    //nStart = GetTimeOfDay();
    nErr = mAudioProcess->getOutputBuffer(&mSinkBuffer);
    //nEnd = GetTimeOfDay();
    //TTInt64 nGetDataTime = nEnd - nEnd;
    //TTInt64 nAfterRender = 0;
    if (nErr == TTKErrNone){
        if(mSinkBuffer.nFlag & TT_FLAG_BUFFER_EOS) {
            setEOS();
            mIosEOS = true;
            nErr = TTKErrEof;
        } else if(mSinkBuffer.nFlag & TT_FLAG_BUFFER_NEW_FORMAT){
            audioFormatChanged();
            startOne(-1);
        } else {
            if(mSinkBuffer.nFlag & TT_FLAG_BUFFER_TIMESTAMP_RESET)	{
                if(mObserver) {
                    mObserver->pObserver(mObserver->pUserData, ENotifyTimeReset, (TTInt)mSinkBuffer.llTime, 0, NULL);
                }
            }
            
            mCritTime.Lock();
            if(bSeeking && mSeekOption && mSinkBuffer.llTime + 200 < mCurPos) {
                mCritTime.UnLock();
                return TTKErrTimedOut;
            }
            mCritTime.UnLock();
            //store data here
            WriteData(&mSinkBuffer);
           //store data here
            //nStart = GetTimeOfDay();
            mCritTime.Lock();
            if(mSeeking && !bSeeking) {
                mCritTime.UnLock();
                return TTKErrNone;
            }
            
            if(mRenderNum == 0)	{
                mResetAudioSystemTime = ETTTrue;
                if(mSeeking) {
                    if(mObserver) {
                        mObserver->pObserver(mObserver->pUserData, ENotifySeekComplete, TTKErrNone, 0, NULL);
                    }
                    mSeeking = false;
                }
                mFrameDuration = mSinkBuffer.nSize*1000/(mAudioFormat.SampleRate*mAudioFormat.Channels*sizeof(short));
            }
            mRenderPCM += mSinkBuffer.nSize;
            mRenderNum++;
            
            mCritTime.UnLock();
            
            iCritical.Lock();
            if(mListFull - mListUsing <= BUFFER_LIMIT)
            {
                startOne(0);
            }
            iCritical.UnLock();
             
        }
    } else if(nErr == TTKErrEof) {
        setEOS();
        mIosEOS = true;
    }
    else if(nErr == TTKErrOverflow) {
        SAFE_FREE(mSinkBuf);
        mSinkBufLen = mSinkBuffer.nSize*3/2;
        mSinkBuf = (TTPBYTE)malloc(mSinkBufLen);
        
        startOne(-1);
    } else if(nErr == TTKErrUnderflow) {
        startOne(2);
    } else if(nErr == TTKErrFormatChanged) {
        audioFormatChanged();
        startOne(0);
    } else{
        startOne(2);
    }
    
    //LOGI("CTTAndroidAudioSink::render nErr %d, nBeforeGet %lld, nGetDataTime %lld,  AfterRender %lld", nErr, nBeforeGet, nGetDataTime, nAfterRender);
    
    return nErr;
    
	//return TTKErrNone;
}


void CTTAudioSink::WriteData(TTBuffer* aBuffer)
{
    /*if(mListFull - mListUsing >= (TTInt)mAudioCount - 1 || mEOS) {
        return 0;//full
    }*/
    int nIndex = mListFull%mAudioCnt;
    TTBuffer* dstBuffer = mListBuffer[nIndex];
    
    if (aBuffer->nSize > mListUnitSize[nIndex]) {
        SAFE_FREE(dstBuffer->pBuffer);
        dstBuffer->pBuffer = (TTPBYTE)malloc(aBuffer->nSize);
        mListUnitSize[nIndex] = aBuffer->nSize;
    }
    
    memcpy(dstBuffer->pBuffer, aBuffer->pBuffer, aBuffer->nSize);
    dstBuffer->nSize = aBuffer->nSize;
    
    dstBuffer->llTime = aBuffer->llTime;
    dstBuffer->nFlag = aBuffer->nFlag;
    dstBuffer->nDuration = 0;
    dstBuffer->lReserve = aBuffer->lReserve;
    
    iCritical.Lock();
    mListFull++;
    iCritical.UnLock();
}

TTInt CTTAudioSink::Open(TTInt SampleRate, TTInt Channels)
{
    NSLogDebug(@"CTTAudioSink::Open! SampleRate:%d,Channels:%d", aAudioDataSetting.iSampleRate,
               aAudioDataSetting.iChannels);
    
    iRealVolume = 1;
    mQueuBufferFull = ETTFalse;
    mResetAudioSystemTime = ETTFalse;
    
    iCurEffectProcessAlignBits = KAudioProcessAlignFramesBits + (Channels == 1 ? 1 : 2);//
    
    AudioStreamBasicDescription tOutputFormat;
    TTInt nSampleSize = sizeof(TTInt16);
    tOutputFormat.mFormatID = kAudioFormatLinearPCM;
    tOutputFormat.mFormatFlags = kAudioFormatFlagsCanonical;
    tOutputFormat.mBitsPerChannel = 8 * nSampleSize;
    tOutputFormat.mChannelsPerFrame = Channels;
    tOutputFormat.mFramesPerPacket = 1;
    tOutputFormat.mBytesPerPacket = tOutputFormat.mBytesPerFrame = Channels * nSampleSize;
    tOutputFormat.mSampleRate = SampleRate;
    
    OSStatus nErr = noErr;
    nErr = NewAUGraph(&iAudioGraph);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    // output unit
    AudioComponentDescription output_desc;
    output_desc.componentType = kAudioUnitType_Output;
	output_desc.componentSubType = kAudioUnitSubType_RemoteIO;
	output_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	output_desc.componentFlags = 0;
	output_desc.componentFlagsMask = 0;
    
    AUNode tOutputNode;
    nErr = AUGraphAddNode(iAudioGraph, &output_desc, &tOutputNode);
	if (nErr != noErr)
        return TTKErrAccessDenied;
    
    AudioComponentDescription mixer_desc;
    mixer_desc.componentType = kAudioUnitType_Mixer;
	mixer_desc.componentSubType = kAudioUnitSubType_MultiChannelMixer;
	mixer_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	mixer_desc.componentFlags = 0;
	mixer_desc.componentFlagsMask = 0;
    
    AUNode mixerNode;
    nErr = AUGraphAddNode(iAudioGraph, &mixer_desc, &mixerNode );
    if (nErr != noErr)
        return TTKErrAccessDenied;
    
    // connect a mixerNode's output to a outputNode's input
	nErr = AUGraphConnectNodeInput(iAudioGraph, mixerNode, 0, tOutputNode, 0);
    if (nErr != noErr)
        return TTKErrAccessDenied;
    
    // open the graph AudioUnits are open but not initialized (no resource allocation occurs here)
	nErr = AUGraphOpen(iAudioGraph);
	if (nErr != noErr)
        return TTKErrAccessDenied;
    
    // grab the audio unit instances from the nodes
	/*nErr = AUGraphNodeInfo(iAudioGraph, tOutputNode, NULL, &iOutputUnit);
    if (nErr != noErr)
        return TTKErrAccessDenied;*/
    
    nErr = AUGraphNodeInfo(iAudioGraph, mixerNode, NULL, &iMixerUnit);
    
    // set bus count
	UInt32 numbuses = 1;
    nErr = AudioUnitSetProperty(iMixerUnit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &numbuses, sizeof(UInt32));
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    AURenderCallbackStruct rcbs;
    rcbs.inputProc = &AudioUnitCallBack;
    rcbs.inputProcRefCon = this;
    
    // set a callback for the specified node's specified input
    nErr = AUGraphSetNodeInputCallback(iAudioGraph, mixerNode, 0, &rcbs);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    // set the input stream format, this is the format of the audio for mixer input
    nErr = AudioUnitSetProperty(iMixerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &tOutputFormat, sizeof(tOutputFormat));
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    nErr = AudioUnitSetProperty(iMixerUnit, kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output, 0, &tOutputFormat, sizeof(tOutputFormat));
    
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    UInt32 maximumFramesPerSlice = MAXSLICEPERFRAME;
    nErr = AudioUnitSetProperty(iMixerUnit,
                                 kAudioUnitProperty_MaximumFramesPerSlice,
                                 kAudioUnitScope_Global,
                                 0,
                                 &maximumFramesPerSlice,
                                 sizeof (maximumFramesPerSlice));
    
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    nErr = AUGraphInitialize(iAudioGraph);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    mIOSStop = ETTFalse;
    iAudioCallBackProcRun = ETTFalse;
    
    AudioUnitParameterValue OnValue = 1.0;
    nErr = AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Enable, kAudioUnitScope_Input, 0, OnValue, 0);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    AudioUnitParameterValue Value = 0;
    nErr = AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, 0, Value, 0);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    //nErr = AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Output, 0, Value, 0);
    
    nErr = AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Pan, kAudioUnitScope_Input, 0, iBalanceVolume, 0);
    if (nErr != TTKErrNone)
        return TTKErrAccessDenied;
    
    iIsRestoreVolume = ETTFalse;
    iHeadDataBlockSize = 0;
    mLastAudioLen = 0;
    
    return TTKErrNone;
}


OSStatus CTTAudioSink::AudioUnitCallBack(void *inRefCon, AudioUnitRenderActionFlags *
                                         ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32
                                         inBusNumber, UInt32 inNumberFrames, AudioBufferList *
                                         ioData)
{
    CTTAudioSink* pAudioSink = (CTTAudioSink*)inRefCon;
    pAudioSink->AudioUnitCallBackProcL(ioData);
    return noErr;
}

TTInt  CTTAudioSink::writeDataToAU(TTUint8* pDstDataPtr, TTInt nRenderSize)
{
    int nIndex;
    int backSize = nRenderSize;
    int curPos = 0;
    while (nRenderSize > 0) {
        nIndex = mListUsing%mAudioCnt;
        mCurBuffer = mListBuffer[nIndex];
        
        if (mCurBuffer->nSize == 0)
        {
            iCritical.Lock();
            if(mListUsing < mListFull) {
                mListUsing++;
                iCritical.UnLock();
                nIndex = mListUsing%mAudioCnt;
                mCurBuffer = mListBuffer[nIndex];
            } else {
                iCritical.UnLock();
                memset(pDstDataPtr + curPos, 0 , backSize - curPos);
                LOGE("UnderFlow 1!  u =%d",mListUsing);
                return TTKErrUnderflow;
            }
        }
        
        TTInt nSrcOffset = mCurBuffer->nDuration;
        if (nSrcOffset == 0) {
            //LOGE("---u =%d",mListUsing);
            mCritTime.Lock();
            if(!mSeeking) {
                mCurPos = mCurBuffer->llTime + mFrameDuration;
                mAudioBufStartTime = mCurBuffer->llTime;
                mAudioSysStartTime = GetTimeOfDay();
                if(mResetAudioSystemTime) {
                    mAudioSystemTime = GetTimeOfDay();
                    mAudioBufferTime = mAudioBufStartTime;
                    mResetAudioSystemTime = ETTFalse;
                }
            }
            mCritTime.UnLock();
        }
        
        TTUint8* pSrcDataPtr = mCurBuffer->pBuffer + nSrcOffset;
        TTInt nValidSize = mCurBuffer->nSize - mCurBuffer->nDuration;
        
        if (nValidSize >= nRenderSize)
        {
#ifdef __MEDIA_PLAYER__
            if (mEffectEnable && mCurBuffer->lReserve == 0) {
                CTTAudioEffectManager* pInstance = CTTAudioEffectManager::QueryInstance();
                if (NULL != pInstance)
                    pInstance->Process(pSrcDataPtr, nRenderSize);
            }
#endif
            memcpy(pDstDataPtr + curPos, pSrcDataPtr, nRenderSize);
            mCurBuffer->nDuration += nRenderSize;
            curPos += nRenderSize;
            
            iHeadDataBlockSize += nRenderSize;
            
            if (nValidSize == nRenderSize) {
                iCritical.Lock();
                mListUsing++;
                iCritical.UnLock();
                mCurBuffer->nDuration =0;
                mCurBuffer->nSize = 0;
                startOne(0);
            }
            
            nRenderSize = 0;
            break;
        }
        else
        {
#ifdef __MEDIA_PLAYER__
            if (mEffectEnable && mCurBuffer->lReserve == 0) {
                CTTAudioEffectManager* pInstance = CTTAudioEffectManager::QueryInstance();
                if (NULL != pInstance)
                    pInstance->Process(pSrcDataPtr, nValidSize);
            }
#endif
            
            memcpy(pDstDataPtr + curPos, pSrcDataPtr, nValidSize);
            mCurBuffer->nSize = 0;
            mCurBuffer->nDuration = 0;
            curPos += nValidSize;
            
            iCritical.Lock();
            if (mListFull > mListUsing) {
                mListUsing++;
                iCritical.UnLock();
                nRenderSize -= nValidSize;
                iHeadDataBlockSize += nValidSize;
            }
            else{
                iCritical.UnLock();
                 startOne(0);
                memset(pDstDataPtr + curPos, 0, backSize - curPos);
                iHeadDataBlockSize += nValidSize;
                LOGE("AudioUnitCallBackProcL UnderFlow 2!");
                break;
            }
        }
    }
    
    return TTKErrNone;
}

void CTTAudioSink::AudioUnitCallBackProcL(AudioBufferList *
                                          ioData)
{
    TTUint8* pDstDataPtr = (TTUint8*)ioData->mBuffers[0].mData;
    TTInt nRenderSize = ioData->mBuffers[0].mDataByteSize;
    iAudioCallBackProcRun = ETTTrue;
    /*if (gSlientImmediately)
    {
        //NSLog(@"iIsPaused:%d,gSlientImmediately:%d!", iIsPaused, gSlientImmediately);
        memset(pDstDataPtr, 0, nRenderSize);
        return;
    }*/
    
    iCritical.Lock();
    if(mListUsing > mListFull) {
        iCritical.UnLock();
        if (!mQueuBufferFull) {
            startOne(0);
            mQueuBufferFull = ETTTrue;
        }
        LOGE("AudioUnitCallBackProcL UnderFlow 0!");
        memset(pDstDataPtr, 0, nRenderSize);
        return;
    }
    iCritical.UnLock();
    
    if(TTKErrNone != writeDataToAU(pDstDataPtr, nRenderSize)){
        if (!mQueuBufferFull) {
            startOne(0);
            mQueuBufferFull = ETTTrue;
        }
    }
    
    if (iPauseResumeFlg != STATUS_INIT)
        HandleNoise();
    
    mQueuBufferFull = ETTFalse;

    if (!iIsRestoreVolume){
        if (iHeadDataBlockSize >= DATA_28k) {
            iIsRestoreVolume = ETTTrue;
            SetRealVolume(iRealVolume);
        }
        else if (iHeadDataBlockSize >= DATA_20k) {
            SetRealVolume(iRealVolume*2/3);
        }
        else if (iHeadDataBlockSize >= DATA_12k) {
            SetRealVolume(iRealVolume/3);
        }
    }
    
    if (mIosEOS) {
        if (mListFull == mListUsing) {
            AudioUnitStop();
            iAudioCallBackProcRun = ETTFalse;
        }
    }
}

TTInt CTTAudioSink::close()
{
    TTCBaseAudioSink::close();
    DisposeAUGraph(iAudioGraph);
    iAudioGraph = NULL;
    return 0;
}

TTInt CTTAudioSink::stop()
{
    TTCBaseAudioSink::stop();
    return TTKErrNone;
}

TTInt CTTAudioSink::AudioUnitStart()
{
    iCritical.Lock();
    if (!iStreamStarted)
    {
        iStreamStarted = ETTTrue;
        iCritical.UnLock();
        OSStatus errorCode = AUGraphStart(iAudioGraph);
        if (errorCode != noErr) {
            NSLogDebug(@"AUGraphStart start failed:%ld",errorCode);
            
            iCritical.Lock();
            iStreamStarted = ETTFalse;
            iCritical.UnLock();
            
            return TTKErrStartAudioUnitFailed;
        }
        //TTBackgroundAudioQueueConfig::EnableBackground(ETTFalse);
    }
    else
        iCritical.UnLock();
    
    return TTKErrNone;
}

void CTTAudioSink::AudioUnitStop()
{
    iCritical.Lock();
    if (iStreamStarted)
    {
        //TTBackgroundAudioQueueConfig::EnableBackground(ETTTrue);
        iStreamStarted = ETTFalse;
        iCritical.UnLock();
        Boolean bIsRunning = ETTFalse;
        OSStatus nErr = AUGraphIsRunning(iAudioGraph, &bIsRunning);
        //GKASSERT(bIsRunning);
        
        nErr = AUGraphStop(iAudioGraph);
    }
    else
        iCritical.UnLock();
}


void CTTAudioSink::FlushData()
{
    iCritical.Lock();
    mListUsing = 0;
    mListFull = 0;
    for(TTUint i = 0; i < mAudioCnt; i++) {
        mListBuffer[i]->nSize = 0;
        mListBuffer[i]->nDuration = 0;
    }
    iCritical.UnLock();
}

TTInt CTTAudioSink::pause(TTBool aFadeOut)
{
    LOGE("CTTAudioSink::Pause");
    TTCBaseAudioSink::pause();
    
    iPauseResumeFlg = STATUS_PAUSE;
    iEnlargedVolume = MAX_ENLARGE_VOLUME;
    iSemaphore.Wait(KWaitIntervalUs*2);
  
    AudioUnitStop();
    
    return 0;
}

TTInt CTTAudioSink::resume(TTBool aWait,TTBool aFadeIn)
{
    NSLogDebug(@"CTTAudioSink::Resume");
   TTCBaseAudioSink::resume();
    
    iPauseResumeFlg = STATUS_RESUME;
    iEnlargedVolume = 0;
    
    if (mIOSSeeking) {
        mIOSSeeking = false;
        FlushData();
    }
    return AudioUnitStart();

    //TTBackgroundAudioQueueConfig::EnableBackground(ETTFalse);
}

void CTTAudioSink::SetVolume(TTInt aLVolume, TTInt aRVolume)
{
    
}

void CTTAudioSink::SetVolume(float aVolume)
{
    iRealVolume = aVolume;
    Boolean bOpen;
    OSStatus nErr = AUGraphIsOpen(iAudioGraph, &bOpen);
    if (nErr == TTKErrNone && bOpen == true)
        SetRealVolume(aVolume);
}

void CTTAudioSink::SetBalanceChannel(float aVolume)
{
    //if (aBalanceValue > 1 || aBalanceValue < -1)
    //    return;
    iBalanceVolume = aVolume;
    Boolean bOpen;
    OSStatus nErr = AUGraphIsOpen(iAudioGraph, &bOpen);
    if (nErr == TTKErrNone && bOpen == true)
        AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Pan, kAudioUnitScope_Input, 0, aVolume, 0);
}

void CTTAudioSink::SetRealVolume(float aVolume)
{
    AudioUnitParameterValue Value = aVolume;
    AudioUnitSetParameter(iMixerUnit, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, 0, Value, 0);
}


void CTTAudioSink::HandleNoise()
{
    float aVolume;
    switch (iPauseResumeFlg) {
        case STATUS_PAUSE:
            iEnlargedVolume -= MAX_ENLARGE_VOLUME/2;
            aVolume = (float)iEnlargedVolume / MAX_ENLARGE_VOLUME;
            if (aVolume > iRealVolume) {
                aVolume = iRealVolume;
            }
            SetRealVolume(aVolume);
            if (iEnlargedVolume == 0) {
                iPauseResumeFlg = STATUS_INIT;
            }
            break;
        case STATUS_RESUME:
            aVolume = (float)iEnlargedVolume / MAX_ENLARGE_VOLUME;
            if (aVolume > iRealVolume) {
                aVolume = iRealVolume;
            }
            SetRealVolume(aVolume);
            
            if (iEnlargedVolume < MAX_ENLARGE_VOLUME)
            {
                iEnlargedVolume += MAX_ENLARGE_VOLUME/3;
            }
            if (iEnlargedVolume >= MAX_ENLARGE_VOLUME) {
                iEnlargedVolume = MAX_ENLARGE_VOLUME;
                iPauseResumeFlg = STATUS_INIT;
            }
            break;
            
        default:
            break;
    }
}

TTInt CTTAudioSink::newAudioTrack()
{
    mIosEOS = false;
    FlushData();
    
    
    if (iAudioGraph != NULL) {
        AudioUnitStop();
        DisposeAUGraph(iAudioGraph);
        iAudioGraph = NULL;
        iIsRestoreVolume = ETTFalse;
        iHeadDataBlockSize = 0;
        iEnlargedVolume = 0;
        iPauseResumeFlg = STATUS_INIT;
    }
    
    return Open(mAudioFormat.SampleRate, mAudioFormat.Channels);
}

TTInt CTTAudioSink::freeAudioTrack()
{
    if (mIOSStop && iAudioCallBackProcRun)
    {
        SetRealVolume(0);
        iSemaphore.Wait(KWaitIntervalUs);
        iAudioCallBackProcRun = ETTFalse;
    }
    
    mIOSStop = ETTFalse;
    //CTTBaseDataSink::Stop();
    AudioUnitStop();
    //Cancel();
    iIsRestoreVolume = ETTFalse;
    iHeadDataBlockSize = 0;
    iEnlargedVolume = 0;
    iPauseResumeFlg = STATUS_INIT;
    
    TTCBaseAudioSink::freeAudioTrack();
    mIosEOS = false;
    return 0;
}

TTInt CTTAudioSink::flush()
{
    TTCBaseAudioSink::flush();
    return TTKErrNone;
}

TTInt CTTAudioSink::syncPosition(TTUint64 aPosition, TTInt aOption)
{
    TTCBaseAudioSink::syncPosition(aPosition, aOption);
    mIOSSeeking = true;
    
    return TTKErrNone;
}

 void CTTAudioSink::setIOSVersion()
{
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 8.0)
    {
        gIos8Above = 1;
    }
    else
        gIos8Above = 0;
}

TTInt CTTAudioSink::setParam(TTInt aID, void* pValue)
{
    TTInt ret = TTCBaseAudioSink::setParam(aID, pValue);
    if(aID == TT_PID_AUDIO_DOEFFECT && pValue != NULL) {
        mEffectEnable = *((TTBool *)pValue);
    }
    
    return ret;
    
}


