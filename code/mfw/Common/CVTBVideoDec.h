/*******************************************************************************
	File:		CVTBVideoDec.h

	Contains:	The video tool box dec wrap header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2017-03-01		Jun Lin			Create file

*******************************************************************************/
#ifndef __CVTBVideoDec_H__
#define __CVTBVideoDec_H__

#include "CBaseVideoDec.h"
#include "qcData.h"

typedef struct
{
    unsigned int (* Init) (void** phDec, int nWidth, int nHeight);
    unsigned int (* SetInputData) (void* hDec, QC_DATA_BUFF * pInput);
    unsigned int (* GetOutputData) (void* hDec, QC_DATA_BUFF* pOutput, QC_VIDEO_BUFF* pInfo);
    unsigned int (* SetParam) (void* hDec, int uParamID, void* pData);
    unsigned int (* GetParam) (void* hDec, int uParamID, void* pData);
    unsigned int (* Uninit) (void* hDec);
    unsigned int (* Flush) (void* hDec);
    unsigned int (* ForceOutputAll) (void* hDec);
    unsigned int (* SetHeadData) (void* hDec, unsigned char* pBuff, int nBufferSize);
} VTB_DECAPI;


class CVTBVideoDec : public CBaseVideoDec
{
public:
	CVTBVideoDec(CBaseInst* pInst, void * hInst);
	virtual ~CVTBVideoDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);
    
    virtual int		Start (void);
    virtual int		Stop (void);
    virtual int     PushRestOut (void);
    virtual int		RecvEvent(int nEventID);

private:
    int		InitNewFormat (QC_VIDEO_FORMAT* pFmt, QC_DATA_BUFF * pBuff);
    int     UpdateHeadData (QC_DATA_BUFF* pBuff);

protected:
    void*           m_hDec;
    VTB_DECAPI      m_api;
    bool            m_bInitHeadData;
    bool            m_bStop;
};

#endif // __CVTBVideoDec_H__
