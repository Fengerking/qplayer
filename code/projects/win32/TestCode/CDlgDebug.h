/*******************************************************************************
	File:		CDlgDebug.h

	Contains:	the debug dialog header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-25		Bangfei			Create file

*******************************************************************************/
#ifndef __CDlgDebug_H__
#define __CDlgDebug_H__

#include "CNodeList.h"
#include "UMsgMng.h"

class CDlgDebug : public CMsgReceiver
{
public:
	static INT_PTR CALLBACK OpenDebugDlgProc (HWND, UINT, WPARAM, LPARAM);

public:
	CDlgDebug(HINSTANCE hInst, HWND hWnd);
	virtual ~CDlgDebug(void);

	int				OpenDlg (void);

	virtual int		ReceiveMsg (CMsgItem * pItem);

protected:
	virtual int		FillMsg (void);
	virtual int		ShowMsgItem (CMsgItem * pItem, bool bOne = true);

protected:
	HINSTANCE		m_hInst;
	HWND			m_hParent;
	HWND			m_hDlg;
	HWND			m_hLstMsg;

	char			m_szFilter[128];
	int				m_nStartTime;

	CObjectList<CMsgItem>	m_lstMsg;
	int						m_nIndex;
	bool					m_bPause;
};
#endif //__CDlgDebug_H__