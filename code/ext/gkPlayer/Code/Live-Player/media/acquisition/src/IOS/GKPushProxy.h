/**
* File : GKPushProxy.h
 * Created on : 2015-12-12
* Description : CGKPushProxy
*/
#import <Foundation/Foundation.h>

#import "GKPushWrap.h"
#include "GKNotifyEnumDef.h"
#import "GKPushInterface.h"

@interface CGKMsgObject : NSObject {

}
@property (readonly) GKPushNotifyMsg iMsg;
@property (readonly) TTInt       iError0;
@property (readonly) TTInt       iError1;
- (id) initwithMsg: (GKPushNotifyMsg) aMsg andError0: (TTInt) aError0 andError1: (TTInt) aError1;
@end

@interface CGKPushProxy : NSObject <GKPushInterface> {

@public
    CGKPushWrap*     iPush;
@private
    NSString*         iCurUrl;
}

- (void) ProcessNotifyEventWithMsg : (GKPushNotifyMsg) aMsg andError0: (TTInt) aError0 andError1: (TTInt) aError1;

@end
