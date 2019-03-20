/*******************************************************************************
	File:		CDlgSetting.h

	Contains:	the debug dialog header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#ifndef __CDlgSetting_H__
#define __CDlgSetting_H__

#include "CNodeList.h"
#include "UMsgMng.h"

class CDlgSetting
{
public:
	static INT_PTR CALLBACK OpenDebugDlgProc (HWND, UINT, WPARAM, LPARAM);

public:
	CDlgSetting(HINSTANCE hInst, HWND hWnd);
	virtual ~CDlgSetting(void);

	int				OpenDlg (void);
	HWND			GetDlg(void) { return m_hDlg; }

protected:
	HINSTANCE		m_hInst;
	HWND			m_hParent;
	HWND			m_hDlg;

public:
	bool			m_bLoop;
	bool			m_bHttpPD;
	bool			m_bSameSource;
	bool			m_bHWDec;
};
#endif //__CDlgSetting_H__