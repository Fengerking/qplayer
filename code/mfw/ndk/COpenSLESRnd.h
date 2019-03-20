/*******************************************************************************
	File:		COpenSLESRnd.h

	Contains:	The ndk audio render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-12		Bangfei			Create file

*******************************************************************************/
#ifndef __COpenSLESRnd_H__
#define __COpenSLESRnd_H__

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "CBaseAudioRnd.h"
#include "CNodeList.h"
#include "CMutexLock.h"
#include "CNDKSendBuff.h"

typedef struct OpenSLES_RndBuff {
	unsigned char *		pBuff;
	int					nSize;
	long long			llTime;
} OPENSLES_RNDBUFF;

class COpenSLESRnd : public CBaseAudioRnd
{
public:
	COpenSLESRnd (CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenSLESRnd(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);
	virtual int		Flush (void);
	
	virtual int		SetVolume (int nVolume);
	virtual int		GetVolume (void);

	virtual int		OnStart(void);
	virtual int		OnStop(void);
	virtual int 	SetParam(int nID, void * pParam);
	virtual int		SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer);

protected:
	virtual int		CreateEngine (void);
	virtual int		DestroyEngine (void);

	virtual int		GetSampleRate (void);
	virtual int		ReleaseBuffer (void);

protected:
	SLObjectItf						m_pObject;
	SLEngineItf						m_pEngine;
	SLObjectItf						m_pOutputMix;
	SLObjectItf						m_pPlayer;
	SLPlayItf						m_pPlay;
	SLVolumeItf						m_pVolume;
	SLAndroidSimpleBufferQueueItf	m_pBuffQueue;

	CObjectList<OPENSLES_RNDBUFF>	m_lstEmpty;
	CObjectList<OPENSLES_RNDBUFF>	m_lstPlay;
	CMutexLock						m_mtList;
	OPENSLES_RNDBUFF *				m_pCurBuff;

	int								m_nVolume;

	CNDKSendBuff *					m_pSendBuff;
	int								m_nInRender;

	int								m_nSysTime;

public:
	static void		RenderCallback (SLAndroidSimpleBufferQueueItf buffQueue, void * pContext);
};

#endif // __COpenSLESRnd_H__
