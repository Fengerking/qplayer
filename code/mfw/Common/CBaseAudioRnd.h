/*******************************************************************************
	File:		CBaseAudioRnd.h

	Contains:	The audio render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseAudioRnd_H__
#define __CBaseAudioRnd_H__
#ifdef __QC_OS_NDK__
#include "jni.h"
#endif // __QC_OS_NDK__
#include "CBaseObject.h"

#include "CBaseClock.h"

class CBaseAudioRnd : public CBaseObject
{
public:
	CBaseAudioRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBaseAudioRnd(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);
	virtual int		Uninit (void);

	virtual int		Start (void);
	virtual int		Pause (void);
	virtual int		Stop (void);

	virtual int		OnStart(void);
	virtual int		OnStop (void);

	virtual int		Flush (void);
	virtual int		SetSpeed (double fSpeed);

	virtual int		Render (QC_DATA_BUFF * pBuff);

	virtual int		SetVolume (int nVolume);
	virtual int		GetVolume (void);

	virtual int 	SetParam(int nID, void * pParam) { return QC_ERR_FAILED; }
	virtual int		GetParam(int nID, void * pParam) { return QC_ERR_FAILED; }

	virtual int					GetRndCount (void) {return m_nRndCount;}
	virtual CBaseClock *		GetClock (void) {return m_pClock;}
	virtual QC_AUDIO_FORMAT *	GetFormat (void) {return &m_fmtAudio;}

#ifdef __QC_OS_NDK__
	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer) {return 0;}
#endif // __QC_OS_NDK__

protected:

protected:
	CMutexLock			m_mtRnd;
	void *				m_hInst;
	bool				m_bAudioOnly;
	QC_AUDIO_FORMAT		m_fmtAudio;
	CBaseClock *		m_pClock;


	unsigned char *		m_pPCMData;
	int					m_nPCMSize;
	unsigned char *		m_pPCMBuff;
	int					m_nPCMLen;

	double				m_fSpeed;
	long long			m_llPrevTime;
	int					m_nSizeBySec;

	int					m_nRndCount;
};

#endif // __CBaseAudioRnd_H__
