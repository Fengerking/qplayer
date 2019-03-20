/*******************************************************************************
	File:		CTestBase.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CTestBase.h"

CTestBase::CTestBase(CTestInst * pInst)
	: m_pInst(pInst)
{
}

CTestBase::~CTestBase(void)
{
}

int	CTestBase::PostTask(int nTaskID, int nDelay, int nValue, long long llValue, char * pName)
{
	return QC_ERR_FAILED;
}

int	CTestBase::ResetTask(void)
{
	return QC_ERR_NONE;
}

