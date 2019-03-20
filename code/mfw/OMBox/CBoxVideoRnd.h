/*******************************************************************************
	File:		CBoxVideoRnd.h

	Contains:	the video render box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxVideoRnd_H__
#define __CBoxVideoRnd_H__

#include "CBoxRender.h"
#include "CBaseVideoRnd.h"

class CBoxVideoRnd : public CBoxRender
{
public:
	CBoxVideoRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxVideoRnd(void);

	virtual int				SetView (void * hView, RECT * pRect);
	virtual int				SetAspectRatio (int w, int h);
	virtual int				DisableVideo (int nFlag);

	virtual int				SetSource (CBoxBase * pSource);

	virtual int				Start (void);
	virtual int				Pause (void);
	virtual int				Stop (void);

	virtual long long		SetPos (long long llPos);
	virtual void			SetNewSource(bool bNewSource);
	virtual int				GetRndCount(void);

	virtual RECT *			GetRenderRect (void );
	virtual CBaseClock * 	GetClock (void);
	virtual long long	 	GetDelayTime (void) {return m_llDelayTime;}

	virtual QC_VIDEO_FORMAT *	GetVideoFormat (void) {return &m_fmtVideo;}

	virtual int				RecvEvent(int nEventID);
    virtual int				CaptureImage(long long llTime);


protected:
	virtual int		OnWorkItem (void);
	virtual int		OnStopFunc(void);

	virtual int		WaitRenderTime (long long llTime);
	virtual void	ResetMembers (void);
    void			UpdateStartTime(void);

	virtual int		CreateGDIRnd(void);
	virtual int		UpdateVideoFormat(void);
	QC_DATA_BUFF *	UpdateVideoData(QC_DATA_BUFF * pSourceData);
    
    int				SetViewInterval(void * hView, RECT * pRect);

protected:
	void *				m_hView;
	RECT				m_rcView;
	int					m_nARW;
	int					m_nARH;

	QC_VIDEO_FORMAT		m_fmtVideo;
	QC_DATA_BUFF		m_bufData;
	QC_VIDEO_BUFF		m_bufVideo;
	QC_DATA_BUFF		m_bufOutData;

	int					m_nVideoWidth;
	int					m_nVideoHeight;
	bool				m_bZoom;
	bool				m_bRotate;
    int					m_nZoomLeft;
    int                 m_nZoomTop;
	int					m_nZoomWidth;
	int					m_nZoomHeight;
	int					m_nRotateAngle;

	CBaseVideoRnd *		m_pRnd;
	int					m_nDisableRnd;
	int					m_nDroppedFrames;
	
	long long			m_llDelayTime;
	long long			m_llVideoTime;
	long long			m_llLastFTime;

	bool				m_bNewPos;
	long long			m_llNewTime;
    int					m_nLastClock;
};

#endif // __CBoxVideoRnd_H__
