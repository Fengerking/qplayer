//
//  GKPlayerInterface.h
//  Player
//
//

#include "GKTypedef.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>
#import <CoreMedia/CoreMedia.h>

#include "GKNetWorkType.h"
#include "GKMsgEnumDef.h"

#define AVTYPE_INVALID         0
#define AVTYPE_AUDIO_ONLY      1
#define AVTYPE_VIDEO_ONLY      2
#define AVTYPE_AUDIOVIDEO      3


@protocol GKPlayerInterface <NSObject>

@required

- (void) pause;
- (void) resume;
- (void) resumeAndSetMixOthers;
- (void) inactiveAudioSession;

- (void) setPosition : (CMTime) aTime;

- (void) setPlayRangeWithStartTime : (CMTime) aStartTime EndTime : (CMTime) aEndTime;

- (CMTime) getPosition;

- (CMTime) duration;
- (TTInt) getCurFreqAndWaveWithFreqBuffer : (TTInt16*) aFreqBuffer andWaveBuffer : (TTInt16*) aWaveBuffer andSamplenum :(TTInt) aSampleNum;
- (enum GKPlayStatus) getPlayStatus;

- (TTInt) playWithUrl : (NSString*) aUrl andFlag : (int) aFlag;
- (TTInt) stop;
- (TTInt) start;

- (void) ProcessNotifyEventWithMsg : (enum GKNotifyMsg) aMsg andError : (TTInt) aError  andError0: (TTInt) aError0;
- (TTInt) ConfigGeminiWithUrl : (NSString*) aUrl;
- (void) SetActiveNetWorkType : (enum GKActiveNetWorkType) aType;
- (void) SetCacheFilePath: (NSString *) path;
- (void) AddIrFilePath: (NSString *) path;

- (void) stopPlayerWhenNoMusic;//when no song plays, stop player
+ (void) activateAudioDevicesWhenAppLaunchFinished;//in order to avoid init audio unit failed, first activate audio devices

- (TTInt) BufferedPercent;
- (TTUint) fileSize;
- (TTUint) bufferedFileSize;

- (void) SetView : (UIView*) aView;
- (void) SetEffectBackgroundHandle :(bool)aBackground;
- (void) VideoBackgroundHandle;
- (void) VideoForegroundHandle : (UIView*) aView;
- (TTInt) BandWidth ;
- (TTUint) BufferedSize;
- (void) SetRotate ;
- (TTInt) GetAVPlayType;
- (void) SetRendType : (TTInt) aRenderType;
- (void) SetMotionEnable : (bool) aEnable;

- (void) SetHostAddr: (NSString *) hostAddr;

- (void) SetProxyServerConfig: (unsigned int)aIP andPort: (int)aPort andAuthen :(NSString *)aAuthen andUseProxy :(bool)aUseProxy;

- (void) SetProxyServerConfigByDomain: (NSString *)aDomain andPort: (int)aPort andAuthen :(NSString *)aAuthen andUseProxy :(bool)aUseProxy;

@optional

- (void) setBalanceChannel:(float)aVolume;
- (void) setVolume:(float)aVolume;
- (void) setLogOpenSwitch:(bool)aSwitch;

@end
