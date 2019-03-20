/*******************************************************************************
	File:		CFLVParser.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "CFLVParser.h"
#include "CMsgMng.h"

#include "UIntReader.h"
#include "USystemFunc.h"
#include "ULogFunc.h"
#include "UUrlParser.h"

CFLVParser::CFLVParser(CBaseInst * pBaseInst)
	: CBaseParser (pBaseInst)
	, m_llOffset (0)
	, m_pTagAudio (NULL)
	, m_pTagVideo (NULL)
	, m_pTagSubtt (NULL)
	, m_pTagBuffer (NULL)
	, m_nMaxSize (10240)
	, m_pMetaData(NULL)
	, m_nCurrMetaDataSize(0)
	, m_nMaxMetaDataSize(0)
	, m_llAudioLastTime(0)
	, m_nAudioLoopTimes(0)
	, m_llVideoLastTime(0)
	, m_nVideoLoopTimes(0)
	, m_llLoopOffsetTime(0)
	, m_nStartTime(0)
{
	SetObjectName ("CFLVParser");
}

CFLVParser::~CFLVParser(void)
{
	Close ();
}

int CFLVParser::Open (QC_IO_Func * pIO, const char * pURL, int nFlag)
{
    if(pIO == NULL || pIO->hIO == NULL)
        return QC_ERR_EMPTYPOINTOR;
	strcpy(m_szURL, pURL);
	m_fIO = pIO;

	int			nStartTime = 0;
	QCIOType	nIOType = m_fIO->GetType(m_fIO->hIO);
	if (nIOType == QC_IOTYPE_RTMP || nIOType == QC_IOTYPE_HTTP_LIVE)
	{
		m_bLive = true;
		nStartTime = qcGetSysTime();
	}
	if (!m_bLive)
	{
		if (m_fIO->GetSize(m_fIO->hIO) <= 0)
		{
			if (m_fIO->Open(m_fIO->hIO, pURL, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
				return QC_ERR_IO_FAILED;
		}
		else
		{
			m_fIO->SetPos(m_fIO->hIO, 0, QCIO_SEEK_BEGIN);
		}
	}

	unsigned char pHeadBuff[16];
	int nErr = QC_ERR_NONE;
	int nRead = 9;
	nErr = m_fIO->Read(m_fIO->hIO, pHeadBuff, nRead, true, QCIO_READ_DATA);
	while (m_bLive)
	{
		if (qcGetSysTime() - nStartTime > 20000)
			return QC_ERR_FAILED;
		if (m_pBaseInst->m_bForceClose)
			return QC_ERR_STATUS;
		if (nErr == QC_ERR_RETRY && !m_pBaseInst->m_bCheckReopn)
		{
			qcSleep(2000);
			nRead = 9;
			nErr = m_fIO->Read(m_fIO->hIO, pHeadBuff, nRead, true, QCIO_READ_DATA);
			continue;
		}
		if (nErr == QC_ERR_NONE)
			break;
		else
			return QC_ERR_IO_FAILED;
	}

	if (pHeadBuff[0] != 'F' || pHeadBuff[1] != 'L' || pHeadBuff[2] != 'V')
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PARSER_FLV_ERROR, 0, 0);
		return QC_ERR_FORMAT;
	}

	int nFLVFlag = pHeadBuff[4];
	m_llOffset = qcIntReadUint32BE(pHeadBuff + 5);

	if (nFLVFlag & FLV_HEADER_FLAG_HASVIDEO || m_bLive) 
	{
		m_nStrmVideoCount = 1;
		m_nStrmVideoPlay = 0;
		m_pFmtVideo = new QC_VIDEO_FORMAT ();
		memset (m_pFmtVideo, 0, sizeof (QC_VIDEO_FORMAT));
        m_pTagVideo = new CFLVTag (m_pBaseInst, m_pBuffMng, FLV_STREAM_TYPE_VIDEO);
		m_pTagVideo->SetParser(this);
	}
	if (nFLVFlag & FLV_HEADER_FLAG_HASAUDIO || m_bLive) 
	{
		m_nStrmAudioCount = 1;
		m_nStrmAudioPlay = 0;
		m_pFmtAudio = new QC_AUDIO_FORMAT ();
		memset (m_pFmtAudio, 0, sizeof (QC_AUDIO_FORMAT));
		m_pTagAudio = new CFLVTag(m_pBaseInst, m_pBuffMng, FLV_STREAM_TYPE_AUDIO);
		m_pTagAudio->SetParser(this);
	}

	int nTagCount = 0;
	while(nTagCount < 500)
	{
		if (m_pBaseInst->m_bForceClose == true)
			return QC_ERR_FORCECLOSE;

		if (ReadNextTag () < 0)
			break;
	
		if (m_pBaseInst->m_bForceClose == true)
			return QC_ERR_FORCECLOSE;

		if (CheckHaveBuff (nFLVFlag)) 
			break;

		nTagCount++;
		if (m_bLive && nTagCount > 10)
			break;
	}

	// Fill the audio and video format info
	if (m_pTagAudio != NULL)
	{
		m_pTagAudio->FillAudioFormat(m_pFmtAudio);
		if (m_pBuffMng != NULL)
		{
			m_pBuffMng->SetFormat(QC_MEDIA_Audio, m_pFmtAudio);
		}
	}
	if (m_pTagVideo != NULL)
	{
		m_pTagVideo->FillVideoFormat(m_pFmtVideo);
		if (m_pBuffMng != NULL)
			m_pBuffMng->SetFormat(QC_MEDIA_Video, m_pFmtVideo);
	}
    
	int nDLNotify = 1;
	m_fIO->SetParam(m_fIO->hIO, QCIO_PID_HTTP_NOTIFYDL_PERCENT, &nDLNotify);

	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->Notify(QC_MSG_HTTP_CONTENT_SIZE, 0, m_llFileSize);

    OnOpenDone(pURL);
	return QC_ERR_NONE;
}

int CFLVParser::Close (void)
{
	QC_DEL_A (m_pTagBuffer);
	QC_DEL_P (m_pTagAudio);
	QC_DEL_P (m_pTagVideo);
    QC_DEL_P (m_pTagSubtt);
    QC_DEL_A (m_pMetaData);
    ReleaseResInfo();

	CBaseParser::Close ();

	return QC_ERR_NONE;
}

int CFLVParser::Read (QC_DATA_BUFF * pBuff)
{
#if 0 // for debug reconnect
	if (m_nStartTime == 0)
		m_nStartTime = qcGetSysTime();
	if (qcGetSysTime() - m_nStartTime > 15000 && qcGetSysTime() - m_nStartTime < 20000)
	{
		m_bDebugDisconnect = false;
		return QC_ERR_RETRY;
	}
	if (qcGetSysTime() - m_nStartTime >= 20000)
	{
		if (!m_bDebugDisconnect)
		{
			m_fIO->Close(m_fIO->hIO);
			m_fIO->Open(m_fIO->hIO, m_szURL, 0, QCIO_FLAG_READ);
			m_bDebugDisconnect = true;
		}
	}
#endif // for debug
	int nRC = ReadNextTag ();
	if (nRC == QC_ERR_Disconnected)
    {
        if(m_bLive)
            nRC = QC_ERR_FINISH;
        else
            return QC_ERR_RETRY;
    }

	if (nRC == QC_ERR_FINISH)
	{
		if (m_bLive)
		{
//            int nInterval = m_pBaseInst?m_pBaseInst->m_pSetting->g_qcs_nReconnectInterval:5000;//jim
//            if (m_nLastReadTime == 0)
//                m_nLastReadTime = qcGetSysTime ();
//            if (qcGetSysTime () - m_nLastReadTime > nInterval)
//            {
//                m_nLastReadTime = 0;
//                m_llOffset = 0;
//                if(m_pTagVideo)
//                    m_pTagVideo->OnDisconnect();
//                if(m_pTagAudio)
//                    m_pTagAudio->OnDisconnect();
//                nRC = m_fIO->Reconnect (m_fIO->hIO, NULL, -1);
//                return QC_ERR_NONE;
//            }
			return QC_ERR_RETRY;
		}
		else
		{
			m_bEOS = true;
			return QC_ERR_FINISH;
		}
	}
    
    if(nRC == QC_ERR_STATUS)
        return QC_ERR_RETRY;

	if (nRC < 0)
	{
		m_bEOS = true;
		return QC_ERR_FINISH;
	}

	// it will support for out customer later.
	if (pBuff != NULL)
	{
		return QC_ERR_NONE;
	}

	return QC_ERR_NONE;
}

int CFLVParser::Send(QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;

	if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
	{
		if (pBuff->nMediaType == QC_MEDIA_Video && pBuff->pFormat != NULL)
		{
			QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
			m_pBuffMng->SetFormat(QC_MEDIA_Video, pFmt);
		}
	}

	if (!m_bLive)
		return m_pBuffMng->Send(pBuff);
    if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
    {
        //pBuff->uFlag |= QCBUFF_NEW_POS;
        return m_pBuffMng->Send(pBuff);
    }
    
	int nCheckOffsetTime = 2000;
	if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
        //QCLOGI("[B]Time audio: %lld, audio loop %d, video loop %d", pBuff->llTime, m_nAudioLoopTimes, m_nVideoLoopTimes);
		if (m_nAudioLoopTimes >= m_nVideoLoopTimes)
			pBuff->llTime += m_llLoopOffsetTime;

		if (pBuff->llTime + nCheckOffsetTime < m_llAudioLastTime)
		{
			m_nAudioLoopTimes++;
			if (m_nAudioLoopTimes > m_nVideoLoopTimes)
            {
                m_llLoopOffsetTime = m_llAudioLastTime - pBuff->llTime + 30;
                QCLOGW("[B]Loop offset time(audio) %lld, audio %d, video %d", m_llLoopOffsetTime, m_nAudioLoopTimes, m_nVideoLoopTimes);
            }
				
			pBuff->uFlag |= QCBUFF_NEW_POS;
			pBuff->llTime += m_llLoopOffsetTime;
		}
		m_llAudioLastTime = pBuff->llTime;
	}
	else if (pBuff->nMediaType == QC_MEDIA_Video)
	{
        //QCLOGI("[B]Time video: %lld, audio loop %d, video loop %d", pBuff->llTime, m_nAudioLoopTimes, m_nVideoLoopTimes);
		if (m_nVideoLoopTimes >= m_nAudioLoopTimes)
			pBuff->llTime += m_llLoopOffsetTime;

		if (pBuff->llTime + nCheckOffsetTime < m_llVideoLastTime)
		{
			m_nVideoLoopTimes++;
			if (m_nVideoLoopTimes > m_nAudioLoopTimes)
            {
                m_llLoopOffsetTime = m_llVideoLastTime - pBuff->llTime + 30;
                QCLOGW("[B]Loop offset time(video) %lld, audio %d, video %d", m_llLoopOffsetTime, m_nAudioLoopTimes, m_nVideoLoopTimes);
            }
				
			pBuff->uFlag |= QCBUFF_NEW_POS;
			pBuff->llTime += m_llLoopOffsetTime;
		}
		m_llVideoLastTime = pBuff->llTime;
	}


	return m_pBuffMng->Send(pBuff);
}

int	CFLVParser::CanSeek (void)
{
	if (m_nIndexNum == 0)
		return 0;
	else
		return 1;
}

long long CFLVParser::GetPos (void)
{
	if (m_pBuffMng != NULL)
		return m_pBuffMng->GetPlayTime (QC_MEDIA_Audio);
	return 0;
}

long long CFLVParser::SetPos (long long llPos)
{
	m_llSeekPos = llPos;
	int i = 0;
	for (i = 0; i < m_nIndexNum; i++)
	{
		if (m_pIndexList[i * 2] > llPos)
		{
			if (i > 0)
				i--;
			m_llOffset = m_pIndexList[i * 2 + 1] - FLV_PREVTAG_SIZE;
			break;
		}
	}

	m_llSeekIOPos = m_llOffset;

	return QC_ERR_NONE;
}

int CFLVParser::GetParam (int nID, void * pParam)
{
	return CBaseParser::GetParam(nID, pParam);
}

int CFLVParser::SetParam (int nID, void * pParam)
{
	return CBaseParser::SetParam(nID, pParam);
}

int CFLVParser::ReadNextTag (void)
{
    if(!m_fIO)
        return QC_ERR_EMPTYPOINTOR;

    long long llOffsetStart = m_llOffset;
	unsigned char pHeadBuff[16];
	int nErr = QC_ERR_NONE;
	int nRead = FLV_PREVTAG_SIZE;
	nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset, pHeadBuff, nRead, true, QCIO_READ_DATA);
	if(nErr != QC_ERR_NONE)
        return nErr;

	if(pHeadBuff[0] == 'F' && pHeadBuff[1] == 'L' && pHeadBuff[2] == 'V') 
    {
        // live stream not support read backward
        int nLeft = FLV_HEAD_SIZE - nRead;
		nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset+nRead, pHeadBuff+nRead, nLeft, true, QCIO_READ_DATA);
		if(nErr != QC_ERR_NONE) 
			return nErr;
		m_llOffset = qcIntReadUint32BE(pHeadBuff + 5);
		return QC_ERR_RETRY;
    }
	m_llOffset += FLV_PREVTAG_SIZE;

	nRead = FLV_TAG_HEAD_SIZE;
	nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset, pHeadBuff, nRead, true, QCIO_READ_DATA);
	if(nErr != QC_ERR_NONE)
    {
        m_llOffset = llOffsetStart;
		m_fIO->SetPos(m_fIO->hIO, m_llOffset, QCIO_SEEK_BEGIN);
        return nErr;
    }
		
	m_llOffset += FLV_TAG_HEAD_SIZE;
    

	int				nStreamType = pHeadBuff[0];
	unsigned int	nDataSize = qcIntReadBytesNBE(pHeadBuff + 1, 3);
	long long		llTimeStamp = qcIntReadBytesNBE(pHeadBuff + 4, 3);
	llTimeStamp |= (pHeadBuff[7] << 24);

	if (nDataSize == 0)
    {
     //   m_llOffset = llOffsetStart;
		m_fIO->SetPos(m_fIO->hIO, m_llOffset, QCIO_SEEK_BEGIN);
        return QC_ERR_RETRY;
    }

	if(nDataSize > m_nMaxSize || m_pTagBuffer == NULL) 
	{
		if (m_pTagBuffer != NULL)
			delete []m_pTagBuffer;
		m_pTagBuffer = new unsigned char [QC_MAX (m_nMaxSize, nDataSize)];
		if (m_nMaxSize < nDataSize)
			m_nMaxSize = nDataSize;
	}

    int nReadFlag = QCIO_READ_DATA;
    if (nStreamType == FLV_TAG_TYPE_AUDIO)
        nReadFlag = QCIO_READ_AUDIO;
    if (nStreamType == FLV_TAG_TYPE_VIDEO)
        nReadFlag = QCIO_READ_VIDEO;

	nRead = nDataSize;
	nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset, m_pTagBuffer, nRead, true, nReadFlag);
	if(nErr != QC_ERR_NONE)
    {
        m_llOffset = llOffsetStart;
		m_fIO->SetPos(m_fIO->hIO, m_llOffset, QCIO_SEEK_BEGIN);
        return nErr;
    }
		
	m_llOffset += nDataSize;

	switch( nStreamType )
	{
	case FLV_TAG_TYPE_AUDIO:
		if (m_pTagAudio == NULL)
		{
			m_pTagAudio = new CFLVTag(m_pBaseInst, m_pBuffMng, FLV_STREAM_TYPE_AUDIO);
			m_pTagAudio->SetParser(this);
		}
		m_pTagAudio->AddTag(m_pTagBuffer, nDataSize, llTimeStamp);
		if (m_pFmtAudio != NULL && m_pFmtAudio->nCodecID == QC_CODEC_ID_NONE)
			m_pTagAudio->FillAudioFormat(m_pFmtAudio);
		break;
	case FLV_TAG_TYPE_VIDEO:
		if (m_pTagVideo == NULL)
		{
			m_pTagVideo = new CFLVTag(m_pBaseInst, m_pBuffMng, FLV_STREAM_TYPE_VIDEO);
			m_pTagVideo->SetParser(this);
		}
		m_pTagVideo->AddTag(m_pTagBuffer, nDataSize, llTimeStamp);
		if (m_pFmtVideo != NULL && m_pFmtVideo->nCodecID == QC_CODEC_ID_NONE)
			m_pTagVideo->FillVideoFormat(m_pFmtVideo);
		break;
	case FLV_TAG_TYPE_META:
		nRead = ReadMetaData(m_pTagBuffer,  nDataSize);
		if (m_nIndexNum == 0)
		{
			if (m_fIO != NULL && (m_fIO->GetType (m_fIO->hIO) == QC_IOTYPE_FILE))
			{
				long long llCurPos = m_llOffset;
				FillKeyFrameList ();
				m_llOffset = llCurPos;
			}
		}
		break;
	default:
		break;
	}
			
	return QC_ERR_NONE;
}

int CFLVParser::ReadMetaData(unsigned char*	pBuffer, unsigned int nLength)
{
	char	szName[4096];
	int		nOffset = 0;
	int		type = pBuffer[0];

	nOffset++;
	if(type != AMF_DATA_TYPE_STRING) 
		return QC_ERR_UNSUPPORT;

	int nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
	if(nRead < 0) 
		return QC_ERR_UNSUPPORT;

	nOffset += 2 + nRead;

	if (!strcmp(szName, "onTextData"))
        return 1;

    if (strcmp(szName, "onMetaData") && strcmp(szName, "onCuePoint"))
        return 2;

    int nMaxSize = (nLength - nOffset) * 2;
    if(nMaxSize <= 0)
       return 0;
    if(nMaxSize > (int)m_nMaxMetaDataSize)
    {
        QC_DEL_A(m_pMetaData);
        m_nMaxMetaDataSize = nMaxSize;
        m_pMetaData = new char[m_nMaxMetaDataSize];
    }
    memset(m_pMetaData, 0, m_nMaxMetaDataSize);
    m_nCurrMetaDataSize = 0;
    
	nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
    if(m_nCurrMetaDataSize > 0)
    {
        m_nCurrMetaDataSize += sprintf(m_pMetaData+m_nCurrMetaDataSize, "%s", "}");
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_RTMP_METADATA, 0, (long long)0, m_pMetaData, NULL);
        //QCLOGI("Meta data encode: %s", m_pMetaData);
    }
	if(nRead < 0)
		return QC_ERR_UNSUPPORT;

	return 0;
}

bool CFLVParser::CheckHaveBuff (int nFlag) 
{
	long long llBuffTimeAudio = -1;
	long long llBuffTimeVideo = -1;

	if((nFlag & FLV_HEADER_FLAG_HASAUDIO))
	{
		if(m_pTagAudio == NULL)
			llBuffTimeAudio = 0;
		else 
			llBuffTimeAudio = m_pBuffMng->GetBuffTime (QC_MEDIA_Audio);
	}

	if((nFlag & FLV_HEADER_FLAG_HASVIDEO))
	{
		if(m_pTagVideo == NULL)
			llBuffTimeVideo = 0;
		else 
			llBuffTimeVideo = m_pBuffMng->GetBuffTime (QC_MEDIA_Video);

	}

	if(llBuffTimeAudio != 0 && llBuffTimeVideo != 0) 
		return true;

	return false;
}

int CFLVParser::AmfGetString(unsigned char*	pSrcBuffer, int nLength, char* pDesString)
{
    int length = qcIntReadUint16BE (pSrcBuffer);
    if (length >= nLength) 
        return -1;

    memcpy(pDesString, pSrcBuffer + 2, length);

    pDesString[length] = '\0';

    return length;
}

int CFLVParser::AmfReadObject(unsigned char* pBuffer, int nLength, const char* key)
{
	if(nLength < 0) 
		return -1;

	int		nOffset = 0;
	int		nRead = 0;
	double	dVal;
	char *	szName = new char[nLength+2];

	AMFDataType amf_type = (AMFDataType)pBuffer[0];
	nOffset++;

	switch (amf_type) 
	{
	case AMF_DATA_TYPE_NUMBER:
		dVal = qcIntReadDouble64(pBuffer + nOffset);
		nOffset += 8;
        EncodeMetaData(key, AMF_DATA_TYPE_NUMBER, (void*)&dVal);
		break;
	case AMF_DATA_TYPE_BOOL:
		dVal = pBuffer[nOffset];
		nOffset++;
        EncodeMetaData(key, AMF_DATA_TYPE_BOOL, (void*)&dVal);
		break;
	case AMF_DATA_TYPE_STRING:
		nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
		if (nRead < 0)
		{
			delete[]szName;
			return nRead;
		}
		nOffset += 2 + nRead;
        EncodeMetaData(key, AMF_DATA_TYPE_STRING, (void*)szName);
		break;
	case AMF_DATA_TYPE_OBJECT:
		if (key && !strcmp(KEYFRAMES_TAG, key)) 
		{
			nRead = KeyFrameIndex(pBuffer + nOffset, nLength - nOffset);
			if(nRead < 0) 
				return nRead;
			nOffset += nRead;
		}
		while (nOffset < nLength - 2) 
		{
			nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) 
			{
				delete[]szName;
				return nRead;
			}
			nOffset += 2 + nRead;
			nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0)
			{
				delete[]szName;
				return nRead;
			}
			nOffset += nRead;
		}
		if (pBuffer[nOffset] != AMF_END_OF_OBJECT)
		{
			delete[]szName;
			return -1;
		}
		break;
	case AMF_DATA_TYPE_NULL:
	case AMF_DATA_TYPE_UNDEFINED:
	case AMF_DATA_TYPE_UNSUPPORTED:
		break;     
	case AMF_DATA_TYPE_MIXEDARRAY:
		unsigned int nLen;
		nLen = qcIntReadUint32BE(pBuffer + nOffset);
		nOffset += 4;
		while (nOffset < nLength - 2) {
			nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) 
			{
				delete[]szName;
				return nRead;
			}
			nOffset += 2 + nRead;
			nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, szName);
			if(nRead < 0) 
			{
				delete[]szName;
				return nRead;
			}
			nOffset += nRead;
		}

		if (pBuffer[nOffset] != AMF_END_OF_OBJECT) 
		{
			delete[]szName;
			return -1;
		}
		break;
	case AMF_DATA_TYPE_ARRAY:
		{
			unsigned int arraylen, i;
			arraylen = qcIntReadUint32BE(pBuffer + nOffset);
			nOffset += 4;
			for (i = 0; i < arraylen && nOffset < nLength - 1; i++) {
				nRead = AmfReadObject(pBuffer + nOffset, nLength - nOffset, NULL);
				if(nRead < 0) 
				{
					delete[]szName;
					return nRead;
				}
				nOffset += nRead;
			}
		}
		break;
	case AMF_DATA_TYPE_DATE:
		nOffset += 8 + 2;  
		break;
	default:                    
		delete[]szName;
		return -1;
	}

	if (key) 
	{
		if (amf_type == AMF_DATA_TYPE_NUMBER || amf_type == AMF_DATA_TYPE_BOOL) 
		{
			if (!strcmp(key, "duration")) 
				m_llDuration = (int)(dVal * 1000);
			else if (!strcmp(key, "width")) 
			{
				if (m_pFmtVideo != NULL)
					m_pFmtVideo->nWidth = (int)dVal * 1000;
			}
			else if (!strcmp(key, "height")) 
			{
				if (m_pFmtVideo != NULL)
					m_pFmtVideo->nHeight = (int)dVal * 1000;
			}
		}
	}

	delete[]szName;
	return nOffset;
}

int CFLVParser::KeyFrameIndex(unsigned char* pBuffer, int nLength)
{	
	unsigned int	nTimesLen = 0;
	unsigned int	nFileposLen = 0;
	int				nRead = 0;
	int				i;
    char			szName[256];
	int				nOffset = 0;

	while (nOffset < nLength - 2) 
	{
		nRead = AmfGetString(pBuffer + nOffset, nLength - nOffset, szName);
		if(nRead < 0)
			return nRead;
		nOffset += 2 + nRead;

		AMFDataType amf_type = (AMFDataType)pBuffer[nOffset];
		nOffset++;

        if (amf_type != AMF_DATA_TYPE_ARRAY)
            break;

        int nLen = qcIntReadUint32BE(pBuffer + nOffset);
		nOffset += 4;
        if (nLen >> 28)
            break;
		
		int nType = 0;
        if(!strcmp(KEYFRAMES_TIMESTAMP_TAG , szName)) 
            nTimesLen = nLen;
		else if (!strcmp(KEYFRAMES_BYTEOFFSET_TAG, szName)) {
            nFileposLen = nLen;
			nType = 1;
		} else 
            break;

		if(nTimesLen != 0 && nFileposLen != 0 && nFileposLen != nTimesLen) 
			break;

		if(m_pIndexList == NULL) {
			m_nIndexSize = nLen;
			m_pIndexList = new long long[m_nIndexSize * 2]; 
		}

		int nNum = 0;
        for (i = 0; i < nLen && nOffset < nLength - 1; i++) 
		{
            if (pBuffer[nOffset] != AMF_DATA_TYPE_NUMBER)
                break;
			nOffset++;
			if (nType == 0) // It is time
		       m_pIndexList[2 * i + nType] = (long long)(qcIntReadDouble64(pBuffer + nOffset) * 1000);
			else
		       m_pIndexList[2 * i + nType] = (long long)(qcIntReadDouble64(pBuffer + nOffset));
			nOffset += 8;
			nNum++;
        }
		if (m_nIndexNum < nNum)
			m_nIndexNum = nNum;
    }

	return 0;
}

int CFLVParser::FillKeyFrameList (void)
{
	if (m_fIO->GetType (m_fIO->hIO) != QC_IOTYPE_FILE)
		return QC_ERR_FAILED;

	int				nErr = QC_ERR_NONE;
	unsigned char	pHeadBuff[1024];
	unsigned char*	pDataBuff;
	int				nStreamType;
	int				nDataSize;
	long long		llTimeStamp;
	int				nRead = 1;
	int				nFlag;
	int				AVCPacketType;
	int				nKeyFrame;

	int				nStepSize = 8096;
	long long *		pIndexList[8096];
	int				nIndexNum = 0;
	int				nIndexUse = 0;
    

	memset (pIndexList, 0, 8096 * 4);

	while (nRead > 0)
	{
        long long llOffsetStart = m_llOffset;
		nRead = FLV_PREVTAG_SIZE;
		nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset, pHeadBuff, nRead, true, QCIO_READ_DATA);
		if(nErr != QC_ERR_NONE)
        {
            m_llOffset = llOffsetStart;
			m_fIO->SetPos(m_fIO->hIO, m_llOffset, QCIO_SEEK_BEGIN);
            break;
        }
			
		m_llOffset += FLV_PREVTAG_SIZE;

		nRead = FLV_TAG_HEAD_SIZE + 4;
		nErr = m_fIO->ReadAt (m_fIO->hIO, m_llOffset, pHeadBuff, nRead, true, QCIO_READ_DATA);
		if(nErr != QC_ERR_NONE)
        {
            m_llOffset = llOffsetStart;
			m_fIO->SetPos(m_fIO->hIO, m_llOffset, QCIO_SEEK_BEGIN);
            break;
        }
			
		m_llOffset += FLV_TAG_HEAD_SIZE;

		nDataSize = qcIntReadBytesNBE(pHeadBuff + 1, 3);
		m_llOffset += nDataSize;

		nStreamType = pHeadBuff[0];
		pDataBuff = pHeadBuff + FLV_TAG_HEAD_SIZE;
		if (nStreamType == FLV_TAG_TYPE_VIDEO)
		{
			nFlag = pDataBuff[0];
			AVCPacketType = pDataBuff[1];
			nKeyFrame = ((nFlag & FLV_FRAME_KEY) && AVCPacketType)? 1 : 0;
			if (!nKeyFrame)
				continue;
		}
		else if (m_pTagVideo != NULL)
		{
			continue;
		}

		if (nIndexUse == nStepSize/2)
		{
			nIndexNum++;
			nIndexUse = 0;
		}
		if (pIndexList[nIndexNum] ==NULL)
			pIndexList[nIndexNum] = new long long [nStepSize];

		llTimeStamp = qcIntReadBytesNBE(pHeadBuff + 4, 3);
		llTimeStamp |= (pHeadBuff[7] << 24);
		pIndexList[nIndexNum][nIndexUse * 2] = llTimeStamp;
		pIndexList[nIndexNum][nIndexUse * 2 + 1] = m_llOffset - nDataSize - FLV_TAG_HEAD_SIZE;
		nIndexUse++;
	}

	m_nIndexNum = nIndexNum * nStepSize / 2 + nIndexUse;
	m_nIndexSize = m_nIndexNum * 2;
	m_pIndexList = new long long [m_nIndexSize];
	char *		pListBuff = (char *)m_pIndexList;
	int			i = 0;
	for (i = 0; i < nIndexNum; i++)
	{
		memcpy (pListBuff, pIndexList[i], nStepSize * sizeof (long long));
		pListBuff += nStepSize * sizeof (long long);
	}
	memcpy (pListBuff, pIndexList[nIndexNum], nIndexUse * sizeof (long long) * 2);

	for (i = 0; i < nIndexNum; i++)
		delete []pIndexList[i];
	if (nIndexUse > 0)
		delete []pIndexList[nIndexNum];

	return QC_ERR_NONE;
}

int CFLVParser::EncodeMetaData(const char* pKey, int nValueType, void* pValue)
{
    if(!pKey || !pValue || !m_pMetaData)
        return 0;
    
    if(m_nCurrMetaDataSize == 0)
        m_nCurrMetaDataSize += sprintf(m_pMetaData+m_nCurrMetaDataSize, "%s", "{");
    else
        m_nCurrMetaDataSize += sprintf(m_pMetaData+m_nCurrMetaDataSize, "%s", ",");
    
    if(nValueType == AMF_DATA_TYPE_BOOL)
        m_nCurrMetaDataSize += sprintf(m_pMetaData+m_nCurrMetaDataSize, "\"%s\":%d", pKey, (int)(*(double*)pValue));
    else if(nValueType == AMF_DATA_TYPE_NUMBER)
        m_nCurrMetaDataSize += sprintf(m_pMetaData+m_nCurrMetaDataSize, "\"%s\":%d", pKey, (int)(*(double*)pValue));
	else if (nValueType == AMF_DATA_TYPE_STRING)
		m_nCurrMetaDataSize += sprintf(m_pMetaData + m_nCurrMetaDataSize, "\"%s\":\"%s\"", pKey, (char*)pValue);
    
    return m_nCurrMetaDataSize;
}
