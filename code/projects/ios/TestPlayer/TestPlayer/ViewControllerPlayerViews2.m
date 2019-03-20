//
//  ViewController.m
//  SlidPageDemo
//
//  Created by cuctv-duan on 16/8/26.
//  Copyright © 2016年 cuctv-duan. All rights reserved.
//

#import "ViewControllerPlayerViews2.h"
#import "LivingInfoView.h"

#define Screen_width [UIScreen mainScreen].bounds.size.width
#define Screen_height [UIScreen mainScreen].bounds.size.height

//#define ENABLE_MOVE

@interface ViewControllerPlayerViews2 ()
{
    BOOL 			isBeginSlid;//记录开始触摸的方向
 
    UIPlayerView*	playLayer;
    LivingInfoView* livingInfoView;
    
    //UIPlayerView*	videoView;
}

@end

@implementation ViewControllerPlayerViews2

- (void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer
{
    if(recognizer.direction == UISwipeGestureRecognizerDirectionDown)
    {
        NSLog(@"swipe down");
        if(_currURL >= 0)
        {
            [playLayer setPlayer:_corePlayer];
            [playLayer open:_urlList[_currURL==0?0:--_currURL]];
            playLayer.tag = _currURL;
        }
    }
    if(recognizer.direction == UISwipeGestureRecognizerDirectionUp)
    {
        NSLog(@"swipe up");
        if(_currURL <= [_urlList count]-1)
        {
            [playLayer setPlayer:_corePlayer];
            [playLayer open:_urlList[_currURL==([_urlList count]-1)?_currURL:++_currURL]];
            playLayer.tag = _currURL;
        }
    }
    if(recognizer.direction == UISwipeGestureRecognizerDirectionLeft)
    {
        NSLog(@"swipe left");
    }
    if(recognizer.direction == UISwipeGestureRecognizerDirectionRight)
    {
        NSLog(@"swipe right");
    }
}

- (void)viewDidLoad {
    // Do any additional setup after loading the view, typically from a nib.
    self.view.backgroundColor = [UIColor whiteColor];
    
//    播放层
    playLayer = [[UIPlayerView alloc]initWithFrame:CGRectMake(0, 0, Screen_width, Screen_height)];
    [self.view addSubview:playLayer];
    playLayer.backgroundColor = [UIColor blackColor];
    [playLayer release];
    
    //playLayer.image = [UIImage imageNamed:@"PlayView"];
    playLayer.userInteractionEnabled = YES;
    
#ifndef ENABLE_MOVE

    UISwipeGestureRecognizer* recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
    [recognizer setDirection:(UISwipeGestureRecognizerDirectionUp)];
    [playLayer addGestureRecognizer:recognizer];
    
    recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
    [recognizer setDirection:(UISwipeGestureRecognizerDirectionDown)];
    [self.view addGestureRecognizer:recognizer];
    
	/*
    recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
    [recognizer setDirection:(UISwipeGestureRecognizerDirectionRight)];
    [self.view addGestureRecognizer:recognizer];
    
    recognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(handleSwipeFrom:)];
    [recognizer setDirection:(UISwipeGestureRecognizerDirectionLeft)];
    [self.view addGestureRecognizer:recognizer];
     */
#endif
    
    
//    播放信息层
    livingInfoView = [[LivingInfoView alloc]initWithFrame:CGRectMake(0, 0, Screen_width, Screen_height)];
    livingInfoView.backgroundColor = [UIColor colorWithRed:242 / 255.0 green:156 / 255.0 blue:177 / 255.0 alpha:0.6];
    [playLayer addSubview:livingInfoView];
    [livingInfoView release];
    [livingInfoView setHidden:YES];
    
    //
//    videoView = [[UIPlayerView alloc] initWithFrame: CGRectMake(0, 0, Screen_width, Screen_width)];
//    videoView.layer.borderWidth = 1;
//    videoView.layer.borderColor = [[UIColor orangeColor] CGColor];
//    videoView.backgroundColor = [UIColor blackColor];
//    [self.view insertSubview:videoView atIndex:0];
//    [videoView release];

    
    [super viewDidLoad];
}

-(void)stop
{
    if(playLayer)
        [playLayer stop];
}

#pragma mark - 界面的滑动
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    isBeginSlid = YES;
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{

    //    1.获取手指
    UITouch *touch = [touches anyObject];
    //    2.获取触摸的上一个位置
    CGPoint lastPoint;
    CGPoint currentPoint;
    
    //    3.获取偏移位置
    CGPoint tempCenter;
    
    if (isBeginSlid) {//首次触摸进入
        lastPoint = [touch previousLocationInView:playLayer];
        currentPoint = [touch locationInView:playLayer];
        
        
        //判断是左右滑动还是上下滑动
        if (ABS(currentPoint.x - lastPoint.x) > ABS(currentPoint.y - lastPoint.y)) {
            //    3.获取偏移位置
            tempCenter = livingInfoView.center;
            tempCenter.x += currentPoint.x - lastPoint.x;//左右滑动
            //禁止向左划
            if (livingInfoView.frame.origin.x == 0 && currentPoint.x -lastPoint.x > 0) {//滑动开始是从0点开始的，并且是向右滑动
                livingInfoView.center = tempCenter;
                
            }
//            else if(livingInfoView.frame.origin.x > 0){
//                livingInfoView.center = tempCenter;
//            }
//            NSLog(@"%@-----%@",NSStringFromCGPoint(tempCenter),NSStringFromCGPoint(livingInfoView.center));
        }else{
            //    3.获取偏移位置
            tempCenter = playLayer.center;
            tempCenter.y += currentPoint.y - lastPoint.y;//上下滑动
            
#ifdef ENABLE_MOVE
            playLayer.center = tempCenter;
#endif
        }
    }else{//滑动开始后进入，滑动方向要么水平要么垂直
        if (playLayer.frame.origin.y != 0){//垂直的优先级高于左右滑，因为左右滑的判定是不等于0
            
            lastPoint = [touch previousLocationInView:playLayer];
            currentPoint = [touch locationInView:playLayer];
            tempCenter = playLayer.center;
            
            tempCenter.y += currentPoint.y -lastPoint.y;
#ifdef ENABLE_MOVE
            playLayer.center = tempCenter;
#endif
        }else if (livingInfoView.frame.origin.x != 0) {
            
            lastPoint = [touch previousLocationInView:livingInfoView];
            currentPoint = [touch locationInView:livingInfoView];
            tempCenter = livingInfoView.center;
            
            tempCenter.x += currentPoint.x -lastPoint.x;
            
            //禁止向左划

            if (livingInfoView.frame.origin.x == 0 && currentPoint.x -lastPoint.x > 0) {//滑动开始是从0点开始的，并且是向右滑动
                livingInfoView.center = tempCenter;
        
            }else if(livingInfoView.frame.origin.x > 0){
                livingInfoView.center = tempCenter;
            }
            
        }
    }
  
    isBeginSlid = NO;
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
//    NSLog(@"%.2f-----%.2f",livingInfoView.frame.origin.y,Screen_height * 0.8);
    
//    水平滑动判断
//在控制器这边滑动判断如果滑动范围没有超过屏幕的十分之八livingInfoView还是离开屏幕
    if (livingInfoView.frame.origin.x > Screen_width * 0.8) {
        [UIView animateWithDuration:0.06 animations:^{
            CGRect frame = livingInfoView.frame;
            frame.origin.x = Screen_width;
            livingInfoView.frame = frame;
        }];
        
    }else{//否则则回到屏幕0点
        [UIView animateWithDuration:0.2 animations:^{
            CGRect frame = livingInfoView.frame;
            frame.origin.x = 0;
            livingInfoView.frame = frame;
            
        }];
    }
    
//    上下滑动判断
    if (playLayer.frame.origin.y > Screen_height * 0.2) {
//        切换到上一频道
        if(_currURL >= 0)
        {
            [playLayer setPlayer:_corePlayer];
            [playLayer open:_urlList[_currURL==0?0:--_currURL]];
            playLayer.tag = _currURL;
        }
    }else if (playLayer.frame.origin.y < - Screen_height * 0.2){
//        切换到下一频道
        if(_currURL < [_urlList count]-1)
        {
            [playLayer setPlayer:_corePlayer];
            [playLayer open:_urlList[++_currURL]];
            playLayer.tag = _currURL;
        }
    }
//        回到原始位置等待界面重新加载
    CGRect frame = playLayer.frame;
    frame.origin.y = 0;
    playLayer.frame = frame;
    
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
    
    //    水平滑动判断
    //在控制器这边滑动判断如果滑动范围没有超过屏幕的十分之八livingInfoView还是离开屏幕
    if (livingInfoView.frame.origin.x > Screen_width * 0.8) {
        [UIView animateWithDuration:0.06 animations:^{
            CGRect frame = livingInfoView.frame;
            frame.origin.x = Screen_width;
            livingInfoView.frame = frame;
        }];
        
    }else{//否则则回到屏幕0点
        [UIView animateWithDuration:0.2 animations:^{
            CGRect frame = livingInfoView.frame;
            frame.origin.x = 0;
            livingInfoView.frame = frame;
            
        }];
    }
    
    //    上下滑动判断
    if (livingInfoView.frame.origin.y > Screen_height * 0.2) {
        //        切换到下一频道
        livingInfoView.backgroundColor = [UIColor colorWithRed:arc4random_uniform(256) / 255.0 green:arc4random_uniform(256) / 255.0 blue:arc4random_uniform(256) / 255.0 alpha:0.5];
        
    }else if (livingInfoView.frame.origin.y < - Screen_height * 0.2){
        //        切换到上一频道
        livingInfoView.backgroundColor = [UIColor colorWithRed:arc4random_uniform(256) / 255.0 green:arc4random_uniform(256) / 255.0 blue:arc4random_uniform(256) / 255.0 alpha:0.5];
        
    }
    //        回到原始位置等待界面重新加载
    CGRect frame = livingInfoView.frame;
    frame.origin.y = 0;
    livingInfoView.frame = frame;
}

@end
