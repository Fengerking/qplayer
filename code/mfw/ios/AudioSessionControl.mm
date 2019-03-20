/*******************************************************************************
	File:		AudioSessionControl.m
 
	Contains:	Audio session controller implement file.
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-14		Jun Lin			Create file
 
 ******************************************************************************/

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

#include "ULogFunc.h"
#import "AudioSessionControl.h"

// For iOS < 6.0
void audioSessionInterruptionListener(void* userData, UInt32 interruptionState)
{
    AudioSessionControl* asc = (__bridge AudioSessionControl*)userData;
    
    [asc onASInterrupt:interruptionState];
}


@interface AudioSessionControl()
{
    int  m_nRefCount;
    char m_szObjName[32];
    //BOOL m_available;
}

// for iOS lower than 6.0
-(void) onASInterrupt:(int)interruptionState;

// for iOS higher than 6.0
-(void) handleInterruption:(NSNotification *)notification;
-(void) handleRouteChange:(NSNotification *)notification;
-(void) handleMediaServicesWereReset:(NSNotification *)notification;

@end

@implementation AudioSessionControl

-(bool) enableAudioSession:(BOOL)enable force:(BOOL)force
{
    if(enable)
        m_nRefCount++;
    else
        m_nRefCount--;
    
    QCLOGI("[AU]Config audio session: %s, count %d", enable?"ENABLE":"DISABLE", m_nRefCount);
    
    if(force)
    {
        if(enable == NO && m_nRefCount > 0)
            return true;
        
        NSError *err = nil;
        AVAudioSession* as = [AVAudioSession sharedInstance];
        
        if(NO == [as setActive:enable error:&err])
        {
            if(enable)
                m_nRefCount--;
            else
                m_nRefCount++;
            QCLOGW("[AU]%p setActive error : %d, %d, ref %d", self, (int)err.code, enable, m_nRefCount);
            return false;
        }
        
        if(YES == enable)
        {
            if(NO == [as setCategory:AVAudioSessionCategoryPlayback error:&err])
            {
                if(enable)
                    m_nRefCount--;
                else
                    m_nRefCount++;
                QCLOGW("[AU]%p setCategory error : %d, %d, ref %d", self, (int)err.code, enable, m_nRefCount);
                return false;
            }
        }
    }
    
    return true;
}

-(void) setEventCB:(ASL*)cb
{
    memcpy(&_asl, cb, sizeof(ASL));
}

-(int)	getRefCount
{
    QCLOGW("[AU]AS ref count %d", m_nRefCount);
    return m_nRefCount;
}

//-(BOOL) isAudioSessionAvailable
//{
//    return m_available;
//}

-(void) uninit
{
    memset(&_asl, 0, sizeof(ASL));
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                 name:AVAudioSessionInterruptionNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVAudioSessionRouteChangeNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVAudioSessionMediaServicesWereResetNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationDidBecomeActiveNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationWillResignActiveNotification
                                                  object:nil];
}

- (id) init
{
    self = [super init];
    
    if (nil != self)
    {
        //m_available = true;
        memset(&_asl, 0, sizeof(ASL));
        m_nRefCount = 0;
        memset(m_szObjName, 0, 32);
        strcpy(m_szObjName, "AudioSessionControl");
        
        float osVersion = [[UIDevice currentDevice].systemVersion floatValue];
     
        if (osVersion >=6.0)
        {
            AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];
            
            // add the interruption handler
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleInterruption:)
                                                         name:AVAudioSessionInterruptionNotification
                                                       object:sessionInstance];
            
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleRouteChange:)
                                                         name:AVAudioSessionRouteChangeNotification
                                                       object:sessionInstance];
            
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleMediaServicesWereReset:)
                                                         name:AVAudioSessionMediaServicesWereResetNotification
                                                       object:sessionInstance];
            
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];

            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
        }
        else // < 6.0
        {
            // Only init once
//            OSStatus result = AudioSessionInitialize(NULL, NULL, audioSessionInterruptionListener, self);
//
//            if (result == kAudioSessionNoError)
//            {
//                UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
//                AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
//            }
        }
    }
    
    //[AudioSessionControl setAudioMix:FALSE];
    
    return self;
}

+(void)setAudioMix:(BOOL)bMix
{
    float osVersion = [[UIDevice currentDevice].systemVersion floatValue];
    if (osVersion >=6.0)
    {
        if (bMix)
        {
            [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
        }
        else
        {
            [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:0 error:nil];
        }
    }
    else
    {
        if (bMix)
        {
            //QCLOGE("Don't support mix below 6.0");
        }
        else
        {
            UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
            AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
        }
    }
}

-(void) handleRouteChange:(NSNotification *)notification
{
    UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
    
    AVAudioSessionRouteDescription* routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
    
    switch (reasonValue) {
        case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
        QCLOGI("NewDeviceAvailable");
        break;
        case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
        QCLOGI("OldDeviceUnavailable");
        break;
        case AVAudioSessionRouteChangeReasonCategoryChange:
        QCLOGI("CategoryChange");
        break;
        case AVAudioSessionRouteChangeReasonOverride:
        QCLOGI("Override");
        break;
        case AVAudioSessionRouteChangeReasonWakeFromSleep:
        QCLOGI("WakeFromSleep");
        break;
        case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
        QCLOGI("NoSuitableRouteForCategory");
        break;
        default:
        QCLOGI("ReasonUnknown");
    }
    
    NSString *str = [NSString stringWithFormat:@"%@", routeDescription];
    //QCLOGI("routeDescription: %s", [str UTF8String]);
}

-(void) handleInterruption:(NSNotification *)notification
{
    UInt8 theInterruptionType = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
    
    QCLOGI("Session interrupted! %s", theInterruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
    
    int ID = ASE_INTERRUPTION_END;
    
    if (theInterruptionType == AVAudioSessionInterruptionTypeBegan)
    {
        //m_available = false;
        ID = ASE_INTERRUPTION_BEGIN;
    }
    else if (theInterruptionType == AVAudioSessionInterruptionTypeEnded)
    {
        //m_available = true;
        ID = ASE_INTERRUPTION_END;
    }
    
    if (NULL != _asl.listener)
    {
        _asl.listener(_asl.userData, ID, 0, 0);
    }
}

-(void)handleMediaServicesWereReset:(NSNotification *)notification
{
    QCLOGI("Session handleMediaServicesWereReset");
    
    if (NULL != _asl.listener)
    {
        _asl.listener(_asl.userData, ASE_MEDIA_SERVICE_RESET, 0, 0);
    }
}

-(void) onASInterrupt:(int)interruptionState
{
    int ID = ASE_INTERRUPTION_END;
    
    if (kAudioSessionBeginInterruption == interruptionState)
    {
        //m_available = false;
        ID = ASE_INTERRUPTION_BEGIN;
    }
    else
    {
        //m_available = true;
        ID = ASE_INTERRUPTION_END;
    }
    
    if (NULL != _asl.listener)
    {
        _asl.listener(_asl.userData, ID, 0, 0);
    }
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
}

- (void)applicationBecomeActive:(NSNotification *)notification
{
    if (NULL != _asl.listener)
    {
        _asl.listener(_asl.userData, ASE_APP_RESUME, 0, 0);
    }
}


@end
