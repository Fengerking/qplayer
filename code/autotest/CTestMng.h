/*******************************************************************************
	File:		CTestMng.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestMng_H__
#define __CTestMng_H__
#include "qcData.h"
#include "CBaseObject.h"

#include "CTestPlayer.h"

#include "CTestItem.h"
#include "CTestTask.h"

#include "CNodeList.h"
#include "CThreadWork.h"

class CTestMng : public CTestBase , public CThreadFunc
{
public:
	CTestMng(void);
    virtual ~CTestMng(void);

    virtual int     AddTestFile(const char * pFile);
	virtual int		OpenTestFile(const char * pFile);
    virtual int		StartTest(void);
	virtual int		ExitTest(void);
    virtual int		SetPlayer(CTestPlayer* pPlayer);
    

	virtual int		PostTask(int nTaskID, int nDelay, int nValue, long long llValue, char * pName);
	virtual int		ResetTask(void);
	CTestInst *		GetInst(void) { return m_pInst; }

protected:
	virtual int		OnHandleEvent(CThreadEvent * pEvent);
	virtual int		OnWorkItem(void);

protected:
	CTestPlayer *			m_pPlayer;
	CObjectList<CTestTask>	m_lstTask;
	CTestTask *				m_pCurTask;

	CThreadWork *			m_pThreadWork;
	int						m_nStartTime;
	int						m_nGetPosTime;
};

#endif //__CTestMng_H__
