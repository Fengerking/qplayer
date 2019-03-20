//
//  bPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 30/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "bPlayer.h"


@interface bPlayer()
{
    BDCloudMediaPlayerController*	_player;
}

@end

@implementation bPlayer

- (void)authStart
{
    NSLog(@"认证开始。");
}

- (void)authEnd:(NSError*)error
{
    NSLog(@"认证完成，error为空表示认证成功。");
}
-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        [[BDCloudMediaPlayerAuth sharedInstance] setAccessKey:@"a3925f51e99d489d8c7ef269a4bb4a9d"];
        //[[BDCloudMediaPlayerAuth sharedInstance] setAccessKey:@"724c9abc6cd9403daece9d4d17c3e31b"];
        //[BaiduAPM startWithApplicationToken:@"de0b9578cf3741b99df94a81d1ee4780"];

        _player = nil;
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
    
    [super dealloc];
}

- (int)open:(NSString*)url openFlag:(int)flag
{
    [self stop];
    [self sendEvent:QC_MSG_PLAY_OPEN_START withValue:NULL];
    [super open:url openFlag:flag];
    
    if(!_player)
    {
        _player = [[BDCloudMediaPlayerController alloc] initWithContentString:url];
        _player.view.frame = _videoView.bounds;
        [_videoView addSubview:_player.view];
        [self setupNotifications];
    }

    [_player setVideoDecodeMode:
     (flag&QCPLAY_OPEN_VIDDEC_HW)?BDCloudMediaPlayerVideoDecodeModeHardware:BDCloudMediaPlayerVideoDecodeModeSoftware];
    _player.contentString = url;
    _player.shouldAutoplay = NO;
    [_player prepareToPlay];

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
        [_player play];
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
        [_player stop];
        [_player reset];
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

-(NSString*)getName
{
    return [NSString stringWithFormat:@"%@ %@", @"bPlayer", [BDCloudMediaPlayerController getSDKVersion]];
}

- (void)setupNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerPrepared:)
                                                 name:BDCloudMediaPlayerPlaybackIsPreparedToPlayNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerFinish:)
                                                 name:BDCloudMediaPlayerPlaybackDidFinishNotification
                                               object:nil];
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerState:)
                                                 name:BDCloudMediaPlayerPlaybackStateDidChangeNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerBufferingStart:)
                                                 name:BDCloudMediaPlayerBufferingStartNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerBufferingUpdate:)
                                                 name:BDCloudMediaPlayerBufferingUpdateNotification
                                               object:nil];
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerBufferingEnd:)
                                                 name:BDCloudMediaPlayerBufferingEndNotification
                                               object:nil];
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerSeekCompleted:)
                                                 name:BDCloudMediaPlayerSeekCompleteNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerFirstVFrame:)
                                                 name:BDCloudMediaPlayerFirstVideoFrameRenderedNotification
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPlayerFirstAFrame:)
                                                 name:BDCloudMediaPlayerFirstAudioFrameRenderedNotification
                                               object:nil];
    
}

- (void)onPlayerFirstVFrame:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    [self sendEvent:QC_MSG_SNKV_FIRST_FRAME withValue:NULL];
}

- (void)onPlayerFirstAFrame:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
//    int connectTime = [basePlayer getSysTime] - _openTime;
//    [self sendEvent:QC_MSG_PRIVATE_CONNECT_TIME withValue:&connectTime];
    
    [self sendEvent:QC_MSG_SNKA_FIRST_FRAME withValue:NULL];
}

- (void)onPlayerPrepared:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    
    [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];
    QC_STREAM_FORMAT fmt;
    fmt.nWidth = _player.naturalSize.width;
    fmt.nHeight = _player.naturalSize.height;
    [self sendEvent:QC_MSG_SNKV_NEW_FORMAT withValue:&fmt];

    [self run];
}

- (void)onPlayerFinish:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    
    NSNumber* reasonNumber = notification.userInfo[BDCloudMediaPlayerPlaybackDidFinishReasonUserInfoKey];
    BDCloudMediaPlayerFinishReason reason = (BDCloudMediaPlayerFinishReason)reasonNumber.integerValue;
    switch (reason) {
        case BDCloudMediaPlayerFinishReasonEnd:
            NSLog(@"player finish with reason: play to end time");
            break;
        case BDCloudMediaPlayerFinishReasonError:
            NSLog(@"player finished with reason: error");
            break;
        case BDCloudMediaPlayerFinishReasonUser:
            NSLog(@"player finished with reason: stopped by user");
            break;
        default:
            break;
    }
    
    [self stop];
}

- (void)onPlayerState:(NSNotification*)notification {
    if (notification.object != _player) {
        return;
    }
    
    //BDCloudMediaPlayerPlaybackState state = _player.playbackState;
}

- (void)onPlayerBufferingStart:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    [self sendEvent:QC_MSG_BUFF_START_BUFFERING withValue:NULL];
    NSLog(@"buffering start");
}

- (void)onPlayerBufferingUpdate:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    
    
    NSLog(@"buffering update progress %@", notification.userInfo[BDCloudMediaPlayerBufferingProgressUserInfoKey]);
}

- (void)onPlayerBufferingEnd:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    NSLog(@"buffering end");
    [self sendEvent:QC_MSG_BUFF_END_BUFFERING withValue:NULL];
}

- (void)onPlayerSeekCompleted:(NSNotification*)notification
{
    if (notification.object != _player) {
        return;
    }
    NSLog(@"player seek completed");
    [self sendEvent:QC_MSG_PLAY_SEEK_DONE withValue:NULL];
}

@end
