//
//  GKRtmpPushItf.h
//  acquisition
//
//  Created by on 15/12/11.
//  Copyright © 2015年 All rights reserved.
//

#ifndef GKRtmpPushItf_h
#define GKRtmpPushItf_h
#include "GKOsalConfig.h"
#include "GKMacrodef.h"
#include "GKTypedef.h"
#include "GKInterface.h"
#include "GKNotifyEnumDef.h"

class IGKMsgObserver
{
public:
    /**
     * \fn                           void NotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2)
     * \brief
     * \param [in]      aMsg   		操作ID
     * \param [in]	   aArg1        参数1
     */
    virtual void NotifyEvent(GKPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2) = 0;
    
#ifdef __GK_OS_ANDROID__
    virtual long GetMobileTx() = 0;
#endif
    
};

class IGKRtmpPush : public IGKInterface
{
public:
    virtual TTInt			Open(const TTChar* aUrl)= 0;
 
    virtual TTInt           SetPublishSource(const TTChar* aUrl)= 0;
    
    virtual TTInt			Stop()= 0;
    
    virtual void			TransferVdieoData(TTPBYTE pdata, TTInt size, TTUint32 pts,TTInt nFlag)= 0;
    
    virtual void			TransferAudioData(TTPBYTE pdata, TTInt size, TTUint32 pts)= 0;
    
    virtual void			SetAudioConfig(TTPBYTE pdata, TTInt size)= 0;
    
    virtual void			SetVideoConfig(TTPBYTE sps, TTInt spslen, TTPBYTE pps, TTInt ppslen)= 0;

    virtual void			SetCollectionType(int type)= 0;

#ifndef __TT_OS_IOS__

	virtual	void			TransferVdieoRawData(TTPBYTE pdata, TTInt size, TTUint32 pts)= 0;

	virtual	void			VideoEncoderInit()= 0;

	virtual	void			SetVideoFpsBitrate(int fps,int Bitrate)= 0;

	virtual void			SetVideoWidthHeight(int width,int height)= 0;

#endif
    
#ifdef __TT_OS_IOS__
    virtual void            Netdisconnect()= 0;
#endif
    
    virtual void            Setbitrate(int bitrate)= 0;

};

#endif /* GKRtmpPushItf_h */
