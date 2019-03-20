//
//  corePlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 07/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "corePlayer.h"
#include "qcPlayer.h"
#include "qcData.h"

@interface corePlayer()
{
    QCM_Player	_player;
}

@end

@implementation corePlayer

-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _videoView = nil;
        qcCreatePlayer(&_player, NULL);
        [self reset];
    }
    
    return self;
}

- (void)setListener:(QCPlayerNotifyEvent)playerEvt withUserData:(void*)userData
{
    if(_player.hPlayer)
    	_player.SetNotify(_player.hPlayer, playerEvt, userData);
}

-(void)reset
{
    _openDone = NO;
    _needRun = NO;
}

-(BOOL)isSame:(NSString*)newURL
{
    if(!_url)
        return false;
    
    return [_url isEqualToString:newURL];
}

- (int)open:(NSString*)url openFlag:(int)flag
{
    //[self stop];
    [super open:url openFlag:flag];
    
    if(_player.hPlayer)
    {
        NSLog(@"PLAYER handle : %X", _player.hPlayer);
        [self reset];
        _needRun = true;
        //int log = 0;
        //_player.SetParam(_player.hPlayer, QCPLAY_PID_Log_Level, &log);
        //_player.SetParam(_player.hPlayer, QCPLAY_PID_DRM_KeyText, (void*)"XXXXXXXXXXXX");
//        int nVal = QC_PLAY_VideoDisable_Render;
//        _player.SetParam(_player.hPlayer, QCPLAY_PID_Disable_Video, &nVal);

#if 0
        int loop = 1;
        _player.SetParam(_player.hPlayer, QCPLAY_PID_Playback_Loop, &loop);
#endif

        return _player.Open(_player.hPlayer, [url UTF8String], flag);
    }
    
    return QC_ERR_NONE;
}

- (int)setView:(UIView*)view
{
    [super setView:view];
    if(_player.hPlayer)
    {
//        _rect.right /= 2;
//        _rect.bottom /= 2;
        return _player.SetView(_player.hPlayer, (void*)view, &_rect);
    }
    
    return QC_ERR_NONE;
}

- (int)run
{
    if(_player.hPlayer)
    {
        return _player.Run(_player.hPlayer);
    }
    return QC_ERR_NONE;
}

- (int)pause
{
    if(_player.hPlayer)
    {
        return _player.Pause(_player.hPlayer);
    }
    return QC_ERR_NONE;
}

- (int)stop
{
    if(_player.hPlayer)
    {
		[self reset];
        //int time = [basePlayer getSysTime];
        _player.Stop(_player.hPlayer);
        //NSLog(@"[K]core stop, %d", [basePlayer getSysTime]-time);
        //_player.Close(_player.hPlayer);
        //NSLog(@"[K]core close, %d", [basePlayer getSysTime]-time);
    }
    return QC_ERR_NONE;
}

- (long long)seek:(NSNumber*)pos
{
    if(_player.hPlayer)
    {
        return _player.SetPos(_player.hPlayer, [pos longLongValue]);
    }
    return QC_ERR_NONE;
}

- (int)setVolume:(NSInteger)volume
{
    if(_player.hPlayer)
    {
        _player.SetVolume(_player.hPlayer, (int)volume);
    }
    return QC_ERR_NONE;
}

- (int)setSpeed:(double)speed
{
    if(_player.hPlayer)
    {
        _player.SetParam(_player.hPlayer, QCPLAY_PID_Speed, &speed);
    }
    return QC_ERR_NONE;
}


-(bool)isLive
{
    if(_player.hPlayer)
    {
        if(_player.GetDur(_player.hPlayer) > 0)
            return NO;
    }
    
    return YES;
}

- (int)setParam:(int)PID value:(void*)value
{
    if(_player.hPlayer)
    {
        return _player.SetParam(_player.hPlayer, PID, value);
    }
    
    return QC_ERR_STATUS;
}

-(NSString*)getName
{
//    return [NSString stringWithFormat:@"%@ %d.%d.%d.%d",
//            @"corePlayer",
//            (_player.nVersion&0xFF000000)>>24,
//            (_player.nVersion&0x00FF0000)>>16,
//            (_player.nVersion&0x0000FF00)>>8,
//             _player.nVersion&0x000000FF];
    return [NSString stringWithFormat:@"%@ %d.%d.%d.%d",
            @"corePlayer",
            (_player.nVersion>>24) & 0xFF,
            (_player.nVersion>>16) & 0xFF,
            (_player.nVersion>>8) & 0xFF,
            _player.nVersion&0xFF];
    
}

-(long long)getPlayingTime
{
    if(_player.hPlayer)
    {
        return _player.GetPos(_player.hPlayer);
    }
    return 0;
}

-(long long)getDuration
{
    if(_player.hPlayer)
    {
        return _player.GetDur(_player.hPlayer);
    }
    return 0;
}

-(int)getPlayerType
{
    return CORE_PLAYER;
}


-(void)dealloc
{
    if(_player.hPlayer)
    {
        _player.Close(_player.hPlayer);
        qcDestroyPlayer(&_player);
        _player.hPlayer = NULL;
    }
    
    [super dealloc];
}

@end
