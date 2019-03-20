//
//  tPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 03/07/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "tPlayer.h"
#import <TXRTMPSDK/TXLivePlayer.h>

@interface tPlayer() <TXLivePlayListener>
{
    TXLivePlayer*	_player;
    TXLivePlayConfig*  _config;
    QC_STREAM_FORMAT _fmt;
}
@end

@implementation tPlayer
-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _player = nil;
        _config = nil;
    }
    
    return self;
}

-(void)dealloc
{
    if(_player)
    {
        [_player release];
        _player = nil;
    }
    if(_config)
    {
        [_config release];
        _config = nil;
    }
    
    
    [super dealloc];
}

- (int)open:(NSString*)url openFlag:(int)flag
{
    //[self stop];
    [self sendEvent:QC_MSG_PLAY_OPEN_START withValue:NULL];
    [super open:url openFlag:flag];
    
    if(!_player)
    {
        _player = [[TXLivePlayer alloc] init];
        _player.delegate = self;
    }
    [_player setupVideoWidget:CGRectMake(0, 0, 0, 0) containView:_videoView insertIndex:0];
    
    if(!_config)
    {
        _config = [[TXLivePlayConfig alloc] init];
        //自动模式
//        _config.bAutoAdjustCacheTime   = YES;
//        _config.minAutoAdjustCacheTime = 1;
//        _config.maxAutoAdjustCacheTime = 5;
        //极速模式
        _config.bAutoAdjustCacheTime   = YES;
        _config.minAutoAdjustCacheTime = 1;
        _config.maxAutoAdjustCacheTime = 1;
        //流畅模式
//        _config.bAutoAdjustCacheTime   = NO;
//        _config.cacheTime              = 5;
        
        [_player setConfig: _config];
    }
    
    _fmt.nWidth = 0;
    [self setView:_videoView];
    _player.enableHWAcceleration = (flag&QCPLAY_OPEN_VIDDEC_HW)?YES:NO;
    [_player startPlay:url type:PLAY_TYPE_LIVE_RTMP];
    
    return 0;
}


- (int)setVolume:(NSInteger)volume
{
    return 0;
}

- (int)run
{
    if(_player)
    {
        [_player resume];
    }
    
    return 0;
}

- (int)pause
{
    if(_player)
        [_player pause];
    return 0;
}

- (int)stop
{
    if(_player)
    {
        [_player stopPlay];
        [_player removeVideoWidget];
    }
    
    return 0;
}

- (long long)seek:(NSNumber*)pos
{
    return 0;
}

- (bool)isLive
{
    return YES;
}

- (int)setView:(UIView *)view
{
    if(_player)
    {
        if(_videoView.contentMode == UIViewContentModeScaleToFill || _videoView.contentMode == UIViewContentModeScaleAspectFill)
            [_player setRenderMode: RENDER_MODE_FILL_SCREEN];
        else if(_videoView.contentMode == UIViewContentModeScaleAspectFit)
            [_player setRenderMode:RENDER_MODE_FILL_EDGE];
    }
    
    return [super setView:view];
}

-(void) onPlayEvent:(int)EvtID withParam:(NSDictionary*)param
{
    if(EvtID == PLAY_EVT_CONNECT_SUCC)
    {
        [self sendEvent:QC_MSG_RTMP_CONNECT_SUCESS withValue:NULL];
    }
    else if(EvtID == PLAY_EVT_RCV_FIRST_I_FRAME)
    {
        [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];
    }
    else if(EvtID == PLAY_EVT_PLAY_BEGIN)
    {
        [self sendEvent:QC_MSG_SNKV_FIRST_FRAME withValue:NULL];
        [self sendEvent:QC_MSG_BUFF_END_BUFFERING withValue:NULL];
    }
    else if(EvtID == PLAY_EVT_PLAY_LOADING)
    {
        [self sendEvent:QC_MSG_BUFF_START_BUFFERING withValue:NULL];
    }
    else if(EvtID == PLAY_EVT_PLAY_END)
    {
    }
}

-(void) onNetStatus:(NSDictionary*) param
{
    if(_fmt.nWidth == 0)
    {
        _fmt.nWidth = (int)[[param objectForKey:NET_STATUS_VIDEO_WIDTH] integerValue];
        _fmt.nHeight = (int)[[param objectForKey:NET_STATUS_VIDEO_HEIGHT] integerValue];
        [self sendEvent:QC_MSG_SNKV_NEW_FORMAT withValue:&_fmt];
    }
    
    int vFPS = (int)[[param objectForKey:NET_STATUS_VIDEO_FPS] integerValue];
    [self sendEvent:QC_MSG_BUFF_VFPS withValue:&vFPS];
    
    int vBuf = (int)[[param objectForKey:NET_STATUS_VIDEO_BITRATE] integerValue];
    [self sendEvent:QC_MSG_BUFF_VBITRATE withValue:&vBuf];
    
    int aBuf = (int)[[param objectForKey:NET_STATUS_AUDIO_BITRATE] integerValue];
    [self sendEvent:QC_MSG_BUFF_ABITRATE withValue:&aBuf];
}

-(NSString*)getName
{
    return [NSString stringWithFormat:@"%@ %@.%@.%@", @"tPlayer",
            [[TXLivePlayer getSDKVersion] objectAtIndex:0], [[TXLivePlayer getSDKVersion] objectAtIndex:1], [[TXLivePlayer getSDKVersion] objectAtIndex:2]];
}

@end
