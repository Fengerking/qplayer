/*******************************************************************************
	File:		CBaseVideoRnd.cpp

	Contains:	The base Video render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseVideoRnd.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

CBaseVideoRnd::CBaseVideoRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_pClock (NULL)
	, m_pExtClock (NULL)
	, m_bPlay (false)
	, m_hView (NULL)
	, m_nARWidth (1)
	, m_nARHeight (1)
	, m_nMaxWidth (3840)
	, m_nMaxHeight (2160)
	, m_bUpdateView (false)
	, m_nRndCount (0)
	, m_bNewSource (false)
	, m_fColorCvtR(NULL)
{
	SetObjectName ("CBaseVideoRnd");

	memset (&m_rcVideo, 0, sizeof (RECT));
	memset (&m_rcView, 0, sizeof (RECT));
	memset (&m_rcRender, 0, sizeof (RECT));

	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
	memset (&m_bufVideo, 0, sizeof (m_bufVideo));
	m_bufVideo.nType = QC_VDT_YUV420_P;

	memset(&m_bufRotate, 0, sizeof(m_bufRotate));
	m_bufRotate.nType = QC_VDT_YUV420_P;

	memset(&m_bufRender, 0, sizeof(m_bufRender));

#ifdef __QC_LIB_ONE__
	m_fColorCvtR = qcColorCvtRotate;
#else
	if (m_pBaseInst != NULL && m_pBaseInst->m_hLibCodec != NULL)
		m_fColorCvtR = (QCCOLORCVTROTATE)qcLibGetAddr(m_pBaseInst->m_hLibCodec, "qcColorCvtRotate", 0);
#endif // __QC_LIB_ONE__
}

CBaseVideoRnd::~CBaseVideoRnd(void)
{
	Uninit ();
	QC_DEL_P (m_pClock);
}

int CBaseVideoRnd::SetView (void * hView, RECT * pRect)
{
	CAutoLock lock (&m_mtDraw);

	m_hView = hView;
	if (pRect != NULL)
		memcpy (&m_rcView, pRect, sizeof (RECT));

	return QC_ERR_NONE;
}

int CBaseVideoRnd::UpdateDisp (void)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseVideoRnd::SetAspectRatio (int w, int h)
{
	CAutoLock lock (&m_mtDraw);
	if (m_nARWidth == w && m_nARHeight == h)
		return QC_ERR_NONE;
	m_nARWidth = w;
	m_nARHeight = h;
	UpdateRenderSize ();
	return QC_ERR_NONE;
}

int CBaseVideoRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	m_fmtVideo.nWidth = pFmt->nWidth;
	m_fmtVideo.nHeight = pFmt->nHeight;
	m_fmtVideo.nNum = pFmt->nNum;
	m_fmtVideo.nDen = pFmt->nDen;
	m_fmtVideo.nCodecID = pFmt->nCodecID;
	return QC_ERR_NONE;
}

int CBaseVideoRnd::Uninit (void)
{
	QC_DEL_A (m_fmtVideo.pHeadData);
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));

	QC_DEL_A(m_bufVideo.pBuff[0]);
	QC_DEL_A(m_bufVideo.pBuff[1]);
	QC_DEL_A(m_bufVideo.pBuff[2]);
	m_bufVideo.nWidth = 0;
	m_bufVideo.nHeight = 0;

	QC_DEL_A(m_bufRotate.pBuff[0]);
	QC_DEL_A(m_bufRotate.pBuff[1]);
	QC_DEL_A(m_bufRotate.pBuff[2]);
	m_bufRotate.nWidth = 0;
	m_bufRotate.nHeight = 0;

	return QC_ERR_NONE;
}

int	CBaseVideoRnd::Start (void)
{
	m_bPlay = true;
	return QC_ERR_NONE;
}

int CBaseVideoRnd::Pause (void)
{
	if (m_pClock != NULL)
		m_pClock->Pause ();

	m_bPlay = false;
	return QC_ERR_NONE;
}

int	CBaseVideoRnd::Stop (void)
{
	m_bPlay = false;
	return QC_ERR_NONE;
}

int CBaseVideoRnd::OnStart (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoRnd::OnStop (void)
{
	return QC_ERR_NONE;
}

int CBaseVideoRnd::Render (QC_DATA_BUFF * pBuff)
{
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
			m_nRndCount = 0;
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && (m_fmtVideo.nWidth != pFmt->nWidth || m_fmtVideo.nHeight != pFmt->nHeight))
		{
			m_fmtVideo.nWidth = pFmt->nWidth;
			m_fmtVideo.nHeight = pFmt->nHeight;
			m_fmtVideo.nNum = pFmt->nNum;
			m_fmtVideo.nDen = pFmt->nDen;
			UpdateRenderSize();
		}

		if (m_bufVideo.nWidth < m_fmtVideo.nWidth || m_bufVideo.nHeight < m_fmtVideo.nHeight)
		{
			QC_DEL_A(m_bufVideo.pBuff[0]);
			QC_DEL_A(m_bufVideo.pBuff[1]);
			QC_DEL_A(m_bufVideo.pBuff[2]);
			m_bufVideo.nWidth = 0;
			m_bufVideo.nHeight = 0;
		}
	}

	ConvertYUVData (pBuff);

	return QC_ERR_NONE;
}

int CBaseVideoRnd::WaitRendTime (long long llTime)
{
	if (m_pExtClock == NULL)
		return QC_ERR_STATUS;
	long long llPlayTime = m_pExtClock->GetTime ();
	while (llPlayTime < llTime)
	{
		qcSleep (5000);
		llPlayTime = m_pExtClock->GetTime ();
		if (!m_bPlay)
			return -1;
		if (m_bNewSource)
			return -1;
		if (m_pBaseInst != NULL && m_pBaseInst->m_bForceClose)
			break;
	}
	m_nRndCount++;
	return 0;
}

CBaseClock * CBaseVideoRnd::GetClock (void)
{
	if (m_pClock == NULL)
		m_pClock = new CBaseClock(m_pBaseInst);
	return m_pClock;
}

bool CBaseVideoRnd::UpdateRenderSize (void)
{
	if (m_fmtVideo.nWidth == 0 || m_fmtVideo.nHeight == 0)
		return false;

	m_rcVideo.top = 0;
	m_rcVideo.left = 0;
	m_rcVideo.right = m_fmtVideo.nWidth;
	m_rcVideo.bottom = m_fmtVideo.nHeight;

	int nRndW = m_rcView.right - m_rcView.left;
	int nRndH = m_rcView.bottom - m_rcView.top;

	if (m_nARWidth != 1 || m_nARHeight != 1)
	{
		if (nRndH * m_nARWidth >= m_nARHeight * nRndW)
			nRndH = nRndW * m_nARHeight / m_nARWidth;
		else 
			nRndW = nRndH * m_nARWidth / m_nARHeight;
	}
	else
	{
		int nWidth = m_fmtVideo.nWidth;
		int nHeight = m_fmtVideo.nHeight;
		if ((m_fmtVideo.nNum == 0 || m_fmtVideo.nNum == 1) &&
			(m_fmtVideo.nDen == 1 || m_fmtVideo.nDen == 0))
		{
			if (nWidth * nRndH >= nHeight * nRndW)
				nRndH = nRndW * nHeight / nWidth;
			else 
				nRndW = nRndH * nWidth / nHeight;
		}
		else
		{
			if (m_fmtVideo.nDen == 0)
				m_fmtVideo.nDen = 1;
			nWidth = nWidth * m_fmtVideo.nNum / m_fmtVideo.nDen;
			if (nWidth * nRndH >= nHeight * nRndW)
				nRndH = nRndW * nHeight / nWidth;
			else 
				nRndW = nRndH * nWidth / nHeight;
		}
	}

	m_rcRender.left = m_rcView.left + (GetRectW (&m_rcView) - nRndW) / 2;
	m_rcRender.top = m_rcView.top + (GetRectH (&m_rcView) - nRndH) / 2;
	m_rcRender.right = m_rcRender.left + nRndW;
	m_rcRender.bottom = m_rcRender.top + nRndH;
	m_rcRender.left = m_rcRender.left & ~3;
	m_rcRender.top = m_rcRender.top & ~1;
	m_rcRender.right = (m_rcRender.right+3) & ~3;
	m_rcRender.bottom = (m_rcRender.bottom+1) & ~1;

	m_bUpdateView = true;

	return true;
}

QC_VIDEO_BUFF * CBaseVideoRnd::ConvertYUVData(QC_DATA_BUFF * pBuff)
{
	QC_VIDEO_BUFF * pVideoBuff = NULL;
	if (pBuff->uBuffType == QC_BUFF_TYPE_Video)
		pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (pVideoBuff == NULL)
		return NULL;
	if (pVideoBuff->nType == QC_VDT_YUV420_P)
		return pVideoBuff;

	int nW, nH;
	unsigned char * pU1 = pVideoBuff->pBuff[1];
	unsigned char * pV1 = pVideoBuff->pBuff[1];
	unsigned char * pU2 = pVideoBuff->pBuff[2];
	unsigned char * pV2 = pVideoBuff->pBuff[2];

	if (m_bufVideo.pBuff[0] == NULL)
	{
		m_bufVideo.nWidth = m_fmtVideo.nWidth;
		m_bufVideo.nHeight = m_fmtVideo.nHeight;
		m_bufVideo.nStride[0] = ((m_bufVideo.nWidth + 32) + 3) / 4 * 4;
		m_bufVideo.nStride[1] = ((m_bufVideo.nStride[0] / 2) + 3) / 4 * 4;
		m_bufVideo.nStride[2] = ((m_bufVideo.nStride[0] / 2) + 3) / 4 * 4;
		m_bufVideo.pBuff[0] = new unsigned char[m_bufVideo.nStride[0] * m_bufVideo.nHeight];
		m_bufVideo.pBuff[1] = new unsigned char[m_bufVideo.nStride[0] * m_bufVideo.nHeight / 4];
		m_bufVideo.pBuff[2] = new unsigned char[m_bufVideo.nStride[0] * m_bufVideo.nHeight / 4];
	}

	if (pVideoBuff->nType == QC_VDT_YUV422_P)
	{
		// Cpoy Y data
		for (nH = 0; nH < pVideoBuff->nHeight; nH++)
			memcpy(m_bufVideo.pBuff[0] + m_bufVideo.nStride[0] * nH, pVideoBuff->pBuff[0] + pVideoBuff->nStride[0] * nH, pVideoBuff->nWidth);

		for (nH = 0; nH < pVideoBuff->nHeight / 2; nH++)
		{
			// Copy U data
			pU1 = m_bufVideo.pBuff[1] + m_bufVideo.nStride[1] * nH;
			pU2 = pVideoBuff->pBuff[1] + pVideoBuff->nStride[1] * nH * 2;
			memcpy(pU1, pU2, pVideoBuff->nWidth / 2);

			// Copy V data
			pV1 = m_bufVideo.pBuff[2] + m_bufVideo.nStride[2] * nH;
			pV2 = pVideoBuff->pBuff[2] + pVideoBuff->nStride[2] * nH * 2;
			memcpy(pV1, pV2, pVideoBuff->nWidth / 2);
		}
		return &m_bufVideo;
	}
	if (pVideoBuff->nType == QC_VDT_YUV444_P)
	{
		// Cpoy Y data
		for (nH = 0; nH < pVideoBuff->nHeight; nH++)
			memcpy(m_bufVideo.pBuff[0] + m_bufVideo.nStride[0] * nH, pVideoBuff->pBuff[0] + pVideoBuff->nStride[0] * nH, pVideoBuff->nWidth);

		for (nH = 0; nH < pVideoBuff->nHeight / 2; nH++)
		{
			// Copy U data
			pU1 = m_bufVideo.pBuff[1] + m_bufVideo.nStride[1] * nH;
			pU2 = pVideoBuff->pBuff[1] + pVideoBuff->nStride[1] * nH * 2;
			for (nW = 0; nW < pVideoBuff->nWidth / 2; nW++)
				*(pU1 + nW) = *(pU2 + nW * 2);

			// Copy V data
			pV1 = m_bufVideo.pBuff[2] + m_bufVideo.nStride[2] * nH;
			pV2 = pVideoBuff->pBuff[2] + pVideoBuff->nStride[2] * nH * 2;
			for (nW = 0; nW < pVideoBuff->nWidth / 2; nW++)
				*(pV1 + nW) = *(pV2 + nW * 2);
		}
		return &m_bufVideo;
	}

	return pVideoBuff;
}

QC_VIDEO_BUFF * CBaseVideoRnd::RotateYUVData(QC_VIDEO_BUFF * pBuff, int nAngle)
{
	if (pBuff == NULL)
		return NULL;
	if (pBuff->nType != QC_VDT_YUV420_P)
		return pBuff;

	if (nAngle == 90 || nAngle == 270)
	{
		if (m_bufRotate.nWidth < pBuff->nHeight || m_bufRotate.nHeight < pBuff->nWidth)
		{
			QC_DEL_A(m_bufRotate.pBuff[0]);
			QC_DEL_A(m_bufRotate.pBuff[1]);
			QC_DEL_A(m_bufRotate.pBuff[2]);
		}
		m_bufRotate.nWidth = (pBuff->nHeight + 15) / 16 * 16;
		m_bufRotate.nHeight = (pBuff->nWidth + 15) / 16 * 16;
	}
	else
	{
		if (m_bufRotate.nWidth < pBuff->nWidth || m_bufRotate.nHeight < pBuff->nHeight)
		{
			QC_DEL_A(m_bufRotate.pBuff[0]);
			QC_DEL_A(m_bufRotate.pBuff[1]);
			QC_DEL_A(m_bufRotate.pBuff[2]);
		}
		m_bufRotate.nWidth = (pBuff->nWidth + 15) / 16 * 16;
		m_bufRotate.nHeight = (pBuff->nHeight + 15) / 16 * 16;

	}

	if (m_bufRotate.pBuff[0] == NULL)
	{
		m_bufRotate.nStride[0] = ((m_bufRotate.nWidth + 32) + 3) / 4 * 4;
		m_bufRotate.nStride[1] = ((m_bufRotate.nStride[0] / 2) + 3) / 4 * 4;
		m_bufRotate.nStride[2] = ((m_bufRotate.nStride[0] / 2) + 3) / 4 * 4;
		m_bufRotate.pBuff[0] = new unsigned char[m_bufRotate.nStride[0] * m_bufRotate.nHeight];
		memset(m_bufRotate.pBuff[0], 0, m_bufRotate.nStride[0] * m_bufRotate.nHeight);
		m_bufRotate.pBuff[1] = new unsigned char[m_bufRotate.nStride[0] * m_bufRotate.nHeight / 4];
		memset(m_bufRotate.pBuff[1], 127, m_bufRotate.nStride[0] * m_bufRotate.nHeight / 4);
		m_bufRotate.pBuff[2] = new unsigned char[m_bufRotate.nStride[0] * m_bufRotate.nHeight / 4];
		memset(m_bufRotate.pBuff[2], 127, m_bufRotate.nStride[0] * m_bufRotate.nHeight / 4);
	}

	if (m_fColorCvtR != NULL)
		m_fColorCvtR(pBuff, &m_bufRotate, nAngle);

	return &m_bufRotate;
}
