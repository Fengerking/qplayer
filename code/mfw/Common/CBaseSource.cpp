/*******************************************************************************
	File:		CBaseSource.cpp

	Contains:	the base source implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"

#include "CBaseSource.h"
#include "CMsgMng.h"

#include "USourceFormat.h"
#include "USystemFunc.h"
#include "ULogFunc.h"

#define QC_NOTIFY_BUFF_TIME		5000

CBaseSource::CBaseSource(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_pSourceName(NULL)
	, m_pIO (NULL)
	, m_nStmSourceSel (-1)
	, m_pFmtStream (NULL)
	, m_pFmtAudio (NULL)
	, m_pFmtVideo (NULL)
	, m_pFmtSubtt (NULL)
	, m_llMaxBuffTime (60000)
	, m_llMinBuffTime (2000)
	, m_bNeedBuffing (false)
	, m_nBuffRead (0)
	, m_bSubttEnable (false)
	, m_nOpenCache(0)
	, m_bSourceLive (false)
	, m_pBuffMng (NULL)
	, m_pReadThread (NULL)
{
	SetObjectName ("CBaseSource");
	memset (&m_fIO, 0, sizeof (QC_IO_Func));
	m_fIO.pBaseInst = m_pBaseInst;

	memset (&m_buffInfo, 0, sizeof (m_buffInfo));
	UpdateInfo ();
}

CBaseSource::~CBaseSource(void)
{
	Close ();
}

int CBaseSource::Open (const char * pSource, int nType)
{
	UpdateInfo ();

	m_nSourceType = nType;
	if (pSource != NULL)
	{
		QC_DEL_A (m_pSourceName);
		m_pSourceName = new char[strlen(pSource) + 1];
		strcpy(m_pSourceName, pSource);
	}

	if (m_pBuffMng == NULL)
		m_pBuffMng = new CBuffMng (m_pBaseInst);

	return QC_ERR_NONE;
}

int CBaseSource::OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource)
{
	CBaseSource::Open(pSource, nType);
	return QC_ERR_NONE;
}

int CBaseSource::Close (void)
{
	Stop ();

	if (m_fIO.hIO != NULL)
		qcDestroyIO (&m_fIO);

	QC_DEL_P (m_pBuffMng);
	QC_DEL_A (m_pSourceName);

	return QC_ERR_NONE;
}

int CBaseSource::ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait)
{
	if (pBuffInfo == NULL || ppBuffData == NULL)
		return QC_ERR_ARG;
	if (m_pBuffMng == NULL)
		return QC_ERR_STATUS;
	CAutoLock lock(&m_lckReader);
	long long llAudioBuffTime = GetBuffTime(true);
	long long llVideoBuffTime = GetBuffTime(false);
	if (m_bNeedBuffing)
	{
		bool bAudioReady = (llAudioBuffTime >= m_llMinBuffTime * 2) || m_bEOA;
		bool bVideoReady = (llVideoBuffTime >= m_llMinBuffTime * 2) || m_bEOV;
		if ((llAudioBuffTime >= m_llMinBuffTime * 4) || (llVideoBuffTime >= m_llMinBuffTime * 4))
		{
			bAudioReady = true;
			bVideoReady = true;
		}
        if(!bAudioReady || !bVideoReady)
        {
			qcSleep (5000);
            if(!m_bSourceLive)
                return QC_ERR_BUFFERING;
            else
            {
            	if(!bAudioReady && !bVideoReady)
                    return QC_ERR_BUFFERING;
            }
		}
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_END_BUFFERING, (int)llAudioBuffTime, llVideoBuffTime);
		m_pBaseInst->m_bStartBuffing = false;
		m_bNeedBuffing = false;
	}
	
	int nRC = QC_ERR_NONE;
	*ppBuffData = NULL;
	if (m_bVideoNewPos && pBuffInfo->nMediaType == QC_MEDIA_Video)
		pBuffInfo->llTime = 0;

	nRC = m_pBuffMng->Read (pBuffInfo->nMediaType, pBuffInfo->llTime, ppBuffData);
	if (nRC == QC_ERR_FINISH)
	{
		*ppBuffData = NULL;
		return nRC;
	}
	else if (nRC == QC_ERR_RETRY)
	{
		if (pBuffInfo->nMediaType == QC_MEDIA_Audio && m_bEOA)
			nRC = QC_ERR_FINISH;
		if (pBuffInfo->nMediaType == QC_MEDIA_Video && m_bEOV)
			nRC = QC_ERR_FINISH;
		if (nRC == QC_ERR_FINISH)
		{
			*ppBuffData = NULL;
			return nRC;
		}
	}
	if (*ppBuffData == NULL && bWait)
	{
		int nTryTimes = 0;
		while (*ppBuffData == NULL && nTryTimes < 20 && !IsLive())
		{
			if (pBuffInfo->nMediaType == QC_MEDIA_Video)
			{
				if (llAudioBuffTime >= m_llMinBuffTime * 2)
					return QC_ERR_RETRY;
			}
			else if (pBuffInfo->nMediaType == QC_MEDIA_Audio)
			{
				if (llVideoBuffTime >= m_llMinBuffTime * 2)
					return QC_ERR_RETRY;
			}

			qcSleep (5000);
			if (m_pBaseInst->m_bForceClose == true)
				return QC_ERR_STATUS;				
			nRC = m_pBuffMng->Read (pBuffInfo->nMediaType, pBuffInfo->llTime, ppBuffData);
			nTryTimes++;
		}
	}
	if (*ppBuffData != NULL)
	{
		if (pBuffInfo->nMediaType == QC_MEDIA_Video)
		{
			if (m_bVideoNewPos)
			{
				(*ppBuffData)->uFlag |= QCBUFF_NEW_POS;
				m_bVideoNewPos = false;
			}
		}
		else if (pBuffInfo->nMediaType == QC_MEDIA_Audio)
		{
			if (m_bAudioNewPos)
			{
				(*ppBuffData)->uFlag |= QCBUFF_NEW_POS;
				m_bAudioNewPos = false;
			}
		}
		else if (pBuffInfo->nMediaType == QC_MEDIA_Subtt)
		{
			if (m_bSubttNewPos)
			{
				(*ppBuffData)->uFlag |= QCBUFF_NEW_POS;
				m_bSubttNewPos = false;
			}
		}
		m_pBuffMng->Return (*ppBuffData);
		m_nBuffRead++;
		if (m_pBaseInst->m_bStartBuffing)
		{
			m_pBaseInst->m_bStartBuffing = false;
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_END_BUFFERING, 0, 0);
		}
	}
	llAudioBuffTime = GetBuffTime(true);
	llVideoBuffTime = GetBuffTime(false);
	if (m_bSourceLive)
	{
		if (m_nBuffRead > 150 && ((llAudioBuffTime <= 0 && !m_bEOA) && (llVideoBuffTime <= 0 && !m_bEOA)))
		{
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			{
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_START_BUFFERING, 0, GetPos());
				m_pBaseInst->m_bStartBuffing = true;
			}
			m_bNeedBuffing = true;
		}
	}
	else
	{
		if (m_nBuffRead > 150 && ((llAudioBuffTime <= 0 && !m_bEOA) || (llVideoBuffTime <= 0 && !m_bEOV)))
		{
			if ((llAudioBuffTime < m_llMinBuffTime * 4) && (llVideoBuffTime < m_llMinBuffTime * 4))
			{
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				{
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_START_BUFFERING, 0, GetPos());
					m_pBaseInst->m_bStartBuffing = true;
				}
				m_bNeedBuffing = true;
			}
		}
	}

	return nRC;
}

int	CBaseSource::CanSeek (void)
{
	return 1;
}

long long CBaseSource::SetPos (long long llPos)
{
	CAutoLock lock(&m_lckReader);

	if (m_llSeekPos == llPos && (m_bAudioNewPos || m_bVideoNewPos))
		return m_llSeekPos;

	m_llSeekPos = llPos;

	m_bAudioNewPos = m_nStmAudioNum > 0 ? true : false;
	m_bVideoNewPos = m_nStmVideoNum > 0 ? true : false;
	m_bSubttNewPos = m_nStmSubttNum > 0 ? true : false;

	m_bEOA = m_nStmAudioNum > 0 ? false : true;
	m_bEOV = m_nStmVideoNum > 0 ? false : true;

	int nRC = QC_ERR_NEEDMORE;
	if (m_pBuffMng != NULL)
		nRC = m_pBuffMng->SetPos (llPos);
	m_nBuffRead = 0;

	if (nRC == QC_ERR_NEEDMORE)
		return 0;
	else
		return llPos;
}

long long CBaseSource::GetPos (void)
{
	if (m_pBuffMng == NULL)
		return 0;

	if (m_nStmAudioPlay >= 0)
		return m_pBuffMng->GetPlayTime (QC_MEDIA_Audio);
	else
		return m_pBuffMng->GetPlayTime (QC_MEDIA_Video);
}
 
long long CBaseSource::GetBuffTime (bool bAudio)
{
	if (m_pBuffMng == NULL)
		return 0;
	return m_pBuffMng->GetBuffTime (bAudio ? QC_MEDIA_Audio : QC_MEDIA_Video);
}

int CBaseSource::SetBuffTimer (long long llBuffTime)
{
	if (m_pBuffMng == NULL)
		return QC_ERR_STATUS;
	m_llMaxBuffTime = llBuffTime;
	return m_pBuffMng->SetBuffTime (llBuffTime);
}

int CBaseSource::GetStreamCount (QCMediaType nType)
{
	if (nType == QC_MEDIA_Source)
		return m_nStmSourceNum;
	else if (nType == QC_MEDIA_Video)
		return m_nStmVideoNum;
	else if (nType == QC_MEDIA_Audio)
		return m_nStmAudioNum;
	else if (nType == QC_MEDIA_Subtt)
		return m_nStmSubttNum;
	else
		return 0;
}

int CBaseSource::GetStreamPlay (QCMediaType nType)
{
	if (nType == QC_MEDIA_Source)
		return m_nStmSourceSel;//m_nStmSourcePlay;
	else if (nType == QC_MEDIA_Video)
		return m_nStmVideoPlay;
	else if (nType == QC_MEDIA_Audio)
		return m_nStmAudioPlay;
	else if (nType == QC_MEDIA_Subtt)
		return m_nStmSubttPlay;
	else
		return 0;
}

int CBaseSource::SetStreamPlay (QCMediaType nType, int nStream)
{
	if (nType == QC_MEDIA_Source)
		m_nStmSourceSel = nStream;
	else if (nType == QC_MEDIA_Video)
		m_nStmVideoPlay = nStream;
	else if (nType == QC_MEDIA_Audio)
		m_nStmAudioPlay = nStream;
	else if (nType == QC_MEDIA_Subtt)
		m_nStmSubttPlay = nStream;

	return QC_ERR_NONE;
}

long long CBaseSource::GetDuration (void)
{
	return m_llDuration;
}

bool CBaseSource::IsEOS (void)
{
	return m_bEOS;
}

bool CBaseSource::IsLive (void)
{
	return m_bSourceLive;
}

int	CBaseSource::Start (void)
{
	if (m_pReadThread == NULL)
	{
		m_pReadThread = new CThreadWork (m_pBaseInst);
		m_pReadThread->SetOwner (m_szObjName);
		m_pReadThread->SetWorkProc (this, &CThreadFunc::OnWork);
	}
	return m_pReadThread->Start ();
}

int CBaseSource::Pause (void)
{
	if (m_pReadThread == NULL)
		return QC_ERR_STATUS;
	return m_pReadThread->Pause ();
}

int	CBaseSource::Stop (void)
{
	if (m_pReadThread == NULL)
		return QC_ERR_STATUS;
	m_pReadThread->Stop ();
	QC_DEL_P (m_pReadThread);

	return QC_ERR_NONE;
}

int CBaseSource::SetParam (int nID, void * pParam)
{
	return QC_ERR_PARAMID;
}

int CBaseSource::GetParam (int nID, void * pParam)
{
	return QC_ERR_PARAMID;
}

QC_STREAM_FORMAT * CBaseSource::GetStreamFormat (int nID)
{
	if (nID == -1 || nID == 0)
		return m_pFmtStream;
	return NULL;
}

QC_AUDIO_FORMAT * CBaseSource::GetAudioFormat (int nID)
{
	if (nID == -1 || nID == 0)
		return m_pFmtAudio;
	return NULL;
}

QC_VIDEO_FORMAT * CBaseSource::GetVideoFormat (int nID) 
{
	if (nID == -1 || nID == 0)
		return m_pFmtVideo;
	return NULL;
}

QC_SUBTT_FORMAT * CBaseSource::GetSubttFormat (int nID) 
{
	if (nID == -1 || nID == 0)
		return m_pFmtSubtt;
	return NULL;
}

int CBaseSource::UpdateInfo (void)
{
	m_nStmSourceNum = -1;
	m_nStmVideoNum = -1;
	m_nStmAudioNum = -1;
	m_nStmSubttNum = -1;
	m_nStmSourcePlay = -1;
	m_nStmVideoPlay = -1;
	m_nStmAudioPlay = -1;
	m_nStmSubttPlay = -1;

	m_llDuration = 0;

	m_llSeekPos = 0;
	m_bVideoNewPos = false;
	m_bAudioNewPos = false;
	m_bSubttNewPos = false;

	m_bEOS = false;
	m_bEOV = false;
	m_bEOA = false;
	m_bNeedBuffing = false;

	m_pFmtStream = NULL;
	m_pFmtAudio = NULL;
	m_pFmtVideo = NULL;
	m_pFmtSubtt = NULL;

	return QC_ERR_NONE;
}

QCParserFormat CBaseSource::GetSourceFormat (const char * pSource)
{
	QCParserFormat	nFormat = QC_PARSER_NONE;
	int				nRC = QC_ERR_NONE;
	if (m_pIO == NULL)
		return nFormat;
	if (m_pIO->hIO == NULL)
	{
		nRC = qcCreateIO (&m_fIO, qcGetSourceProtocol (pSource));
		if (nRC < 0)
			return nFormat;
	}
	if (m_pIO->GetSize (m_pIO->hIO) <= 0)
	{
		nRC = m_pIO->Open(m_pIO->hIO, pSource, 0, QCIO_FLAG_READ);
		if (nRC != QC_ERR_NONE)
		{
			qcDestroyIO (&m_fIO);
			return nFormat;
		}
	}
	long long nSize = m_pIO->GetSize (m_pIO->hIO);
	if (nSize > 1024 * 1 || nSize == -1)
		nSize = 1024 * 1;
	unsigned char * pData = new unsigned char[nSize];
	nRC = m_pIO->ReadSync (m_pIO->hIO, 0, pData, nSize, 0);
	if (nRC <= 0)
	{
		qcDestroyIO (&m_fIO);
		delete []pData;
		return nFormat;
	}
	m_pIO->SetPos(m_pIO->hIO, 0, QCIO_SEEK_BEGIN);

	if (!strncmp ((char *)pData, "#EXTM3U", 7))
		nFormat = QC_PARSER_M3U8;
	else if (!strncmp ((char *)pData, "FLV", 3))
		nFormat = QC_PARSER_FLV;
	else
	{
		char szMP4Ext[5];
		strcpy (szMP4Ext, "moov");
		char szMP4Ext1[16];
		strcpy(szMP4Ext1, "ftypmp42");
		char szMP4Ext2[16];
		strcpy(szMP4Ext2, "ftypisom");
		char szMP4Ext3[16];
		strcpy(szMP4Ext3, "ftypqt");
		unsigned char * pFind = pData;
		while ((pFind - pData) < nSize - 4)
		{
			if (!memcmp (pFind, szMP4Ext, 4))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext1, 8))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext2, 8))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			else if (!memcmp(pFind, szMP4Ext3, 6))
			{
				nFormat = QC_PARSER_MP4;
				break;
			}
			pFind++;
		}
	}

	if (nFormat == QC_PARSER_NONE)
	{
		char * pType = NULL;
		if (m_pIO->GetParam(m_pIO->hIO, QCIO_PID_HTTP_CONTENT_TYPE, &pType) == QC_ERR_NONE)
		{
			if (!strcmp(pType, "audio/mpeg"))
				nFormat = QC_PARSER_MP3;
			else if (!strcmp(pType, "audio/aac"))
				nFormat = QC_PARSER_AAC;
			else if (!strcmp(pType, "video/mp4"))
				nFormat = QC_PARSER_MP4;
			else if (!strcmp(pType, "video/flv"))
				nFormat = QC_PARSER_FLV;
			else if (!strcmp(pType, "video/hls"))
				nFormat = QC_PARSER_M3U8;
			else if (!strcmp(pType, "video/m3u8"))
				nFormat = QC_PARSER_M3U8;
		}
	}
	delete []pData;
	return nFormat;
}
