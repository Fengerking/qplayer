/*******************************************************************************
	File:		CFLVTag.cpp

	Contains:	flv tag parser implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-07		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CFLVTag.h"

#include "CFLVParser.h"

#include "UIntReader.h"
#include "UAVParser.h"
#include "ULogFunc.h"

CFLVTag::CFLVTag(CBaseInst * pBaseInst, CBuffMng * pBuffMng, unsigned int nStreamType)
	: CBaseObject (pBaseInst)
	, m_pParser(NULL)
	, m_pBuffMng (pBuffMng)
	, m_nStreamType(nStreamType)
	, m_nNalLength(0)
	, m_nVideoCodec(0)
	, m_nWidth(0)
	, m_nHeight(0)
	, m_nNumRef (0)
	, m_nSarWidth (1)
	, m_nSarHeight (1)
	, m_pHeadVideoData (NULL)
	, m_nHeadVideoSize (0)
	, m_pExtVideoData(NULL)
	, m_nExtVideoSize(0)
	, m_nSyncWord(0X01000000)
	, m_nAudioCodec(0)
	, m_nSampleRate(0)
	, m_nChannel(0)
	, m_nSampleBits(0)
	, m_pHeadAudioData (NULL)
	, m_nHeadAudioSize (0)
	, m_bDisconnect(false)
{
	SetObjectName ("CFLVTag");
    memset(&m_FmtVideo, 0, sizeof(m_FmtVideo));
    memset(&m_FmtAudio, 0, sizeof(m_FmtAudio));
}

CFLVTag::~CFLVTag()
{
	QC_DEL_A(m_pHeadVideoData);
	QC_DEL_A(m_pHeadAudioData);
	QC_DEL_A(m_FmtVideo.pHeadData);
	QC_DEL_A(m_pExtVideoData);
}

int CFLVTag::AddTag(unsigned char * pData, unsigned int nSize, long long llTime) 
{
	int nErr = 0;
    
    if(nSize == 0 || pData == NULL) 
        return QC_ERR_ARG;

	if(m_nStreamType == FLV_STREAM_TYPE_VIDEO) 
		nErr = AddVideoTag(pData, nSize, llTime);
	else if(m_nStreamType == FLV_STREAM_TYPE_AUDIO) 
		nErr = AddAudioTag(pData, nSize, llTime);
	return nErr;
}

int CFLVTag::FillAudioFormat (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	if(m_nAudioCodec == FLV_CODECID_AAC) 
		pFmt->nCodecID = QC_CODEC_ID_AAC;
	else if (m_nAudioCodec == FLV_CODECID_MP3) 
		pFmt->nCodecID = QC_CODEC_ID_MP3;
	else if (m_nAudioCodec == FLV_CODECID_SPEEX)
		pFmt->nCodecID = QC_CODEC_ID_SPEEX;
	else
		pFmt->nCodecID = QC_CODEC_ID_AAC;
	pFmt->nSourceType = QC_SOURCE_QC;
	pFmt->nChannels = m_nChannel;
	pFmt->nSampleRate = m_nSampleRate;
	pFmt->nBits = m_nSampleBits;

	return QC_ERR_NONE;
}

int CFLVTag::FillVideoFormat (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	if(m_nVideoCodec == FLV_CODECID_H264) 
		pFmt->nCodecID = QC_CODEC_ID_H264;
	else if(m_nVideoCodec == FLV_CODECID_HEVC) 
		pFmt->nCodecID = QC_CODEC_ID_H265;
	else
		pFmt->nCodecID = QC_CODEC_ID_NONE;
	pFmt->nWidth = m_nWidth;
	pFmt->nHeight = m_nHeight;
	pFmt->nNum = m_nSarWidth;
	pFmt->nDen = m_nSarHeight;
	if (m_pHeadVideoData != NULL)
	{
		pFmt->nHeadSize = m_nHeadVideoSize;
		QC_DEL_A (pFmt->pHeadData);
		pFmt->pHeadData = new unsigned char[pFmt->nHeadSize];
		memcpy (pFmt->pHeadData, m_pHeadVideoData, pFmt->nHeadSize);
	}
	return QC_ERR_NONE;
}

int CFLVTag::AddAudioTag (unsigned char * pData, unsigned int nSize, long long llTime)
{
	int nFlag = pData[0];
    int nType = pData[1];

	int nAudioCodec = nFlag & FLV_AUDIO_CODECID_MASK;
	int nChannel = (nFlag & FLV_AUDIO_CHANNEL_MASK) == FLV_STEREO ? 2 : 1;
    int nSampleRate = 44100 << ((nFlag & FLV_AUDIO_SAMPLERATE_MASK) >> FLV_AUDIO_SAMPLERATE_OFFSET) >> 3;
    int nSampleBits = (nFlag & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;

	if(m_nAudioCodec == 0)
		m_nAudioCodec = nAudioCodec;
	else if(m_nAudioCodec != nAudioCodec)
		return QC_ERR_STATUS;

	if(m_nAudioCodec == FLV_CODECID_AAC) 
	{
		if(nType == 0) 
		{
            QCLOGI("[B]FLV audio head data comes, %d", m_bDisconnect?1:0);
			if(qcAV_ParseAACConfig (pData + 2, nSize - 2, &nSampleRate, &nChannel) == 0) 
			{
				m_nChannel = nChannel;
				m_nSampleRate = nSampleRate;
                QCLOGI("Audio format, %d, %d", m_nSampleRate, m_nChannel);
			}
			else if (m_nChannel == 0)
			{
				QCLOGW("The audio config data is wrong.");
				m_nChannel = nChannel;
				m_nSampleRate = nSampleRate;
			}
			return QC_ERR_NONE;
		} 
	}
	else if (m_nAudioCodec == FLV_CODECID_MP3)
	{
		m_nChannel = nChannel;
		m_nSampleRate = nSampleRate;
	}
	else if (m_nAudioCodec == FLV_CODECID_SPEEX)
	{
		if (m_nSampleBits == 0)
		{
			m_nChannel = nChannel;
			m_nSampleRate = 16000;
			if ((nFlag & FLV_AUDIO_SAMPLERATE_MASK) == 0X08)
				m_nSampleRate = 8000;
			else if ((nFlag & FLV_AUDIO_SAMPLERATE_MASK) == 0X00)
				m_nSampleRate = 16000;
			else if ((nFlag & FLV_AUDIO_SAMPLERATE_MASK) == 0X0C) // ?
				m_nSampleRate = 32000;
			m_nSampleBits = nSampleBits;
		}
	}

	QC_DATA_BUFF * pBuff = m_pBuffMng->GetEmpty (QC_MEDIA_Audio, nSize + 1024);
	if (pBuff == NULL)
		return QC_ERR_MEMORY;
	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	pBuff->nMediaType = QC_MEDIA_Audio;
	pBuff->llTime = llTime;
	pBuff->uFlag = QCBUFF_KEY_FRAME;

	if(pBuff->uBuffSize < nSize + 1024) 
	{
		QC_DEL_A (pBuff->pBuff);
		pBuff->uBuffSize = nSize + 1024;
	}
	if (pBuff->pBuff == NULL)
		pBuff->pBuff = new unsigned char [pBuff->uBuffSize];

	if(m_nAudioCodec == FLV_CODECID_AAC) 
	{
		if(qcAV_ConstructAACHeader(pBuff->pBuff, pBuff->uBuffSize, m_nSampleRate, m_nChannel, nSize - 2) != 7) 
		{
			m_pBuffMng->Return (pBuff);
			return QC_ERR_STATUS;
		}
		memcpy(pBuff->pBuff + 7, pData + 2, nSize - 2);
		pBuff->uSize = nSize + 5;
	}
	else if (m_nAudioCodec == FLV_CODECID_SPEEX)
	{
		memcpy(pBuff->pBuff, pData + 1, nSize - 1);
		pBuff->uSize = nSize - 1;
	}
	else
	{
		memcpy(pBuff->pBuff, pData + 1, nSize - 1);
		pBuff->uSize = nSize - 1;
	}
    
	if (m_nSampleRate != m_FmtAudio.nSampleRate || m_nChannel != m_FmtAudio.nChannels || m_FmtAudio.nCodecID == QC_CODEC_ID_NONE)
    {
        QCLOGI("Audio format changed, %d, %d", m_nSampleRate, m_nChannel);
        FillAudioFormat(&m_FmtAudio);
        pBuff->uFlag |= QCBUFF_NEW_FORMAT;
        pBuff->pFormat = &m_FmtAudio;
    }
	pBuff->nUsed--;	
	return m_pParser->Send (pBuff);
}

int CFLVTag::AddVideoTag(unsigned char * pData, unsigned int nSize, long long llTime)
{
	int nFlag = pData[0];
	int nVideoCodec = nFlag & FLV_VIDEO_CODECID_MASK;
	
	if(m_nVideoCodec == 0) 
	{
		if(nVideoCodec != FLV_CODECID_H264 && nVideoCodec != FLV_CODECID_HEVC)
			return QC_ERR_UNSUPPORT;
		if(nSize < 5)
			return QC_ERR_ARG;
		m_nVideoCodec = nVideoCodec;
	} 
	else if(m_nVideoCodec != nVideoCodec)
		return QC_ERR_STATUS;
	
	int AVCPacketType = pData[1];
	int CTS = qcIntReadBytesNBE (pData + 2, 3);
	int nErr = 0;
	int nKey = ((nFlag & FLV_FRAME_KEY) && AVCPacketType)? 1 : 0;

	QC_DATA_BUFF * pBuff = m_pBuffMng->GetEmpty (QC_MEDIA_Video, nSize + 4096);
	if (pBuff == NULL)
		return QC_ERR_MEMORY;
	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	pBuff->nMediaType = QC_MEDIA_Video;
	pBuff->llTime = llTime;
	pBuff->uFlag = nKey ? QCBUFF_KEY_FRAME : 0;

//    QCLOGI ("FLV Video Time(DTS): %lld, CTS: %d", llTime, CTS);
    if (abs(CTS) > 1000*10) // workaround invalid timestamp, jira:324
        CTS = 0;

	if(pBuff->uBuffSize < nSize + 4096) 
	{
		QC_DEL_A (pBuff->pBuff);
		pBuff->uBuffSize = nSize + 4096;
	}
	if (pBuff->pBuff == NULL)
		pBuff->pBuff = new unsigned char [pBuff->uBuffSize];

	int nBuffSize = pBuff->uBuffSize;
	if(AVCPacketType == 0) 
	{
		m_nExtVideoSize = nSize - 5;
		QC_DEL_A(m_pExtVideoData);
		m_pExtVideoData = new unsigned char[m_nExtVideoSize];
		memcpy(m_pExtVideoData, pData + 5, m_nExtVideoSize);

		if(m_nVideoCodec == FLV_CODECID_H264) 
		{
			nErr = qcAV_ConvertAVCNalHead(pBuff->pBuff, nBuffSize, pData + 5, nSize - 5, m_nNalLength);
			qcAV_FindAVCDimensions(pBuff->pBuff, nBuffSize, &m_nWidth, &m_nHeight, &m_nNumRef, &m_nSarWidth, &m_nSarHeight);
		}
		else if(m_nVideoCodec == FLV_CODECID_HEVC) 
		{
			if (memcmp(&m_nSyncWord, pData + 5, sizeof(m_nSyncWord)))
			{
				nErr = qcAV_ConvertHEVCNalHead(pBuff->pBuff, nBuffSize, pData + 5, nSize - 5, m_nNalLength);
				qcAV_FindHEVCDimensions(pBuff->pBuff, nBuffSize, &m_nWidth, &m_nHeight);
				if (m_nWidth < 0 || m_nHeight < 0)
				{
					m_nWidth = 0;
					m_nHeight = 0;
				}
			}
			else
			{
				m_nNalLength = 4;
				nBuffSize = nSize - 5;
				memcpy(pBuff->pBuff, pData + 5, nBuffSize);
			}
		}
		if(nErr < 0)
		{
			pBuff->nUsed--;
			m_pBuffMng->Return (pBuff);
			return nErr;
		}

		pBuff->uSize = nBuffSize;
		pBuff->llTime = -1;

	} 
	else if(AVCPacketType == 1) 
	{
        llTime += CTS;
		int isKeyFrame = 0;
        pBuff->llTime = llTime;
		if (m_nVideoCodec == FLV_CODECID_H264)
		{
			nErr = qcAV_ConvertAVCNalFrame(pBuff->pBuff, nBuffSize, pData + 5, nSize - 5, m_nNalLength, isKeyFrame, m_nVideoCodec);
		}
		else
		{
			if (memcmp(&m_nSyncWord, pData + 5, sizeof(m_nSyncWord)))
			{
				nErr = qcAV_ConvertAVCNalFrame(pBuff->pBuff, nBuffSize, pData + 5, nSize - 5, m_nNalLength, isKeyFrame, m_nVideoCodec);
				isKeyFrame = qcAV_IsHEVCReferenceFrame(pData + 5 + m_nNalLength, nSize - 5 - m_nNalLength);
			}
			else
			{
				isKeyFrame = qcAV_IsHEVCReferenceFrame(pData + 5, nSize - 5);
			}
			if (isKeyFrame)
				pBuff->uFlag |= QCBUFF_KEY_FRAME;
			else
				pBuff->uFlag = 0;
		}

		if (nErr < 0)
		{
			pBuff->nUsed--;
			m_pBuffMng->Return (pBuff);
			return nErr;
		}

		if(m_nNalLength < 3) 
		{
			// onPayloadData(m_pBuffer, nSize, llTime);
			pBuff->uSize = nSize;
		}
		else
		{
			// onPayloadData(pData + 5, nSize - 5, llTime);
			memcpy (pBuff->pBuff, pData + 5, nSize - 5);
			pBuff->uSize = nSize - 5;
		}
	}
	pBuff->nUsed--;
	if (AVCPacketType == 0)
	{
        QCLOGI("[B]FLV video head data comes, %d", m_bDisconnect?1:0);
		pBuff->uFlag |= QCBUFF_HEADDATA;
        if(m_bDisconnect)
        {
            pBuff->uFlag |= QCBUFF_NEW_POS;
            m_bDisconnect = false;
        }
		QC_DEL_A (m_pHeadVideoData);
		m_nHeadVideoSize = pBuff->uSize;
		m_pHeadVideoData = new unsigned char[m_nHeadVideoSize + 32];
		memcpy (m_pHeadVideoData, pBuff->pBuff, m_nHeadVideoSize);
		if (m_nWidth != m_FmtVideo.nWidth || m_nHeight != m_FmtVideo.nHeight || m_FmtVideo.nCodecID == QC_CODEC_ID_NONE)
        {
            QCLOGI("Video format changed, %d X %d", m_nWidth, m_nHeight);
            FillVideoFormat(&m_FmtVideo);
            pBuff->uFlag |= QCBUFF_NEW_FORMAT;
            pBuff->pFormat = &m_FmtVideo;
        }
	}
	return m_pParser->Send(pBuff);
}

int CFLVTag::TagHeader (unsigned char * pData, unsigned int nSize, int &nStreamType, int &nDataSize, int &nTime)
{
	if(pData == NULL || nSize < 11)
        return QC_ERR_ARG;

	nStreamType = pData[0];
	nDataSize = qcIntReadBytesNBE (pData + 1, 3);
	nTime = qcIntReadBytesNBE (pData + 4, 3);
	nTime |= (pData[7] << 24);

	int streamID = qcIntReadUint32BE (pData + 8);

	return 0;
}

int CFLVTag::OnDisconnect()
{
    m_bDisconnect = true;
    return QC_ERR_NONE;
}
