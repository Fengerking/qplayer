/*******************************************************************************
File:		CAdaptiveStreamHLS.cpp

Contains:	CAdaptiveStreamHLS implement file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-04		Qichao			Create file

*******************************************************************************/
#include "CAdaptiveStreamBA.h"
#include "qcErr.h"

#include "ULogFunc.h"
#include "qcData.h"

CAdaptiveStreamBA::CAdaptiveStreamBA(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
{
	SetObjectName("CAdaptiveStreamBA");
	m_pBitrateInfo = NULL;
	m_eWorkMode = E_ADAPTIVESTREAMING_ADAPTION_AUTO;
	m_iCurBitrateInfoIndex = 0;
	m_illCurBitrate = 0;
	m_ullLastBASegmentTimeInM3u8InMillSec = 0;
	m_ullLastSegmentTransactTimeInMillSec = 0;
	m_ullMinBAIntervalDelayBitRise = DEFAULT_MIN_BA_INTERVAL_VALUE_DELAY_BITRATE_RISE;
	m_ullBufferTimeDelayBitDrop = DEFAULT_MIN_BUFFER_TIME_DELAY_BITRATE_DROP;
	m_ullLastSegmentTimeInM3u8InMillSec = 0;
	m_ulBACount = 0;
}

CAdaptiveStreamBA::~CAdaptiveStreamBA(void)
{
	QC_DEL_A(m_pBitrateInfo);
}

int CAdaptiveStreamBA::Init()
{
	return 0;
}

int CAdaptiveStreamBA::UnInit()
{
	return 0;
}

int CAdaptiveStreamBA::SetBitrateInfo(S_ADAPTIVESTREAM_BITRATE*  pBitrateInfo, int iBitrateCount)
{
	int iIndex = 0;
	int iPos = 0;
	S_ADAPTIVESTREAM_BITRATE*   pBitrateInfoNew = NULL;

	if (pBitrateInfo == NULL)
	{
		return QC_ERR_FAILED;
	}

	QC_DEL_A(m_pBitrateInfo);
	pBitrateInfoNew = new S_ADAPTIVESTREAM_BITRATE[iBitrateCount];
	if (pBitrateInfoNew == NULL)
	{
		return QC_ERR_MEMORY;
	}

	m_iBitrateInfoCount = iBitrateCount;

	//sort
	for (iIndex = 0; iIndex < iBitrateCount; iIndex++)
	{
		iPos = iIndex;
		for (int i = 0; i < iIndex; i++)
		{
			if (pBitrateInfoNew[iIndex - 1 - i].illBitrateInManifest >= pBitrateInfo[iIndex].illBitrateInManifest)
			{
				pBitrateInfoNew[iIndex - i] = pBitrateInfoNew[iIndex - 1 - i];
				iPos = iIndex - i - 1;
			}
			else
			{
				break;
			}
		}

		pBitrateInfoNew[iPos] = pBitrateInfo[iIndex];
	}

	m_pBitrateInfo = pBitrateInfoNew;
	return QC_ERR_NONE;
}

int CAdaptiveStreamBA::GetStreamIDForNext(int& iNextStreamId, long long ullCurBufferTime, bool&  bBetter, int& iBAMode)
{
	int iRet = QC_ERR_FAILED;
	int iIndex = 0;
	int iLastIndex = m_iCurBitrateInfoIndex;
	int iNewIndex = 0;
	long long   illBitrateLimit = BA_BITRATE_LIMITATION;

	if (m_illCurBitrate == 0)
	{
		return QC_ERR_FAILED;
	}

	if (m_ulBACount == 0 && m_iBitrateInfoCount > 2)
	{
		illBitrateLimit = FIRST_BA_BITRATE_LIMITATION;
	}

	iNextStreamId = m_pBitrateInfo[m_iCurBitrateInfoIndex].ulStreamID;
	QCLOGI("In BA: cur buffer time:%lld, ullLastSegmentStartTime:%lld, ullLastSegmenetTransTime:%lld, ullLastBASegment:%lld", ullCurBufferTime, m_ullLastBASegmentTimeInM3u8InMillSec, m_ullLastSegmentTransactTimeInMillSec, m_ullLastBASegmentTimeInM3u8InMillSec);

	if (m_eWorkMode == E_ADAPTIVESTREAMING_ADAPTION_FORCE)
	{
		iNextStreamId = m_pBitrateInfo[m_iCurBitrateInfoIndex].ulStreamID;
		iBAMode = QC_BA_MODE_MANUAL;
	}
	else if (m_eWorkMode == E_ADAPTIVESTREAMING_ADAPTION_AUTO)
	{
		iBAMode = QC_BA_MODE_AUTO;
		for (iIndex = 0; iIndex < m_iBitrateInfoCount; iIndex++)
		{
			//1.3 is experience value
			if (((m_pBitrateInfo[m_iBitrateInfoCount - 1 - iIndex].illBitrateInManifest)*1.3) <= m_illCurBitrate && 
				m_pBitrateInfo[m_iBitrateInfoCount - 1 - iIndex].illBitrateInManifest < illBitrateLimit)
			{
				break;
			}
		}

		iNewIndex = m_iBitrateInfoCount - 1 - iIndex;
		if (iNewIndex < 0)
		{
			iNewIndex = 0;
		}

		if (iLastIndex != iNewIndex && m_iCurBitrateInfoIndex != -1)
		{
			iRet = CheckNeedBitrateChange(m_pBitrateInfo[iLastIndex].illBitrateInManifest, m_pBitrateInfo[iNewIndex].illBitrateInManifest, ullCurBufferTime, m_ullLastSegmentTimeInM3u8InMillSec, bBetter);
			if (iRet == QC_ERR_NONE)
			{
				m_iCurBitrateInfoIndex = iNewIndex;
				iNextStreamId = m_pBitrateInfo[iNewIndex].ulStreamID;
				m_ullLastBASegmentTimeInM3u8InMillSec = m_ullLastSegmentTimeInM3u8InMillSec;
				m_ulBACount++;
			}
		}
	}
	else
	{
		return QC_ERR_FAILED;
	}

	return iRet;
}

int CAdaptiveStreamBA::SetCurBitrate(int iStreamId, long long illCurBitrate, long long ullLastSegmentTransactTimeInMillSec, long long ullLastSegmentStartTimeInM3u8)
{
	//QCLOGI("In BA: stream id:%d, cur bitrate:%lld, last transact time:%lld, segment start time:%lld", iStreamId, illCurBitrate, ullLastSegmentTransactTimeInMillSec, ullLastSegmentStartTimeInM3u8);
	m_illCurBitrate = illCurBitrate;
	m_ullLastSegmentTransactTimeInMillSec = ullLastSegmentTransactTimeInMillSec;
	m_ullLastSegmentTimeInM3u8InMillSec = ullLastSegmentStartTimeInM3u8;

	for (int i = 0; i < m_iBitrateInfoCount; i++)
	{
		if (m_pBitrateInfo[i].ulStreamID == iStreamId)
		{
			m_pBitrateInfo[i].illBitrateActualInNetwork = illCurBitrate;
		}
	}

	//QCLOGI("set current stream id:%d, download speed:%lld", iStreamId, illCurBitrate);
	return 0;
}

int CAdaptiveStreamBA::SelectStream(unsigned int ulStreamID)
{
	int		iLastIndex = m_iCurBitrateInfoIndex;
	int		iIndex = 0;
	int		iRet = QC_ERR_NONE;
	bool	bFind = false;

	if (ulStreamID == ADAPTIVE_AUTO_STREAM_ID)
	{
		m_eWorkMode = E_ADAPTIVESTREAMING_ADAPTION_AUTO;
		return 0;
	}
	
	for (iIndex = 0; iIndex < m_iBitrateInfoCount; iIndex++)
	{
		//1.3 is experience value
		if (m_pBitrateInfo[iIndex].ulStreamID == ulStreamID)
		{
			m_eWorkMode = E_ADAPTIVESTREAMING_ADAPTION_FORCE;
			bFind = true;
			m_iCurBitrateInfoIndex = iIndex;
			break;
		}
	}

	if (bFind == false)
	{
		return QC_ERR_FAILED;
	}

	if (iLastIndex != m_iCurBitrateInfoIndex)
	{
		return QC_ERR_NONE;
	}

	return QC_ERR_FAILED;
}

int CAdaptiveStreamBA::GetParam(int nID, void * pParam)
{
	return 0;
}

int CAdaptiveStreamBA::SetParam(int nID, void * pParam)
{
	return 0;
}

void CAdaptiveStreamBA::ResetContext()
{
	m_illCurBitrate = 0;
	m_ullLastBASegmentTimeInM3u8InMillSec = 0;//UNKNOWN_BA_POSITION;
	m_ullLastSegmentTransactTimeInMillSec = 0;
	m_ullMinBAIntervalDelayBitRise = DEFAULT_MIN_BA_INTERVAL_VALUE_DELAY_BITRATE_RISE;
	m_ullBufferTimeDelayBitDrop = DEFAULT_MIN_BUFFER_TIME_DELAY_BITRATE_DROP;
	m_ullLastSegmentTimeInM3u8InMillSec = 0;
}


int CAdaptiveStreamBA::CheckNeedBitrateChange(long long ullCurBitrateInM3u8, long long ullNewBitrateInM3u8, long long ullCurBufferTime, long long ullNewSegmentStartTimeInM3u8InMillSec, bool&  bBetter)
{
	//low->high
	long long  ullTimeNeedForNewBitrate = 0;

	if (ullCurBitrateInM3u8 < ullNewBitrateInM3u8)
	{
		if ((unsigned long long)ullNewSegmentStartTimeInM3u8InMillSec < (m_ullLastBASegmentTimeInM3u8InMillSec + m_ullMinBAIntervalDelayBitRise))
		{
			QCLOGI("Last BA Segment Time:%lld, New Segment Time:%lld, delay!", m_ullLastBASegmentTimeInM3u8InMillSec, ullNewSegmentStartTimeInM3u8InMillSec);
			return QC_ERR_FAILED;
		}
		else
		{
			//1.1 is the protection factor
			ullTimeNeedForNewBitrate = (long long)(m_ullLastSegmentTransactTimeInMillSec*((double)(ullNewBitrateInM3u8 / ullCurBitrateInM3u8))*1.1);
			if (ullTimeNeedForNewBitrate > ullCurBufferTime)
			{
				QCLOGI("Time Need Next bitrate:%lld, Cur Buffer Time:%lld", ullTimeNeedForNewBitrate, ullCurBufferTime);
				return QC_ERR_FAILED;
			}
		}

		bBetter = true;
	}

	//high->low
	if (ullCurBitrateInM3u8 > ullNewBitrateInM3u8)
	{
		if ((unsigned long long)ullCurBufferTime > m_ullBufferTimeDelayBitDrop && (ullCurBitrateInM3u8*DEFAULT_BITRATE_DROP_THRESHOLD_PERCENT * 100)<(m_illCurBitrate))
		{
			QCLOGI("actual bitrate:%lld, buffer time:%lld, bitrate drop threshold:%lld, enough buffer time, delay!", m_illCurBitrate, ullCurBufferTime, m_ullBufferTimeDelayBitDrop);
			return QC_ERR_FAILED;
		}
		bBetter = false;
	}

	return QC_ERR_NONE;
}