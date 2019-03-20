/**
* File : CGKPushProxy.mm
 * Created on : 2015-12-12
* Description : CGKPushProxy
*/

#import "GKPushProxy.h"
#import <AVFoundation/AVFoundation.h>

#include "TTLog.h"
#include <TargetConditionals.h>


#define PRINT_LOG_OPEN  1
#define PRINT_LOG_CLOSE 0

//int g_LogOpenFlag =1;
@implementation CGKMsgObject

@synthesize iMsg;
@synthesize iError0;
@synthesize iError1;

- (id) initwithMsg : (GKPushNotifyMsg) aMsg andError0: (TTInt) aError0 andError1: (TTInt) aError1
{
    iMsg = aMsg;
    iError0 = aError0;
    iError1 = aError1;
    return self;
}
@end

@interface CGKPushProxy()

- (void) notifyProcessProcL : (id) aMsgObject;
- (void) dealloc;

@end

@implementation CGKPushProxy

- (id) init                                                             
{
    self = [super init];
    
    iPush = new CGKPushWrap((__bridge void*)self);
    iCurUrl = NULL;
    return self;
}

- (void)dealloc
{
    SAFE_DELETE(iPush);
    //[super dealloc];
}

- (TTInt) startpublish : (NSString*) aUrl
{
    //iUrlUpdated = false;
    iPush->startpublish([aUrl UTF8String]);
    
    return 0;
}

- (void) setbitrate:(int) bitrate
{
    iPush->setbitrate(bitrate);
}

- (void) stop
{
    iPush->stop();
}

- (void) ProcessNotifyEventWithMsg : (GKPushNotifyMsg) aMsg andError0: (TTInt) aError0 andError1: (TTInt) aError1
{
    CGKMsgObject* pMsgObject = [[CGKMsgObject alloc] initwithMsg : aMsg andError0 : aError0 andError1:aError1];
    [self performSelectorOnMainThread:@selector(notifyProcessProcL:) withObject:pMsgObject waitUntilDone:NO];
}

- (void) notifyProcessProcL : (id) aMsgObject;
{
    CGKMsgObject* pMsg = (CGKMsgObject *)aMsgObject;
    
    if (pMsg != NULL)
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"PushNotifyEvent"     object:pMsg];
    }
}

@end