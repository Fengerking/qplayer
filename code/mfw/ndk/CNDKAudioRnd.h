/*******************************************************************************
	File:		CNDKAudioRnd.h

	Contains:	The ndk audio render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __CNDKAudioRnd_H__
#define __CNDKAudioRnd_H__

#include "CBaseAudioRnd.h"

#include "jni.h"

class CNDKAudioRnd : public CBaseAudioRnd
{
public:
	CNDKAudioRnd (CBaseInst * pBaseInst, void * hInst);
	virtual ~CNDKAudioRnd(void);

	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);

	virtual int		OnStop (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

protected:
	virtual int		UpdateFormat (JNIEnv * pEnv, QC_AUDIO_FORMAT * pFmt);

protected:
	JavaVM *			m_pjVM;
	jclass     			m_pjCls;
	jobject				m_pjObj;
	jmethodID			m_fPostEvent;
	jmethodID			m_fPushAudio;

	JNIEnv *			m_pEnv;
	jbyteArray			m_pDataBuff;
	int					m_nDataSize;
	int					m_nBuffSize;

	bool				m_bFormatUpdate;
};

#endif // __CNDKAudioRnd_H__
