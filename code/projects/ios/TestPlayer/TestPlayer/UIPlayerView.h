//
//  UIPlayerView.h
//  TestPlayer
//
//  Created by Jun Lin on 26/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "basePlayer.h"

@interface CGLView : UIView
{
    
}
@end

@interface UIPlayerView : UIView
-(id)initWithFrame:(CGRect)frame player:(basePlayer*)player;
-(void)setPlayer:(basePlayer*)player;
-(void)setVideoView:(UIView*)view;
-(void)open:(NSString*)url;
-(void)stop;
-(void)run;
-(void)pause;
-(void)setVolume:(NSInteger)volume;
-(void)showInfo:(BOOL)show;
-(void)showClose:(BOOL)show;
-(PLAYERINFO*)getPlayerInfo;
-(void)preCache:(NSString*)url;
@end
