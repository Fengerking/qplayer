/**
 * File : VideoRecordCapture.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : VideoRecordCapture
 */

#import "VideoRecordCapture.h"
#import <AVFoundation/AVFoundation.h>
#include "VideoEncoder.h"
#include <sys/sysctl.h>

#include "TTVideoView.h"
#include "TTGLRenderBase.h"
#include "TTGLRenderES2.h"
#include "TTGLRenderES2_FTU.h"
#include "TTCollectCommon.h"

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1

#define clip(x)  ((x) > 255 ? 255 : ( (x) < 0 ? 0 : (x) ))

@interface VideoRecordCapture ()
<AVCaptureVideoDataOutputSampleBufferDelegate>
{
    CTTVideoEncoder* VideoEncoder;
    int mwidth;
    int mhight;
    int mfps ;
    int mbitrate;
    int cameratype;
    int mSwapFlag;
    int mSwapCnt;
    
    TTGLRenderBase*  m_pRender;
    TTVideoBuffer	 mSinkBuffer;
    EAGLContext* mPContext;
    UIView*  mUIView;
    
    CVPixelBufferRef mPixelBuffer;
    
    AVCaptureDevice *videoDevice;
    float   mfactor;
    bool    mPreivew;
}
@end

@implementation VideoRecordCapture


- (bool)opencamera
{
    self.mIsRunning=false;
    if (VideoEncoder) {
        delete VideoEncoder;
        VideoEncoder = NULL;
    }
    VideoEncoder = new CTTVideoEncoder(MEDIATYPE_RECORD);
    mSwapCnt = 0;
    mSwapFlag = 0;
    mPreivew = true;
    //mResetEncoder = false;
    
    NSError *error;
    
    captureSession = [[AVCaptureSession alloc] init];
    captureSession.sessionPreset = AVCaptureSessionPresetInputPriority;
    
    [captureSession beginConfiguration];
    
    videoDevice = NULL;
    if (cameratype == 0) {
        videoDevice = [self cameraWithPosition:AVCaptureDevicePositionFront];
    }
    else{
        videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack];
    }
    
    [videoDevice lockForConfiguration:nil];
    if ([videoDevice isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure]) {
        //[videoDevice setExposureMode:AVCaptureExposureModeContinuousAutoExposure ];
    }
    [videoDevice unlockForConfiguration];
    
    AVCaptureDeviceInput *videoIn = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
    
    if (error) {
        NSLog(@"Video input creation failed");
        return false;
    }
    
    if (![captureSession canAddInput:videoIn]) {
        NSLog(@"Video input add-to-session failed");
        return false;
    }
    
    [captureSession addInput:videoIn];
    
    // video data output
    vdOutput = [[AVCaptureVideoDataOutput alloc] init];
    [captureSession addOutput: vdOutput];
    dispatch_queue_t VcaptureQueue = dispatch_queue_create("VcaptureQueue", NULL);
    [vdOutput setSampleBufferDelegate:self queue:VcaptureQueue];
    //dispatch_release(queue);
    
    NSString* key = (NSString*)kCVPixelBufferPixelFormatTypeKey;
    NSNumber* val = [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange];//nv12 uvuv
    NSDictionary* videoSettings =[NSDictionary dictionaryWithObject:val forKey:key];
    
    vdOutput.videoSettings = videoSettings;
    
    AVCaptureConnection* connection = [vdOutput connectionWithMediaType:AVMediaTypeVideo];
    connection.videoMinFrameDuration = CMTimeMake(1, 15);//15 is fps
    
    if ([connection isVideoOrientationSupported])
        [connection setVideoOrientation:AVCaptureVideoOrientationPortrait];
    
    [captureSession commitConfiguration];
    
    mfactor = videoDevice.activeFormat.videoMaxZoomFactor;
    
    return true;
    
}


-(void)RenderInit
{
    if ( mPContext == NULL) {
        
        mPContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        
        if (nil != mPContext)
        {
            if (NULL == m_pRender) {
                m_pRender = new TTGLRenderES2_FTU(mPContext);
                m_pRender->SetView((UIView*)mUIView);
                m_pRender->SetDisplayRatio(DISAPLAY_RATIO_5_3);
                m_pRender->init();
                m_pRender->SetTexture(mwidth,mhight);
            }
        }
        
        CVPixelBufferCreate(NULL, mwidth, mhight, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &mPixelBuffer);
    }
}

- (id) initWithCameratype:(int) type
 {
    self = [super init];
    
    cameratype = type;
     
     mUIView = nil;
    
     bool open = [self opencamera];
     
     if (open == false) {
         return nil;
     }
     
     m_pRender = NULL;
     mPContext = NULL;
     
     return self;
 }

- (AVCaptureDevice *)cameraWithPosition:(AVCaptureDevicePosition)position
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for ( AVCaptureDevice *device in devices )
        if ( device.position == position )
            return device;
    return nil;
}

- (void) setCameraType:(int)type
{
    cameratype = type;
}

- (void)setPreviewView:(UIView *)previewView {

    mUIView = previewView;
}

- (void)setPush:(void*)obj
{
    VideoEncoder->setPush(obj);
}

- (void)start
{
    VideoEncoder->setvideoparameter(mwidth, mhight, mfps, mbitrate);
    VideoEncoder->init();
    [captureSession startRunning];
    //self.mIsRunning= YES;
}


- (void)setRelativeVideoOrientation {
}

- (void)toggleContentsGravity {
    
    if ([previewLayer.videoGravity isEqualToString:AVLayerVideoGravityResizeAspectFill]) {
    
        previewLayer.videoGravity = AVLayerVideoGravityResizeAspect;
    }
    else {
        previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    }
}


- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    [self rescale_to_480288:sampleBuffer];
    mSinkBuffer.Buffer[0] = (TTPBYTE)mPixelBuffer;
    
    if (cameratype == CAMEAR_FRONT) {
        VideoEncoder->revert(mPixelBuffer);
    }
    if (mPreivew) {
        m_pRender->RenderYUV(&mSinkBuffer);
    }
    
    if (self.mIsRunning == 0)
        return;
    
    if (mSwapFlag > 0) {
        mSwapCnt--;
        if (mSwapCnt<=0) {
            mSwapCnt = 0;
            mSwapFlag = 0;
        }
        else
            return;
    }

    VideoEncoder->encoder2(mPixelBuffer, ihecHandle, cameratype);
}

-(void) rescale_to_480288:(CMSampleBufferRef)sampleBuffer
{
    CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    //CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    //int w =  CVPixelBufferGetWidth(imageBuffer);
    int h = CVPixelBufferGetHeight(imageBuffer);
    
    size_t RowByters  = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer,0);
    
    unsigned char* pY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
    unsigned char* pVUSrc = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 1);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    
    int start = (h - mhight)/2;
    
    pY = pY + RowByters*start;
    memcpy(targetY, pY, RowByters*mhight);
    
    int uvstart = (h/2-mhight/2)/2;
    pVUSrc = pVUSrc + uvstart*RowByters;
    memcpy(targetUV, pVUSrc, RowByters*mhight/2);
    
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    CVPixelBufferUnlockBaseAddress(mPixelBuffer, 0);
}

- (void)stoptransfer
{
    self.mIsRunning=false;
    mPreivew = false;
}
- (void)pause
{
    self.mIsRunning=false;
}
- (void)resume
{
    self.mIsRunning=true;
}

- (void) stop
{
    self.mIsRunning=false;
    [captureSession stopRunning];
    if(VideoEncoder != NULL){
        VideoEncoder->uninit();
        delete VideoEncoder;
    }
    VideoEncoder = NULL;
    
    for(AVCaptureInput *input1 in captureSession.inputs) {
        [captureSession removeInput:input1];
    }
    
    for(AVCaptureOutput *output1 in captureSession.outputs) {
        [captureSession removeOutput:output1];
    }
    
    [vdOutput release];
    vdOutput = nil;

    SAFE_DELETE(m_pRender);
    CVPixelBufferRelease(mPixelBuffer);
   
    [captureSession release];
    captureSession = nil;
}


- (void)startEncode
{
    self.mIsRunning = true;
    mPreivew = true;
}

- (bool) setvideoparameter: (int)width andhi:(int)hight andfps:(int)fps andbitrate:(int) bitrate
{
    if (width > hight) {
        mwidth = hight;
        mhight = width;
    }
    else{
        mwidth = width;
        mhight = hight;
    }
    
    mfps = fps;
    mbitrate = bitrate;
    
    bool ret = true;
    [captureSession beginConfiguration];

    if ((width == 640 && hight == 480) || (width == 480 && hight == 640)){
        if ([captureSession canSetSessionPreset:AVCaptureSessionPreset640x480]){
            [captureSession setSessionPreset:[NSString stringWithString:AVCaptureSessionPreset640x480]];
            
            mwidth = 480;
            mhight = 288;
        }
        else
            ret = false;
    }
    else
        ret = false;
    
    [captureSession commitConfiguration];
    
    [self RenderInit];
    
    return ret;
}

//switch between front and back camera
- (int)swapFrontAndBackCameras {
    // Assume the session is already running

    NSArray *inputs = captureSession.inputs;
    for ( AVCaptureDeviceInput *input in inputs ) {
        AVCaptureDevice *device = input.device;
        if ( [device hasMediaType:AVMediaTypeVideo] ) {
            AVCaptureDevicePosition position = device.position;
            //AVCaptureDevice *newCamera = nil;
            AVCaptureDeviceInput *newInput = nil;
            
            if (position == AVCaptureDevicePositionFront){
                videoDevice = [self cameraWithPosition:AVCaptureDevicePositionBack];
                cameratype = CAMEAR_BACK;
            }
            else{
                videoDevice = [self cameraWithPosition:AVCaptureDevicePositionFront];
                cameratype = CAMEAR_FRONT;
            }
            newInput = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:nil];
            
            mSwapFlag = 1;
            mSwapCnt = 5;
            
            // beginConfiguration ensures that pending changes are not applied immediately
            [captureSession beginConfiguration];
            [captureSession removeInput:input];
            [captureSession addInput:newInput];
            
            AVCaptureConnection* connection = [vdOutput connectionWithMediaType:AVMediaTypeVideo];
            connection.videoMinFrameDuration = CMTimeMake(1, 15);//15 is fps
            
            [[vdOutput connectionWithMediaType:AVMediaTypeVideo] setVideoOrientation:AVCaptureVideoOrientationPortrait];
            [captureSession commitConfiguration];
          
            break;
        }
    }
    
    return cameratype;
}

- (UIImage*)getfirstpic{
    
    if (VideoEncoder == NULL) {
        return NULL;
    }
    unsigned char* pRGB;
    /*if (mUseBeauty){
        pRGB = VideoEncoder->getRGB(mPixelBuffer);
    }
    else*/
    pRGB= VideoEncoder->getRGB();
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef newContext = CGBitmapContextCreate(pRGB,
                                                    mwidth, mhight, 8,
                                                    mwidth*4,
                                                    colorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGImageRef cgImage = CGBitmapContextCreateImage(newContext);
    
    UIImage* image = [[UIImage alloc]initWithCGImage:cgImage];
    
    [image retain];
    
    CGImageRelease(cgImage);
    
    CGContextRelease(newContext);
    
    CGColorSpaceRelease(colorSpace);
    
    
    return image;
}

-(void) ManualFocus:(GLfloat) x andy:(GLfloat)y
{
    
    if (videoDevice == nil) {
        return;
    }
    float zoomLevel = 2.0;//videoDevice.activeFormat.videoMaxZoomFactor;
    
    if (videoDevice.isFocusPointOfInterestSupported &&[videoDevice isFocusModeSupported:AVCaptureFocusModeAutoFocus]) {
        
        NSError *error = nil;
        //对videoDevice进行操作前，需要先锁定，防止其他线程访问，
        [videoDevice lockForConfiguration:&error];
        [videoDevice setFocusMode:AVCaptureFocusModeAutoFocus];
        [videoDevice setFocusPointOfInterest:CGPointMake(x,y)];
        [videoDevice setVideoZoomFactor:zoomLevel];
        [videoDevice unlockForConfiguration];
    }
}

- (float)GetMaxZoomFactor
{
    if (videoDevice == nil) {
        return 0;
    }
    else{
        return videoDevice.activeFormat.videoMaxZoomFactor;
    }
}

-(void) zoomInOut:(GLfloat) level
{
    if (videoDevice == nil) {
        return ;
    }
    if (level > videoDevice.activeFormat.videoMaxZoomFactor) {
        level = videoDevice.activeFormat.videoMaxZoomFactor;
    }
    NSError *error = nil;
    [videoDevice lockForConfiguration:&error];
    [videoDevice setVideoZoomFactor:level];
    [videoDevice unlockForConfiguration];
}

@end
