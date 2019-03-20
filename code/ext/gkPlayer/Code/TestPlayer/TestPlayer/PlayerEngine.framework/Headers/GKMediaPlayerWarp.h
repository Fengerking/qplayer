/**
* File : GKMediaPlayerWarp.h
* Created on : 2011-9-1
* Description : CGKMediaPlayerWarp 定义文件
*/
#ifndef __GK_MEDIA_PLAYER_WARP__
#define __GK_MEDIA_PLAYER_WARP__
#include "GKMediaPlayerItf.h"

class CGKMediaPlayerWarp : public IGKMediaPlayerObserver
{
public:
    CGKMediaPlayerWarp(void* aMediaPlayerProxy);
    ~CGKMediaPlayerWarp();
//    void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aError);
    void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3);
    TTInt Play();
    TTInt Stop();
    void Pause();
    void Resume();
    void SetPosition(TTUint aTime, TTInt aOption = 0);
    void SetPlayRange(TTUint aStartTime, TTUint aEndTime);
    TTUint GetPosition();
    
    TTUint Duration();
    TTInt GetCurFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum);
    GKPlayStatus GetPlayStatus();
    
    TTInt SetDataSourceAsync(const TTChar* aUrl, int aFlag);
    TTInt SetDataSourceSync(const TTChar* aUrl);
    
    void SetPowerDown();
    void SetBalanceChannel(float aVolume);
    
    TTInt BufferedPercent();
    TTUint fileSize();
    TTUint bufferedFileSize();
    TTUint BufferedSize();
    
    void SetActiveNetWorkType(GKActiveNetWorkType aType);
    void SetCacheFilePath(const TTChar *aCacheFilePath);
    void AddIrFilePath(const TTChar *aIrFilePath);
    
    void SetProxyServerConfig(TTUint32 aIP, TTInt aPort, const TTChar* aAuthen, TTBool aUseProxy);
    void SetProxyServerConfigByDomain(const TTChar* aDomain, TTInt aPort, const TTChar* aAuthen, TTBool aUseProxy);
    
    void SetView(void* aView);
    
    TTInt BandWidth();
    
    void SetRotate();
    
    void SetVolume(float aVolume);
    
    TTBool IsLiveMode();
    
    void SetHostAddr(const TTChar *aHostAddr);
    
    TTInt GetAVPlayType();
    
    void  SetRendType(TTInt aRenderType);
    
    void  SetMotionEnable(bool aEnable);
    
    
public:
    IGKMediaPlayer* iMediaPlayer;   
    void*           iMediaPlayerProxy;
};

#endif