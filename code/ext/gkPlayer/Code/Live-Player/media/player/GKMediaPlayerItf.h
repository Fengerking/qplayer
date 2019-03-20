/**
* File : GKMediaPlayerItf.h
* Created on : 2011-3-11
* Description : IGKMediaPlayer
*/

#ifndef  __GK_MEDIA_PLAYER_ITF_H__
#define  __GK_MEDIA_PLAYER_ITF_H__

#include "GKOsalConfig.h"
#include "GKMacrodef.h"
#include "GKTypedef.h"
#include "GKInterface.h"
#include "GKNetWorkConfig.h"
#include "GKMsgEnumDef.h"


class IGKMediaPlayerObserver
{
public:
	/**
	* \fn                           void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3)
	* \brief                        播放开始后，状态改变通知上层
	* \param [in]      aMsg   		操作ID
	* \param [in]	   aArg1        参数1
	* \param [in]	   aArg2		参数2
	* \param [in]	   aArg3		参数3
	*/
	virtual void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3) = 0;
};

class IGKMediaPlayer : public IGKInterface
{
public:
	/**
	* \fn                           TTUint Duration
	* \brief                        媒体时长
	* \return   					时长，毫秒为单位
	*/
	virtual TTUint		            Duration() = 0;

    /**
	* \fn                           TTUint Size()
	* \brief                        获取媒体大小
	* \return   					媒体大小，单位字节
	*/
	virtual TTUint		            Size() = 0;

    /**
    * \fn                           const TTChar* Url()
    * \brief                        查询媒体名字
    * \return   					媒体名
    */
    virtual const TTChar*           Url() = 0;
    
	/**
	* \fn                           TTInt SetDataSourceSync(const TTChar* aUrl)
	* \brief                        异步设置Src
	* \param [in]   aUrl			媒体路径
	* \return						返回状态
	*/
	virtual TTInt					SetDataSourceSync(const TTChar* aUrl, TTInt nFlag = 0) = 0;

	/**
	* \fn                           TTInt SetDataSourceAsync(const TTChar* aUrl)
	* \brief                        异步设置src
	* \param [in]   aUrl			媒体路径
	* \return						返回状态
	*/
	virtual TTInt					SetDataSourceAsync(const TTChar* aUrl, TTInt nFlag = 0) = 0;

	/**
	* \fn                           TTInt SetParam(TTInt nID, void* aParam)
	* \brief                        设置参数
	* \param [in]   nID				参数ID，指定什么参数
	* \param [in]   aParam			参数值
	* \return						返回状态
	*/
	virtual TTInt					SetParam(TTInt nID, void* aParam) = 0;

	/**
	* \fn                           TTInt GetParam(TTInt nID, void* aParam)
	* \brief                        获取参数
	* \param [in]   nID				参数ID，指定什么参数
	* \param [in]   aParam			参数值
	* \return						返回状态
	*/
	virtual TTInt					GetParam(TTInt nID, void* aParam) = 0;

	/**
	* \fn                           void Play()
	* \brief                        启动播放
	* \return						返回状态
	*/
	virtual TTInt					Play() = 0;

	/**
	* \fn                           void Pause()
	* \brief                        Pause
	*/
	virtual void					Pause(TTBool aFadeOut = ETTFalse)= 0;

	/**
	* \fn                           void Resume()
	* \brief                        Resume
	*/
	virtual void                    Resume(TTBool aFadeIn = ETTFalse)= 0;
	/**
	* \fn                           void Stop()
	* \brief                        停止播放
	* \param [in]   aSync			是否同步stop，default是异步
	* \return                       操作状态
	*/
	virtual TTInt                   Stop(TTBool aSync = false) = 0;

	/**
	* \fn                           void SetVolume(TTInt aLVolume, TTInt aRVolume);
	* \brief                        设置音量
	* \param [in]   aLVolume         音量值
	* \param [in]   aRVolume         音量值
	*/
	virtual void                    SetVolume(TTInt aLVolume, TTInt aRVolume) = 0;

	/**
	* \fn                           TInt GetVolume();
	* \brief                        获取音量
	* \return                       音量值
	*/
	virtual TTInt                   GetVolume() = 0;

	/**
	* \fn                           void SetView(TTInt aLVolume, TTInt aRVolume);
	* \brief                        设置音量
	* \param [in]   aLVolume         音量值
	* \param [in]   aRVolume         音量值
	*/
	virtual void                    SetView(void* aView) = 0;

	/**
	* \fn                           GKPlayStatus GetPlayStatus();
	* \brief                        获取播放器状态
	* \return                       状态
	*/
	virtual GKPlayStatus            GetPlayStatus() = 0;

	/**
	* \fn                           void SetPosition(TTUint aPosition)
	* \brief                        设置播放位置
	* \param [in]   aPosition       位置时间，以毫秒为单位
	* \param [in]   aOption	        播放属性，快速seek还是准确seek
	*/
	virtual TTInt64                 SetPosition(TTInt64 aPosition, TTInt aOption = 0) = 0;

	/**
	* \fn                           TTUint GetPosition()
	* \brief                        获取播放时间
	* \return                       位置时间，以毫秒为单位
	*/
	virtual TTUint64				GetPosition() = 0;

	/**
	* \fn                           SetVolumeRamp(TTUint aFadeIn, TTUint aFadeOut)
	* \brief                        设置声音淡入淡出时间
	* \param [in]   aFadeIn         淡入时间
	* \param [in]   aFadeOut        淡出时间
	*/
	//virtual void                  SetVolumeRamp(TTUint aFadeIn,  TTUint aFadeOut) = 0;

	/**
	* \fn                           void SetPlayWindow(TTInt aReaptedNum, TTUint aStart, TTUint aEnd)
	* \brief                        设置重复播放次数及起始时间
	* \param [in]   aReaptedNum     复读次数
	* \param [in]   aStart          复读起始时间
	* \param [in]   aEnd            复读终止时间
	*/
	//virtual void                  SetPlayWindow(TTInt aReaptedNum, TTUint aStart, TTUint aEnd) = 0;

	/**
	* \fn                           void RepeatCancel()
	* \brief                        取消重复播放
	*/
	//virtual void                  RepeatCancel() = 0;

	/**
	* \fn                           TTInt GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum);
	* \brief                        获取当前频谱
	* \param[out]    aFreq          频谱数据指针，不需要的话输入NULL
	* \param[out]    aWave          波形数据指针,必须非空
	* \param[in]     aSampleNum     采样点的个数
	* \return                       操作状态
	*/
	virtual TTInt                   GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum) = 0;

	/**
	* \fn							void SetPlayRange(TTUint aStartTime, TTUint aEndTime);
	* \brief						设置播放范围
	* \param[in]	aStartTime		播放范围起始时间，单位毫秒
	* \param[in]	aEndTime		播放范围结束时间，单位毫秒
	*/
	virtual void					SetPlayRange(TTUint aStartTime, TTUint aEndTime) = 0;

	/**
	* \fn							TTUint BufferedSize()
	* \brief						获取缓冲大小
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BufferedSize() = 0;


	/**
	* \fn							TTUint BandWidth()
	* \brief						获取网络下载带宽大小
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BandWidth() = 0;

	/**
	* \fn							TTUint BandPercent()
	* \brief						获取网络下载百分比
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BandPercent() = 0;

	/**
	* \fn							TTInt BufferedPercent(TTInt& aBufferedPercent)
	* \brief						获取以缓冲的数据
	* \param[out]	aBufferedPercent百分比
	* \return						操作状态
	*/
	virtual TTInt					BufferedPercent(TTInt& aBufferedPercent) = 0;

	/**
	* \fn							void SetActiveNetWorkType(TTActiveNetWorkType aNetWorkType)
	* \brief						设置网络连接类型
	* \param[in]	aNetWorkType	网络连接类型
	*/
	virtual void					SetActiveNetWorkType(GKActiveNetWorkType aNetWorkType) = 0;


	/**
	* \fn							void SetNetWorkProxy(TTBool aNetWorkProxy)
	* \brief						设置网络是否代理
	* \param[in]	aNetWorkProxy	网络是否代理
	*/
	virtual void					SetNetWorkProxy(TTBool aNetWorkProxy) = 0;

	/**
	* \fn							void SetDecoderType(GKDecoderType aDecoderType)
	* \brief						设置解码类型
	* \param[in]	aDecoderType	网络连接类型
	*/
	virtual void					SetDecoderType(GKDecoderType aDecoderType) = 0;

#ifdef __TT_OS_IOS__
    /**
     * \fn						void SetBalanceChannel(float aVolume)
     * \brief                          set balane channel volume
     * \param[in] aVolume  ,value range: [-1,+1]
     */
    virtual void                    SetBalanceChannel(float aVolume) = 0;
    
    virtual void                    SetRotate() = 0;
    
    virtual void                    SetVolume(float aVolume) = 0;
    
    virtual TTBool                  IsLiveMode() = 0;
    
    virtual TTInt                   GetAVPlayType()= 0;
#endif
    
    virtual void                    SetRendType(TTInt aRenderType) = 0;
    
    
    virtual void                    SetMotionEnable(bool aEnable) = 0;
    
    virtual void                    setTouchEnable(bool aEnable) = 0;

	/**
	* \fn                           void SetCacheFilePath(const TTChar* aCacheFilePath)
	* \brief                        设置网络试听缓冲文件的路径
	* \param[in] aCacheFilePath     文件路径
	*/
	virtual void					SetCacheFilePath(const TTChar* aCacheFilePath) = 0;
#ifdef __MEDIA_PLAYER__
		/**
	* \fn							void Decode(const TTChar* aUrl, TTInt size)
	* \brief					    外部调用解码接口
	* \param[in] aUrl			    源文件url	
	* \param[in] size			    需要获取的pcm数据size	
	* \return						解码结果		
	*/
	virtual	TTInt					Decode(const TTChar* aUrl, TTInt size)= 0;

	/**
	* \fn							TTInt GetPCMData();
	* \brief						获取解码数据
	* \return						解码数据量		
	*/
	virtual TTUint8*				GetPCMData()= 0;

	/**
	* \fn							TTInt GetPCMDataSize();
	* \brief						获取解码数据量
	* \return						解码数据量值		
	*/
	virtual TTInt				    GetPCMDataSize()= 0;

	/**
	* \fn							void GetChannels();
	* \brief			            获取声道数
	* \return						声道值	
	*/
	virtual TTInt				    GetPCMDataChannle()= 0;

	/**
	* \fn							void GetSamplerate();
	* \brief			            获取采样率
	* \return						采样率值	
	*/
	virtual TTInt				    GetPCMDataSamplerate()= 0;

	/**
	* \fn							void CancelGetPCM();
	* \brief			            取消操作	
	*/
	virtual void					CancelGetPCM()= 0;

	/**
	* \fn							void SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos)
	* \brief						设置播放范围
	* \param[in] start			    起始值	
	* \param[in] end			    结束值	
	* \param[in] decodeStartPos		起始解码位置值	
	*/
	virtual void					SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos)= 0;

	virtual TTInt					DecodeEntireFile(const TTChar* aUrl, const TTChar* OutPutPath)= 0;

	virtual	TTInt					GetDataFromSpecifiedTimePos(const TTChar* aUrl, TTInt timePos, TTInt duration) = 0;
#endif

#ifdef __TT_OS_ANDROID__
	/**
	* \fn							void SaveAudioTrackJClass(void* pJclass)
	* \brief						保存JNI相关信息
	* \param[in]	pJclass	        java层 AudioTrack class
	*/
	virtual	void					SaveAudioTrackJClass(void* pJclass) = 0;

    /**
     * \fn							void GetAudiotrackClass();
     * \brief						获取java层 AudioTrack class,实现ITTMediaPlayer对应接口
	 * \return                      AudioTrack class pointer
     */
	virtual void*					GetAudiotrackClass() = 0;


	virtual void 				    SetMaxOutPutSamplerate(TTInt aSampleRate) =0;

	/**
	* \fn							void SaveVideoTrackJClass(void* pJclass)
	* \brief						保存JNI相关信息
	* \param[in]	pJclass	        java层 VideoTrack class
	*/
	virtual	void					SaveVideoTrackJClass(void* pJclass) = 0;

    /**
     * \fn							void GetVideotrackClass();
     * \brief						获取java层 VideoTrack class,实现ITTMediaPlayer对应接口
	 * \return                      VideoTrack class pointer
     */
	virtual void*					GetVideotrackClass() = 0;

#endif

};


class CGKMediaPlayerFactory
{

public:

	/**
	* \fn                               ITTMediaPlayer* NewL(ITTMediaPlayerObserver* aPlayerObserver)
	* \brief                            创建ITTMediaPlayer接口对象
	* \param[in]	aPlayerObserver		MediaPlayer观察者
	* \return                           ITTMediaPlayer接口对象指针
	*/
	static IGKMediaPlayer*              NewL(IGKMediaPlayerObserver* aPlayerObserver);
};

#endif
