/*******************************************************************************
	File:		CQCVideoDec.h

	Contains:	The qc video dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCVideoDec_H__
#define __CQCVideoDec_H__
#include "qcCodec.h"
#include "CBaseVideoDec.h"
#include "ULibFunc.h"

class CQCVideoDec : public CBaseVideoDec
{
public:
	CQCVideoDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCVideoDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);
	virtual int		PushRestOut (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	virtual	int		InitNewFormat (QC_VIDEO_FORMAT * pFmt);

protected:
	qcLibHandle			m_hLib;
	QC_Codec_Func		m_fCodec;

	bool				m_bFirstFrame;
};

#endif // __CQCVideoDec_H__
