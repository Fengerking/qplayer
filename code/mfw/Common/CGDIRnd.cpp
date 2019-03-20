/*******************************************************************************
	File:		CGDIRnd.cpp

	Contains:	The base Video GDI render implement code

	Written by:	Fenger King

	Change History (most recent first):
	2013-09-02		Fenger			Create file

*******************************************************************************/
#include "CGDIRnd.h"

#include "ClConv.h"
#include "qcClrConv.h"
#include "ULogFunc.h"

#define iMASK_COLORS 3
#define SIZE_MASKS (iMASK_COLORS * sizeof(DWORD))
const DWORD bits565[3] = {0X0000F800, 0X000007E0, 0X0000001F,};

CGDIRnd::CGDIRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
	, m_hWnd (NULL)
	, m_hWinDC(NULL)
	, m_hMemDC(NULL)
	, m_hBmpVideo (NULL)
	, m_pBmpBuff (NULL)
	, m_pBmpInfo (NULL)
	, m_nPixelBits (32)
	, m_nRndStride (0)
	, m_hBmpOld (NULL)
{
	SetObjectName ("CGDIRnd");
	m_bufRender.nType = QC_VDT_RGBA;
}

CGDIRnd::~CGDIRnd(void)
{
	Uninit ();
}

int	CGDIRnd::SetView (void * hView, RECT * pRect)
{
	CAutoLock lock (&m_mtDraw);

	if (m_hWnd != (HWND)hView)
		ReleaseResDC ();

	m_hWnd = (HWND)hView;
	m_hView =hView;

	if (pRect == NULL)
		GetClientRect (m_hWnd, &m_rcView);
	else
		memcpy (&m_rcView, pRect, sizeof (RECT));
	if (m_rcView.right == 0 || m_rcView.bottom == 0)
		GetClientRect (m_hWnd, &m_rcView);

	if (m_hWinDC == NULL && m_hWnd != NULL)
	{
		m_hWinDC = GetDC (m_hWnd);
		m_hMemDC = ::CreateCompatibleDC (m_hWinDC);
	}

	UpdateRenderSize ();	

	return QC_ERR_NONE;
}

int CGDIRnd::UpdateDisp (void)
{
	CAutoLock lock (&m_mtDraw);
	if (m_hWinDC != NULL && m_hMemDC != NULL)
	{
		StretchBlt (m_hWinDC, m_rcRender.left, m_rcRender.top, GetRectW (&m_rcRender), GetRectH (&m_rcRender),
					m_hMemDC, 0, 0, m_fmtVideo.nWidth, m_fmtVideo.nHeight, SRCCOPY);
		return QC_ERR_NONE;
	}
	return QC_ERR_IMPLEMENT;
}

int CGDIRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	if (m_hWnd == NULL)
		return QC_ERR_STATUS;

	CAutoLock lock (&m_mtDraw);
	memcpy (&m_fmtVideo, pFmt, sizeof (QC_VIDEO_FORMAT));

	UpdateRenderSize ();

	if (CreateResBMP ())
	{
		return QC_ERR_NONE;
	}

	return QC_ERR_FAILED;
}

int CGDIRnd::Uninit (void)
{
	ReleaseResBMP ();
	ReleaseResDC ();
	return QC_ERR_NONE;
}

int CGDIRnd::Render (QC_DATA_BUFF * pBuff)
{
	int nRC = 0;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt->nWidth != m_fmtVideo.nWidth || pFmt->nHeight != m_fmtVideo.nHeight)
			Init(pFmt);
	}
	CBaseVideoRnd::Render (pBuff);
	if (m_hWnd == NULL || !IsWindowVisible (m_hWnd))
		return QC_ERR_NONE;

	RECT rcView;
	GetClientRect (m_hWnd, &rcView);
	if (rcView.bottom <= 16 || rcView.right <= 16)
		return QC_ERR_NONE;

	CAutoLock lock (&m_mtDraw);
	if (m_hBmpVideo == NULL)
	{
		CreateResBMP ();
		if (m_hBmpVideo == NULL)
			return QC_ERR_MEMORY;
	}
	QC_VIDEO_BUFF * pVideoBuff = NULL;
	if (pBuff->uBuffType == QC_BUFF_TYPE_Video)
		pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (pVideoBuff == NULL)
		return QC_ERR_RETRY;

//	qcColorConvert_c (pVideoBuff->pBuff[0], pVideoBuff->pBuff[2], pVideoBuff->pBuff[1], pVideoBuff->nStride[0],
//					m_pBmpBuff, m_nRndStride, m_fmtVideo.nWidth, m_fmtVideo.nHeight, pVideoBuff->nStride[1],pVideoBuff->nStride[2]);

//	nRC = libyuv::I420ToARGB(pVideoBuff->pBuff[0], pVideoBuff->nStride[0], pVideoBuff->pBuff[1], pVideoBuff->nStride[1], pVideoBuff->pBuff[2], pVideoBuff->nStride[2],
//						m_pBmpBuff, m_nRndStride, m_fmtVideo.nWidth, m_fmtVideo.nHeight);
	if (m_fColorCvtR != NULL)
		m_fColorCvtR(pVideoBuff, &m_bufRender, 0);

	//BitBlt(m_hWinDC, m_rcRender.left, m_rcRender.top, GetRectW (&m_rcRender), GetRectH (&m_rcRender), m_hMemDC, 0, 0, SRCCOPY);
	SetStretchBltMode(m_hWinDC, HALFTONE);
	StretchBlt (m_hWinDC, m_rcRender.left, m_rcRender.top, GetRectW (&m_rcRender), GetRectH (&m_rcRender),
				m_hMemDC, 0, 0, m_fmtVideo.nWidth, m_fmtVideo.nHeight, SRCCOPY);

	return nRC;
}

bool CGDIRnd::UpdateRenderSize (void)
{
	bool bRC = CBaseVideoRnd::UpdateRenderSize ();

	return bRC;
}

bool CGDIRnd::CreateResBMP (void )
{
	BITMAPINFO * pBmpInfo = NULL;
	if (m_pBmpInfo != NULL)
	{
		pBmpInfo = (BITMAPINFO *)m_pBmpInfo;
		if (m_fmtVideo.nWidth == pBmpInfo->bmiHeader.biWidth && pBmpInfo->bmiHeader.biHeight == -m_fmtVideo.nHeight)
			return true;
		delete[]m_pBmpInfo;
	}

	if (m_fmtVideo.nWidth == 0 || m_fmtVideo.nHeight == 0)
		return false;

	int nBmpSize = sizeof(BITMAPINFOHEADER);
	if (m_nPixelBits == 16)
		nBmpSize += SIZE_MASKS; // for RGB bitMask;

	m_pBmpInfo = new BYTE[nBmpSize];
	memset (m_pBmpInfo, 0, nBmpSize);

	pBmpInfo = (BITMAPINFO *)m_pBmpInfo;
	pBmpInfo->bmiHeader.biSize			= nBmpSize;
	pBmpInfo->bmiHeader.biWidth			= m_fmtVideo.nWidth;
	pBmpInfo->bmiHeader.biHeight		= 0 - m_fmtVideo.nHeight;
	pBmpInfo->bmiHeader.biBitCount		= (WORD)m_nPixelBits;
	if (m_nPixelBits == 16)
		pBmpInfo->bmiHeader.biCompression	= BI_BITFIELDS;
	else
		pBmpInfo->bmiHeader.biCompression	= BI_RGB;
	pBmpInfo->bmiHeader.biXPelsPerMeter	= 0;
	pBmpInfo->bmiHeader.biYPelsPerMeter	= 0;
	pBmpInfo->bmiHeader.biPlanes			= 1;

	if (m_nPixelBits == 16)
	{
		DWORD *	pBmiColors = (DWORD *)((LPBYTE)pBmpInfo + sizeof(BITMAPINFOHEADER));
		for (int i = 0; i < 3; i++)
    		*(pBmiColors + i) = bits565[i];
	}

	m_nRndStride = ((pBmpInfo->bmiHeader.biWidth * pBmpInfo->bmiHeader.biBitCount / 8) + 3) & ~3;
	pBmpInfo->bmiHeader.biSizeImage	= m_nRndStride * m_fmtVideo.nHeight;

	if (m_hWinDC == NULL)
		return false;

	m_hBmpVideo = CreateDIBSection(m_hWinDC , (BITMAPINFO *)m_pBmpInfo , DIB_RGB_COLORS , (void **)&m_pBmpBuff, NULL , 0);
	if (m_pBmpBuff != NULL)
		memset (m_pBmpBuff, 0, ((BITMAPINFO *)m_pBmpInfo)->bmiHeader.biSizeImage);

	m_hBmpOld = (HBITMAP)SelectObject (m_hMemDC, m_hBmpVideo);

	m_bufRender.nWidth = m_fmtVideo.nWidth;
	m_bufRender.nHeight = m_fmtVideo.nHeight;
	m_bufRender.pBuff[0] = m_pBmpBuff;
	m_bufRender.nStride[0] = m_nRndStride;

	return true;
}

bool CGDIRnd::ReleaseResBMP (void)
{
	QC_DEL_A (m_pBmpInfo);

	if (m_hBmpOld != NULL && m_hMemDC != NULL)
		SelectObject (m_hMemDC, m_hBmpOld);

	if (m_hBmpVideo != NULL)
		DeleteObject (m_hBmpVideo);
	m_hBmpVideo = NULL;
	m_pBmpBuff = NULL;

	return true;
}

bool CGDIRnd::ReleaseResDC (void)
{
	if (m_hWinDC != NULL)
	{
		DeleteDC (m_hMemDC);
		ReleaseDC (m_hWnd, m_hWinDC);
	}

	m_hMemDC = NULL;
	m_hWinDC = NULL;

	return true;
}
