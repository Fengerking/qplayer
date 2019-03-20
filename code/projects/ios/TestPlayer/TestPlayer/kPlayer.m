//
//  kPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 24/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "kPlayer.h"
#import "KSYMoviePlayerController.h"
@interface kPlayer()
{
    KSYMoviePlayerController*	_player;
    NSMutableArray*				_registeredNotifications;
}
@end


@implementation kPlayer

-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _player = nil;
        _registeredNotifications = nil;
    }
    
    return self;
}

-(void)dealloc
{
    [self destroy];
    
    [super dealloc];
}

-(void)destroy
{
    if(_player)
    {
        // stop is same as close
        [_player stop];
        [_player removeObserver:self forKeyPath:@"currentPlaybackTime" context:nil];
        [_player removeObserver:self forKeyPath:@"clientIP" context:nil];
        [_player removeObserver:self forKeyPath:@"localDNSIP" context:nil];
        [self releaseObservers:_player];
        [_player.view removeFromSuperview];
        [_player release];
        _player = nil;
    }
}

- (int)open:(NSString*)url openFlag:(int)flag
{
    [self stop];
    [super open:url openFlag:flag];
    [self sendEvent:QC_MSG_PLAY_OPEN_START withValue:NULL];
    
    if(!_player)
    {
        _player = [[KSYMoviePlayerController alloc] initWithContentURL:[NSURL URLWithString:url] fileList:nil sharegroup:nil];
        //播放视频的实时信息
        _player.controlStyle = MPMovieControlStyleNone;
        [_player.view setFrame: _videoView.bounds];  // player's frame must match parent's
        [_videoView addSubview: _player.view];
        _videoView.autoresizesSubviews = TRUE;
        _player.videoDecoderMode |= ((flag&QCPLAY_OPEN_VIDDEC_HW)==QCPLAY_OPEN_VIDDEC_HW) ? MPMovieVideoDecoderMode_Hardware : MPMovieVideoDecoderMode_Software;
        _player.view.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    }
    else
    {
        _player.videoDecoderMode |= ((flag&QCPLAY_OPEN_VIDDEC_HW)==QCPLAY_OPEN_VIDDEC_HW) ? MPMovieVideoDecoderMode_Hardware : MPMovieVideoDecoderMode_Software;
        [_player setUrl:[NSURL URLWithString:url]];
        [_player prepareToPlay];
        return 0;
    }
    
    [self setupObservers:_player];
    
    _player.logBlock = ^(NSString *logJson){
        NSLog(@"logJson is %@",logJson);
    };
    
//    _player.videoDataBlock = ^(CMSampleBufferRef sampleBuffer){
//        //写入视频sampleBuffer
//        if(weakSelf && weakSelf.AVWriter && weakSelf.bRecording)
//            [weakSelf.AVWriter processVideoSampleBuffer:sampleBuffer];
//    };
    
//    _player.audioDataBlock = ^(CMSampleBufferRef sampleBuffer){
//        //写入音频sampleBuffer
//        if(weakSelf && weakSelf.AVWriter && weakSelf.bRecording)
//            [weakSelf.AVWriter processAudioSampleBuffer:sampleBuffer];
//    };
    _player.messageDataBlock = ^(NSDictionary *message, int64_t pts, int64_t param){
        if(message)
        {
            NSMutableString *msgString = [[NSMutableString alloc] init];
            NSEnumerator * enumeratorKey = [message keyEnumerator];
            //快速枚举遍历所有KEY的值
            for (NSObject *object in enumeratorKey) {
                [msgString appendFormat:@"\"%@\":\"%@\"\n", object, [message objectForKey:object]];
            }
        }
    };
    
    
        //设置播放参数
    _player.videoDecoderMode = ((flag&QCPLAY_OPEN_VIDDEC_HW)==QCPLAY_OPEN_VIDDEC_HW)?MPMovieVideoDecoderMode_Hardware:MPMovieVideoDecoderMode_Software;
    //_player.scalingMode = config.contentMode;
    _player.shouldAutoplay = NO;
    //_player.deinterlaceMode = config.deinterlaceMode;
    _player.shouldLoop = NO;
    //_player.bInterruptOtherAudio = config.bAudioInterrupt;
    //_player.bufferTimeMax = config.bufferTimeMax;
    //_player.bufferSizeMax = config.bufferSizeMax;
    //[_player setTimeout:config.connectTimeout readTimeout:config.readTimeout];

    
    NSKeyValueObservingOptions opts = NSKeyValueObservingOptionNew;
    [_player addObserver:self forKeyPath:@"currentPlaybackTime" options:opts context:nil];
    [_player addObserver:self forKeyPath:@"clientIP" options:opts context:nil];
    [_player addObserver:self forKeyPath:@"localDNSIP" options:opts context:nil];
    
    [_player prepareToPlay];
    return 0;
}

- (int)run
{
    if(_player)
    {
        if(_player.shouldAutoplay == NO)
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
        [_player reset:NO];
    }
    
    return 0;
}

- (long long)seek:(NSNumber*)pos
{
    return 0;
}

- (bool)isLive
{
    return true;
}

- (int)setVolume:(NSInteger)volume
{
    return 0;
}

-(NSString*)getName
{
    return [NSString stringWithFormat:@"%@ %@", @"kPlayer", _player?[_player getVersion]:@""];
}


- (void)registerObserver:(NSString *)notification player:(KSYMoviePlayerController*)player {
    [[NSNotificationCenter defaultCenter]addObserver:self
                                            selector:@selector(handlePlayerNotify:)
                                                name:(notification)
                                              object:player];
}

- (void)setupObservers:(KSYMoviePlayerController*)player
{
    [self registerObserver:MPMediaPlaybackIsPreparedToPlayDidChangeNotification player:player];
    [self registerObserver:MPMoviePlayerPlaybackStateDidChangeNotification player:player];
    [self registerObserver:MPMoviePlayerPlaybackDidFinishNotification player:player];
    [self registerObserver:MPMoviePlayerLoadStateDidChangeNotification player:player];
    [self registerObserver:MPMovieNaturalSizeAvailableNotification player:player];
    [self registerObserver:MPMoviePlayerFirstVideoFrameRenderedNotification player:player];
    [self registerObserver:MPMoviePlayerFirstAudioFrameRenderedNotification player:player];
    [self registerObserver:MPMoviePlayerSuggestReloadNotification player:player];
    [self registerObserver:MPMoviePlayerPlaybackStatusNotification player:player];
    [self registerObserver:MPMoviePlayerNetworkStatusChangeNotification player:player];
    [self registerObserver:MPMoviePlayerSeekCompleteNotification player:player];
}

- (void)releaseObservers:(KSYMoviePlayerController*)player
{
    for (NSString *name in _registeredNotifications)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:name
                                                      object:player];
    }
}

-(void)handlePlayerNotify:(NSNotification*)notify
{
    if (!_player) {
        return;
    }
    NSLog(@"Recv msg %@", notify.name);
    if (MPMediaPlaybackIsPreparedToPlayDidChangeNotification ==  notify.name)
    {
        [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];
        
        NSLog(@"KSYPlayerVC: %@ -- ip:%@", [[_player contentURL] absoluteString], [_player serverAddress]);
        //reloading = NO;
    }
    if (MPMoviePlayerPlaybackStateDidChangeNotification ==  notify.name) {
        NSLog(@"------------------------");
        NSLog(@"player playback state: %ld", (long)_player.playbackState);
        NSLog(@"------------------------");
    }
    if (MPMoviePlayerLoadStateDidChangeNotification ==  notify.name) {
        NSLog(@"player load state: %ld", (long)_player.loadState);
        if (MPMovieLoadStateStalled & _player.loadState) {
            //labelStat.text = [NSString stringWithFormat:@"player start caching"];
            NSLog(@"player start caching");
        }
        
        if (_player.bufferEmptyCount &&
            (MPMovieLoadStatePlayable & _player.loadState ||
             MPMovieLoadStatePlaythroughOK & _player.loadState)){
                NSLog(@"player finish caching");
                NSString *message = [[NSString alloc]initWithFormat:@"loading occurs, %d - %0.3fs",
                                     (int)_player.bufferEmptyCount,
                                     _player.bufferEmptyDuration];
                //[self toast:message];
            }
    }
    if (MPMoviePlayerPlaybackDidFinishNotification ==  notify.name) {
        NSLog(@"player finish state: %ld", (long)_player.playbackState);
        NSLog(@"player download flow size: %f MB", _player.readSize);
        NSLog(@"buffer monitor  result: \n   empty count: %d, lasting: %f seconds",
              (int)_player.bufferEmptyCount,
              _player.bufferEmptyDuration);
        //结束播放的原因
//        int reason = [[[notify userInfo] valueForKey:MPMoviePlayerPlaybackDidFinishReasonUserInfoKey] intValue];
//        if (reason ==  MPMovieFinishReasonPlaybackEnded) {
//            labelStat.text = [NSString stringWithFormat:@"player finish"];
//        }else if (reason == MPMovieFinishReasonPlaybackError){
//            labelStat.text = [NSString stringWithFormat:@"player Error : %@", [[notify userInfo] valueForKey:@"error"]];
//        }else if (reason == MPMovieFinishReasonUserExited){
//            labelStat.text = [NSString stringWithFormat:@"player userExited"];
//            
//        }
    }
    if (MPMovieNaturalSizeAvailableNotification ==  notify.name) {
        NSLog(@"video size %.0f-%.0f, rotate:%ld\n", _player.naturalSize.width, _player.naturalSize.height, (long)_player.naturalRotate);
        QC_VIDEO_FORMAT fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.nWidth = _player.naturalSize.width;
        fmt.nHeight = _player.naturalSize.height;
        [self sendEvent:QC_MSG_SNKV_NEW_FORMAT withValue:&fmt];
        if(((_player.naturalRotate / 90) % 2  == 0 && _player.naturalSize.width > _player.naturalSize.height) ||
           ((_player.naturalRotate / 90) % 2 != 0 && _player.naturalSize.width < _player.naturalSize.height))
        {
            //如果想要在宽大于高的时候横屏播放，你可以在这里旋转
        }
    }
    if (MPMoviePlayerFirstVideoFrameRenderedNotification == notify.name)
    {
//        fvr_costtime = (int)((long long int)([self getCurrentTime] * 1000) - prepared_time);
//        NSLog(@"first video frame show, cost time : %dms!\n", fvr_costtime);
        [self sendEvent:QC_MSG_SNKV_FIRST_FRAME withValue:NULL];
    }
    
    if (MPMoviePlayerFirstAudioFrameRenderedNotification == notify.name)
    {
//        far_costtime = (int)((long long int)([self getCurrentTime] * 1000) - prepared_time);
//        NSLog(@"first audio frame render, cost time : %dms!\n", far_costtime);
        [self sendEvent:QC_MSG_SNKA_FIRST_FRAME withValue:NULL];
    }
    
    if (MPMoviePlayerSuggestReloadNotification == notify.name)
    {
        NSLog(@"suggest using reload function!\n");
//        if(!reloading)
//        {
//            reloading = YES;
//            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(){
//                if (_player) {
//                    NSLog(@"reload stream");
//                    [_player reload:_reloadUrl flush:YES mode:MPMovieReloadMode_Accurate];
//                }
//            });
//        }
    }
    
    if(MPMoviePlayerPlaybackStatusNotification == notify.name)
    {
        int status = [[[notify userInfo] valueForKey:MPMoviePlayerPlaybackStatusUserInfoKey] intValue];
        if(MPMovieStatusVideoDecodeWrong == status)
            NSLog(@"Video Decode Wrong!\n");
        else if(MPMovieStatusAudioDecodeWrong == status)
            NSLog(@"Audio Decode Wrong!\n");
        else if (MPMovieStatusHWCodecUsed == status )
            NSLog(@"Hardware Codec used\n");
        else if (MPMovieStatusSWCodecUsed == status )
            NSLog(@"Software Codec used\n");
        else if(MPMovieStatusDLCodecUsed == status)
            NSLog(@"AVSampleBufferDisplayLayer  Codec used");
    }
    if(MPMoviePlayerNetworkStatusChangeNotification == notify.name)
    {
        int currStatus = [[[notify userInfo] valueForKey:MPMoviePlayerCurrNetworkStatusUserInfoKey] intValue];
        int lastStatus = [[[notify userInfo] valueForKey:MPMoviePlayerLastNetworkStatusUserInfoKey] intValue];
        //NSLog(@"network reachable change from %@ to %@\n", [self netStatus2Str:lastStatus], [self netStatus2Str:currStatus]);
    }
    if(MPMoviePlayerSeekCompleteNotification == notify.name)
    {
        NSLog(@"Seek complete");
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    if([keyPath isEqual:@"currentPlaybackTime"])
    {
        //progressView.playProgress = _player.currentPlaybackTime / _player.duration;
        
        NSDictionary* meta = [_player getMetadata];
        KSYQosInfo *info = _player.qosInfo;
        
        if(_player.currentPlaybackTime>0 && _player.currentPlaybackTime<5)
        {
            int connectTime = (int)[(NSNumber *)[meta objectForKey:kKSYPLYHttpConnectTime] integerValue];
            [self sendEvent:QC_MSG_PRIVATE_CONNECT_TIME withValue:&connectTime];
            
            int firstByteTime = (int)[(NSNumber *)[meta objectForKey:kKSYPLYHttpFirstDataTime] integerValue];
            [self sendEvent:QC_MSG_IO_FIRST_BYTE_DONE withValue:&firstByteTime];
        }
        
        int vBufTime = info.videoBufferTimeLength;
        int aBufTime = info.audioBufferTimeLength;
        [self sendEvent:QC_MSG_BUFF_VBUFFTIME withValue:&vBufTime];
        [self sendEvent:QC_MSG_BUFF_ABUFFTIME withValue:&aBufTime];
        
        int vFPS = (int)info.videoRefreshFPS;
        [self sendEvent:QC_MSG_RENDER_VIDEO_FPS withValue:&vFPS];
        

    }
    else if([keyPath isEqual:@"clientIP"])
    {
        NSLog(@"client IP is %@\n", [change objectForKey:NSKeyValueChangeNewKey]);
    }
    else if([keyPath isEqual:@"localDNSIP"])
    {
        NSLog(@"local DNS IP is %@\n", [change objectForKey:NSKeyValueChangeNewKey]);
    }
    else if ([keyPath isEqualToString:@"player"]) {
    }
}


@end
