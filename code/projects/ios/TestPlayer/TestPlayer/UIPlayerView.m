//
//  UIPlayerView.m
//  TestPlayer
//
//  Created by Jun Lin on 26/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "UIPlayerView.h"
#import "corePlayer.h"

//#define _SHOW_BUFFERING_ICON

@interface UIPlayerView()
{
    basePlayer*		_player;
    PLAYERINFO		_playerInfo;
    NSTimer*		_timer;
    
    UIButton*		_btnScale;
    UIView*			_viewScale;
    
    UILabel*		_labelInfo;
    UIButton*		_btnClose;
    UIButton*		_btnFullScreen;
    UIButton*		_btnCache;
    BOOL			_showInfo;
    int				_openFlag;
    
    UIView*			_videoView;
    UIImageView*	_coverView;
    CGRect			_rectOfVideoView;
    BOOL			_portraitMode;
    
    CGSize			_videoSize;
    BOOL			_isFullScreen;
    UIView*			_supperView;
    
    UILabel*		_labelTips;
    
    UIActivityIndicatorView* _loadingView;

    UIPlayerView*	_floatVideoView;
    
    int				_animationStepWidth;
    NSTimer* 		_timerAnimation;
}
@end

@implementation UIPlayerView


-(id)initWithFrame:(CGRect)frame
{
    return [self initWithFrame:frame player:nil];
}


- (BOOL) canBecomeFirstResponder
{
    return  YES;
}

//针对于copy的实现
-(void)copy:(id)sender
{
    if([_player getURL])
    {
        UIPasteboard *pboard = [UIPasteboard generalPasteboard];
        pboard.string = [_player getURL];
    }
}

-(void)paste:(id)sender
{
    UIPasteboard* pboard = [UIPasteboard generalPasteboard];
    if(pboard.string)
    {
//        if(!_player)
//        {
//            _player = [basePlayer createPlayer];
//            [_player setView:_videoView];
//        }
        [self setPlayer:_player];
        
        if((_openFlag&QCPLAY_OPEN_SAME_SOURCE) != QCPLAY_OPEN_SAME_SOURCE)
        	[_player stop];
        [self open:pboard.string];
    }
}


-(BOOL)canPerformAction:(SEL)action withSender:(id)sender{
    
    //这里也是间接影响显示在UIMenuController的控件
    if (action == @selector(copy:))
    {
        return YES;
    }
    else if (action == @selector(paste:))
    {
        return YES;
    }
    else
    {
        return NO;
        return [super canPerformAction:action withSender:sender];
    }
}

-(void)attachTapHandler
{
    self.userInteractionEnabled = YES;

    UILongPressGestureRecognizer* touch = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
    touch.minimumPressDuration = 0.5;
    //touch.allowableMovement = 15;
    touch.numberOfTouchesRequired = 1;
    [self addGestureRecognizer:touch];
    [touch release];
    
    UITapGestureRecognizer* singleTouch = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
    singleTouch.numberOfTapsRequired = 1;
    [self addGestureRecognizer:singleTouch];
    [singleTouch release];
    
    UITapGestureRecognizer* doubleTouch = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTap:)];
    doubleTouch.numberOfTapsRequired = 2;
    [self addGestureRecognizer:doubleTouch];
    [doubleTouch release];
}

- (IBAction)onTimerAnimation:(id)sender
{
    if(_floatVideoView)
    {
        if(_floatVideoView.frame.size.width >= self.frame.size.width)
            return;
        
        int width = _floatVideoView.frame.size.width + _animationStepWidth;
        if(width >= self.frame.size.width)
        {
            width = self.frame.size.width;
        }
        
        int height = width;
        _floatVideoView.frame = CGRectMake((self.frame.size.width-width)/2, (self.frame.size.height-height)/2, width, height);
        [_floatVideoView onScaleChanged:_btnScale];
    }
}

-(void)handleDoubleTap:(UIGestureRecognizer*) recognizer
{
    [self runFloatPlayer:YES];
}

-(void)handleSingleTap:(UIGestureRecognizer*) recognizer
{
#if 0
    if(_floatVideoView)
    {
        double interval = 0.033;
        int step = 1.0 / interval;
        int offset 	= self.frame.size.width - _floatVideoView.frame.size.width;
        _animationStepWidth 	= offset / step;
        
        _timer = [NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(onTimerAnimation:) userInfo:nil repeats:YES];
    }
#else
    [[UIMenuController sharedMenuController] setMenuVisible:NO animated: YES];
#endif
}

-(void)handleLongPress:(UIGestureRecognizer*) recognizer
{
    if([[UIMenuController sharedMenuController] isMenuVisible])
        return;
    
    [self becomeFirstResponder];
//    UIMenuItem* copyLink = [[[UIMenuItem alloc] initWithTitle:@"Copy URL" action:@selector(copyURL:)]autorelease];
//    UIMenuItem* playLink = [[[UIMenuItem alloc] initWithTitle:@"Play URL" action:@selector(playURL:)]autorelease];
//    [[UIMenuController sharedMenuController] setMenuItems:[NSArray arrayWithObjects:copyLink, playLink, nil]];
    [[UIMenuController sharedMenuController] setTargetRect:self.frame inView:self.superview];
    [[UIMenuController sharedMenuController] setMenuVisible:YES animated: YES];
    
}

- (void)runFloatPlayer:(BOOL)run
{
    if(!_floatVideoView)
    {
        int width = self.frame.size.width/2;
        int height = width;
        _floatVideoView = [[UIPlayerView alloc]initWithFrame:CGRectMake((self.frame.size.width-width)/2, (self.frame.size.height-height)/2, width, height)];
        _floatVideoView.backgroundColor = [UIColor brownColor];
        [_floatVideoView setHidden:YES];
        [_floatVideoView showInfo:NO];
        [_floatVideoView showClose:YES];
        [_floatVideoView setPlayer:nil];
        [self addSubview:_floatVideoView];
        [self bringSubviewToFront:_floatVideoView];
        [_floatVideoView release];
    }

    if(run)
    {
        [_floatVideoView setHidden:NO];
        [_floatVideoView open:[UIPasteboard generalPasteboard].string];
    }
    else
    {
        [_floatVideoView stop];
        [_floatVideoView removeFromSuperview];
        _floatVideoView = nil;
        //[_floatVideoView setHidden:YES];
    }
}

-(id)initWithFrame:(CGRect)frame player:(basePlayer*)player
{
    self = [super initWithFrame:frame];
    
    if(self != nil)
    {
        [self attachTapHandler];
        //self.userInteractionEnabled = YES;
        [self registerSysEvent];
        
        _openFlag = QCPLAY_OPEN_SAME_SOURCE;
        //_openFlag |= QCPLAY_OPEN_VIDDEC_HW;
        _videoSize.width = 0;
        _videoSize.height = 0;
        _portraitMode = YES;
        _isFullScreen = NO;
        _supperView = self.superview;
//        _rectOfVideoView.size.width = self.frame.size.width;
//        _rectOfVideoView.size.height = self.frame.size.height;
//        _rectOfVideoView.origin.x = 0;
//        _rectOfVideoView.origin.y = 0;
        _rectOfVideoView = self.bounds;
        
        _videoView = [[UIView alloc] initWithFrame:_rectOfVideoView];
#if 0
//        self.layer.borderWidth = 1;
//        self.layer.borderColor = [[UIColor redColor] CGColor];
        _videoView.layer.borderWidth = 1;
        _videoView.layer.borderColor = [[UIColor orangeColor] CGColor];
#endif
        [_videoView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight)];
        [self insertSubview:_videoView atIndex:0];
        _videoView.contentMode = UIViewContentModeScaleAspectFit;
        _videoView.backgroundColor = [UIColor blackColor];
        
        [_videoView release];
        
        _showInfo = true;
        _labelInfo = [[UILabel alloc] initWithFrame:CGRectMake(10, 20, frame.size.width-10, 100)];
        _labelInfo.text = @"URL";
        _labelInfo.font = [UIFont systemFontOfSize:10];
        _labelInfo.textColor = [UIColor redColor];
        _labelInfo.numberOfLines = 0;
        _labelInfo.backgroundColor = [UIColor clearColor];
        [self addSubview:_labelInfo];
        [_labelInfo release];
        
        //
        _labelTips = [[UILabel alloc] initWithFrame:CGRectMake(0, self.frame.size.height-100, frame.size.width, 30)];
        _labelTips.text = @"上下滑动来切换视频源 / 长按屏幕paste来播放剪贴板URL / 双击屏幕启动第二个实例来播放剪贴板URL";
        _labelTips.textAlignment = NSTextAlignmentCenter;
        _labelTips.font = [UIFont systemFontOfSize:10];
        _labelTips.textColor = [UIColor redColor];
        _labelTips.numberOfLines = 0;
        _labelTips.backgroundColor = [UIColor clearColor];
        [self addSubview:_labelTips];
        [_labelTips release];


        //
        _btnScale = [[UIButton alloc] initWithFrame:CGRectMake(self.frame.size.width-40, self.frame.size.height-self.frame.size.height/3, 40, 20)];
        [_btnScale setTitle:@"Scale" forState:UIControlStateNormal];
        [_btnScale setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        //_btnScale.backgroundColor = [UIColor whiteColor];
        _btnScale.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        //_btnScale.userInteractionEnabled = YES;
        [_btnScale addTarget:self action:@selector(onScale) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_btnScale];
        [_btnScale release];

        //
        _btnCache = [[UIButton alloc] initWithFrame:CGRectMake(self.frame.size.width-80, _btnScale.frame.origin.y+30, 80, 20)];
        [_btnCache setTitle:@"Enable cache" forState:UIControlStateNormal];
        [_btnCache setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        _btnCache.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        _btnCache.tag = 0;
        [_btnCache addTarget:self action:@selector(onCache) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_btnCache];
        [_btnCache release];

        //
        _btnClose = [[UIButton alloc] initWithFrame:CGRectMake(self.frame.size.width-40, 20, 40, 20)];
        [_btnClose setTitle:@"Close" forState:UIControlStateNormal];
        [_btnClose setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        _btnClose.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        //_btnClose.userInteractionEnabled = YES;
        [_btnClose addTarget:self action:@selector(onHide) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_btnClose];
        [_btnClose release];
        [_btnClose setHidden:YES];
        
        _btnFullScreen = [[UIButton alloc] initWithFrame:CGRectMake(self.frame.size.width-60, 0, 60, 20)];
        [_btnFullScreen setTitle:@"FScreen" forState:UIControlStateNormal];
        [_btnFullScreen setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        _btnFullScreen.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        //_btnFullScreen.userInteractionEnabled = YES;
        [_btnFullScreen addTarget:self action:@selector(fullScreen) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_btnFullScreen];
        [_btnFullScreen setHidden:YES];
        [_btnFullScreen release];
    
        
        //
//        UIImage* sourceImage = [UIImage imageNamed:@"cover.png"];
//        CIImage * ciImage    = [[CIImage alloc] initWithImage:sourceImage];
//        CIFilter * blurFilter = [CIFilter filterWithName:@"CIGaussianBlur"];
//        [blurFilter setValue:ciImage forKey:kCIInputImageKey];
//        [blurFilter setValue:@(3) forKey:kCIInputRadiusKey];
//        //NSLog(@"查看blurFilter的属性--- %@",blurFilter.attributes);
//        CIImage * outCiImage    = [blurFilter valueForKey:kCIOutputImageKey];
//        CIContext * context      = [CIContext contextWithOptions:nil];
//        CGImageRef outCGImageRef = [context createCGImage:outCiImage fromRect:[outCiImage extent]];
//        UIImage * resultImage    = [UIImage imageWithCGImage:outCGImageRef];
//        CGImageRelease(outCGImageRef);
        
//        UIVisualEffect *blurEffect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleLight];
//        UIVisualEffectView *visualEffectView = [[UIVisualEffectView alloc] initWithEffect:blurEffect];
//        visualEffectView.frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
//        visualEffectView.alpha = 0.8;
        

//        _coverView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)];
//        [_coverView setImage:[UIImage imageNamed:@"cover.png"]];
//        //[_coverView addSubview:visualEffectView];
//        [self addSubview:_coverView];
//        //[_coverView setHidden:YES];
//        [_coverView release];
        
        _loadingView = [[UIActivityIndicatorView alloc]initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        _loadingView.center = self.center;
        [self addSubview:_loadingView];
        [_loadingView release];
    }
    
    return self;
}

void onEvent (void * pUserData, int nID, void * pValue1)
{
    UIPlayerView* vc = (UIPlayerView*)pUserData;
    [vc onPlayerEvent:nID withParam:pValue1];
}

int onAudioData (void* pUserData, QC_DATA_BUFF * pBuffer)
{
    //NSLog(@"Audio raw data comes, %p, %d", pBuffer->pBuff, pBuffer->uSize);
    
    //memset(pBuffer->pBuff, 0, pBuffer->uSize);
    // 11 means process it outside
    //pBuffer->nValue = 11;
    
    return QC_ERR_NONE;
}

int onVideoData (void* pUserData, QC_DATA_BUFF * pBuffer)
{
    //NSLog(@"Video raw data comes, %p, %d", pBuffer->pBuff, pBuffer->uSize);
    
    // 11 means process it outside
    //pBuffer->nValue = 11;
#if 0
    QC_VIDEO_BUFF* pVideo = (QC_VIDEO_BUFF*)pBuffer->pBuffPtr;
    if(pVideo)
    {
        if(pVideo->nType == QC_VDT_YUV420_P)
        {
            unsigned char* Y = pVideo->pBuff[0];
            unsigned char* U = pVideo->pBuff[1];
            unsigned char* V = pVideo->pBuff[2];
            NSLog(@"Video raw data comes, Y %p(%d), U %p(%d), V %p(%d)",
                  Y, pVideo->nStride[0], U, pVideo->nStride[1], V, pVideo->nStride[2]);
            
            CVPixelBufferRef pNewPixelBuffer = NULL;
            CVReturn nRC = CVPixelBufferCreate(NULL, pVideo->nWidth, pVideo->nHeight,
                                               kCVPixelFormatType_420YpCbCr8Planar, NULL, &pNewPixelBuffer);
            
            if(kCVReturnSuccess != nRC || NULL == pNewPixelBuffer)
            {
                NSLog(@"Fail to allocate pixel buffer, %d, %p", nRC, pNewPixelBuffer);
                return QC_ERR_FAILED;
            }
            
            int widthUV = pVideo->nWidth/2;
            CVPixelBufferLockBaseAddress(pNewPixelBuffer, 0);
            // copy Y
            for(int i=0; i<pVideo->nHeight; i++)
                memcpy(Y + i*pVideo->nWidth, pVideo->pBuff[0]+i*pVideo->nStride[0], pVideo->nStride[0]);
            
            // copy U
            for (int i = 0; i < pVideo->nHeight/2; i++)
                memcpy (U + widthUV * i, pVideo->pBuff[1] + pVideo->nStride[1] * i, pVideo->nStride[1]);
            
            // copy V
            for (int i = 0; i < pVideo->nHeight/2; i++)
                memcpy (V + widthUV * i, pVideo->pBuff[2] + pVideo->nStride[2] * i, pVideo->nStride[2]);
            
            CVPixelBufferUnlockBaseAddress(pNewPixelBuffer, 0);
            
            // don't forget to release it after use it
            CVPixelBufferRelease(pNewPixelBuffer);
        }
        else if(pVideo->nType == QC_VDT_NV12)
        {
            CVPixelBufferRef pPixel = (__bridge CVPixelBufferRef)pVideo->pBuff[0];
            if(pPixel)
            {
                NSLog(@"Video raw data comes, %p", pPixel);
                
                // if you want to use it, please retain and don't forget release it.
                //CVPixelBufferRetain(pPixel);
            }
        }
    }
#endif
    return QC_ERR_NONE;
}

-(void) onScale
{
    if(!_viewScale)
    {
        int width = 70;
        int height = 75;
        _viewScale = [[UIView alloc] initWithFrame:
                      CGRectMake(_btnScale.frame.origin.x+_btnScale.frame.size.width-width,
                                 _btnScale.frame.origin.y-height, width, height)];
        _viewScale.backgroundColor = [UIColor clearColor];
        [self addSubview:_viewScale];
        [_viewScale release];
        
        UIButton* btn = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, _viewScale.frame.size.width, 25)];
        btn.tag = UIViewContentModeScaleToFill;
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
        [btn setTitle:@"Fill" forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        btn.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        [btn addTarget:self action:@selector(onScaleChanged:) forControlEvents:UIControlEventTouchUpInside];
        [_viewScale addSubview:btn];
        [btn release];

        btn = [[UIButton alloc] initWithFrame:CGRectMake(0, 25, _viewScale.frame.size.width, 25)];
        btn.tag = UIViewContentModeScaleAspectFill;
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
        [btn setTitle:@"AspectFill" forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        btn.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        [btn addTarget:self action:@selector(onScaleChanged:) forControlEvents:UIControlEventTouchUpInside];
        [_viewScale addSubview:btn];
        [btn release];

        btn = [[UIButton alloc] initWithFrame:CGRectMake(0, 50, _viewScale.frame.size.width, 25)];
        btn.tag = UIViewContentModeScaleAspectFit;
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
        [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
        [btn setTitle:@"AspectFit" forState:UIControlStateNormal];
        [btn setTitleColor:[UIColor redColor] forState:UIControlStateNormal];
        btn.titleLabel.font = [UIFont boldSystemFontOfSize:12];
        [btn addTarget:self action:@selector(onScaleChanged:) forControlEvents:UIControlEventTouchUpInside];
        [_viewScale addSubview:btn];
        [btn release];

        [_viewScale setHidden:YES];
    }
    
    [UIView animateWithDuration:0.3 animations:^{
       [_viewScale setHidden:![_viewScale isHidden]];
    }];
}

-(void)onScaleChanged:(UIButton*)sender
{
    if(_player)
    {
        _videoView.contentMode = sender.tag;
        [self setVideoView:_videoView];
        [_viewScale setHidden:YES];
    }
}

- (void)onCache
{
    if(_btnCache.tag == 0)
    {
        _btnCache.tag = 1;
        [_btnCache setTitle:@"Disable cache" forState:UIControlStateNormal];
    }
    else
    {
        _btnCache.tag = 0;
        [_btnCache setTitle:@"Enable cache" forState:UIControlStateNormal];
    }
}

- (void)onHide
{
    [self stop];
    [self setHidden:YES];
}

-(void)showInfo:(BOOL)show
{
    _showInfo = show;
    if(_labelInfo)
    	[_labelInfo setHidden:!show];
}

-(void)showClose:(BOOL)show
{
    if(_btnClose)
    {
        [_btnClose setHidden:!show];
    }
}

-(void)setVideoView:(UIView*)view
{
    if(_player)
    {
        RECT drawRect = {(int)view.bounds.origin.x, (int)view.bounds.origin.y, (int)view.bounds.size.width, (int)view.bounds.size.height};
        
//        int width = drawRect.right - drawRect.left;
//        int height = drawRect.bottom - drawRect.top;
//        
//        drawRect.left += 30;
//        drawRect.top += 50;
//        drawRect.right = drawRect.left + width/2;
//        drawRect.bottom = drawRect.top + height/2;

        [_player setView:view withDrawArea:&drawRect];
    }
    
}

-(void)setPlayer:(basePlayer*)player
{
    // NULL indicator create player by self
    if(nil == player)
    {
        if(_player)
            return;
        _player = [basePlayer createPlayer];
        _openFlag &= ~QCPLAY_OPEN_SAME_SOURCE;
    }
    else
    {
        [player retain];
        if(_player)
            [_player release];
        _player = player;
        _openFlag |= QCPLAY_OPEN_SAME_SOURCE;
    }
    [self setVideoView:_videoView];
}

-(void)enableFileCacheMode
{
    NSString* docPathDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    docPathDir = [docPathDir stringByAppendingString:@"/cache/"];
    
    if(_player && _btnCache.tag==1)
    {
        [_player setParam:QCPLAY_PID_PD_Save_Path value:(void*)[docPathDir UTF8String]];
        int nProtocol = QC_IOPROTOCOL_HTTPPD;
        [_player setParam:QCPLAY_PID_Prefer_Protocol value:&nProtocol];
    }
}


-(void)open:(NSString*)url
{
    memset(&_playerInfo, 0, sizeof(_playerInfo));
    
    if(_coverView && _openFlag&QCPLAY_OPEN_SAME_SOURCE)
        [_coverView setHidden:NO];

    _playerInfo.startup = [basePlayer getSysTime];
    [_player setListener:onEvent withUserData:self];
    [self enableFileCacheMode];
    
    [_player open:url openFlag:_openFlag];
    [self enableTimer:YES];
    
    
//    UIActivityIndicatorView *indicate = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
//    indicate.frame = self.bounds;
//    [self addSubview:indicate];
//    [indicate startAnimating];

}

-(void)run
{
    if(_player)
        [_player run];
}

-(void)pause
{
    if(_player)
        [_player pause];
}

-(void)enableTimer:(BOOL)enable
{
    if(enable)
    {
        if(!_timer)
        	_timer = [NSTimer scheduledTimerWithTimeInterval:1.0/100.0 target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
    }
    else
    {
        if(_timer)
        {
            [_timer invalidate];
            _timer = nil;
        }
    }
}

-(void)stop
{
    [self enableTimer:NO];
    if(_player)
        [_player stop];
    if(_coverView)
        [_coverView setHidden:NO];
}

-(void)setVolume:(NSInteger)volume
{
    if(_player)
        [_player setVolume:volume];
}

-(void)preCache:(NSString*)url
{
    if(_player)
        [_player setParam:QCPLAY_PID_ADD_Cache value:(void*)[url UTF8String]];
}

-(void)rotate
{
    //上下flip
    //_videoView.transform = CGAffineTransformMakeScale(1, -1);
    //左右flip
    //_videoView.transform = CGAffineTransformMakeScale(-1, 1);
    //恢复正常
    //_videoView.transform = CGAffineTransformMakeScale(1, 1);
    
    //顺时针旋转90度
    //_videoView.transform = CGAffineTransformMakeRotation(90*M_PI / 180.0);
    //顺时针旋转270度
    //_videoView.transform = CGAffineTransformMakeRotation(270*M_PI / 180.0);
    //恢复正常
    //_videoView.transform = CGAffineTransformMakeRotation(0);
}

- (IBAction)onTimer:(id)sender
{
    if(!_player)
        return;
    
    PLAYERINFO* info = &_playerInfo;
    _labelInfo.text = [NSString stringWithFormat:@"Index: %06ld \tName: %@\nStartup: %06ld \tConnect: %06ld\nSize: %dx%d \tABuffer: %06ld \tVBuffer: %06ld\n AFPS: %02ld(%02ld)   \tVFPS: %02ld(%02ld)   \tABit: %.1f   \tVBit: %.1f\nOpen:%s  \tHW:%d   \tSpeed: %ld kbps   GOP: %ld",
                       self.tag, _player?[_player getName]:@"unknown", info->startup, info->connect, info->video.nWidth, info->video.nHeight,
                       info->abuftime, info->vbuftime, info->arfps, info->afps, info->vrfps, info->vfps,
                       info->abitrate/1024.0, info->vbitrate/1024.0,
                    	(_openFlag&QCPLAY_OPEN_SAME_SOURCE)==QCPLAY_OPEN_SAME_SOURCE?"same protocol":"normal",
                       (_openFlag&QCPLAY_OPEN_VIDDEC_HW)==QCPLAY_OPEN_VIDDEC_HW?1:0,
                       _playerInfo.dspeed*8/1024, _playerInfo.gop];
}

-(void)startBufferingIcon:(BOOL)show
{
#ifdef _SHOW_BUFFERING_ICON
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        if(show)
            [_loadingView startAnimating];
        else
        	[_loadingView stopAnimating];
    }];
#endif
}

- (void)onPlayerEvent:(int)nID withParam:(void*)pParam
{
    if (nID == QC_MSG_PLAY_OPEN_START)
    {
        _playerInfo.startup = [basePlayer getSysTime];
        
        [self startBufferingIcon:YES];
    }
    else if(nID == QC_MSG_BUFF_START_BUFFERING)
    {
        [self startBufferingIcon:YES];
    }
    else if(nID == QC_MSG_BUFF_END_BUFFERING)
    {
        [self startBufferingIcon:NO];
    }
    else if(nID == QC_MSG_SNKV_FIRST_FRAME)
    {
        if(pParam && CORE_PLAYER!=[_player getPlayerType])
            _playerInfo.startup = *(int*)pParam;
        else
        	_playerInfo.startup = [basePlayer getSysTime] - _playerInfo.startup;
        NSLog(@"[EVT]First frame %p, %ld", self, _playerInfo.startup);
        
        if(_coverView && ![_coverView isHidden])
        {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [_coverView setHidden:YES];
            }];
        }
        
        [self startBufferingIcon:NO];
    }
    else if(nID == QC_MSG_SNKA_FIRST_FRAME)
    {
        if(_coverView && ![_coverView isHidden])
        {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [_coverView setHidden:YES];
            }];
        }
    }
    else if (nID == QC_MSG_PLAY_OPEN_DONE)
    {
        if(_player)
        {
            //[_player setSpeed:1.5];
            [_player setParam:QCPLAY_PID_SendOut_AudioBuff value:(void*)onAudioData];
            [_player setParam:QCPLAY_PID_SendOut_VideoBuff value:(void*)onVideoData];
        }

        [self run];
    }
    else if(nID == QC_MSG_BUFF_GOPTIME)
    {
        _playerInfo.gop = *(int*)pParam;
    }
    else if(nID == QC_MSG_PLAY_OPEN_FAILED)
    {
        _playerInfo.connect = -1;
        _playerInfo.startup = -1;
        NSLog(@"[EVT]Open fail");
        [self startBufferingIcon:NO];
    }
    else if(nID == QC_MSG_BUFF_ABITRATE)
    {
        // in bytes/sec
        if(_playerInfo.abitrate == 0)
            _playerInfo.abitrate = *(int*)pParam;

        _playerInfo.abitrate += *(int*)pParam;
        _playerInfo.abitrate /= 2;
    }
    else if(nID == QC_MSG_BUFF_VBITRATE)
    {
        // in bytes/sec
        if(_playerInfo.vbitrate == 0)
            _playerInfo.vbitrate = *(int*)pParam;

        _playerInfo.vbitrate += *(int*)pParam;
        _playerInfo.vbitrate /= 2;
    }
    else if(nID == QC_MSG_RTMP_DOWNLOAD_SPEED || nID == QC_MSG_HTTP_DOWNLOAD_SPEED)
    {
        // in bytes/sec
        if(_playerInfo.dspeed == 0)
            _playerInfo.dspeed = *(int*)pParam;
            
        _playerInfo.dspeed += *(int*)pParam;
        _playerInfo.dspeed /= 2;
    }
    else if (nID == QC_MSG_PLAY_COMPLETE)
    {
        NSLog(@"[EVT]EOS\n");
        if((_openFlag&QCPLAY_OPEN_SAME_SOURCE) != QCPLAY_OPEN_SAME_SOURCE)
        {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self stop];
            }];
        }
    }
    else if (nID == QC_MSG_PLAY_SEEK_DONE)
    {
        NSLog(@"[EVT]Seek done\n");
    }
    else if (nID == QC_MSG_HTTP_DISCONNECTED || nID == QC_MSG_RTMP_DISCONNECTED)
    {
        NSLog(@"[EVT]Connection loss, %x", nID);
    }
    else if(nID == QC_MSG_HTTP_RECONNECT_FAILED || nID == QC_MSG_RTMP_RECONNECT_FAILED)
    {
        NSLog(@"[EVT]Reconnect fail, %x", nID);
    }
    else if (nID == QC_MSG_HTTP_RECONNECT_SUCESS || nID == QC_MSG_RTMP_RECONNECT_SUCESS)
    {
        NSLog(@"[EVT]Reconnect success, %x", nID);
    }
    else if(nID == QC_MSG_RTMP_METADATA)
    {
        NSLog(@"[EVT]METADATA: %s", (char*)pParam);
    }
    else if(nID == QC_MSG_RTMP_CONNECT_START || nID == QC_MSG_HTTP_CONNECT_START)
    {
        _playerInfo.connect = [basePlayer getSysTime];
        NSLog(@"[EVT]Connect start, %x", nID);
    }
    else if(nID == QC_MSG_RTMP_CONNECT_SUCESS || nID == QC_MSG_HTTP_CONNECT_SUCESS)
    {
        NSLog(@"[EVT]Connect sucss, %x", nID);
        _playerInfo.connect = [basePlayer getSysTime] - _playerInfo.connect;
    }
    else if(nID == QC_MSG_RTMP_CONNECT_FAILED || nID == QC_MSG_HTTP_CONNECT_FAILED)
    {
        NSLog(@"[EVT]Connect fail, %x", nID);
    }
    else if(nID == QC_MSG_RENDER_VIDEO_FPS)
    {
        _playerInfo.vrfps = *(int*)pParam;
        //NSLog(@"VFPS %ld", _playerInfo.vfps);
    }
    else if(nID == QC_MSG_RENDER_AUDIO_FPS)
    {
        _playerInfo.arfps = *(int*)pParam;
        //NSLog(@"AFPS %ld", _playerInfo.afps);
    }
    else if(nID == QC_MSG_BUFF_AFPS)
    {
        _playerInfo.afps = *(int*)pParam;
    }
    else if(nID == QC_MSG_BUFF_VFPS)
    {
        _playerInfo.vfps = *(int*)pParam;
    }
    else if(nID == QC_MSG_BUFF_VBUFFTIME)
    {
        _playerInfo.vbuftime = *(int*)pParam;
        
        if([_player getDuration] > 0)
        {
            long long nowDuration = [_player getPlayingTime] + *(int*)pParam;
            float percent = nowDuration * 100 / [_player getDuration];
            NSLog(@"Current cache duration: %f%%", percent);
        }
    }
    else if(nID == QC_MSG_BUFF_ABUFFTIME)
    {
        _playerInfo.abuftime = *(int*)pParam;
    }
    else if(nID == QC_MSG_SNKV_NEW_FORMAT)
    {
        _playerInfo.video.nWidth = ((QC_VIDEO_FORMAT*)pParam)->nWidth;
        _playerInfo.video.nHeight = ((QC_VIDEO_FORMAT*)pParam)->nHeight;
        NSLog(@"Video size changed, %d x %d", _playerInfo.video.nWidth, _playerInfo.video.nHeight);
        
//        if([self onVideoSizeChanged:_playerInfo.video.nWidth andHeight:_playerInfo.video.nHeight])
//        {
//            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
//                [self adjustUIPosition];
//            }];
//        }
    }
    else if(nID == QC_MSG_PRIVATE_CONNECT_TIME)
    {
        _playerInfo.connect = *(int*)pParam;
    }
    else if(nID == QC_MSG_BUFF_SEI_DATA)
    {
//        QC_DATA_BUFF* sei = (QC_DATA_BUFF*)pParam;
//        NSLog(@"[EVT]SEI data, %d, %p", sei->uSize, sei);
//        //sei->pBuff[17] = 'c';
//        
//        NSData* data = [NSData dataWithBytes:sei->pBuff+4 length:sei->uSize-4-1];
//        NSMutableDictionary* dict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil];
//        if(dict)
//            NSLog(@"%@", dict);

#if 0
        FILE* hFile = NULL;
        if (NULL == hFile)
        {
            char szTmp[256];
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *filePath = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
            strcpy(szTmp, [filePath UTF8String]);
            
            strcat(szTmp, "dump.flv");
            hFile = fopen(szTmp, "wb");
        }
        
        if ((NULL != hFile) && (sei->uSize > 0))
        {
            fwrite(sei->pBuff, 1, sei->uSize, hFile);
            fclose(hFile);
        }
#endif
    }
    else if(nID == QC_MSG_SNKV_ROTATE)
    {
        NSLog(@"ROTATE: %d", *(int*)pParam);
    }

}

-(void)adjustUIPosition
{
    _videoView.frame = _rectOfVideoView;
    
    if(!_portraitMode)
    {
        [_btnFullScreen setHidden:NO];
        [_btnFullScreen setFrame:CGRectMake(_videoView.frame.size.width - _btnFullScreen.frame.size.width,
                                            _videoView.frame.origin.y + _videoView.frame.size.height - _btnFullScreen.frame.size.height,
                                            _btnFullScreen.frame.size.width, _btnFullScreen.frame.size.height)];
    }
    else
    {
        [_btnFullScreen setHidden:YES];
    }
}


#pragma mark - Screen Orientation

- (void)statusBarOrientationChange:(NSNotification *)notification {
    if (_portraitMode) return;
    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    if (orientation == UIDeviceOrientationLandscapeLeft) {
        //        NSLog(@"UIDeviceOrientationLandscapeLeft");
        [self orientationLeftFullScreen];
    }else if (orientation == UIDeviceOrientationLandscapeRight) {
        //        NSLog(@"UIDeviceOrientationLandscapeRight");
        [self orientationRightFullScreen];
    }else if (orientation == UIDeviceOrientationPortrait) {
        //        NSLog(@"UIDeviceOrientationPortrait");
        [self smallScreen];
    }
}

- (void)fullScreen
{
//    self.transform = CGAffineTransformMakeRotation(90 *M_PI / 180.0);
//    return;

    int widthScreen = [UIScreen mainScreen].bounds.size.width;
    int heightScreen = [UIScreen mainScreen].bounds.size.height;
    
    if(!_isFullScreen)
    {
        _isFullScreen = YES;
        [[UIDevice currentDevice]setValue:[NSNumber numberWithInteger:UIDeviceOrientationLandscapeLeft] forKey:@"orientation"];
    }
    else
    {
        _isFullScreen = NO;
        [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationPortrait] forKey:@"orientation"];
    }
    
//    [self onVideoViewSizeChanged:heightScreen andHeight:widthScreen];
//    
//    if(_isFullScreen)
//        [[UIApplication sharedApplication].keyWindow addSubview:self];
//    else
//    {
//        [self removeFromSuperview];
//    }
    
    [UIView animateWithDuration:0.3 animations:^{
        self.frame = CGRectMake(0, 0, heightScreen, widthScreen);
        [self adjustUIPosition];
    } completion:^(BOOL finished) {
    }];
}

- (void)orientationLeftFullScreen
{
//    isFullScreen = YES;
//    [self.keyWindow addSubview:self];
//    
//    [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationLandscapeLeft] forKey:@"orientation"];
//    [self updateConstraintsIfNeeded];
//    
//    [UIView animateWithDuration:0.3 animations:^{
//        self.transform = CGAffineTransformMakeRotation(M_PI / 2);
//        self.frame = self.keyWindow.bounds;
//        self.bottomBar.frame = CGRectMake(0, self.keyWindow.bounds.size.width - bottomBaHeight, self.keyWindow.bounds.size.height, bottomBaHeight);
//        self.playOrPauseBtn.frame = CGRectMake((self.keyWindow.bounds.size.height - playBtnSideLength) / 2, (self.keyWindow.bounds.size.width - playBtnSideLength) / 2, playBtnSideLength, playBtnSideLength);
//        self.activityIndicatorView.center = CGPointMake(self.keyWindow.bounds.size.height / 2, self.keyWindow.bounds.size.width / 2);
//    }];
//    
//    [self setStatusBarHidden:YES];
}

- (void)orientationRightFullScreen
{
//    self.isFullScreen = YES;
//    self.zoomScreenBtn.selected = YES;
//    [self.keyWindow addSubview:self];
//    
//    [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationLandscapeRight] forKey:@"orientation"];
//    
//    [self updateConstraintsIfNeeded];
//    
//    [UIView animateWithDuration:0.3 animations:^{
//        self.transform = CGAffineTransformMakeRotation(-M_PI / 2);
//        self.frame = self.keyWindow.bounds;
//        self.bottomBar.frame = CGRectMake(0, self.keyWindow.bounds.size.width - bottomBaHeight, self.keyWindow.bounds.size.height, bottomBaHeight);
//        self.playOrPauseBtn.frame = CGRectMake((self.keyWindow.bounds.size.height - playBtnSideLength) / 2, (self.keyWindow.bounds.size.width - playBtnSideLength) / 2, playBtnSideLength, playBtnSideLength);
//        self.activityIndicatorView.center = CGPointMake(self.keyWindow.bounds.size.height / 2, self.keyWindow.bounds.size.width / 2);
//    }];
//    [self setStatusBarHidden:YES];
}

- (void)smallScreen
{
//    self.isFullScreen = NO;
//    self.zoomScreenBtn.selected = NO;
//    [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIDeviceOrientationPortrait] forKey:@"orientation"];
//    
//    if (self.bindTableView) {
//        UITableViewCell *cell = [self.bindTableView cellForRowAtIndexPath:self.currentIndexPath];
//        [cell.contentView addSubview:self];
//    }
//    
//    [UIView animateWithDuration:0.3 animations:^{
//        self.transform = CGAffineTransformMakeRotation(0);
//        self.frame = self.playerOriginalFrame;
//        self.bottomBar.frame = CGRectMake(0, self.playerOriginalFrame.size.height - bottomBaHeight, self.self.playerOriginalFrame.size.width, bottomBaHeight);
//        self.playOrPauseBtn.frame = CGRectMake((self.playerOriginalFrame.size.width - playBtnSideLength) / 2, (self.playerOriginalFrame.size.height - playBtnSideLength) / 2, playBtnSideLength, playBtnSideLength);
//        self.activityIndicatorView.center = CGPointMake(self.playerOriginalFrame.size.width / 2, self.playerOriginalFrame.size.height / 2);
//        [self updateConstraintsIfNeeded];
//    }];
//    [self setStatusBarHidden:NO];
}

-(void)onAppActive:(BOOL)active
{
    if(!_player)
        return;
    
    bool isLive = [_player isLive];
    
    if(active)
    {
        if(isLive)
        {
            int nVal = QC_PLAY_VideoEnable;
            [_player setParam:QCPLAY_PID_Disable_Video value:&nVal];
        }
        else
            [self run];
    }
    else
    {
        if(isLive)
        {
            int nVal = ((_openFlag&QCPLAY_OPEN_VIDDEC_HW)==QCPLAY_OPEN_VIDDEC_HW)?
            (QC_PLAY_VideoDisable_Decoder|QC_PLAY_VideoDisable_Render) : QC_PLAY_VideoDisable_Render;
            
            [_player setParam:QCPLAY_PID_Disable_Video value:&nVal];
        }
        else
            [self pause];
    }
}

#pragma mark - app notif

- (void)appDidEnterBackground:(NSNotification*)note {
    
    NSLog(@"appDidEnterBackground");
}

- (void)appWillEnterForeground:(NSNotification*)note {
    NSLog(@"appWillEnterForeground");
}

- (void)appwillResignActive:(NSNotification *)note {
    NSLog(@"appwillResignActive %p", self);
    [self onAppActive:NO];
}

- (void)appBecomeActive:(NSNotification *)note {
    NSLog(@"appBecomeActive, %p", self);
    [self onAppActive:YES];
}



-(void)registerSysEvent
{
    //screen orientation change
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(statusBarOrientationChange:) name:UIDeviceOrientationDidChangeNotification object:[UIDevice currentDevice]];
    
    //show or hiden gestureRecognizer
//    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showOrHidenBar)];
//    [self addGestureRecognizer:tap];
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appwillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
}

-(PLAYERINFO*)getPlayerInfo
{
    return &_playerInfo;
}

//视频分辨率大小发生改变
-(BOOL)onVideoSizeChanged:(int)widthVideo andHeight:(int)heightVideo
{
    if((_videoSize.width == widthVideo) && (heightVideo == _videoSize.height))
        return NO;
    
    _portraitMode = (heightVideo > widthVideo);
    
    if(_portraitMode)
    {
        // Fit to full size of parent view
        _rectOfVideoView = self.bounds;
    }
    else
    {
        _rectOfVideoView.origin.x = 0;
        _rectOfVideoView.size.width = self.bounds.size.width;
        _rectOfVideoView.size.height = self.bounds.size.width*heightVideo / widthVideo;
        _rectOfVideoView.origin.y = (self.bounds.size.height-_rectOfVideoView.size.height)/2;
    }
    
    _videoSize.width = widthVideo;
    _videoSize.height = heightVideo;
    
    NSLog(@"[K]Video size %fx%f, area(%f, %f, %f, %f), portrait %d",
          _videoSize.width, _videoSize.height, _rectOfVideoView.origin.x, _rectOfVideoView.origin.y,
          _rectOfVideoView.size.width, _rectOfVideoView.size.height, (int)_portraitMode);
    
    return !CGRectEqualToRect(_videoView.frame, _rectOfVideoView);
}

//视频窗口大小发生改变
-(BOOL)onVideoViewSizeChanged:(int)widthView andHeight:(int)heightView
{
    //_portraitMode = (heightVideo > widthVideo);
    
    if(_portraitMode)
    {
        // Fit to full size of parent view
        _rectOfVideoView = self.bounds;
    }
    else
    {
        _rectOfVideoView.origin.x = 0;
        _rectOfVideoView.size.width = widthView;
        _rectOfVideoView.size.height = widthView*_playerInfo.video.nHeight / _playerInfo.video.nWidth;
        _rectOfVideoView.origin.y = (heightView-_rectOfVideoView.size.height)/2;
    }
    
    NSLog(@"[K]Video view size change, %fx%f, area(%f, %f, %f, %f), portrait %d",
          _videoSize.width, _videoSize.height, _rectOfVideoView.origin.x, _rectOfVideoView.origin.y,
          _rectOfVideoView.size.width, _rectOfVideoView.size.height, (int)_portraitMode);
    
    return !CGRectEqualToRect(_videoView.frame, _rectOfVideoView);
}

-(void)dealloc
{
    [super dealloc];
    
    [self enableTimer:NO];
    
    if(_floatVideoView)
    {
        
    }
    
    if(_labelInfo)
    {
        [_labelInfo release];
        _labelInfo = nil;
    }
    
    if(_player)
    {
        [_player release];
        _player = nil;
    }
}

//CGPoint  beginPoint;
//CGPoint  movePoint;
//CGPoint  tempPoint;
//NSString  *directionString;
//CGRect   playViewFrame;
//UIPanGestureRecognizer  *panHandler;
//
//panHandler = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panHandler:)];
//panHandler.maximumNumberOfTouches = 1;
//[panHandler delaysTouchesBegan];
//[_viewVideo addGestureRecognizer:panHandler];
//
//
//#define KEY_WINDOW [[UIApplication sharedApplication] keyWindow]
//- (void)panHandler:(UIPanGestureRecognizer *)pan {
//    switch (pan.state) {
//        case UIGestureRecognizerStateBegan:
//            beginPoint = [pan locationInView:_viewVideo];
//            tempPoint = beginPoint;
//            directionString = nil;
//            playViewFrame = _viewVideo.frame;
//            break;
//        case UIGestureRecognizerStateChanged:
//            movePoint = [pan locationInView:[[UIApplication sharedApplication] keyWindow]];
//            if (directionString == nil) {
//                if (fabs(movePoint.y - beginPoint.y) > fabs(movePoint.x - beginPoint.x) && fabs(movePoint.y - beginPoint.y) > 20) {
//                    directionString = @"1";//竖直
//                }else if (fabs(movePoint.x - beginPoint.x) > fabs(movePoint.y - beginPoint.y) && fabs(movePoint.x - beginPoint.x) > 20) {
//                    directionString = @"0";//水平
//                }
//            }else {
//                if ([directionString isEqualToString:@"1"]) {
//                    [self uprightPanHandlerWithPlayViewFrame:playViewFrame withIsEnd:NO];
//                }
//            }
//            break;
//        case UIGestureRecognizerStateEnded:
//        {
//            if ([directionString isEqualToString:@"1"]) {
//                [self uprightPanHandlerWithPlayViewFrame:playViewFrame withIsEnd:YES];
//            }
//        }
//            break;
//        default:
//            break;
//    }
//    [pan setTranslation:CGPointMake(0, 0) inView:_viewVideo];
//}
//
//- (void)uprightPanHandlerWithPlayViewFrame:(CGRect)playViewFrame withIsEnd:(BOOL)isEnd {
//    CGFloat moveY = movePoint.y - beginPoint.y;
//    CGFloat moveX = movePoint.x - beginPoint.x;
//    CGFloat velocity = [panHandler velocityInView:KEY_WINDOW].y;
//    NSLog(@"%f",velocity);
//    if (!isEnd) {
//        CGRect rect = playViewFrame;
//        rect.origin.y += moveY;
//        if (moveY > 0) {
//            CGFloat ratioK = 1 - moveY/KEY_WINDOW.frame.size.height;
//            rect.size.width *= ratioK;
//            rect.size.height *= ratioK;
//            ratioK *= 0.8;
//            if (ratioK < 0.1) {
//                ratioK = 0.1;
//            }
//            
//        }else {
//            rect.origin.y = 0;
//        }
//        rect.origin.x = (KEY_WINDOW.frame.size.width-rect.size.width)/2+moveX;
//        _viewVideo.frame = rect;
//        RECT drawRect = {0, 0, (int)rect.size.width, (int)rect.size.height};
//        _player.SetView(_player.hPlayer, (__bridge void*)_viewVideo, &drawRect);
//    }else {
//        //        if (moveY >= SCREEN_MAX_LENGTH/6) {
//        //            [self closeUpRightVideoWithAnimation];
//        //        }else {
//        //            if (velocity > 1000) {
//        //                [self closeUpRightVideoWithAnimation];
//        //            }else{
//        //                [UIView animateWithDuration:0.35 animations:^{
//        //                    self.view.frame = KEY_WINDOW.bounds;
//        //                    [self.view layoutIfNeeded];
//        //                } completion:^(BOOL finished) {
//        //                    [uprightBackView removeFromSuperview];
//        //                    isPanMove = NO;
//        //                }];
//        //            }
//        //        }
//    }
//}


@end


@implementation CGLView

+(Class) layerClass
{
    return [CAEAGLLayer class];
}

- (void)drawRect:(CGRect)rect
{
    // Drawing code
}

@end
