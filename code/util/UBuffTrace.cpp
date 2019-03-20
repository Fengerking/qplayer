/*******************************************************************************
	File:		UBuffTrace.cpp

	Contains:	The buff trace implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "UBuffTrace.h"

#include "CBuffTrace.h"

CBuffTrace * qc_pBuffTrace = NULL;

int	qcBuff_Init (void)
{
	if (qc_pBuffTrace != NULL)
		return QC_ERR_NONE;
	qc_pBuffTrace = new CBuffTrace (NULL);
	return QC_ERR_NONE;
}

int	qcBuff_Close (void)
{
	QC_DEL_P (qc_pBuffTrace);
	return QC_ERR_NONE;
}

int	qcBuff_Trace (QC_DATA_BUFF * pBuff)
{
	if (qc_pBuffTrace == NULL)
		return QC_ERR_IMPLEMENT;
	return qc_pBuffTrace->Trace (pBuff);
}
