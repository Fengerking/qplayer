/**
 * File : VideoCapture.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Inc. All rights reserved.
 * Description : VideoCapture
 */

#import "VideoCapture.h"
#import <AVFoundation/AVFoundation.h>
#include "VideoEncoder.h"
#include "GkCollectCommon.h"
#include <sys/sysctl.h>

#include "TTVideoView.h"
#include "TTGLRenderBase.h"
#include "TTGLRenderES2.h"
#include "TTGLRenderES2_FTU.h"


#include "TTSysTime.h"

#include "YCbCr_tools.h"
#include "beeps_filter.h"
#include "sw_lc.h"

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1

#define clip(x)  ((x) > 255 ? 255 : ( (x) < 0 ? 0 : (x) ))

@interface VideoCapture ()
<AVCaptureVideoDataOutputSampleBufferDelegate>
{
    CGKVideoEncoder* VideoEncoder;
    int mwidth;
    int mhight;
    int mfps ;
    int mbitrate;
    int cameratype;
    int mSwapFlag;
    int mSwapCnt;
    bool mFake640480;
    bool mIphone4S;
    bool mUseBeauty;
    
    TTGLRenderBase*  m_pRender;
    TTVideoBuffer	 mSinkBuffer;
    EAGLContext* mPContext;
    UIView*  mUIView;
    
    unsigned char* mRgb;
    
    bool beginBeauty;
    uint8_t *srcBuf;
    uint8_t *dstBuf;
    //void *ytHandle;
    void *bpHandle;
    void *swHandle;
    //bool mResetEncoder;
    
    float	RGBYUV02990[256],RGBYUV05870[256],RGBYUV01140[256];
    float	RGBYUV01684[256],RGBYUV03316[256];
    float	RGBYUV04187[256],RGBYUV00813[256];
    
    unsigned char*	mU;
    unsigned char*	mV;
    
    CVPixelBufferRef mPixelBuffer;
    bool mBeautySwitch;
    
    TTUint64 m_preSecond ;
    bool m_ajust;
    int m_tick;
    int m_Cnt;
    
    bool mIsStop;
    RGKCritical iCritical;
}
@end

@implementation VideoCapture

- (bool)opencamera
{
    mFake640480 = false;
    self.mIsRunning=false;
    mIsStop = false;
    if (VideoEncoder) {
        delete VideoEncoder;
        VideoEncoder = NULL;
    }
    VideoEncoder = new CGKVideoEncoder();
    mSwapCnt = 0;
    mSwapFlag = 0;
    m_preSecond = 0;
    //mResetEncoder = false;
    
    NSError *error;
    
    captureSession = [[AVCaptureSession alloc] init];
    captureSession.sessionPreset = AVCaptureSessionPresetInputPriority;
    
    [captureSession beginConfiguration];
    
    AVCaptureDevice *videoDevice = NULL;
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
    
    if (mUseBeauty) {
        NSString* key = (NSString*)kCVPixelBufferPixelFormatTypeKey;
        NSNumber* val = [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA];//nv12 uvuv
        NSDictionary* videoSettings =[NSDictionary dictionaryWithObject:val forKey:key];
        
        vdOutput.videoSettings = videoSettings;
    }
    else
    {
        NSString* key = (NSString*)kCVPixelBufferPixelFormatTypeKey;
        NSNumber* val = [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange];//nv12 uvuv
        NSDictionary* videoSettings =[NSDictionary dictionaryWithObject:val forKey:key];
        
        vdOutput.videoSettings = videoSettings;
    }
    
    
    AVCaptureConnection* connection = [vdOutput connectionWithMediaType:AVMediaTypeVideo];
    connection.videoMinFrameDuration = CMTimeMake(1, 15);//15 is fps

    
    if ([connection isVideoOrientationSupported])
        [connection setVideoOrientation:AVCaptureVideoOrientationPortrait];
    
    [captureSession commitConfiguration];
    return true;
    
}

- (void)dealloc
{
    iCritical.Destroy();
    [super dealloc];
}

-(void) rescale_360640:(CMSampleBufferRef)sampleBuffer
{
    
    CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    //CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
    size_t w =  CVPixelBufferGetWidth(imageBuffer);
    size_t h = CVPixelBufferGetHeight(imageBuffer);
    
    unsigned char* addrline1=  (unsigned char*)CVPixelBufferGetBaseAddress(imageBuffer);
    size_t RowByters  = CVPixelBufferGetBytesPerRow(imageBuffer);
    
    unsigned char* addrline2 = addrline1 + RowByters;
    int b,g,r;
    int index = 0;
    if (cameratype == CAMEAR_BACK){
        for (int m = 0; m<h/2; m++) {
            for (int i = 0; i< w/2; i++) {
                b = (addrline1[i<<3] + addrline1[(i<<3)+4]+addrline2[i<<3] + addrline2[(i<<3)+4])>>2;
                g = (addrline1[(i<<3)+1] + addrline1[(i<<3)+5]+addrline2[(i<<3)+1] + addrline2[(i<<3)+5])>>2;
                r = (addrline1[(i<<3)+2] + addrline1[(i<<3)+6]+addrline2[(i<<3)+2] + addrline2[(i<<3)+6])>>2;
                mRgb[index++] = clip(b);
                mRgb[index++] = clip(g);
                mRgb[index++] = clip(r);
                //mRgb[index++] = 0xff;
            }
            
            addrline1 += 2*RowByters;
            addrline2 = addrline1 + RowByters;
        }
    }
    else{
        //RGBÍ¼Ïñ×óÓÒ·­×ª
        int startOffset = 0;
        for (int m = 0; m<h/2; m++) {
            index = w*3/2-1;
            for (int i = 0; i< w/2; i++) {
                b = (addrline1[i<<3] + addrline1[(i<<3)+4]+addrline2[i<<3] + addrline2[(i<<3)+4])>>2;
                g = (addrline1[(i<<3)+1] + addrline1[(i<<3)+5]+addrline2[(i<<3)+1] + addrline2[(i<<3)+5])>>2;
                r = (addrline1[(i<<3)+2] + addrline1[(i<<3)+6]+addrline2[(i<<3)+2] + addrline2[(i<<3)+6])>>2;
                mRgb[startOffset + index - 2] = clip(b);
                mRgb[startOffset + index - 1] = clip(g);
                mRgb[startOffset + index] = clip(r);
                
                index -= 3;
                //mRgb[index++] = 0xff;
            }
            
            startOffset += w*3/2;
            
            addrline1 += 2*RowByters;
            addrline2 = addrline1 + RowByters;
        }
    }
    
    CVPixelBufferUnlockBaseAddress(imageBuffer,0);
}

-(void)InitLookUptable
{
    for (int i=0;i<256;i++)
    {
        RGBYUV02990[i] = (float)0.2990 * i;
        RGBYUV05870[i] = (float)0.5870 * i;
        RGBYUV01140[i] = (float)0.1140 * i;
        RGBYUV01684[i] = (float)0.1684 * i;
        RGBYUV03316[i] = (float)0.3316 * i;
        RGBYUV04187[i] = (float)0.4187 * i;
        RGBYUV00813[i] = (float)0.0813 * i;
    }
}

-(void)RenderAndBeautyInit
{
    if (mUseBeauty && mPContext == NULL) {
        
        mPContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        
        if (nil != mPContext)
        {
            if (NULL == m_pRender) {
                m_pRender = new TTGLRenderES2_FTU(mPContext);
                m_pRender->SetView((UIView*)mUIView);
                m_pRender->init();
                m_pRender->SetTexture(mwidth,mhight);
            }
        }
        
        mRgb = (unsigned char*) malloc(mwidth * mhight * 3);
        
        beginBeauty = false;
        srcBuf = new uint8_t[mwidth * mhight * 4];
        dstBuf = new uint8_t[mwidth * mhight * 4];
        
        //ytHandle = yt_init(h, w);
        bpHandle = beeps_init(mhight, mwidth, 0);
        float baseBeta = 1.1;
        swHandle = sw_lc_init(mhight, mwidth, baseBeta);
        
        [self InitLookUptable];
        
        mU = (unsigned char *)malloc(mwidth * mhight);
        mV = (unsigned char *)malloc(mwidth * mhight);
        
        NSDictionary* pixelBufferOptions = @{ (NSString*) kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange),
                                              (NSString*) kCVPixelBufferWidthKey : @(mwidth),
                                              (NSString*) kCVPixelBufferHeightKey : @(mhight),
                                              (NSString*) kCVPixelBufferOpenGLESCompatibilityKey : @YES,
                                              
                                              (NSString*) kCVPixelBufferIOSurfacePropertiesKey : @{}};
        
        CVPixelBufferCreate(kCFAllocatorDefault, mwidth, mhight, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, (CFDictionaryRef)pixelBufferOptions, &mPixelBuffer);
    }
}

- (id) initWithCameratype:(int) type
 {
    self = [super init];
    
    cameratype = type;
     
     mIphone4S = false;
     mUseBeauty = true;
     mBeautySwitch = true;//cs
     mUIView = nil;
     
     char    deviceString[256];
     size_t  size = 255;
     sysctlbyname("hw.machine", NULL, &size, NULL, 0);
     if (size > 255) { size = 255; }
     sysctlbyname("hw.machine", deviceString, &size, NULL, 0);
     
     if (strcmp(deviceString,"iPhone4,1") == 0){
         mIphone4S = true;
         mUseBeauty = false;
     }
     else if (strcmp(deviceString,"iPhone5,1") == 0 || strcmp(deviceString,"iPhone5,2") == 0){// iphone 5
         mUseBeauty = false;
     }
     else if (strcmp(deviceString,"iPhone5,3") == 0 || strcmp(deviceString,"iPhone5,4") == 0){// iphone 5
         mUseBeauty = false;
     }
     //5.3 5.4 --> 5C   6.1 6.2-->5S   7.1 7.2 -->6
     
     iCritical.Create();
     
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

    if (mUseBeauty) {
        mUIView = previewView;
    }
    else{
        previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:captureSession];
        previewLayer.frame = previewView.bounds;
        previewLayer.contentsGravity = kCAGravityResizeAspectFill;
        previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
        [previewView.layer insertSublayer:previewLayer atIndex:0];
    }
}

- (void)setPush:(void*)obj
{
    VideoEncoder->setPush(obj);
}

- (void)start
{
    VideoEncoder->setvideoparameter(mwidth, mhight, mfps, mbitrate);
    VideoEncoder->set640480(mFake640480);
    VideoEncoder->init();
    [captureSession startRunning];
    //self.mIsRunning= YES;
}

- (void) resetEncoder:(int)framerate andbitrate:(int) bitrate
{
    if (_mIsRunning==true) {
        mfps = framerate;
        mbitrate = bitrate;
        //mResetEncoder = true;
    }
    
    /*if (VideoEncoder != NULL) {
        VideoEncoder->resetEncoder(framerate, bitrate);
    }*/
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

-(void)RGB2YUV:( unsigned char *) rgbData ands :(int)lBufferSize andy:(unsigned char*)yuv andi:(int)padding
{
    int i,j, nr,ng,nb,nSize;
    int nTmp;
    
    unsigned char *y = yuv;
    unsigned char *uv = yuv + (mwidth+padding)*mhight;
    
    if(lBufferSize != mwidth*mhight*3) {
        return ;
    }
    int k = 0;
    for (i = 0; i<=mhight - 1; i++) {
        unsigned char* rgb = rgbData + i*mwidth*3;
        for(j=0, nSize = 0;j<mwidth;j++)
        {
            nb = rgb[nSize];
            ng = rgb[nSize+1];
            nr = rgb[nSize+2];
            nTmp =(int)(RGBYUV02990[nr]+RGBYUV05870[ng]+RGBYUV01140[nb]);
            y[k] = (unsigned char)clip(nTmp);
            nTmp = (int)(-RGBYUV01684[nr]-RGBYUV03316[ng]+nb/2+128);
            mU[k] = (unsigned char)clip(nTmp);
            nTmp = (int)(nr/2-RGBYUV04187[ng]-RGBYUV00813[nb]+128);
            mV[k] = (unsigned char)clip(nTmp);
            
            nSize += 3;
            k++;
        }
    }
    
    //uv
    k = 0;
    for (i=0;i<mhight;i+=2) {
        for(j=0;j<mwidth;j+=2)
        {
            nTmp = (mU[i*mwidth+j]+mU[(i+1)*mwidth+j]+mU[i*mwidth+j+1]+mU[(i+1)*mwidth+j+1])/4;
            uv[k++] = (unsigned char)clip(nTmp);
            nTmp = (mV[i*mwidth+j]+mV[(i+1)*mwidth+j]+mV[i*mwidth+j+1]+mV[(i+1)*mwidth+j+1])/4;
            uv[k++] = (unsigned char)clip(nTmp);
        }
        k += padding;
    }
}

-(void)RGB2YUV_Padding:( unsigned char *) rgbData ands :(int)lBufferSize andy:(unsigned char*)yuv andi:(int)padding
{
    int i,j, nr,ng,nb,nSize;
    int nTmp;
    
    unsigned char *y = yuv;
    unsigned char *uv = yuv + (mwidth+padding)*mhight;
    
    if(lBufferSize != mwidth*mhight*3) {
        return ;
    }
    int k = 0;
    int t = 0;
    for (i = 0; i<=mhight - 1; i++) {
        unsigned char* rgb = rgbData + i*mwidth*3;
        for(j=0, nSize = 0;j<mwidth;j++)
        {
            nb = rgb[nSize];
            ng = rgb[nSize+1];
            nr = rgb[nSize+2];
            nTmp =(int)(RGBYUV02990[nr]+RGBYUV05870[ng]+RGBYUV01140[nb]);
            y[t] = (unsigned char)clip(nTmp);
            nTmp = (int)(-RGBYUV01684[nr]-RGBYUV03316[ng]+nb/2+128);
            mU[k] = (unsigned char)clip(nTmp);
            nTmp = (int)(nr/2-RGBYUV04187[ng]-RGBYUV00813[nb]+128);
            mV[k] = (unsigned char)clip(nTmp);
            
            nSize += 3;
            k++;
            t++;
        }
        t += padding;
    }
    
    //uv
    k = 0;
    for (i=0;i<mhight;i+=2) {
        for(j=0;j<mwidth;j+=2)
        {
            nTmp = (mU[i*mwidth+j]+mU[(i+1)*mwidth+j]+mU[i*mwidth+j+1]+mU[(i+1)*mwidth+j+1])/4;
            uv[k++] = (unsigned char)clip(nTmp);
            nTmp = (mV[i*mwidth+j]+mV[(i+1)*mwidth+j]+mV[i*mwidth+j+1]+mV[(i+1)*mwidth+j+1])/4;
            uv[k++] = (unsigned char)clip(nTmp);
        }
        k += padding;
    }
}


- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    if (true == [self IsDiscardingFrame]) {
        return;
    }
        
    if (mUseBeauty) {
        [self rescale_360640:sampleBuffer];
        
        CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
        unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
        unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
        size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 0);
        
        if (mBeautySwitch) {
            sw_lc_process(mRgb, dstBuf, swHandle);
            //size_t targetRowByter1  = CVPixelBufferGetBytesPerRowOfPlane(mPixelBuffer, 1);
            [self RGB2YUV:dstBuf ands:mwidth * mhight*3 andy:srcBuf andi:(targetRowByter0-mwidth)];
            memcpy(targetUV, srcBuf + targetRowByter0*mhight, targetRowByter0 * mhight/2);
            
            //bgr2yuv(dstBuf, srcBuf, ytHandle);
            beeps_process_padding(srcBuf, dstBuf, 10, targetRowByter0-mwidth,bpHandle);
            memcpy(targetY, dstBuf, targetRowByter0 * mhight);
        }
        else{
            [self RGB2YUV_Padding:mRgb ands:mwidth * mhight*3 andy:srcBuf andi:(targetRowByter0-mwidth)];
            memcpy(targetY, srcBuf, targetRowByter0 * mhight);
            memcpy(targetUV, srcBuf + targetRowByter0*mhight, targetRowByter0 * mhight/2);
        }
        CVPixelBufferUnlockBaseAddress(mPixelBuffer, 0);
        
        mSinkBuffer.Buffer[0] = (TTPBYTE)mPixelBuffer;
        
        iCritical.Lock();
        if (mIsStop == false) {
            m_pRender->RenderYUV(&mSinkBuffer);
        }
        iCritical.UnLock();
       
    }
    
    iCritical.Lock();
    if (self.mIsRunning == 0){
        iCritical.UnLock();
        return;
    }
     iCritical.UnLock();
    /*if (mResetEncoder) {
        mResetEncoder = false;
        if (VideoEncoder != NULL) {
            VideoEncoder->resetEncoder(mfps, mbitrate);
        }
        return;
    }*/
    
    if (mSwapFlag > 0) {
        mSwapCnt--;
        if (mSwapCnt<=0) {
            mSwapCnt = 0;
            mSwapFlag = 0;
        }
        else
            return;
    }
    
    if (mUseBeauty) {
        VideoEncoder->encoder2(mPixelBuffer, ihecHandle, cameratype);
    }
    else
        VideoEncoder->encoder(sampleBuffer, ihecHandle);

}

- (bool)IsDiscardingFrame
{
    bool ret = false;
    TTUint64 currentTime = GetTimeOfDay();
    if (m_preSecond == 0){
        m_preSecond = currentTime;
        m_ajust = false;
    }
    
    long diff = currentTime - m_preSecond;
    m_tick++;
    m_Cnt++;
    if (diff > 1000) {
        m_preSecond = currentTime;
        m_ajust = true;
        m_Cnt = 0;
        
        if (m_tick < 20) {
            m_ajust = false;
        }
        m_tick = 0;
        return false;
    }
    
    if (m_ajust) {
        if (m_Cnt % 2 == 0) {
            ret = false;
        }
        else
            ret = true;
    }
    
    return ret;
}

- (void)stoptransfer
{
    iCritical.Lock();
    self.mIsRunning=false;
    iCritical.UnLock();
}

- (void) stop
{
    iCritical.Lock();
    self.mIsRunning=false;
    mIsStop = true;
    iCritical.UnLock();
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
    
    if (mUseBeauty) {
        beeps_uninit(bpHandle);
        sw_lc_uninit(swHandle);
        SAFE_FREE(mV);
        SAFE_FREE(mU);
        SAFE_DELETE_ARRAY(srcBuf);
        SAFE_DELETE_ARRAY(dstBuf);
        SAFE_DELETE(m_pRender);
        SAFE_FREE(mRgb);
        CVPixelBufferRelease(mPixelBuffer);
    }
    else{
        [previewLayer removeFromSuperlayer];
        [previewLayer release];
        previewLayer = nil;
    }
    
    [captureSession release];
    captureSession = nil;
}


- (void)startEncode
{
    iCritical.Lock();
    self.mIsRunning= true;
    iCritical.UnLock();
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
    
    if (mwidth == 720 && mhight == 1280) {
        mwidth = mwidth/2;
        mhight = mhight/2;
    }
    
    mfps = fps;
    mbitrate = bitrate;
    
    bool ret = true;
    [captureSession beginConfiguration];

    if ((width == 1280 && hight == 720) || (width == 720 && hight == 1280)){
        if (mIphone4S) {
            ret = false;
        }
        else{
            if ([captureSession canSetSessionPreset:AVCaptureSessionPreset1280x720])
                [captureSession setSessionPreset:[NSString stringWithString:AVCaptureSessionPreset1280x720]];
            else
                ret = false;//4S FRONT CAMERA NOT SUPPORT
        }
    }
    else if ((width == 640 && hight == 480) || (width == 480 && hight == 640)){
        if ([captureSession canSetSessionPreset:AVCaptureSessionPreset640x480]){
            [captureSession setSessionPreset:[NSString stringWithString:AVCaptureSessionPreset640x480]];
            mFake640480 = false;
        }
        else
            ret = false;
    }
    else
        ret = false;
    
    if (ret == false && (width == 1280 || width == 720)) {
        //degrade to 640*480, finallly to 640*360
        mFake640480 = true;
        mwidth = 360;
        mhight = 640;
        if ([captureSession canSetSessionPreset:AVCaptureSessionPreset640x480]){
            [captureSession setSessionPreset:[NSString stringWithString:AVCaptureSessionPreset640x480]];
            ret = true;
        }
        else
            ret = false;
    }
    
    [captureSession commitConfiguration];
    
    [self RenderAndBeautyInit];
    
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
            AVCaptureDevice *newCamera = nil;
            AVCaptureDeviceInput *newInput = nil;
            
            if (position == AVCaptureDevicePositionFront){
                newCamera = [self cameraWithPosition:AVCaptureDevicePositionBack];
                cameratype = CAMEAR_BACK;
            }
            else{
                newCamera = [self cameraWithPosition:AVCaptureDevicePositionFront];
                cameratype = CAMEAR_FRONT;
            }
            newInput = [AVCaptureDeviceInput deviceInputWithDevice:newCamera error:nil];
            
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

- (UIImage*)getlastpic{
    
    if (VideoEncoder == NULL) {
        return NULL;
    }
    unsigned char* pRGB = VideoEncoder->getRGB();
    
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

- (void) useBeauty:(bool) bty
{
    mBeautySwitch = bty;
}
@end
