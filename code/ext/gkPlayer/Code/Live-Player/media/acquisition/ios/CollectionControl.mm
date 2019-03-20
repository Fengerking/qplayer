/**
 * File : CollectionControl.mm
 * Created on : 2015-12-12
 * Author : Kevin
 * Copyright : Copyright (c) 2015 GoKu Software Ltd. All rights reserved.
 * Description : CollectionControl
 */

#import <Foundation/Foundation.h>
#import "CollectionControl.h"
#import "VideoCapture.h"
#import "AudioCapture.h"
#import "GKPushProxy.h"
#include "GKCollectCommon.h"
#include "AVTimeStamp.h"

@interface CollectionControl ()
{
    CGKAudioCapture* aCapture;
    VideoCapture *vCapture;
    NSString*      murl;
    CGKPushProxy* proxy;
    int           msamplerate;
    int           mchannel;
    int           mcameratype;
    int           mwith;
    int           mheith;
    int           mfps;
    int           mbitrate;
    bool          mStartLive;
    bool          mPorxyStop;
}


@end

@implementation CollectionControl

- (id)initWithCameratype:(int) type
{
    self = [super init];
  
    CAVTimeStamp::reset();
    vCapture= [[VideoCapture alloc] initWithCameratype:type];
    aCapture = new CGKAudioCapture();
    proxy= [[CGKPushProxy alloc] init];
    mcameratype = type;
    
    proxy->iPush->settype(MEDIATYPE_LIVE);
    
    aCapture->setPush(proxy->iPush);
    [vCapture setPush:proxy->iPush];
    
    mStartLive = false;
    mPorxyStop = false;
    
    return self;
}

- (void)setPreviewView:(UIView *)previewView
{
    [vCapture setPreviewView:previewView];
}

-(void)startcapture
{
    aCapture->start();
    [vCapture start];
}

-(void)closecamera
{
    if (vCapture != nil) {
        [vCapture stop];
    }
   
    if (mPorxyStop) {
        if (proxy != nil) {
            //[proxy dealloc];
            [proxy release];
            proxy = nil;
        }
        mPorxyStop = false;
    }
    if (vCapture != nil) {
        [vCapture release];
        vCapture = nil;
    }
    
    if (aCapture != NULL) {
        aCapture->stop();
        delete aCapture;
        aCapture = NULL;
    }
}

-(void)stoppush
{
    if (mStartLive) {
        if (aCapture != NULL) {
            aCapture->stop();
            delete aCapture;
            aCapture = NULL;
        }
        if (proxy != nil) {
            
            [proxy stop];
            
            if (vCapture == nil) {
                //[proxy dealloc];
                [proxy release];
                proxy = nil;
            }
        }
        mPorxyStop = true;
        mStartLive = false;
    }
    
    if (vCapture != nil) {
        [vCapture stoptransfer];
    }
}

- (void) setaudioparameter: (int)samplerate andch: (int)channel
{
    msamplerate = samplerate;
    mchannel = channel;
    aCapture->setaudioparameter(samplerate,channel);
}

- (bool) setvideoparameter: (int)width andhi:(int)hight andfps:(int)fps andbitrate:(int) bitrate
{
    mwith = width;
    mheith = hight;
    mfps = 15;
    mbitrate = bitrate;
     return [vCapture setvideoparameter:width andhi:hight andfps:fps andbitrate:bitrate];
}

- (void) setpushlishurl:(NSString*)aurl
{
    murl = aurl;
    //[proxy startpublish: aurl];
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
    CAVTimeStamp::reset();
    if (vCapture == nil) {
        vCapture= [[VideoCapture alloc] initWithCameratype:mcameratype];
        [vCapture setPreviewView:previewView];
        [vCapture setvideoparameter:mwith andhi:mheith andfps:mfps andbitrate:mbitrate];
    }
    else{
        [vCapture opencamera];
        [vCapture setPreviewView:previewView];
        [vCapture setvideoparameter:mwith andhi:mheith andfps:mfps andbitrate:mbitrate];
    }
    
    if (proxy == nil) {
        proxy= [[CGKPushProxy alloc] init];
        proxy->iPush->settype(MEDIATYPE_LIVE);
    }
    mPorxyStop = false;
    
    if (aCapture == NULL){
        aCapture = new CGKAudioCapture();
        aCapture->setaudioparameter(msamplerate,mchannel);
        aCapture->setPush(proxy->iPush);
        aCapture->start();
    }
    
    //[vCapture setPreviewView:previewView];
    [vCapture setPush:proxy->iPush];
    [vCapture start];
}


- (void) startlive
{
    [proxy startpublish: murl];
    [vCapture startEncode];
    aCapture->startEncoder();
    
    mStartLive = true;
}

- (void) resetEncoder:(int)framerate andbitrate:(int) bitrate
{
    [vCapture resetEncoder:framerate andbitrate:bitrate];
}

- (UIImage*)getlastpic
{
    if (vCapture != nil) {
        return [vCapture getlastpic];
    }
    return nil;
}

- (void) useBeauty:(bool) bty
{
    if (vCapture != nil) {
        return [vCapture useBeauty:bty];
    }
}

- (void) netdisconnect
{
    if (mStartLive) {
        if (proxy && proxy->iPush) {
            proxy->iPush->netdisconnect();
        }
    }
}

@end

