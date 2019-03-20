//
//  basePlayer.h
//  TestPlayer
//
//  Created by Jun Lin on 22/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#include "qcPlayer.h"
#include "qcData.h"
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#define QC_MSG_PRIVATE_CONNECT_TIME		0x73009000
#define QC_MSG_PRIVATE_DLD_SPEED		0x73009001


typedef struct
{
    NSString*			url;
    NSInteger			connect;
    NSInteger			startup;
    QC_AUDIO_FORMAT		audio;
    QC_STREAM_FORMAT 	video;
    NSInteger			afps;
    NSInteger			vfps;
    NSInteger			arfps;
    NSInteger			vrfps;
    NSInteger			abuftime;
    NSInteger			vbuftime;
    NSInteger			abitrate;
    NSInteger			vbitrate;
    NSInteger			dspeed;
    NSInteger			gop;
}PLAYERINFO;

typedef enum
{
    CORE_PLAYER,
    AV_PLAYER,
    REF1_PLAYER,
    REF2_PLAYER,
    REF3_PLAYER,
    UNKNOWN_PLAYER
}PLAYERTYPE;

@interface basePlayer : NSObject
{
    NSString* 		_url;
    BOOL			_openDone;
    BOOL			_needRun;
    UIView*			_videoView;
    QCPlayerNotifyEvent _evtCallback;
    void*				_evtUserData;
    RECT				_rect;
}
- (void)setListener:(QCPlayerNotifyEvent)playerEvt withUserData:(void*)userData;
- (int)open:(NSString*)url openFlag:(int)flag;
- (int)setView:(UIView*)view;
- (int)setView:(UIView*)view withDrawArea:(RECT*)rect;
- (int)setVolume:(NSInteger)volume;
- (int)setSpeed:(double)speed;
- (int)run;
- (int)pause;
- (int)stop;
- (long long)seek:(NSNumber*)pos;
- (bool)isLive;
- (int)setParam:(int)PID value:(void*)value;
-(int)sendEvent:(int)evtID withValue:(void*)value;
-(NSString*)getName;
-(NSString*)getURL;
-(long long)getPlayingTime;
-(long long)getDuration;
-(int)getPlayerType;



+(basePlayer*)createPlayer;
+(basePlayer*)createPlayer:(int)type;
+(int)getSysTime;
@end
