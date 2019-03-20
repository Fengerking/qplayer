/**
 * File : VideoRecordCapture.h
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : VideoRecordCapture
 */

#import <Foundation/Foundation.h>
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureOutput.h>
#import <UIKit/UIKit.h>

@interface VideoRecordCapture : NSObject
{
   AVCaptureSession*           captureSession;
   AVCaptureVideoPreviewLayer*  previewLayer;
   AVCaptureVideoDataOutput*    vdOutput;
   void*                    ihecHandle;
    
}

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
- (UIImage*)getfirstpic;
-(void) rescale_to_480288:(CMSampleBufferRef)sampleBuffer;
- (void)pause;
- (void)resume;
-(void) ManualFocus:(GLfloat) x andy:(GLfloat)y;
- (float)GetMaxZoomFactor;
-(void) zoomInOut:(GLfloat) level;
@end
