/*******************************************************************************
	File:		CNetworkSta.h
 
	Contains:	Stataus detect class implement code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-06-14		Jun			Create file
 
 *******************************************************************************/

#import <arpa/inet.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <CoreFoundation/CoreFoundation.h>

#import "NetworkSta.h"

#define kShouldPrintNetworkStaFlags 1
static void PrintNetworkStaFlags(SCNetworkReachabilityFlags flags, const char* comment)
{
#if kShouldPrintNetworkStaFlags

    NSLog(@"networkSta Flag Status: %c%c %c%c%c%c%c%c%c %s\n",
          (flags & kSCNetworkReachabilityFlagsIsWWAN)				? 'W' : '-',
          (flags & kSCNetworkReachabilityFlagsReachable)            ? 'R' : '-',

          (flags & kSCNetworkReachabilityFlagsTransientConnection)  ? 't' : '-',
          (flags & kSCNetworkReachabilityFlagsConnectionRequired)   ? 'c' : '-',
          (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)  ? 'C' : '-',
          (flags & kSCNetworkReachabilityFlagsInterventionRequired) ? 'i' : '-',
          (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)   ? 'D' : '-',
          (flags & kSCNetworkReachabilityFlagsIsLocalAddress)       ? 'l' : '-',
          (flags & kSCNetworkReachabilityFlagsIsDirect)             ? 'd' : '-',
          comment
          );
#endif
}


static void networkStaCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info)
{
#pragma unused (target, flags)

    CNetworkSta* noteObject = (__bridge CNetworkSta *)info;
    if(noteObject)
    	[noteObject networkStaChanged];
}


@implementation CNetworkSta
{
	SCNetworkReachabilityRef _networkStaRef;
    NetworkConnectionChangedEvt _callback;
    void*						_userData;
}

+ (instancetype)networkStaWithHostName:(NSString *)hostName
{
	CNetworkSta* returnValue = NULL;
	SCNetworkReachabilityRef networkSta = SCNetworkReachabilityCreateWithName(NULL, [hostName UTF8String]);
	if (networkSta != NULL)
	{
		returnValue= [[self alloc] init];
		if (returnValue != NULL)
		{
			returnValue->_networkStaRef = networkSta;
		}
        else {
            CFRelease(networkSta);
        }
	}
	return returnValue;
}


+ (instancetype)networkStaWithAddress:(const struct sockaddr *)hostAddress
{
	SCNetworkReachabilityRef networkSta = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, hostAddress);

	CNetworkSta* returnValue = NULL;

	if (networkSta != NULL)
	{
		returnValue = [[self alloc] init];
		if (returnValue != NULL)
		{
			returnValue->_networkStaRef = networkSta;
		}
        else {
            CFRelease(networkSta);
        }
	}
	return returnValue;
}


+ (instancetype)networkStaForInternetConnection
{
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;
    
    return [self networkStaWithAddress: (const struct sockaddr *) &zeroAddress];
}


- (void) networkStaChanged
{
    NetworkStatus netStatus = [self currentNetworkStaStatus];
    BOOL connectionRequired = [self connectionRequired];
    
    if(_callback && _userData)
        _callback(_userData, (int)netStatus);
    
    if (connectionRequired)
    {
        NSLog(@"Cellular data network is available.\nInternet traffic will be routed through it after a connection is established.");
    }
    else
    {
        NSLog(@"Cellular data network is active.\nInternet traffic will be routed through it.");
    }
}


- (BOOL)startNotifier
{
	BOOL returnValue = NO;
	SCNetworkReachabilityContext context = {0, (__bridge void *)(self), NULL, NULL, NULL};

	if (SCNetworkReachabilitySetCallback(_networkStaRef, networkStaCallback, &context))
	{
		if (SCNetworkReachabilityScheduleWithRunLoop(_networkStaRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode))
		{
			returnValue = YES;
		}
	}
    
	return returnValue;
}


- (void)stopNotifier
{
    if (_networkStaRef != NULL)
    {
        SCNetworkReachabilityUnscheduleFromRunLoop(_networkStaRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        SCNetworkReachabilityContext context = {0, (__bridge void *)(self), NULL, NULL, NULL};
        SCNetworkReachabilitySetCallback(_networkStaRef, nil, &context);
        _userData = nil;
        _callback = nil;
        CFRelease(_networkStaRef);
        _networkStaRef = nil;
    }
}


- (void)dealloc
{
	[self stopNotifier];
    [super dealloc];
}

- (NetworkStatus)networkStatusForFlags:(SCNetworkReachabilityFlags)flags
{
	PrintNetworkStaFlags(flags, "networkStatusForFlags");
	if ((flags & kSCNetworkReachabilityFlagsReachable) == 0)
	{
		// The target host is not reachable.
		return NotReachable;
	}

    NetworkStatus returnValue = NotReachable;

	if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0)
	{
		/*
         If the target host is reachable and no connection is required then we'll assume (for now) that you're on Wi-Fi...
         */
		returnValue = ReachableViaWiFi;
	}

	if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
        (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0))
	{
        /*
         ... and the connection is on-demand (or on-traffic) if the calling application is using the CFSocketStream or higher APIs...
         */

        if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0)
        {
            /*
             ... and no [user] intervention is needed...
             */
            returnValue = ReachableViaWiFi;
        }
    }

	if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN)
	{
		/*
         ... but WWAN connections are OK if the calling application is using the CFNetwork APIs.
         */
		returnValue = ReachableViaWWAN;
	}
    
	return returnValue;
}


- (BOOL)connectionRequired
{
	NSAssert(_networkStaRef != NULL, @"connectionRequired called with NULL networkStaRef");
	SCNetworkReachabilityFlags flags;

	if (SCNetworkReachabilityGetFlags(_networkStaRef, &flags))
	{
		return (flags & kSCNetworkReachabilityFlagsConnectionRequired);
	}

    return NO;
}


- (NetworkStatus)currentNetworkStaStatus
{
	NSAssert(_networkStaRef != NULL, @"currentNetworkStatus called with NULL SCNetworknetworkStaRef");
	NetworkStatus returnValue = NotReachable;
	SCNetworkReachabilityFlags flags;
    
	if (SCNetworkReachabilityGetFlags(_networkStaRef, &flags))
	{
        returnValue = [self networkStatusForFlags:flags];
	}
    
	return returnValue;
}

- (void)setEvtCallback:(NetworkConnectionChangedEvt)callback userData:(void*)userData
{
    _callback = callback;
    _userData = userData;
}


@end
