/*******************************************************************************
	File:		CQCSpeexDec.cpp

	Contains:	The qc audio dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-09		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCSpeexDec.h"
#include "ULogFunc.h"

CQCSpeexDec::CQCSpeexDec(CBaseInst * pBaseInst, void * hInst)
	: CQCAudioDec(pBaseInst, hInst)
{
	SetObjectName ("CQCSpeexDec");
}

CQCSpeexDec::~CQCSpeexDec(void)
{
	Uninit ();
}

int CQCSpeexDec::Init (QC_AUDIO_FORMAT * pFmt)
{
	int nRC = CQCAudioDec::Init(pFmt);
	if (nRC != QC_ERR_NONE)
		return nRC;

	m_fmtAudio.nChannels = pFmt->nChannels;
	m_fmtAudio.nSampleRate = pFmt->nSampleRate;

	return QC_ERR_NONE;
}
