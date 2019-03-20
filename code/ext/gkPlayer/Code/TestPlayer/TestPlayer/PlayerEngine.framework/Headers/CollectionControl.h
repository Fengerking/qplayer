/**
 * File : CollectionControl.h
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : CollectionControl
 */

#ifndef CollectionControl_h
#define CollectionControl_h
#import <UIKit/UIKit.h>

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1
@interface CollectionControl : NSObject
{

}

@property(nonatomic,assign) bool   mIsRunning;

- (id)initWithCameratype:(int) type;
-(void)startcapture;
-(void)closecamera;
-(void)stoppush;
- (void)setPreviewView:(UIView *)previewView;
- (void) setaudioparameter: (int)samplerate andch: (int)channel;
- (bool) setvideoparameter: (int)width andhi:(int)hight andfps:(int)fps andbitrate:(int) bitrate;

- (void) setpushlishurl:(NSString*)aurl;
- (void) setCameraType:(int)type;
- (void) swap;
- (void) opencamera: (UIView *)previewView;
- (void) startlive;
- (UIImage*)getlastpic;

- (void) useBeauty:(bool) bty;
- (void) netdisconnect;

- (void) resetEncoder:(int)framerate andbitrate:(int) bitrate;
@end


#endif /* CollectionControl_h */
