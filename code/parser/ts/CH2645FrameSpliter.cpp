/*******************************************************************************
File:		CH2645FrameSpliter.h

Contains:	CH2645FrameSpliter header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-07-05		Qichao			Create file

*******************************************************************************/

#include "CH2645FrameSpliter.h"
#include "qcDef.h"
#include "qcErr.h"
#include "string.h"
#include "tsparser.h"

CH2645FrameSpliter::CH2645FrameSpliter(CBaseInst * pBaseInst)
	:CFrameSpliter(pBaseInst)
{
	SetObjectName("CH2645FrameSpliter");
	InitContext();
}

CH2645FrameSpliter::~CH2645FrameSpliter(void)
{
	UnInitContext();
}

void CH2645FrameSpliter::UnInitContext()
{
	QC_DEL_A(m_pWorkBuf);
}

void CH2645FrameSpliter::InitContext()
{
	m_pWorkBuf = new unsigned char[DEFAULT_MAX_FRAME_SIZE];
	m_iBufMaxSize = DEFAULT_MAX_FRAME_SIZE;
	m_iCurBufSize = 0;
	m_iDataOffset = 0;
	m_ullLastTimeStamp = 0;
	m_iCurFrameCount = 0;
}


int CH2645FrameSpliter::CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
{
	uint8* pSync = NULL;
	int    iFindSize = 0;
	uint8* pFind = NULL;
	uint8* pNew = NULL;

	if (m_iDataOffset != 0)
	{
		memmove(m_pWorkBuf, m_pWorkBuf + m_iDataOffset, m_iCurBufSize);
		m_iDataOffset = 0;
	}

	if (m_iBufMaxSize < (iInputMediaSize + m_iCurBufSize))
	{
		pNew = new unsigned char[iInputMediaSize + m_iCurBufSize + 128];
		if (pNew == NULL)
		{
			return QC_ERR_MEMORY;
		}

		memcpy(pNew, m_pWorkBuf, m_iCurBufSize);
		QC_DEL_A(m_pWorkBuf);
		m_pWorkBuf = pNew;
		m_iBufMaxSize = iInputMediaSize + m_iCurBufSize + 128;
	}


	memcpy(m_pWorkBuf + m_iCurBufSize, pInputMediaData, iInputMediaSize);
	pFind = m_pWorkBuf + m_iCurBufSize;
	iFindSize = iInputMediaSize;
	m_iCurBufSize += iInputMediaSize;

	if (m_iCurFrameCount == 0)
	{
		m_ullLastTimeStamp = ullInputMediaTimeStamp;
	}

	if (m_iCurFrameCount > 0 && iInputMediaSize > 0)
	{
		pSync = FindSync(pFind, iFindSize);

		if (pSync == NULL)
		{
			iFrameCount = 0;
		}
		else
		{
			pFrameArray[0].pSampleBuffer = m_pWorkBuf;
			pFrameArray[0].ulSampleBufferSize = pSync - m_pWorkBuf;
			pFrameArray[0].ullTimeStamp = m_ullLastTimeStamp;
			pFrameArray[0].ulSampleFlag = IsKeyFrame(pFrameArray[0].pSampleBuffer, pFrameArray[0].ulSampleBufferSize);
			m_iDataOffset = pSync - m_pWorkBuf;
			m_ullLastTimeStamp = ullInputMediaTimeStamp;
			m_iCurFrameCount--;
			iFrameCount = 1;
			m_iCurBufSize -= pFrameArray[0].ulSampleBufferSize;
		}
	}

	m_iCurFrameCount++;
	return 0;
}

int    CH2645FrameSpliter::FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
{
	//memmove(m_pWorkBuf, m_pWorkBuf + m_iDataOffset, m_iCurBufSize);
	//Issue the left data
	iFrameCount = 0;

	if (m_iCurBufSize > 0)
	{
		pFrameArray[iFrameCount].pSampleBuffer = m_pWorkBuf + m_iDataOffset;
		pFrameArray[iFrameCount].ulSampleBufferSize = m_iCurBufSize;
		pFrameArray[iFrameCount].ullTimeStamp = m_ullLastTimeStamp;
		iFrameCount++;
	}

	m_ullLastTimeStamp = 0;
	m_iCurBufSize = 0;
	m_iDataOffset = 0;
	m_iCurFrameCount = 0;
	return 0;
}

uint8*   CH2645FrameSpliter::FindSync(uint8*  pStart, int iSize)
{
	uint8*    pRet = NULL;
	uint8 nalhead[3] = { 0, 0, 1 };
	int ioffset = 0;
	uint8 * pScan = pStart;
	uint8 * pScanEnd = pStart + iSize - 4;
	while (pScan < pScanEnd)
	{
		if (memcmp(pScan, nalhead, 3) == 0)
		{
			if (pScan > pStart && (*(pScan - 1)) == 0)
			{
				pRet = pScan - 1;
			}
			else
			{
				pRet = pScan;
			}
			break;
		}
		else
		{
			pScan++;
		}
	}

	return pRet;
}

int     CH2645FrameSpliter::IsKeyFrame(uint8*  pStart, int iSize)
{
	int iRet = 0;

	switch (m_ulMediaCodecId)
	{
		case STREAM_TYPE_VIDEO_H264:
		{
			iRet = IsIFrameForH264(pStart, iSize);
			break;
		}
		case STREAM_TYPE_VIDEO_HEVC:
		{
			iRet = IsIFrameForHEVC(pStart, iSize);
			break;
		}
	}

	return iRet;
}