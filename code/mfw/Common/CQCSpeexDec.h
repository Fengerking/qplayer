/*******************************************************************************
	File:		CQCSpeexDec.h

	Contains:	The qc speex dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-09		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCSpeexDec_H__
#define __CQCSpeexDec_H__
#include "qcCodec.h"
#include "CQCAudioDec.h"

class CQCSpeexDec : public CQCAudioDec
{
public:
	CQCSpeexDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCSpeexDec(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt);

};

#endif // __CQCSpeexDec_H__
