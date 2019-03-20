/*******************************************************************************
	File:		CQCAdpcmDec.h

	Contains:	The qc audio dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-02		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCAdpcmDec_H__
#define __CQCAdpcmDec_H__
#include "qcCodec.h"
#include "CBaseAudioDec.h"
#include "ULibFunc.h"

class CQCAdpcmDec : public CBaseAudioDec
{
public:
	CQCAdpcmDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCAdpcmDec(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	short			g711Decode(unsigned char cData);

protected:
	QC_DATA_BUFF 		m_buffData;
	bool				m_bNewFormat;
};

#endif // __CQCAdpcmDec_H__
