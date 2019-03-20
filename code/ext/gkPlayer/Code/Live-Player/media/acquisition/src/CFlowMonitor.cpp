#include "CFlowMonitor.h"
#include "TTLog.h"
#include "GKMacrodef.h"
#include <string.h> 

#ifdef __TT_OS_IOS__
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif

#define THRESHOLD_UP        10
#define THRESHOLD_UPTODOWN  3
#define THRESHOLD_DOWNTOUP  3
int flowspeed[] = {550000,450000,400000,350000,300000,250000,200000};

int flowspeed15fps[] = {105,85,75,70,60,48,43};
int flowspeed20fps[] = {110,90,80,75,65,56,48};

CFlowMonitor::CFlowMonitor()
:mType(AJUSTSPEED_DOWN)
,mlastFlowValue(0)
,mMonitorInterval(6)
,mbitrate(0)
,mDownflag(0)
,mUpflag(0)
,mUptoDownflag(0)
,mDowntoUpflag(0)
{
	mLock.Create();

}


CFlowMonitor::~CFlowMonitor() 
{
	mLock.Destroy();
}

#ifdef __TT_OS_IOS__
TTInt CFlowMonitor::IosCheckNetworkflow()
{
    struct ifaddrs *ifa_list = 0, *ifa;
    if (getifaddrs(&ifa_list) == -1)
    {
        return 0;
    }

    uint32_t G4OBytes = 0;
    uint32_t wifiOBytes = 0;
    //struct timeval time ;
    
    for (ifa = ifa_list; ifa; ifa = ifa->ifa_next)
    {
        if (AF_LINK != ifa->ifa_addr->sa_family)
            continue;
        
        if (!(ifa->ifa_flags & IFF_UP) && !(ifa->ifa_flags & IFF_RUNNING))
            continue;
        
        if (ifa->ifa_data == 0)
            continue;
        
        //wifi flow
        /*if (!strcmp(ifa->ifa_name, "en0"))
        {
            struct if_data *if_data = (struct if_data *)ifa->ifa_data;
            
            wifiOBytes += if_data->ifi_obytes;
        }*/
        
        //3G and gprs flow
        if (!strcmp(ifa->ifa_name, "pdp_ip0"))
        {
            struct if_data *if_data = (struct if_data *)ifa->ifa_data;
            
            G4OBytes += if_data->ifi_obytes;
        }
    }
    freeifaddrs(ifa_list);

	return G4OBytes;
}

#endif


TTInt CFlowMonitor::CheckNetworkflow(int androidFlowValue)
{
    int bitrate = 0;
/*	mCurrent = 0;

#ifdef __TT_OS_ANDROID__
	mCurrent = androidFlowValue;

	if (mCurrent <0)
		return 0;
#endif

#ifdef __TT_OS_IOS__
    mCurrent = IosCheckNetworkflow();
#endif

    if (mCurrent == 0)
		return 0;
    
    if (mlastFlowValue == 0 && mCurrent > 0) {
        mlastFlowValue = mCurrent;
    }
    
    int curSpeed = mCurrent - mlastFlowValue;
    mlastFlowValue = mCurrent;
    int diff;
    int tmp=0;
    
    if (curSpeed>0) {
        
        curSpeed = curSpeed/1024/mMonitorInterval;
        int i = 0;
        if (mType == AJUSTSPEED_DOWN) {
            for (i=0; i<7; i++) {
                if (mbitrate == flowspeed[i]) {
                    tmp = flowspeed15fps[i];
                    break;
                }
            }
            if (i<7) {
                if (curSpeed >= flowspeed15fps[i]) {
                    mDownflag = 0;
                    
                    mDowntoUpflag++;
                    if (mDowntoUpflag >= THRESHOLD_DOWNTOUP) {
                        mUptoDownflag = 0;
                        mDowntoUpflag = 0;
                        mUpflag = 0;
                        mType = AJUSTSPEED_UP;
                        bitrate = (i-1>=0) ? flowspeed[i-1]: flowspeed[0];
                        //printf("\n down-to-up , turn to up\n");
                    }
                }
                else{
                    mDownflag++;
                    mDowntoUpflag = 0;
                    if (mDownflag >1) {
                        bitrate = (i+2<7) ? flowspeed[i+2] : flowspeed[6];
                        diff = flowspeed15fps[i]-curSpeed;
                        if (diff <=5) {
                            bitrate = 0;
                            mDownflag = 0;
                        }
                        else if (diff < 10 && mbitrate >=400000) {
                            bitrate = flowspeed[i+1];
                        }
                    }
                }
            }
        }
        else{
            for (i=0; i<7; i++) {
                if (mbitrate == flowspeed[i]) {
                    tmp = flowspeed15fps[i];
                    break;
                }
            }
            if (i<7) {
                if (curSpeed >= flowspeed15fps[i]) {
                    mUpflag++;
                    mUptoDownflag = 0;
                    //return 0;
                    if (mbitrate <= 250000 && mUpflag >THRESHOLD_UP ) {
                        mUpflag = 0;
                        bitrate = flowspeed[i-1];
                    }
                    else if (mbitrate <= 350000 && mUpflag >THRESHOLD_UP+5){
                        mUpflag = 0;
                        bitrate = flowspeed[i-1];
                    }
                    else if (mbitrate <= 450000 && mUpflag >THRESHOLD_UP*2){
                        mUpflag = 0;
                        bitrate = flowspeed[i-1];
                    }
                }
                else{
                    //ready to down
                    mUpflag = 0;
                    if (mbitrate > 250000){
                        mUptoDownflag++;
                        if (mUptoDownflag > THRESHOLD_UPTODOWN) {
                            mUptoDownflag = 0;
                            mDownflag = 0;
                            mDowntoUpflag = 0;
                            mUpflag = 0;
                            mType = AJUSTSPEED_DOWN;
                            bitrate = (i+1<7) ? flowspeed[i+1] : flowspeed[6];
                            //printf("\n up-to-down , turn to  down\n");
                        }
                    }
                }
            }
        }
    }
    //printf("\n-- flow = %d [%d] bitrate = %d",curSpeed, tmp, bitrate);
    */
    return bitrate;
}

void CFlowMonitor::SetMonitorTime(TTInt atime)
{
    mMonitorInterval = atime;
}

void CFlowMonitor::SetBitrate(TTInt bitrate)
{
    mbitrate = bitrate;
    
    mUptoDownflag = 0;
    mDownflag = 0;
    mDowntoUpflag = 0;
    mUpflag = 0;
}


