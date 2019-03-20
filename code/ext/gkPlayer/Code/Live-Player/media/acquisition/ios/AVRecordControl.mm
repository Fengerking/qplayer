/**
 * File : CTTRecordControl.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : CTTRecordControl
 */

#import <Foundation/Foundation.h>
#import "AVRecordControl.h"
#import "AVRecordCapture.h"
#import "AudioCapture.h"
#import "GKPushProxy.h"
#include "AVTimeStamp.h"

#include "GkCollectCommon.h"

@interface CAVRecordControl ()
{
    AVRecordCapture *vCapture;
    NSString*      mPath;
    int           mcameratype;
    int           mwith;
    int           mheith;
}


@end

@implementation CAVRecordControl

- (id)initWithCameratype:(int) type
{
    self = [super init];

    vCapture= [[AVRecordCapture alloc] initWithCameratype:type];
    mcameratype = type;
    
    return self;
}

- (void)setPreviewView:(UIView *)previewView
{
    [vCapture setPreviewView:previewView];
}

- (bool) setvideoparameter: (int)width andhi:(int)hight
{
    mwith = width;
    mheith = hight;
    
    if (vCapture != nil) {
        [vCapture setfilepath:mPath];
    }
    
     return [vCapture setvideoparameter:width andhi:hight];
}

- (void) setfilepath:(NSString*)apath
{
    mPath = apath;
    
    if (vCapture != nil) {
         [vCapture setfilepath:mPath];
    }
   
}

- (void) setCameraType:(int)type
{
    [vCapture setCameraType:type];
}

- (void) swap
{
    mcameratype = [vCapture swapFrontAndBackCameras];
}

- (void) opencamera: (UIView *)previewView
{
    if (vCapture == nil) {
        vCapture= [[AVRecordCapture alloc] initWithCameratype:mcameratype];
        [vCapture setPreviewView:previewView];
        [vCapture setfilepath:mPath];
        [vCapture setvideoparameter:mwith andhi:mheith];
    }
    else{
        [vCapture opencamera];
        [vCapture setPreviewView:previewView];
        [vCapture setfilepath:mPath];
        [vCapture setvideoparameter:mwith andhi:mheith];
    }
    
    //[vCapture setPreviewView:previewView];
    [vCapture start];
}

-(void)startcapture
{
    //aCapture->start();
    [vCapture start];
}

-(int)stopRecord
{
    int size = 0;
    if (vCapture != nil) {
        size = [vCapture stop];
        [vCapture release];
        vCapture = nil;
    }
    return size;
}

- (void) resetRecord{
    if (vCapture != nil) {
        [vCapture resetRecord];
    }
}

- (void) startRecord
{
    CAVTimeStamp::reset();
    [vCapture startEncode];
}

- (UIImage*)getfirstpic
{
    if (vCapture != nil) {
        return [vCapture getfirstpic];
    }
    return nil;
}

- (void) recordpause
{
    if (vCapture != nil){
        [vCapture pause];
    }
    CAVTimeStamp::pause();
}

- (void) recordresume
{
    CAVTimeStamp::resume();
    
    if (vCapture != nil){
        [vCapture resume];
    }
}

-(void) manualFocus:(GLfloat) x andy:(GLfloat)y
{
    if (vCapture != nil){
        [vCapture ManualFocus:x andy:y];
    }
}

-(void) zoomInOut:(GLfloat) level
{
    if (vCapture != nil){
        [vCapture zoomInOut:level];
    }
}

- (float)GetMaxZoomFactor
{
    if (vCapture != nil){
        return [vCapture GetMaxZoomFactor];
    }
    else
        return 1.0;
}

-(long) getRecordTime
{
    if (vCapture != nil){
        return [vCapture getRecordTime];
    }
        return 0;
}

-(long) getPeriodTime
{
    return CAVTimeStamp::getPeriodTime();
}

@end

