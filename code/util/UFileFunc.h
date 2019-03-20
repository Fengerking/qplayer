/*******************************************************************************
	File:		UFileFunc.h

	Contains:	The base utility for file operation header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __UFileFunc_H__
#define __UFileFunc_H__
#ifdef __QC_OS_WIN32__
#include <windows.h>
#endif // __QC_OS_WIN32__

#include "qcType.h"

#ifdef __QC_OS_WIN32__
#define qcFile		HANDLE
#elif defined __QC_OS_NDK__ || defined __QC_OS_IOS__ || defined __QC_OS_MACOS__
#define	qcFile		int
#endif // __QC_OS_WIN32__

#define	QCFILE_READ		1
#define	QCFILE_WRITE	2

#define	QCFILE_BEGIN	1
#define	QCFILE_CUR		2
#define	QCFILE_END		3

qcFile		qcFileOpen (char * pFile, int nFlag);
int			qcFileRead (qcFile hFile, unsigned char * pBuff, int nSize);
int			qcFileWrite (qcFile hFile, unsigned char * pBuff, int nSize);
long long	qcFileSeek (qcFile hFile, long long llPos, int nFlag);
long long	qcFileSize (qcFile hFile);
int			qcFileClose (qcFile hFile);

#endif // __UFileFunc_H__
