/*******************************************************************************
	File:		CNDKPlayer.h

	Contains:	qPlayer NDK player header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-15		Fenger			Create file

*******************************************************************************/
#ifndef __CNDKPlayer_H__
#define __CNDKPlayer_H__

#include <jni.h>

#include "qcPlayer.h"
#include "CBaseObject.h"
#include "CMutexLock.h"

#include "CNDKAudioRnd.h"
#include "CNDKVideoRnd.h"
#include "CNDKVDecRnd.h"
#include "COpenSLESRnd.h"
#include "CAudioTrack.h"

#include "UMsgMng.h"

#define QC_FLAG_NETWORK_CHANGED         0x20000000
#define QC_FLAG_NETWORK_TYPE			0x20000001
#define QC_FLAG_ISP_NAME        		0x20000002
#define QC_FLAG_WIFI_NAME               0x20000003
#define QC_FLAG_SIGNAL_DB   			0x20000004
#define QC_FLAG_SIGNAL_LEVEL			0x20000005

#define QC_FLAG_APP_VERSION				0x21000001
#define QC_FLAG_APP_NAME				0x21000002
#define QC_FLAG_SDK_ID					0x21000003
#define QC_FLAG_SDK_VERSION				0x21000004

#define	PARAM_PID_AUDIO_VOLUME			0X101
#define	PARAM_PID_QPLAYER_VERSION		0X110

class CNDKPlayer : public CBaseObject, public CMsgReceiver
{	
public:
	CNDKPlayer(void);
	virtual ~CNDKPlayer();
	
	virtual int		Init(JavaVM * jvm, JNIEnv * env, jclass clsPlayer, jobject objPlayer, char * pPath, int nFlag);	
	virtual int		Uninit(JNIEnv* env);
	
	virtual int		SetView(JNIEnv* env, jobject pView);
	
	virtual int		Open (const char* szUrl, int nFlag);
	
	virtual int		Play (void);
	virtual int		Pause (void); 
	virtual int		Stop (void); 
		
	virtual long long	GetDuration();

	virtual int		GetPos (long long * pCurPos);
	virtual int		SetPos (long long llCurPos);	
	virtual int		WaitRendTime (long long llTime);
	
	virtual int		GetParam (JNIEnv* env, int nID, int nParam, jobject pValue);
	virtual int		SetParam (JNIEnv* env, int nID, int nParam, jobject pValue);

	virtual int		ReceiveMsg (CMsgItem * pItem);
	
public:
	static	void 	NotifyEvent (void * pUserData, int nID, void * pV1);
	
protected:
	virtual void	HandleEvent (int nID, void * pV1);
	virtual int		GetOutputLatency (void);	
	
protected:
	JavaVM *			m_pjVM;
	jclass     			m_pjCls;
	jobject				m_pjObj;
	jmethodID			m_fPostEvent;
	jmethodID			m_fPushVideo;	
	jmethodID			m_fPushSubTT;
	jmethodID			m_fPushTTByte;
	int					m_nVersion;
	bool				m_bEventDone;
		
	jobject				m_pView;	

	QCM_Player			m_Player;
	int					m_nPlayFlag;
	CBaseAudioRnd *		m_pRndAudio;
	CNDKVideoRnd *		m_pRndVideo;
	CNDKVDecRnd *		m_pRndVidDec;

	int					m_nLatency;
	long long			m_llSeekPos;

	int					m_nStartTime;
	int					m_nIndex;

	CMutexLock			m_mtNofity;

	JNIEnv *			m_pEnv;
	jbyteArray			m_pDataBuff;	
	int					m_nDataSize;
	int					m_nMsgThread;

	QC_DATA_BUFF		m_buffAudio;
	QC_DATA_BUFF		m_buffVideo;
	QC_DATA_BUFF		m_buffSource;

	char				m_szDebugLine[1024];
	char				m_szDebugItem[1024];
};

#endif //__CNDKPlayer_H__
