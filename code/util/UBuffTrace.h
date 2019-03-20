/*******************************************************************************
	File:		UBuffTrace.h

	Contains:	The buffer trace header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __UBuffTrace_H__
#define __UBuffTrace_H__

#include "qcType.h"
#include "qcData.h"

int		qcBuff_Init (void);
int		qcBuff_Close (void);

int		qcBuff_Trace (QC_DATA_BUFF * pBuff);

#endif // __UBuffTrace_H__
