/*******************************************************************************
	File:		CVideoDecRnd.cpp

	Contains:	Window base implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#include "windows.h"
#include "tchar.h"

#include "CVideoDecRnd.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CVideoDecRnd::CVideoDecRnd(void * hInst)
	: m_hInst (hInst)
	, m_pPlay (NULL)
	, m_hView (NULL)
	, m_pDec (NULL)
	, m_pRnd (NULL)
	, m_pVDecRnd (NULL)
{
	memset (&m_buffData, 0, sizeof (m_buffData));
	m_buffData.nMediaType = QC_MEDIA_Video;
	m_buffData.uBuffType = QC_BUFF_TYPE_Data;
}

CVideoDecRnd::~CVideoDecRnd(void)
{
	QC_DEL_P (m_pRnd);
	QC_DEL_P (m_pDec);
	QC_DEL_P (m_pVDecRnd);
}

void CVideoDecRnd::SetView (HWND hView)
{
	m_hView = hView;
	if (m_pRnd != NULL)
		m_pRnd->SetView (m_hView, NULL);
	if (m_pDec != NULL)
		m_pDec->Flush ();
}

int CVideoDecRnd::SetPlayer (QCM_Player * pPlay)
{
	m_pPlay = pPlay;

	m_pVDecRnd = new CNDKVDecRnd (NULL, m_hInst);
	m_pVDecRnd->SetSendFunc (ReceiveData, this);
	m_pPlay->SetParam (m_pPlay->hPlayer, QCPLAY_PID_EXT_VideoRnd, m_pVDecRnd);


	return QC_ERR_NONE;
}

int CVideoDecRnd::Render (unsigned char * pBuff, int nSize, long long llTime, int nFlag)
{
	if (m_pDec == NULL)
	{
		QC_VIDEO_FORMAT * pFmt = m_pVDecRnd->GetFormat ();
		m_pDec = new  CQCVideoDec (NULL, m_hInst);
		m_pDec->Init (pFmt);
		m_pRnd = new CDDrawRnd (NULL, m_hInst);
		m_pRnd->SetView (m_hView, NULL);
		m_pRnd->Init (pFmt);
	}

//	if ((nFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
//		m_pDec->Flush ();

	m_buffData.pBuff = pBuff;
	m_buffData.uSize = nSize;
	m_buffData.llTime = llTime;
	m_buffData.uFlag = nFlag;

	int nRC = m_pDec->SetBuff (&m_buffData);
	if (nRC != QC_ERR_NONE)
		return 0;

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
		//QCLOGT ("QCLOG", "Output Time = % 8lld", m_pBuffRnd->llTime);
		m_pVDecRnd->WaitRendTime (m_pBuffRnd->llTime);
		if (nRC == QC_ERR_NONE)
			nRndCount++;
	}
	return nRndCount;
}

int	CVideoDecRnd::ReceiveData (void * pUserData, unsigned char * pBuff, int nSize, long long llTime, int nFlag)
{
	return ((CVideoDecRnd *)pUserData)->Render (pBuff, nSize, llTime,  nFlag);
}