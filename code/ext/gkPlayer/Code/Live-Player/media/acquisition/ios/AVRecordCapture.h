/**
 * File : AVRecordCapture.h
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : AVRecordCapture
 */

#import <Foundation/Foundation.h>
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureOutput.h>
#import <UIKit/UIKit.h>
#import <AVFoundation//AVAssetWriter.h>
#import <AVFoundation//AVAssetWriterInput.h>

@interface AVRecordCapture : NSObject
{
   AVCaptureSession*           captureSession;
   AVCaptureVideoPreviewLayer*  previewLayer;
   AVCaptureVideoDataOutput*    vdOutput;
   AVCaptureAudioDataOutput*    audioOut;
   AVAssetWriter*             videoWriter ;
   AVAssetWriterInput*       videoWriterInput;
    AVAssetWriterInput*      audioWriterInput;
    AVAssetWriterInputPixelBufferAdaptor* adaptor;
   void*                    ihecHandle;
    bool                    StopRecord;
    
}

@property(nonatomic,assign) bool   mIsRunning;

- (id)initWithCameratype:(int) type;
- (void)setPreviewView:(UIView *)previewView;
- (void)start;
- (int)stop;
- (bool)setvideoparameter: (int)width andhi:(int)hight;
- (void) setCameraType:(int)type;
- (int)swapFrontAndBackCameras;
- (void)startEncode;
- (bool)opencamera;
- (UIImage*)getfirstpic;
-(void) rescale_to_480288:(CMSampleBufferRef)sampleBuffer;
- (void)pause;
- (void)resume;
-(void) ManualFocus:(GLfloat) x andy:(GLfloat)y;
- (float)GetMaxZoomFactor;
-(void) zoomInOut:(GLfloat) level;
- (void) setfilepath:(NSString*)apath;
-(long) getRecordTime;
-(void) resetRecord;
@end
