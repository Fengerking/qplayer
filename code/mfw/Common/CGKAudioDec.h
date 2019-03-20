/*******************************************************************************
	File:		CGKAudioDec.h

	Contains:	The gk audio dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CGKAudioDec_H__
#define __CGKAudioDec_H__

#include "CBaseAudioDec.h"
#include "ULibFunc.h"

#include "TTAudio.h"
#include "ttAACDec.h"

class CGKAudioDec : public CBaseAudioDec
{
public:
	CGKAudioDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CGKAudioDec(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	qcLibHandle			m_hLib;
	TTAudioCodecAPI		m_fAPI;
	TTHandle			m_hDec;

	TTAudioFormat		m_ttFmt;

	TTBuffer			m_Input;
	TTBuffer			m_Output;
	TTAudioFormat		m_OutputInfo;
};

#endif // __CGKAudioDec_H__
