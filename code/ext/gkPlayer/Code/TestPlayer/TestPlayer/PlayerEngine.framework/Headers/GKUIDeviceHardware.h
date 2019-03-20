/**
 * File : GKUIDeviceHardware.h
 * Created on : 2011-9-7
 * Description : GKUIDeviceHardware∂®“ÂŒƒº˛
 */

#ifndef __GK_UI_DEVICE_HARDWARE_H__
#define __GK_UI_DEVICE_HARDWARE_H__
#import <Foundation/Foundation.h>
@interface GKUIDeviceHardware : NSObject {
@private
    NSString*   iPlatformString;
    Boolean     iIOS4_X;
}
+ (GKUIDeviceHardware*) instance;
- (void) systemInfo;
- (NSString*) platform;
- (Boolean)   IsSystemVersion4X;
- (Boolean)   IsDevice3GS;
- (BOOL) isSystemVersionLargeEqual6;
- (BOOL) isSystemVersion5X;
- (BOOL) isSystemVersionLargeEqual7;
@end
#endif
