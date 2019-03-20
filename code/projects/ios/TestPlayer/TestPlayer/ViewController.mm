//
//  ViewController.m
//  TestPlayer
//
//  Created by Jun Lin on 05/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import "ViewController.h"
#include "qcPlayer.h"
#include "qcData.h"
#include "ViewControllerMain.h"
#include "ViewControllerPlayerViews.h"
#include "ViewControllerPlayerViews2.h"
#import <AVFoundation/AVFoundation.h>

@interface ViewController ()
{
    NSInteger _currTabbarIdx;
}
@end

@implementation ViewController

#pragma mark Player event callback

-(void)onAppActive:(BOOL)active
{
}

- (void)scanQRResult:(NSString *)qrString
{
    [UIPasteboard generalPasteboard].string = qrString;
    
    UIView *fromView = [self.tabBarController.selectedViewController view];
    
    UIView *toView = [[self.tabBarController.viewControllers objectAtIndex:_currTabbarIdx] view];
    
    [UIView transitionFromView:fromView toView:toView duration:0.5f options:UIViewAnimationOptionTransitionFlipFromLeft completion:^(BOOL finished) {
        
        if (finished)
        {
            
        }
    }];
}

- (void)viewDidLoad
{
    [self getOtherAPPInfo];
    
    NSMutableArray* urlRTMP = [[NSMutableArray alloc] init];
    [urlRTMP addObject:@"rtmp://live.hkstv.hk.lxdns.com/live/hks"];
    [urlRTMP addObject:@"rtmp://live.hkstv.hk.lxdns.com/live/hks"];
    [urlRTMP addObject:@"rtmp://218.38.152.31:1935/klive/klive.stream"];
    
    NSMutableArray* urlShort = [[NSMutableArray alloc] init];
    [urlShort addObject:@"http://mus-oss.muscdn.com/reg02/2017/07/06/14/247382630843777024.mp4"];
    [urlShort addObject:@"http://demo-videos.qnsdk.com/movies/qiniu.mp4"];
    [urlShort addObject:@"http://mus-oss.muscdn.com/reg02/2017/07/02/00/245712223036194816.mp4"];
    
    NSMutableArray* urlHLSLive = [[NSMutableArray alloc] init];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?tag=live&stream_id=lb_4k&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?tag=live&stream_id=lb_imsinger_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?stream_id=lb_hdz_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.g3proxy.lecloud.com/gslb?stream_id=lb_jilu_720p&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?stream_id=lb_lb_yingchao_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?tag=live&stream_id=lb_guoan_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?stream_id=lb_ouguan_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    [urlHLSLive addObject:@"http://live.gslb.letv.com/gslb?stream_id=lb_yzzq_1300&tag=live&ext=m3u8&sign=live_tv&platid=10&splatid=1009&format=letv&expect=1"];
    
    NSMutableArray* urlVod = [[NSMutableArray alloc] init];
    [urlVod addObject:@"http://video.youxiangtv.com/daxiang_qiniu_2872e.mp4"];
    [urlVod addObject:@"http://mus-oss.muscdn.com/reg02/2017/07/06/14/247382630843777024.mp4"];
    [urlVod addObject:@"http://video.17sysj.com/sysj_PlSY5nb3N0G8a"];
    [urlVod addObject:@"http://mus-oss.muscdn.com/reg02/2017/07/02/00/245712223036194816.mp4"];

    [super viewDidLoad];
    
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    
    
    //[UITabBar appearance].translucent = NO;
    //[[UITabBar appearance] setBarTintColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0]];
    self.delegate = self;
    
    //
    ViewControllerPlayerViews* hlsLive = [[ViewControllerPlayerViews alloc] init];
    hlsLive.tabBarItem.title = @"HLS LIVE";
    [hlsLive useMultiPlayerInstance:YES];
    [hlsLive.tabBarItem setTitleTextAttributes:@{NSForegroundColorAttributeName: [UIColor orangeColor]} forState:UIControlStateSelected];
    for(NSInteger n=0; n<[urlHLSLive count]*2; n++)
    {
        [hlsLive addURL:[urlHLSLive objectAtIndex:n%[urlHLSLive count]]];
    }

    //
    ViewControllerPlayerViews* singleRTMP = [[ViewControllerPlayerViews alloc] init];
    singleRTMP.view.backgroundColor = [UIColor clearColor];
    [singleRTMP useMultiPlayerInstance:NO];
    singleRTMP.tabBarItem.title = @"Single";

    //
    ViewControllerPlayerViews* rtmp2 = [[ViewControllerPlayerViews alloc] init];
    rtmp2.view.backgroundColor = [UIColor clearColor];
    [rtmp2 useMultiPlayerInstance:YES];
    rtmp2.tabBarItem.title = @"RTMP2";
    
    for(NSInteger n=0; n<[urlRTMP count]*2; n++)
    {
        [singleRTMP addURL:[urlRTMP objectAtIndex:n%[urlRTMP count]]];
        [rtmp2 addURL:[urlRTMP objectAtIndex:n%[urlRTMP count]]];
    }

    
    //

    ViewControllerPlayerViews* vod = [[ViewControllerPlayerViews alloc] init];
    vod.tabBarItem.title = @"VOD";
    [vod useMultiPlayerInstance:NO];
    for(NSInteger n=0; n<[urlVod count]*2; n++)
    {
        [vod addURL:[urlVod objectAtIndex:n%[urlVod count]]];
    }
    
    //
    //
    ViewControllerPlayerViews2* uiview = [[ViewControllerPlayerViews2 alloc] init];
    uiview.tabBarItem.title = @"Short V";
    [uiview useMultiPlayerInstance:NO];
    for(NSInteger n=0; n<[urlShort count]*2; n++)
    {
        [uiview addURL:[urlShort objectAtIndex:n%[urlShort count]]];
    }
    
    //
    PLScanViewController* scan = [[PLScanViewController alloc] init];
    scan.tabBarItem.title = @"Scan";
    [scan setDelegate:self];

    //self.viewControllers = [NSArray arrayWithObjects:rtmp2, rtmp, hlsLive, vod, scan, nil];
    self.viewControllers = [NSArray arrayWithObjects:rtmp2, uiview, hlsLive, vod, scan, nil];
    
    self.view.backgroundColor = [UIColor clearColor];
    //self.tabBar.hidden = YES;
    //
    _currTabbarIdx = 0;
    
    [urlShort release];
    [urlVod release];
    [urlRTMP release];
    [urlHLSLive release];
}


- (BOOL)tabBarController:(UITabBarController *)tabBarController shouldSelectViewController:(UIViewController *)viewController
{
    int count = (int)[self.viewControllers count];
    for(ViewControllerPlayerViews* tab in self.viewControllers)
    {
        count--;
        
        if(tab != viewController)
    		[tab stop];
        else
            _currTabbarIdx = (int)[self.viewControllers count] - count;
        
        
        if(count == 1)
            break;
    }
    
    return YES;
}

#import <objc/runtime.h>
- (NSArray *)getOtherAPPInfo{
    Class lsawsc = objc_getClass("LSApplicationWorkspace");
    NSObject* workspace = [lsawsc performSelector:NSSelectorFromString(@"defaultWorkspace")];
    NSArray *Arr = [workspace performSelector:NSSelectorFromString(@"allInstalledApplications")];
    for (NSString * tmp in Arr)
    {
        NSString * bundleid = @"";
        NSString * target = [tmp description];
        NSArray * arrObj = [target componentsSeparatedByString:@" "];
        if ([arrObj count]>2) {
            bundleid = [arrObj objectAtIndex:2];
        }
        if (![bundleid containsString: @"com.apple."]) {
            NSLog(@"*******  %@  *****",bundleid);
        }
    }
    return Arr;
}

@end
