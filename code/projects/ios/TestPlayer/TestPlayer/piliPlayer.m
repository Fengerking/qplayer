//
//  piliPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 23/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "piliPlayer.h"
#import <PLPlayerKit/PLPlayerKit.h>

static NSString *status[] = {
    @"PLPlayerStatusUnknow",
    @"PLPlayerStatusPreparing",
    @"PLPlayerStatusReady",
    @"PLPlayerStatusCaching",
    @"PLPlayerStatusPlaying",
    @"PLPlayerStatusPaused",
    @"PLPlayerStatusStopped",
    @"PLPlayerStatusError"
};


@interface piliPlayer() <PLPlayerDelegate>
{
    PLPlayer*	_player;
    int 		_reconnectCount;
}
@end

@implementation piliPlayer

- (int)open:(NSString*)url openFlag:(int)flag
{
    //[self stop];
    
    [self sendEvent:QC_MSG_PLAY_OPEN_START withValue:NULL];
    
    [super open:url openFlag:flag];
    
    if(!_player && [AVAudioSession isPlayable])
    {
        PLPlayerOption *option = [PLPlayerOption defaultOption];
        [option setOptionValue:@10 forKey:PLPlayerOptionKeyTimeoutIntervalForMediaPackets];
        
        _player = [PLPlayer playerWithURL:[NSURL URLWithString:url] option:option];
        _player.delegate = self;
        _player.delegateQueue = dispatch_get_main_queue();
        _player.backgroundPlayEnable = YES;
    }
    
    if (!_player.playerView.superview)
    {
        _player.playerView.backgroundColor = [UIColor whiteColor];
        _player.playerView.contentMode = UIViewContentModeScaleAspectFit;
        _player.playerView.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin
        | UIViewAutoresizingFlexibleTopMargin
        | UIViewAutoresizingFlexibleLeftMargin
        | UIViewAutoresizingFlexibleRightMargin
        | UIViewAutoresizingFlexibleWidth
        | UIViewAutoresizingFlexibleHeight;
        [_videoView addSubview:_player.playerView];
    }
    
    [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];
    return 0;
}

- (int)setVolume:(NSInteger)volume
{
    return 0;
}

- (int)run
{
    if(_player)
       [_player play];
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
    //if(_player)
        //[_player stop];

    return 0;
}

- (long long)seek:(NSNumber*)pos
{
    if(_player)
        [_player play];

    return 0;
}

- (bool)isLive
{
    return true;
}

-(NSString*)getName
{
    return @"piliPlayer";
}

#pragma mark - <PLPlayerDelegate>

- (void)player:(nonnull PLPlayer *)player statusDidChange:(PLPlayerStatus)state
{
    if (PLPlayerStatusCaching == state)
    {
        [self sendEvent:QC_MSG_BUFF_START_BUFFERING withValue:NULL];
        //[self.activityIndicatorView startAnimating];
    } else
    {
        [self sendEvent:QC_MSG_BUFF_END_BUFFERING withValue:NULL];
        //[self.activityIndicatorView stopAnimating];
    }
    NSLog(@"%@", status[state]);
}

- (void)player:(nonnull PLPlayer *)player stoppedWithError:(nullable NSError *)error
{
    //[self.activityIndicatorView stopAnimating];
    [self tryReconnect:error];
}

- (void)tryReconnect:(nullable NSError *)error {
//    if (self.reconnectCount < 3) {
//        _reconnectCount ++;
//        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"错误" message:[NSString stringWithFormat:@"错误 %@，播放器将在%.1f秒后进行第 %d 次重连", error.localizedDescription,0.5 * pow(2, self.reconnectCount - 1), _reconnectCount] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
//        [alert show];
//        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * pow(2, self.reconnectCount) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
//            [self.player play];
//        });
//    }else {
//        if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"8.0")) {
//            UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Error"
//                                                                           message:error.localizedDescription
//                                                                    preferredStyle:UIAlertControllerStyleAlert];
//            __weak typeof(self) wself = self;
//            UIAlertAction *cancel = [UIAlertAction actionWithTitle:@"OK"
//                                                             style:UIAlertActionStyleCancel
//                                                           handler:^(UIAlertAction *action) {
//                                                               __strong typeof(wself) strongSelf = wself;
//                                                               [strongSelf.navigationController performSelectorOnMainThread:@selector(popViewControllerAnimated:) withObject:@(YES) waitUntilDone:NO];
//                                                           }];
//            [alert addAction:cancel];
//            [self presentViewController:alert animated:YES completion:nil];
//        }
//        else {
//            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error" message:error.localizedDescription delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
//            [alert show];
//        }
//        [UIApplication sharedApplication].idleTimerDisabled = NO;
//        NSLog(@"%@", error);
//    }
}



@end
