/*******************************************************************************
 File:        qcMuxer.h
 
 Contains:    Muxer interface define header file.
 
 Written by:    Jun Lin
 
 Change History (most recent first):
 2018-01-06        Jun            Create file
 
 *******************************************************************************/
#ifndef __qcMuxer_h__
#define __qcMuxer_h__
#include "qcErr.h"
#include "qcDef.h"
#include "qcType.h"
#include "qcData.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * the qc muxer interface
 */
typedef struct
{
    // Define the version of the IO. It shuild be 1
    int                nVer;
    
    // Define the base instance
    void *            pBaseInst;

    // The muxer handle, it will fill in function.
    void *            hMuxer;
    
    // Func: Open the Muxer with file path.
    int             (* Open)        (void * hMuxer, const char * pURL);
    int             (* Close)       (void * hMuxer);
	int				(* Init)        (void * hMuxer, void * pVideoFmt, void * pAudioFmt);
    int             (* Write)       (void * hMuxer, QC_DATA_BUFF* pBuffer);
    
    // for extend function later.
    int             (* GetParam)    (void * hMuxer, int nID, void * pParam);
    int             (* SetParam)    (void * hMuxer, int nID, void * pParam);
} QC_Muxer_Func;

// create the Parser with Parser type.
//DLLEXPORT_C int        qcCreateMuxer (QC_Muxer_Func * pMuxer, QCParserFormat nFormat);
//
//// destory the Parser
//DLLEXPORT_C int        qcDestroyMuxer (QC_Muxer_Func * pMuxer);

DLLEXPORT_C int        ffCreateMuxer(QC_Muxer_Func * pMuxer, QCParserFormat nFormat);
typedef int (*FFCREATEMUXER) (QC_Muxer_Func * pMuxer, QCParserFormat nFormat);

DLLEXPORT_C int        ffDestroyMuxer(QC_Muxer_Func * pMuxer);
typedef int (*FFDESTROYMUXER) (QC_Muxer_Func * pMuxer);
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // __qcMuxer_h__

