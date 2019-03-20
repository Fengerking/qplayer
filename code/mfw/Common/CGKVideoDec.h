/*******************************************************************************
	File:		CGKVideoDec.h

	Contains:	The gk video dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CGKVideoDec_H__
#define __CGKVideoDec_H__

#include "CBaseVideoDec.h"
#include "ULibFunc.h"

#include "TTVideo.h"
#include "ttAACDec.h"

class CGKVideoDec : public CBaseVideoDec
{
public:
	CGKVideoDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CGKVideoDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);
	virtual int		PushRestOut (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	virtual int		InitNewFormat (QC_VIDEO_FORMAT * pFmt);

protected:
	qcLibHandle			m_hLib;
	TTVideoCodecAPI		m_fAPI;
	TTHandle			m_hDec;

	TTVideoFormat		m_ttFmt;

	TTBuffer			m_Input;
	TTVideoBuffer		m_Output;
	TTVideoFormat		m_OutputInfo;
};

#endif // __CGKVideoDec_H__
