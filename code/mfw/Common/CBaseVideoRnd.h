/*******************************************************************************
	File:		CBaseVideoRnd.h

	Contains:	The base video render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseVideoRnd_H__
#define __CBaseVideoRnd_H__

#include "CBaseObject.h"
#include "CBaseClock.h"
#include "qcErr.h"

#include "qcCodec.h"

class CBaseVideoRnd : public CBaseObject
{
public:
	CBaseVideoRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBaseVideoRnd(void);

	virtual int		SetView (void * hView, RECT * pRect);
	virtual int		UpdateDisp (void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Start (void);
	virtual int		Pause (void);
	virtual int		Stop (void);

	virtual int		OnStart (void);	
	virtual int		OnStop (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

	virtual int		WaitRendTime (long long llTime);
	virtual int		SetAspectRatio (int w, int h);
	virtual void	SetNewSource (bool bNewSource) {m_bNewSource = bNewSource;}

	virtual int 	SetParam(int nID, void * pParam) { return QC_ERR_FAILED; }
	virtual int		GetParam(int nID, void * pParam) { return QC_ERR_FAILED; }

	virtual RECT *			GetRenderRect (void ) {return &m_rcRender;}
	virtual int				GetRndCount (void) {return m_nRndCount;}
	virtual CBaseClock *	GetClock (void);
	virtual void			SetClock (CBaseClock * pClock) {m_pExtClock = pClock;}
	QC_VIDEO_FORMAT *		GetFormat (void) {return &m_fmtVideo;}
	virtual QC_VIDEO_BUFF *	ConvertYUVData(QC_DATA_BUFF * pBuff);
	virtual QC_VIDEO_BUFF *	RotateYUVData(QC_VIDEO_BUFF * pBuff, int nAngle);
	virtual QC_VIDEO_BUFF * GetRotateData(void) { return &m_bufRotate; }

protected:
	virtual bool	UpdateRenderSize (void);

	int				GetRectW (RECT * pRect) {return pRect->right - pRect->left;}
	int				GetRectH (RECT * pRect) {return pRect->bottom - pRect->top;}

protected:
	void *				m_hInst;
	CBaseClock *		m_pClock;
	CBaseClock *		m_pExtClock;
	bool				m_bPlay;

	CMutexLock			m_mtDraw;
	void *				m_hView;
	RECT				m_rcVideo;
	RECT				m_rcView;
	RECT				m_rcRender;
	int					m_nARWidth;
	int					m_nARHeight;
	int					m_nMaxWidth;
	int					m_nMaxHeight;

	QC_VIDEO_FORMAT		m_fmtVideo;
	QC_VIDEO_BUFF		m_bufVideo;
	QC_VIDEO_BUFF		m_bufRotate;
	QC_VIDEO_BUFF		m_bufRender;

	bool				m_bUpdateView;
	int					m_nRndCount;

	bool				m_bNewSource;

	QCCOLORCVTROTATE 	m_fColorCvtR;
};

#endif // __CBaseVideoRnd_H__
