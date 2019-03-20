/*******************************************************************************
File:		CCheckTimestampCache.h

Contains:	CCheckTimestampCache header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-29		Qichao			Create file

*******************************************************************************/
#ifndef __QP_CCHECKTIMESTAMPCACHE_H__
#define __QP_CCHECKTIMESTAMPCACHE_H__

#include "CFrameSpliter.h"
#include "cmbasetype.h"
#include "tsbase.h"


#define MAX_CACHE_BUF_COUNT  128
#define MAX_CACHE_SIZE 256*1024

class CheckTimestampCache
{
public:
	CheckTimestampCache(void);
	~CheckTimestampCache(void);

	void Init();
	void Uninit();
	uint32 GetPIDValue();
	void SetPIDValue(uint32  ulPIDValue);
	void SetCodecValue(uint32  ulCodecValue);
	void Reset();
	uint64 GetLastTimeStamp();
	void   SetLastTimeStamp(uint64 ullTimeStamp);
	bool InsertFrame(S_Ts_Media_Sample* pFrame);
	int  GetBufferCount();
	uint64 CalculateAvgTS(uint64 ullEndTS);
	S_Ts_Media_Sample*  GetCacheFrameArray();
	void  FlushAllData();
private:
	uint32              m_ulType;
	uint32              m_ulCodecType;
	uint32              m_ulPID;
	uint64				m_ullLastTimestamp;
	uint32				m_ulBufCount;
	S_Ts_Media_Sample        m_CheckTimestampBuf[MAX_CACHE_BUF_COUNT];

	uint8*			    m_pCache;
	uint32				m_ulCurrCachePos;
};

#endif // __CBaseParser_H__