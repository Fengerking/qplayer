//
//  basePlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 22/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "basePlayer.h"
#import "avPlayer.h"

#ifdef COREPLAYER
#import "corePlayer.h"
#endif

#ifdef PILIPLAYER
#import "piliPlayer.h"
#endif

#ifdef KPLAYER
#import "kPlayer.h"
#endif

#ifdef APLAYER
#import "aPlayer.h"
#endif

#ifdef BPLAYER
#import "bPlayer.h"
#endif

#ifdef TPLAYER
#import "tPlayer.h"
#endif

@implementation basePlayer

-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _url = nil;
        _videoView = nil;
        _openDone = NO;
        _needRun = NO;
        _evtCallback = NULL;
        _evtUserData = NULL;
    }
    
    return self;
}

-(void)dealloc
{
    [super dealloc];
    
    if(_url)
    {
        [_url release];
        _url = nil;
    }
}

-(int)open:(NSString *)url openFlag:(int)flag
{
    if(_url)
        [_url release];
    _url = [url retain];
    NSLog(@"Open URL : %@", _url);
    return 0;
}

- (int)setView:(UIView*)view
{
    _videoView = view;
    
    return QC_ERR_NONE;
}

- (int)setView:(UIView*)view withDrawArea:(RECT*)rect
{
    if(rect)
    	memcpy(&_rect, rect, sizeof(RECT));
    
    return [self setView:view];
}

- (void)setListener:(QCPlayerNotifyEvent)playerEvt withUserData:(void*)userData
{
    _evtUserData = userData;
    _evtCallback = playerEvt;
}

- (int)setParam:(int)PID value:(void*)value
{
    return QC_ERR_STATUS;
}

- (int)setSpeed:(double)speed
{
    return QC_ERR_IMPLEMENT;
}

-(long long)getPlayingTime
{
    return 0;
}

-(long long)getDuration
{
    return 0;
}

-(int)getPlayerType
{
    return UNKNOWN_PLAYER;
}


-(int)sendEvent:(int)evtID withValue:(void*)value
{
    if(_evtCallback && _evtUserData)
    {
        _evtCallback(_evtUserData, evtID, value);
    }
    return 0;
}

-(NSString*)getURL
{
    return _url;
}


+(basePlayer*)createPlayer
{
#ifdef AVPLAYER
    return [[avPlayer alloc] init];
#elif defined COREPLAYER
    return [[corePlayer alloc] init];
#elif defined KPLAYER
    return [[kPlayer alloc] init];
#elif defined APLAYER
    return [[aPlayer alloc] init];
#elif defined BPLAYER
    return [[bPlayer alloc] init];
#elif defined TPLAYER
    return [[tPlayer alloc] init];
#endif

    
    return [[avPlayer alloc] init];
}

+(basePlayer*)createPlayer:(int)type
{
//    if(type == CORE_PLAYER)
//        return [[corePlayer alloc] init];
//    else if(type == AV_PLAYER)
//        return [[avPlayer alloc] init];
    
    return nil;
}

+(int)getSysTime
{
    return [[NSProcessInfo processInfo] systemUptime] * 1000;
}
@end
