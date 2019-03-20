/**
* File : GKPushWarp.h
 * Created on : 2015-12-12
 * Copyright : Copyright (c) 2015 All rights reserved.
*/
#ifndef __TT_PUSH_WARP__
#define __TT_PUSH_WARP__

#include "GKRtmpPushItf.h"
#include "GKNotifyEnumDef.h"

class CGKPushWrap : IGKMsgObserver
{
public:
    CGKPushWrap(void* aMediaPlayerProxy);
    ~CGKPushWrap();
    
    virtual void NotifyEvent(GKPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2);
    
    int startpublish(const TTChar* url);
    void stop();
    void setbitrate(int bitrate);
    
    int sendvideopacket(TTPBYTE arry,int size, long pts, int flag);
    int sendaudiopacket(TTPBYTE arry,int size, long pts);
    int audioconfig(TTPBYTE arry,int size);
    void SetSpsPpsConfig(TTPBYTE sps, TTInt spslen, TTPBYTE pps, TTInt ppslen);
    
    void netdisconnect();
    void  settype(int type);
    
public:
    IGKRtmpPush*  iRtmpPush;
    void*   iiRtmpPushProxy;
};

#endif