#ifndef __TT_RTMPDOWNLOAD_PROXY_H__
#define __TT_RTMPDOWNLOAD_PROXY_H__

#include "GKInterface.h"
#include "TThread.h"
#include "GKCritical.h"
#include "TTSemaphore.h"
#include "TTDataReaderItf.h"
#include "TTStreamBufferingItf.h"

#include "rtmp_sys.h"
#include "rtmpamf.h"

class CTTCacheBuffer;
class CTTRtmpInfoProxy;

class CTTRtmpDownload : public IGKInterface
{	
public:
	/**
	* \fn							CTTRtmpDownload();
	* \brief						构造函数	
	*/
	CTTRtmpDownload(CTTRtmpInfoProxy* aRtmpProxy);

	/**
	* \fn							~CTTRtmpDownload();
	* \brief						析构函数	
	*/
	virtual ~CTTRtmpDownload();

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

    void                            Cancel();

	TTUint							ProxySize();

	TTUint							BandWidth();

	/**
	* \fn							void SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver);
	* \brief						设置数据流需要缓冲的监控接口	
	*/
	void							SetStreamBufferingObserver(ITTStreamBufferingObserver* aObserver);

	TTUint							GetHostIP();

	void							SetNetWorkProxy(TTBool aNetWorkProxy);

	TTInt							ConnectRtmpServer();

	TTInt							ReceiveNetData();

private:
    enum TTReadStatus 
	{
		ETTReadStatusNotReady = 0,
		ETTReadStatusReading,
		ETTReadStatusToStopRead
	};

private:
	static void* 					DownloadThreadProc(void* aPtr);
	void 							DownloadThreadProcL(void* aPtr);
    TTInt                           WriteData(unsigned char * aData, int size);
    
    TTInt							ReConnectServer();

private:
	TTChar*						iUrl;
	TTReadStatus				iReadStatus;		/**< 线程间共享，需要加锁 */

	RGKCritical					iCritical;
	RTTSemaphore				iSemaphore;
	ITTStreamBufferingObserver*	iStreamBufferingObserver;
	TTBool				        iCancel;
    RTThread					iThreadHandle;
	TTInt64						iBandWidthTimeStart;
	TTInt64						iBandWidthSize;
	TTInt						iBandWidthData;

	TTBool						iNetUseProxy;
	TTInt						iProxySize;
	TTChar*						iCacheUrl;

	TTInt						iBufferSize;
	unsigned char *				iBuffer;

	RTMP						iRtmp;
    CTTRtmpInfoProxy*           iRtmpInfoProxy;
    TTBool                      iFirstPacket;
	TTBool                      iPrefechComplete;
	TTInt						iErrorCode;
	pthread_t					iConnectionTid;
	TTInt						iWSAStartup;
};

#endif