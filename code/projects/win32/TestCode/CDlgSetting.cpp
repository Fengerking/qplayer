/*******************************************************************************
	File:		CDlgSetting.cpp

	Contains:	the debug dialog implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#include "windows.h"
#include "tchar.h"

#include "resource.h"

#include "CDlgSetting.h"

#pragma warning (disable : 4996)

CDlgSetting::CDlgSetting(HINSTANCE hInst, HWND hWnd)
	: m_hInst (hInst)
	, m_hParent (hWnd)
	, m_hDlg (NULL)
{
	m_bLoop = false;
	m_bHttpPD = false;
	m_bSameSource = false;
	m_bHWDec = false;
}

CDlgSetting::~CDlgSetting(void)
{

}

int CDlgSetting::OpenDlg (void)
{
//	int nRC = DialogBoxParam (m_hInst, MAKEINTRESOURCE(IDD_DIALOG_DEBUG), m_hParent, OpenDebugDlgProc, (LPARAM)this);
	m_hDlg = CreateDialog(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_SETTING), m_hParent, OpenDebugDlgProc);

	RECT rcDlg;
	GetClientRect (m_hDlg, &rcDlg);

	SetWindowLong (m_hDlg, GWL_USERDATA, (LONG)this);
	ShowWindow (m_hDlg, SW_SHOW);

	return QC_ERR_NONE;
}

INT_PTR CALLBACK CDlgSetting::OpenDebugDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int				wmId, wmEvent;
	RECT			rcDlg;
	CDlgSetting *	pDlg = NULL;

	if (hDlg != NULL)
	{
		GetClientRect (hDlg, &rcDlg);
		pDlg = (CDlgSetting *)GetWindowLong(hDlg, GWL_USERDATA);
	}

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_CHECK_HW:
			pDlg->m_bHWDec = SendMessage(GetDlgItem(hDlg, wmId), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			break;

		case IDC_CHECK_SAME:
			pDlg->m_bSameSource = SendMessage(GetDlgItem(hDlg, wmId), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			break;

		case IDC_CHECK_PD:
			pDlg->m_bHttpPD = SendMessage(GetDlgItem(hDlg, wmId), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			break;

		case IDC_CHECK_LOOP:
			pDlg->m_bLoop = SendMessage(GetDlgItem(hDlg, wmId), BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
			break;

		case IDOK:
			break;

		case IDCANCEL:
			DestroyWindow (hDlg);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}

	return (INT_PTR)FALSE;
}
