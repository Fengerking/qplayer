//
//  ViewController.h
//  TestPlayer
//
//  Created by LinJacky on 16/7/16.
//  Copyright © 2016年 JackyLin. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/glKit.h>
#import <PlayerEngine/PlayerEngine.h>

@interface ViewController : UIViewController {
        GKMediaPlayerProxy* iPlayer;
}

- (IBAction)Play:(id)sender;
- (IBAction)Stop:(id)sender;

- (IBAction)Pause:(id)sender;
- (IBAction)Resume:(id)sender;
- (IBAction)TouchEnable:(id)sender;


@property (weak, nonatomic) IBOutlet UIButton *btnPlay;
@property (weak, nonatomic) IBOutlet UIButton *btnStop;
@property (weak, nonatomic) IBOutlet UIButton *btnResume;
@property (weak, nonatomic) IBOutlet UIButton *btnPause;
@property (weak, nonatomic) IBOutlet UIButton *btnTouchEnable;
@property (weak, nonatomic) IBOutlet UIButton *btnSeek;

@end

