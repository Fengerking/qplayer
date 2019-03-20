/*******************************************************************************
	File:		CBaseFFParser.cpp

	Contains:	base io implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcDef.h"
#include "CBaseFFParser.h"

CBaseFFParser::CBaseFFParser(QCParserFormat nFormat)
	: m_nFormat (nFormat)
	, m_pFmtStream (NULL)
	, m_pFmtAudio (NULL)
	, m_pFmtVideo (NULL)
	, m_pFmtSubtt (NULL)
	, m_bEOS (false)
	, m_bLive (false)
	, m_nIOType(QC_IOTYPE_NONE)
	, m_nStrmSourceCount (1)
	, m_nStrmVideoCount (0)
	, m_nStrmAudioCount (0)
	, m_nStrmSubttCount (0)
	, m_nStrmSourcePlay (0)
	, m_nStrmVideoPlay (-1)
	, m_nStrmAudioPlay (-1)
	, m_nStrmSubttPlay (-1)
	, m_llDuration (0)
	, m_llSeekPos (0)
	, m_bEnableSubtt (false)
	, m_nNALLengthSize (4)
	, m_pAVCBuffer (NULL)
	, m_nAVCSize (0)
{
}

CBaseFFParser::~CBaseFFParser(void)
{
	Close ();
}

int CBaseFFParser::Open (QC_IO_Func * pIO, const char * pURL, int nFlag)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseFFParser::Close (void)
{
	if (m_pFmtStream != NULL)
	{
		QC_DEL_P (m_pFmtStream);
	}
	if (m_pFmtAudio != NULL)
	{
		QC_DEL_A (m_pFmtAudio->pHeadData);
		QC_DEL_P (m_pFmtAudio);
	}
	if (m_pFmtVideo != NULL)
	{
		QC_DEL_A (m_pFmtVideo->pHeadData);
		QC_DEL_P (m_pFmtVideo);
	}
	if (m_pFmtSubtt != NULL)
	{
		QC_DEL_A (m_pFmtSubtt->pHeadData);
		QC_DEL_P (m_pFmtSubtt);
	}

	QC_DEL_A (m_pAVCBuffer);
	m_nAVCSize = 0;

	return QC_ERR_NONE;
}

int	CBaseFFParser::GetStreamCount (QCMediaType nType)
{
	if (nType == QC_MEDIA_Source)
		return m_nStrmSourceCount;
	else if (nType == QC_MEDIA_Video)
		return m_nStrmVideoCount;
	else if (nType == QC_MEDIA_Audio)
		return m_nStrmAudioCount;
	else if (nType == QC_MEDIA_Subtt)
		return m_nStrmSubttCount;
	return QC_ERR_IMPLEMENT;
}

int	CBaseFFParser::GetStreamPlay (QCMediaType nType)
{
	if (nType == QC_MEDIA_Source)
		return m_nStrmSourcePlay;
	else if (nType == QC_MEDIA_Video)
		return m_nStrmVideoPlay;
	else if (nType == QC_MEDIA_Audio)
		return m_nStrmAudioPlay;
	else if (nType == QC_MEDIA_Subtt)
		return m_nStrmSubttPlay;
	return QC_ERR_IMPLEMENT;
}

int	CBaseFFParser::SetStreamPlay (QCMediaType nType, int nStream)
{
	return QC_ERR_IMPLEMENT;
}

long long CBaseFFParser::GetDuration (void)
{
	return m_llDuration;
}

int	CBaseFFParser::GetStreamFormat (int nID, QC_STREAM_FORMAT ** ppStreamFmt)
{
	if (ppStreamFmt == NULL)
		return QC_ERR_ARG;
	*ppStreamFmt = m_pFmtStream;
	return QC_ERR_NONE;
}

int	CBaseFFParser::GetAudioFormat (int nID, QC_AUDIO_FORMAT ** ppAudioFmt)
{
	if (ppAudioFmt == NULL)
		return QC_ERR_ARG;
	*ppAudioFmt = m_pFmtAudio;
	return QC_ERR_NONE;
}

int	CBaseFFParser::GetVideoFormat (int nID, QC_VIDEO_FORMAT ** ppVideoFmt)
{
	if (ppVideoFmt == NULL)
		return QC_ERR_ARG;
	*ppVideoFmt = m_pFmtVideo;
	return QC_ERR_NONE;
}

int	CBaseFFParser::GetSubttFormat (int nID, QC_SUBTT_FORMAT ** ppSubttFmt)
{
	if (ppSubttFmt == NULL)
		return QC_ERR_ARG;
	*ppSubttFmt = m_pFmtSubtt;
	return QC_ERR_NONE;
}

bool CBaseFFParser::IsEOS (void)
{
	return m_bEOS;
}

bool CBaseFFParser::IsLive (void)
{
	return m_bLive;
}

int	CBaseFFParser::EnableSubtt (bool bEnable)
{
	m_bEnableSubtt = bEnable;
	return QC_ERR_NONE;
}

int CBaseFFParser::Run (void)
{
	return QC_ERR_NONE;
}

int CBaseFFParser::Pause (void)
{
	return QC_ERR_NONE;
}

int CBaseFFParser::Stop (void)
{
	return QC_ERR_NONE;
}

int CBaseFFParser::Read (QC_DATA_BUFF * pBuff)
{
	return QC_ERR_IMPLEMENT;
}

int	CBaseFFParser::Process (unsigned char * pBuff, int nSize)
{
	return 0;
}

int	CBaseFFParser::CanSeek (void)
{
	return 1;
}

long long CBaseFFParser::GetPos (void)
{
	return QC_ERR_IMPLEMENT;
}

long long CBaseFFParser::SetPos (long long llPos)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseFFParser::GetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseFFParser::SetParam (int nID, void * pParam)
{
	return QC_ERR_IMPLEMENT;
}

int CBaseFFParser::DeleteFormat (QCMediaType nType)
{
	switch (nType)
	{
	case QC_MEDIA_Source:
		if (m_pFmtStream == NULL)
			break;
		delete m_pFmtStream;
		m_pFmtStream = NULL;
		break;
	case QC_MEDIA_Video:
		if (m_pFmtVideo == NULL)
			break;
		QC_DEL_A (m_pFmtVideo->pHeadData);
		QC_DEL_P (m_pFmtVideo);
		break;
	case QC_MEDIA_Audio:
		if (m_pFmtAudio == NULL)
			break;
		QC_DEL_A (m_pFmtAudio->pHeadData);
		QC_DEL_P (m_pFmtAudio);
		break;
	case QC_MEDIA_Subtt:
		if (m_pFmtSubtt == NULL)
			break;
		QC_DEL_A (m_pFmtAudio->pHeadData);
		QC_DEL_P (m_pFmtSubtt);
		break;

	default:
		break;
	}
	return QC_ERR_NONE;
}
/*
int CBaseFFParser::ConvertAVCHead(QCAVCDecoderSpecificInfo* AVCDecoderSpecificInfo, unsigned char * pInBuffer, int nInSzie)
{
	if (pInBuffer == NULL)
		return QC_ERR_ARG;
	if(AVCDecoderSpecificInfo == NULL || AVCDecoderSpecificInfo->iData == NULL 
		|| AVCDecoderSpecificInfo->iPpsData == NULL || AVCDecoderSpecificInfo->iSpsData == NULL) 
			return QC_ERR_ARG; 
	if (nInSzie < 12)
		return QC_ERR_UNSUPPORT;

	m_nNALLengthSize =  (pInBuffer[4]&0x03)+1;
	unsigned int nNalWord = 0x01000000;
	if (m_nNALLengthSize == 3)
		nNalWord = 0X010000;

	int nNalLen = m_nNALLengthSize;
	if (m_nNALLengthSize < 3)	
		nNalLen = 4;

	unsigned int	HeadSize = 0;
	int				i = 0;
	int				nSPSNum = pInBuffer[5]&0x1f;
	int				nSPSSize = 0;
	unsigned char * pBuffer = pInBuffer + 6;
	unsigned char * pOutBuffer = AVCDecoderSpecificInfo->iData;

	for (i = 0; i< nSPSNum; i++)
	{
		nSPSSize = 0;
		int nSPSLength = (pBuffer[0]<<8)| pBuffer[1];
		pBuffer += 2;

		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;
		memcpy(AVCDecoderSpecificInfo->iSpsData, &nNalWord, nNalLen);
		nSPSSize += nNalLen;

		if(nSPSLength > (nInSzie - (pBuffer - pInBuffer))){
			return QC_ERR_UNSUPPORT;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nSPSLength);
        memcpy(AVCDecoderSpecificInfo->iSpsData + nSPSSize, pBuffer, nSPSLength);
		HeadSize += nSPSLength;
		pBuffer += nSPSLength;
		nSPSSize += nSPSLength;
	}
    AVCDecoderSpecificInfo->iSpsSize = nSPSSize;

	int nPPSNum = *pBuffer++;
	int nPPSSize = 0;
	for (i=0; i< nPPSNum; i++)
	{
		int nPPSLength = (pBuffer[0]<<8) | pBuffer[1];
		pBuffer += 2;
		
		nPPSSize = 0;
		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;
		memcpy(AVCDecoderSpecificInfo->iPpsData, &nNalWord, nNalLen);
		nPPSSize += nNalLen;
		
		if(nPPSLength > (nInSzie - (pBuffer - pInBuffer))){
			return QC_ERR_UNSUPPORT;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nPPSLength);
        memcpy (AVCDecoderSpecificInfo->iPpsData + nPPSSize, pBuffer, nPPSLength);
		HeadSize += nPPSLength;
		pBuffer += nPPSLength;
		nPPSSize += nPPSLength;
	}
    AVCDecoderSpecificInfo->iPpsSize = nPPSSize;
	AVCDecoderSpecificInfo->iSize = HeadSize;

	return QC_ERR_NONE;
}
*/
int CBaseFFParser::ConvertHEVCHead(unsigned char * pOutBuffer, unsigned int& nOutSize, unsigned char * pInBuffer, int nInSzie)
{
	if (pOutBuffer == NULL || pInBuffer == NULL)
		return QC_ERR_ARG;
	if (nInSzie < 22)
		return QC_ERR_UNSUPPORT;

	unsigned char * pData = pInBuffer;
	m_nNALLengthSize =  (pData[21]&0x03)+1;
	int nNalLen = m_nNALLengthSize;
	if (m_nNALLengthSize < 3)	
		nNalLen = 4;

	unsigned int nNalWord = 0x01000000;
	if (m_nNALLengthSize == 3)
		nNalWord = 0X010000;

	int nHeadSize = 0;
	unsigned char * pBuffer = pOutBuffer;
	int nArrays = pData[22];
	int nNum = 0;;

	pData += 23;
	if(nArrays)
	{
		for(nNum = 0; nNum < nArrays; nNum++)
		{
			unsigned char nal_type = 0;
			nal_type = pData[0]&0x3F;
			pData += 1;
			switch(nal_type)
			{
			case 33://sps
				{
					int nSPSNum = (pData[0] << 8)|pData[1];
					pData += 2;
					for(int i = 0; i < nSPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						int nSPSLength = (pData[0] << 8)|pData[1];
						pData += 2;
						if(nSPSLength > (nInSzie - (pData - pInBuffer))){
							nOutSize = 0;
							return QC_ERR_UNSUPPORT;
						}

						memcpy (pBuffer + nHeadSize, pData, nSPSLength);
						nHeadSize += nSPSLength;
						pData += nSPSLength;
					}
				}
				break;
			case 34://pps
				{
					int nPPSNum = (pData[0] << 8) | pData[1];
					pData += 2;
					for(int i = 0; i < nPPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						int nPPSLength = (pData[0] << 8)| pData[1];
						pData += 2;
						if(nPPSLength > (nInSzie - (pData - pInBuffer))){
							nOutSize = 0;
							return QC_ERR_UNSUPPORT;
						}
						memcpy (pBuffer + nHeadSize, pData, nPPSLength);
						nHeadSize += nPPSLength;
						pData += nPPSLength;
					}
				}
				break;
			case 32: //vps
				{
					int nVPSNum = (pData[0] << 8 )| pData[1] ;
					pData += 2;
					for(int i = 0; i < nVPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						int nVPSLength = (pData[0] << 8 )|pData[1];
						pData += 2;
						if(nVPSLength > (nInSzie - (pData - pInBuffer))){
							nOutSize = 0;
							return QC_ERR_UNSUPPORT;
						}
						memcpy (pBuffer + nHeadSize, pData, nVPSLength);
						nHeadSize += nVPSLength;
						pData += nVPSLength;
					}
				}
				break;
			default://just skip the data block
				{
					int nSKP = (pData[0] << 8 )|pData[1];
					pData += 2;
					for(int i = 0; i < nSKP; i++)
					{
						int nAKPLength = (pData[0] << 8) | pData[1];
						if(nAKPLength > (nInSzie - (pData - pInBuffer))){
							nOutSize = 0;
							return QC_ERR_UNSUPPORT;
						}
						pData += 2;
						pData += nAKPLength;
					}

				}
				break;
			}
		}
	}

	nOutSize = nHeadSize;

	return QC_ERR_NONE;
}

int CBaseFFParser::ConvertAVCFrame(unsigned char * pFrame, int nSize, unsigned int& nFrameLen, int& IsKeyFrame)	
{
	if (m_nNALLengthSize == 0)
		return QC_ERR_UNSUPPORT;

	unsigned char *  pBuffer = pFrame;
	int	 nNalLen = 0;
	int	 nNalType = 0;
	
	nFrameLen = 0;

	int nNalWord = 0x01000000;
	if (m_nNALLengthSize == 3)
		nNalWord = 0X010000;

	if(m_nNALLengthSize < 3) 	
	{
		if(m_nAVCSize < nSize + 512)
		{
			QC_DEL_A (m_pAVCBuffer);
			m_nAVCSize = nSize + 512;
			m_pAVCBuffer = new unsigned char[m_nAVCSize];
		}
	}
	else 
	{
		nFrameLen = nSize;
	}

	int i = 0;
	int leftSize = nSize;

	while (pBuffer - pFrame + m_nNALLengthSize < nSize)
	{
		nNalLen = *pBuffer++;
		for (i = 0; i < (int)m_nNALLengthSize - 1; i++)
		{
			nNalLen = nNalLen << 8;
			nNalLen += *pBuffer++;
		}

		if(nNalType != 1 && nNalType != 5) {
			nNalType = pBuffer[0]&0x0f;
		}

		leftSize -= m_nNALLengthSize;

		if(nNalLen > leftSize || nNalLen < 0)
		{
            nNalLen = leftSize;
            IsKeyFrame |= QCBUFF_FLUSH;
            nNalType = 1;
			//return TTKErrGeneral;
		}

		if (m_nNALLengthSize == 3 || m_nNALLengthSize == 4)
		{
			memcpy ((pBuffer - m_nNALLengthSize), &nNalWord, m_nNALLengthSize);
		}
		else
		{
			memcpy (m_pAVCBuffer + nFrameLen, &nNalWord, 4);
			nFrameLen += 4;
			memcpy (m_pAVCBuffer + nFrameLen, pBuffer, nNalLen);
			nFrameLen += nNalLen;
		}

		leftSize -= nNalLen;
		pBuffer += nNalLen;
	}

	if(nNalType == 5)
		IsKeyFrame = 1;

	return QC_ERR_NONE;
}
