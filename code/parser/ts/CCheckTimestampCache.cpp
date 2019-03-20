/*******************************************************************************
File:		CCheckTimestampCache.cpp

Contains:	CCheckTimestampCache implement file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-29		Qichao			Create file

*******************************************************************************/

#include "CCheckTimestampCache.h"
#include "stdio.h"
#include "string.h"

CheckTimestampCache::CheckTimestampCache()
{
	m_pCache = NULL;
	Init();
}

CheckTimestampCache::~CheckTimestampCache(void)
{ 
	Uninit();
}

void CheckTimestampCache::Init()
{
	Reset();
	m_pCache = new uint8[MAX_CACHE_SIZE];
}

void CheckTimestampCache::Uninit()
{
	if (m_pCache)
	{
		delete m_pCache;
		m_pCache = NULL;
	}

	Reset();
}

void CheckTimestampCache::SetPIDValue(uint32  ulPIDValue)
{
	m_ulPID = ulPIDValue;
}

uint32 CheckTimestampCache::GetPIDValue()
{
	return m_ulPID;
}

void CheckTimestampCache::SetCodecValue(uint32  ulCodecValue)
{
	m_ulCodecType = ulCodecValue;
}

void CheckTimestampCache::Reset()
{
	m_ullLastTimestamp = 0xFFFFFFFFFFFFFFFFLL;
	m_ulBufCount = 0;
	m_ulCurrCachePos = 0;
}

void CheckTimestampCache::FlushAllData()
{

}

uint64 CheckTimestampCache::GetLastTimeStamp()
{
	return m_ullLastTimestamp;
}

bool CheckTimestampCache::InsertFrame(S_Ts_Media_Sample* pFrame)
{
	if (m_ulBufCount + 1 >= MAX_CACHE_BUF_COUNT)
		return false;

	m_CheckTimestampBuf[m_ulBufCount].ulMediaCodecId = pFrame->ulMediaCodecId;
	m_CheckTimestampBuf[m_ulBufCount].ullTimeStamp = pFrame->ullTimeStamp;
	m_CheckTimestampBuf[m_ulBufCount].ulSampleBufferSize = pFrame->ulSampleBufferSize;
	m_CheckTimestampBuf[m_ulBufCount].ulSampleFlag = pFrame->ulSampleFlag;
	m_CheckTimestampBuf[m_ulBufCount].usMediaType = pFrame->usMediaType;
	m_CheckTimestampBuf[m_ulBufCount].usTrackId = pFrame->usTrackId;


	if ((m_ulCurrCachePos + pFrame->ulSampleBufferSize) <= MAX_CACHE_SIZE)
	{
		memcpy(m_pCache + m_ulCurrCachePos, pFrame->pSampleBuffer, pFrame->ulSampleBufferSize);
		m_CheckTimestampBuf[m_ulBufCount].pSampleBuffer = m_pCache + m_ulCurrCachePos;
		m_ulCurrCachePos += pFrame->ulSampleBufferSize;
	}
	else
	{
		return false;
	}

	m_ullLastTimestamp = pFrame->ullTimeStamp;
	m_ulBufCount++;
	return true;
}

uint64 CheckTimestampCache::CalculateAvgTS(uint64 ullEndTS)
{
	bool   bBigThanLastTime = true;
	uint64 avg = 0;
	if (ullEndTS > m_ullLastTimestamp)
	{
		avg = (ullEndTS - m_ullLastTimestamp) / (m_ulBufCount + 1);
		bBigThanLastTime = true;
	}
	else
	{
		avg = (m_ullLastTimestamp - ullEndTS) / (m_ulBufCount + 1);
		bBigThanLastTime = false;
	}

	for (uint32 n = 0; n < m_ulBufCount; n++)
	{
		if (bBigThanLastTime == true)
		{
			m_CheckTimestampBuf[n].ullTimeStamp = m_ullLastTimestamp + avg*(n + 1);
		}
		else
		{
			m_CheckTimestampBuf[n].ullTimeStamp = m_ullLastTimestamp - avg*(n + 1);
		}
	}

	return avg;
}

int  CheckTimestampCache::GetBufferCount()
{
	return m_ulBufCount;
}

void  CheckTimestampCache::SetLastTimeStamp(uint64 ullTimeStamp)
{
	m_ullLastTimestamp = ullTimeStamp;
}

S_Ts_Media_Sample*    CheckTimestampCache::GetCacheFrameArray()
{
	return m_CheckTimestampBuf;
}