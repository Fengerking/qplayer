/*******************************************************************************
	File:		CWaveOutRnd.h

	Contains:	The wave out render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-26		Bangfei			Create file

*******************************************************************************/
#ifndef __CWaveOutRnd_H__
#define __CWaveOutRnd_H__

#include "windows.h"
#include "mmSystem.h"

#include "CBaseAudioRnd.h"

#include "CNodeList.h"
#include "CMutexLock.h"

#define MAXINPUTBUFFERS		3

class CWaveOutRnd : public CBaseAudioRnd
{
public:
	typedef struct WAVEHDRINFO
	{
		long long	llTime;
		void *		pData;
	} WAVEHDRINFO;

	static bool CALLBACK QCWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, 
													DWORD dwParam1, DWORD dwParam2);
public:
	CWaveOutRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CWaveOutRnd(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt, bool bAudioOnly);
	virtual int		Uninit (void);

	virtual int		Start (void);
	virtual int		Stop (void);
	virtual int		Flush (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

	virtual int		SetVolume (int nVolume);
	virtual int		GetVolume (void);

protected:
	virtual bool	InitDevice (QC_AUDIO_FORMAT * pFmt);
	virtual bool	AllocBuffer (void);
	virtual bool	ReleaseBuffer (void);
	virtual int		UpdateFormat (QC_AUDIO_FORMAT * pFmt);

	virtual int		WaitAllBufferDone (int nWaitTime);

	virtual bool	AudioDone (WAVEHDR * pWaveHeader);

protected:
	CMutexLock				m_mtWaveOut;
	HWAVEOUT				m_hWaveOut;
	UINT					m_nDeviceID;
	WAVEFORMATEX 			m_wavFormat;
	DWORD					m_dwVolume;
	bool					m_bReseted;
	bool					m_bFirstRnd;
	int						m_nWriteCount;

	CMutexLock				m_mtList;
	CObjectList<WAVEHDR>	m_lstFull;
	CObjectList<WAVEHDR>	m_lstFree;
	unsigned int			m_nBufSize;
	long long				m_llBuffTime;
	long long				m_llRendTime;
};

#endif // __CWaveOutRnd_H__
