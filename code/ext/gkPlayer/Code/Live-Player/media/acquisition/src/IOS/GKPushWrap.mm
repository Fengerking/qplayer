/**
* File : GKPushWarp.cpp
* Description : CTTPushWrap
*/

#include "GKPushWrap.h"
#include "GKPushProxy.h"
#include "TTLog.h"
#import "GKRtmpPush.h"

CGKPushWrap::CGKPushWrap(void* aProxy)
{
    GKASSERT(aProxy != NULL);
    iiRtmpPushProxy = aProxy;
    iRtmpPush = new CGKRtmpPush(this);
}

CGKPushWrap::~CGKPushWrap()
{
    SAFE_RELEASE(iRtmpPush);
}

void CGKPushWrap::NotifyEvent(GKPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2)
{
    LOGE("\nPlayerNotifyEvent aMsg: %d",  aMsg);
    [((__bridge CGKPushProxy*)iiRtmpPushProxy) ProcessNotifyEventWithMsg : aMsg andError0 : aArg1 andError1 : aArg2];
}

TTInt CGKPushWrap::startpublish(const TTChar* url)
{
    GKASSERT(iRtmpPush != NULL);
    return iRtmpPush->SetPublishSource(url);
}

TTInt CGKPushWrap::sendvideopacket(TTPBYTE arry,int size, long pts, int flag)
{
    GKASSERT(iRtmpPush != NULL);
    iRtmpPush->TransferVdieoData(arry,size,pts, flag);
    return 0;
}
TTInt CGKPushWrap::sendaudiopacket(TTPBYTE arry,int size, long pts)
{
    GKASSERT(iRtmpPush != NULL);
    iRtmpPush->TransferAudioData(arry,size,pts);
    return 0;
}
TTInt CGKPushWrap::audioconfig(TTPBYTE arry,int size)
{
    GKASSERT(iRtmpPush != NULL);
    iRtmpPush->SetAudioConfig(arry,size);
    return 0;
}

void CGKPushWrap::SetSpsPpsConfig(TTPBYTE sps, TTInt spslen, TTPBYTE pps, TTInt ppslen)
{
    GKASSERT(iRtmpPush != NULL);
    iRtmpPush->SetVideoConfig(sps,spslen,pps,ppslen);
}

void CGKPushWrap::stop()
{
    GKASSERT(iRtmpPush != NULL);
    iRtmpPush->Stop();
}

void CGKPushWrap::netdisconnect()
{
    iRtmpPush->Netdisconnect();
}

void CGKPushWrap::setbitrate(int bitrate)
{
    iRtmpPush->Setbitrate(bitrate);
}

void  CGKPushWrap::settype(int type)
{
    iRtmpPush->SetCollectionType(type);
}