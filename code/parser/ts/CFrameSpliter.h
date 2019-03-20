/*******************************************************************************
File:		CFrameSpliter.h

Contains:	CFrameSpliter header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-27		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CFRAMESPLITER_H__
#define __QP_CFRAMESPLITER_H__

#include "CBaseObject.h"
#include "cmbasetype.h"
#include "tsbase.h"
#include "stdio.h"


class CFrameSpliter : public CBaseObject
{
public:
	CFrameSpliter(CBaseInst * pBaseInst);
	~CFrameSpliter(void);

	void SetCodecID(int iMediaCodecID);
	void SetPIDValue(uint32   ulPIDValue);
	uint32 GetPIDValue();
	virtual void UnInitContext();
	virtual void InitContext();
	virtual int CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);
	virtual int FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);

protected:
	uint32 m_ulMediaCodecId;
	uint32 m_ulPIDValue;
};

#endif // __CBaseParser_H__