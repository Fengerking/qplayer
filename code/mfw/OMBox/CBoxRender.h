/*******************************************************************************
	File:		CBoxRender.h

	Contains:	the base render box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxRender_H__
#define __CBoxRender_H__
#include "qcPlayer.h"
#include "CBoxBase.h"

#include "CQCVideoEnc.h"
#include "CThreadWork.h"

class CBoxRender : public CBoxBase , public CThreadFunc
{
public:
	typedef enum {
		QCRND_INIT = 0,
		QCRND_RUN,
		QCRND_PAUSE,
		QCRND_STOP,
	} QCRND_STATUS;

public:
	CBoxRender(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxRender(void);

	virtual int				SetOtherRender (CBoxRender * pRnd);
	virtual int				SetAspectRatio (int w, int h){return QC_ERR_FAILED;}

	virtual int				Start (void);
	virtual int				Pause (void);
	virtual int				Stop (void);

	virtual long long		SetPos (long long llPos);
	virtual void			SetSeekMode (int nSeekMode) {m_nSeekMode = nSeekMode;}
	virtual void			SetNewSource(bool bNewSource);
	virtual bool			GetNewSource(void) { return m_bNewSource; }

	virtual int				GetRndCount (void) {return m_nRndCount;}

	virtual int				SetSpeed (double fSpeed) {return QC_ERR_NONE;}
	virtual int				SetVolume (int nVolume) {return QC_ERR_NONE;}
	virtual int				GetVolume (void) {return QC_ERR_NONE;}

	virtual int				SetView (void * hView, RECT * pRect) {return QC_ERR_NONE;}
	virtual int				SetExtRnd (void * pExtRnd) {m_pExtRnd = pExtRnd; return QC_ERR_FAILED;}
	virtual int				DisableVideo (int nFlag) {return QC_ERR_NONE;}
	virtual int				CaptureImage(long long llTime) { m_llCaptureTime = llTime; return QC_ERR_NONE; }
	virtual RECT *			GetRenderRect (void ){return NULL;}

	virtual void			SetSendOutData(void * pUserData, QCPlayerOutAVData fSendOutData);

	virtual long long		GetDelayTime (void) {return 0;}
	virtual long long		GetRndTime(void) { return m_llLastRndTime; }
	virtual CBaseClock *	GetClock (void);
    virtual bool			IsDropFrame (void) { return m_bDropFrame; };

protected:
	virtual int		OnStartFunc (void);
	virtual int		OnWorkItem (void);

	virtual int		WaitRenderTime (QC_DATA_BUFF * pBuff);
	virtual void	WaitForExitRender (unsigned int nMaxWaitTime);
	virtual int		WaitOtherRndFirstFrame (void);

	virtual int		CaptureVideoImage(QC_VIDEO_FORMAT * pFormat, QC_VIDEO_BUFF * pVideoBuff);
    
    void			UpdateStartTime(void);

protected:
	CMutexLock			m_mtRnd;
	QCMediaType			m_nMediaType;
	CBoxRender *		m_pOtherRnd;
	CBaseClock *		m_pDataClock;

	int					m_nSeekMode;

	void *				m_pExtRnd;
	int					m_nRndCount;
	bool				m_bInRender;
	bool				m_bBuffering;

	CThreadWork *		m_pThreadWork;
	long long			m_llStartTime;

	int					m_nSourceType;
	bool				m_bNewSource;

	long long			m_llCaptureTime;
	CQCVideoEnc *		m_pVideoEnc;
	QC_DATA_BUFF *		m_pVideoBuf;

	void *				m_pUserData;
	QCPlayerOutAVData	m_fSendOutData;

	long long			m_llLastRndTime;
    int					m_nPauseTime;
    int					m_nLastRndCount;
    int					m_nLastSysTime;
    bool				m_bDropFrame;
};

#endif // __CBoxRender_H__
