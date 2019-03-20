/*******************************************************************************
	File:		ULibMng.cpp

	Contains:	The utility for library load file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#include "stdlib.h"
#include "ULibMng.h"

#include "ttAACDec.h"
#include "ttVideoDec.h"
#include "qcCodec.h"
#include "qcParser.h"

#ifdef __QC_OS_WIN32__
#pragma warning (disable : 4996)
#endif // __QC_OS_WIN32__


void * qcStaticLibGetAddr(void * hLib, const char * pFuncName, int nFlag)
{
//    if(!strcmp(pFuncName, qcGetGKAACDecApiName()))
//        return (void*)ttGetAACDecAPI;
//    
//    if(!strcmp(pFuncName, qcGetGKH264DecApiName()))
//        return (void*)ttGetH264DecAPI;
    if(!strcmp(pFuncName, qcGetQCFFMPEGDecCreateName()))
        return (void*)qcCreateDecoder;
    if(!strcmp(pFuncName, qcGetQCFFMPEGDecDestroyName()))
    	return (void*)qcDestroyDecoder;
    if(!strcmp(pFuncName, qcGetQCFFParserCreateName()))
        return (void*)ffCreateParser;
    if(!strcmp(pFuncName, qcGetQCFFParserDestroyName()))
        return (void*)ffDestroyParser;
    if(!strcmp(pFuncName, qcGetQCFFMPEGEncCreateName()))
        return (void*)qcCreateEncoder;
    if(!strcmp(pFuncName, qcGetQCFFMPEGEncDestroyName()))
        return (void*)qcDestroyEncoder;
    if(!strcmp(pFuncName, qcGetQCFFMPEGEncImgName()))
        return (void*)qcEncodeImage;
    if(!strcmp(pFuncName, qcGetQCColorCvtRotateName()))
        return (void*)qcColorCvtRotate;
    
    return NULL;
}

const char* qcGetGKH264DecApiName(){return "ttGetH264DecAPI";};
const char* qcGetGKAACDecApiName(){return "ttGetAACDecAPI";};
const char* qcGetQCVTBDecApiName(){return "qcGetVTBDecAPI";};
const char* qcGetQCFFMPEGDecCreateName(){return "qcCreateDecoder";};
const char* qcGetQCFFMPEGDecDestroyName(){return "qcDestroyDecoder";};
const char* qcGetQCFFParserCreateName(){return "ffCreateParser";};
const char* qcGetQCFFParserDestroyName(){return "ffDestroyParser";};
const char* qcGetQCFFMPEGEncCreateName(){return "qcCreateEncoder";};
const char* qcGetQCFFMPEGEncDestroyName(){return "qcDestroyEncoder";};
const char* qcGetQCFFMPEGEncImgName(){return "qcEncodeImage";};
const char* qcGetQCColorCvtRotateName(){return "qcColorCvtRotate";};


