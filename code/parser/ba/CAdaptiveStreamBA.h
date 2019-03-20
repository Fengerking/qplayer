/*******************************************************************************
File:		CAdaptiveStreamBA.h

Contains:	CAdaptiveStreamBA Header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-10		Qichao			Create file

*******************************************************************************/

#ifndef __ADAPTIVE_STREAM_BA_H__
#define __ADAPTIVE_STREAM_BA_H__

#include "AdaptiveStreamParser.h"
#include "CBaseObject.h"

#define DEFAULT_MIN_BA_INTERVAL_VALUE_DELAY_BITRATE_RISE     20000
#define DEFAULT_MIN_BUFFER_TIME_DELAY_BITRATE_DROP           18000
#define UNKNOWN_BA_POSITION                                  0xffffffff
#define DEFAULT_BITRATE_DROP_THRESHOLD_PERCENT               80
#define FIRST_BA_BITRATE_LIMITATION                          3000000
#define BA_BITRATE_LIMITATION                                10000000

class CAdaptiveStreamBA : public CBaseObject
{
public:
	CAdaptiveStreamBA(CBaseInst * pBaseInst);
	virtual ~CAdaptiveStreamBA(void);

	virtual int			Init ();
	virtual int 		UnInit ();
	virtual int         SetBitrateInfo(S_ADAPTIVESTREAM_BITRATE*  pBitrateInfo, int iBitrateCount);
	virtual int         GetStreamIDForNext(int& iNextStreamId, long long ullCurBufferTime, bool&  bBetter, int& iBAMode);
	virtual int         SetCurBitrate(int iStreamId, long long illCurBitrate, long long ullLastSegmentTransactTimeInMillSec, long long ullSegmentStartTimeInM3u8InMillSec);
	virtual int         SelectStream(unsigned int ulStreamID);
	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

	void                ResetContext();

private:
	int                 CheckNeedBitrateChange(long long ullCurBitrateInM3u8, long long ullNewBitrateInM3u8, long long ullCurBufferTime, long long ullNewSegmentStartTimeInM3u8InMillSec, bool&  bBetter);

private:
	S_ADAPTIVESTREAM_BITRATE*			m_pBitrateInfo;
	int									m_iBitrateInfoCount;
	E_ADAPTIVESTREAMING_ADAPTION_MODE   m_eWorkMode;
	int                                 m_iCurBitrateInfoIndex;
	long long                           m_illCurBitrate;
	unsigned long long                  m_ullLastBASegmentTimeInM3u8InMillSec;
	unsigned long long                  m_ullLastSegmentTimeInM3u8InMillSec;
	unsigned long long                  m_ullLastSegmentTransactTimeInMillSec;
	unsigned long long                  m_ullMinBAIntervalDelayBitRise;
	unsigned long long                  m_ullBufferTimeDelayBitDrop;
	unsigned int                        m_ulBACount;
};

#endif