/*******************************************************************************
	File:		CMP4Parser.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-04		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "CMP4Parser.h"
#include "CMsgMng.h"

#include "UIntReader.h"
#include "UAVParser.h"
#include "ULogFunc.h"
#include "USourceFormat.h"

CMP4Parser::CMP4Parser(CBaseInst * pBaseInst)
	: CBaseParser (pBaseInst)
	, m_pSourceURL(NULL)
	, m_pIOReader(NULL)
	, m_nTimeScale(1)
	, m_nDuration(1)
	, m_nTotalBitrate(0)
	, m_nDataInterlace(0)
	, m_pVideoTrackInfo(NULL)
	, m_pAudioTrackInfo (NULL)
	, m_pCurTrackInfo(NULL)
	, m_pCurAudioInfo(NULL)
	, m_pCurVideoInfo(NULL)
	, m_bADTSHeader (true)
	, m_llRawDataBegin (0)
	, m_llRawDataEnd (0)
	, m_bReadVideoHead(false)
	, m_bReadAudioHead(true)
	, m_fIONew(NULL)
	, m_bNotifyDLPercent(false)
	, m_nIOProtocol(QC_IOPROTOCOL_HTTP)
	, m_nBoxSkipSize(1024 * 8)
	, m_nPreLoadSamples(300)
	, m_nConnectTime(0)
	, m_hMoovThread(NULL)
	, m_bBuildSample(true)
{
	SetObjectName ("CMP4Parser");

	m_pFragment = NULL;
	m_pTrackExt = NULL;
	m_nTrexCount = 0;
	m_nTrackIndex = 0;
	m_llMoofFirstPos = 0;
	m_llMoofNextPos = 0;
	m_bMoofIndexTab = false;
	m_llLoopTime = 0;
}

CMP4Parser::~CMP4Parser(void)
{
	Close ();
}

int CMP4Parser::Open (QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	int nRC = QC_ERR_NONE;
	m_fIO = pIO;
	if (m_pSourceURL == NULL)
		m_pSourceURL = new char[strlen(pURL) + 1];
	strcpy(m_pSourceURL, pURL);
	m_nBoxSkipSize = m_pBaseInst->m_pSetting->g_qcs_nMoovBoxSkipSize;
	m_nPreLoadSamples = m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem;
	if (m_bOpenCache)
	{
		m_nPreLoadSamples = m_nPreLoadSamples * 1024;
		m_nBoxSkipSize = 1024 * 1024 * 2;
	}
	m_bNotifyDLPercent = false;

	if (m_fIO->GetSize (m_fIO->hIO) <= 0)
	{
		int nStartTime = qcGetSysTime();
		nRC = m_fIO->Open(m_fIO->hIO, pURL, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			QCLOGI ("IO Open failed!");
			return QC_ERR_IO_FAILED;
		}
		m_nConnectTime = qcGetSysTime() - nStartTime;
	}
	else
	{
		m_fIO->SetPos (m_fIO->hIO, 0, QCIO_SEEK_BEGIN);
	}
	m_llFileSize = m_fIO->GetSize(m_fIO->hIO);
	if (m_llFileSize >= 0X7FFFFFFF && m_fIO->m_nProtocol == QC_IOPROTOCOL_HTTPPD)
	{
		m_fIO->Close(m_fIO->hIO);
		qcDestroyIO(m_fIO);

		int nStartTime = qcGetSysTime();
		qcCreateIO(m_fIO, QC_IOPROTOCOL_HTTP);
		nRC = m_fIO->Open(m_fIO->hIO, m_pSourceURL, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
			return QC_ERR_IO_FAILED;
		m_nConnectTime = qcGetSysTime() - nStartTime;
		QCLOGI("The file size is larger than 2G. The HTTPPD IO switch to HTTP.");
	}
	m_nIOProtocol = m_fIO->m_nProtocol;
	m_pIOReader = new CIOReader (m_pBaseInst, this);

	long long	nReadPos = 0;
	long long	nBoxSize = 0;
	int			nHeadSize = 0;
	m_nMP4KeySize = 0;
	if ((nHeadSize = LocationBox(nReadPos, nBoxSize, "moov", false)) < 0)
	{
		QCLOGW ("LocationBox moov failed!");
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_MP4_ERROR, 0, 0);
		return QC_ERR_FORMAT;
	}
	if ((int)(nBoxSize + nReadPos) > m_fIO->GetSize(m_fIO->hIO))
	{
		QCLOGW ("boasize is error!");	
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_MP4_ERROR, 0, 0);
		return QC_ERR_FORMAT;
	}
	if (m_nMP4KeySize > 0 && m_pBaseInst != NULL && strcmp(m_pBaseInst->m_pSetting->g_qcs_pCompKeyText, m_szCompKeyTxt))
	{
		QCLOGW("the comp key is error!");
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_MP4_ERROR, 0, 0);
		return QC_ERR_FORMAT;
	}

	//#define KBoxHeaderSize 16
	long long	llMoovPos = nReadPos + nHeadSize - 16;
	int			nMoovSize = (int)(nBoxSize - nHeadSize);
	m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_MOOVPOS, &llMoovPos);
	m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_MOOVSIZE, &nMoovSize);
	int nErr = ReadBoxMoov(nReadPos + nHeadSize, nBoxSize - nHeadSize);
	if (nErr != QC_ERR_NONE)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_MP4_ERROR, 0, 0);
		return QC_ERR_FORMAT;
	}

	UpdateFormat(false);

	int nTotalSamples = 0;
	if (m_pVideoTrackInfo != NULL)
		nTotalSamples += m_pVideoTrackInfo->iSampleCount;
	if (m_pAudioTrackInfo != NULL)
		nTotalSamples += m_pAudioTrackInfo->iSampleCount;
//	if (nTotalSamples == 0)
//		return QC_ERR_FORMAT;

	if (nTotalSamples > 0)
	{
		if (m_llRawDataBegin == 0)
		{
			nReadPos += nBoxSize;
			nHeadSize = LocationBox(nReadPos, nBoxSize, "mdat", false);
			if (nHeadSize < 0)
			{
				m_llRawDataBegin = nReadPos;
				m_llRawDataEnd = m_fIO->GetSize(m_fIO->hIO);
			}
			else {
				m_llRawDataBegin = nReadPos + nHeadSize;
				m_llRawDataEnd = nReadPos + nBoxSize;
			}
		}
		//m_fIO->SetPos(m_fIO->hIO, m_llRawDataBegin, QCIO_SEEK_BEGIN);
		long long llDataPos = m_llRawDataBegin;
		if (m_pAudioTrackInfo != NULL && m_pAudioTrackInfo->iSampleInfoTab != NULL)
			llDataPos = m_pAudioTrackInfo->iSampleInfoTab[0].iSampleFileOffset;
		if (m_pVideoTrackInfo != NULL && m_pVideoTrackInfo->iSampleInfoTab != NULL)
		{
			if (llDataPos > m_pVideoTrackInfo->iSampleInfoTab[0].iSampleFileOffset)
				llDataPos = m_pVideoTrackInfo->iSampleInfoTab[0].iSampleFileOffset;
		}
		m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_DATAPOS, &llDataPos);
		m_fIO->SetPos(m_fIO->hIO, llDataPos, QCIO_SEEK_BEGIN);

		CheckDataInterlace();

		if (m_pAudioTrackInfo != NULL && m_pAudioTrackInfo->iSampleInfoTab != NULL && m_pAudioTrackInfo->iSampleInfoTab->iSampleTimeStamp > 100)
			m_pBaseInst->m_llFAudioTime = m_pAudioTrackInfo->iSampleInfoTab->iSampleTimeStamp;
		if (m_pVideoTrackInfo != NULL && m_pVideoTrackInfo->iSampleInfoTab != NULL && m_pVideoTrackInfo->iSampleInfoTab->iSampleTimeStamp > 100)
			m_pBaseInst->m_llFVideoTime = m_pVideoTrackInfo->iSampleInfoTab->iSampleTimeStamp;
	}

	if (m_nStrmVideoCount > 0)
	{
		m_nStrmVideoPlay = 0;
		if (m_pVideoTrackInfo->iDuration > 0)
			m_llDuration = m_pVideoTrackInfo->iDuration;
	}
	if (m_nStrmAudioCount > 0)
	{
		m_nStrmAudioPlay = 0;
		if ((m_nStrmVideoCount > 0) && (m_llDuration > 0))
		{
			if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop == 0)
			{
				if (m_pAudioTrackInfo->iDuration > m_llDuration)
					m_llDuration = m_pAudioTrackInfo->iDuration;
			}
			else
			{
				if (m_pAudioTrackInfo->iDuration < m_llDuration)
					m_llDuration = m_pAudioTrackInfo->iDuration;
			}
		}
		else
		{
			m_llDuration = m_pAudioTrackInfo->iDuration;
		}
	}

	if (m_fIO->m_nProtocol == QC_IOPROTOCOL_HTTPPD && m_nDataInterlace != 0)
	{
		int nDel = 1;
		m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_DEL_FILE, &nDel);
		m_fIO->Close(m_fIO->hIO);
		qcDestroyIO(m_fIO);

		m_nIOProtocol = QC_IOPROTOCOL_HTTP;
		qcCreateIO(m_fIO, QC_IOPROTOCOL_HTTP);
		nRC = m_fIO->Open(m_fIO->hIO, m_pSourceURL, m_llRawDataBegin, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
			return QC_ERR_IO_FAILED;
		QCLOGI("The AV data is not interlace. The IO switches to HTTP.");
	}

	m_nTimeScale = 1;
	m_bReadVideoHead = false;
	m_bReadAudioHead = true;
	m_nAudioLoopTimes = 0;
	m_nVideoLoopTimes = 0;

	int nDLNotify = 1;
	if (m_fIO != NULL && m_fIO->hIO != NULL)
		m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_NOTIFYDL_PERCENT, &nDLNotify);
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONTENT_SIZE, 0, m_llFileSize);

	CreateNewIO();

	OnOpenDone(pURL);

	if (nTotalSamples <= 0)
	{
		m_llMoofFirstPos = 0;
		long long	llPos = nReadPos + nBoxSize;
		int			nLen = (int)(m_llFileSize - llPos);
		nRC = ReadBoxMoof(llPos, nLen);
		if (nRC != QC_ERR_NONE)
			return QC_ERR_FORMAT;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::Close (void)
{
	QC_DEL_P (m_pIOReader);
	while (m_hMoovThread != NULL)
		qcSleep(1000);

	QCMP4TrackInfo * pInfo = m_lstAudioTrackInfo.RemoveHead ();
	while (pInfo != NULL)
	{
		RemoveTrackInfo (pInfo);
		pInfo = m_lstAudioTrackInfo.RemoveHead ();
	}
	RemoveTrackInfo (m_pVideoTrackInfo);
	m_pVideoTrackInfo = NULL;

	if (m_fIONew != NULL)
	{
		qcDestroyIO(m_fIONew);
		QC_DEL_P(m_fIONew);
	}
	QC_DEL_A(m_pSourceURL);

	TTMoofIndexInfo * pMoofInfo = m_lstMoofInfo.RemoveHead();
	while (pMoofInfo != NULL)
	{
		delete pMoofInfo;
		pMoofInfo = m_lstMoofInfo.RemoveHead();
	}
	TTSampleInfo * pSampleInfo = m_lstSampleInfo.RemoveHead();
	while (pSampleInfo != NULL)
	{
		delete pSampleInfo;
		pSampleInfo = m_lstSampleInfo.RemoveHead();
	}
	QC_DEL_P(m_pFragment);
	QC_DEL_A(m_pTrackExt);

	CBaseParser::Close ();

	return QC_ERR_NONE;
}

int	CMP4Parser::SetStreamPlay (QCMediaType nType, int nStream)
{
	switch (nType)
	{
	case QC_MEDIA_Source:
		return QC_ERR_IMPLEMENT;
	case QC_MEDIA_Video:
		return QC_ERR_IMPLEMENT;
	case QC_MEDIA_Audio:
		if (m_nStrmAudioCount <= 1 || nStream == m_nStrmAudioPlay)
			return QC_ERR_IMPLEMENT;
		m_nStrmAudioPlay = nStream;
		UpdateFormat (true);
		break;
	case QC_MEDIA_Subtt:
		return QC_ERR_IMPLEMENT;
	default:
		break;
	}
	return QC_ERR_NONE;
}

int CMP4Parser::Read (QC_DATA_BUFF * pBuff)
{
	if (m_fIO->hIO == NULL)
		return QC_ERR_RETRY;

	CAutoLock lock(&m_mtSample);
	if (m_pFragment != NULL)
	{
		int	nRC = QC_ERR_FINISH;
		if (m_llMoofNextPos < m_llFileSize)
			nRC = ReadBoxMoof(m_llMoofNextPos, (int)(m_llFileSize - m_llMoofNextPos));
		if (nRC == QC_ERR_FINISH)
		{
			m_bEOS = true;
			if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop == 0)
				return QC_ERR_FINISH;
			else
			{
				m_llMoofNextPos = m_llMoofFirstPos;
				m_llLoopTime = m_llDuration;
				m_nAudioLoopTimes++;
				m_nVideoLoopTimes++;
			}
		}
		return QC_ERR_NONE;
	}

	if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop > 0 && m_nAudioLoopTimes != m_nVideoLoopTimes)
	{
		if (pBuff->nMediaType == QC_MEDIA_Audio && m_nAudioLoopTimes > m_nVideoLoopTimes && m_nStrmVideoCount > 0)
			pBuff->nMediaType = QC_MEDIA_Video;
		else if (pBuff->nMediaType == QC_MEDIA_Video && m_nVideoLoopTimes > m_nAudioLoopTimes && m_nStrmAudioCount > 0)
			pBuff->nMediaType = QC_MEDIA_Audio;
	}

	TTSampleInfo*	pSampleInfo = NULL;
	if(pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if(m_pCurAudioInfo == NULL && m_pAudioTrackInfo != NULL)
			m_pCurAudioInfo = m_pAudioTrackInfo->iSampleInfoTab;
		if(m_pCurAudioInfo == NULL)
			return QC_ERR_FINISH;
		if (m_pCurAudioInfo->iSampleIdx == 0x7fffffff)
			return QC_ERR_FINISH;
		if (m_pCurAudioInfo->iSampleFileOffset == 0 || m_pCurAudioInfo->iSampleTimeStamp == QC_MAX_NUM64_S)
			return QC_ERR_RETRY;
		pSampleInfo = m_pCurAudioInfo;
	}
	else if(pBuff->nMediaType == QC_MEDIA_Video)
	{
		if(m_pCurVideoInfo == NULL && m_pVideoTrackInfo != NULL)
			m_pCurVideoInfo = m_pVideoTrackInfo->iSampleInfoTab;
		if(m_pCurVideoInfo == NULL)
			return QC_ERR_RETRY;
		if (!m_bReadVideoHead)
		{
			m_bReadVideoHead = true;
			if (CreateHeadDataBuff(pBuff) == QC_ERR_NONE)
				return QC_ERR_NONE;
		}
		if (m_pCurVideoInfo->iSampleIdx == 0x7fffffff)
			return QC_ERR_FINISH;
		if (m_pCurVideoInfo->iSampleFileOffset == 0 || m_pCurVideoInfo->iSampleTimeStamp == QC_MAX_NUM64_S)
			return QC_ERR_RETRY;
		pSampleInfo = m_pCurVideoInfo;
	}
	if (pSampleInfo == NULL)
		return QC_ERR_STATUS;

	QC_DATA_BUFF * pNewBuff = m_pBuffMng->GetEmpty (pBuff->nMediaType, pSampleInfo->iSampleEntrySize + 1024);
	if (pNewBuff == NULL)
		return QC_ERR_MEMORY;
	pNewBuff->uBuffType = QC_BUFF_TYPE_Data;
	pNewBuff->nMediaType = pBuff->nMediaType;
	pNewBuff->llTime = pSampleInfo->iSampleTimeStamp;
	if (pSampleInfo->iFlag > 0)
		pNewBuff->uFlag = QCBUFF_KEY_FRAME;
	if(pNewBuff->uBuffSize < pSampleInfo->iSampleEntrySize + 1024) 
	{
		QC_DEL_A (pNewBuff->pBuff);
		pNewBuff->uBuffSize = pSampleInfo->iSampleEntrySize + 1024;
	}
	if (pNewBuff->pBuff == NULL)
		pNewBuff->pBuff = new unsigned char [pNewBuff->uBuffSize];

	int nReadSize  = 0;
	if(pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if (m_bADTSHeader)
			nReadSize = ReadSourceData (pSampleInfo->iSampleFileOffset, pNewBuff->pBuff + 7, pSampleInfo->iSampleEntrySize, QCIO_READ_AUDIO);
		else
			nReadSize = ReadSourceData (pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_AUDIO);
	}
	else if(pBuff->nMediaType == QC_MEDIA_Video)
		nReadSize = ReadSourceData (pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_VIDEO);
	else
		nReadSize = ReadSourceData (pSampleInfo->iSampleFileOffset, pNewBuff->pBuff, pSampleInfo->iSampleEntrySize, QCIO_READ_DATA);
	if (nReadSize != pSampleInfo->iSampleEntrySize)
	{
		m_pBuffMng->Return (pNewBuff);
		if (nReadSize == QC_ERR_Disconnected || m_nExitRead > 0)
			return QC_ERR_RETRY;
		return QC_ERR_FINISH;
	}

	if (pBuff->nMediaType == QC_MEDIA_Audio)
		m_pCurAudioInfo++;
	else if (pBuff->nMediaType == QC_MEDIA_Video)
		m_pCurVideoInfo++;

	pNewBuff->uSize = nReadSize;

	// For encrype data
	if (m_nMP4KeySize > 0)
	{
		int nKeySize = strlen(m_szFileKeyTxt);
		for (int i = 0; i < nReadSize + 8; i++)
		{
			for (int j = 0; j < nKeySize; j++)
				pNewBuff->pBuff[i] = pNewBuff->pBuff[i] ^ (m_szFileKeyTxt[j] + (nKeySize - j));
		}
	}

	if(pBuff->nMediaType == QC_MEDIA_Video)
	{
		if (m_pVideoTrackInfo->iCodecType == QC_CODEC_ID_MPEG4)
		{

		}
		else
		{
			unsigned int	nFrame = 0;
			int				nKeyFrame = 0;
			int	nErr = ConvertAVCFrame(pNewBuff->pBuff, nReadSize, nFrame, nKeyFrame);
			if (nErr != QC_ERR_NONE)
			{
				m_pBuffMng->Return(pNewBuff);
				return nErr;
			}
			if (m_pVideoTrackInfo->iCodecType == QC_CODEC_ID_H264 && nKeyFrame)
				pNewBuff->uFlag = QCBUFF_KEY_FRAME;
			if (m_nNALLengthSize < 3)
				pNewBuff->uSize = nFrame;
		}
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		if (pNewBuff->llTime < 0)
		{
//			m_pBuffMng->Return(pNewBuff);
//			return QC_ERR_RETRY;
		}
		if (m_bADTSHeader)
		{
			int	nHeadSize = qcAV_ConstructAACHeader(pNewBuff->pBuff, pNewBuff->uBuffSize, m_pFmtAudio->nSampleRate, m_pFmtAudio->nChannels, nReadSize);
			if (nHeadSize != 7)
			{
				m_pBuffMng->Return(pNewBuff);
				return QC_ERR_RETRY;
			}
			pNewBuff->uSize = nReadSize + 7;
		}
		if (!m_bReadAudioHead)
		{
			m_bReadAudioHead = true;
			pNewBuff->uFlag += QCBUFF_NEW_FORMAT;
			pNewBuff->pFormat = m_pFmtAudio;
		}
	}

	if (m_pBaseInst->m_pSetting->g_qcs_nPlaybackLoop > 0 && m_llDuration > 0)
	{
		if (pBuff->nMediaType == QC_MEDIA_Audio)
		{
			pNewBuff->llTime = pNewBuff->llTime + m_llDuration * m_nAudioLoopTimes;
			if ((m_pCurAudioInfo != NULL && m_pCurAudioInfo->iSampleIdx == 0x7fffffff) || pSampleInfo->iSampleTimeStamp >= m_llDuration)
			{
				m_pCurAudioInfo = m_pAudioTrackInfo->iSampleInfoTab;
				long long llIOPos = m_pCurAudioInfo->iSampleFileOffset;
				if (m_pVideoTrackInfo != NULL && m_pVideoTrackInfo->iSampleInfoTab != NULL)
				{
					if (llIOPos > m_pVideoTrackInfo->iSampleInfoTab->iSampleFileOffset)
						llIOPos = m_pVideoTrackInfo->iSampleInfoTab->iSampleFileOffset;
				}
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_LOOP_TIMES, m_nAudioLoopTimes, 0);
				if (m_llDuration * m_nAudioLoopTimes > QC_MAX_NUM64_S - 60000)
					m_nAudioLoopTimes = 0;
				else
					m_nAudioLoopTimes++;
				if (m_fIONew != NULL)
				{
					if (m_nDataInterlace == QC_MP4DATA_VIDEO_AUDIO)
						m_fIONew->SetPos(m_fIONew->hIO, llIOPos, QCIO_SEEK_BEGIN);
					else
						m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
				}
				else
				{
					if (m_pCurVideoInfo != NULL)
					{
						if (m_nAudioLoopTimes == m_nVideoLoopTimes)
							m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
					}
					else
					{
						m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
					}
				}
				QCLOGI ("Audio play loop times = %d. SetPos = % 8lld", m_nAudioLoopTimes, llIOPos);
			}
		}
		else
		{
			pNewBuff->llTime = pNewBuff->llTime + m_llDuration * m_nVideoLoopTimes;
			if ((m_pCurVideoInfo != NULL && m_pCurVideoInfo->iSampleIdx == 0x7fffffff) || pSampleInfo->iSampleTimeStamp >= m_llDuration)
			{
				m_pCurVideoInfo = m_pVideoTrackInfo->iSampleInfoTab;
				long long llIOPos = m_pCurVideoInfo->iSampleFileOffset;
				if (m_pAudioTrackInfo != NULL && m_pAudioTrackInfo->iSampleInfoTab != NULL)
				{
					if (llIOPos > m_pAudioTrackInfo->iSampleInfoTab->iSampleFileOffset)
						llIOPos = m_pAudioTrackInfo->iSampleInfoTab->iSampleFileOffset;
				}
				if (m_llDuration * m_nVideoLoopTimes > QC_MAX_NUM64_S - 60000)
					m_nVideoLoopTimes = 0;
				else
					m_nVideoLoopTimes++;
				if (m_nStrmAudioCount == 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_LOOP_TIMES, m_nVideoLoopTimes, 0);
				if (m_fIONew != NULL)
				{
					if (m_nDataInterlace == QC_MP4DATA_AUDIO_VIDEO)
						m_fIONew->SetPos(m_fIONew->hIO, llIOPos, QCIO_SEEK_BEGIN);
					else
						m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
				}
				else
				{
					if (m_pCurAudioInfo != NULL)
					{
						if (m_nAudioLoopTimes == m_nVideoLoopTimes)
							m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
					}
					else
					{
						m_fIO->SetPos(m_fIO->hIO, llIOPos, QCIO_SEEK_BEGIN);
					}
				}
				QCLOGI("Video play loop times = %d, SetPos = % 8lld", m_nVideoLoopTimes, llIOPos);
			}
		}
	}
	m_pBuffMng->Send(pNewBuff);

	return QC_ERR_NONE;
}

int CMP4Parser::CreateHeadDataBuff (QC_DATA_BUFF * pBuff)
{
	if (m_pVideoTrackInfo == NULL)
		return QC_ERR_STATUS;

	unsigned char *	pData = NULL;
	unsigned int	nSize = 0;
	if (m_pVideoTrackInfo->iAVCDecoderSpecificInfo != NULL)
	{
		pData = m_pVideoTrackInfo->iAVCDecoderSpecificInfo->iData;
		nSize = m_pVideoTrackInfo->iAVCDecoderSpecificInfo->iSize;
	}
	else if (m_pVideoTrackInfo->iMP4DecoderSpecificInfo != NULL)
	{
		pData = m_pVideoTrackInfo->iMP4DecoderSpecificInfo->iData;
		nSize = m_pVideoTrackInfo->iMP4DecoderSpecificInfo->iSize;
	}
	if (pData == NULL)
		return QC_ERR_STATUS;

	QC_DATA_BUFF * pNewBuff = m_pBuffMng->GetEmpty (pBuff->nMediaType, nSize + 1024);
	if (pNewBuff == NULL)
		return QC_ERR_MEMORY;
	pNewBuff->uBuffType = QC_BUFF_TYPE_Data;
	pNewBuff->nMediaType = pBuff->nMediaType;
	pNewBuff->llTime = 0;
	pNewBuff->uFlag = QCBUFF_HEADDATA;
	pNewBuff->uSize = nSize;
	if(pNewBuff->uBuffSize < pNewBuff->uSize + 1024) 
	{
		QC_DEL_A (pNewBuff->pBuff);
		pNewBuff->uBuffSize = pNewBuff->uSize + 1024;
	}
	if (pNewBuff->pBuff == NULL)
		pNewBuff->pBuff = new unsigned char [pNewBuff->uBuffSize];
	memcpy (pNewBuff->pBuff, pData, pNewBuff->uSize);
	m_pBuffMng->Send (pNewBuff);

	return QC_ERR_NONE;
}

int	CMP4Parser::CanSeek (void)
{
	return 1;
}

long long CMP4Parser::GetPos (void)
{
	if (m_pBuffMng != NULL)
		return m_pBuffMng->GetPlayTime (QC_MEDIA_Audio);
	return 0;
}

long long CMP4Parser::SetPos (long long llPos)
{
	if (m_pFragment != NULL)
		return SetFragPos(llPos);

	long long		llTimePos = llPos;
	long long		llSeekPos = -1;	
	long long		llDuration = 0;
	int				nKeyIdx = 0;
	int				i = 0;
	int				nVideoEOS = 0;

	TTSampleInfo *	pSeekVideo = NULL;
	TTSampleInfo *	pSampleInfo = NULL;
	int				nSampleCount = 0;

	int				nStartTime = qcGetSysTime();
	while (llSeekPos <= 0)
	{
        if (qcGetSysTime() - nStartTime > 5000)
            return -1;
		if (m_pBaseInst->m_bForceClose)
			return QC_ERR_FORCECLOSE;

		qcSleep(5000);
		CAutoLock lock(&m_mtSample);
		if (m_pVideoTrackInfo && m_pVideoTrackInfo->iSampleInfoTab)
		{
			pSampleInfo = m_pVideoTrackInfo->iSampleInfoTab;
			nSampleCount = m_pVideoTrackInfo->iSampleCount;

			if (llTimePos != QC_MAX_NUM64_S && llTimePos >= m_pVideoTrackInfo->iDuration)
			{
				nVideoEOS = 1;
				m_pCurVideoInfo = &pSampleInfo[nSampleCount];
			}
			else
			{
				if (m_pVideoTrackInfo->iKeyFrameSampleEntryNum > 0 && m_pVideoTrackInfo->iKeyFrameSampleTab != NULL)
				{
					for (i = 0; i < m_pVideoTrackInfo->iKeyFrameSampleEntryNum; i++)
					{
						nKeyIdx = m_pVideoTrackInfo->iKeyFrameSampleTab[i];
						if (nKeyIdx == 0XFFFFFFFF)
							break;
						if (pSampleInfo[nKeyIdx].iSampleTimeStamp >= llTimePos)
							break;
					}
					if (i > 0 && pSampleInfo[nKeyIdx].iSampleTimeStamp != QC_MAX_NUM64_S)
						nKeyIdx = m_pVideoTrackInfo->iKeyFrameSampleTab[i - 1];
				}
				// The sample didn't build finished
				if (nKeyIdx == 0XFFFFFFFF || pSampleInfo[nKeyIdx].iSampleTimeStamp == QC_MAX_NUM64_S)
					continue;
				llSeekPos = pSampleInfo[nKeyIdx].iSampleFileOffset;
				if (llSeekPos == 0) 
					continue;

				pSeekVideo = &pSampleInfo[nKeyIdx];
				llTimePos = pSeekVideo->iSampleTimeStamp;
				llSeekPos = pSeekVideo->iSampleFileOffset;
			}
		}

		if (m_pAudioTrackInfo != NULL &&m_pAudioTrackInfo->iSampleInfoTab != NULL)
		{
			pSampleInfo = m_pAudioTrackInfo->iSampleInfoTab;
			llDuration = m_pAudioTrackInfo->iDuration;
			nSampleCount = m_pAudioTrackInfo->iSampleCount;

			if (llTimePos != QC_MAX_NUM64_S && llTimePos >= llDuration)
			{
				m_pCurAudioInfo = &pSampleInfo[nSampleCount];
				if (nVideoEOS || m_pVideoTrackInfo == NULL)
					return QC_ERR_FINISH;
			}

			nKeyIdx = 0;
			while (nKeyIdx < nSampleCount)
			{
				if (pSampleInfo[nKeyIdx].iSampleTimeStamp >= llTimePos)
					break;
				nKeyIdx++;
			}

			if (nKeyIdx >= nSampleCount)
			{
				nKeyIdx = nSampleCount;
				if (nVideoEOS || m_pVideoTrackInfo == NULL)
					return QC_ERR_FINISH;
			}

			if (m_pVideoTrackInfo == NULL)
				llSeekPos = pSampleInfo[nKeyIdx].iSampleFileOffset;
			// The sample didn't finished
			if (pSampleInfo[nKeyIdx].iSampleFileOffset == 0 || pSampleInfo[nKeyIdx].iSampleTimeStamp == QC_MAX_NUM64_S) 
			{
				llSeekPos = 0;
                llTimePos = llPos;
				continue;
			}

			m_pCurAudioInfo = &pSampleInfo[nKeyIdx];
			if (m_fIONew != NULL && m_nDataInterlace == QC_MP4DATA_VIDEO_AUDIO)
			{
				llSeekPos = m_pCurAudioInfo->iSampleFileOffset;
				m_fIONew->SetPos(m_fIONew->hIO, llSeekPos, QCIO_SEEK_BEGIN);
			}
			else
			{
				if (llSeekPos > m_pCurAudioInfo->iSampleFileOffset)
					llSeekPos = m_pCurAudioInfo->iSampleFileOffset;
				m_fIO->SetPos(m_fIO->hIO, llSeekPos, QCIO_SEEK_BEGIN);
			}
		}
	}
	QCLOGI ("SetPos time = % 8lld,  Pos = % 8lld", llTimePos, llSeekPos);

	if (pSeekVideo != NULL)
	{
		m_pCurVideoInfo = pSeekVideo;
		if (m_fIONew != NULL && m_nDataInterlace == QC_MP4DATA_AUDIO_VIDEO)
			m_fIONew->SetPos(m_fIONew->hIO, llSeekPos, QCIO_SEEK_BEGIN);
		if (m_nStrmAudioCount <= 0)
			m_fIO->SetPos(m_fIO->hIO, llSeekPos, QCIO_SEEK_BEGIN);
	}

	m_llSeekIOPos = llSeekPos;
	m_llSeekPos = llTimePos;

	m_bReadVideoHead = false;
	m_bReadAudioHead = true;
	m_nAudioLoopTimes = 0;
	m_nVideoLoopTimes = 0;

	return QC_ERR_NONE;
}

int CMP4Parser::GetParam (int nID, void * pParam)
{
	return CBaseParser::GetParam (nID, pParam);
}

int CMP4Parser::SetParam (int nID, void * pParam)
{
	switch (nID)
	{
	case QCPARSER_PID_OpenCache:
		if (*(int*)pParam > 0)
			m_bOpenCache = true;
		else
			m_bOpenCache = false;
		return QC_ERR_NONE;

	default:
		break;
	}
	return CBaseParser::SetParam(nID, pParam);
}

int CMP4Parser::UpdateFormat (bool bAudioOnly)
{
	CAutoLock lock(&m_mtSample);
	if (!bAudioOnly && m_nStrmVideoCount > 0 && m_pVideoTrackInfo != NULL)
	{
		DeleteFormat (QC_MEDIA_Video);
		m_pFmtVideo = new QC_VIDEO_FORMAT ();
		memset (m_pFmtVideo, 0, sizeof (QC_VIDEO_FORMAT));
		m_pFmtVideo->nSourceType = QC_SOURCE_QC;
		m_pFmtVideo->nCodecID = m_pVideoTrackInfo->iCodecType;
		m_pFmtVideo->nWidth = m_pVideoTrackInfo->iWidth;
		m_pFmtVideo->nHeight = m_pVideoTrackInfo->iHeight;
		m_pFmtVideo->nNum = m_pVideoTrackInfo->iNum;
		m_pFmtVideo->nDen = m_pVideoTrackInfo->iDen;
		if (m_pFmtVideo->nDen == 0)
			m_pFmtVideo->nDen = 1;

		if (m_pVideoTrackInfo->iAVCDecoderSpecificInfo != NULL)
		{
			m_pFmtVideo->nHeadSize = m_pVideoTrackInfo->iAVCDecoderSpecificInfo->iSize;
			QC_DEL_A(m_pFmtVideo->pHeadData);
			m_pFmtVideo->pHeadData = new unsigned char[m_pFmtVideo->nHeadSize];
			memcpy(m_pFmtVideo->pHeadData, m_pVideoTrackInfo->iAVCDecoderSpecificInfo->iData, m_pFmtVideo->nHeadSize);
		}
		else if (m_pVideoTrackInfo->iMP4DecoderSpecificInfo != NULL)
		{
			m_pFmtVideo->nHeadSize = m_pVideoTrackInfo->iMP4DecoderSpecificInfo->iSize;
			QC_DEL_A(m_pFmtVideo->pHeadData);
			m_pFmtVideo->pHeadData = new unsigned char[m_pFmtVideo->nHeadSize];
			memcpy(m_pFmtVideo->pHeadData, m_pVideoTrackInfo->iMP4DecoderSpecificInfo->iData, m_pFmtVideo->nHeadSize);
		}

		if (m_pVideoTrackInfo->iAVCDecoderSpecificInfo != NULL)
		{
			// Send the head data with first video buffer
			//m_pFmtVideo->pPrivateData = m_pVideoTrackInfo->iAVCDecoderSpecificInfo;
			//m_pFmtVideo->nPrivateFlag = 1;
		}
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetFormat(QC_MEDIA_Video, m_pFmtVideo);
	}
	if (m_nStrmAudioCount > 0 && m_lstAudioTrackInfo.GetCount () > 0)
	{
		DeleteFormat (QC_MEDIA_Audio);
		m_pFmtAudio = new QC_AUDIO_FORMAT ();
		memset (m_pFmtAudio, 0, sizeof (QC_AUDIO_FORMAT));

		QCMP4TrackInfo *	pAudioTrackInfo = NULL;
		int					nSelTrack = m_nStrmAudioPlay >= 0 ? m_nStrmAudioPlay : 0;
		NODEPOS				pPos = m_lstAudioTrackInfo.GetHeadPosition ();
		while (pPos != NULL)
		{
			pAudioTrackInfo = m_lstAudioTrackInfo.GetNext (pPos);
			if (nSelTrack == 0)
				break;
			nSelTrack--;
		}
		m_pAudioTrackInfo = pAudioTrackInfo;
		m_pFmtAudio->nSourceType = QC_SOURCE_QC;
		m_pFmtAudio->nCodecID = pAudioTrackInfo->iCodecType;
		if (m_pFmtAudio->nCodecID == QC_CODEC_ID_AAC)
			m_bADTSHeader = true;
		else
			m_bADTSHeader = false;
		if (pAudioTrackInfo->iM4AWaveFormat != NULL)
		{
			m_pFmtAudio->nChannels = pAudioTrackInfo->iM4AWaveFormat->iChannels;
			m_pFmtAudio->nSampleRate = pAudioTrackInfo->iM4AWaveFormat->iSampleRate;
			m_pFmtAudio->nBits = pAudioTrackInfo->iM4AWaveFormat->iSampleBit;
		}
		if (!m_bADTSHeader)
		{
			m_pFmtAudio->nFourCC = pAudioTrackInfo->iFourCC;
			if (pAudioTrackInfo->iMP4DecoderSpecificInfo != NULL)
			{
				m_pFmtAudio->pPrivateData = pAudioTrackInfo->iMP4DecoderSpecificInfo;
				m_pFmtAudio->nPrivateFlag = 1;
			}
		}
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetFormat(QC_MEDIA_Audio, m_pFmtAudio);

		long long llTimeStamp = 0;
		if (m_pCurAudioInfo != NULL)
		{
			llTimeStamp = 0;
			if (m_pBuffMng != NULL)
			{
				llTimeStamp = m_pBuffMng->GetPlayTime(QC_MEDIA_Audio);
				m_pBuffMng->EmptyBuff(QC_MEDIA_Audio);
			}
			TTSampleInfo * pSampleInfo = m_pAudioTrackInfo->iSampleInfoTab;
			while (pSampleInfo != NULL && (pSampleInfo->iSampleTimeStamp < llTimeStamp))
			{
				pSampleInfo++;
				if (pSampleInfo->iSampleFileOffset == 0)
					break;
			}
			m_pCurAudioInfo = pSampleInfo;
			m_bReadAudioHead = false;
		}
	}

	return QC_ERR_NONE;
}

int CMP4Parser::CheckDataInterlace(void)
{
	m_nDataInterlace = QC_MP4DATA_INTERLACE;

	if (m_nStrmVideoCount <= 0 || m_nStrmAudioCount <= 0)
		return QC_ERR_NONE;

	if (m_pAudioTrackInfo == NULL || m_pAudioTrackInfo->iSampleInfoTab == NULL)
		return QC_ERR_NONE;

	if (m_pVideoTrackInfo == NULL || m_pVideoTrackInfo->iSampleInfoTab == NULL)
		return QC_ERR_NONE;

	long long		llBigPos = 0;
	int				nSampleCount = 0;
	TTSampleInfo *	pSampleInfo = NULL;
	long long llAudioStartPos = m_pAudioTrackInfo->iSampleInfoTab[0].iSampleFileOffset;
	long long llVideoStartPos = m_pVideoTrackInfo->iSampleInfoTab[0].iSampleFileOffset;
	if (llAudioStartPos > llVideoStartPos)
	{
		llBigPos = llAudioStartPos;
		pSampleInfo = m_pVideoTrackInfo->iSampleInfoTab;
		nSampleCount = m_pVideoTrackInfo->iSampleCount;
	}
	else
	{
		llBigPos = llVideoStartPos;
		pSampleInfo = m_pAudioTrackInfo->iSampleInfoTab;
		nSampleCount = m_pAudioTrackInfo->iSampleCount;
	}
	for (int i = 0; i < nSampleCount; i++)
	{
		if (pSampleInfo[i].iSampleFileOffset > llBigPos)
			return QC_ERR_NONE;
		if (pSampleInfo[i].iSampleTimeStamp > 5000 || llBigPos - pSampleInfo[i].iSampleFileOffset > 1024 * 1024 * 2)
			break;
	}

	if (llAudioStartPos > llVideoStartPos)
		m_nDataInterlace = QC_MP4DATA_VIDEO_AUDIO;
	else
		m_nDataInterlace = QC_MP4DATA_AUDIO_VIDEO;

	return QC_ERR_NONE;
}

int CMP4Parser::MP4ReadAt (long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	int nRead = ReadSourceData(llPos, pBuff, nSize, nFlag);
	return nRead == nSize ? QC_ERR_NONE : QC_ERR_FAILED;
}

int	CMP4Parser::ReadSourceData (long long llPos, unsigned char * pBuff, int nSize, int nFlag)
{
	if (m_fIO == NULL)
		return QC_ERR_STATUS;

//	if (nFlag == QCIO_READ_AUDIO || nFlag == QCIO_READ_VIDEO)
//		QCLOGI("Flag = %d,  pos: % 8lld", nFlag, llPos);

	int nRC = QC_ERR_FAILED;
	if (nFlag == QCIO_READ_HEAD)
	{
		long long llDownPos = 0;
		QCIO_READ_INFO readInfo;
		readInfo.llPos = llPos;
		readInfo.nSize = nSize;
		if (m_fIO->GetParam(m_fIO->hIO, QCIO_PID_HTTP_HAD_DOWNLOAD, &readInfo) != QC_ERR_NONE)
			llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
		if ((llDownPos > 0 && llDownPos + m_nBoxSkipSize < llPos + nSize))
		{
			int nStartTime = qcGetSysTime ();
			nRC = m_fIO->SetPos(m_fIO->hIO, llPos, QCIO_SEEK_BEGIN);
			QCLOGI("HTTP set pos = % 8lld, downpos = % 8lld. It used time = % 8d", llPos, llDownPos, qcGetSysTime() - nStartTime);
		}
		nRC = m_fIO->ReadSync(m_fIO->hIO, llPos, pBuff, nSize, nFlag);
		if (nRC <= 0)
			m_llFileSize = m_fIO->GetSize(m_fIO->hIO);

		return nRC;
	}

	if (m_fIONew != NULL)
	{
		if (nFlag == QCIO_READ_AUDIO && m_nDataInterlace == 2)
			nRC = m_fIONew->ReadSync(m_fIONew->hIO, llPos, pBuff, nSize, nFlag);
		else if (nFlag == QCIO_READ_VIDEO && m_nDataInterlace == 1)
			nRC = m_fIONew->ReadSync(m_fIONew->hIO, llPos, pBuff, nSize, nFlag);
	}
	if (nRC < 0)
		nRC = m_fIO->ReadSync(m_fIO->hIO, llPos, pBuff, nSize, nFlag);
	if (nRC <= 0)
	{
		m_llFileSize = m_fIO->GetSize(m_fIO->hIO);
		QCLOGW("ReadSourceData   % 8lld,   % 8d   Error = 0X%08X", llPos, nSize, nRC);
	}
	return nRC;
}

int	CMP4Parser::CreateNewIO(void)
{
//	if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
//		return QC_ERR_NONE;

	if (m_nDataInterlace == 0 || m_fIONew != NULL)
		return QC_ERR_NONE;

	m_fIONew = new QC_IO_Func();
	memset(m_fIONew, 0, sizeof(QC_IO_Func));
	m_fIONew->pBaseInst = m_pBaseInst;
	int nRC = qcCreateIO(m_fIONew, m_nIOProtocol);
	if (nRC != QC_ERR_NONE)
	{
		qcDestroyIO(m_fIONew);
		QC_DEL_P(m_fIONew);
		return nRC;
	}

	if (m_nDataInterlace == 1) // audio -- video
	{
		nRC = m_fIONew->Open(m_fIONew->hIO, m_pSourceURL, m_pVideoTrackInfo->iSampleInfoTab[0].iSampleFileOffset, QCIO_FLAG_READ);
	}
	else
	{
		nRC = m_fIONew->Open(m_fIONew->hIO, m_pSourceURL, m_pAudioTrackInfo->iSampleInfoTab[0].iSampleFileOffset, QCIO_FLAG_READ);
	}
	if (nRC != QC_ERR_NONE)
	{
		qcDestroyIO(m_fIONew);
		QC_DEL_P(m_fIONew);
	}
	return nRC;
}

