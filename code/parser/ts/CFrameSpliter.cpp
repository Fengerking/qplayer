/*******************************************************************************
File:		CFrameSpliter.cpp

Contains:	CFrameSpliter implement file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-27		Qichao			Create file

*******************************************************************************/

#include "CFrameSpliter.h"
#include "tsbase.h"

CFrameSpliter::CFrameSpliter(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
{
	SetObjectName("CFrameSpliter");
}

CFrameSpliter::~CFrameSpliter(void)
{
	m_ulMediaCodecId = 0;
}

void CFrameSpliter::SetCodecID(int iMediaCodecID)
{
	m_ulMediaCodecId = iMediaCodecID;
	return ;
}

void CFrameSpliter::UnInitContext()
{

}

void CFrameSpliter::InitContext()
{

}

int CFrameSpliter::CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
{
	return 0;
}

void CFrameSpliter::SetPIDValue(uint32   ulPIDValue)
{
	m_ulPIDValue = ulPIDValue;
}

uint32 CFrameSpliter::GetPIDValue()
{
	return m_ulPIDValue;
}

int CFrameSpliter::FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
{
	return 0;
}
