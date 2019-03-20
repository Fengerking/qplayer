/**
 * File : VideoCapture.h
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : VideoCapture
 */

#import <Foundation/Foundation.h>
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureOutput.h>
#import <UIKit/UIKit.h>

@interface VideoCapture : NSObject
{
   AVCaptureSession*           captureSession;
   AVCaptureVideoPreviewLayer*  previewLayer;
   AVCaptureVideoDataOutput*    vdOutput;
   void*                    ihecHandle;
    
}
//@interface AVCaptureManager

@property(nonatomic,assign) bool   mIsRunning;

- (id)initWithCameratype:(int) type;
- (void)setPreviewView:(UIView *)previewView;
- (void)start;
- (void)stop;
- (bool)setvideoparameter: (int)width andhi:(int)hight andfps:(int)fps andbitrate:(int) bitrate;
- (void)setPush:(void*)obj;
- (void) setCameraType:(int)type;
- (int)swapFrontAndBackCameras;
- (void)startEncode;
- (bool)opencamera;
- (void)stoptransfer;
- (void) resetEncoder:(int)framerate andbitrate:(int) bitrate;
- (UIImage*)getlastpic;
-(void) rescale_360640:(CMSampleBufferRef)sampleBuffer;
- (void) useBeauty:(bool) bty;
- (bool)IsDiscardingFrame;
@end
