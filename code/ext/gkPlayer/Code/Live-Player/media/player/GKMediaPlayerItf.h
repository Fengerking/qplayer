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
	* \brief                        ���ſ�ʼ��״̬�ı�֪ͨ�ϲ�
	* \param [in]      aMsg   		����ID
	* \param [in]	   aArg1        ����1
	* \param [in]	   aArg2		����2
	* \param [in]	   aArg3		����3
	*/
	virtual void PlayerNotifyEvent(GKNotifyMsg aMsg, TTInt aArg1, TTInt aArg2, const TTChar* aArg3) = 0;
};

class IGKMediaPlayer : public IGKInterface
{
public:
	/**
	* \fn                           TTUint Duration
	* \brief                        ý��ʱ��
	* \return   					ʱ��������Ϊ��λ
	*/
	virtual TTUint		            Duration() = 0;

    /**
	* \fn                           TTUint Size()
	* \brief                        ��ȡý���С
	* \return   					ý���С����λ�ֽ�
	*/
	virtual TTUint		            Size() = 0;

    /**
    * \fn                           const TTChar* Url()
    * \brief                        ��ѯý������
    * \return   					ý����
    */
    virtual const TTChar*           Url() = 0;
    
	/**
	* \fn                           TTInt SetDataSourceSync(const TTChar* aUrl)
	* \brief                        �첽����Src
	* \param [in]   aUrl			ý��·��
	* \return						����״̬
	*/
	virtual TTInt					SetDataSourceSync(const TTChar* aUrl, TTInt nFlag = 0) = 0;

	/**
	* \fn                           TTInt SetDataSourceAsync(const TTChar* aUrl)
	* \brief                        �첽����src
	* \param [in]   aUrl			ý��·��
	* \return						����״̬
	*/
	virtual TTInt					SetDataSourceAsync(const TTChar* aUrl, TTInt nFlag = 0) = 0;

	/**
	* \fn                           TTInt SetParam(TTInt nID, void* aParam)
	* \brief                        ���ò���
	* \param [in]   nID				����ID��ָ��ʲô����
	* \param [in]   aParam			����ֵ
	* \return						����״̬
	*/
	virtual TTInt					SetParam(TTInt nID, void* aParam) = 0;

	/**
	* \fn                           TTInt GetParam(TTInt nID, void* aParam)
	* \brief                        ��ȡ����
	* \param [in]   nID				����ID��ָ��ʲô����
	* \param [in]   aParam			����ֵ
	* \return						����״̬
	*/
	virtual TTInt					GetParam(TTInt nID, void* aParam) = 0;

	/**
	* \fn                           void Play()
	* \brief                        ��������
	* \return						����״̬
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
	* \brief                        ֹͣ����
	* \param [in]   aSync			�Ƿ�ͬ��stop��default���첽
	* \return                       ����״̬
	*/
	virtual TTInt                   Stop(TTBool aSync = false) = 0;

	/**
	* \fn                           void SetVolume(TTInt aLVolume, TTInt aRVolume);
	* \brief                        ��������
	* \param [in]   aLVolume         ����ֵ
	* \param [in]   aRVolume         ����ֵ
	*/
	virtual void                    SetVolume(TTInt aLVolume, TTInt aRVolume) = 0;

	/**
	* \fn                           TInt GetVolume();
	* \brief                        ��ȡ����
	* \return                       ����ֵ
	*/
	virtual TTInt                   GetVolume() = 0;

	/**
	* \fn                           void SetView(TTInt aLVolume, TTInt aRVolume);
	* \brief                        ��������
	* \param [in]   aLVolume         ����ֵ
	* \param [in]   aRVolume         ����ֵ
	*/
	virtual void                    SetView(void* aView) = 0;

	/**
	* \fn                           GKPlayStatus GetPlayStatus();
	* \brief                        ��ȡ������״̬
	* \return                       ״̬
	*/
	virtual GKPlayStatus            GetPlayStatus() = 0;

	/**
	* \fn                           void SetPosition(TTUint aPosition)
	* \brief                        ���ò���λ��
	* \param [in]   aPosition       λ��ʱ�䣬�Ժ���Ϊ��λ
	* \param [in]   aOption	        �������ԣ�����seek����׼ȷseek
	*/
	virtual TTInt64                 SetPosition(TTInt64 aPosition, TTInt aOption = 0) = 0;

	/**
	* \fn                           TTUint GetPosition()
	* \brief                        ��ȡ����ʱ��
	* \return                       λ��ʱ�䣬�Ժ���Ϊ��λ
	*/
	virtual TTUint64				GetPosition() = 0;

	/**
	* \fn                           SetVolumeRamp(TTUint aFadeIn, TTUint aFadeOut)
	* \brief                        �����������뵭��ʱ��
	* \param [in]   aFadeIn         ����ʱ��
	* \param [in]   aFadeOut        ����ʱ��
	*/
	//virtual void                  SetVolumeRamp(TTUint aFadeIn,  TTUint aFadeOut) = 0;

	/**
	* \fn                           void SetPlayWindow(TTInt aReaptedNum, TTUint aStart, TTUint aEnd)
	* \brief                        �����ظ����Ŵ�������ʼʱ��
	* \param [in]   aReaptedNum     ��������
	* \param [in]   aStart          ������ʼʱ��
	* \param [in]   aEnd            ������ֹʱ��
	*/
	//virtual void                  SetPlayWindow(TTInt aReaptedNum, TTUint aStart, TTUint aEnd) = 0;

	/**
	* \fn                           void RepeatCancel()
	* \brief                        ȡ���ظ�����
	*/
	//virtual void                  RepeatCancel() = 0;

	/**
	* \fn                           TTInt GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum);
	* \brief                        ��ȡ��ǰƵ��
	* \param[out]    aFreq          Ƶ������ָ�룬����Ҫ�Ļ�����NULL
	* \param[out]    aWave          ��������ָ��,����ǿ�
	* \param[in]     aSampleNum     ������ĸ���
	* \return                       ����״̬
	*/
	virtual TTInt                   GetCurrentFreqAndWave(TTInt16 *aFreq, TTInt16 *aWave, TTInt aSampleNum) = 0;

	/**
	* \fn							void SetPlayRange(TTUint aStartTime, TTUint aEndTime);
	* \brief						���ò��ŷ�Χ
	* \param[in]	aStartTime		���ŷ�Χ��ʼʱ�䣬��λ����
	* \param[in]	aEndTime		���ŷ�Χ����ʱ�䣬��λ����
	*/
	virtual void					SetPlayRange(TTUint aStartTime, TTUint aEndTime) = 0;

	/**
	* \fn							TTUint BufferedSize()
	* \brief						��ȡ�����С
	* \return						�����С(��λ:�ֽ�)
	*/
	virtual	TTUint					BufferedSize() = 0;


	/**
	* \fn							TTUint BandWidth()
	* \brief						��ȡ�������ش����С
	* \return						�����С(��λ:�ֽ�)
	*/
	virtual	TTUint					BandWidth() = 0;

	/**
	* \fn							TTUint BandPercent()
	* \brief						��ȡ�������ذٷֱ�
	* \return						�����С(��λ:�ֽ�)
	*/
	virtual	TTUint					BandPercent() = 0;

	/**
	* \fn							TTInt BufferedPercent(TTInt& aBufferedPercent)
	* \brief						��ȡ�Ի��������
	* \param[out]	aBufferedPercent�ٷֱ�
	* \return						����״̬
	*/
	virtual TTInt					BufferedPercent(TTInt& aBufferedPercent) = 0;

	/**
	* \fn							void SetActiveNetWorkType(TTActiveNetWorkType aNetWorkType)
	* \brief						����������������
	* \param[in]	aNetWorkType	������������
	*/
	virtual void					SetActiveNetWorkType(GKActiveNetWorkType aNetWorkType) = 0;


	/**
	* \fn							void SetNetWorkProxy(TTBool aNetWorkProxy)
	* \brief						���������Ƿ����
	* \param[in]	aNetWorkProxy	�����Ƿ����
	*/
	virtual void					SetNetWorkProxy(TTBool aNetWorkProxy) = 0;

	/**
	* \fn							void SetDecoderType(GKDecoderType aDecoderType)
	* \brief						���ý�������
	* \param[in]	aDecoderType	������������
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
	* \brief                        �����������������ļ���·��
	* \param[in] aCacheFilePath     �ļ�·��
	*/
	virtual void					SetCacheFilePath(const TTChar* aCacheFilePath) = 0;
#ifdef __MEDIA_PLAYER__
		/**
	* \fn							void Decode(const TTChar* aUrl, TTInt size)
	* \brief					    �ⲿ���ý���ӿ�
	* \param[in] aUrl			    Դ�ļ�url	
	* \param[in] size			    ��Ҫ��ȡ��pcm����size	
	* \return						������		
	*/
	virtual	TTInt					Decode(const TTChar* aUrl, TTInt size)= 0;

	/**
	* \fn							TTInt GetPCMData();
	* \brief						��ȡ��������
	* \return						����������		
	*/
	virtual TTUint8*				GetPCMData()= 0;

	/**
	* \fn							TTInt GetPCMDataSize();
	* \brief						��ȡ����������
	* \return						����������ֵ		
	*/
	virtual TTInt				    GetPCMDataSize()= 0;

	/**
	* \fn							void GetChannels();
	* \brief			            ��ȡ������
	* \return						����ֵ	
	*/
	virtual TTInt				    GetPCMDataChannle()= 0;

	/**
	* \fn							void GetSamplerate();
	* \brief			            ��ȡ������
	* \return						������ֵ	
	*/
	virtual TTInt				    GetPCMDataSamplerate()= 0;

	/**
	* \fn							void CancelGetPCM();
	* \brief			            ȡ������	
	*/
	virtual void					CancelGetPCM()= 0;

	/**
	* \fn							void SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos)
	* \brief						���ò��ŷ�Χ
	* \param[in] start			    ��ʼֵ	
	* \param[in] end			    ����ֵ	
	* \param[in] decodeStartPos		��ʼ����λ��ֵ	
	*/
	virtual void					SetTimeRange(TTInt start, TTInt end, TTInt decodeStartPos)= 0;

	virtual TTInt					DecodeEntireFile(const TTChar* aUrl, const TTChar* OutPutPath)= 0;

	virtual	TTInt					GetDataFromSpecifiedTimePos(const TTChar* aUrl, TTInt timePos, TTInt duration) = 0;
#endif

#ifdef __TT_OS_ANDROID__
	/**
	* \fn							void SaveAudioTrackJClass(void* pJclass)
	* \brief						����JNI�����Ϣ
	* \param[in]	pJclass	        java�� AudioTrack class
	*/
	virtual	void					SaveAudioTrackJClass(void* pJclass) = 0;

    /**
     * \fn							void GetAudiotrackClass();
     * \brief						��ȡjava�� AudioTrack class,ʵ��ITTMediaPlayer��Ӧ�ӿ�
	 * \return                      AudioTrack class pointer
     */
	virtual void*					GetAudiotrackClass() = 0;


	virtual void 				    SetMaxOutPutSamplerate(TTInt aSampleRate) =0;

	/**
	* \fn							void SaveVideoTrackJClass(void* pJclass)
	* \brief						����JNI�����Ϣ
	* \param[in]	pJclass	        java�� VideoTrack class
	*/
	virtual	void					SaveVideoTrackJClass(void* pJclass) = 0;

    /**
     * \fn							void GetVideotrackClass();
     * \brief						��ȡjava�� VideoTrack class,ʵ��ITTMediaPlayer��Ӧ�ӿ�
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
	* \brief                            ����ITTMediaPlayer�ӿڶ���
	* \param[in]	aPlayerObserver		MediaPlayer�۲���
	* \return                           ITTMediaPlayer�ӿڶ���ָ��
	*/
	static IGKMediaPlayer*              NewL(IGKMediaPlayerObserver* aPlayerObserver);
};

#endif
