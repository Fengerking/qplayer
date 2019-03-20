/*******************************************************************************
	File:		ULibMng.h

	Contains:	The base utility for library header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

	******************************************************************************/
#ifndef __ULibMng_H__
#define __ULibMng_H__

void* qcStaticLibGetAddr(void * hLib, const char * pFuncName, int nFlag);

const char* qcGetGKH264DecApiName();
const char* qcGetGKAACDecApiName();
const char* qcGetQCVTBDecApiName();
const char* qcGetQCFFMPEGDecCreateName();
const char* qcGetQCFFMPEGDecDestroyName();
const char* qcGetQCFFParserCreateName();
const char* qcGetQCFFParserDestroyName();
const char* qcGetQCFFMPEGEncCreateName();
const char* qcGetQCFFMPEGEncDestroyName();
const char* qcGetQCFFMPEGEncImgName();
const char* qcGetQCColorCvtRotateName();


#endif // __ULibMng_H__
