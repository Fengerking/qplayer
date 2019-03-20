//
//  ViewController.h
//  TestPlayer
//
//  Created by Jun Lin on 05/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PLScanViewController.h"

@interface ViewController : UITabBarController <UITabBarControllerDelegate, PLScanViewControlerDelegate>

-(void)onAppActive:(BOOL)active;
@end

