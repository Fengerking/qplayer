/**
* File : TTHLSInfoProxy.h 
* Created on : 2015-5-12
* Author : yongping.lin
* Description : CTTHLSInfoProxy
*/

#ifndef __TT_HLS_INFO_PROXY_H__
#define __TT_HLS_INFO_PROXY_H__

#include "GKTypedef.h"
#include "TTMediainfoDef.h"
#include "TTStreamBufferingItf.h"
#include "TTIMediaInfo.h"
#include "GKCritical.h"
#include "TTEventThread.h"
#include "TTSemaphore.h"
#include "TTTSParserProxy.h"
#include "TTPackedaudioParser.h"
#include "TTPlaylistManager.h"
#include "TTIOClient.h"
#include "TTLiveSession.h"
#include "TTList.h"

enum TTMediaInfoMsg
{
	EMediaListUpdated = 1
	, ECheckBandWidth = 2
	, ECheckCPULoading = 3
	, ECheckBufferStatus = 4
	, ECheckStartBASession = 5
	, ECheckBAStatus = 6
	, ECheckCancelBASession = 7
	, ECheckCloseSession = 8
};

enum TTSwitchStatus
{
	EBASwitchNone = 0
	, EBASwitchDownStart = 1
	, EBASwitchUpStart = 2
	, EBASwitching = 3
	, EBASwitched = 4
	, EBASwitchCancel = 10
};


class CTTHLSInfoProxy : public ITTStreamBufferingObserver, public ITTMediaInfo
{
public:

	/**
	* \fn								 CTTHLSInfoProxy(TTObserver& aObserver);
	* \brief							 ���캯��
	* \param[in]   aObserver			 Observer����
	*/
	CTTHLSInfoProxy(TTObserver* aObserver);

	/**
	* \fn								 ~CTTHLSInfoProxy();
	* \brief							 ��������
	*/
	virtual ~CTTHLSInfoProxy();

public:

	/**
	* \fn								 TTInt Open(const TTChar* aUrl);
	* \brief							 ��ý���ļ�
	* \param[in]   aUrl					 �ļ�·��  
	* \param[in]   aStreamBufferingObserver	 ITTStreamBufferingObserver����ָ�� 
	* \return							 ����״̬
	*/
	virtual TTInt						  Open(const TTChar* aUrl, TTInt aFlag);
	
	/**
	* \fn								 TTInt Parse();
	* \brief							 �����ļ�
	* \return							 ����״̬
	*/
	virtual TTInt						 Parse();

	/**
	* \fn								 void Close();
	* \brief							 �رս�����
	*/
	virtual void						 Close();
	
	/**
	* \fn								 TTUint MediaDuration()
	* \brief							 ��ȡý��ʱ��(����)
	* \param[in]   aMediaStreadId		 ý����Id  
	* \return							 ý��ʱ��
	*/
	virtual TTUint						 MediaDuration();

	/**
	* \fn								 TTMediaInfo GetMediaInfo();
	* \brief							 �����ļ�
	* \param[in]	aUrl				 �ļ�·��
	*/
	virtual const TTMediaInfo&	     	 GetMediaInfo();

	/**
	* \fn								 TTUint MediaSize()
	* \brief							 ��ȡý���С(�ֽ�)
	* \return							 ý���С
	*/
	virtual TTUint						 MediaSize();

	/**
	* \fn								 TTBool IsSeekAble();
	* \brief							 �Ƿ���Խ���Seek����
	* \return                            ETTTrueΪ����
	*/
	virtual TTBool						 IsSeekAble();
	/**
	* \fn								 void CreateFrameIndex();
	* \brief							 ��ʼ����֡����������û���������ļ�����Ҫ�첽����������
	* \									 ���������������ļ���ֱ�Ӷ�ȡ
	*/
	virtual void						 CreateFrameIndex();

	/**
	* \fn								 TTMediaInfo GetMediaSample();
	* \brief							 ���audio��video����sample��Ϣ
	* \param[in]	aUrl				 �ļ�·��
	*/
	virtual TTInt			 			 GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer);

	/**
	* \fn								 TTInt SelectStream()
	* \brief							 ѡ��audio stream��
	* \param[in]	aStreamId			 Media Stream ID,����audio����video stream
	* \return							 ����״̬
	*/
	virtual TTInt			     		 SelectStream(TTMediaType aType, TTInt aStreamId);
	
	/**
	* \fn								 void Seek(TTUint aPosMS);
	* \brief							 Seek����
	* \param[in]	aPosMS				 λ�ã���λ������
	* \return							 ��������seek��λ�ã�����Ǹ���������seek ʧ�ܡ�
	*/
	virtual TTInt64						 Seek(TTUint64 aPosMS,TTInt aOption = 0);


	virtual void						 SetNetWorkProxy(TTBool aNetWorkProxy);

	/**
	* \fn								 TTInt SetParam()
	* \brief							 ���ò�����
	* \param[in]	aType				 ��������
	* \param[in]	aParam				 ����ֵ
	* \return							 ����״̬
	*/
	virtual TTInt				     	 SetParam(TTInt aType, TTPtr aParam);

	/**
	* \fn								 TTInt GetParam()
	* \brief							 ��ȡ����ֵ��
	* \param[in]	aType				 ��������
	* \param[in/out]	aParam			 ����ֵ
	* \return							 ����״̬
	*/
	virtual TTInt	     				 GetParam(TTInt aType, TTPtr aParam);

public:
	/**
	* \fn								 void CancelReader()
	* \brief							 ȡ������reader
	*/
	virtual void						 CancelReader();

	/**
	* \fn								 TTUint BandWidth()
	* \brief							 ��ȡ�������ش����С
	* \return							 �����С(��λ:�ֽ�)
	*/
	virtual TTUint						 BandWidth();

	/**
	* \fn								 TTUint BandPercent()
	* \brief							 ��ȡ�����������ݰٷֱ�
	* \return							 �����С(��λ:�ֽ�)
	*/
	virtual TTUint						 BandPercent();


public:
	/**
	* \fn								 TTBool IsCreateFrameIdxComplete();
	* \brief							 �������Ƿ����
	* \return							 ���ΪETTrue
	*/
	virtual TTBool						 IsCreateFrameIdxComplete();

	/**
	* \fn								 TTUint BufferedSize()
	* \brief							 ��ȡ�ѻ�������ݴ�С
	* \return							 �ѻ�������ݴ�С
	*/
	virtual TTUint						 BufferedSize();


	virtual TTUint						 ProxySize();

	virtual void					     SetDownSpeed(TTInt aFast);

	/**
	* \fn								 TTInt BufferedPercent(TTInt& aBufferedPercent)
	* \brief							 ��ȡ�ѻ������ݵİٷֱ�
	* \param[out]	aBufferedPercent	 �ٷֱ�
	* \return							 ����״̬
	*/
	virtual TTInt						 BufferedPercent(TTInt& aBufferedPercent);

	virtual void						 SetObserver(TTObserver*	aObserver);

public:// from ITTStreamBufferingObserver
	/**
	* \fn								 void StreamIsBuffering();
	* \brief						     ����������������	
	*/
	virtual void						 BufferingStart(TTInt nErr, TTInt nStatus, TTUint32 aParam);

	/**
	* \fn								 void StreamBufferCompleted();
	* \brief						 	 �������ݻ������	
	*/
	virtual void						 BufferingDone();
    
	/**
	* \fn								 void DNSDone();
	* \brief							 DNS�������
	*/
	virtual void						 DNSDone();

	/**
	* \fn								 void ConnectDone();
	* \brief							 �������
	*/
	virtual void						 ConnectDone();

	/**
	* \fn								 void HttpHeaderReceived();
	* \brief							 Httpͷ���ճɹ�
	*/
	virtual void						 HttpHeaderReceived();

    /**
	* \fn								 void PrefetchStart(TTUint32 aParam);
	* \brief						 	 ���翪ʼ����
	*/
    virtual void                    	 PrefetchStart(TTUint32 aParam);

	/**
	* \fn								 void PrefetchCompleted();
	* \brief							 Ԥȡ�������
	*/
	virtual void						 PrefetchCompleted();
	
	/**
	* \fn								 void StreamBufferCompleted();
	* \brief						 	 ����ý������ȫ�����	
	*/
	virtual void						 CacheCompleted(const TTChar* pFileName);

	virtual void	    				 DownLoadException(TTInt errorCode, TTInt nParam2, void *pParam3);

protected:
	int									 IsHLSSource(const TTChar* aUrl);

	int									 GetMediaSamplebyID(CLiveSession* aLiveSession, TTMediaType aStreamType, TTBuffer* pMediaBuffer, TTInt elementID);

	TTInt								 onInfoBandWidth();
	TTInt								 onInfoCPULoading();
	TTInt								 onInfoBufferStatus(int nDuration);
	TTInt								 onInfoBufferStart(CLiveSession*	aLiveSession);
	TTInt								 onInfoStartBASession(TTInt nIndex, TTInt upDown);
	TTInt								 onInfoCheckBAStatus(TTInt nStatus, TTInt nCount);
	TTInt								 onInfoCancelBASession(TTInt nStatus);
	TTInt								 onInfoCloseSession(CLiveSession*	aLiveSession);

	TTInt								 onInfoHandle(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);

	virtual TTInt						 postInfoMsgEvent(TTInt  nDelayTime, TTInt32 nMsg, int nParam1, int nParam2, void * pParam3);

	TTInt								 initPlayList(const TTChar* aUrl, TTInt aFlag);
	TTInt								 resetInitPlayList();
	TTInt								 updatePlayList(ListItem* vItem);
	TTInt								 postPlayList(ListItem* vItem, TTInt nErr, TTInt isBAItem);

	TTInt								 updateMediaInfo(CLiveSession*	aLiveSession);
	CLiveSession*						 getLiveSession();
	TTInt								 putLiveSession(CLiveSession* aLiveSession);
	TTInt								 freeLiveSession();

	void								 resetTimeStamp(TTMediaType aStreamType, TTBuffer* pMediaBuffer);
	void								 upDateTimeStamp(TTMediaType aStreamType, TTBuffer* pMediaBuffer);

	void								 setBAStatus(int aBAStatus);
	int									 getBAStatus();


	TTInt								 isHeadReady(CLiveSession*	aLiveSession, int checkAudio, int checkVideo);

	int									 doDownLoadList(const char* url, char* actualUrl);

private:
	TTObserver*				iObserver;
	TTMediaInfo				iMediaInfo;

	TTEventThread*			mMsgThread;
	PlaylistManager*		mPlayListManager;
	TTInt					mCurBitrateIndex;
	TTInt					mCurMinBuffer;
	TTInt					mCurBandWidth;

	CTTIOClient*			mPL;
	unsigned char*			mPlayListBuffer;
	unsigned int			mPlayListSize;

	TTInt					mAudioStreamID;
	TTInt					mVideoStreamID;

	bool					mFirstPTSValid;
    TTInt64					mFirstPTS;
	TTInt64					mResetAudioPTS;
	TTInt64					mResetVideoPTS;
	TTInt64					mLastAudioPTS;
	TTInt64					mLastVideoPTS;
	TTInt64					mFirstTimeOffsetUs;

	TTInt64					mSeekTime;
	TTInt					mSeekOption;
	bool					mSeeking;

	
	CLiveSession*			mCurSession;
	CLiveSession*			mBASession;
	ListItem*				mCurItem;
	ListItem*				mBAItem;

	List<CLiveSession *>	mListLSFree;

	TTInt					mBufferStatus;
	TTInt					mBASwitchStatus;
	TTInt					mSwitchAudioStatus;
	TTInt					mSwitchVideoStatus;
	TTInt					mBABitrateIndex;
	TTInt					mUpCount;
	TTInt					mDownCount;
	TTInt					mStartHLS;
	TTInt					mLiveErrNum;

	TTInt					mCancel;
	RTTSemaphore			mSemaphore;
	RGKCritical				mCriSession;
	RGKCritical				mCriEvent;
	RGKCritical				mCriBA;
	RGKCritical				mCriPlayList;
};

class TTCHLSProxyEvent : public TTBaseEventItem
{
public:
    TTCHLSProxyEvent(CTTHLSInfoProxy * pHLSInfo, TTInt (CTTHLSInfoProxy::* method)(TTInt, TTInt, TTInt, void*),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0, void* nVar3 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2, nVar3)
	{
		mHLSProxy = pHLSInfo;
		mMethod = method;
    }

    virtual ~TTCHLSProxyEvent()
	{
	}

    virtual void fire (void) 
	{
        (mHLSProxy->*mMethod)(mMsg, mVar1, mVar2, mVar3);
    }

protected:
    CTTHLSInfoProxy *		mHLSProxy;
    int (CTTHLSInfoProxy::* mMethod) (TTInt, TTInt, TTInt, void*);
};

#endif  //__TT_HLS_INFO_PROXY_H__
