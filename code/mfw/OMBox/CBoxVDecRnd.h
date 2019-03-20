/*******************************************************************************
	File:		CBoxVDecRnd.h

	Contains:	the video decode and render box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxVDecRnd_H__
#define __CBoxVDecRnd_H__

#include "CBoxRender.h"
#include "CBaseVideoRnd.h"

class CBoxVDecRnd : public CBoxRender
{
public:
	CBoxVDecRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxVDecRnd(void);

	virtual int				SetView(void * hView, RECT * pRect);
	virtual int				SetAspectRatio(int w, int h);
	virtual int				DisableVideo (int nFlag);
	virtual int				SetSource (CBoxBase * pSource);
	
	virtual long long		SetPos (long long llPos);
	virtual int				GetRndCount (void);

	virtual int				Start (void);
	virtual int				Pause (void);
	virtual int				Stop (void);
	virtual void			SetNewSource (bool bNewSource);

	QC_VIDEO_FORMAT *		GetVideoFormat (void) {return &m_fmtVideo;}

protected:
	virtual int		OnWorkItem (void);
	virtual int		OnStartFunc (void);
	virtual int		OnStopFunc (void);

protected:
	QC_VIDEO_FORMAT		m_fmtVideo;
	int					m_nARW;
	int					m_nARH;
	int					m_nDisableRnd;

	CBaseVideoRnd *		m_pRnd;

	bool				m_bNewPos;
	long long			m_llNewTime;
	int					m_nLastClock;	
};

#endif // __CBoxVDecRnd_H__
