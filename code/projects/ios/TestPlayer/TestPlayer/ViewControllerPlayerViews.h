//
//  ViewControllerLive.h
//  TestPlayer
//
//  Created by Jun Lin on 07/05/2017.
//  Copyright © 2017 Qiniu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "BaseViewController.h"

@interface ViewControllerPlayerViews : BaseViewController<UICollectionViewDataSource, UICollectionViewDelegate, UICollectionViewDelegateFlowLayout, UIGestureRecognizerDelegate>
-(void)stop;
@end
