/*******************************************************************************
	File:		CMP4Parser.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-04		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "math.h"

#include "CMP4Parser.h"

#include "CHTTPClient.h"

#include "UIntReader.h"
#include "ULogFunc.h"
#include "UMsgMng.h"

int	CMP4Parser::MoovWorkProc(void * pParam)
{
	CMP4Parser * pParser = (CMP4Parser *)pParam;

	int nRC = QC_ERR_NONE;
	bool bFinish = false;

	qcSleepEx(1000000, &pParser->m_pBaseInst->m_bForceClose);

	QCMP4TrackInfo * pAudioTrackInfo = NULL;

	while (!bFinish)
	{
		bFinish = true;
		if (pParser->m_pVideoTrackInfo != NULL)
		{
			nRC = pParser->ReadMoovData(pParser->m_pVideoTrackInfo);
			if (nRC != QC_ERR_NONE)
				bFinish = false;
		}

#if 1
		NODEPOS pos = pParser->m_lstAudioTrackInfo.GetHeadPosition();
		while (pos != NULL)
		{
			pAudioTrackInfo = pParser->m_lstAudioTrackInfo.GetNext(pos);
			nRC = pParser->ReadMoovData(pAudioTrackInfo);
			if (nRC != QC_ERR_NONE)
				bFinish = false;
		}
#else
		if (pParser->m_pAudioTrackInfo != NULL)
		{		
			nRC = pParser->ReadMoovData(pParser->m_pAudioTrackInfo);
			if (nRC != QC_ERR_NONE)
				bFinish = false;
		}
#endif // 0		
		if (bFinish)
			break;

		qcSleep(5000);
		if (pParser->m_pBaseInst->m_bForceClose)
			break;
	}

    qcThreadClose(pParser->m_hMoovThread, 0);
	pParser->m_hMoovThread = NULL;
	return QC_ERR_NONE;
}

int	CMP4Parser::ReadMoovData(QCMP4TrackInfo * pTrackInfo)
{
	if (pTrackInfo == NULL)
		return QC_ERR_NONE;

	CHTTPClient *	pHttp = new CHTTPClient(m_pBaseInst, NULL);
	pHttp->SetNotifyMsg(false);

	char *			pBuff = NULL;
	int				nRead = 0;
	unsigned char *	pData = NULL;
	int				nRC = 0;
	bool			bFinish = true;
	int				i = 0;

	long long		llStartPos = pTrackInfo->lSTCOStartPos;
	int				nBuffSize = pTrackInfo->nSTCOBuffSize;
	int				nEntryNum = pTrackInfo->iChunkOffsetEntryNum;
	int				nEntryBeg = 0;
	bool			bNeedBuild = false;

	// Read rest stco data
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = nRead & (~0X03);

		long long* pChunkOffsetTab = pTrackInfo->iChunkOffsetTab;
		nEntryBeg = (nEntryNum * 4 - nBuffSize) / 4;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 4 >= nRead)
				break;
			pChunkOffsetTab[i] = qcReadUint32BE(pData);
			pData += 4;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lSTCOStartPos += nRead;
			pTrackInfo->nSTCOBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lSTCOStartPos = 0;
			pTrackInfo->nSTCOBuffSize = 0;
		}
		delete []pBuff;
	}

	// Read rest co64 data
	llStartPos = pTrackInfo->lCO64StartPos;
	nBuffSize = pTrackInfo->nCO64BuffSize;
	nEntryNum = pTrackInfo->iChunkOffsetEntryNum;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = nRead & (~0X07);

		long long* pChunkOffsetTab = pTrackInfo->iChunkOffsetTab;
		nEntryBeg = (nEntryNum * 8 - nBuffSize) / 8;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 8 >= nRead)
				break;
			pChunkOffsetTab[i] = qcReadUint32BE(pData);
			pData += 4;
			pChunkOffsetTab[i] = (pChunkOffsetTab[i] << 32) + qcReadUint32BE(pData);
			pData += 4;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lCO64StartPos += nRead;
			pTrackInfo->nCO64BuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lCO64StartPos = 0;
			pTrackInfo->nCO64BuffSize = 0;
		}
		delete []pBuff;
	}

	// Read rest stsz data
	llStartPos = pTrackInfo->lSTSZStartPos;
	nBuffSize = pTrackInfo->nSTSZBuffSize;
	nEntryNum = pTrackInfo->iSampleCount;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		int nMaxFrameSize = pTrackInfo->iMaxFrameSize;
		long long llTotalSize = pTrackInfo->iTotalSize;

		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = nRead & (~0X03);

		int * pSampleSizeTab = pTrackInfo->iVariableSampleSizeTab;
		nEntryBeg = (nEntryNum * 4 - nBuffSize) / 4;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 4 >= nRead)
				break;
			pSampleSizeTab[i] = qcReadUint32BE(pData);
			pData += 4;
			bNeedBuild = true;

			if (pSampleSizeTab[i] > nMaxFrameSize)
				nMaxFrameSize = pSampleSizeTab[i];
			llTotalSize += pSampleSizeTab[i];
		}
		pTrackInfo->iMaxFrameSize = nMaxFrameSize;
		pTrackInfo->iTotalSize = llTotalSize;

		if (nRead < nBuffSize)
		{
			pTrackInfo->lSTSZStartPos += nRead;
			pTrackInfo->nSTSZBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lSTSZStartPos = 0;
			pTrackInfo->nSTSZBuffSize = 0;
		}
		delete[]pBuff;
	}

	// Read rest stsc data
	llStartPos = pTrackInfo->lSTSCStartPos;
	nBuffSize = pTrackInfo->nSTSCBuffSize;
	nEntryNum = pTrackInfo->iChunkToSampleEntryNum;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = (nRead / 12) * 12;

		TTChunkToSample * pChunk2SampleTab = pTrackInfo->iChunkToSampleTab;
		nEntryBeg = (nEntryNum * 12 - nBuffSize) / 12;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 12 >= nRead)
				break;
			pChunk2SampleTab[i].iFirstChunk = qcReadUint32BE(pData);
			pData += 4;
			pChunk2SampleTab[i].iSampleNum = qcReadUint32BE(pData);
			pData += 8;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lSTSCStartPos += nRead;
			pTrackInfo->nSTSCBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lSTSCStartPos = 0;
			pTrackInfo->nSTSCBuffSize = 0;
			pTrackInfo->iChunkToSampleTab[i].iFirstChunk = pTrackInfo->iChunkToSampleTab[i - 1].iFirstChunk + 1;
			pTrackInfo->iChunkToSampleTab[i].iSampleNum = 0;

		}
		delete[]pBuff;
	}

	// Read rest ctts data
	llStartPos = pTrackInfo->lCTTSStartPos;
	nBuffSize = pTrackInfo->nCTTSBuffSize;
	nEntryNum = pTrackInfo->iComTimeSampleEntryNum;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = (nRead / 8) * 8;

		TCompositionTimeSample * pCompTimeTab = pTrackInfo->iComTimeSampleTab;
		nEntryBeg = (nEntryNum * 8 - nBuffSize) / 8;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 8 >= nRead)
				break;
			pCompTimeTab[i].iSampleCount = qcReadUint32BE(pData);
			pData += 4;
			pCompTimeTab[i].iSampleOffset = qcReadUint32BE(pData);
			pData += 4;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lCTTSStartPos += nRead;
			pTrackInfo->nCTTSBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lCTTSStartPos = 0;
			pTrackInfo->nCTTSBuffSize = 0;
		}
		delete[]pBuff;
	}

	// Read rest stts data
	llStartPos = pTrackInfo->lSTTSStartPos;
	nBuffSize = pTrackInfo->nSTTSBuffSize;
	nEntryNum = pTrackInfo->iTimeToSampleEntryNum;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = (nRead / 8) * 8;

		TTimeToSample * pTimeSampTab = pTrackInfo->iTimeToSampleTab;
		nEntryBeg = (nEntryNum * 8 - nBuffSize) / 8;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 8 >= nRead)
				break;
			pTimeSampTab[i].iSampleCount = qcReadUint32BE(pData);
			pData += 4;
			pTimeSampTab[i].iSampleDelta = qcReadUint32BE(pData);
			pData += 4;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lSTTSStartPos += nRead;
			pTrackInfo->nSTTSBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lSTTSStartPos = 0;
			pTrackInfo->nSTTSBuffSize = 0;
		}
		delete[]pBuff;
	}

	// Read rest stss data
	llStartPos = pTrackInfo->lSTSSStartPos;
	nBuffSize = pTrackInfo->nSTSSBuffSize;
	nEntryNum = pTrackInfo->iKeyFrameSampleEntryNum;
	nEntryBeg = 0;
	if (nBuffSize > 0)
	{
		pBuff = new char[nBuffSize + 1024];
		nRead = ReadHttpData(pHttp, llStartPos, pBuff, nBuffSize);
		nRead = nRead & (~0X03);

		int * pKeySampTab = pTrackInfo->iKeyFrameSampleTab;
		nEntryBeg = (nEntryNum * 4 - nBuffSize) / 4;
		pData = (unsigned char *)pBuff;
		for (i = nEntryBeg; i < nEntryNum; i++)
		{
			if ((i - nEntryBeg) * 4 >= nRead)
				break;
			pKeySampTab[i] = qcReadUint32BE(pData) - 1;
			pData += 4;
			bNeedBuild = true;
		}
		if (nRead < nBuffSize)
		{
			pTrackInfo->lSTSSStartPos += nRead;
			pTrackInfo->nSTSSBuffSize -= nRead;
			bFinish = false;
		}
		else
		{
			pTrackInfo->lSTSSStartPos = 0;
			pTrackInfo->nSTSSBuffSize = 0;
		}
		delete[]pBuff;
	}

	delete pHttp;

	if (bNeedBuild)
		BuildSampleTab(pTrackInfo);

	if (bFinish)
		return QC_ERR_NONE;
	else
		return QC_ERR_RETRY;
}

int	CMP4Parser::ReadHttpData(void * pIO, long long llStartPos, char * pBuff, int nSize)
{
	if (pIO == NULL || pBuff == NULL || nSize <= 0)
		return QC_ERR_ARG;

	int nRead = 0;
	QCLOG_CHECK_FUNC(&nRead, m_pBaseInst, nSize);

	int nStartTime = qcGetSysTime();
	int nRC = 0;
	CHTTPClient *	pHttp = (CHTTPClient *)pIO;
	nRC = pHttp->Connect(m_pSourceURL, llStartPos, -1);
	if (nRC != QC_ERR_NONE)
		return 0;

	while (nRead < nSize)
	{
		nRC = pHttp->Read(pBuff + nRead, nSize - nRead);
		if (nRC >= 0)
		{
			if (nRC == 0)
				qcSleep(1000);
			nRead += nRC;
			//qcSleep(1000);
		}
		else
		{
			break;
		}
		if (m_pBaseInst->m_bForceClose)
			break;
		if (qcGetSysTime() - nStartTime >= 300 && nRead > 4)
			break;
	}

	pHttp->Disconnect();
	
	if (nRead >= 4)
	{
		QCMP4_MOOV_BUFFER	moovBuff;
		moovBuff.m_llMoovPos = llStartPos;
		moovBuff.m_pMoovData = pBuff;
		moovBuff.m_nMoovSize = nRead;

		m_fIO->SetParam(m_fIO->hIO, QCIO_PID_SAVE_MOOV_BUFFER, &moovBuff);
	}
	
	return nRead;
}
