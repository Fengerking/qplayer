/**
* File : GKMediaPlayerProxy.h 
* Created on : 2011-9-1
* Description : CGKMediaPlayerWarp 定义文件
*/
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "GKMediaPlayerWarp.h"
#import "GKPlayerInterface.h"

extern NSString *NewCallingNotification;
extern NSString *kIsCalling;


@protocol GKMediaPlayerEventChangedProtocol <NSObject>

- (void) mediaPlayerEventChanged : (id) aMsgObject;

@end

@interface GKMsgObject : NSObject {

}
@property (readonly) GKNotifyMsg iMsg;
@property (readonly) TTInt       iError;
@property (readonly) TTInt       iError0;
- (id) initwithMsg: (GKNotifyMsg) aMsg andError: (TTInt) aError andError0: (TTInt) aError0;
@end

@interface GKMediaPlayerProxy : NSObject <GKPlayerInterface> {

@private
    CGKMediaPlayerWarp*     iPlayer;
    //UIBackgroundTaskIdentifier  backgroundTaskId;
    Boolean                 iUrlUpdated;
    NSString*               iCurUrl;
    Boolean                 iAssetReaderFailOrLongPaused;
    NSTimer*                iPowerDownCountDownTimer;
    NSString*               iGeminiUrl;
    Boolean                 iGeminiDone;
    NSThread*               iGeminiThreadHandle;
    NSLock*                 iCritical;
    TTUint                  iSavedPos;
    GKPlayStatus            iSaveStatus;
    
    BOOL                    playerPausedManually;
}


@property (assign, nonatomic) UIBackgroundTaskIdentifier backgroundTaskId;
@property (readonly) Boolean       interruptedWhilePlaying;
@property (nonatomic, assign)  id<GKMediaPlayerEventChangedProtocol> playerDelegate;

@end
