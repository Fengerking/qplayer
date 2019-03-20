//
//  avPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 22/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "avPlayer.h"
#import <AVFoundation/AVFoundation.h>

//NSString * const kTracksKey         = @"tracks";
//NSString * const kPlayableKey		= @"playable";
NSString * const kStatusKey         = @"status";
NSString * const kTimeRangesKVO     = @"loadedTimeRanges";
NSString * const kPlaybackBufferEmpty     = @"playbackBufferEmpty";
NSString * const kPlaybackLikelyToKeepUp     = @"playbackLikelyToKeepUp";

static void *AVPlayerPlaybackViewControllerRateObservationContext = &AVPlayerPlaybackViewControllerRateObservationContext;
static void *AVPlayerPlaybackViewControllerStatusObservationContext = &AVPlayerPlaybackViewControllerStatusObservationContext;
static void *AVPlayerPlaybackViewControllerCurrentItemObservationContext = &AVPlayerPlaybackViewControllerCurrentItemObservationContext;
static void *AVPlayerPlaybackViewControllerPlaybackBufferObservationContext = &AVPlayerPlaybackViewControllerPlaybackBufferObservationContext;



@interface avPlayer()
{
    AVAsset* 		_asset;
    AVPlayer*		_player;
    AVPlayerItem*	_item;
    AVPlayerLayer*	_layer;
}

@end

@implementation avPlayer


-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _player = nil;
        _item = nil;
    }
    
    return self;
}


- (int)open:(NSString*)url openFlag:(int)flag
{
    [self stop];
    [super open:url openFlag:flag];
    [self sendEvent:QC_MSG_PLAY_OPEN_START withValue:NULL];
    
    NSURL *sourceURL = nil;
    if([url hasPrefix:@"/var"])
    	sourceURL = [NSURL fileURLWithPath: url];
    else
        sourceURL = [NSURL URLWithString: url];
    
    _asset = [AVURLAsset URLAssetWithURL:sourceURL options:nil];
    _item     = [AVPlayerItem playerItemWithAsset:_asset];
    
    [_item addObserver:self
            		forKeyPath:kStatusKey
                    options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                    context:AVPlayerPlaybackViewControllerStatusObservationContext];
    [_item addObserver:self
            		forKeyPath:kPlaybackBufferEmpty
               		options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
               		context:AVPlayerPlaybackViewControllerStatusObservationContext];
    [_item addObserver:self
            		forKeyPath:kTimeRangesKVO
               		options:NSKeyValueObservingOptionNew
               		context:AVPlayerPlaybackViewControllerPlaybackBufferObservationContext];
    [_item addObserver:self
            		forKeyPath:kPlaybackLikelyToKeepUp
               		options:NSKeyValueObservingOptionNew
               		context:AVPlayerPlaybackViewControllerPlaybackBufferObservationContext];
    
    _player = [AVPlayer playerWithPlayerItem:_item];
    
    [_item retain];
    [_player retain];
    
    if(_videoView)
    {
        _layer = [AVPlayerLayer playerLayerWithPlayer:_player];
        _layer.frame = _videoView.bounds;
        [_videoView.layer addSublayer:_layer];
        
        [CATransaction begin];
        [CATransaction setAnimationDuration:0];
        [CATransaction setDisableActions:YES];
        _layer.videoGravity = AVLayerVideoGravityResizeAspect;
        // Fit window : AVLayerVideoGravityResize;
        [CATransaction commit];
    }

    [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];

	//[_player play];

    /*
    _asset = [AVAsset assetWithURL:[NSURL URLWithString:url]];
    
    [_asset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            if (_asset.playable)
            {
                NSArray *array = _asset.tracks;
                for (AVAssetTrack *track in array) {
                    if ([track.mediaType isEqualToString:AVMediaTypeVideo]) {
                        //videoSizeBlock(track.naturalSize);
                    }
                }
                [self loadedResourceForPlay];
            }
        });
    }];
     */
    
    return QC_ERR_NONE;
}

- (int)run
{
    if(_player)
    {
        [_player play];
    }

    return QC_ERR_NONE;
}

- (int)pause
{
    if(_player)
    {
        [_player pause];
    }

    return QC_ERR_NONE;
}

- (int)stop
{
    if(_item)
    {
        [_item removeObserver:self forKeyPath:kStatusKey];
        [_item removeObserver:self forKeyPath:kTimeRangesKVO];
        [_item removeObserver:self forKeyPath:kPlaybackBufferEmpty];
        [_item removeObserver:self forKeyPath:kPlaybackLikelyToKeepUp];
    }
    
    if(_player)
    {
        [_player setRate:0.0];
        //[_player setActionAtItemEnd:<#(AVPlayerActionAtItemEnd)#>];
        //[_player seekToTime:CMTimeMake(0, 1)];
        //[_player pause];
    }
    if(_player)
    {
        [_player release];
        _player         = nil;
    }
    if(_item)
    {
        [_item release];
        _item         = nil;
    }
    if(_layer)
    {
        //[_layer removeFromSuperlayer];
        //[_layer setPlayer: nil];
    }
    return QC_ERR_NONE;
}

- (long long)seek:(NSNumber*)pos
{
    return QC_ERR_NONE;
}

- (int)setVolume:(NSInteger)volume
{
    return QC_ERR_NONE;
}

- (int)setSpeed:(double)speed
{
    if(_item)
    	[self enableAudioTracks:YES inPlayerItem:_item];
    if(_player)
    	[_player setRate:speed];
    return 0;
}

- (void)enableAudioTracks:(BOOL)enable inPlayerItem:(AVPlayerItem*)playerItem
{
    for (AVPlayerItemTrack *track in playerItem.tracks)
    {
        if ([track.assetTrack.mediaType isEqual:AVMediaTypeAudio])
        {
            track.enabled = enable;
        }
    }
}

-(bool)isLive
{
    if(_item)
    {
        CMTime duration = [_item duration];
        if(duration.flags == kCMTimeFlags_Indefinite)
            return YES;
        
        return NO;
    }
    return YES;
}


-(NSString*)getName
{
    return @"native player";
}

- (void)observeValueForKeyPath:(NSString*) path
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context
{
    /* AVPlayerItem "status" property value observer. */
    if (context == AVPlayerPlaybackViewControllerStatusObservationContext)
    {
        AVPlayerStatus status = (AVPlayerStatus)[[change objectForKey:NSKeyValueChangeNewKey] integerValue];
        switch (status)
        {
                /* Indicates that the status of the player is not yet known because
                 it has not tried to load new media resources for playback */
            case AVPlayerStatusUnknown:
            {
            }
                break;
                
            case AVPlayerStatusReadyToPlay:
            {
//                NSArray* tracks = _asset.tracks;
//                for (AVAssetTrack* track in tracks)
//                {
//                    if ([track.mediaType isEqualToString:AVMediaTypeVideo])
//                    {
//                        QC_STREAM_FORMAT fmt;
//                        memset(&fmt, 0, sizeof(fmt));
//                        fmt.nWidth = track.naturalSize.width;
//                        fmt.nHeight = track.naturalSize.height;
//                        [self sendEvent:QC_MSG_SNKV_NEW_FORMAT withValue:&fmt];
//                        break;
//                    }
//                }

                [_player play];
                [self sendEvent:QC_MSG_SNKV_FIRST_FRAME withValue:NULL];
            }
                break;
                
            case AVPlayerStatusFailed:
            {
                NSLog(@"AVPLayer status failed!!!");
            }
                break;
        }
    }
    else if (context == AVPlayerPlaybackViewControllerPlaybackBufferObservationContext)
    {
        if (object == _item && [path isEqualToString:kPlaybackBufferEmpty])
        {
            if (_item.playbackBufferEmpty)
            {
                
                int nCurrPos = 0;
                if (CMTimeGetSeconds([_player currentTime]))
                {
                    CMTime cTime = [_player currentTime];
                    //nCurrPos = int((cTime.value*1000) / cTime.timescale);
                }
                
                //[self sendEvent:<#(int)#> withValue:<#(void *)#>];
            }
        }
        else if (object == _item && [path isEqualToString:kPlaybackLikelyToKeepUp])
        {
            if (_item.playbackLikelyToKeepUp)
            {
                
            }
        }
        else if (object == _item && [path isEqualToString:kTimeRangesKVO])
        {
            NSArray *loadedTimeRanges = [_item loadedTimeRanges];
            CMTimeRange timeRange = [loadedTimeRanges.firstObject CMTimeRangeValue];// 获取缓冲区域
            
            float startSeconds = CMTimeGetSeconds(timeRange.start);
            float durationSeconds = CMTimeGetSeconds(timeRange.duration);
            NSTimeInterval timeInterval = startSeconds + durationSeconds;// 计算缓冲总进度
            CMTime duration = _item.duration;
            CGFloat totalDuration = CMTimeGetSeconds(duration);
            
            int bufTime = durationSeconds * 1000;
            [self sendEvent:QC_MSG_BUFF_VBUFFTIME withValue:&bufTime];
            [self sendEvent:QC_MSG_BUFF_ABUFFTIME withValue:&bufTime];
            
            //NSLog(@"下载进度：%.2f", timeInterval / totalDuration);
        }
    }
    /* AVPlayer "rate" property value observer. */
    else if (context == AVPlayerPlaybackViewControllerRateObservationContext)
    {
    }
    /* AVPlayer "currentItem" property observer.
     Called when the AVPlayer replaceCurrentItemWithPlayerItem:
     replacement will/did occur. */
    else if (context == AVPlayerPlaybackViewControllerCurrentItemObservationContext)
    {
        return;
    }
    else
    {
        [super observeValueForKeyPath:path ofObject:object change:change context:context];
    }
}



-(void)dealloc
{
    [super dealloc];
    
    if(_player)
    {
        [_player release];
        _player = nil;
    }
}
@end
