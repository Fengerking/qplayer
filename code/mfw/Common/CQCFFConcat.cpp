/*******************************************************************************
	File:		CQCFFConcat.cpp

	Contains:	qc CQCFFConcat source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcPlayer.h"

#include "CQCFFConcat.h"
#include "CQCSource.h"
#include "CQCFFSource.h"
#include "CMsgMng.h"

#include "UAVParser.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "UUrlParser.h"
#include "ULogFunc.h"

CFFCatBuffMng::CFFCatBuffMng(CBaseInst * pBaseInst, CQCFFConcat * pConcat)
	: CBuffMng(pBaseInst)
	, m_pConcat(pConcat)
{
}

CFFCatBuffMng::~CFFCatBuffMng(void)
{
}

int	CFFCatBuffMng::Send(QC_DATA_BUFF * pBuff)
{
	if (m_pConcat->SendBuff(pBuff) == QC_ERR_NONE)
		return CBuffMng::Send(pBuff);
	return CBuffMng::Return(pBuff);
}

CQCFFConcat::CQCFFConcat(CBaseInst * pBaseInst, void * hInst)
	: CBaseSource(pBaseInst, hInst)
	, m_pFolder(NULL)
	, m_pCurItem(NULL)
	, m_pSource(NULL)
	, m_nOpenFlag(0)
{
	SetObjectName ("CQCFFConcat");
	m_bSourceLive = false;
	m_nPreloadTimeDef = m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem;
}

CQCFFConcat::~CQCFFConcat(void)
{
	Close ();
}

int CQCFFConcat::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	m_nSourceType = nType;
	if (pSource != NULL)
	{
		QC_DEL_A(m_pSourceName);
		m_pSourceName = new char[strlen(pSource) + 1];
		strcpy(m_pSourceName, pSource);
	}
	if (m_pBuffMng == NULL)
		m_pBuffMng = new CFFCatBuffMng(m_pBaseInst, this);

	m_nOpenFlag = nType;
	m_llItemTime = 0;
	m_llBuffTime = 0;
	m_llPlayTime = 0;
	int nRC = OpenConcat(pIO, nType, nFormat, pSource);
	if (nRC != QC_ERR_NONE)
		return nRC;

	m_buffInfo.nMediaType = QC_MEDIA_MAX;
	nRC = OpenItemSource(m_pCurItem);

	UpdateInfo();

	QCFF_CONCAT_ITEM *	pItem = NULL;
	long long			llDur = 0;
	NODEPOS pos = m_lstItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_lstItem.GetNext(pos);
		if (pItem->llDur == 0 && pItem->llStop != 0)
			pItem->llDur = pItem->llStop - pItem->llStart;
		if (pItem->llDur < 0)
			pItem->llDur = 0;

		if (pItem->llDur == 0 && llDur > 0)
			pItem->llDur = llDur;

		llDur = pItem->llDur;
		m_llDuration += llDur;
	}

	if ((nType & QCPLAY_OPEN_SAME_SOURCE) == QCPLAY_OPEN_SAME_SOURCE)
	{
		m_llSeekPos = 0;
		m_bAudioNewPos = true;
		m_bVideoNewPos = true;
		m_bSubttNewPos = true;
	}

	return nRC;
}

int CQCFFConcat::Close(void)
{
	QC_DEL_P(m_pSource);

	QC_DEL_A(m_pFolder);
	QCFF_CONCAT_ITEM * pItem = m_lstItem.RemoveHead();
	while (pItem != NULL)
	{
		QC_DEL_A(pItem->pURL);
		delete pItem;
		pItem = m_lstItem.RemoveHead();
	}
	return CBaseSource::Close();
}

int	CQCFFConcat::CanSeek(void)
{
	return 1;
}

long long CQCFFConcat::SetPos(long long llPos)
{
	long long llNewPos = CBaseSource::SetPos(llPos);
	if (llNewPos > 0)
		return llNewPos;

	CAutoLock lock(&m_lckParser);
	m_pCurItem = NULL;
	int nRC = QC_ERR_NONE;
	long long llDur = 0;
	QCFF_CONCAT_ITEM * pItem = NULL;
	NODEPOS pos = m_lstItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = m_lstItem.GetNext(pos);
		llDur += pItem->llDur;
		if (llPos < llDur)
		{
			m_pCurItem = pItem;
			break;
		}
	}
	if (m_pCurItem == NULL)
		return QC_ERR_STATUS;
	m_llItemTime = llDur - m_pCurItem->llDur;
	m_pCurItem->llSeek = llPos - m_llItemTime;

	OpenItemSource(m_pCurItem);
	m_buffInfo.nMediaType = QC_MEDIA_MAX;

	return llNewPos;
}

int	CQCFFConcat::GetStreamCount(QCMediaType nType)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return 0;
	return m_pSource->GetStreamCount(nType);
}

int	CQCFFConcat::GetStreamPlay(QCMediaType nType)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return -1;
	return m_pSource->GetStreamPlay(nType);
}

int CQCFFConcat::SetStreamPlay(QCMediaType nType, int nStream)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return QC_ERR_STATUS;
	return m_pSource->SetStreamPlay(nType, nStream);
}

QC_STREAM_FORMAT * CQCFFConcat::GetStreamFormat(int nID)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return NULL;
	return m_pSource->GetStreamFormat(nID);
}

QC_AUDIO_FORMAT * CQCFFConcat::GetAudioFormat(int nID)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return NULL;
	return m_pSource->GetAudioFormat(nID);
}

QC_VIDEO_FORMAT * CQCFFConcat::GetVideoFormat(int nID)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return NULL;
	return m_pSource->GetVideoFormat(nID);
}

QC_SUBTT_FORMAT * CQCFFConcat::GetSubttFormat(int nID)
{
	CAutoLock lock(&m_lckParser);
	if (m_pSource == NULL)
		return NULL;
	return m_pSource->GetSubttFormat(nID);
}

int CQCFFConcat::SetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	nRC = CBaseSource::SetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CQCFFConcat::GetParam (int nID, void * pParam)
{
	int nRC = QC_ERR_NONE;

	nRC = CBaseSource::GetParam (nID, pParam);
	if (nRC == QC_ERR_NONE)
		return QC_ERR_NONE;

	return QC_ERR_PARAMID;
}

int CQCFFConcat::OpenConcat(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	if (pIO == NULL || pIO->hIO == NULL)
		return QC_ERR_ARG;
	int nFileSize = pIO->GetSize(pIO->hIO);
	if (nFileSize < 8)
		return QC_ERR_FAILED;

	char * pExt = strrchr((char *)pSource, '/');
	if (pExt == NULL)
		pExt = strrchr((char *)pSource, '\\');
	if (pExt == NULL)
		return QC_ERR_ARG;
	QC_DEL_A(m_pFolder);
	m_pFolder = new char[strlen(pSource)];
	memset(m_pFolder, 0, strlen(pSource));
	strncpy(m_pFolder, pSource, pExt - pSource + 1);

	char *	pTxtFile = new char[nFileSize + 1];
	int nRC = pIO->Read(pIO->hIO, (unsigned char *)pTxtFile, nFileSize, true, QCIO_READ_DATA);
	if (nRC != QC_ERR_NONE)
	{
		delete[] pTxtFile;
		return nRC;
	}
	pTxtFile[nFileSize] = 0;

	char	szLine[4086];
	char *	pLine = pTxtFile;
	char *	pText = NULL;
	int		nLineSize = 0;
	char *	pSpace1 = NULL;
	char *	pSpace2 = NULL;
	QCFF_CONCAT_ITEM *	pItem = NULL;

	while (pLine - pTxtFile < nFileSize)
	{
		memset(szLine, 0, sizeof(szLine));
		nLineSize = qcReadTextLine(pLine, strlen(pLine), szLine, sizeof(szLine));
		if (pLine == pTxtFile)
		{
			if (strncmp(pLine, "ffconcat", 8))
			{
				delete[] pTxtFile;
				return QC_ERR_FAILED;
			}
			else
			{
				pLine += nLineSize;
				continue;
			}
		}
		pLine += nLineSize;
		if (szLine[0] == '#' || nLineSize <= 4)
			continue;

		pSpace1 = strchr(szLine, ' ');
		if (pSpace1 == NULL)
			continue;
		pSpace2 = pSpace1;
		while (*pSpace2 == ' ')
			pSpace2++;
		if (pSpace2 - szLine > nLineSize)
			continue;
		*pSpace1 = 0;

		if (!strcmp(szLine, "file"))
		{
			pItem = new QCFF_CONCAT_ITEM();
			m_lstItem.AddTail(pItem);
			memset(pItem, 0, sizeof(QCFF_CONCAT_ITEM));
			pItem->pURL = new char[strlen(m_pFolder) + strlen(pSpace2)];
			memset(pItem->pURL, 0, strlen(m_pFolder) + strlen(pSpace2));
			if (!strncmp(pSpace2, "subdir", 6))
			{
				strcpy(pItem->pURL, m_pFolder);
				strcat(pItem->pURL, pSpace2 + 6);
			}
			else if (pSpace2[0] == '\'')
			{
				strcpy(pItem->pURL, pSpace2 + 1);
				pItem->pURL[strlen(pSpace2) - 2] = 0;
			}
			else
			{
				strcpy(pItem->pURL, pSpace2);
			}
		}
		else if (!strcmp(szLine, "duration"))
		{
			if (pItem == NULL)
				return QC_ERR_FAILED;
			pItem->llDur = (long long)(atof(pSpace2) * 1000);
		}
		else if (!strcmp(szLine, "inpoint"))
		{
			if (pItem == NULL)
				return QC_ERR_FAILED;
			pItem->llStart = (long long)(atof(pSpace2) * 1000);
		}
		else if (!strcmp(szLine, "outpoint"))
		{
			if (pItem == NULL)
				return QC_ERR_FAILED;
			pItem->llStop = (long long)(atof(pSpace2) * 1000);
		}
	}
	delete[] pTxtFile;
	if (m_lstItem.GetCount() <= 0)
		return QC_ERR_FAILED;
	m_pCurItem = m_lstItem.GetHead();

	return QC_ERR_NONE;
}

int	CQCFFConcat::OpenItemSource(QCFF_CONCAT_ITEM * pItem)
{
	if (pItem == NULL)
		return QC_ERR_ARG;

	CAutoLock lock(&m_lckParser);
	int nRC = QC_ERR_NONE;
	m_pBaseInst->m_bForceClose = true;
	QC_DEL_P(m_pSource);
	m_pBaseInst->m_bForceClose = false;

	char	szURL[2048];
	memset(szURL, 0, sizeof(szURL));
	QCParserFormat	nSrcFormat = QC_PARSER_NONE;
	const char * pSource = pItem->pURL;
	QCIOProtocol	nProtocol = qcGetSourceProtocol(pSource);
	if (nProtocol == QC_IOPROTOCOL_HTTP || nProtocol == QC_IOPROTOCOL_RTMP || nProtocol == QC_IOPROTOCOL_RTSP)
		qcUrlConvert(pSource, szURL, sizeof(szURL));
	else
		strcpy(szURL, pSource);

	if (nProtocol == QC_IOPROTOCOL_RTSP)
	{
		m_pSource = new CQCFFSource(m_pBaseInst, m_hInst);
	}
	else if (nProtocol == QC_IOPROTOCOL_RTMP)
	{
		nSrcFormat = QC_PARSER_FLV;
		m_pSource = new CQCSource(m_pBaseInst, m_hInst);
	}
	else
	{
		nSrcFormat = (QCParserFormat)m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt;
		if (nSrcFormat == QC_PARSER_NONE)
		{
			if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD && nProtocol == QC_IOPROTOCOL_HTTP)
				qcCreateIO(&m_fIO, QC_IOPROTOCOL_HTTPPD);
			else
				qcCreateIO(&m_fIO, nProtocol);
			nRC = m_fIO.Open(m_fIO.hIO, pSource, 0, QCIO_FLAG_READ);
			if (nRC != QC_ERR_NONE)
				qcDestroyIO(&m_fIO);
			else
				nSrcFormat = qcGetSourceFormat(szURL, &m_fIO);
			if (nSrcFormat == QC_PARSER_NONE)
				nSrcFormat = qcGetSourceFormat(pSource);
			if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
			{
				if (nSrcFormat != QC_PARSER_MP4)
				{
					if (m_fIO.hIO != NULL)
						qcDestroyIO(&m_fIO);
				}
			}
		}
		switch (nSrcFormat)
		{
		case QC_PARSER_M3U8:
		case QC_PARSER_MP4:
		case QC_PARSER_FLV:
			m_pSource = new CQCSource(m_pBaseInst, m_hInst);
			break;

		default:
			m_pSource = new CQCFFSource(m_pBaseInst, m_hInst);
			break;
		}
	}
	if (m_fIO.hIO == NULL && nProtocol != QC_IOPROTOCOL_RTSP)
	{
		qcCreateIO(&m_fIO, nProtocol);
		nRC = m_fIO.Open(m_fIO.hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			m_pBaseInst->m_bHadOpened = true;
			return nRC;
		}
	}
	m_pSource->EnableSubtt(m_bSubttEnable);
	QC_Parser_Func * pParser = m_pSource->GetParserFunc();
	if (pParser != NULL)
		pParser->pBuffMng = m_pBuffMng;

	long long llSeekPos = pItem->llStart + pItem->llSeek;
	pItem->llSeek = 0;
	if (llSeekPos > 0)
		m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = llSeekPos * 30  / 1000 + 5 * 30;
	else
		m_pBaseInst->m_pSetting->g_qcs_nMoovPreLoadItem = m_nPreloadTimeDef;

	nRC = m_pSource->OpenIO(&m_fIO, m_nOpenFlag, nSrcFormat, (const char *)szURL);
	m_pBaseInst->m_bHadOpened = true;
	if (nRC != QC_ERR_NONE)
		return nRC;

	if (llSeekPos > 0)
		llSeekPos = m_pSource->SetPos(llSeekPos);

	pItem->bEOV = true;
	pItem->bEOA = true;
	if (m_pSource->GetStreamCount(QC_MEDIA_Video) > 0)
		pItem->bEOV = false;
	if (m_pSource->GetStreamCount(QC_MEDIA_Audio) > 0)
		pItem->bEOA = false;
	m_llBuffTime = 0;
	m_llPlayTime = 0;

	long long llDur = 0;
	if (pItem->llStop != 0)
	{
		llDur = pItem->llStop - pItem->llStart;
	}
	else
	{
		llDur = m_pSource->GetDuration();
		if (pItem->llStart > 0)
			llDur = llDur - pItem->llStart;
	}
	if (llDur != pItem->llDur)
	{
		pItem->llDur = llDur;
		m_llDuration = 0;
		NODEPOS pos = m_lstItem.GetHeadPosition();
		while (pos != NULL)
		{
			pItem = m_lstItem.GetNext(pos);
			m_llDuration += pItem->llDur;
		}
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_DURATION, 0, m_llDuration);
	}

	return nRC;
}

int CQCFFConcat::UpdateInfo(void)
{
	if (m_pSource == NULL)
		return QC_ERR_STATUS;

	CBaseSource::UpdateInfo();
	m_nStmSourceNum = m_pSource->GetStreamCount(QC_MEDIA_Source);
	m_nStmVideoNum = m_pSource->GetStreamCount(QC_MEDIA_Video);
	m_nStmAudioNum = m_pSource->GetStreamCount(QC_MEDIA_Audio);
	m_nStmSubttNum = m_pSource->GetStreamCount(QC_MEDIA_Subtt);
	m_nStmSourcePlay = m_pSource->GetStreamPlay(QC_MEDIA_Source);
	m_nStmAudioPlay = m_pSource->GetStreamPlay(QC_MEDIA_Audio);
	m_nStmVideoPlay = m_pSource->GetStreamPlay(QC_MEDIA_Video);
	m_nStmSubttPlay = m_pSource->GetStreamPlay(QC_MEDIA_Subtt);

	m_bEOA = m_nStmAudioNum > 0 ? false : true;
	m_bEOV = m_nStmVideoNum > 0 ? false : true;

	return QC_ERR_NONE;
}

int	CQCFFConcat::OnWorkItem(void)
{
	int nRC = QC_ERR_NONE;
	if ((m_bEOA && m_bEOV) || m_pBaseInst->m_bForceClose)
	{
		qcSleep(2000);
		return QC_ERR_STATUS;
	}

	long long llBuffAudio = m_pBuffMng->GetBuffTime(QC_MEDIA_Audio);
	long long llBuffVideo = m_pBuffMng->GetBuffTime(QC_MEDIA_Video);
	if (!m_pBuffMng->InSwitching() && (llBuffVideo > m_llMaxBuffTime || llBuffAudio > m_llMaxBuffTime))
	{
		qcSleep(2000);
		return QC_ERR_RETRY;
	}

	if (m_buffInfo.nMediaType == QC_MEDIA_MAX)
	{
		m_buffInfo.nMediaType = QC_MEDIA_Audio;
		if (m_bEOA)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
		if (llBuffVideo < llBuffAudio && !m_bEOV)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
	}

	// read the buffer from parser
	{
		CAutoLock lockParser(&m_lckParser);
		nRC = ReadParserBuff(&m_buffInfo);
	}
	if (nRC == QC_ERR_FINISH)
	{
		if (m_buffInfo.nMediaType == QC_MEDIA_Audio)
        {
            m_bEOA = true;
            m_bAudioNewPos = false;
        }
		else if (m_buffInfo.nMediaType == QC_MEDIA_Video)
        {
            m_bEOV = true;
            m_bVideoNewPos = false;
        }
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetAVEOS(m_bEOA, m_bEOV);
	}
	else if (nRC == QC_ERR_RETRY)
	{
		qcSleep(2000);
		return QC_ERR_RETRY;
	}

	if (nRC != QC_ERR_NONE)
	{
		if (m_buffInfo.nMediaType == QC_MEDIA_Audio && !m_bEOV)
			m_buffInfo.nMediaType = QC_MEDIA_Video;
		else if (!m_bEOA)
			m_buffInfo.nMediaType = QC_MEDIA_Audio;
		else
			m_buffInfo.nMediaType = QC_MEDIA_MAX;
		if (m_buffInfo.nMediaType != QC_MEDIA_MAX)
			nRC = ReadParserBuff(&m_buffInfo);
		if (nRC == QC_ERR_FINISH)
		{
			if (m_buffInfo.nMediaType == QC_MEDIA_Audio)
				m_bEOA = true;
			else if (m_buffInfo.nMediaType == QC_MEDIA_Video)
				m_bEOV = true;
			if (m_pBuffMng != NULL)
				m_pBuffMng->SetAVEOS(m_bEOA, m_bEOV);
		}
	}

	if (m_bEOA && m_bEOV)
	{
		m_pBuffMng->SetEOS(true);
		qcSleep(5000);
	}
	m_buffInfo.nMediaType = QC_MEDIA_MAX;

	return nRC;
}

int	CQCFFConcat::ReadParserBuff(QC_DATA_BUFF * pBuffInfo)
{
	if (m_pSource == NULL || m_pCurItem == NULL)
		return QC_ERR_NONE;

	int nRC = m_pSource->ReadParserBuff(pBuffInfo);
	if (m_pCurItem->llStop > 0 && m_llBuffTime > m_pCurItem->llStop)
	{
		if (pBuffInfo->nMediaType == QC_MEDIA_Audio)
			m_pCurItem->bEOA = true;
		else if (pBuffInfo->nMediaType == QC_MEDIA_Video)
			m_pCurItem->bEOV = true;
	}
	if (nRC == QC_ERR_FINISH)
	{
		if (pBuffInfo->nMediaType == QC_MEDIA_Audio)
			m_pCurItem->bEOA = true;
		else if (pBuffInfo->nMediaType == QC_MEDIA_Video)
			m_pCurItem->bEOV = true;
	}
	if (m_pCurItem == m_lstItem.GetTail())
	{
		if (pBuffInfo->nMediaType == QC_MEDIA_Audio && m_pCurItem->bEOA)
			return QC_ERR_FINISH;
		if (pBuffInfo->nMediaType == QC_MEDIA_Video && m_pCurItem->bEOV)
			return QC_ERR_FINISH;
	}

	if (m_pCurItem->bEOA && m_pCurItem->bEOV)
	{
		m_llItemTime = m_llItemTime + m_llPlayTime;
		QCFF_CONCAT_ITEM * pItem = NULL;
		NODEPOS pos = m_lstItem.GetHeadPosition();
		while (pos != NULL)
		{
			pItem = m_lstItem.GetNext(pos);
			if (pItem == m_pCurItem)
			{
				m_pCurItem = m_lstItem.GetNext(pos);
				OpenItemSource(m_pCurItem);
				return QC_ERR_NONE;
			}
		}
	}
	return QC_ERR_NONE;
}

int	CQCFFConcat::SendBuff(QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_pCurItem == NULL)
		return QC_ERR_ARG;

	m_llBuffTime = pBuff->llTime;
	if (m_pCurItem->llStop > 0 && pBuff->llTime > m_pCurItem->llStop)
		return QC_ERR_FINISH;

	if (m_pCurItem->llStart > 0)
	{
		if ((pBuff->uFlag && QCBUFF_HEADDATA) == 0)
		{
			if (pBuff->llTime < m_pCurItem->llStart)
				m_pCurItem->llStart = pBuff->llTime;
			pBuff->llTime = pBuff->llTime - m_pCurItem->llStart;
		}
	}
	m_llPlayTime = pBuff->llTime;
	pBuff->llTime += m_llItemTime;

	return QC_ERR_NONE;
}
