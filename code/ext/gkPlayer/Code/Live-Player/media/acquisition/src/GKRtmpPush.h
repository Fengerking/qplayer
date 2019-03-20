#ifndef __GK_RTMP_PUSH_H__
#define __GK_RTMP_PUSH_H__

#include "GKOsalConfig.h"

#ifndef __TT_OS_IOS__
#include "TTVCapProcess.h"
#include "TTAudioCapture.h"
#endif

#include "GKInterface.h"
#include "TThread.h"
#include "GKCritical.h"
#include "TTSemaphore.h"

#include "rtmp_sys.h"
#include "rtmpamf.h"
#include "CBufferManager.h"
#include "TTEventThread.h"
#include "GKNotifyEnumDef.h"
#include "GKRtmpPushItf.h"
#include "CGKBuffer.h"

#include "CFlowMonitor.h"

//#define DUMP_FLV 1
#define NET_SEND 1

class CGKRtmpPush : public IGKRtmpPush
{	
public:
	/**
	* \fn							CTTRtmpPush();
	* \brief						构造函数	
	*/
	CGKRtmpPush(IGKMsgObserver* aObserver);

	/**
	* \fn							~CTTRtmpPush();
	* \brief						析构函数	
	*/
	virtual ~CGKRtmpPush();

	virtual TTInt                   Open(const TTChar* aUrl);
    virtual TTInt                   Stop();
    virtual TTInt					SetPublishSource(const TTChar* aUrl);
    virtual void					TransferVdieoData(TTPBYTE pdata, TTInt size, TTUint32 pts,TTInt nFlag);
    virtual void					TransferAudioData(TTPBYTE pdata, TTInt size, TTUint32 pts);
    virtual void					SetAudioConfig(TTPBYTE pdata, TTInt size);
    virtual void					SetVideoConfig(TTPBYTE sps, TTInt spslen, TTPBYTE pps, TTInt ppslen);
    virtual void                    Setbitrate(int bitrate);
    
	virtual	void					SetCollectionType(int type);

#ifdef __TT_OS_IOS__
    virtual void                    Netdisconnect();
#endif

public:

#ifdef __TT_OS_ANDROID__    
    //andriod hardencode use
	void							SetVideoConfig(TTPBYTE buf, TTInt len);
	void							TransferVdieoData(TTPBYTE pdata, TTInt size, TTUint32 pts);

	void				        	GetLastPic(int* prgb,TTInt size);
    void                            SetFilePath(const TTChar* aUrl);
#endif

	TTInt							Close();
    void                            Cancel();
	TTInt							ConnectRtmpServer();
	TTInt							onPublishSource(TTInt nMsg, TTInt nVar1);
	TTInt							onStop(TTInt nMsg, TTInt nVar1);
	TTInt							postPublishSourceEvent (TTInt  nDelayTime);
	TTInt							postStopEvent(TTInt  nDelayTime);
    TTInt                           postFlowMonitorEvent(TTInt  nDelayTime);
    TTInt                           postCongestionEvent(TTInt  nDelayTime);
    
    TTInt                           onFlowMonitor(TTInt nMsg, TTInt nVar1);
    TTInt                           onNetCongestion(TTInt nMsg, TTInt nVar1);

	//used by android
	void							TransferVdieoData(CGKBuffer* pbuffer);
	void							SetVideoSps(TTPBYTE buf, TTInt len);
	void							SetVideoPps(TTPBYTE buf, TTInt len);

#ifdef __TT_OS_ANDROID__ 
	void							TransferVdieoRawData(TTPBYTE pdata, TTInt size, TTUint32 pts,TTInt rotateType);
	void							TransferAudioRawData(TTPBYTE pdata, TTInt size, TTUint32 pts);

	void							VideoEncoderInit();
	void							SetVideoFpsBitrate(int fps,int Bitrate);
	void							SetVideoWidthHeight(int width,int height);

	void							HandleRawdata(unsigned char* src,unsigned char* dst,int with, int heigh, int rotateType);
	void							SetColorFormat(int color);

	void							RecordStart(int smaplerate, int channel);
	void							RecordPause();
	void							RecordResume();
	void							RecordClose();
	void							SetRenderProxy(void* proxy);
    
    long					        GetRecordTime();
    
    //file record
    void                            WriteVdieoData(CTTBuffer* pbuffer);
    void                            WriteAudioData(CTTBuffer* pbuffer);
    void							WriteVdieoConfig(TTUint32 TimeStamp);
    void							WriteAudioConfig(TTUint32 TimeStamp);
#endif

	int	 OpenFLV();
	int  CloseFLV();
	int  WriteFLVTag(RTMPPacket * packet);
	int  WriteMetaData();

private:
	static void* 					PushThreadProc(void* aPtr);
	void 							PushThreadProcL(void* aPtr);

#ifdef __TT_OS_ANDROID__   
	static TTInt					OnVideoSend(void * pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	static TTInt					OnAudioSend(void * pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	TTInt							ProcessVSendData(TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	TTInt							ProcessASendData(TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
#endif    
    TTInt							ReConnectServer();

	TTInt							SendVideoData(CGKBuffer* aBuffer);

	TTInt							SendVideoConfig(int pts);

	TTInt							SendAACConfig(int pts);

	TTInt							SendAACData(CGKBuffer* aBuffer);

    void						    NotifyEvent(GKPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2);

private:
	TTChar*						iUrl;
	RGKCritical					iCritical;
	RGKCritical					iMsgCritical;
	RTTSemaphore				iSemaphore;
	TTBool				        iCancel;
    RTThread					iThreadHandle;

	TTChar*						iCacheUrl;
	unsigned long				start_time;

	TTInt						iBufferSize;
	unsigned char *				iBuffer;

	TTPBYTE						m_aacBufferConfig;
	TTInt						m_aacConfigLength;
	TTPBYTE						m_spsBuffer;
	TTInt						m_spsLength;
	TTPBYTE						m_ppsBuffer;
	TTInt						m_ppsLength;

	RTMP						iRtmp;
    TTBool                      iFirstPacket;
	TTInt						iErrorCode;
	pthread_t					iConnectionTid;

	TTEventThread*				mHandleThread;
	TTChar*						mUrlPath;

	IGKMsgObserver*				mObserver;
	CBufferManager				mBufferManager;
	TTBool                      iAAcConfigSet;
	TTBool						mOpenStatus;
	TTBool						mFirstConnect;
	TTBool						mClose;
    TTInt                       mReConnectMsg;
    TTInt                       mReConnecttimes;
    TTInt                       mConnectErrorCode;
    TTInt64                     mlastMonitor;
    TTInt                       mVtimes;
    TTInt                       mAtimes;

	RGKCritical					mCtritConfig;

#ifndef __TT_OS_IOS__
	TTObserver					mSendVideo;
	CTTVCapProcess*        		mVCapProcess;	

	TTObserver					mSendAideo;
	TTAudioCapture*				mACapProcess;

#endif


	FILE*						mFileFLV;
	int							mCollectType;
	int							mCollectStart;
	CFlowMonitor                mFlowMonitor;
	long                        mRecordTime;
    
    TTChar*						iRecordPath;
    FILE*						mRecordFLV;
};


class GKCRtmpEvent : public TTBaseEventItem
{
public:
    GKCRtmpEvent(CGKRtmpPush * pPush, TTInt (CGKRtmpPush::* method)(TTInt, TTInt),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2)
	{
		mPush = pPush;
		mMethod = method;
    }

    virtual ~GKCRtmpEvent()
	{
	}

    virtual void fire (void) 
	{
        (mPush->*mMethod)(mMsg, mVar1);
    }

protected:
    CGKRtmpPush *		mPush;
    int (CGKRtmpPush::* mMethod) (TTInt, TTInt);
};

#endif
