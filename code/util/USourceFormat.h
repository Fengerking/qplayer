/*******************************************************************************
	File:		USourceFormat.h

	Contains:	The base utility for file operation header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-22		Bangfei			Create file

*******************************************************************************/
#ifndef __USourceFormat_H__
#define __USourceFormat_H__

#include "qcData.h"
#include "qcIO.h"

QCIOProtocol	qcGetSourceProtocol (const char * pSource);
QCParserFormat	qcGetSourceFormat (const char * pSource);
QCParserFormat	qcGetSourceFormat(const char * pSource, QC_IO_Func * pIO);

#endif // __USourceFormat_H__
