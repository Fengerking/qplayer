/*******************************************************************************
	File:		CBoxSource.cpp

	Contains:	source box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcPlayer.h"

#include "CBoxSource.h"
#include "CBoxMonitor.h"

#include "CQCSource.h"
#include "CQCFFSource.h"
#include "CQCFFConcat.h"
#include "CExtAVSource.h"
#include "CExtIOSource.h"

#include "USourceFormat.h"
#include "UUrlParser.h"
#include "USystemFunc.h"
#include "ULogFunc.h"

CBoxSource::CBoxSource(CBaseInst * pBaseInst, void * hInst)
	: CBoxBase (pBaseInst, hInst)
	, m_pMediaSource (NULL)
	, m_bEnableSubtt (false)
{
	SetObjectName ("CBoxSource");
	m_nBoxType = OMB_TYPE_SOURCE;
	strcpy (m_szBoxName, "Source Box");

	memset(&m_fIO, 0, sizeof(QC_IO_Func));
	m_fIO.pBaseInst = m_pBaseInst;

	m_pInstCache = NULL;
}

CBoxSource::~CBoxSource(void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	Close();
	if (m_pInstCache != NULL)
		m_pInstCache->m_bForceClose = true;
	DelCache(NULL);
	QC_DEL_P(m_pInstCache);
}

int CBoxSource::OpenSource (const char * pSource, int nFlag)
{
	int nRC = QC_ERR_NONE;
	QCLOG_CHECK_FUNC(&nRC, m_pBaseInst, nFlag);

	m_pBaseInst->m_bHadOpened = false;

	CAutoLock lock(&m_mtRead);
	m_pBaseInst->SetForceClose(true);
	Close();
	m_pBaseInst->SetForceClose(false);

	m_pBaseInst->m_llFVideoTime = 0;
	m_pBaseInst->m_llFAudioTime = 0;
	m_pBaseInst->m_nVideoRndCount = 0;
	m_pBaseInst->m_nAudioRndCount = 0;

	if (nFlag == QCPLAY_OPEN_EXT_SOURCE_AV)
	{
		m_pMediaSource = new CExtAVSource(m_pBaseInst, m_hInst);
		m_pBaseInst->m_bHadOpened = true;
		return QC_ERR_NONE;
	}
	if (nFlag == QCPLAY_OPEN_EXT_SOURCE_IO && m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt == QC_PARSER_TS)
	{
		m_pMediaSource = new CExtIOSource(m_pBaseInst, m_hInst);
		m_pMediaSource->OpenIO(NULL, 0, QC_PARSER_TS, pSource);
		m_pBaseInst->m_bHadOpened = true;
		return QC_ERR_NONE;
	}

	CAutoLock lockCache(&m_mtCache);
	QCParserFormat	nSrcFormat = QC_PARSER_NONE;
	QC_IO_Func *	pIO = NULL;
	QC_SRC_Cache *	pCache = GetCache(pSource);
	if (pCache != NULL)
	{
		nSrcFormat = pCache->nSrcFormat;		
		QCLOGI ("Find the cache source! the format is % 8X", nSrcFormat);
	}
	pIO = &m_fIO;

	char	szURL[2048];
	memset(szURL, 0, sizeof(szURL));
	QCIOProtocol	nProtocol = qcGetSourceProtocol(pSource);
	if (nProtocol == QC_IOPROTOCOL_HTTP || nProtocol == QC_IOPROTOCOL_RTMP || nProtocol == QC_IOPROTOCOL_RTSP)
		qcUrlConvert(pSource, szURL, sizeof(szURL));
	else
		strcpy(szURL, pSource);

	if (nFlag == QCPLAY_OPEN_EXT_SOURCE_IO)
	{
		qcCreateIO(&m_fIO, QC_IOPROTOCOL_EXTIO);
		m_fIO.Open(m_fIO.hIO, szURL, 0, 0);
		nSrcFormat = qcGetSourceFormat(szURL, pIO);
	}

	if (nProtocol == QC_IOPROTOCOL_RTSP)
	{
#ifdef __QC_OS_WIN32__
		m_pMediaSource = new CQCSource(m_pBaseInst, m_hInst);
#else
		return QC_ERR_UNSUPPORT;
#endif // __QC_OS_WIN32__
	}
	else if (nProtocol == QC_IOPROTOCOL_RTMP)
	{
		nSrcFormat = QC_PARSER_FLV;
		m_pMediaSource = new CQCSource(m_pBaseInst, m_hInst);
	}
	else 
	{
		if (pIO->hIO == NULL)
		{
			if (nSrcFormat == QC_PARSER_NONE)
				nSrcFormat = (QCParserFormat)m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt;
			if (nSrcFormat == QC_PARSER_NONE)
			{
				if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD && nProtocol == QC_IOPROTOCOL_HTTP)
					qcCreateIO(pIO, QC_IOPROTOCOL_HTTPPD);
				else
					qcCreateIO(pIO, nProtocol);
				nRC = pIO->Open(pIO->hIO, pSource, 0, QCIO_FLAG_READ);
				if (nRC != QC_ERR_NONE)
					qcDestroyIO(pIO);
				else
					nSrcFormat = qcGetSourceFormat(szURL, pIO);
				if (nSrcFormat == QC_PARSER_NONE)
					nSrcFormat = qcGetSourceFormat(pSource);
				if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
				{
					if (nSrcFormat != QC_PARSER_MP4)
					{
						if (pIO->hIO != NULL)
							qcDestroyIO(pIO);
					}
				}
			}
		}
		switch(nSrcFormat)
		{
		case QC_PARSER_M3U8:
		case QC_PARSER_MP4:
		case QC_PARSER_FLV:
		case QC_PARSER_TS:
			m_pMediaSource = new CQCSource(m_pBaseInst, m_hInst);
			break;

		case QC_PARSER_FFCAT:
			m_pMediaSource = new CQCFFConcat(m_pBaseInst, m_hInst);
			break;

		default:
			m_pMediaSource = new CQCFFSource(m_pBaseInst, m_hInst);
			break;
		}
	}
	m_pMediaSource->EnableSubtt(m_bEnableSubtt);
	if (pIO->hIO == NULL && nProtocol != QC_IOPROTOCOL_RTSP)
	{
        if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD && 
			nProtocol == QC_IOPROTOCOL_HTTP && nSrcFormat == QC_PARSER_MP4)
            qcCreateIO(pIO, QC_IOPROTOCOL_HTTPPD);
        else
            qcCreateIO(pIO, nProtocol);
		if (pCache != NULL && pCache->pIO != NULL)
		{
			QCLOGI ("Set the cache IO data!");
			pIO->SetParam(pIO->hIO, QCIO_PID_HTTP_COPYMEM, pCache->pIO->hIO);
		}
		nRC = pIO->Open(pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
        {
            m_pBaseInst->m_bHadOpened = true;
            return nRC;
        }			
	}

	if (nProtocol != QC_IOPROTOCOL_RTSP)
	{
		nRC = m_pMediaSource->OpenIO(pIO, nFlag, nSrcFormat, (const char *)szURL);
	}
	else
	{
		nRC = m_pMediaSource->Open((const char *)szURL, nSrcFormat);
	}
	if (nRC != QC_ERR_NONE && m_fIO.hIO != NULL)
	{
		int nDel = 1;
		m_fIO.SetParam(m_fIO.hIO, QCIO_PID_HTTP_DEL_FILE, &nDel);
	}

	m_pBaseInst->m_bHadOpened = true;

	return nRC;
}

int CBoxSource::Close (void)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;
	m_pMediaSource->Close ();
	QC_DEL_P (m_pMediaSource);

	if (m_fIO.hIO != NULL)
		qcDestroyIO(&m_fIO);

	return 0;
}

char * CBoxSource::GetSourceName (void)
{
	if (m_pMediaSource == NULL)
		return NULL;
	return m_pMediaSource->GetSourceName ();
}

int CBoxSource::GetStreamCount (QCMediaType nType)
{
	if (m_pMediaSource == NULL)
		return 0;
	return m_pMediaSource->GetStreamCount (nType);
}

int CBoxSource::GetStreamPlay (QCMediaType nType)
{
	if (m_pMediaSource == NULL)
		return 0;
	return m_pMediaSource->GetStreamPlay (nType);
}

int CBoxSource::SetStreamPlay (QCMediaType nType, int nIndex)
{
	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;
	int nRC = m_pMediaSource->SetStreamPlay (nType, nIndex);
	return nRC;
}

long long CBoxSource::GetDuration (void)
{
	if (m_pMediaSource == NULL)
		return 0;
	return m_pMediaSource->GetDuration ();
}

int	CBoxSource::Start (void)
{
	if (m_pMediaSource != NULL)
		m_pMediaSource->Start ();
	m_nStatus = OMB_STATUS_RUN;
	return QC_ERR_NONE;
}

int CBoxSource::Pause (void)
{
	m_nStatus = OMB_STATUS_PAUSE;
	if (m_pMediaSource != NULL)
		m_pMediaSource->Pause ();
	return QC_ERR_NONE;
}

int	CBoxSource::Stop (void)
{
	m_nStatus = OMB_STATUS_STOP;
	if (m_pMediaSource != NULL)
		m_pMediaSource->Stop ();
	return QC_ERR_NONE;
}

int CBoxSource::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	if (pBuffInfo == NULL || ppBuffData == NULL)
		return QC_ERR_ARG;
	*ppBuffData = NULL;

	CAutoLock lock (&m_mtRead);
	int nRC = QC_ERR_NONE;
	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;

	memcpy (m_pBuffInfo, pBuffInfo, sizeof (QC_DATA_BUFF));
	BOX_READ_BUFFER_REC_SOURCE

	nRC = m_pMediaSource->ReadBuff (pBuffInfo, ppBuffData, bWait);
	if (*ppBuffData != NULL)
		m_pBuffInfo->llTime = (*ppBuffData)->llTime;

	return nRC;
}

int CBoxSource::CanSeek (void)
{
	if (m_pMediaSource == NULL)
		return false;
	return m_pMediaSource->CanSeek ();
}

long long CBoxSource::SetPos (long long llPos)
{
	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;
	m_llSeekPos = llPos;
	return m_pMediaSource->SetPos (llPos);
}

long long CBoxSource::GetPos (void)
{
	if (m_pMediaSource == NULL)
		return 0;
	return m_pMediaSource->GetPos ();
}

int CBoxSource::SetParam (int nID, void * pParam)
{
	if (nID == QCPLAY_PID_EXT_SOURCE_DATA && m_fIO.hIO)
	{
		QC_DATA_BUFF * pBuff = (QC_DATA_BUFF *)pParam;
		int nWrite = pBuff->uSize;
		return m_fIO.Write(m_fIO.hIO, pBuff->pBuff, nWrite, -1);
	}

	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;
	return m_pMediaSource->SetParam (nID, pParam);
}

int CBoxSource::GetParam (int nID, void * pParam)
{
	if (m_pMediaSource != NULL)
	{
		if (nID == BOX_GET_AudioBuffTime)
			return (int)m_pMediaSource->GetBuffTime (true);
		else if (nID == BOX_GET_VideoBuffTime)
			return (int)m_pMediaSource->GetBuffTime (false);
		else if (nID == BOX_GET_SourceType)
			return m_pMediaSource->IsLive ();
	}

	if (m_pMediaSource == NULL)
		return QC_ERR_STATUS;

	return m_pMediaSource->GetParam (nID, pParam);
}

QC_STREAM_FORMAT * CBoxSource::GetStreamFormat (int nID) 
{
	if (m_pMediaSource != NULL)
		return m_pMediaSource->GetStreamFormat (nID);
	return NULL;
}

QC_AUDIO_FORMAT * CBoxSource::GetAudioFormat (int nID) 
{
	if (m_pMediaSource != NULL)
		return m_pMediaSource->GetAudioFormat (nID);
	return NULL;
}

QC_VIDEO_FORMAT * CBoxSource::GetVideoFormat (int nID) 
{
	if (m_pMediaSource != NULL)
		return m_pMediaSource->GetVideoFormat (nID);
	return NULL;
}

QC_SUBTT_FORMAT * CBoxSource::GetSubttFormat (int nID) 
{
	if (m_pMediaSource != NULL)
		return m_pMediaSource->GetSubttFormat (nID);
	return NULL;
}

int	CBoxSource::AddCache(const char * pSource, bool bIO)
{
	if (pSource == NULL)
		return QC_ERR_ARG;

	CBuffMng * pBuffMng = (CBuffMng *)m_pBaseInst->m_pBuffMng;
	long long llBuffAudio = 0;
	long long llBuffVideo = 0;
	while (m_nStatus == OMB_STATUS_PAUSE || m_nStatus == OMB_STATUS_RUN)
	{
		m_mtRead.Lock();
		if (pBuffMng != NULL)
		{
			llBuffAudio = pBuffMng->GetBuffTime(QC_MEDIA_Audio);
			llBuffVideo = pBuffMng->GetBuffTime(QC_MEDIA_Video);
		}
		m_mtRead.Unlock();
		if (llBuffAudio > 500 || llBuffVideo > 500)
			break;
		qcSleep(2000);
	}
	if (m_pBaseInst->m_bForceClose)
		return QC_ERR_STATUS;

	CAutoLock lock(&m_mtCache);
	if (m_lstCache.GetCount() >= 16)
		return QC_ERR_MEMORY;

	if (m_pInstCache == NULL)
	{
		m_pInstCache = new CBaseInst();
		strcpy(m_pInstCache->m_pSetting->g_qcs_szDNSServerName, m_pBaseInst->m_pSetting->g_qcs_szDNSServerName);
		strcpy(m_pInstCache->m_pSetting->g_qcs_szPDFileCachePath, m_pBaseInst->m_pSetting->g_qcs_szPDFileCachePath);
		strcpy(m_pInstCache->m_pSetting->g_qcs_szPDFileCacheExtName, m_pBaseInst->m_pSetting->g_qcs_szPDFileCacheExtName);
		strcpy(m_pInstCache->m_pSetting->g_qcs_pFileKeyText, m_pBaseInst->m_pSetting->g_qcs_pFileKeyText);
	}

	int nRC = QC_ERR_NONE;
	QC_SRC_Cache * pCache = NULL;
	pCache = GetCache(pSource);
	if (pCache != NULL)
		return QC_ERR_NONE;

	QCParserFormat	nSrcFormat = QC_PARSER_NONE;
	QCIOProtocol	nProtocol = qcGetSourceProtocol(pSource);
	if (nProtocol == QC_IOPROTOCOL_RTMP || nProtocol == QC_IOPROTOCOL_RTSP)
		return QC_ERR_UNSUPPORT;
	if (nProtocol != QC_IOPROTOCOL_HTTP)
		return QC_ERR_UNSUPPORT;

	pCache = new QC_SRC_Cache();
	memset(pCache, 0, sizeof(QC_SRC_Cache));
	pCache->pSource = new char[strlen(pSource) + 1];
	strcpy(pCache->pSource, pSource);
	pCache->pIO = new QC_IO_Func();
	memset(pCache->pIO, 0, sizeof(QC_IO_Func));
	m_pInstCache->m_bForceClose = false;
	pCache->pIO->pBaseInst = m_pInstCache;

	CBaseSource *	pMediaSource = NULL;
	char	szURL[2048];
	memset(szURL, 0, sizeof(szURL));
	qcUrlConvert(pSource, szURL, sizeof(szURL));

	if (bIO)
	{
		qcCreateIO(pCache->pIO, nProtocol);
		int nNotifyMsg = 0;
		pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_NOTIFY, &nNotifyMsg);
		nNotifyMsg = 1;
		pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_OPENCACHE, &nNotifyMsg);
		int nCacheSize = m_pBaseInst->m_pSetting->g_qcs_nIOCacheDownSize;
		pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_CACHE_SIZE, &nCacheSize);
		nRC = pCache->pIO->Open(pCache->pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			ReleaseCache(pCache);
			return nRC;
		}
		m_lstCache.AddTail(pCache);
		return QC_ERR_NONE;
	}

	nSrcFormat = (QCParserFormat)m_pBaseInst->m_pSetting->g_qcs_nPerferFileForamt;
	if (nSrcFormat == QC_PARSER_NONE)
	{
		if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD && nProtocol == QC_IOPROTOCOL_HTTP)
			qcCreateIO(pCache->pIO, QC_IOPROTOCOL_HTTPPD);
		else
			qcCreateIO(pCache->pIO, nProtocol);
		int nNotifyMsg = 0;
		pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_NOTIFY, &nNotifyMsg);
		nNotifyMsg = 1;
		pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_OPENCACHE, &nNotifyMsg);
		nRC = pCache->pIO->Open(pCache->pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			ReleaseCache(pCache);
			return nRC;
		}
		nSrcFormat = qcGetSourceFormat(szURL, pCache->pIO);
		if (nSrcFormat == QC_PARSER_NONE)
			nSrcFormat = qcGetSourceFormat(pSource);
		if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD)
		{
			if (nSrcFormat != QC_PARSER_MP4)
			{
				if (pCache->pIO->hIO != NULL)
					qcDestroyIO(pCache->pIO);
			}
		}
	}
	switch (nSrcFormat)
	{
	case QC_PARSER_M3U8:
	case QC_PARSER_MP4:
	case QC_PARSER_FLV:
		pMediaSource = new CQCSource(m_pInstCache, m_hInst);
		break;

	case QC_PARSER_FFCAT:
		pMediaSource = new CQCFFConcat(m_pInstCache, m_hInst);
		break;

	default:
		pMediaSource = new CQCFFSource(m_pInstCache, m_hInst);
		break;
	}
	if (pCache->pIO->hIO == NULL)
	{
		if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD &&
			nProtocol == QC_IOPROTOCOL_HTTP && nSrcFormat == QC_PARSER_MP4)
			qcCreateIO(pCache->pIO, QC_IOPROTOCOL_HTTPPD);
		else
			qcCreateIO(pCache->pIO, nProtocol);
		nRC = pCache->pIO->Open(pCache->pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			delete pMediaSource;
			ReleaseCache(pCache);
			return nRC;
		}
	}
	m_pInstCache->m_bHadOpened = false;
	pMediaSource->SetOpenCache(1);
	nRC = pMediaSource->OpenIO(pCache->pIO, 0, nSrcFormat, (const char *)szURL);
	int nStartTime = qcGetSysTime();
	while (qcGetSysTime() - nStartTime < 100)
	{
		qcSleep(2000);
		if (m_pBaseInst->m_bForceClose || m_pInstCache->m_bForceClose)
			break;
	}
	delete pMediaSource;
	pCache->pIO->Stop(pCache->pIO->hIO);
	pCache->pIO->SetParam(pCache->pIO->hIO, QCIO_PID_HTTP_DISCONNECT, NULL);
	if (nRC != QC_ERR_NONE)
	{
		ReleaseCache(pCache);
		m_pInstCache->m_bHadOpened = true;
		return nRC;
	}
	pCache->nSrcFormat = nSrcFormat;
	if (m_pBaseInst->m_pSetting->g_qcs_nPerferIOProtocol == QC_IOPROTOCOL_HTTPPD && nProtocol == QC_IOPROTOCOL_HTTP)
		ReleaseCache(pCache);
	else
		m_lstCache.AddTail(pCache);
	m_pInstCache->m_bHadOpened = true;

	return nRC;
}

int	CBoxSource::DelCache(const char * pSource)
{
	CAutoLock lock(&m_mtCache);
	QC_SRC_Cache * pCache = NULL;
	if (pSource == NULL)
	{
		pCache = m_lstCache.RemoveHead();
		while (pCache != NULL)
		{
			ReleaseCache(pCache);
			pCache = m_lstCache.RemoveHead();
		}
		return QC_ERR_NONE;
	}

	pCache = GetCache(pSource);
	if (pCache != NULL)
	{
		m_lstCache.Remove(pCache);
		ReleaseCache(pCache);
	}
	return QC_ERR_NONE;
}

QC_SRC_Cache * CBoxSource::GetCache(const char * pSource)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	if (pSource == NULL)
		return NULL;

	CAutoLock lock(&m_mtCache);
	QC_SRC_Cache * pCache = NULL;
	NODEPOS pos = m_lstCache.GetHeadPosition();
	while (pos != NULL)
	{
		pCache = m_lstCache.GetNext(pos);
		if (pCache->pSource != NULL && strcmp(pCache->pSource, pSource) == 0)
		{
			if (pCache->nSrcFormat == QC_PARSER_NONE)
				pCache->nSrcFormat = qcGetSourceFormat(pCache->pSource, pCache->pIO);
			return pCache;
		}
	}
	return NULL;
}

void CBoxSource::CancelCache(void)
{
	if (m_pInstCache != NULL)
		m_pInstCache->m_bForceClose = true;
}

int	CBoxSource::ReleaseCache(QC_SRC_Cache * pCache)
{
	if (pCache == NULL)
		return QC_ERR_ARG;

	QC_DEL_P(pCache->pSource);
	if (pCache->pIO != NULL)
	{
		if (pCache->pIO->hIO != NULL)
		{
			pCache->pIO->Close(pCache->pIO->hIO);
			qcDestroyIO(pCache->pIO);
		}
		delete pCache->pIO;
	}
	delete pCache;
	return QC_ERR_NONE;
}

int CBoxSource::GetIOType()
{
    if (m_fIO.GetType && m_fIO.hIO)
        return m_fIO.GetType (m_fIO.hIO);
    return QC_IOTYPE_NONE;
}
