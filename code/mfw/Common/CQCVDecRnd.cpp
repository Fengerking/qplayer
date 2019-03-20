/*******************************************************************************
	File:		CQCVDecRnd.cpp

	Contains:	The video decoder and render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2018-04-16		Bangfei			Create file

*******************************************************************************/
#include <stdlib.h>
#include "qcErr.h"
#include "qcMsg.h"

#include "CQCVDecRnd.h"

#include "CQCVideoDec.h"
#ifdef __QC_OS_WIN32__
#include "CDDrawRnd.h"
#include "CGDIRnd.h"
#endif // __QC_OS_WIN32__

#include "USystemFunc.h"
#include "ULogFunc.h"

CQCVDecRnd::CQCVDecRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
	, m_pVideoDec(NULL)
	, m_pVideoRnd(NULL)
{
	SetObjectName ("CQCVDecRnd");
}

CQCVDecRnd::~CQCVDecRnd(void)
{
	Uninit ();
}

int CQCVDecRnd::SetView(void * hView, RECT * pRect)
{
	int nRC = CBaseVideoRnd::SetView(hView, pRect);
	if (m_pVideoRnd != NULL)
		nRC = m_pVideoRnd->SetView(m_hView, &m_rcView);
	return nRC;
}

int CQCVDecRnd::UpdateDisp(void)
{
	if (m_pVideoRnd != NULL)
		return m_pVideoRnd->UpdateDisp();
	return QC_ERR_IMPLEMENT;
}

int CQCVDecRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	QCLOGI ("Init format %d X %d m_fmtVideo.nWidth = %d", pFmt->nWidth, pFmt->nHeight, m_fmtVideo.nWidth);
	if (pFmt->nWidth == 0 || pFmt->nHeight == 0)
		return QC_ERR_NONE;
	
	int nRC = CBaseVideoRnd::Init (pFmt);

	if (m_pVideoDec == NULL)
		m_pVideoDec = new CQCVideoDec(m_pBaseInst, NULL);
	if (m_pVideoDec == NULL)
		return QC_ERR_MEMORY;
	nRC = m_pVideoDec->Init(pFmt);
	if (nRC != NULL)
		return nRC;

	if (m_pVideoRnd == NULL)
#ifdef __QC_OS_WIN32__
		m_pVideoRnd = new CDDrawRnd(m_pBaseInst, NULL);
#endif // __QC_OS_WIN32__
	if (m_pVideoRnd == NULL)
		return QC_ERR_MEMORY;
	m_pVideoRnd->SetView(m_hView, &m_rcView);
	QC_VIDEO_FORMAT * pVideoFmt = m_pVideoDec->GetVideoFormat();
	if (pVideoFmt != NULL)
		m_pVideoRnd->Init(pVideoFmt);
	
	UpdateVideoSize (pFmt);		

	return QC_ERR_NONE;
}

int	CQCVDecRnd::Uninit(void)
{
	QC_DEL_P(m_pVideoRnd);
	QC_DEL_P(m_pVideoDec);
	return CBaseVideoRnd::Uninit();
}

void CQCVDecRnd::SetNewSource (bool bNewSource)
{
	m_bNewSource = bNewSource;
}

int CQCVDecRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || pBuff->pBuff == NULL)
		return QC_ERR_ARG;
	if (m_pVideoDec == NULL || m_pVideoRnd == NULL)
		return QC_ERR_STATUS;

	if (m_fmtVideo.nWidth == 0 && ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT))
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		QCLOGI ("Render new format %d X %d ", pFmt->nWidth, pFmt->nHeight);		
		Init (pFmt);
	}		
	int nRC = CBaseVideoRnd::Render (pBuff);

	while (nRC == QC_ERR_NONE)
	{
		m_pBuffData = NULL;
		nRC = m_pVideoDec->GetBuff(&m_pBuffData);
		if (nRC == QC_ERR_NONE && m_pBuffData != NULL)
		{
			WaitRendTime(m_pBuffData->llTime);
			m_pVideoRnd->Render(m_pBuffData);
		}
	}

	nRC = m_pVideoDec->SetBuff(pBuff);
	if (nRC != QC_ERR_NONE)
		return nRC;

	while (nRC == QC_ERR_NONE)
	{
		m_pBuffData = NULL;
		nRC = m_pVideoDec->GetBuff(&m_pBuffData);
		if (nRC == QC_ERR_NONE && m_pBuffData != NULL)
		{
			WaitRendTime(m_pBuffData->llTime);
			m_pVideoRnd->Render(m_pBuffData);
		}
	}

	return QC_ERR_NONE;
}

int CQCVDecRnd::WaitRendTime (long long llTime)
{
	if (m_pExtClock == NULL)
		return QC_ERR_STATUS;
	m_nRndCount++;
	if (m_pExtClock->IsPaused ())
		m_pExtClock->Start ();	
	long long llPlayTime = m_pExtClock->GetTime ();
	while (llPlayTime < llTime)
	{
		if (abs ((int)(llTime - llPlayTime)) > 50000 && llPlayTime != 0)
		{
			qcSleep (30000);
			return QC_ERR_NONE;
		}
		qcSleep (2000);		
		llPlayTime = m_pExtClock->GetTime ();
		if (!m_bPlay)
			return -1;
		if (m_bNewSource)
			return -1;
		if (m_pBaseInst->m_bForceClose)
			break;
	}
	return 0;
}

void CQCVDecRnd::UpdateVideoSize (QC_VIDEO_FORMAT * pFmtVideo)
{
	QCLOGI ("Update Video Size: %d X %d  Ratio: %d, %d", pFmtVideo->nWidth, pFmtVideo->nHeight, pFmtVideo->nNum, pFmtVideo->nDen);
	m_rcView.top = 0;
	m_rcView.left = 0;
	m_rcView.right = m_fmtVideo.nWidth;
	m_rcView.bottom = m_fmtVideo.nHeight;	

	CBaseVideoRnd::UpdateRenderSize ();
	int nWidth = m_rcRender.right - m_rcRender.left;
	int nHeight = m_rcRender.bottom - m_rcRender.top;

	QCLOGI ("Update display Size: %d X %d ", nWidth, nHeight);	
}
