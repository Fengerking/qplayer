/*******************************************************************************
	File:		ULibFunc.h

	Contains:	The base utility for library header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

	******************************************************************************/
#ifndef __ULibFunc_H__
#define __ULibFunc_H__

#include "qcType.h"

#ifdef __QC_OS_WIN32__
#define qcLibHandle HMODULE
#else
#define qcLibHandle void *
#endif // __QC_OS_WIN32__

void * 		qcLibLoad (const char * pLibName, int nFlag);
void *		qcLibGetAddr (void * hLib, const char * pFuncName, int nFlag);
int			qcLibFree (void * hLib, int nFlag);

#endif // __ULibFunc_H__
