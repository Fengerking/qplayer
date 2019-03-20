/*******************************************************************************
File:		CH2645FrameSpliter.h

Contains:	CH2645FrameSpliter header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-07-05		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CADH2645FRAMESPLITER_H__
#define __QP_CADH2645FRAMESPLITER_H__

#define DEFAULT_MAX_FRAME_SIZE  (512*1024)

#include "CFrameSpliter.h"

class CH2645FrameSpliter:public CFrameSpliter
{
public:
	CH2645FrameSpliter(CBaseInst * pBaseInst);
	~CH2645FrameSpliter(void);

	virtual void UnInitContext();
	virtual void InitContext();
	virtual int CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);
	virtual int FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount);

protected:
	uint8*   FindSync(uint8*  pStart, int iSize);
	int      IsKeyFrame(uint8*  pStart, int iSize);


private:
	uint8*    m_pWorkBuf;
	int       m_iCurBufSize;
	int       m_iDataOffset;
	int       m_iBufMaxSize;
	uint64    m_ullLastTimeStamp;
	int       m_iCurFrameCount;
};

#endif // __CBaseParser_H__