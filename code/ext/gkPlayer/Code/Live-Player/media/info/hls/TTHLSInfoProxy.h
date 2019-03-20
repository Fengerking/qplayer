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
	* \brief							 构造函数
	* \param[in]   aObserver			 Observer引用
	*/
	CTTHLSInfoProxy(TTObserver* aObserver);

	/**
	* \fn								 ~CTTHLSInfoProxy();
	* \brief							 析构函数
	*/
	virtual ~CTTHLSInfoProxy();

public:

	/**
	* \fn								 TTInt Open(const TTChar* aUrl);
	* \brief							 打开媒体文件
	* \param[in]   aUrl					 文件路径  
	* \param[in]   aStreamBufferingObserver	 ITTStreamBufferingObserver对象指针 
	* \return							 返回状态
	*/
	virtual TTInt						  Open(const TTChar* aUrl, TTInt aFlag);
	
	/**
	* \fn								 TTInt Parse();
	* \brief							 解析文件
	* \return							 返回状态
	*/
	virtual TTInt						 Parse();

	/**
	* \fn								 void Close();
	* \brief							 关闭解析器
	*/
	virtual void						 Close();
	
	/**
	* \fn								 TTUint MediaDuration()
	* \brief							 获取媒体时长(毫秒)
	* \param[in]   aMediaStreadId		 媒体流Id  
	* \return							 媒体时长
	*/
	virtual TTUint						 MediaDuration();

	/**
	* \fn								 TTMediaInfo GetMediaInfo();
	* \brief							 分析文件
	* \param[in]	aUrl				 文件路径
	*/
	virtual const TTMediaInfo&	     	 GetMediaInfo();

	/**
	* \fn								 TTUint MediaSize()
	* \brief							 获取媒体大小(字节)
	* \return							 媒体大小
	*/
	virtual TTUint						 MediaSize();

	/**
	* \fn								 TTBool IsSeekAble();
	* \brief							 是否可以进行Seek操作
	* \return                            ETTTrue为可以
	*/
	virtual TTBool						 IsSeekAble();
	/**
	* \fn								 void CreateFrameIndex();
	* \brief							 开始创建帧索引，对于没有索引的文件，需要异步建立索引，
	* \									 对于已有索引的文件，直接读取
	*/
	virtual void						 CreateFrameIndex();

	/**
	* \fn								 TTMediaInfo GetMediaSample();
	* \brief							 获得audio，video，等sample信息
	* \param[in]	aUrl				 文件路径
	*/
	virtual TTInt			 			 GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer);

	/**
	* \fn								 TTInt SelectStream()
	* \brief							 选择audio stream。
	* \param[in]	aStreamId			 Media Stream ID,或者audio或者video stream
	* \return							 返回状态
	*/
	virtual TTInt			     		 SelectStream(TTMediaType aType, TTInt aStreamId);
	
	/**
	* \fn								 void Seek(TTUint aPosMS);
	* \brief							 Seek操作
	* \param[in]	aPosMS				 位置，单位：毫秒
	* \return							 返回真正seek的位置，如果是负数，那是seek 失败。
	*/
	virtual TTInt64						 Seek(TTUint64 aPosMS,TTInt aOption = 0);


	virtual void						 SetNetWorkProxy(TTBool aNetWorkProxy);

	/**
	* \fn								 TTInt SetParam()
	* \brief							 设置参数。
	* \param[in]	aType				 参数类型
	* \param[in]	aParam				 参数值
	* \return							 返回状态
	*/
	virtual TTInt				     	 SetParam(TTInt aType, TTPtr aParam);

	/**
	* \fn								 TTInt GetParam()
	* \brief							 获取参数值。
	* \param[in]	aType				 参数类型
	* \param[in/out]	aParam			 参数值
	* \return							 返回状态
	*/
	virtual TTInt	     				 GetParam(TTInt aType, TTPtr aParam);

public:
	/**
	* \fn								 void CancelReader()
	* \brief							 取消网络reader
	*/
	virtual void						 CancelReader();

	/**
	* \fn								 TTUint BandWidth()
	* \brief							 获取网络下载带宽大小
	* \return							 缓冲大小(单位:字节)
	*/
	virtual TTUint						 BandWidth();

	/**
	* \fn								 TTUint BandPercent()
	* \brief							 获取网络下载数据百分比
	* \return							 缓冲大小(单位:字节)
	*/
	virtual TTUint						 BandPercent();


public:
	/**
	* \fn								 TTBool IsCreateFrameIdxComplete();
	* \brief							 建索引是否完成
	* \return							 完成为ETTrue
	*/
	virtual TTBool						 IsCreateFrameIdxComplete();

	/**
	* \fn								 TTUint BufferedSize()
	* \brief							 获取已缓冲的数据大小
	* \return							 已缓冲的数据大小
	*/
	virtual TTUint						 BufferedSize();


	virtual TTUint						 ProxySize();

	virtual void					     SetDownSpeed(TTInt aFast);

	/**
	* \fn								 TTInt BufferedPercent(TTInt& aBufferedPercent)
	* \brief							 获取已缓冲数据的百分比
	* \param[out]	aBufferedPercent	 百分比
	* \return							 操作状态
	*/
	virtual TTInt						 BufferedPercent(TTInt& aBufferedPercent);

	virtual void						 SetObserver(TTObserver*	aObserver);

public:// from ITTStreamBufferingObserver
	/**
	* \fn								 void StreamIsBuffering();
	* \brief						     网络数据真正缓冲	
	*/
	virtual void						 BufferingStart(TTInt nErr, TTInt nStatus, TTUint32 aParam);

	/**
	* \fn								 void StreamBufferCompleted();
	* \brief						 	 网络数据缓冲完成	
	*/
	virtual void						 BufferingDone();
    
	/**
	* \fn								 void DNSDone();
	* \brief							 DNS解析完成
	*/
	virtual void						 DNSDone();

	/**
	* \fn								 void ConnectDone();
	* \brief							 连接完成
	*/
	virtual void						 ConnectDone();

	/**
	* \fn								 void HttpHeaderReceived();
	* \brief							 Http头接收成功
	*/
	virtual void						 HttpHeaderReceived();

    /**
	* \fn								 void PrefetchStart(TTUint32 aParam);
	* \brief						 	 网络开始接收
	*/
    virtual void                    	 PrefetchStart(TTUint32 aParam);

	/**
	* \fn								 void PrefetchCompleted();
	* \brief							 预取缓存完成
	*/
	virtual void						 PrefetchCompleted();
	
	/**
	* \fn								 void StreamBufferCompleted();
	* \brief						 	 网络媒体数据全部完成	
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
