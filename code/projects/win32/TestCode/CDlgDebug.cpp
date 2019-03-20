/*******************************************************************************
	File:		CDlgDebug.cpp

	Contains:	the debug dialog implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#include "windows.h"
#include "tchar.h"

#include "resource.h"

#include "CDlgDebug.h"

#pragma warning (disable : 4996)

CDlgDebug::CDlgDebug(HINSTANCE hInst, HWND hWnd)
	: m_hInst (hInst)
	, m_hParent (hWnd)
	, m_hDlg (NULL)
	, m_hLstMsg (NULL)
	, m_nIndex (0)
	, m_bPause (false)
{
	strcpy (m_szFilter, "");
	m_nStartTime = 0;
}

CDlgDebug::~CDlgDebug(void)
{
	CMsgItem * pItem = m_lstMsg.RemoveHead ();
	while (pItem != NULL)
	{
		delete pItem;
		pItem = m_lstMsg.RemoveHead ();
	}
}

int CDlgDebug::OpenDlg (void)
{
	m_bPause = false;
//	int nRC = DialogBoxParam (m_hInst, MAKEINTRESOURCE(IDD_DIALOG_DEBUG), m_hParent, OpenDebugDlgProc, (LPARAM)this);
	m_hDlg = CreateDialog (m_hInst, MAKEINTRESOURCE(IDD_DIALOG_DEBUG), NULL, OpenDebugDlgProc);

	RECT rcDlg;
	GetClientRect (m_hDlg, &rcDlg);

	SetWindowLong (m_hDlg, GWL_USERDATA, (LONG)this);
	m_hLstMsg = GetDlgItem (m_hDlg, IDC_LIST_INFO);
	SetWindowPos (m_hDlg, NULL, (GetSystemMetrics (SM_CXSCREEN) - rcDlg.right) / 2, 
					(GetSystemMetrics (SM_CYSCREEN) - rcDlg.bottom ) / 2, 0, 0, SWP_NOSIZE);
	ShowWindow (m_hDlg, SW_SHOW);
	FillMsg ();

	return QC_ERR_NONE;
}

INT_PTR CALLBACK CDlgDebug::OpenDebugDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int				wmId, wmEvent;
	RECT			rcDlg;
	CDlgDebug *		pDlgDebug = NULL;

	if (hDlg != NULL)
	{
		GetClientRect (hDlg, &rcDlg);
		pDlgDebug = (CDlgDebug *)GetWindowLong (hDlg, GWL_USERDATA);
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
		case IDC_BUTTON_UPDATE:
			GetDlgItemText (hDlg, IDC_EDIT_FILTER, pDlgDebug->m_szFilter, sizeof (pDlgDebug->m_szFilter));
			pDlgDebug->FillMsg ();
			break;

		case IDC_BUTTON_PAUSE:
			pDlgDebug->m_bPause = !pDlgDebug->m_bPause;
			if (pDlgDebug->m_bPause)
			{
				SetDlgItemText (hDlg, IDC_BUTTON_PAUSE, "Continue");
			}
			else
			{
				pDlgDebug->FillMsg ();
				SetDlgItemText (hDlg, IDC_BUTTON_PAUSE, "Pause");
			}
			break;

		case IDOK:
		{
			char szFilter[64];
			GetDlgItemText (hDlg, IDC_EDIT_FILTER, szFilter, sizeof (szFilter));
			if (strcmp (pDlgDebug->m_szFilter, szFilter))
			{
				strcpy (pDlgDebug->m_szFilter, szFilter);
				pDlgDebug->FillMsg ();
			}
		}
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

int	CDlgDebug::ReceiveMsg (CMsgItem * pItem)
{
	if (pItem->m_nMsgID == QC_MSG_SNKA_RENDER || pItem->m_nMsgID == QC_MSG_SNKV_RENDER)
		return 0;

	CMsgItem * pNewItem = new CMsgItem (pItem);
	m_lstMsg.AddTail (pNewItem);

	ShowMsgItem (pNewItem, true);

	return 0;
}

int CDlgDebug::FillMsg (void)
{
	SendMessage (m_hLstMsg, LB_RESETCONTENT, 0, 0);
	m_nIndex = 0;

	CMsgItem * pItem = NULL;
	NODEPOS pPos = m_lstMsg.GetHeadPosition ();
	while (pPos != NULL)
	{
		pItem = m_lstMsg.GetNext (pPos);
		ShowMsgItem (pItem, false);
	}
	return 0;
}

int CDlgDebug::ShowMsgItem (CMsgItem * pItem, bool bOne)
{
	if (m_hLstMsg == NULL || m_bPause)
		return -1;

	int		nStartPos = 0;
	int		nTxtLen = 1024;
	if (pItem->m_szValue != NULL)
		nTxtLen = strlen(pItem->m_szValue) + 512;
	char * pTxtLine = new char[nTxtLen];
	char * pTxtItem = new char[nTxtLen];

	if (m_nStartTime == 0)
		m_nStartTime = pItem->m_nTime;
	int		tm = (pItem->m_nTime - m_nStartTime) / 1000;

	memset(pTxtLine, ' ', nTxtLen);
	pTxtLine[nTxtLen - 1] = 0;

	sprintf (pTxtItem, "% 6d",	m_nIndex++);
	memcpy (pTxtLine + nStartPos, pTxtItem, strlen (pTxtItem));
	nStartPos += 10;

	memcpy (pTxtLine + nStartPos, pItem->m_szIDName, strlen (pItem->m_szIDName));
	nStartPos += 32;

	sprintf (pTxtItem, "%02d : %02d : %02d : %03d", tm/3600, (tm%3600)/60, tm%60, (pItem->m_nTime - m_nStartTime)%1000);
	memcpy (pTxtLine + nStartPos, pTxtItem, strlen (pTxtItem));
	nStartPos += 20;

	sprintf (pTxtItem, "% 10d", pItem->m_nValue);
	memcpy (pTxtLine + nStartPos, pTxtItem, strlen (pTxtItem));
	nStartPos += 12;

	sprintf (pTxtItem, "% 12lld", pItem->m_llValue);
	memcpy (pTxtLine + nStartPos, pTxtItem, strlen (pTxtItem));
	nStartPos += 16;

	int nLen = 0;
	if (pItem->m_szValue != NULL)
	{
		nLen = strlen (pItem->m_szValue);
		if (nLen > 1023 - nStartPos)
			nLen = 1023 - nStartPos;
		memcpy (pTxtLine + nStartPos, pItem->m_szValue, nLen);
	}
	pTxtLine[nStartPos + nLen + 1] = 0;

	if (strlen (m_szFilter) > 0)
	{
		if (strstr (pTxtLine, m_szFilter) == NULL)
			return 0;
	}
	int nID = SendMessage (m_hLstMsg, LB_INSERTSTRING, 0, (LPARAM)pTxtLine);
	delete []pTxtLine;
	delete []pTxtItem;
	return 0;
}
