/**
* File : TTBufferReaderProxy.h  
* Created on : 2014-10-21
* Author : yongping.lin
* Description : CTTHttpReaderProxy定义文件
*/
#ifndef __TT_BUFFER_READER_PROXY_READER_H__
#define __TT_BUFFER_READER_PROXY_READER_H__

#include "GKInterface.h"
#include "TThread.h"
#include "GKCritical.h"
#include "TTSemaphore.h"
#include "TTDataReaderItf.h"
#include "TTStreamBufferingItf.h"

class CTTHttpClient;
class CTTCacheBuffer;

class CTTBufferReaderProxy : public IGKInterface
{	
public:
	/**
	* \fn							CTTBufferReaderProxy();
	* \brief						构造函数	
	*/
	CTTBufferReaderProxy();

	/**
	* \fn							~CTTBufferReaderProxy();
	* \brief						析构函数	
	*/
	virtual ~CTTBufferReaderProxy();

	/**
	* \fn
	* \brief						获取Url
	* \return						Url
	*/
	TTChar*							Url();

	/**
	* \fn							TTInt Open(const TTChar* aUrl);
	* \brief						打开Http流	
	* \param[in]  aUrl				Http流路径
	* \return						操作状态
	*/
	TTInt							Open(const TTChar* aUrl);

	/**
	* \fn							TTInt Close();
	* \brief						关闭Http流
	*/
	TTInt							Close();

	/**
	* \fn						    void Cancel()
	* \brief                        取消文件读取
	*/
    void                            Cancel();

	/**
	* \fn							TTInt Read(TTUint8* aReadBuffer, TTInt aReadPos, TTInt aReadSize);
	* \brief						读取文件	
	* \param[in]  aReadBuffer		Http流路径
	* \param[in]  aReadPos			读取位置
	* \param[in]  aReadSize			读取大小
	* \return						操作状态
	*/
	TTInt							Read(TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize);


	TTInt							ReadWait(TTUint8* aReadBuffer, TTInt64 aReadPos, TTInt aReadSize);

	/**
	* \fn							TTInt Size() const;
	* \brief						获取文件大小	
	* \return						文件大小
	*/
	TTInt64							Size() const;

	/**
	* \fn							TTInt BufferedSize() const;
	* \brief						当前已经缓冲的数据	
	* \return						缓冲的数据大小
	*/
	TTInt64							BufferedSize();

	void							SetDownSpeed(TTInt aFast);


	TTUint							ProxySize();


	TTUint							BandWidth();

	TTUint							BandPercent();

	/**
	* \fn							void SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver);
	* \brief						设置数据流需要缓冲的监控接口	
	*/
	void							SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver);
    
    /**
     * \fn							CheckOnLineBuffering	()
     * \brief
     */
    void                            CheckOnLineBuffering();

	/**
     * \fn							SetBitrate	()
     * \brief
     */
	void							SetBitrate(TTInt aMediaType, TTInt aBitrate);

	TTInt							GetStatusCode();

	TTUint							GetHostIP();

	void							SetNetWorkProxy(TTBool aNetWorkProxy);

	TTInt							PrepareCache(TTInt64 aReadPos, TTInt aReadSize, TTInt aFlag);

private:
    enum TTReadStatus 
	{
		ETTReadStatusNotReady = 0,
		ETTReadStatusReading,
		ETTReadStatusToStopRead
	};
    
    enum TTDataReceiveStatus
    {
		ETTDataReceiveStatusPrefetchReceiving = 0,
        ETTDataReceiveStatusPrefetchReceived
    };
    
    enum TTBufferingStatus
    {
        ETTBufferingInValid = -1,
        ETTBufferingStart = 0,
        ETTBufferingDone
    };

private:
	static void* 					DownloadThreadProc(void* aPtr);
	void 							DownloadThreadProcL(void* aPtr);
	void							DownloadThreadChunk(void* aPtr);
	void							DownloadThreadLive(void* aPtr);

	TTBool							IsDesiredDataBuffering(TTInt64 aDesiredPos, TTInt aDesiredSize);
	TTBool							IsDesiredNewRequire(TTInt64 aDesiredPos, TTInt aDesiredSize, TTInt aLevel);
    TTBool							IsBuffering();
	TTBool							ProcessBufferingIssue(TTInt64 aDesireBufferedPos, TTInt aDesiredBufferedSize);

	TTBool							ParseUrl(const TTChar* aUrl);
	void							CheckBufferingDone();
    
    TTInt							ReConnectServer(TTInt64 aOffset);

	TTInt							WriteChunk(TTUint8* aBuffer, TTInt aWritePos, TTInt aWriteSize, TTInt& nWrited);
	
	TTInt							GetChunkSize(TTUint8* aBuffer, TTInt aWritePos);

	TTInt							ConvertToValue(TTChar* aBuffer);

private:
	TTChar*						iUrl;
	CTTHttpClient*				iHttpClient;
	CTTCacheBuffer*				iCacheBuffer;
	TTReadStatus				iReadStatus;		/**< 线程间共享，需要加锁 */
	TTBufferingStatus           iBufferStatus;		/**< 线程间共享，需要加锁 */
	TTDataReceiveStatus         iDataReceiveStatus;
	TTInt						iPrefetchSize;
	RGKCritical					iCritical;
	RTTSemaphore				iSemaphore;
	ITTStreamBufferingObserver*	iStreamBufferingObserver;
	TTBool				        iCancel;
    RTThread					iThreadHandle;
    TTInt64                     iCurrentReadDesiredPos;
	TTInt                       iCurrentReadDesiredSize;	
	TTInt64						iBandWidthTimeStart;
	TTInt64						iBandWidthSize;
	TTInt						iBandWidthData;
	TTInt						iStartBuffering;
	TTInt						iFastDownLoad;


	TTBool						iNetWorkChanged;
	TTBool						iNetUseProxy;
	TTInt						iProxySize;
	TTInt						iStatusCode;
	TTUint						iHostIP;
	TTInt64						iOffSet;
	TTInt64						iNewOffSet;
	TTChar*						iCacheUrl;

	TTInt						iChunkSize;
	TTInt						iChunkLeft;
	TTInt						iBufferLeft;

	TTInt						iRevCount;
	TTInt						iRevTime[MAX_RECEIVED_COUNT];
	TTInt						iRevSize[MAX_RECEIVED_COUNT];

	TTInt						iAudioBitrate;
	TTInt						iVideoBitrate;
};

#endif
