/*******************************************************************************
	File:		CTestPlayer.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestPlayer_H__
#define __CTestPlayer_H__
#include "qcData.h"
#include "CBaseObject.h"

#include "qcPlayer.h"
#include "CTestBase.h"

class CTestPlayer : public CTestBase
{
public:
	CTestPlayer(CTestInst * pInst);
    virtual ~CTestPlayer(void);

	void			SetCurTask(void * pCurTask) { m_pCurTask = pCurTask; }
	virtual int		RecordFunc(int nRC, const char * pFuncName, int nID, void * pParam);

	int				Create(void);

	int				SetNotify	(QCPlayerNotifyEvent pFunc, void * pUserData);
	int				SetView		(void * hView, RECT * pRect);
	int				Open		(const char * pURL, int nFlag);
	int				Close		(void);
	int				Run			(void);
	int				Pause		(void);
	int				Stop		(void);
	QCPLAY_STATUS	GetStatus	(void);
	long long		GetDur		(void);
	long long		GetPos		(void);
	long long		SetPos		(long long llPos);
	int				SetVolume	(int nVolume);
	int				GetVolume	(void);
	int				GetParam	(int nID, void * pParam);
	int				SetParam	(int nID, void * pParam);

public:
#ifdef __QC_OS_WIN32__
	HMODULE				m_hDll;
#endif // __QC_OS_WIN32__
	QCCREATEPLAYER *	m_fCreate;
	QCDESTROYPLAYER *	m_fDestroy;
	QCM_Player			m_player;
	CBaseInst *			m_pBaseInst;
	
	void *				m_pCurTask;
};

#endif //__CTestPlayer_H__
