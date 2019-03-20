/*******************************************************************************
	File:		CDDrawRnd.h

	Contains:	The Video DDraw render header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-26		Bangfei			Create file

*******************************************************************************/
#ifndef __CDDrawRnd_H__
#define __CDDrawRnd_H__
#include "windows.h"

#include "CBaseVideoRnd.h"

#include <ddraw.h>
#include <mmsystem.h>

class CDDrawRnd : public CBaseVideoRnd
{
public:
	CDDrawRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CDDrawRnd(void);

	virtual int		SetView (void * hView, RECT * pRect);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

protected:
	virtual bool	UpdateRenderSize (void);
	bool			CreateDD (void);
	bool			ReleaseDD (void);

protected:
	HWND					m_hWnd;
	RECT					m_rcDest;	
	IDirectDraw7 *			m_pDD;			
	IDirectDrawSurface7 *	m_pDDSPrimary;  
	IDirectDrawSurface7 *	m_pDDSOffScr;
	DDSURFACEDESC2			m_ddsd;			
	DDCAPS					m_DDCaps;
	DDBLTFX					m_ddBltFX;
	DWORD *					m_pFourCC;
	DWORD					m_dwFourCC;

	RECT					m_rcDraw;
};

#endif // __CDDrawRnd_H__
