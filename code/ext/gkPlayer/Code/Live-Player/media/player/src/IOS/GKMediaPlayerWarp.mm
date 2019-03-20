/**
* File : GKMediaPlayerWarp.cpp
* Description : CTTMediaPlayerWarp
*/

#include "GKMediaPlayerProxy.h"
#include "TTLog.h"

extern void ConfigProxyServer(TTUint32 aIP, TTInt Port, const TTChar* aAuthen, TTBool aUseProxy);

extern void ConfigProxyServerByDomain(const TTChar* aDomain, TTInt Port, const TTChar* aAuthen, TTBool aUseProxy);

extern void SetHostMetaData(const TTChar* aHostHead);

CGKMediaPlayerWarp::CGKMediaPlayerWarp(void* aMediaPlayerProxy)
{
    GKASSERT(aMediaPlayerProxy != NULL);
    iMediaPlayerProxy = aMediaPlayerProxy;
    iMediaPlayer = CGKMediaPlayerFactory::NewL(this);
}

CGKMediaPlayerWarp::~CGKMediaPlayerWarp()
{
    SAFE_RELEASE(iMediaPlayer);
}

void CGKMediaPlayerWarp::PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3)
{
    LOGE("PlayerNotifyEvent aMsg: %d",  aMsg);
    [((GKMediaPlayerProxy*)iMediaPlayerProxy) ProcessNotifyEventWithMsg : aMsg andError : aArg2  andError0 : aArg1];
}

TTInt CGKMediaPlayerWarp::SetDataSourceAsync(const TTChar* aUrl, int aFlag)
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->SetDataSourceAsync(aUrl, aFlag);
}

TTInt CGKMediaPlayerWarp::SetDataSourceSync(const TTChar* aUrl)
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->SetDataSourceSync(aUrl);
}

TTInt CGKMediaPlayerWarp::Play()
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->Play();
}

TTInt CGKMediaPlayerWarp::Stop()
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->Stop();
}

void CGKMediaPlayerWarp::Pause()
{
    GKASSERT(iMediaPlayer != NULL);
    iMediaPlayer->Pause();
}

void CGKMediaPlayerWarp::Resume()
{
    GKASSERT(iMediaPlayer != NULL);
    iMediaPlayer->Resume();
}

void CGKMediaPlayerWarp::SetPosition(TTUint aTime, TTInt aOption)
{
    GKASSERT(iMediaPlayer != NULL);
    iMediaPlayer->SetPosition(aTime, aOption);
}

void CGKMediaPlayerWarp::SetPlayRange(TTUint aStartTime, TTUint aEndTime)
{
    GKASSERT(iMediaPlayer != NULL);
    iMediaPlayer->SetPlayRange(aStartTime, aEndTime);
}

TTUint CGKMediaPlayerWarp::GetPosition()
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->GetPosition();
}

TTUint CGKMediaPlayerWarp::Duration()
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->Duration();
}

TTInt CGKMediaPlayerWarp::GetCurFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum)
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->GetCurrentFreqAndWave(aFreq, aWave, aSampleNum);
}

GKPlayStatus CGKMediaPlayerWarp::GetPlayStatus()
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->GetPlayStatus();
}

void CGKMediaPlayerWarp::SetPowerDown()
{
}

TTInt CGKMediaPlayerWarp::BufferedPercent()
{
    TTInt nErr = TTKErrNotReady;
    TTInt nBufferPercent = 0;
    if (iMediaPlayer != NULL)
    {
        if (TTKErrNone == iMediaPlayer->BufferedPercent(nBufferPercent)) 
        {
            return nBufferPercent;
        } 
    }
    
    return nErr;
}

TTUint CGKMediaPlayerWarp::BufferedSize()
{
    TTUint Size = 0;
    if (iMediaPlayer != NULL) {
        Size = iMediaPlayer->BufferedSize();
    }
    
    return Size;
}

TTUint CGKMediaPlayerWarp::fileSize()
{
    TTUint fileSize = 0;
    if (iMediaPlayer != NULL) {
        fileSize = iMediaPlayer->Size();
    }
    
    return fileSize;
}

TTUint CGKMediaPlayerWarp::bufferedFileSize()
{
    TTUint bufferedFileSize = 0;
    if (iMediaPlayer != NULL) {
        bufferedFileSize = iMediaPlayer->BufferedSize();
    }
    
    return bufferedFileSize;
}

void CGKMediaPlayerWarp::SetActiveNetWorkType(GKActiveNetWorkType aType)
{
    GKASSERT(iMediaPlayer != NULL);
    return iMediaPlayer->SetActiveNetWorkType(aType);
}

void CGKMediaPlayerWarp::SetCacheFilePath(const TTChar *aCacheFilePath)
{
    return iMediaPlayer->SetCacheFilePath(aCacheFilePath);
}

void CGKMediaPlayerWarp::SetBalanceChannel(float aVolume)
{
    if (iMediaPlayer != NULL)
		iMediaPlayer->SetBalanceChannel(aVolume);
}

void CGKMediaPlayerWarp::SetProxyServerConfig(TTUint32 aIP, TTInt aPort, const TTChar* aAuthen, TTBool aUseProxy)
{
    ConfigProxyServer(aIP, aPort, aAuthen, aUseProxy);
}

void CGKMediaPlayerWarp::SetView(void* aView)
{
    if (iMediaPlayer != NULL)
        iMediaPlayer->SetView(aView);
}

TTInt CGKMediaPlayerWarp::BandWidth()
{
    if (iMediaPlayer != NULL) {
        return iMediaPlayer->BandWidth();
    }
    return 0;
}

void CGKMediaPlayerWarp::SetRotate()
{
    if (iMediaPlayer != NULL) {
        return iMediaPlayer->SetRotate();
    }
}

void CGKMediaPlayerWarp::SetProxyServerConfigByDomain(const TTChar* aDomain, TTInt aPort, const TTChar* aAuthen, TTBool aUseProxy)
{
    ConfigProxyServerByDomain(aDomain, aPort, aAuthen, aUseProxy);
}

void CGKMediaPlayerWarp::SetVolume(float aVolume)
{
    if (iMediaPlayer != NULL)
        iMediaPlayer->SetVolume(aVolume);
}

TTBool CGKMediaPlayerWarp::IsLiveMode()
{
    TTBool ret = ETTFalse;
    if (iMediaPlayer != NULL)
        ret = iMediaPlayer->IsLiveMode();
    return ret;
}

void CGKMediaPlayerWarp::SetHostAddr(const TTChar *aHostAddr)
{
    SetHostMetaData(aHostAddr);
}

TTInt CGKMediaPlayerWarp::GetAVPlayType()
{
    TTInt ret = 0;
    if (iMediaPlayer != NULL)
        ret = iMediaPlayer->GetAVPlayType();
    return ret;
}

void CGKMediaPlayerWarp::SetRendType(TTInt aRenderType)
{
    if (iMediaPlayer != NULL)
        iMediaPlayer->SetRendType(aRenderType);
}


void CGKMediaPlayerWarp::SetMotionEnable(bool aEnable)
{
    if (iMediaPlayer != NULL)
        iMediaPlayer->SetMotionEnable(aEnable);
}

void CGKMediaPlayerWarp::SetTouchEnable(bool aEnable)
{
    if (iMediaPlayer != NULL)
        iMediaPlayer->setTouchEnable(aEnable);
}
