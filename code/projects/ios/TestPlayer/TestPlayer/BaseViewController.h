//
//  BaseViewController.h
//  TestPlayer
//
//  Created by Jun Lin on 17/06/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "UIPlayerView.h"
#import "basePlayer.h"

@interface BaseViewController : UIViewController
{
    int					_currURL;
    NSMutableArray* 	_urlList;
    UIPlayerView*		_singleView;
    basePlayer*			_corePlayer;
}

-(void)addURL:(NSString*)url;
-(void)stop;
-(void)useMultiPlayerInstance:(BOOL)use;

@end
