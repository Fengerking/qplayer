//
//  corePlayer.h
//  TestPlayer
//
//  Created by Jun Lin on 07/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "basePlayer.h"

static NSString*	evtOpenDone = @"cpOpenDone";


@interface corePlayer : basePlayer
- (void)setListener:(QCPlayerNotifyEvent)playerEvt withUserData:(void*)userData;
- (int)open:(NSString*)url openFlag:(int)flag;
- (int)setView:(UIView*)view;
- (int)setVolume:(NSInteger)volume;
- (int)run;
- (int)pause;
- (int)stop;
- (long long)seek:(NSNumber*)pos;
- (bool)isLive;
- (int)setParam:(int)PID value:(void*)value;
- (int)setSpeed:(double)speed;
-(NSString*)getName;
@end
