/*******************************************************************************
	File:		CNetworkSta.h
 
	Contains:	Stataus detect class head code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-06-14		Jun			Create file
 
 *******************************************************************************/

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

typedef void (* NetworkConnectionChangedEvt) (void* pUserData, int nConnectionStatus);

typedef enum : NSInteger {
	NotReachable = 0,
	ReachableViaWiFi,
	ReachableViaWWAN
} NetworkStatus;

@interface CNetworkSta : NSObject

/*!
 * Use to check the detect of a given host name.
 */
+ (instancetype)networkStaWithHostName:(NSString *)hostName;

/*!
 * Use to check the detect of a given IP address.
 */
+ (instancetype)networkStaWithAddress:(const struct sockaddr *)hostAddress;

/*!
 * Checks whether the default route is available. Should be used by applications that do not connect to a particular host.
 */
+ (instancetype)networkStaForInternetConnection;


/*!
 * Start listening for notifications on the current run loop.
 */
- (void)setEvtCallback:(NetworkConnectionChangedEvt)callback userData:(void*)userData;
- (BOOL)startNotifier;
- (void)stopNotifier;

- (NetworkStatus)currentNetworkStaStatus;
- (void) networkStaChanged;

/*!
 * WWAN may be available, but not active until a connection has been established. WiFi may require a connection for VPN on Demand.
 */
- (BOOL)connectionRequired;

@end


