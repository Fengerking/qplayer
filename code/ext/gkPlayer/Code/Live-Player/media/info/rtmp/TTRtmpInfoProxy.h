/**
* File : TTRtmpInfoProxy.h 
* Created on : 2015-5-12
* Author : kevin
* Copyright : Copyright (c) 2015 GoKu Ltd. All rights reserved.
* Description : CTTRtmpInfoProxy�����ļ�
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
	* \brief							 ���캯��
	* \param[in]   aObserver			 Observer����
	*/
	CTTRtmpInfoProxy(TTObserver* aObserver);

	/**
	* \fn								 ~CTTRtmpInfoProxy();
	* \brief							 ��������
	*/
	virtual ~CTTRtmpInfoProxy();

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