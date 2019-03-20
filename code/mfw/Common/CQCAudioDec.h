/*******************************************************************************
	File:		CQCAudioDec.h

	Contains:	The qc audio dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-02		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCAudioDec_H__
#define __CQCAudioDec_H__
#include "qcCodec.h"
#include "CBaseAudioDec.h"
#include "ULibFunc.h"

class CQCAudioDec : public CBaseAudioDec
{
public:
	CQCAudioDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCAudioDec(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);
	virtual int		PushRestOut (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

	virtual int		SetVolume(int nVolume);

protected:
	virtual	int		InitNewFormat (QC_AUDIO_FORMAT * pFmt);

	virtual int		ConvertData (void);

protected:
	qcLibHandle			m_hLib;
	QC_Codec_Func		m_fCodec;
	int					m_nScale;

	unsigned char *		m_pPCMData;
	int					m_nPCMSize;
	QC_AUDIO_FRAME *	m_pFrmInfo;
	int					m_nChannels;
    long long			m_llFirstFrameTime;

};

#endif // __CQCAudioDec_H__
