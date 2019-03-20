/*******************************************************************************
	File:		CBuffTrace.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#ifndef __CBuffTrace_H__
#define __CBuffTrace_H__
#include "qcData.h"
#include "CBaseObject.h"

class CBuffTrace : public CBaseObject
{
public:
	CBuffTrace(CBaseInst * pBaseInst);
    virtual ~CBuffTrace(void);

	int		Trace (QC_DATA_BUFF * pBuff);

};

#endif //__CBuffTrace_H__
