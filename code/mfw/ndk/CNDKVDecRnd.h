/*******************************************************************************
	File:		CNDKVDecRnd.h

	Contains:	The ndk audio render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKVDecRnd_H__
#define __CNDKVDecRnd_H__

#include "CBaseVideoRnd.h"
#ifdef __QC_OS_NDK__
#include "jni.h"
#include "CJniEnvUtil.h"
#include "CMediaCodecDec.h"
#endif // __QC_OS_NDK__

#define	QCPLAY_BUFF_NEW_POS					0X00000001
#define	QCPLAY_BUFF_NEW_FORMAT				0X00000002
#define	QCPLAY_BUFF_EOS						0X00000004
#define	QCPLAY_BUFF_KEY_FRAME				0X00000008

#define	QCPLAY_BUFF_AUDIO					0X10000000
#define	QCPLAY_BUFF_VIDEO					0X20000000

//  Call back function of data send out, return > 0 had rend
typedef int (QC_API * QCPlayerDataSendOut) (void * pUserData, unsigned char * pBuff, int nSize, long long llTime, int nFlag);

class CNDKVDecRnd : public CBaseVideoRnd
{
public:
	CNDKVDecRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CNDKVDecRnd(void);
#ifdef __QC_OS_NDK__
	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer, int nVer);
	virtual int		SetSurface (JNIEnv* pEnv, jobject pSurface);	
#endif // __QC_OS_NDK__
	
	virtual int		SetSendFunc (QCPlayerDataSendOut fSend, void * pUserData);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Render (QC_DATA_BUFF * pBuff);
	
	virtual int		WaitRendTime (long long llTime);
	virtual void	SetEventDone (bool bEventDone);
	virtual void	SetNewSource (bool bNewSource);

	virtual int		OnStart (void);	
	virtual int		OnStop (void);
		
protected:
	virtual void	UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo);

protected:
#ifdef __QC_OS_NDK__
	JavaVM *			m_pJVM;
	jclass     			m_pjCls;
	jobject				m_pjObj;
	jmethodID			m_fPostEvent;
	jmethodID			m_fPushVideo;

	CJniEnvUtil	*		m_pEnvUtil;
	JNIEnv *			m_pEnv;
	jbyteArray			m_pDataBuff;

	CMediaCodecDec *	m_pDec;
	jobject 			m_pSurface;
#endif // __QC_OS_NDK__
	int					m_nVer;
	QCPlayerDataSendOut	m_fSend;
	void *				m_pUser;

	int					m_nDataSize;
	int					m_nBuffSize;
	bool				m_bEventDone;	
};

#endif // __CNDKVDecRnd_H__
