/**
* File : TTRtmpInfoProxy.h 
* Created on : 2015-5-12
* Author : kevin
* Copyright : Copyright (c) 2015 GoKu Ltd. All rights reserved.
* Description : CTTRtmpInfoProxy定义文件
*/

#ifndef __TT_RTMP_INFO_PROXY_H__
#define __TT_RTMP_INFO_PROXY_H__

#include "GKTypedef.h"
#include "TTMediainfoDef.h"
#include "TTStreamBufferingItf.h"
#include "TTIMediaInfo.h"
#include "GKCritical.h"
#include "TTEventThread.h"
#include "TTSemaphore.h"
#include "TTList.h"
#include "TTRtmpDownload.h"
#include "TTFLVTag.h"
#include "TTMediaParserItf.h"


class CTTRtmpInfoProxy : public ITTStreamBufferingObserver, public ITTMediaInfo,public ITTMediaParserObserver
{
public:

	/**
	* \fn								 CTTRtmpInfoProxy(TTObserver& aObserver);
	* \brief							 构造函数
	* \param[in]   aObserver			 Observer引用
	*/
	CTTRtmpInfoProxy(TTObserver* aObserver);

	/**
	* \fn								 ~CTTRtmpInfoProxy();
	* \brief							 析构函数
	*/
	virtual ~CTTRtmpInfoProxy();

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

public:
    virtual void						 CreateFrameIdxComplete(){};
    virtual void                         BufferingEmtpy(TTInt nErr, TTInt nStatus, TTUint32 aParam);
    virtual void                         BufferingReady();
    
    int                                  AddAudioTag(unsigned char *data, unsigned int size, TTInt64 timeUs);
    int                                  AddVideoTag(unsigned char *data, unsigned int size, TTInt64 timeUs);
    
    void                                 DiscardUselessBuffer();
    void                                 SendBufferStartEvent();
    void                                 EOFNotify();
    
protected:
	int									 IsRtmpSource(const TTChar* aUrl);

private:
	TTObserver*				iObserver;
	TTMediaInfo				iMediaInfo;
	TTInt					mCurMinBuffer;
	TTInt					mCurBandWidth;

	TTInt64					mFirstTimeOffsetUs;

	TTInt					mCancel;
	RTTSemaphore			mSemaphore;
	RGKCritical				mCriEvent;
    RGKCritical				mCriStatus;

	CTTRtmpDownload*        iRtmpDownload;
    
    TTInt					iBufferStatus;
    TTInt				    iCountAMAX;
    TTInt				    iCountVMAX;
    
    CTTFlvTagStream*		iAudioStream;
    CTTFlvTagStream*		iVideoStream;
};

#endif 