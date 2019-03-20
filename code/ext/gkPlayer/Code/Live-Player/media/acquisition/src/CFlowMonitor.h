#ifndef __TTFLOW_MONITOR_H_
#define __TTFLOW_MONITOR_H_

#include "GKOsalConfig.h"
#include "GKCritical.h"

#define AJUSTSPEED_DOWN 0
#define AJUSTSPEED_UP   1

class CFlowMonitor {
public:

    CFlowMonitor();
	virtual ~CFlowMonitor();
    
    void SetBitrate(TTInt);
    
    void SetMonitorTime(TTInt atime);
    
    TTInt CheckNetworkflow(int androidFlowValue);

#ifdef __TT_OS_IOS__
	TTInt IosCheckNetworkflow();
#endif

	 
private:

    RGKCritical mLock;
    
    TTInt mType;
    
    TTInt mbitrate;
    
    TTInt mMonitorInterval;

	TTInt64 mStartTimeUs;
    
    unsigned int mlastFlowValue;
    unsigned int mCurrent;
    TTInt mDownflag;
    TTInt mDowntoUpflag;
    
    TTInt mUpflag;
    TTInt mUptoDownflag;
#ifdef __TT_OS_ANDROID__

#endif
};


#endif  // __TTBUFFER_MANAGER_H_
