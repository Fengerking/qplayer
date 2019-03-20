//
//  ViewController.m
//  TestPlayer
//
//  Created by LinJacky on 16/7/16.
//  Copyright © 2016年 JackyLin. All rights reserved.
//

#import "ViewController.h"

@interface ViewController () {
    BOOL    isHideStateView;
    
    int    nRendType;
    int    nCount;
    
    bool    nTouchEnable;
}
@property (weak, nonatomic) IBOutlet UIView *VideoView;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    self.VideoView.autoresizingMask = UIViewAutoresizingNone;
    
    nTouchEnable = false;
    nCount = -1;
    
    iPlayer = [[GKMediaPlayerProxy alloc] init];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(PlayerNotifyEvent:) name:@"PlayerNotifyEvent" object:nil];
    
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(backgroundNotify:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(foregroundNotify:) name:UIApplicationDidBecomeActiveNotification object:nil];
    
    //增加单击手势识别
    UITapGestureRecognizer  *tapGestureRecognizer = [[UITapGestureRecognizer alloc] init];
    tapGestureRecognizer.numberOfTapsRequired = 1;//设置单击一次触发方法
    [tapGestureRecognizer addTarget:self action:@selector(tapGestureAction:)];
    
    [self.view addGestureRecognizer:tapGestureRecognizer];//添加到当前view
    
    //增加双击手势识别
    UITapGestureRecognizer  *tap2GestureRecognizer = [[UITapGestureRecognizer alloc] init];
    tap2GestureRecognizer.numberOfTapsRequired = 2;//设置单击一次触发方法
    [tap2GestureRecognizer addTarget:self action:@selector(tap2GestureAction:)];
    
    [self.view addGestureRecognizer:tap2GestureRecognizer];//添加到当前view
    
    [tapGestureRecognizer requireGestureRecognizerToFail:tap2GestureRecognizer];
    
//    UISwipeGestureRecognizer *recognizer;
//    
//    recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
//    
//    [recognizer setDirection:(UISwipeGestureRecognizerDirectionUp)];
//    [[self view] addGestureRecognizer:recognizer];
}

- (void)viewDidLayoutSubviews {
    if (UIInterfaceOrientationIsLandscape(self.interfaceOrientation)) {
        self.VideoView.frame = self.view.frame;
    }else {
        self.VideoView.frame = (CGRect){{0,0}, {self.view.bounds.size.width, self.view.bounds.size.height}};
    }
}

-(void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer{
//    if(!nTouchEnable) {
//        nTouchEnable = true;
//        [iPlayer stop];
//        [iPlayer SetView:self.VideoView];
//        nRendType = ERenderGlobelView | ERenderSplitView|ERenderMeshDistortion;
//        [iPlayer SetRendType:nRendType];
//        //NSString* pStr =@"http://pull99.a8.com/live/1471412095644748.flv?ikHost=ws&ikOp=1&CodecInfo=8192";
//        NSString* pStr = [[NSBundle mainBundle] pathForResource:@"MTV" ofType:@"mp4"];
//        NSLog(@"%@", pStr);
//        [iPlayer playWithUrl:pStr andFlag:1];//*/
//    } else {
//        nTouchEnable = false;
//        [iPlayer stop];
//        [iPlayer SetView:self.VideoView];
//        nRendType = ERenderGlobelView | ERenderSplitView|ERenderMeshDistortion;
//        [iPlayer SetRendType:nRendType];
//        //NSString* pStr =@"http://pull99.a8.com/live/1471411911664645.flv?ikHost=ws&ikOp=1&CodecInfo=8192";
//        NSString* pStr = [[NSBundle mainBundle] pathForResource:@"MTV" ofType:@"mp4"];
//        NSLog(@"%@", pStr);
//        [iPlayer playWithUrl:pStr andFlag:1];//*/
//    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)tapGestureAction:(UITapGestureRecognizer *)sender
{
    NSLog(@"tapGestureAction called");
    if ( isHideStateView ) {
        [_btnPlay setHidden:NO];
        [_btnStop setHidden:NO];
        [_btnPause setHidden:NO];
        [_btnResume setHidden:NO];
        [_btnTouchEnable setHidden:NO];
        [_btnSeek setHidden:NO];
    }else{
        [_btnPlay setHidden:YES];
        [_btnStop setHidden:YES];
        [_btnPause setHidden:YES];
        [_btnResume setHidden:YES];
        [_btnTouchEnable setHidden:YES];
        [_btnSeek setHidden:YES];
    }
    isHideStateView = !isHideStateView;
}


- (void)tap2GestureAction:(UITapGestureRecognizer *)sender
{
    NSLog(@"tap2GestureAction called");
    nCount++;
    if(nCount == 6) {
        nCount = 0;
    }
        
    if(nCount == 0) {
        nRendType = ERenderGlobelView;
    } else if(nCount == 1) {
        nRendType = ERenderGlobelView | ERenderSplitView;
    } else if(nCount == 2) {
        nRendType = ERenderGlobelView | ERenderSplitView | ERenderMeshDistortion;;
    } else if(nCount == 3) {
        nRendType = ERenderDefault;
    } else if(nCount == 4) {
        nRendType = ERenderSplitView;
    } else if(nCount == 5) {
        nRendType = ERenderSplitView|ERenderMeshDistortion;
    }
    
    [iPlayer SetRendType:nRendType];
}


- (IBAction)Play:(id)sender {
    [iPlayer SetView:self.VideoView];
    nRendType = ERenderDefault;//|ERenderGlobelView|ERenderSplitView|ERenderMeshDistortion;//|ERender180View|ERenderLR3D;
    [iPlayer SetRendType:nRendType];

    
    bool bMove = true;
    [iPlayer SetMotionEnable:bMove];
    
    //NSString* pStr =@"http://static.cdn.doubo.tv/search/e0e46eb23084400f81337ea941ba9a0a.mp4";
    //NSString* pStr =@"http://vod.vr800.com/zyl/1013sd.mp4";
    NSString* pStr = [[NSBundle mainBundle] pathForResource:@"MTV" ofType:@"mp4"];
    NSLog(@"%@", pStr);
    [iPlayer playWithUrl:pStr andFlag:1];//*/
}

- (IBAction)Stop:(id)sender {
     [iPlayer stop];
}

- (IBAction)Pause:(id)sender {
    [iPlayer pause];
}

- (IBAction)Resume:(id)sender {
    [iPlayer resume];
}

- (IBAction)TouchEnable:(id)sender {
    if(nTouchEnable) {
        nTouchEnable = false;
        [iPlayer SetTouchEnable:nTouchEnable];
    } else {
        nTouchEnable = true;
        [iPlayer SetTouchEnable:nTouchEnable];
    }
}
- (IBAction)SeekTime:(id)sender {
    CMTime playerProgress = [iPlayer getPosition];
    //printf("current time : %lld\n",playerProgress.value);
    playerProgress.value += 1000000;
    [iPlayer setPosition:playerProgress];
    
}

- (void) foregroundNotify : (NSNotification*) aNotification
{
    if (iPlayer) {
        printf("---for\n");
        [iPlayer VideoForegroundHandle: self.view];
    }
}

- (void) backgroundNotify : (NSNotification*) aNotification
{
    if (iPlayer) {
        printf("---back\n");
        [iPlayer VideoBackgroundHandle];
    }
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    //return UIInterfaceOrientationMaskPortrait;
    return UIInterfaceOrientationMaskLandscape;
}

- (void)PlayerNotifyEvent : (NSNotification*) aNotification
{
    GKMsgObject* pMsgObject = (GKMsgObject*)[aNotification object];
    
    if (pMsgObject.iMsg == ENotifyComplete)
    {
    }
    else if (pMsgObject.iMsg == ENotifyStop)
    {

    }
    else if(pMsgObject.iMsg == ENotifyPrepare)
    {
        [iPlayer start];
    }else if (pMsgObject.iMsg == ENotifyUpdateDuration)
    {
        printf("duration:%f",CMTimeGetSeconds([iPlayer duration]));
    }
    
    //printf("receive NotifyPlayEvent: Msg:%d, Error:%d\n", pMsgObject.iMsg, pMsgObject.iError);
}

@end
