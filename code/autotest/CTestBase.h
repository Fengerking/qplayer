/*******************************************************************************
	File:		CTestBase.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CTestBase_H__
#define __CTestBase_H__
#include "qcData.h"

#include "CTestInst.h"

#define	QCTEST_TASK_START		1001
#define	QCTEST_TASK_EXIT		1002

#define	QCTEST_TASK_ITEM		1100

class CTestBase
{
public:
	CTestBase(CTestInst * pInst);
    virtual ~CTestBase(void);

	virtual int		PostTask(int nTaskID, int nDelay, int nValue, long long llValue, char * pName);
	virtual int		ResetTask(void);

protected:
	CTestInst *		m_pInst;

};

#endif //__CTestBase_H__
