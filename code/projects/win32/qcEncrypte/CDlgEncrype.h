/*******************************************************************************
	File:		CDlgEncrype.h

	Contains:	the debug dialog header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#ifndef __CDlgEncrype_H__
#define __CDlgEncrype_H__

#include "CRegMng.h"
#include "CNodeList.h"

typedef struct
{
	char *	pSrcFile;
	char *	pDstFile;
	char *	pKeyComp;
	char *	pKeyFile;
} QC_ENCRYPE_ITEM;

class CDlgEncrype
{
public:
	static INT_PTR CALLBACK OpenEncrypeDlgProc (HWND, UINT, WPARAM, LPARAM);

public:
	CDlgEncrype(HINSTANCE hInst, HWND hWnd);
	virtual ~CDlgEncrype(void);

	int				OpenDlg (void);
	HWND			GetDlg(void) { return m_hDlg; }

protected:
	bool			OpenMP4File(bool bOpen);
	bool			EncrypeFile(void);
	bool			OpenLstFile(void);
	bool			EncrypeItem(void);
	bool			FreeList(void);
	int				ReadTextLine(char * pData, int nSize, char * pLine, int nLine);

protected:
	HINSTANCE		m_hInst;
	HWND			m_hParent;
	HWND			m_hDlg;
	CRegMng *		m_pReg;

	CObjectList<QC_ENCRYPE_ITEM>	m_lstItem;
	NODEPOS							m_hCurPos;
	int								m_nCount;
	char *							m_pFileBuff;
};
#endif //__CDlgEncrype_H__