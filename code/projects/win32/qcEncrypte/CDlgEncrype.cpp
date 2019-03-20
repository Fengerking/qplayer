/*******************************************************************************
	File:		CDlgEncrype.cpp

	Contains:	the debug dialog implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#include "windows.h"
#include "tchar.h"
#include <Commdlg.h>
#include <CommCtrl.h>
#include <winuser.h>
#include <shellapi.h>

#include "resource.h"

#include "CDlgEncrype.h"
#include "CMP4Encrype.h"

#pragma warning (disable : 4996)

#define QC_ENCRYPE_LISTFILE_HEAD "<This is qiniu mp4 encrype file>"

CDlgEncrype::CDlgEncrype(HINSTANCE hInst, HWND hWnd)
	: m_hInst (hInst)
	, m_hParent (hWnd)
	, m_hDlg (NULL)
	, m_pFileBuff(NULL)
{
	m_pReg = new CRegMng("KeyText");
}

CDlgEncrype::~CDlgEncrype(void)
{
	DestroyWindow(m_hDlg);
	delete m_pReg;
	FreeList();
	if (m_pFileBuff != NULL)
		delete[]m_pFileBuff;
}

int CDlgEncrype::OpenDlg (void)
{
	m_hDlg = CreateDialog(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_ENCRYPE), m_hParent, OpenEncrypeDlgProc);

	SetWindowLong (m_hDlg, GWL_USERDATA, (LONG)this);
	ShowWindow (m_hDlg, SW_SHOW);

	TCHAR * pKey = m_pReg->GetTextValue("CompKey");
	SetDlgItemText(m_hDlg, IDC_EDIT_COMPKEY, pKey);
	pKey = m_pReg->GetTextValue("FileKey");
	SetDlgItemText(m_hDlg, IDC_EDIT_FILEKEY, pKey);

//	CMP4Encrype encryFile;
//	encryFile.UnencrypeFile("C:\\Temp\\s033d.mp4", "C:\\Temp\\s033d_0.mp4");

	return 0;
}

INT_PTR CALLBACK CDlgEncrype::OpenEncrypeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int				wmId, wmEvent;
	RECT			rcDlg;
	CDlgEncrype *	pDlg = NULL;

	if (hDlg != NULL)
	{
		GetClientRect (hDlg, &rcDlg);
		pDlg = (CDlgEncrype *)GetWindowLong(hDlg, GWL_USERDATA);
	}

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_TIMER:
		pDlg->EncrypeItem();
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_BUTTON_OPEN:
			pDlg->OpenMP4File(true);
			break;

		case IDC_BUTTON_SAVE:
			pDlg->OpenMP4File(false);
			break;

		case IDC_BUTTON_ENCRYPE:
			pDlg->EncrypeFile();
			break;

		case ID_FILE_FILE:
			pDlg->OpenLstFile();
			break;

		case IDOK:
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

bool CDlgEncrype::OpenMP4File(bool bOpen)
{
	char				szFile[1024] = { 0 };
	OPENFILENAME		ofn;
	memset(szFile, 0, sizeof(szFile));
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hDlg;
	ofn.lpstrFilter = TEXT("Media File (*.mp4)\0*.mp4\0");
	if (_tcsstr(szFile, _T(":/")) != NULL)
		_tcscpy(szFile, _T("*.mp4"));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;

	if (bOpen)
	{
		ofn.lpstrTitle = TEXT("Open mp4 File");
		ofn.Flags = OFN_READONLY;
		if (!GetOpenFileName(&ofn))
			return false;
		SetDlgItemText(m_hDlg, IDC_EDIT_FILE, szFile);

		char * pDot = strrchr(szFile, '.');
		if (pDot == NULL)
			return false;
		*pDot = 0;
		strcat(szFile, "_encry.mp4");
		SetDlgItemText(m_hDlg, IDC_EDIT_FILE2, szFile);
	}
	else
	{
		ofn.lpstrTitle = TEXT("Save mp4 File");
		ofn.Flags = OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn))
			return false;
		SetDlgItemText(m_hDlg, IDC_EDIT_FILE2, szFile);
	}

	return true;
}

bool CDlgEncrype::EncrypeFile(void)
{
	char szSrcFile[1024];
	char szDstFile[1024];
	char szKeyComp[256];
	char szKeyFile[256];

	memset(szSrcFile, 0, sizeof(szSrcFile));
	memset(szDstFile, 0, sizeof(szDstFile));
	memset(szKeyComp, 0, sizeof(szKeyComp));
	memset(szKeyFile, 0, sizeof(szKeyFile));
	GetDlgItemText(m_hDlg, IDC_EDIT_FILE, szSrcFile, sizeof(szSrcFile));
	GetDlgItemText(m_hDlg, IDC_EDIT_FILE2, szDstFile, sizeof(szDstFile));
	GetDlgItemText(m_hDlg, IDC_EDIT_COMPKEY, szKeyComp, sizeof(szKeyComp));
	GetDlgItemText(m_hDlg, IDC_EDIT_FILEKEY, szKeyFile, sizeof(szKeyFile));

	if (strlen(szSrcFile) <= 4)
	{
		MessageBox(m_hDlg, "源文件名错误！", "错误", MB_OK);
		return false;
	}
	if (strlen(szDstFile) <= 4)
	{
		MessageBox(m_hDlg, "目标文件名错误！", "错误", MB_OK);
		return false;
	}
	if (strlen(szKeyComp) <= 0 || strlen(szKeyComp) > 8)
	{
		MessageBox(m_hDlg, "公司秘钥错误！", "错误", MB_OK);
		return false;
	}
	if (strlen(szKeyFile) <= 0 || strlen(szKeyFile) > 8)
	{
		MessageBox(m_hDlg, "文件秘钥错误！", "错误", MB_OK);
		return false;
	}

	m_pReg->SetTextValue("CompKey", szKeyComp);
	m_pReg->SetTextValue("FileKey", szKeyFile);

	CMP4Encrype encryFile;
	//	encryFile.EncrypeFile("c:/temp/20180719152757656867701.mp4", "c:/temp/20180719152757656867701_encry.mp4", "rckv", "ukll");
	if (encryFile.EncrypeFile(szSrcFile, szDstFile, szKeyComp, szKeyFile))
	{
		MessageBox(m_hDlg, "加密成功！", "结果", MB_OK);
	}
	else
	{
		MessageBox(m_hDlg, "加密失败！", "结果", MB_OK);
	}

	return true;
}

bool CDlgEncrype::OpenLstFile(void)
{
	char				szFile[1024] = { 0 };
	OPENFILENAME		ofn;
	memset(szFile, 0, sizeof(szFile));
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hDlg;
	ofn.lpstrFilter = TEXT("Encrype List File (*.lst)\0*.lst\0");
	if (_tcsstr(szFile, _T(":/")) != NULL)
		_tcscpy(szFile, _T("*.lst"));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;

	ofn.lpstrTitle = TEXT("Open List File");
	ofn.Flags = OFN_READONLY;
	if (!GetOpenFileName(&ofn))
		return false;

	FreeList();

	HANDLE hFileSrc = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, (DWORD)0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
		return false;
	DWORD		dwRead = 0;
	DWORD		dwSize = GetFileSize(hFileSrc, NULL);
	if (dwSize < 32)
	{
		CloseHandle(hFileSrc);
		return false;
	}

	if (m_pFileBuff != NULL)
		delete[]m_pFileBuff;
	m_pFileBuff = new char[dwSize];
	ReadFile(hFileSrc, m_pFileBuff, dwSize, &dwRead, NULL);
	CloseHandle(hFileSrc);

	char	szLine[1024];
	int		nRestSize = dwRead;
	char *	pBuff = m_pFileBuff;
	int		nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
	if (strstr(szLine, QC_ENCRYPE_LISTFILE_HEAD) == NULL)
		return false;

	nRestSize -= nLineSize;
	pBuff += nLineSize;
	while (nRestSize > 0)
	{
		nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
		nRestSize -= nLineSize;
		pBuff += nLineSize;
		if (nRestSize <= 0)
			return false;

		if (szLine[0] == '[')
		{
			QC_ENCRYPE_ITEM * pItem = new QC_ENCRYPE_ITEM();
			memset(pItem, 0, sizeof(QC_ENCRYPE_ITEM));
			m_lstItem.AddTail(pItem);

			// Get the source file name
			nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
			nRestSize -= nLineSize;
			pBuff += nLineSize;
			pItem->pSrcFile = new char[strlen(szLine) + 1];
			strcpy(pItem->pSrcFile, szLine);
			if (nRestSize <= 0)
				return false;

			// Get the dest file name
			nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
			nRestSize -= nLineSize;
			pBuff += nLineSize;
			pItem->pDstFile = new char[strlen(szLine) + 1];
			strcpy(pItem->pDstFile, szLine);
			if (nRestSize <= 0)
				return false;

			// Get the company key
			nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
			nRestSize -= nLineSize;
			pBuff += nLineSize;
			pItem->pKeyComp = new char[strlen(szLine) + 1];
			strcpy(pItem->pKeyComp, szLine);
			if (nRestSize <= 0)
				return false;

			// Get the file key
			nLineSize = ReadTextLine(pBuff, nRestSize, szLine, sizeof(szLine));
			nRestSize -= nLineSize;
			pBuff += nLineSize;
			pItem->pKeyFile = new char[strlen(szLine) + 1];
			strcpy(pItem->pKeyFile, szLine);
		}
	}

	m_hCurPos = m_lstItem.GetHeadPosition();
	m_nCount = 0;
	SetTimer(m_hDlg, 1001, 100, NULL);

	return true;
}

bool CDlgEncrype::EncrypeItem(void)
{
	if (m_hCurPos == NULL)
		return false;

	QC_ENCRYPE_ITEM * pItem = m_lstItem.GetNext(m_hCurPos);
	if (pItem == NULL)
		return false;
	if (pItem->pSrcFile == NULL || pItem->pDstFile == NULL || pItem->pKeyComp == NULL || pItem->pKeyFile == NULL)
		return false;

	SetDlgItemText(m_hDlg, IDC_EDIT_FILE, pItem->pSrcFile);
	SetDlgItemText(m_hDlg, IDC_EDIT_FILE2, pItem->pDstFile);
	SetDlgItemText(m_hDlg, IDC_EDIT_COMPKEY, pItem->pKeyComp);
	SetDlgItemText(m_hDlg, IDC_EDIT_FILEKEY, pItem->pKeyFile);

	CMP4Encrype encryFile;
	encryFile.EncrypeFile(pItem->pSrcFile, pItem->pDstFile, pItem->pKeyComp, pItem->pKeyFile);
	m_nCount++;
	SendMessage(GetDlgItem(m_hDlg, IDC_PROG_ENCRYPE), PBM_SETPOS, m_nCount * 100 / m_lstItem.GetCount (), 0);

	if (m_hCurPos == NULL)
	{
		KillTimer(m_hDlg, 1001);
		MessageBox(m_hDlg, "加密完成！", "结果", MB_OK);
	}
}

bool CDlgEncrype::FreeList(void)
{
	QC_ENCRYPE_ITEM * pItem = m_lstItem.RemoveHead();
	while (pItem != NULL)
	{
		if (pItem->pSrcFile != NULL)
			delete[]pItem->pSrcFile;
		if (pItem->pDstFile != NULL)
			delete[]pItem->pDstFile;
		if (pItem->pKeyComp != NULL)
			delete[]pItem->pKeyComp;
		if (pItem->pKeyFile != NULL)
			delete[]pItem->pKeyFile;
		delete pItem;
		pItem = m_lstItem.RemoveHead();
	}
	m_hCurPos = NULL;
	m_nCount = 0;
	return true;
}

int	CDlgEncrype::ReadTextLine(char * pData, int nSize, char * pLine, int nLine)
{
	if (pData == NULL)
		return 0;
	char * pBuff = pData;
	while (pBuff - pData < nSize)
	{
		if (*pBuff == '\r' || *pBuff == '\n')
		{
			pBuff++;
			if (*(pBuff) == '\r' || *(pBuff) == '\n')
				pBuff++;
			break;
		}
		pBuff++;
	}

	int nLineLen = pBuff - pData;
	if (nLine > nLineLen)
	{
		int nRNLen = 0;
		pBuff--;
		while (pBuff >= pData && (*pBuff == '\r' || *pBuff == '\n' || *pBuff == ' '))
		{
			nRNLen++;
			pBuff--;
		}

		memset(pLine, 0, nLine);
		strncpy(pLine, pData, nLineLen - nRNLen);
	}
	return nLineLen;
}
