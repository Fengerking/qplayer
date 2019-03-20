//
//  aPlayer.m
//  TestPlayer
//
//  Created by Jun Lin on 26/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "aPlayer.h"
#import <AliyunPlayerSDK/AliyunPlayerSDK.h>

#define PROP_DOUBLE_VIDEO_DECODE_FRAMES_PER_SECOND  10001
#define PROP_DOUBLE_VIDEO_OUTPUT_FRAMES_PER_SECOND  10002

#define FFP_PROP_DOUBLE_OPEN_FORMAT_TIME                 18001
#define FFP_PROP_DOUBLE_FIND_STREAM_TIME                 18002
#define FFP_PROP_DOUBLE_OPEN_STREAM_TIME                 18003
#define FFP_PROP_DOUBLE_1st_VFRAME_SHOW_TIME             18004
#define FFP_PROP_DOUBLE_1st_AFRAME_SHOW_TIME             18005
#define FFP_PROP_DOUBLE_1st_VPKT_GET_TIME                18006
#define FFP_PROP_DOUBLE_1st_APKT_GET_TIME                18007
#define FFP_PROP_DOUBLE_1st_VDECODE_TIME                 18008
#define FFP_PROP_DOUBLE_1st_ADECODE_TIME                 18009
#define FFP_PROP_DOUBLE_DECODE_TYPE                 	 18010

#define FFP_PROP_DOUBLE_LIVE_DISCARD_DURATION            18011
#define FFP_PROP_DOUBLE_LIVE_DISCARD_CNT                 18012
#define FFP_PROP_DOUBLE_DISCARD_VFRAME_CNT               18013

#define FFP_PROP_DOUBLE_RTMP_OPEN_DURATION               18040
#define FFP_PROP_DOUBLE_RTMP_OPEN_RTYCNT                 18041
#define FFP_PROP_DOUBLE_RTMP_NEGOTIATION_DURATION        18042
#define FFP_PROP_DOUBLE_HTTP_OPEN_DURATION               18060
#define FFP_PROP_DOUBLE_HTTP_OPEN_RTYCNT                 18061
#define FFP_PROP_DOUBLE_HTTP_REDIRECT_CNT                18062
#define FFP_PROP_DOUBLE_TCP_CONNECT_TIME                 18080
#define FFP_PROP_DOUBLE_TCP_DNS_TIME                     18081

//decode type
#define     FFP_PROPV_DECODER_UNKNOWN                   0
#define     FFP_PROPV_DECODER_AVCODEC                   1
#define     FFP_PROPV_DECODER_MEDIACODEC                2
#define     FFP_PROPV_DECODER_VIDEOTOOLBOX              3

#define FFP_PROP_INT64_VIDEO_CACHED_DURATION            20005
#define FFP_PROP_INT64_AUDIO_CACHED_DURATION            20006
#define FFP_PROP_INT64_VIDEO_CACHED_BYTES               20007
#define FFP_PROP_INT64_AUDIO_CACHED_BYTES               20008
#define FFP_PROP_INT64_VIDEO_CACHED_PACKETS             20009
#define FFP_PROP_INT64_AUDIO_CACHED_PACKETS             20010


@interface aPlayer()
{
    AliVcMediaPlayer*	_player;
    NSTimer*			_timer;
    BOOL				_playing;
}
@end


@implementation aPlayer

-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _player = nil;
    }
    
    return self;
}

-(void)dealloc
{
    if(_player)
    {
        [self removePlayerObserver];
        [_player destroy];
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
    _playing = NO;
    if(!_player)
    {
        _player = [[AliVcMediaPlayer alloc] init];
        [_player create:_videoView];
        _player.mediaType = MediaType_AUTO;
        _player.timeout = 25000;
        _player.dropBufferDuration = 8000;
        [self addPlayerObserver];
    }
    
    if(_player)
    {
        //timer to update player progress
        //prepare and play the video
        AliVcMovieErrorCode err = [_player prepareToPlay:[NSURL URLWithString:url]];
        if(err != ALIVC_SUCCESS)
        {
            NSLog(@"preprare failed,error code is %d",(int)err);
            return QC_ERR_FAILED;
        }
    }
    
    //[self run];
    
    return 0;
}


- (int)setVolume:(NSInteger)volume
{
    return 0;
}

- (int)setView:(UIView *)view
{
    if(_player)
    {
        if(_videoView.contentMode == UIViewContentModeScaleToFill)
            _player.scalingMode = scalingModeAspectFitWithCropping;
        else
            _player.scalingMode = scalingModeAspectFit;
    }
    
    return [super setView:view];
}

- (int)run
{
    AliVcMovieErrorCode err = [_player play];
    if(err != ALIVC_SUCCESS)
    {
        NSLog(@"play failed,error code is %d",(int)err);
        return QC_ERR_FAILED;
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
    [_timer invalidate];
    _timer = nil;

    if(_player)
    {
        [_player stop];
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
    return [NSString stringWithFormat:@"%@ %@", @"aPlayer", _player?[_player getSDKVersion]:@""];
}

//recieve prepared notification
- (void)OnVideoPrepared:(NSNotification *)notification
{
    [self sendEvent:QC_MSG_PLAY_OPEN_DONE withValue:NULL];
    QC_STREAM_FORMAT fmt;
    fmt.nWidth = _player.videoWidth;
    fmt.nHeight = _player.videoHeight;
    [self sendEvent:QC_MSG_SNKV_NEW_FORMAT withValue:&fmt];
    [self testInfo];
    _timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
}

//recieve error notification
- (void)OnVideoError:(NSNotification *)notification
{
//    NSString* error_msg = @"未知错误";
//    AliVcMovieErrorCode error_code = _player.errorCode;
//    
//    switch (error_code) {
//        case ALIVC_ERR_FUNCTION_DENIED:
//            error_msg = @"未授权";
//            break;
//        case ALIVC_ERR_ILLEGALSTATUS:
//            error_msg = @"非法的播放流程";
//            break;
//        case ALIVC_ERR_INVALID_INPUTFILE:
//            error_msg = @"无法打开";
//            break;
//        case ALIVC_ERR_NO_INPUTFILE:
//            error_msg = @"无输入文件";
//            break;
//        case ALIVC_ERR_NO_NETWORK:
//            error_msg = @"网络连接失败";
//            break;
//        case ALIVC_ERR_NO_SUPPORT_CODEC:
//            error_msg = @"不支持的视频编码格式";
//            break;
//        case ALIVC_ERR_NO_VIEW:
//            error_msg = @"无显示窗口";
//            break;
//        case ALIVC_ERR_NO_MEMORY:
//            error_msg = @"内存不足";
//            break;
//        case ALIVC_ERR_DOWNLOAD_TIMEOUT:
//            error_msg = @"网络超时";
//            break;
//        case ALIVC_ERR_UNKOWN:
//            error_msg = @"未知错误";
//            break;
//        default:
//            break;
//    }
//    
//    //NSLog(error_msg);
//    
//    if(error_code == ALIVC_ERR_DOWNLOAD_TIMEOUT) {
//        
//        [_player pause];
//        
//        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"错误提示"
//                                                        message:error_msg
//                                                       delegate:self
//                                              cancelButtonTitle:@"等待"
//                                              otherButtonTitles:@"重新连接",nil];
//        
//        [alert show];
//    }
//    //the error message is important when error_cdoe > 500
//    else if(error_code > 500 || error_code == ALIVC_ERR_FUNCTION_DENIED)
//    {
//        
//        [_player reset];
//        
////        UIAlertView *alter = [[UIAlertView alloc] initWithTitle:@"" message:error_msg delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
////        
////        [alter show];
//        return;
//    }
}

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    //[self showLoadingIndicators];
    
    if (buttonIndex == 0) {
        [_player play];
    }
    //reconnect
    else if(buttonIndex == 1) {
        [_player stop];
        //[self replay];
    }
}

//recieve finish notification
- (void)OnVideoFinish:(NSNotification *)notification
{
    [_player stop];
}

//recieve seek finish notification
- (void)OnSeekDone:(NSNotification *)notification
{
    [self sendEvent:QC_MSG_PLAY_SEEK_DONE withValue:NULL];
}

//recieve start cache notification
- (void)OnStartCache:(NSNotification *)notification
{
    [self sendEvent:QC_MSG_BUFF_START_BUFFERING withValue:NULL];
}

//recieve end cache notification
- (void)OnEndCache:(NSNotification *)notification
{
    [self sendEvent:QC_MSG_BUFF_END_BUFFERING withValue:NULL];
}


-(void)addPlayerObserver
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnVideoPrepared:)
                                                 name:AliVcMediaPlayerLoadDidPreparedNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnVideoError:)
                                                 name:AliVcMediaPlayerPlaybackErrorNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnVideoFinish:)
                                                 name:AliVcMediaPlayerPlaybackDidFinishNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnSeekDone:)
                                                 name:AliVcMediaPlayerSeekingDidFinishNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnStartCache:)
                                                 name:AliVcMediaPlayerStartCachingNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(OnEndCache:)
                                                 name:AliVcMediaPlayerEndCachingNotification object:_player];
}

-(void)removePlayerObserver
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerLoadDidPreparedNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerPlaybackErrorNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerPlaybackDidFinishNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerSeekingDidFinishNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerStartCachingNotification object:_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AliVcMediaPlayerEndCachingNotification object:_player];
}

- (IBAction)onTimer:(id)sender
{
    [self testInfo];
}


-(void) testInfo
{
    if (_player == nil) {
        return;
    }
    
    double video_decode_fps = [_player getPropertyDouble:PROP_DOUBLE_VIDEO_DECODE_FRAMES_PER_SECOND defaultValue:0];
    double video_render_fps = [_player getPropertyDouble:PROP_DOUBLE_VIDEO_OUTPUT_FRAMES_PER_SECOND defaultValue:0];
    //printf("video_decode_fps is %lf, video_render_fps is %lf\n",video_decode_fps,video_render_fps);
    
    double open_format_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_OPEN_FORMAT_TIME defaultValue:0];
    double find_stream_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_FIND_STREAM_TIME defaultValue:0];
    double open_stream_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_OPEN_STREAM_TIME defaultValue:0];
    //printf("open_format_time is %lf, find_stream_time is %lf, open_stream_time is %lf\n",open_format_time,find_stream_time,open_stream_time);
    
    
    
    double video_first_decode_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_VDECODE_TIME defaultValue:0];
    double video_first_get_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_VPKT_GET_TIME defaultValue:0];
    double video_first_show_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_VFRAME_SHOW_TIME defaultValue:0];
    //printf("video_first_decode_time is %lf, video_first_get_time is %lf, video_first_show_time is %lf\n",video_first_decode_time,video_first_get_time,video_first_show_time);

    double audio_first_decode_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_ADECODE_TIME defaultValue:0];
    double audio_first_get_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_APKT_GET_TIME defaultValue:0];
    double audio_first_show_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_1st_AFRAME_SHOW_TIME defaultValue:0];
    double video_decode_type = [_player getPropertyDouble:FFP_PROP_DOUBLE_DECODE_TYPE defaultValue:0];
    //printf("audio_first_decode_time is %lf, audio_first_get_time is %lf, audio_first_show_time is %lf,video_decode_type is %lf\n",audio_first_decode_time,audio_first_get_time,audio_first_show_time,video_decode_type);
    
    double rtmp_open_duration = [_player getPropertyDouble:FFP_PROP_DOUBLE_RTMP_OPEN_DURATION defaultValue:0];
    double rtmp_open_retry_count = [_player getPropertyDouble:FFP_PROP_DOUBLE_RTMP_OPEN_RTYCNT defaultValue:0];
    double rtmp_negotiation_duration = [_player getPropertyDouble:FFP_PROP_DOUBLE_RTMP_NEGOTIATION_DURATION defaultValue:0];
    //printf("rtmp_open_duration is %lf, rtmp_open_retry_count is %lf, rtmp_negotiation_duration is %lf\n",rtmp_open_duration,rtmp_open_retry_count,rtmp_negotiation_duration);
    
    double http_open_duration = [_player getPropertyDouble:FFP_PROP_DOUBLE_HTTP_OPEN_DURATION defaultValue:0];
    double http_open_retry_count = [_player getPropertyDouble:FFP_PROP_DOUBLE_HTTP_OPEN_RTYCNT defaultValue:0];
    double http_redirect_count = [_player getPropertyDouble:FFP_PROP_DOUBLE_HTTP_REDIRECT_CNT defaultValue:0];
    //printf("http_open_duration is %lf, http_open_retry_count is %lf, http_redirect_count is %lf\n",http_open_duration,http_open_retry_count,http_redirect_count);
    
    double tcp_connect_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_TCP_CONNECT_TIME defaultValue:0];
    double tcp_dns_time = [_player getPropertyDouble:FFP_PROP_DOUBLE_TCP_DNS_TIME defaultValue:0];
    //printf("tcp_connect_time is %lf, tcp_dns_time is %lf\n",tcp_connect_time,tcp_dns_time);

    int64_t video_cached_duration = [_player getPropertyLong:FFP_PROP_INT64_VIDEO_CACHED_DURATION defaultValue:0];
    int64_t video_cached_bytes = [_player getPropertyLong:FFP_PROP_INT64_VIDEO_CACHED_BYTES defaultValue:0];
    int64_t video_cached_packets = [_player getPropertyLong:FFP_PROP_INT64_VIDEO_CACHED_PACKETS defaultValue:0];
    //printf("video_cached_duration is %lld, video_cached_bytes is %lld, video_cached_packets is %lld, pos %f\n",video_cached_duration,video_cached_bytes,video_cached_packets, _player.currentPosition);

   
    int64_t audio_cached_duration = [_player getPropertyLong:FFP_PROP_INT64_AUDIO_CACHED_DURATION defaultValue:0];
    int64_t audio_cached_bytes = [_player getPropertyLong:FFP_PROP_INT64_AUDIO_CACHED_BYTES defaultValue:0];
    int64_t audio_cached_packets = [_player getPropertyLong:FFP_PROP_INT64_AUDIO_CACHED_PACKETS defaultValue:0];
    //printf("audio_cached_duration is %lld, audio_cached_bytes is %lld, audio_cached_packets is %lld\n",audio_cached_duration,audio_cached_bytes,audio_cached_packets);
    
    double drop_frame_count = [_player getPropertyDouble:FFP_PROP_DOUBLE_LIVE_DISCARD_CNT defaultValue:0];
    double drop_v_frame_count = [_player getPropertyDouble:FFP_PROP_DOUBLE_DISCARD_VFRAME_CNT defaultValue:0];
    double drop_frame_duration = [_player getPropertyDouble:FFP_PROP_DOUBLE_LIVE_DISCARD_DURATION defaultValue:0];
    if(drop_frame_count>0 && drop_v_frame_count>0 && drop_frame_duration>0)
    	printf("drop_frame_count is %lf, drop_v_frame_count is %lf, drop_frame_duration is %lf\n",drop_frame_count,drop_v_frame_count,drop_frame_duration);
    
    if(!_playing)
    {
        if(tcp_connect_time > 0 && video_first_show_time>0 && audio_first_show_time>0)
        {
            int connectTime = tcp_connect_time;
            [self sendEvent:QC_MSG_PRIVATE_CONNECT_TIME withValue:&connectTime];
            
            int vFirstTime = video_first_show_time;
            [self sendEvent:QC_MSG_SNKV_FIRST_FRAME withValue:&vFirstTime];
            
            int aFirstTime = audio_first_show_time;
            [self sendEvent:QC_MSG_SNKA_FIRST_FRAME withValue:&aFirstTime];
            
            _playing = YES;
        }
    }
    int vFPS = (int)video_render_fps;
    [self sendEvent:QC_MSG_RENDER_VIDEO_FPS withValue:&vFPS];
    
    int vBuf = (int)(video_cached_duration/1000.0);
    [self sendEvent:QC_MSG_BUFF_VBUFFTIME withValue:&vBuf];

    int aBuf = (int)(audio_cached_duration/1000.0);
    [self sendEvent:QC_MSG_BUFF_ABUFFTIME withValue:&aBuf];
}

- (int)setSpeed:(double)speed
{
    return 0;
}



@end
