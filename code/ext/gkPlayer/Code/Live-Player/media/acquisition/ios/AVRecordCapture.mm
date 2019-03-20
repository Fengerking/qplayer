/**
 * File : AVRecordCapture.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : AVRecordCapture
 */

#import "AVRecordCapture.h"
#import <AVFoundation/AVFoundation.h>
#include "VideoEncoder.h"
#include <sys/sysctl.h>

#include "TTVideoView.h"
#include "TTGLRenderBase.h"
#include "TTGLRenderES2.h"
#include "TTGLRenderES2_FTU.h"
#include "GKCollectCommon.h"
#include "AVTimeStamp.h"

#define CAMEAR_FRONT 0
#define CAMEAR_BACK  1

#define clip(x)  ((x) > 255 ? 255 : ( (x) < 0 ? 0 : (x) ))
//AVAssetWriter AVFileTypeMPEG4
@interface AVRecordCapture ()
<AVCaptureVideoDataOutputSampleBufferDelegate, AVCaptureAudioDataOutputSampleBufferDelegate>
{
    int mwidth;
    int mhight;
    int cameratype;
    int mSwapFlag;
    int mSwapCnt;
    
    TTGLRenderBase*  m_pRender;
    TTVideoBuffer	 mSinkBuffer;
    EAGLContext* mPContext;
    UIView*  mUIView;
    
    CVPixelBufferRef mPixelBuffer;
    CVPixelBufferRef mFrontBuffer;
    CVPixelBufferRef mfirstBuffer;
    
    AVCaptureDevice *videoDevice;
    float   mfactor;
    bool    mPreivew;
    int     mStatus;
    
    CMItemCount mcount;
    CMSampleBufferRef msout;
    CMTime mnewTimeStamp;
    CMSampleTimingInfo* mpInfo;
    
    NSString* mPath;
    bool    mFirstPicSet;
    unsigned char*  mfirstRGB ;
    long  mRecordTime;
}
@end

@implementation AVRecordCapture

- (void) setfilepath:(NSString*)apath
{
    mPath = apath;
}

- (bool)opencamera
{
    self.mIsRunning=false;
  
    mSwapCnt = 0;
    mSwapFlag = 0;
    mPreivew = true;
    mStatus = 0;
    mFirstPicSet = false;
    mRecordTime = 0;
    StopRecord = false;
    
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
    
    
    AVCaptureDevice * audioDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
    
    AVCaptureDeviceInput *audioInput= [AVCaptureDeviceInput deviceInputWithDevice:audioDevice error:&error];
    
    [captureSession addInput:audioInput];
    
    audioOut = [[AVCaptureAudioDataOutput alloc] init];
    
    [captureSession addOutput:audioOut];
    
    dispatch_queue_t AcaptureQueue = dispatch_queue_create("AcaptureQueue", NULL);
    [audioOut setSampleBufferDelegate:self queue:AcaptureQueue];
    
    
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
                m_pRender->init();
                m_pRender->SetTexture(mwidth,mhight);
            }
        }
        
        CVPixelBufferCreate(NULL, mwidth, mhight, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &mPixelBuffer);
        CVPixelBufferCreate(NULL, mwidth, mhight, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &mFrontBuffer);
        CVPixelBufferCreate(NULL, mwidth, mhight, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, &mfirstBuffer);
        
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

- (void)start
{
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

- (void)revert:(CVPixelBufferRef) PixelBuffer
{
    CVPixelBufferLockBaseAddress(PixelBuffer,0);
    
    CVPixelBufferLockBaseAddress(mPixelBuffer,0);
    
    unsigned char* pY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 0);
    unsigned char* pVUSrc = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(PixelBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(PixelBuffer, 0);
    
    unsigned char* pY1=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* pVUSrc1 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    memcpy(pY, pY1, targetRowByter0*mhight);
    memcpy(pVUSrc, pVUSrc1, targetRowByter0*mhight/2);
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
    
    int i = 0;
    int j = 0;
    unsigned char tmp = 0;
    unsigned char tmp1 = 0;
    for(j = 0; j < mhight; j++){
        for(i = 0; i < mwidth/2; i++) {
            tmp = pY[i];
            pY[i] = pY[mwidth-1-i];
            pY[mwidth-1-i] = tmp;
        }
        pY +=targetRowByter0;
    }
    
    //revert uv
    for(j = 0; j < mhight/2; j++){
        for(i = 0; i < mwidth/2; i+=2) {
            tmp = pVUSrc[i];
            tmp1 = pVUSrc[i+1];
            pVUSrc[i] = pVUSrc[mwidth-1-i-1];
            pVUSrc[i + 1] = pVUSrc[mwidth-1-i];
            pVUSrc[mwidth-1-i] = tmp1;
            pVUSrc[mwidth-1-i-1] = tmp;
        }
        pVUSrc +=targetRowByter0;
    }
    
    CVPixelBufferUnlockBaseAddress(PixelBuffer,0);
}

-(void)SaveFirstPic
{
    CVPixelBufferLockBaseAddress(mfirstBuffer,0);
    CVPixelBufferLockBaseAddress(mPixelBuffer,0);
    
    unsigned char* pY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mfirstBuffer, 0);
    unsigned char* pVUSrc = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mfirstBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mfirstBuffer, 0);
    
    unsigned char* pY1=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 0);
    unsigned char* pVUSrc1 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mPixelBuffer, 1);
    memcpy(pY, pY1, targetRowByter0*mhight);
    memcpy(pVUSrc, pVUSrc1, targetRowByter0*mhight/2);
    
    CVPixelBufferUnlockBaseAddress(mfirstBuffer,0);
    CVPixelBufferUnlockBaseAddress(mPixelBuffer,0);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    if(StopRecord == true)
        return;
    
    if (captureOutput == vdOutput) {
        
        [self rescale_to_480288:sampleBuffer];
        mSinkBuffer.Buffer[0] = (TTPBYTE)mPixelBuffer;
        
        if (cameratype == CAMEAR_FRONT) {
            mSinkBuffer.Buffer[0] = (TTPBYTE) mFrontBuffer;
            [self revert:mFrontBuffer];
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
        
        if( videoWriter.status > AVAssetWriterStatusWriting )
        {
            NSLog(@"Warning: writer status is %ld", (long)videoWriter.status);
            if( videoWriter.status == AVAssetWriterStatusFailed )
                NSLog(@"Error: %@", videoWriter.error);
            
            return;
        }
        
        if ([videoWriterInput isReadyForMoreMediaData]){
            mRecordTime = CAVTimeStamp::getCurrentTime();
            [adaptor appendPixelBuffer:mPixelBuffer withPresentationTime:CMTimeMake(CAVTimeStamp::getCurrentTime(), 1000)];
            
            if (mFirstPicSet == false) {
                mFirstPicSet = true;
                
                [self SaveFirstPic];
            }
        }
    }
    else if (captureOutput == audioOut){
        
        if (self.mIsRunning == 0)
            return;
        
        if( videoWriter.status > AVAssetWriterStatusWriting )
        {
            NSLog(@"Warning: writer status is %ld", (long)videoWriter.status);
            if( videoWriter.status == AVAssetWriterStatusFailed )
                NSLog(@"Error: %@", videoWriter.error);
            
            return;
        }
        if ([audioWriterInput isReadyForMoreMediaData])
        {
            mRecordTime = CAVTimeStamp::getCurrentTime();
            mnewTimeStamp = CMTimeMake(CAVTimeStamp::getCurrentTime(), 1000);
            CMSampleBufferGetSampleTimingInfoArray(sampleBuffer, 0, nil, &mcount);
            mpInfo = (CMSampleTimingInfo*)malloc(sizeof(CMSampleTimingInfo) * mcount);
            CMSampleBufferGetSampleTimingInfoArray(sampleBuffer, mcount, mpInfo, &mcount);
            for (CMItemCount i = 0; i < mcount; i++)
            {
                mpInfo[i].decodeTimeStamp = mnewTimeStamp; // kCMTimeInvalid if in sequence
                mpInfo[i].presentationTimeStamp = mnewTimeStamp;
            }
            
            CMSampleBufferCreateCopyWithNewTiming(kCFAllocatorDefault, sampleBuffer, mcount, mpInfo, &msout);
            free(mpInfo);
            mpInfo = NULL;
            
            if( ![audioWriterInput appendSampleBuffer:msout] )
                NSLog(@"Unable to write to audio input");
            
            CFRelease(msout);
        }
    }
}

-(long) getRecordTime
{
    return mRecordTime;
}

-(void) initVideoAudioWriter

{
    
    CGSize size = CGSizeMake(480, 288);
    NSError *error = nil;
    
    BOOL blHave=[[NSFileManager defaultManager] fileExistsAtPath:mPath];
    if (blHave) {
         [[NSFileManager defaultManager] removeItemAtPath:mPath error:nil];
         NSLog(@" delete exist file");
    }

    //----initialize compression engine
    NSURL   *fileUrl=[NSURL fileURLWithPath:mPath];
    
    videoWriter = [[AVAssetWriter alloc] initWithURL:fileUrl
                                                 fileType:AVFileTypeMPEG4
                                                 error:&error];
    
    NSParameterAssert(videoWriter);
    
    if(error)
        NSLog(@"error = %@", [error localizedDescription]);
    

   /*  AVVideoMaxKeyFrameIntervalKey: [NSNumber numberWithInt: 30] };*/
    
    NSDictionary *videoCompressionProps = [NSDictionary dictionaryWithObjectsAndKeys:
                                           [NSNumber numberWithDouble:1000000],AVVideoAverageBitRateKey,
                                           AVVideoProfileLevelH264Main31,AVVideoProfileLevelKey,
                                           nil ];
    
    NSDictionary *videoSettings = [NSDictionary dictionaryWithObjectsAndKeys:AVVideoCodecH264, AVVideoCodecKey,
                                   [NSNumber numberWithInt:size.width], AVVideoWidthKey,
                                   [NSNumber numberWithInt:size.height],AVVideoHeightKey,
                                   videoCompressionProps,AVVideoCompressionPropertiesKey
                                   ,nil];
    
    videoWriterInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
    NSParameterAssert(videoWriterInput);
    
    videoWriterInput.expectsMediaDataInRealTime = YES;
    
    NSDictionary *sourcePixelBufferAttributesDictionary = [NSDictionary dictionaryWithObjectsAndKeys:                                                           [NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange], kCVPixelBufferPixelFormatTypeKey, nil];
    
    adaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:videoWriterInput
                                               sourcePixelBufferAttributes:sourcePixelBufferAttributesDictionary];
    [adaptor retain];
    NSParameterAssert(videoWriterInput);
    
    NSParameterAssert([videoWriter canAddInput:videoWriterInput]);
   
    if ([videoWriter canAddInput:videoWriterInput])
        NSLog(@"videoWriterInput can add");
    else
        NSLog(@"videoWriterInput add no");
    
    // Add the audio input
    AudioChannelLayout acl;
    bzero( &acl, sizeof(acl));
    acl.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
    
    NSDictionary* audioOutputSettings = nil;

    audioOutputSettings = [ NSDictionary dictionaryWithObjectsAndKeys:
                           [ NSNumber numberWithInt: kAudioFormatMPEG4AAC ], AVFormatIDKey,
                           [ NSNumber numberWithInt:64000], AVEncoderBitRateKey,
                           [ NSNumber numberWithFloat: 44100.0 ], AVSampleRateKey,
                           [ NSNumber numberWithInt: 2 ], AVNumberOfChannelsKey,
                           [ NSData dataWithBytes: &acl length: sizeof( acl ) ], AVChannelLayoutKey,
                           nil ];  
    
    audioWriterInput = [[AVAssetWriterInput
                         assetWriterInputWithMediaType: AVMediaTypeAudio
                         outputSettings: audioOutputSettings ] retain];  

    audioWriterInput.expectsMediaDataInRealTime = YES;  
    
    // add input
    [videoWriter addInput:audioWriterInput];
    [videoWriter addInput:videoWriterInput];
    
    [videoWriter startWriting];
    [videoWriter startSessionAtSourceTime:CMTimeMake(0, 1000)];
    
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

- (void)pause
{
    self.mIsRunning=false;
}
- (void)resume
{
    self.mIsRunning=true;
}

- (int) stop
{
    StopRecord = true;
    self.mIsRunning=false;
     mPreivew = false;
    [captureSession stopRunning];
    
    if (videoWriter != nil) {
        [videoWriter finishWritingWithCompletionHandler:^{
            if (videoWriter.status != AVAssetWriterStatusFailed && videoWriter.status == AVAssetWriterStatusCompleted) {
                //[videoWriterInput markAsFinished];
                //[audioWriterInput markAsFinished];
                videoWriterInput= nil;
                audioWriterInput = nil;
                [videoWriter release];
                videoWriter = nil;
                [adaptor release];
                adaptor = nil;
            }
            
        }];
    }
    
    for(AVCaptureInput *input1 in captureSession.inputs) {
        [captureSession removeInput:input1];
    }
    
    for(AVCaptureOutput *output1 in captureSession.outputs) {
        [captureSession removeOutput:output1];
    }
    
    [vdOutput release];
    vdOutput = nil;
    
    [audioOut release];
    audioOut = nil;
 
    SAFE_DELETE(m_pRender);
    CVPixelBufferRelease(mPixelBuffer);
    CVPixelBufferRelease(mFrontBuffer);
    
   
    [captureSession release];
    captureSession = nil;
    
    int size = 0;
    BOOL blHave=[[NSFileManager defaultManager] fileExistsAtPath:mPath];
    if (blHave) {
        size =  [[[NSFileManager defaultManager]  attributesOfItemAtPath:mPath error:nil] fileSize];
        NSLog(@"file size %d",size);
    }
    return size;
}

-(void) resetRecord
{
    //StopRecord = true;
    self.mIsRunning=false;
    //[captureSession stopRunning];
    if (videoWriter != nil) {
    [videoWriter finishWritingWithCompletionHandler:^{
        if (videoWriter.status != AVAssetWriterStatusFailed && videoWriter.status == AVAssetWriterStatusCompleted) {
            videoWriterInput= nil;
            audioWriterInput = nil;
            [videoWriter release];
            videoWriter = nil;
            [adaptor release];
            adaptor = nil;
            
            BOOL blHave=[[NSFileManager defaultManager] fileExistsAtPath:mPath];
            if (blHave) {
                [[NSFileManager defaultManager] removeItemAtPath:mPath error:nil];
                NSLog(@" delete exist file");
            }
            
            //[captureSession startRunning];
            
        }
    }];
    }
}

- (void)startEncode
{
    self.mIsRunning = true;
    mPreivew = true;
    
    if (videoWriter == nil) {
        [self initVideoAudioWriter];
    }
}

- (bool) setvideoparameter: (int)width andhi:(int)hight
{
    if (width > hight) {
        mwidth = hight;
        mhight = width;
    }
    else{
        mwidth = width;
        mhight = hight;
    }
    
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
    
    [self initVideoAudioWriter];
    
    if (mfirstRGB) {
        free(mfirstRGB);
    }
    mfirstRGB = (unsigned char* )malloc(mwidth*mhight*4);
    
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


-(void)NV12RGB:(unsigned char *)pYuvBuf andpb:(unsigned char *)pRgbBuf andw:(int)nWidth andh:(int)nHeight andpd:(int) padding
{
    unsigned char *yBuf, *uBuf;
    int i, j, m, n, x, y, py, rdif, invgdif, bdif;
    char addhalf = 1;
    m = -nWidth;
    
    unsigned char *pRgbline = pRgbBuf;
    
    yBuf = pYuvBuf;
    
    n = -nWidth;
    uBuf = pYuvBuf + nWidth * nHeight;
    for(y=0; y<nHeight;y++)
    {
        m += nWidth;
        if(addhalf)
        {
            n+=nWidth;
            addhalf = 0;
        }
        else
        {
            addhalf = 1;
        }
        for(x=0; x<nWidth;x++)
        {
            i = m + x;
            j = n + ((x>>1) << 1);
            py = yBuf[i];
            // search tables to get rdif invgdif and bidif
            rdif = Table_FV1[uBuf[j +1]];    // fv1
            invgdif = Table_FU1[uBuf[j]] + Table_FV2[uBuf[j+1]]; // fu1+fv2
            bdif = Table_FU2[uBuf[j]]; // fu2
            
            i =  4*x;
            
            pRgbline[i] = clip(py+bdif);
            pRgbline[i+1] = clip(py-invgdif);
            pRgbline[i+2] = clip(py+rdif);
            pRgbline[i+3] = 0xff;
        }
        pRgbline += padding;
    }
}


-(unsigned char*)getRGB
{
    unsigned char* pFrameData = new unsigned char[mwidth * mhight * 3 / 2];
    
    if (pFrameData == NULL || mfirstRGB == NULL) {
        return NULL;
    }
    
    CVPixelBufferLockBaseAddress(mfirstBuffer, 0);
    
    unsigned char* targetY=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mfirstBuffer, 0);
    unsigned char* targetUV=  (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(mfirstBuffer, 1);
    size_t targetRowByter0  = CVPixelBufferGetBytesPerRowOfPlane(mfirstBuffer, 0);
    size_t targetRowByter1 = CVPixelBufferGetBytesPerRowOfPlane(mfirstBuffer, 1);
    
    if (targetRowByter0 == mwidth )
    {
        memcpy(pFrameData, targetY, mwidth * mhight*3/2);
    }
    else{
        //padding handle
        unsigned char* yuvpalne = pFrameData;
        for (int i= 0; i< mhight; i++) {
            memcpy(yuvpalne, targetY, mwidth);
            yuvpalne += mwidth;
            targetY += targetRowByter0;
        }
        
        //yuvpalne = m_pFrameData + m_nTextureWidth*m_nTextureHeight;
        for (int i= 0; i<mhight/2; i++) {
            memcpy(yuvpalne, targetUV, mwidth);
            yuvpalne += mwidth;
            targetUV += targetRowByter1;
        }
    }
    
    CVPixelBufferUnlockBaseAddress(mfirstBuffer,0);
    
    [self NV12RGB:pFrameData andpb:mfirstRGB andw:mwidth andh:mhight andpd:mwidth*4];
    
    free(pFrameData);
    
    return mfirstRGB;
}

- (UIImage*)getfirstpic{
    
    unsigned char* pRGB = [self getRGB];
    
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
