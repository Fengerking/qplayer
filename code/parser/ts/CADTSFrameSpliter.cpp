/*******************************************************************************
File:		CADTSFrameSpliter.cpp

Contains:	CADTSFrameSpliter implement file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-27		Qichao			Create file

*******************************************************************************/

#include "CADTSFrameSpliter.h"
#include "qcDef.h"
#include "qcErr.h"
#include "string.h"

CADTSFrameSpliter::CADTSFrameSpliter(CBaseInst * pBaseInst)
	:CFrameSpliter(pBaseInst)
{
	SetObjectName("CADTSFrameSpliter");
	InitContext();
}

CADTSFrameSpliter::~CADTSFrameSpliter(void)
{
	UnInitContext();
}

void CADTSFrameSpliter::UnInitContext()
{
	QC_DEL_A(m_pWorkBuf);
}

void CADTSFrameSpliter::InitContext()
{
	m_eSplitState = E_STATE_WAIT_SYNC_WORD;
	m_pWorkBuf = new uint8[8*1024];
	m_iBufMaxSize = 8*1024;
	m_iCurBufSize = 0;
	m_iDataOffset = 0;
}


int CADTSFrameSpliter::CommitInputAndSplitting(uint8* pInputMediaData, int iInputMediaSize, uint64 ullInputMediaTimeStamp, int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
{
	uint8*    pNew = NULL;
	uint8*    pCur = NULL;
	uint8*    pEnd = NULL;
	uint8*    pSync = NULL;
	int       iFrameSize = 0;
	int       iLastLeftSize = m_iCurBufSize;

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
	m_iCurBufSize += iInputMediaSize;

	pCur = m_pWorkBuf;
	pEnd = m_pWorkBuf + m_iCurBufSize;
	iFrameCount = 0;

	while (pCur < pEnd)
	{
		pSync = FindADTSSync(pCur, pEnd - pCur);
		if (pSync != NULL)
		{
			if ((pEnd - pSync) > ADTS_HEADER_BASE_SIZE)
			{
				iFrameSize = ((pSync[3] & 0x03) << 11) | (pSync[4] << 3) | (pSync[5] >> 5);
				if (iFrameSize <= (pEnd - pSync))
				{
					pFrameArray[iFrameCount].pSampleBuffer = pSync;
					pFrameArray[iFrameCount].ulSampleBufferSize = iFrameSize;
					pFrameArray[iFrameCount].ullTimeStamp = ((pSync - m_pWorkBuf) < iLastLeftSize) ? m_ullLastTimeStamp : ullInputMediaTimeStamp;
					pCur = pSync + iFrameSize;
					m_ullLastTimeStamp = ullInputMediaTimeStamp;
					m_iCurBufSize = pEnd - pCur;
					m_iDataOffset = pCur - m_pWorkBuf;
					iFrameCount++;
					continue;
				}
			}

			m_ullLastTimeStamp = ullInputMediaTimeStamp;
			m_iCurBufSize = pEnd - pSync;
			m_iDataOffset = pSync - m_pWorkBuf;
			break;
		}
		else
		{
			//Can find the FF F0, left one byte for next FF F0
			m_iCurBufSize = 1;
			m_iDataOffset = pEnd - m_pWorkBuf - 1;
			break;
		}
	}

	return 0;
}

int    CADTSFrameSpliter::FlushAllData(int  iArraySize, S_Ts_Media_Sample*   pFrameArray, int& iFrameCount)
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

	m_iCurBufSize = 0;
	m_iDataOffset = 0;
	m_ullLastTimeStamp = 0;
	m_eSplitState = E_STATE_WAIT_SYNC_WORD;
	return 0;
}


uint8*   CADTSFrameSpliter::FindADTSSync(uint8*  pStart, int iSize)
{
	uint8*   pCur = pStart;
	uint8*   pEnd = pStart+iSize-1;
	uint8*   pRet = NULL;

	while (pCur != NULL  && pCur < pEnd)
	{
		if ((*pCur) == 0xFF && ((*(pCur + 1)) & 0xF0) == 0xF0)
		{
			pRet = pCur;
			break;
		}
		else
		{
			pCur++;
		}
	}
	
	return pRet;
}