//
//  avPlayer.h
//  TestPlayer
//
//  Created by Jun Lin on 22/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "basePlayer.h"

@interface avPlayer : basePlayer
- (int)open:(NSString*)url openFlag:(int)flag;
- (int)setVolume:(NSInteger)volume;
- (int)run;
- (int)pause;
- (int)stop;
- (long long)seek:(NSNumber*)pos;
- (bool)isLive;
-(NSString*)getName;
@end
