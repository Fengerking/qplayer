/**
 * File : TTBackgroundConfig.h
 * Description : CTTBackgroundConfig declare˛
 */ 
#ifndef __TT_BACKGROUND_CONFIG_H__
#define __TT_BACKGROUND_CONFIG_H__
#include "GKMacrodef.h"
#include "GKCritical.h"
#include <AudioToolbox/AudioQueue.h>
static TTInt const KAudioQueueBufferNum = 3;
static TTInt const KAudioQueueBufferSize = 40 * KILO;

class TTBackgroundAssetReaderConfig
{
public:
    TTBackgroundAssetReaderConfig();
    TTInt EnableBackground(const TTChar* aPodUrl, TTBool aEnable);
    TTBool IsEnable();

private:
    TTBool        iBackgroundEnable;
    void*         iAsset;
    void*         iAssetReader;
    void*         iAssetReaderOutput;
}; 

class TTBackgroundAudioQueueConfig
{
public:
    static TTInt EnableBackground(TTBool aEnable);
    
private:
    static TTInt StartAudioQueue();
    static void AudioQueueCallback(void *aUserData, AudioQueueRef aAudioQueueRef, AudioQueueBufferRef aAudioQueueBufferRef);
private:
    static TTBool                   iBackgroundEnable;
    static AudioQueueRef            iAudioQueue;
    static AudioQueueBufferRef      iAudioBuffer[KAudioQueueBufferNum];
};
#endif
