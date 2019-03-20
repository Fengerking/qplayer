//
//  kPlayer.h
//  TestPlayer
//
//  Created by Jun Lin on 24/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "basePlayer.h"

@interface kPlayer : basePlayer
- (int)open:(NSString*)url openFlag:(int)flag;
- (int)setVolume:(NSInteger)volume;
- (int)run;
- (int)pause;
- (int)stop;
- (long long)seek:(NSNumber*)pos;
- (bool)isLive;
- (NSString*)getName;
@end
