/*******************************************************************************
	File:		CBuffTrace.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBuffTrace.h"

CBuffTrace::CBuffTrace(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
{
	SetObjectName ("CBuffTrace");
}

CBuffTrace::~CBuffTrace(void)
{
}

int	CBuffTrace::Trace (QC_DATA_BUFF * pBuff)
{
	return QC_ERR_NONE;
}

