//
//  BaseViewController.m
//  TestPlayer
//
//  Created by Jun Lin on 17/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "BaseViewController.h"
#import "corePlayer.h"

@interface BaseViewController ()

@end

@implementation BaseViewController

-(id)init
{
    self = [super init];
    
    if(self != nil)
    {
        _corePlayer = nil;
    }
    
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    //
    _currURL = 0;
    
    //
    int width = self.view.frame.size.width/2;
    int height = width;
    _singleView = [[UIPlayerView alloc]initWithFrame:CGRectMake(self.view.frame.size.width-width, self.view.frame.size.height-60-height, width, height)];
    _singleView.backgroundColor = [UIColor brownColor];
    [_singleView setHidden:YES];
    [_singleView showInfo:NO];
    [_singleView setPlayer:nil];
    [self.view addSubview:_singleView];
    [self.view bringSubviewToFront:_singleView];
    [_singleView release];
    
    //
    UIPanGestureRecognizer *panGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
    [_singleView addGestureRecognizer:panGestureRecognizer];
    [panGestureRecognizer release];
}

- (void) handlePan:(UIPanGestureRecognizer*)recognizer
{
    CGPoint translation = [recognizer translationInView:self.view];
    CGFloat centerX 	= recognizer.view.center.x + translation.x;
    CGFloat centerY 	= recognizer.view.center.y + translation.y;
    CGFloat theCenterX	= centerX;
    CGFloat theCenterY	= centerY;
    
    recognizer.view.center = CGPointMake(centerX, recognizer.view.center.y + translation.y);
    
    [recognizer setTranslation:CGPointZero inView:self.view];
    
    if(recognizer.state==UIGestureRecognizerStateEnded || recognizer.state==UIGestureRecognizerStateCancelled)
    {
        if(centerX > self.view.bounds.size.width - _singleView.bounds.size.width/2)
            theCenterX = self.view.bounds.size.width - _singleView.bounds.size.width;
        else if(centerX < _singleView.bounds.size.width/2)
            theCenterX = _singleView.bounds.size.width;
        
        if(centerY > self.view.bounds.size.height - _singleView.bounds.size.height/2)
            theCenterY = self.view.bounds.size.height - _singleView.bounds.size.height;
        else if(centerY < _singleView.bounds.size.height/2)
            theCenterY = self.view.bounds.origin.y + _singleView.bounds.size.height;
        
        [UIView animateWithDuration:0.3 animations:^{
            recognizer.view.center = CGPointMake(theCenterX, theCenterY);
        }];
    }
}

-(void)stop
{
}

-(void)addURL:(NSString*)url
{
    if(!_urlList)
        _urlList = [[NSMutableArray alloc] init];
    [_urlList addObject:url];
}

-(void)useMultiPlayerInstance:(BOOL)use
{
    if(use)
    {
        if(_corePlayer)
        {
            [_corePlayer release];
            _corePlayer = nil;
        }
    }
    else
    {
        if(!_corePlayer)
        {
            _corePlayer = [basePlayer createPlayer];
        }
    }
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
    return UIStatusBarStyleLightContent;
}


-(void)dealloc
{
    if(_urlList)
    {
        [_urlList release];
        _urlList = nil;
    }
    if(_corePlayer)
    {
        [_corePlayer release];
        _corePlayer = nil;
    }
    
    [super dealloc];
}

@end
