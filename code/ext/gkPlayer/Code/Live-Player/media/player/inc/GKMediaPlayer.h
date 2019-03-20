/**
* File : GKMediaPlayer.h
* Created on : 2011-03-03
* Description : CGKMediaPlayer
 */

#ifndef __GK_MEDIA_PLAYER_H__
#define __GK_MEDIA_PLAYER_H__

// INCLUDES
#include "GKMediaPlayerItf.h"
#include "GKCritical.h"
#include "GKArray.h"
#include "TTSrcDemux.h"
#include "TTBaseAudioSink.h"
#include "TTBaseVideoSink.h"
#include "TTEventThread.h"
#ifdef __TT_OS_ANDROID__
#include <jni.h>
#endif

enum GKSrcSwitchStatus
{
	ESwitchNone = 0
	, ESwitchInit = 1
	, ESwitchStarting = 2
	, ESwitchStarted = 3
	, ESwitchDoing = 4
	, ESwitchDone = 5
};


class CGKMediaPlayer : public IGKMediaPlayer
{

public:	// from ITTMediaPlayer
	/**
	* \fn                           TTUint Duration()
	* \brief                        获取媒体时长
	* \return   					媒体时长，单位毫秒
	*/
	virtual TTUint		            Duration();

    /**
	* \fn                           TTUint Size()
	* \brief                        获取媒体大小
	* \return   					媒体大小，单位字节
	*/
	virtual TTUint		            Size();

	/**
    * \fn                           const TTChar* Url()
    * \brief                        获取媒体路径
    * \return   					媒体路径
    */
    virtual const TTChar*           Url();
    
    /**
	* \fn                           TTInt SetDataSourceSync(const TTChar* aUrl)
	* \brief                        设置播放媒体路径
	* \param [in]   aUrl			媒体路径
	* \return						成功返回TTKErrNone,否则返回错误码
	*/
	virtual TTInt					SetDataSourceSync(const TTChar* aUrl, TTInt nFlag = 0);

	/**
	* \fn                           TTInt SetDataSourceAsync(const TTChar* aUrl)
	* \brief                        设置播放媒体路径
	* \param [in]   aUrl			媒体路径
	* \return						成功返回TTKErrNone,否则返回错误码
	*/
	virtual TTInt					SetDataSourceAsync(const TTChar* aUrl, TTInt nFlag = 0);

	/**
	* \fn                           void Play()
	* \brief                        Play
	* \return						成功返回TTKErrNone,否则返回错误码
	*/
	virtual TTInt					Play();

	/**
	* \fn                           void Pause()
	* \brief                        Pause
	*/
	virtual void					Pause(TTBool aFadeOut = ETTFalse);

	/**
	* \fn                           void Resume()
	* \brief                        Resume
	*/
	virtual void                    Resume(TTBool aFadeIn = ETTFalse);
	/**
	* \fn                           void Stop()
	* \brief                        Stop
	* \param [in]   aSync			是否同步stop，default是异步
	* \return						成功返回TTKErrNone,否则返回错误码
	*/
	virtual TTInt                   Stop(TTBool aSync = false);

	
	virtual void                    SetView(void* aView);

	/**
	* \fn                           void SetVolume(TTInt aLVolume, TTInt aRVolume);
	* \brief                        设置播放音量
	* \param [in]   aLVolume        音量值
	* \param [in]   aRVolume        音量值
	*/
	virtual void                    SetVolume(TTInt aLVolume, TTInt aRVolume);

	/**
	* \fn                           TInt GetVolume();
	* \brief                        获取播放音量
	* \return                       音量值
	*/
	virtual TTInt                   GetVolume();

	/**
	* \fn                           GKPlayStatus GetPlayStatus();
	* \brief                        获取播放状态
	* \return                       播放状态
	*/
	virtual GKPlayStatus            GetPlayStatus();

	/**
	* \fn                           TTInt SetParam(TTInt nID, void* aParam)
	* \brief                        设置参数
	* \param [in]   nID				参数ID，指定什么参数
	* \param [in]   aParam			参数值
	* \return						返回状态
	*/
	virtual TTInt					SetParam(TTInt nID, void* aParam);

	/**
	* \fn                           TTInt GetParam(TTInt nID, void* aParam)
	* \brief                        获取参数
	* \param [in]   nID				参数ID，指定什么参数
	* \param [in]   aParam			参数值
	* \return						返回状态
	*/
	virtual TTInt					GetParam(TTInt nID, void* aParam);

	/**
	* \fn                           void SetPosition(TTUint aPosition)
	* \brief                        Seek到指定播放位置
	* \param [in]   aPosition       播放位置，单位毫秒
	* \param [in]   aOption	        播放属性，快速seek还是准确seek
	*/
	virtual TTInt64                 SetPosition(TTInt64 aPosition, TTInt aOption = 0);

	/**
	* \fn                           TTUint GetPosition()
	* \brief                        获取播放进度
	* \return                       播放进度
	*/
	virtual TTUint64				GetPosition();

	/**
	* \fn                           TTInt GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum);
	* \brief                        获取频域和时域数据
	* \param[out]    aFreq          频域数据
	* \param[out]    aWave          时域数据
	* \param[in]     aSampleNum     采样大小
	* \return                       获取成功返回TTKErrNone,否则返回错误码
	*/
	virtual TTInt                   GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum);

	/**
	* \fn							void SetPlayRange(TTUint aStartTime, TTUint aEndTime)
	* \brief						设置播放区间
	* \param[in]	aStartTime		播放区间起始时间
	* \param[in]	aEndTime		播放区间结束时间
	*/
	virtual void					SetPlayRange(TTUint aStartTime, TTUint aEndTime);

	/**
	* \fn							TTUint BufferedSize()
	* \brief						获取缓冲大小
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BufferedSize();

	/**
	* \fn							TTUint BandPercent()
	* \brief						获取网络下载百分比
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BandPercent();


		/**
	* \fn							TTUint BandWidth()
	* \brief						获取网络下载带宽大小
	* \return						缓冲大小(单位:字节)
	*/
	virtual	TTUint					BandWidth();

	/**
	* \fn							TTInt BufferedPercent(TTInt& aBufferedPercent)
	* \brief						获取缓冲进度
	* \param[out]	aBufferedPercent缓冲进度
	* \return						获取成功返回TTKErrNone,否则返回错误码
	*/
	virtual	TTInt					BufferedPercent(TTInt& aBufferedPercent);

	/**
	* \fn							void SetActiveNetWorkType(TTActiveNetWorkType aNetWorkType)
	* \brief						设置网络类型
	* \param[in]	aNetWorkType	网络类型
	*/
	virtual void					SetActiveNetWorkType(GKActiveNetWorkType aNetWorkType);

	/**
	* \fn							void SetNetWorkProxy(TTBool aNetWorkProxy)
	* \brief						设置网络连接类型
	* \param[in]	aNetWorkType	网络连接类型
	*/
	virtual void					SetNetWorkProxy(TTBool aNetWorkProxy);

	/**
	* \fn							void SetDecoderType(GKDecoderType aDecoderType)
	* \brief						设置解码类型
	* \param[in]	aDecoderType	网络连接类型
	*/
	virtual void					SetDecoderType(GKDecoderType aDecoderType);

#ifdef __TT_OS_ANDROID__
	/**
	* \fn							void SaveAudioTrackJClass(void* pJclass)
	* \brief						保存JNI相关信息
	* \param[in]	pJclass	        java层 AudioTrack class
	*/
	virtual	void					SaveAudioTrackJClass(void* pJclass);

    /**
     * \fn							void GetAudiotrackClass();
     * \brief						获取java层 AudioTrack class,实现ITTMediaPlayer对应接口
	 * \return                      AudioTrack class pointer
     */
	virtual void*					GetAudiotrackClass();


	virtual void 				    SetMaxOutPutSamplerate(TTInt aSampleRate);

	/**
	* \fn							void SaveVideoTrackJClass(void* pJclass)
	* \brief						保存JNI相关信息
	* \param[in]	pJclass	        java层 VideoTrack class
	*/
	virtual	void					SaveVideoTrackJClass(void* pJclass);

    /**
     * \fn							void GetVideotrackClass();
     * \brief						获取java层 VideoTrack class,实现ITTMediaPlayer对应接口
	 * \return                      VideoTrack class pointer
     */
	virtual void*					GetVideotrackClass();
#endif


#ifdef __TT_OS_IOS__
    /**
     * \fn						void SetBalanceChannel(float aVolume)
     * \brief                          set balane channel volume
     * \param[in] aVolume  ,value range: [-1,+1]
     */
    virtual void                    SetBalanceChannel(float aVolume);
    
    virtual void                    SetRotate();
    
    virtual void                    SetVolume(float aVolume);
    
    virtual TTBool                  IsLiveMode();
    
    virtual TTInt                   GetAVPlayType();
    
#endif
    
    virtual void                    SetRendType(TTInt aRenderType);
    
    virtual void                    SetMotionEnable(bool aEnable);
    
    virtual void                    setTouchEnable(bool aEnable);

	/**
	* \fn                           void SetCacheFilePath(const TTChar* aCacheFilePath)
	* \brief                        设置缓存文件路径
	* \param[in] aCacheFilePath     缓存文件路径
	*/
	void							SetCacheFilePath(const TTChar* aCacheFilePath);

#ifdef __MEDIA_PLAYER__
		/**
	* \fn							void Decode(const TTChar* aUrl, TTInt size)
	* \brief					    外部调用解码接口
	* \param[in] aUrl			    源文件url	
	* \param[in] size			    需要获取的pcm数据size	
	* \return						解码结果		
	*/
	virtual	TTInt					Decode(const TTChar* aUrl, TTInt size);

	/**
	* \fn							TTInt GetPCMData();
	* \brief						获取解码数据
	* \return						解码数据量		
	*/
	virtual TTUint8*				GetPCMData();

	/**
	* \fn							TTInt GetPCMDataSize();
	* \brief						获取解码数据量
	* \return						解码数据量值		
	*/
	virtual TTInt				    GetPCMDataSize();

	/**
	* \fn							void GetChannels();
	* \brief			            获取声道数
	* \return						声道值	
	*/
	virtual TTInt				    GetPCMDataChannle();

	/**
	* \fn							void GetSamplerate();
	* \brief			            获取采样率
	* \return						采样率值	
	*/
	virtual TTInt				    GetPCMDataSamplerate();

	/**
	* \fn							void CancelGetPCM();
	* \brief			            取消操作	
	*/
	virtual void					CancelGetPCM();

	/**
	* \fn							void SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos)
	* \brief						设置播放范围
	* \param[in] start			    起始值	
	* \param[in] end			    结束值	
	* \param[in] decodeStartPos		起始解码位置值	
	*/
	virtual void					SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos);

	virtual TTInt					DecodeEntireFile(const TTChar* aUrl, const TTChar* OutPutPath);

	virtual	TTInt					GetDataFromSpecifiedTimePos(const TTChar* aUrl, TTInt timePos, TTInt duration);
#endif



public:	// construction and destruction

    CGKMediaPlayer(IGKMediaPlayerObserver* aPlayerObserver, const TTChar* aPluginPath);

    ~CGKMediaPlayer();

public:

	virtual TTInt					postSetDataSourceEvent (TTInt  nDelayTime);
	virtual TTInt					postStopEvent (TTInt  nDelayTime);
	virtual TTInt					postMsgEvent (TTInt  nDelayTime, TTInt32 nID, int nParam1, int nParam2, void * pParam3);
	virtual TTInt					postSCMsgEvent (TTInt  nDelayTime, TTInt32 nID, int nParam1, int nParam2, void * pParam3);
	virtual TTInt					postPreSrcEvent (TTInt  nDelayTime, TTInt32 nID, int nParam1, int nParam2, void * pParam3);

	static	TTInt		 			ttSrcCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	static	TTInt		 			ttAudioCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	static	TTInt		 			ttVideoCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);

	static	TTInt		 			ttSCSrcCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	static	TTInt		 			ttPreSrcCallBack (void* pUserData, TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);

	virtual	TTInt		 			handlePreSrcMsg (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	virtual	TTInt		 			handleSCSrcMsg (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	virtual	TTInt		 			handleSrcMsg (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	virtual	TTInt		 			handleAudioMsg (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);
	virtual	TTInt		 			handleVideoMsg (TTInt32 nID, TTInt32 nParam1, TTInt32 nParam2, void * pParam3);

private:

	TTInt							SetDataSource();
	void							InitSink();
	void							updateView();
	TTInt							onSCEvent(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt							onPreSource(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt							onNotifyEvent(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt							onSetDataSource(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt							onStop(TTInt nMsg, TTInt nVar1, TTInt nVar2, void* nVar3);
	TTInt64							seek(TTInt64 aPosition, TTInt aOption);
	TTInt							doStop(TTBool aIntra);

	TTInt							SetPlayStatus(GKPlayStatus status);

	void							setSeekStatus(TTBool bSeeking);
	TTBool							getSeekable();

	TTInt64							GetPlayTime();
    
    int                             IsLimited();
	
private:

	TTChar							mCacheFilePath[FILE_PATH_MAX_LENGTH];
	TTChar							mPluginPath[FILE_PATH_MAX_LENGTH];
    RGKCritical						mCritical;
	RGKCritical						mCriVideo;
	TTChar*							mUrl;				
	RGKCritical						mCriStatus;
	GKPlayStatus					mPlayStatus;
	TTBool                          mFakePauseFlag;
	TTBool                          mFakeStopFlag;
	TTBool							mSeekable;
	TTBool							mSeeking;
    
    TTInt                           mRenderType;
    TTBool                          mMotionEnable;
    TTBool                          mTouchEnable;

	TTEventThread*					mMsgThread;
	TTEventThread*					mHandleThread;
	TTCBaseAudioSink*				mAudioSink;
	TTCBaseVideoSink*				mVideoSink;
	CTTSrcDemux*					mSrcDemux;
	TTInt							mAudioStreamId;
	TTInt							mVideoStreamId;
//	TTInt							mLastPlayTime;
//	TTInt64							mLastSystemTime;

	CTTSrcDemux*					mPreDemux;
	TTChar*							mPreUrl;
	TTInt							mPreSrcFlag;

	RGKCritical						mCriSCSrc;
	CTTSrcDemux*					mSCDemux;
	TTInt							mSCDoing;
	TTChar*							mSCUrl;
	TTInt							mSCSrcFlag;
	TTInt64							mSCSeekTime;

	TTInt							mException;
	TTInt							mSeekOption;
	TTInt64							mPrePlayPos;

	IGKMediaPlayerObserver*			mPlayerObserver;

	RGKCritical						mCriEvent;
	TTObserver						mSrcObserver;
	TTObserver						mSCSrcObserver;
	TTObserver						mPreSrcObserver;
	TTObserver						mAudioObserver;
	TTObserver						mVideoObserver;
	TTPlayRange						mPlayRange;

	TTInt							mSrcFlag;
	GKDecoderType					mDecoderType;
	void*							mView;
    
    TTInt                           mAVTypeFlag;

#ifdef __TT_OS_ANDROID__
	void*							mpAudiotrackJClass;
	void*							mpVideotrackJClass;
	jobject							mSurfaceObj;
#endif
};

class TTCMediaPlayerEvent : public TTBaseEventItem
{
public:
    TTCMediaPlayerEvent(CGKMediaPlayer * pMediaPlayer, TTInt (CGKMediaPlayer::* method)(TTInt, TTInt, TTInt, void*),
					    TTInt nType, TTInt nMsg = 0, TTInt nVar1 = 0, TTInt nVar2 = 0, void* nVar3 = 0)
		: TTBaseEventItem (nType, nMsg, nVar1, nVar2)
	{
		mPlayer = pMediaPlayer;
		mMethod = method;
    }

    virtual ~TTCMediaPlayerEvent()
	{
	}

    virtual void fire (void) 
	{
        (mPlayer->*mMethod)(mMsg, mVar1, mVar2, mVar3);
    }

protected:
    CGKMediaPlayer *		mPlayer;
    int (CGKMediaPlayer::* mMethod) (TTInt, TTInt, TTInt, void*);
};


#endif //__TT_MEDIA_PLAYER_H__
