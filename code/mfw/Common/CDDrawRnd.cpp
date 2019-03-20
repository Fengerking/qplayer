/*******************************************************************************
	File:		CDDrawRnd.cpp

	Contains:	The Video DDraw render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-26		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CDDrawRnd.h"

#include "ULogFunc.h"

CDDrawRnd::CDDrawRnd(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoRnd (pBaseInst, hInst)
	, m_hWnd (NULL)
	, m_pDD (NULL)
	, m_pDDSPrimary (NULL)
	, m_pDDSOffScr (NULL)
	, m_pFourCC (NULL)
	, m_dwFourCC (0)
{
	SetObjectName ("CDDrawRnd");

	memset (&m_ddsd, 0, sizeof (DDSURFACEDESC));

	memset(&m_ddBltFX, 0, sizeof(DDBLTFX));
	m_ddBltFX.dwSize = sizeof(m_ddBltFX);

	memset (&m_rcDraw, 0, sizeof (m_rcDraw));
}

CDDrawRnd::~CDDrawRnd(void)
{
	Uninit ();
}

int CDDrawRnd::SetView (void * hView, RECT * pRect)
{
	CAutoLock lock (&m_mtDraw);

	if (m_hWnd == (HWND)hView)
	{
		if (pRect != NULL && !memcmp (pRect, &m_rcView, sizeof (m_rcView)))
			return QC_ERR_NONE;
	}
	else
	{
		ReleaseDD ();
	}

	m_hWnd = (HWND)hView;
	m_hView =hView;

	if (pRect == NULL || pRect->bottom == 0)
		GetClientRect (m_hWnd, &m_rcView);
	else
		memcpy (&m_rcView, pRect, sizeof (RECT));
	if (m_rcView.bottom == 0 || m_rcView.right == 0)
		GetClientRect (m_hWnd, &m_rcView);

	UpdateRenderSize ();

	if (m_pDDSPrimary == NULL)
		CreateDD ();

	return QC_ERR_NONE;
}

int CDDrawRnd::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (CBaseVideoRnd::Init (pFmt) != QC_ERR_NONE)
		return QC_ERR_STATUS;
	if (m_hWnd == NULL)
		return QC_ERR_STATUS;

	UpdateRenderSize ();

	if (m_pDD == NULL)
	{
		if (!CreateDD ())
			return QC_ERR_FAILED;
	}

	return QC_ERR_NONE;
}

int CDDrawRnd::Uninit (void)
{
	ReleaseDD ();
	return QC_ERR_NONE;
}

int CDDrawRnd::Render (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_STATUS;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		Init(pFmt);
	}
	CBaseVideoRnd::Render (pBuff);
	if (!IsWindowVisible (m_hWnd))
		return QC_ERR_NONE;
	RECT rcView;
	GetClientRect (m_hWnd, &rcView);
	if (rcView.bottom <= 16 || rcView.right <= 16)
		return QC_ERR_NONE;

	CAutoLock lock (&m_mtDraw);
	if (m_pDDSOffScr == NULL)
		return QC_ERR_STATUS;
	QC_VIDEO_BUFF * pVideoBuff = NULL;
	if (pBuff->uBuffType == QC_BUFF_TYPE_Video)
		pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
	if (pVideoBuff == NULL)
		return QC_ERR_RETRY;
	if (pVideoBuff->nType != QC_VDT_YUV420_P)
	{
		pVideoBuff = &m_bufVideo;
		if (pVideoBuff->nType != QC_VDT_YUV420_P)
			return QC_ERR_STATUS;
	}

	HRESULT ddRval = DDERR_WASSTILLDRAWING;
	while(ddRval == DDERR_WASSTILLDRAWING)
		ddRval = m_pDDSOffScr->Lock (NULL, &m_ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if(ddRval != DD_OK)
		return QC_ERR_FAILED;

	int		i = 0;
	LPBYTE	lpSurfY = (LPBYTE)m_ddsd.lpSurface;
	LPBYTE	lpSurfU = (LPBYTE)m_ddsd.lpSurface + m_ddsd.lPitch * m_ddsd.dwHeight;
	LPBYTE	lpSurfV = (LPBYTE)m_ddsd.lpSurface + m_ddsd.lPitch * m_ddsd.dwHeight * 5 / 4;
	for (i = 0; i < m_fmtVideo.nHeight; i++)
		memcpy (lpSurfY + i * m_ddsd.lPitch, pVideoBuff->pBuff[0] + pVideoBuff->nStride[0] * i, m_fmtVideo.nWidth);
	for (i = 0; i < m_fmtVideo.nHeight / 2; i++)
		memcpy (lpSurfV + i * m_ddsd.lPitch / 2, pVideoBuff->pBuff[1] + pVideoBuff->nStride[1] * i, m_fmtVideo.nWidth / 2);
	for (i = 0; i < m_fmtVideo.nHeight / 2; i++)
		memcpy (lpSurfU + i * m_ddsd.lPitch / 2, pVideoBuff->pBuff[2] + pVideoBuff->nStride[2] * i, m_fmtVideo.nWidth / 2);

	m_pDDSOffScr->Unlock(NULL);

	memcpy (&m_rcDest, &m_rcRender, sizeof (RECT));
	ClientToScreen(m_hWnd, (LPPOINT)&m_rcDest.left);
	ClientToScreen(m_hWnd, (LPPOINT)&m_rcDest.right);
	ddRval = m_pDDSPrimary->Blt(&m_rcDest, m_pDDSOffScr, &m_rcVideo, DDBLT_WAIT, &m_ddBltFX);
	if(ddRval != DD_OK)
		return QC_ERR_FAILED;

	m_nRndCount++;

	return QC_ERR_NONE;
}

bool CDDrawRnd::UpdateRenderSize (void)
{
	bool bRC = CBaseVideoRnd::UpdateRenderSize ();

	memcpy (&m_rcDest, &m_rcRender, sizeof (RECT));

	ClientToScreen(m_hWnd, (LPPOINT)&m_rcDest.left);
	ClientToScreen(m_hWnd, (LPPOINT)&m_rcDest.right);

	memcpy (&m_rcDraw, &m_rcVideo, sizeof (RECT));

	return bRC;
}

bool CDDrawRnd::CreateDD (void)
{
	HRESULT hr = S_OK;
	if (m_fmtVideo.nWidth <= 0 || m_fmtVideo.nHeight <= 0)
		return true;

	ReleaseDD ();
	if (DirectDrawCreateEx(NULL, (VOID**)&m_pDD, IID_IDirectDraw7, NULL) != DD_OK)
		return false;
	memset (&m_DDCaps, 0, sizeof (DDCAPS));
	m_DDCaps.dwSize = sizeof (DDCAPS);
	hr = m_pDD->GetCaps (&m_DDCaps, NULL);
	if (hr != DD_OK)
		return false;

	DWORD	nFourCC = 20;
	hr = m_pDD->GetFourCCCodes (&nFourCC, NULL);
	if (nFourCC > 0)
	{
		m_pFourCC = new DWORD[nFourCC];
		hr = m_pDD->GetFourCCCodes (&nFourCC, m_pFourCC);
	}
	else
	{
		return false;
	}

	if (m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL) != DD_OK)
		return false;

	ZeroMemory(&m_ddsd, sizeof(m_ddsd));
	m_ddsd.dwSize = sizeof(m_ddsd);
	m_ddsd.dwFlags = DDSD_CAPS;
	m_ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (m_pDD->CreateSurface(&m_ddsd, &m_pDDSPrimary, NULL) != DD_OK)
		return false;

	LPDIRECTDRAWCLIPPER  pcClipper;   // Cliper
	if( m_pDD->CreateClipper( 0, &pcClipper, NULL ) != DD_OK )
		return false;

	if( pcClipper->SetHWnd( 0, m_hWnd ) != DD_OK )
	{
		pcClipper->Release();
		return false;
	}

	if( m_pDDSPrimary->SetClipper( pcClipper ) != DD_OK )
	{
		pcClipper->Release();
		return false;
	}

	// Done with clipper
	pcClipper->Release();

	// Create YUV surface 
	ZeroMemory(&m_ddsd, sizeof(m_ddsd));
	m_ddsd.dwSize = sizeof(m_ddsd);
	m_ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	m_ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	m_ddsd.dwWidth = m_nMaxWidth; //m_fmtVideo.nWidth;	//m_nShowWidth;
	m_ddsd.dwHeight = m_nMaxHeight; //m_fmtVideo.nHeight; //m_nShowHeight;
	m_ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	m_ddsd.ddpfPixelFormat.dwFlags  = DDPF_FOURCC | DDPF_YUV ;
	if (m_pFourCC != NULL)
	{
		for (unsigned int i = 0; i < nFourCC; i++)
		{
			if (m_pFourCC[i] == MAKEFOURCC('Y','V', '1', '2'))
			{
				m_dwFourCC = MAKEFOURCC('Y','V', '1', '2');
				break;
			}
		}
	}
	if (m_dwFourCC == 0)
		m_dwFourCC = MAKEFOURCC('N','V', '1', '2');
	m_ddsd.ddpfPixelFormat.dwFourCC = m_dwFourCC;
	m_ddsd.ddpfPixelFormat.dwYUVBitCount = 8;
	m_pDD->CreateSurface(&m_ddsd, &m_pDDSOffScr, NULL);

	HRESULT ddRval = DDERR_WASSTILLDRAWING;
	while(ddRval == DDERR_WASSTILLDRAWING)
		ddRval = m_pDDSOffScr->Lock (NULL, &m_ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if(ddRval != DD_OK)
		return true;
	LPBYTE	lpSurfY = (LPBYTE)m_ddsd.lpSurface;
	LPBYTE	lpSurfU = (LPBYTE)m_ddsd.lpSurface + m_ddsd.lPitch * m_ddsd.dwHeight;
	LPBYTE	lpSurfV = (LPBYTE)m_ddsd.lpSurface + m_ddsd.lPitch * m_ddsd.dwHeight * 5 / 4;
	memset (lpSurfY, 0, m_ddsd.lPitch * m_ddsd.dwHeight);
	memset (lpSurfU, 127, m_ddsd.lPitch * m_ddsd.dwHeight / 4);
	memset (lpSurfV, 127, m_ddsd.lPitch * m_ddsd.dwHeight / 4);
	m_pDDSOffScr->Unlock(NULL);

	return m_pDDSOffScr == NULL ? false : true;
}

bool CDDrawRnd::ReleaseDD(void)
{
	if(m_pDD != NULL)
	{
		if(m_pDDSPrimary != NULL)
		{
			m_pDDSPrimary->Release();
			m_pDDSPrimary = NULL;
		}

		if (m_pDDSOffScr != NULL)
			m_pDDSOffScr->Release ();
		m_pDDSOffScr = NULL;

		m_pDD->Release();
		m_pDD = NULL;
	}

	QC_DEL_A (m_pFourCC);
	m_dwFourCC = 0;

	return TRUE;
}
