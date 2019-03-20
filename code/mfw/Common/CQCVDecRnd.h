/*******************************************************************************
	File:		CQCVDecRnd.h

	Contains:	The video decode render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2018-04-16		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCVDecRnd_H__
#define __CQCVDecRnd_H__

#include "CBaseVideoRnd.h"

#include "CBaseVideoDec.h"

class CQCVDecRnd : public CBaseVideoRnd
{
public:
	CQCVDecRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCVDecRnd(void);
	
	virtual int		SetView(void * hView, RECT * pRect);
	virtual int		UpdateDisp(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit(void);
	virtual int		Render(QC_DATA_BUFF * pBuff);
	
	virtual int		WaitRendTime (long long llTime);
	virtual void	SetNewSource (bool bNewSource);
		
protected:
	virtual void	UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo);

protected:
	CBaseVideoDec *		m_pVideoDec;
	CBaseVideoRnd *		m_pVideoRnd;

	QC_DATA_BUFF *		m_pBuffData;
};

#endif // __CQCVDecRnd_H__
