/*******************************************************************************
File:		CADTSFrameSpliter.h

Contains:	CADTSFrameSpliter header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-27		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CADTSFRAMESPLITER_H__
#define __QP_CADTSFRAMESPLITER_H__

#include "CFrameSpliter.h"

typedef  enum{
	E_STATE_WAIT_SYNC_WORD,
	E_STATE_WAIT_SYNC_FRAME_SIZE_INFO,
	E_STATE_WAIT_SYNC_FRAME_DATA,
	E_STATE_UNKNOWN,
}E_ADTS_SPLIT_STATE;

#define ADTS_HEADER_BASE_SIZE 7

class CADTSFrameSpliter:public CFrameSpliter
{
public:
	CADTSFrameSpliter(CBaseInst * pBaseInst);
	~CADTSFrameSpliter(void);

	virtual void UnInitContext();
	virtual void InitContext();
	virtual int CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);
	virtual int FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);

protected:
	uint8*   FindADTSSync(uint8*  pStart, int iSize);

private:
	E_ADTS_SPLIT_STATE    m_eSplitState;

	uint8*    m_pWorkBuf;
	int       m_iCurBufSize;
	int       m_iDataOffset;
	int       m_iBufMaxSize;
	uint64    m_ullLastTimeStamp;
};

#endif // __CBaseParser_H__