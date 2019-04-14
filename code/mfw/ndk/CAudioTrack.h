/*******************************************************************************
	File:		CAudioTrack.h

	Contains:	The audio track header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2019-04-12		Bangfei			Create file

*******************************************************************************/
#ifndef __CAudioTrack_H__
#define __CAudioTrack_H__
#include "qcData.h"

#include "CMutexLock.h"
#include "CBaseAudioRnd.h"

#include "jni.h"

class CAudioTrack : public CBaseAudioRnd
{
public:
	CAudioTrack(CBaseInst * pBaseInst, void * hInst);
	virtual ~CAudioTrack(void);

	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);
	virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);

	virtual int		Render (QC_DATA_BUFF * pBuff);
	virtual int		OnStart (void);	
	virtual int		OnStop (void);

protected:
    virtual int         CreateTrack (JNIEnv * pEnv);
    virtual int         ReleaseTrack (JNIEnv * pEnv);

    virtual jmethodID   GetMethod (JNIEnv * pEnv, jclass cls, const char * pName, const char * pParam, int nStatic);
	virtual int		    ResetParam (void);

protected:
	JavaVM *			m_pjVM;
    JNIEnv *			m_pEnv;	
	jclass     			m_pjCls;
	jobject				m_pjObj;

	jobject				m_objTrack;
    jclass				m_clsTrack;

    jmethodID           m_initTrack;
    jmethodID           m_getMinBufferSize;
    jmethodID           m_play;
    jmethodID           m_pause;
    jmethodID           m_stop;
    jmethodID           m_flush;
    jmethodID           m_release;
    jmethodID           m_write; 

    jbyteArray          m_buffer;
	int					m_nBuffSize;
};

#endif // __CAudioTrack_H__
