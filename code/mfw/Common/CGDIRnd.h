/*******************************************************************************
	File:		CGDIRnd.h

	Contains:	The Video GDI render header file.

	Written by:	Fenger King

	Change History (most recent first):
	2013-09-02		Fenger			Create file

*******************************************************************************/
#ifndef __CGDIRnd_H__
#define __CGDIRnd_H__
#include "windows.h"

#include "CBaseVideoRnd.h"

class CGDIRnd : public CBaseVideoRnd
{
public:
	CGDIRnd(CBaseInst * pBaseInst, void * hInst);
	virtual ~CGDIRnd(void);

	virtual int		SetView (void * hView, RECT * pRect);
	virtual int		UpdateDisp (void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Render (QC_DATA_BUFF * pBuff);

protected:
	virtual bool	UpdateRenderSize (void);
	bool			CreateResBMP (void);
	bool			ReleaseResBMP (void);
	bool			ReleaseResDC (void);

protected:
	HWND				m_hWnd;
	HDC					m_hWinDC;
	HDC					m_hMemDC;
	HBITMAP				m_hBmpVideo;
	LPBYTE				m_pBmpBuff;
	LPBYTE				m_pBmpInfo;
	int					m_nPixelBits;
	int					m_nRndStride;
	HBITMAP				m_hBmpOld;
};

#endif // __CGDIRnd_H__
