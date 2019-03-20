#include "GKTypedef.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>

@protocol GKPushInterface <NSObject>

@required

- (TTInt) startpublish : (NSString*) aUrl;
- (void) stop;
- (void) setbitrate:(int) bitrate;
@end
