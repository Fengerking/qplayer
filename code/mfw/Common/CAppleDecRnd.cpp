/*******************************************************************************
	File:		CAppleDecRnd.cpp

	Contains:	Window base implement code

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun			Create file

*******************************************************************************/

#include "CAppleDecRnd.h"
#include "CVTBVideoDec.h"
#include "CGKVideoDec.h"
#include "CVideoRndFactory.h"

#include "qcErr.h"
#include "USystemFunc.h"

CAppleDecRnd::CAppleDecRnd(void * hInst)
:CBaseVideoRnd (hInst)
, m_pDec (NULL)
, m_pRnd (NULL)
{
	memset (&m_buffData, 0, sizeof (m_buffData));
	m_buffData.nMediaType = QC_MEDIA_Video;
	m_buffData.uBuffType = QC_BUFF_TYPE_Data;
}

CAppleDecRnd::~CAppleDecRnd(void)
{
	QC_DEL_P (m_pRnd);
	QC_DEL_P (m_pDec);
}

int CAppleDecRnd::SetView (void* hView, RECT* pRect)
{
    CBaseVideoRnd::SetView(hView, pRect);
    
	if (m_pRnd != NULL)
		return m_pRnd->SetView (hView, pRect);
    
    return QC_ERR_NONE;
}

int CAppleDecRnd::Start (void)
{
    CBaseVideoRnd::Start();
    
    if(m_pRnd)
        m_pRnd->Start();
    
    return QC_ERR_NONE;
}

int CAppleDecRnd::Pause (void)
{
    CBaseVideoRnd::Pause();
    
    if(m_pRnd)
        m_pRnd->Pause();
    
    return QC_ERR_NONE;
}

int	CAppleDecRnd::Stop (void)
{
    CBaseVideoRnd::Stop();
    
    if(m_pRnd)
        m_pRnd->Stop();
    
    return QC_ERR_NONE;
}


int CAppleDecRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (m_pDec == NULL)
	{
        QC_VIDEO_FORMAT * pFmt = &m_fmtVideo;
        
        if (pFmt != NULL)
        {
            m_pDec = new CGKVideoDec(NULL);
            m_pDec->Init (pFmt);
            m_pRnd = CVideoRndFactory::Create();
            m_pRnd->SetView (m_hView, NULL);
            m_pRnd->Init (pFmt);
            m_pRnd->Start();
 
            m_fmtVideo.nWidth = pFmt->nWidth;
            m_fmtVideo.nHeight = pFmt->nHeight;
        }
	}
    
    if(!m_pDec)
        return QC_ERR_FAILED;

	m_buffData.pBuff = pBuff->pBuff;
	m_buffData.uSize = pBuff->uSize;
	m_buffData.llTime = pBuff->llTime;
	m_buffData.uFlag = pBuff->uFlag;

	int nRC = m_pDec->SetBuff (&m_buffData);
	int nRndCount = 0;
	while (nRC == QC_ERR_NONE)
	{
		m_pBuffRnd = NULL;
		nRC = m_pDec->GetBuff (&m_pBuffRnd);
		if (m_pBuffRnd == NULL)
			return 0;	
		if ((m_pBuffRnd->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
			m_pRnd->Init ((QC_VIDEO_FORMAT *)m_pBuffRnd->pFormat);
		nRC = m_pRnd->Render (m_pBuffRnd);
		WaitRendTime (m_pBuffRnd->llTime);
		if (nRC == QC_ERR_NONE)
			nRndCount++;
	}
	return nRndCount;
}
