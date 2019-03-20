/**
* File : TTMediaPlayerProxy.mm
* Created on : 2011-9-22
* Description : TTMediaPlayerProxy
*/
#import "GKMediaPlayerProxy.h"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIApplication.h>

#include "TTLog.h"
#include "GKUIDeviceHardware.h"
#include "TTBackgroundConfig.h"
#include <TargetConditionals.h>

#if TARGET_RT_BIG_ENDIAN
#   define FourCC2Str(fourcc) (const char[]){*((char*)&fourcc), *(((char*)&fourcc)+1), *(((char*)&fourcc)+2), *(((char*)&fourcc)+3),0}
#else
#   define FourCC2Str(fourcc) (const char[]){*(((char*)&fourcc)+3), *(((char*)&fourcc)+2), *(((char*)&fourcc)+1), *(((char*)&fourcc)+0),0}
#endif

extern void setLogOpenSwitch(int aSwitch);
#define PRINT_LOG_OPEN  1
#define PRINT_LOG_CLOSE 0

Boolean gIsIOS4X = ETTFalse;
int gIsIphone6Up = 0;
TTInt  gAudioEffectLowDelay = 1;

static TTBackgroundAssetReaderConfig gGeminiLeft;
static TTBackgroundAssetReaderConfig gGeminiRight;
//static GKPlayStatus playerStatusBeforeCalling = EStatusPaused;

@implementation GKMsgObject

@synthesize iMsg;
@synthesize iError;
@synthesize iError0;

- (id) initwithMsg : (GKNotifyMsg) aMsg andError: (TTInt) aError andError0: (TTInt) aError0
{
    iMsg = aMsg;
    iError = aError;
    iError0 = aError0;
    return self;
}

@end

@interface GKMediaPlayerProxy()

- (void)dealloc;
- (TTInt) setDataSource : (NSString*) aUrl andFlag : (int) aFlag;
- (void) notifyProcessProcL : (id) aMsgObject;
- (void) backgroundIssueProcess : (GKMsgObject*) aMsg;
- (GKMsgObject *) playIssueProcess:(GKMsgObject *)aMsg;
- (void) setupAudioSession;
- (void) onTimer;
- (void) startCountDown;
- (void) stopCountDown;
- (void) geminiProcL : (NSObject*) aObj;
@end

@implementation GKMediaPlayerProxy

@synthesize interruptedWhilePlaying;

- (id) init                                                             
{   
    gIsIOS4X = [[GKUIDeviceHardware instance] IsSystemVersion4X];
    NSLogDebug(@"gIsIOS4X:%d", gIsIOS4X);
    
    if([[GKUIDeviceHardware instance] IsDevice6Up]) {
        gIsIphone6Up = 1;
    }
    
    [self setupAudioSession];
    iPlayer = new CGKMediaPlayerWarp((void*)self);
        
    //iUrlUpdated = false;
    
    iAssetReaderFailOrLongPaused =false;
    
    iCurUrl = NULL;
    
    iPowerDownCountDownTimer = NULL;
        
    interruptedWhilePlaying = false;
        
    iGeminiUrl = NULL;
        
    if (gIsIOS4X)
    {
        iCritical = [[NSLock alloc] init];
        iGeminiDone = ETTFalse;
        iGeminiThreadHandle = [[NSThread alloc] initWithTarget:self selector:@selector(geminiProcL:) object:nil];
    }
    else
    {
        iCritical = NULL;
        iGeminiDone = ETTTrue;
        iGeminiThreadHandle = NULL;
    }
    
    return self;
}

- (void)dealloc
{    
    iGeminiDone = ETTTrue;
    
    if (iCurUrl != NULL)
    {
        [iCurUrl release];
        iCurUrl = NULL;
    }
        
    if (iGeminiThreadHandle != NULL)
    {
        [iGeminiThreadHandle release];
        iGeminiThreadHandle = NULL;
    }
    
    if (iCritical != NULL)
    {
        [iCritical release];
    }
    
    if (iGeminiUrl != NULL)
    {
        [iGeminiUrl release];
    }
    
    [self stopCountDown];
    
        
    SAFE_DELETE(iPlayer);
    
    [super dealloc];
}

- (void) setupAudioSession
{
    NSError *sessionError = nil;
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&sessionError];
    
    if(sessionError)
    {
        NSLogDebug(@"%@", sessionError);
    }
    else
    {   sessionError = nil;
        [[AVAudioSession sharedInstance] setActive:YES withOptions:kAudioSessionSetActiveFlag_NotifyOthersOnDeactivation error:&sessionError];
        if(sessionError)
        {
            NSLogDebug(@"%@", sessionError);
        }
    }
      
    if (gIsIOS4X)
    {
        [self setAudioSessionMixWithOther:1];
    }
    
    [[AVAudioSession sharedInstance] setActive:YES error:&sessionError];
    
    if(sessionError)
    {
        NSLogDebug(@"%@", sessionError);
    }
}

- (BOOL)setAudioSessionMixWithOther:(UInt32)mixWithOthers{
    OSStatus tStatus = AudioSessionSetProperty (kAudioSessionProperty_OverrideCategoryMixWithOthers,
                                                sizeof (mixWithOthers),
                                                &mixWithOthers
                                                );
    NSLogDebug(@"AudioSession Config tStatus:%d",(int)tStatus);
    
    return !tStatus;
}

- (BOOL)isUnderPlaybackCategory{
    UInt32 audioSessionCategory = 0;
    UInt32 playBackAudioSession = kAudioSessionCategory_MediaPlayback;
    UInt32 dataSize = sizeof(audioSessionCategory);
    OSStatus result = AudioSessionGetProperty(kAudioSessionProperty_AudioCategory, &dataSize,&audioSessionCategory);
    BOOL isUnderPlayback = NO;
    
    NSString *currentAudioSessionString = @(FourCC2Str(audioSessionCategory));
    NSString *mediaPlaybackAudioSessionString = @(FourCC2Str(playBackAudioSession));
    if ([currentAudioSessionString isEqual:mediaPlaybackAudioSessionString]) {
        isUnderPlayback = YES;
    }
    printf("resut:%ld is under playback category:%hhd\n",result,isUnderPlayback);
    
    return isUnderPlayback;
}

- (void)setPlayBackAudioSession:(BOOL)active {
    [self isUnderPlaybackCategory];
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
    [[AVAudioSession sharedInstance] setActive:active error:nil];
}

- (BOOL)isVideoMode {
    int mediaMode = [self GetAVPlayType];
    return mediaMode == AVTYPE_VIDEO_ONLY ||
           mediaMode == AVTYPE_AUDIOVIDEO;
}

- (TTInt) start
{
    [self setPlayBackAudioSession:YES];
    playerPausedManually = NO;
    
    NSLogDebug(@"Proxy start");
    GKASSERT(iPlayer != NULL);
    return iPlayer->Play();
}

- (void) pause
{
    playerPausedManually = YES;
    
    NSLog(@"Proxy pause");
    GKASSERT(iPlayer != NULL);
    iPlayer->Pause();
}

- (void) inactiveAudioSession
{
    [self setPlayBackAudioSession:NO];
}

- (void) resume
{
    [self setPlayBackAudioSession:YES];
    playerPausedManually = NO;

    LOGE("-resume-");
    GKASSERT(iPlayer != NULL);
    if (iAssetReaderFailOrLongPaused)
    {
        iAssetReaderFailOrLongPaused = false;
        iPlayer->SetPosition(iPlayer->GetPosition());        
    }
    
    iPlayer->Resume();
}

- (void)resumeAndSetMixOthers{
    if (!gIsIOS4X) {
        [self setAudioSessionMixWithOther:0];
    }
    
    [self resume];
}

- (void) setPosition : (CMTime) aTime
{
    NSLogDebug(@"Proxy setPostion");
    GKASSERT(iPlayer != NULL);
    TTUint64 ntmp = aTime.value;
    ntmp *= 1000;
    TTUint32 nMillSecPos = ntmp / aTime.timescale;
    iPlayer->SetPosition(nMillSecPos);    
}

- (void) setPlayRangeWithStartTime : (CMTime) aStartTime EndTime : (CMTime) aEndTime
{    
    GKASSERT(iPlayer != NULL);
    TTUint64 ntmp = aStartTime.value;
    ntmp *= 1000;
    TTUint32 nStartMillSecPos = ntmp / aStartTime.timescale;
    
    ntmp = aEndTime.value;
    ntmp *= 1000;
    TTUint32 nEndMillSecPos = ntmp / aEndTime.timescale;
    
    NSLogDebug(@"Proxy setPlayRange:%ld--%ld", nStartMillSecPos, nEndMillSecPos);
    if (nEndMillSecPos > nStartMillSecPos)
    {
        iPlayer->SetPlayRange(nStartMillSecPos, nEndMillSecPos);
    }
}

- (CMTime) getPosition
{
    GKASSERT(iPlayer != NULL);
    return CMTimeMake(iPlayer->GetPosition(), 1000);
}

- (CMTime) duration
{
    GKASSERT(iPlayer != NULL);
    return CMTimeMake(iPlayer->Duration(), 1000);
}

- (TTInt) getCurFreqAndWaveWithFreqBuffer : (TTInt16*) aFreqBuffer andWaveBuffer : (TTInt16*) aWaveBuffer andSamplenum :(TTInt) aSampleNum
{
    return iPlayer->GetCurFreqAndWave(aFreqBuffer, aWaveBuffer, aSampleNum);
}

- (GKPlayStatus) getPlayStatus
{
    GKASSERT(iPlayer != NULL);
    return iPlayer->GetPlayStatus();
}

- (void) ProcessNotifyEventWithMsg : (GKNotifyMsg) aMsg andError: (TTInt) aError  andError0: (TTInt) aError0
{
    GKMsgObject* pMsgObject = [[GKMsgObject alloc] initwithMsg : aMsg andError : aError andError0 : aError0];
    [self performSelectorOnMainThread:@selector(notifyProcessProcL:) withObject:pMsgObject waitUntilDone:NO];
}

- (void) notifyProcessProcL : (id) aMsgObject
{
    [self backgroundIssueProcess:(GKMsgObject *)aMsgObject];
    GKMsgObject* pMsg = [self playIssueProcess:(GKMsgObject *)aMsgObject];
    
    if (pMsg != NULL)
    {
//        if(self.playerDelegate != nil) {
//            [self.playerDelegate mediaPlayerEventChanged:pMsg];
//        } else {
//            [[NSNotificationCenter defaultCenter] postNotificationName:@"PlayerNotifyEvent"     object:pMsg];
//        }
        [[NSNotificationCenter defaultCenter] postNotificationName:@"PlayerNotifyEvent"     object:pMsg];
        //[self.playerDelegate mediaPlayerEventChanged:pMsg];
        [pMsg release];
    }
}
 
- (GKMsgObject *) playIssueProcess : (GKMsgObject *)aMsg
{    
    GKMsgObject* pMsgObject = aMsg;
    TTInt nMsgId = pMsgObject.iMsg;
    TTInt errorCode = pMsgObject.iError;
    if (nMsgId == ENotifyStop)
    {
        /*if (iUrlUpdated)//change mediaItem
        {
            if (TTKErrNone != [self setDataSource : iCurUrl])
            {
                iPlayer->Stop();
            }
            
            [pMsgObject release];
            pMsgObject = NULL;
        }*/
        //cmd stop
    }        
    else if (nMsgId == ENotifyPrepare)
    {
        /*if (iUrlUpdated)
        {
            iPlayer->Stop();
            [pMsgObject release];
            pMsgObject = NULL;
        }*/
    } 
    else if (nMsgId == ENotifyAssetReaderFail)
    {
        if (gIsIOS4X && [self isPlaying]){
            [self pause];
            iAssetReaderFailOrLongPaused = true;
        }else if (errorCode == TTKErrOperationInterrupted) {
            iAssetReaderFailOrLongPaused = true;
            
            //resume from readerfail right now
            [self playWhenInterruptionFinished];
        }else {
            [pMsgObject release];
            pMsgObject = [[GKMsgObject alloc] initwithMsg:ENotifyException andError:TTKErrNotSupported andError0 :0];
        }
    }    
    /*if (nMsgId == ENotifyStop || nMsgId == ENotifyComplete || nMsgId == ENotifyException)
    {
        [self startCountDown];
    }
    else
    {
        [self stopCountDown];
    }*/
    
    if (nMsgId == ENotifyPause)
    {
        [self startCountDown];
    }
    else
    {
        [self stopCountDown];
    }
    
    return pMsgObject;
}

- (void) backgroundIssueProcess : (GKMsgObject*) aMsg
{
    switch (aMsg.iMsg)
    {
        case ENotifyPause:
            if (!playerPausedManually) {
                [self beginBackgroundTask];
            } else {
                [self endBackgroundTask];
            }
        default:
            break;
    }
}

- (TTInt) playWithUrl : (NSString*) aUrl andFlag : (int) aFlag
{
    NSLogDebug(@"Proxy playWithUrl:%@\n", aUrl);

    playerPausedManually = NO;
    iSavedPos = 0;
    iSaveStatus = EStatusStarting;

    [self beginBackgroundTask];
    [self setPlayBackAudioSession:YES];
    
    GKASSERT(iPlayer != NULL);
    if (iCurUrl != NULL)
    {
        [iCurUrl release];
        iCurUrl = NULL;
    }
    
    iCurUrl = [[NSString alloc] initWithString : aUrl];
    //LOGE("URL = %s\n\n",[iCurUrl UTF8String]);
    //iUrlUpdated = false;
    iPlayer->SetDataSourceAsync([iCurUrl UTF8String], aFlag);
    

    return TTKErrNone;
}

- (TTInt) setDataSource : (NSString*) aUrl andFlag : (int) aFlag
{
    GKASSERT((iCurUrl != NULL) && (iPlayer != NULL));
    if (iPlayer->GetPlayStatus() == EStatusStoped)
    {
        //iUrlUpdated = false;
        TTInt nErr = iPlayer->SetDataSourceAsync([aUrl UTF8String], aFlag);
        GKASSERT(nErr == TTKErrNone);
        
        return TTKErrNone;
    }
    
    return TTKErrInUse;
}

- (TTInt) stop
{
    NSLogDebug(@"Proxy stop");
    GKASSERT(iPlayer != NULL);
    return iPlayer->Stop();
}

- (void)stopPlayerWhenNoMusic
{
    playerPausedManually = NO;
    
    [self endBackgroundTask];
}

+ (void) activateAudioDevicesWhenAppLaunchFinished
{
    TTBackgroundAudioQueueConfig::EnableBackground(YES);
}

- (BOOL) isPlaying {
    GKPlayStatus playStatus = iPlayer->GetPlayStatus();
    return  playStatus == EStatusPlaying ||
           playStatus == EStatusPrepared ||
           playStatus == EStatusStarting;
}

- (void)playWhenInterruptionFinished
{
    NSError* error = nil;
    [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if(error)
    {
        NSLogDebug(@"Error : Can't reactivate audio session,%@", error);
    }
    else
    {
        [self resumeAndSetMixOthers];
    }
}

- (void) endBackgroundTask
{
    UIApplication* application = [UIApplication sharedApplication];
    if(UIBackgroundTaskInvalid != self.backgroundTaskId)
    {
        NSLog(@"BackgroundTask end");
        [application endBackgroundTask:self.backgroundTaskId];
        self.backgroundTaskId = UIBackgroundTaskInvalid;
    }
}

- (void) beginBackgroundTask
{
//    UIApplication* application = [UIApplication sharedApplication];
//    if(UIBackgroundTaskInvalid == self.backgroundTaskId)
//    {
//        NSLog(@"BackgroundTask start");
//        self.backgroundTaskId = [application beginBackgroundTaskWithExpirationHandler:^{
//            NSLog(@"BackgroundTask time out");
//            
//            [application endBackgroundTask:self.backgroundTaskId];
//            self.backgroundTaskId = UIBackgroundTaskInvalid;
//            
//            [self beginBackgroundTask];
//        }];
//    }
    
    UIApplication* application = [UIApplication sharedApplication];
    if(UIBackgroundTaskInvalid == self.backgroundTaskId){
        __block typeof(self) weakSelf = self;
        self.backgroundTaskId = [application beginBackgroundTaskWithExpirationHandler:^{
            [application endBackgroundTask:weakSelf.backgroundTaskId];
            weakSelf.backgroundTaskId = UIBackgroundTaskInvalid;
            [weakSelf beginBackgroundTask];
        }];
    }
}

- (void) onTimer
{
    NSLogDebug(@"onTimer");
    GKASSERT(iPlayer != NULL);
    if (iPlayer->GetPlayStatus() == EStatusPaused)
    {
        iAssetReaderFailOrLongPaused = true;   
    }
}

- (void) startCountDown
{
    [self stopCountDown];    
    GKASSERT(iPowerDownCountDownTimer == NULL);
    iPowerDownCountDownTimer = [NSTimer scheduledTimerWithTimeInterval:(100) target:self selector:@selector(onTimer) userInfo:nil repeats:NO];
    [iPowerDownCountDownTimer retain];
    
    NSLogDebug(@"CountDown started!");
}

- (void) stopCountDown
{
    if (iPowerDownCountDownTimer != NULL)
    {        
        [iPowerDownCountDownTimer invalidate];
        [iPowerDownCountDownTimer release];
        iPowerDownCountDownTimer = NULL;
        
        NSLogDebug(@"CountDown stopped!");
    }    
}

- (TTInt) ConfigGeminiWithUrl : (NSString*) aUrl
{
    NSLogDebug(@"ConfigGeminiWithUrl:%@!", aUrl);
    TTInt nErr = TTKErrNone;
    if (gIsIOS4X && aUrl != NULL)
    {        
        if (![iGeminiThreadHandle isExecuting])
        {
            if (gGeminiLeft.IsEnable())
            {
                nErr = gGeminiRight.EnableBackground([aUrl UTF8String], ETTTrue);
                gGeminiRight.EnableBackground(NULL, ETTFalse);
            }
            else
            {
                nErr = gGeminiLeft.EnableBackground([aUrl UTF8String], ETTTrue);
                
                if ((!gGeminiRight.IsEnable()) && nErr == TTKErrNone)
                {
                    GKASSERT(iGeminiUrl == NULL);
                    iGeminiUrl = [aUrl retain];
                    [iGeminiThreadHandle start];
                }
                else
                {
                    gGeminiLeft.EnableBackground(NULL, ETTFalse);                
                }
            }
        }
        else
        {   
            [iCritical lock];
            GKASSERT(iGeminiUrl != NULL);
            [iGeminiUrl release];
            iGeminiUrl = [aUrl retain];
            [iCritical unlock];
        } 
    }
        
    return nErr;
}


- (void) geminiProcL : (NSObject*) aObj
{
    NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];
    
    while (ETTTrue)
    {
        NSTimeInterval tInterval = 60;
        if (iGeminiDone)
        {            
            gGeminiLeft.EnableBackground(NULL, ETTFalse);
            gGeminiRight.EnableBackground(NULL, ETTFalse);          
            
            break;
        }
        else
        {
            TTInt nErr = TTKErrNone;
            if (gGeminiLeft.IsEnable())
            {
                [iCritical lock];
                nErr = gGeminiRight.EnableBackground([iGeminiUrl UTF8String], ETTTrue);
                [iCritical unlock];
                gGeminiLeft.EnableBackground(NULL, ETTFalse);                
                NSLogDebug(@"switch to Right:%d\n", nErr);
            }    
            else
            {
                [iCritical lock];
                nErr = gGeminiLeft.EnableBackground([iGeminiUrl UTF8String], ETTTrue);
                [iCritical unlock];
                gGeminiRight.EnableBackground(NULL, ETTFalse);
                NSLogDebug(@"switch to Left:%d\n", nErr);
            }            
            
            if (nErr != TTKErrNone) 
            {
                tInterval = 0.5;
            }
        }
        
        [NSThread sleepForTimeInterval:tInterval];
    }
    
    [pool release];
}

- (TTInt) BufferedPercent
{
    if (iPlayer != NULL) 
    {
        return iPlayer->BufferedPercent();
    }
    return TTKErrNotReady;
}

- (TTUint) fileSize
{
    if (iPlayer != NULL) {
        return iPlayer->fileSize();
    }
    return TTKErrNotReady;
}

- (TTUint) bufferedFileSize
{
    if (iPlayer != NULL) {
        return iPlayer->bufferedFileSize();
    }
    return TTKErrNotReady;
}

- (void) SetActiveNetWorkType : (GKActiveNetWorkType) aType
{
    GKASSERT(iPlayer != NULL);
    iPlayer->SetActiveNetWorkType(aType);
}

- (void) SetCacheFilePath: (NSString *) path
{
    GKASSERT(iPlayer != NULL && path != NULL);
    iPlayer->SetCacheFilePath([path UTF8String]);
}

- (void) setBalanceChannel:(float)aVolume
{
    iPlayer->SetBalanceChannel(aVolume);
}

- (void) SetView : (UIView*) aView
{
    iPlayer->SetView(aView);
}

- (void) SetEffectBackgroundHandle :(bool)aBackground
{
    /*if (aBackground) {
        gAudioEffectLowDelay = 0;
    }
    else
        gAudioEffectLowDelay = 1;*/
}

- (void) VideoBackgroundHandle
{
    iSaveStatus = iPlayer->GetPlayStatus();
    if (iPlayer->GetPlayStatus() != EStatusStoped) {
        if (iPlayer->IsLiveMode()) {
            iPlayer->Stop();
        }
        else{
            iPlayer->Pause();
            iSavedPos = iPlayer->GetPosition();
            iPlayer->SetView(NULL);
        }
    }
}

- (void) VideoForegroundHandle : (UIView*) aView
{
    if (iPlayer->IsLiveMode()) {
        iPlayer->SetView(aView);
        iPlayer->SetDataSourceAsync([iCurUrl UTF8String], 1);
    }
    else{
        if (iPlayer->GetPlayStatus() != EStatusStoped) {
            iPlayer->SetView(aView);
            iPlayer->SetPosition(iSavedPos, 1);
            if(iSaveStatus == EStatusPlaying) {
                iPlayer->Resume();
            }
        }
    }
}

- (void) SetRotate
{
    if (iPlayer){
        if(EStatusPaused == iPlayer->GetPlayStatus())
            iPlayer->SetRotate();
    }
}

- (void) SetRendType : (TTInt) aRenderType
{
    if (iPlayer){
        iPlayer->SetRendType(aRenderType);
    }
}

- (void) SetMotionEnable : (bool) aEnable
{
    if (iPlayer){
        iPlayer->SetMotionEnable(aEnable);
    }
}

- (void) SetTouchEnable : (bool) aEnable
{
    if (iPlayer){
        iPlayer->SetTouchEnable(aEnable);
    }
}

- (void) setVolume:(float)aVolume
{
    iPlayer->SetVolume(aVolume);
}

- (void) setLogOpenSwitch:(bool)aSwitch
{
    if (aSwitch) {
        setLogOpenSwitch(PRINT_LOG_OPEN);
    }
    else{
        setLogOpenSwitch(PRINT_LOG_CLOSE);//
    }
}

- (void) SetProxyServerConfig: (unsigned int)aIP andPort: (int)aPort andAuthen :(NSString *)aAuthen andUseProxy :(bool)aUseProxy
{
    if (aUseProxy && aAuthen) {
        iPlayer->SetProxyServerConfig(aIP, aPort, [aAuthen UTF8String], aUseProxy);
    }
    else{
        iPlayer->SetProxyServerConfig(0, 0, NULL, aUseProxy);
    }
}

- (void) SetProxyServerConfigByDomain: (NSString *)aDomain andPort: (int)aPort andAuthen :(NSString *)aAuthen andUseProxy :(bool)aUseProxy
{
    if (aUseProxy && aAuthen && aDomain) {
        iPlayer->SetProxyServerConfigByDomain([aDomain UTF8String], aPort, [aAuthen UTF8String], aUseProxy);
    }
    else{
        iPlayer->SetProxyServerConfigByDomain(NULL, 0, NULL, aUseProxy);
    }
}

- (void) SetHostAddr: (NSString *) hostAddr
{
    GKASSERT(iPlayer != NULL);
    if (hostAddr == NULL) {
        iPlayer->SetHostAddr(NULL);
    }
    else{
        iPlayer->SetHostAddr([hostAddr UTF8String]);
    }
}

- (TTInt) BandWidth
{
    if (iPlayer != NULL) {
        return iPlayer->BandWidth();
    }
    return 0;
}

- (TTInt) GetAVPlayType
{
    if (iPlayer != NULL) {
        return iPlayer->GetAVPlayType();
    }
    return AVTYPE_INVALID;
}

- (TTUint) BufferedSize
{
    if (iPlayer != NULL) {
        return iPlayer->BufferedSize();
    }
    return 0;
}

@end
