/*******************************************************************************
	File:		CBoxAudioRnd.h

	Contains:	the Audio render box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxAudioRnd_H__
#define __CBoxAudioRnd_H__

#include "CBoxRender.h"

#include "CBaseAudioRnd.h"
#include "TDStretch.h"
#include "aflibconverter.h"

class CBoxAudioRnd : public CBoxRender
{
public:
	CBoxAudioRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxAudioRnd(void);

	virtual int				SetSource (CBoxBase * pSource);

	virtual int				Start (void);
	virtual int				Pause (void);
	virtual int				Stop (void);

	virtual long long		SetPos (long long llPos);
	virtual int				GetRndCount (void);

	virtual int				SetSpeed (double fSpeed);
	virtual int				SetVolume (int nVolume);
	virtual int				GetVolume (void);

	virtual CBaseClock *	GetClock (void);

protected:
	virtual int				OnWorkItem (void);
	virtual int				RenderFrame (void);
	virtual int				OnStopFunc (void);

	virtual int				StretchData (QC_DATA_BUFF * pBuffData, QC_DATA_BUFF ** ppBuffStretch, double dSpeed = 1.0);

protected:
	QC_AUDIO_FORMAT		m_fmtAudio;
	bool				m_bNewFormat;
	CBaseAudioRnd *		m_pRnd;
	int					m_nVolume;
	double				m_fSpeed;
	QC_DATA_BUFF *		m_pBuffSpeed;
    bool				m_bSendFormat;

	// for live source
	QC_DATA_BUFF *		m_pBuffStretch;
	TDStretch *			m_pTDStretch;
	int					m_nBlockSize;
	float				m_fStretchSpeed;

	CMutexLock			m_mtRsmp;
	aflibConverter *	m_pResample;
};

#endif // __CBoxAudioRnd_H__
