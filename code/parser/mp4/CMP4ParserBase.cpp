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
#include "CMsgMng.h"

#include "UIntReader.h"
#include "ULogFunc.h"

#define KBoxHeaderSize 16

// return 1 when matrix is identity, 0 otherwise
#define IS_MATRIX_IDENT(matrix)            \
    ( (matrix)[0][0] == (1 << 16) &&       \
      (matrix)[1][1] == (1 << 16) &&       \
      (matrix)[2][2] == (1 << 30) &&       \
     !(matrix)[0][1] && !(matrix)[0][2] && \
     !(matrix)[1][0] && !(matrix)[1][2] && \
     !(matrix)[2][0] && !(matrix)[2][1])

enum {
	KTag_ESDescriptor            = 0x03,
	KTag_DecoderConfigDescriptor = 0x04,
	KTag_DecoderSpecificInfo     = 0x05,
	KTag_SLConfigDescriptor 	 = 0x06,
};

static const int KMaxALACFrameSize = 2 * 1024 * 1024;

int CMP4Parser::ReadBoxMoov(long long aBoxPos, long long aBoxLen)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	long long			nBoxLenLeft = aBoxLen;
	long long			nLastPos = aBoxPos;
	long long			nBoxPos = aBoxPos;
	long long			aOffset = 8;
	QCMP4TrackInfo *	pCurrentTrack = NULL;
	unsigned char		version = 0;
	unsigned int		nTimeScale = 0;
	long long			nDuration = 0;
	unsigned int		nBoxSize;
	unsigned int		nBoxId;

	// for debug moov
	char	szBoxID[16];
	int		nMoovStartTime = qcGetSysTime();
	int		nBoxStartTime = qcGetSysTime ();
	memset (szBoxID, 0, sizeof(szBoxID));
	QCLOGI("MOOVDebug  box: moov    % 8lld     % 8lld", aBoxPos, aBoxLen);

	m_bBuildSample = true;
	while(nBoxLenLeft > 0)	
	{
		if(nBoxLenLeft < 8) 
			break;

		if (m_pBaseInst->m_bForceClose == true)
			return QC_ERR_FAILED;

		nBoxSize = m_pIOReader->ReadUint32BE(nBoxPos);
		nBoxId = m_pIOReader->ReadUint32BE(nBoxPos + 4);
		memcpy(szBoxID + 8, &nBoxId, 4);
		szBoxID[0] = szBoxID[11]; szBoxID[1] = szBoxID[10]; szBoxID[2] = szBoxID[9]; szBoxID[3] = szBoxID[8];
		if(nBoxSize == 1) 
		{
			if(nBoxLenLeft < 16) {
				break;
			}
			nBoxSize = (unsigned int)m_pIOReader->ReadUint64BE(nBoxPos + 8);
			aOffset = 16;
		} 
		if(nBoxSize < 8)
		{
			QCLOGW ("The box id %s pos = % 8lld, size = % 8d Error!", szBoxID, nBoxPos, nBoxSize);				
			return QC_ERR_UNSUPPORT;
		}

		if(nBoxLenLeft < nBoxSize) 
			break;

		switch(nBoxId)
		{
		case QC_MKBETAG('m','v','h','d'): 
			{
				int nNewOffset = aOffset;
				ReadSourceData (nBoxPos + aOffset, &version, sizeof(unsigned char), QCIO_READ_HEAD);
				if(version == 0) {
					nTimeScale = m_pIOReader->ReadUint32BE(nBoxPos + 12 + aOffset);
					nDuration = m_pIOReader->ReadUint32BE(nBoxPos + 16 + aOffset);
					nNewOffset = aOffset + 20;
				} else if(version == 1)	{
					nTimeScale = m_pIOReader->ReadUint32BE(nBoxPos + 20 + aOffset);
					nDuration = m_pIOReader->ReadUint64BE(nBoxPos + 24 + aOffset);
					nNewOffset = aOffset + 32;
				}

				if (nTimeScale)
				{
					m_nDuration = (unsigned int)((nDuration * 1000) / nTimeScale);
					m_nTimeScale = nTimeScale;
				}

				// movie display matrix, store it in main context and use it later on
				nNewOffset += 16;
				for (int i = 0; i < 3; i++) {
					movie_display_matrix[i][0] = m_pIOReader->ReadUint32BE(nBoxPos + nNewOffset); // 16.16 fixed point
					nNewOffset += 4;
					movie_display_matrix[i][1] = m_pIOReader->ReadUint32BE(nBoxPos + nNewOffset); // 16.16 fixed point
					nNewOffset += 4;
					movie_display_matrix[i][2] = m_pIOReader->ReadUint32BE(nBoxPos + nNewOffset); //  2.30 fixed point
					nNewOffset += 4;
				}

				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case QC_MKBETAG('t','r','a','k'):
			{
				UpdateTrackInfo();

				pCurrentTrack = new QCMP4TrackInfo;
				memset(pCurrentTrack, 0, sizeof(QCMP4TrackInfo));
				pCurrentTrack->nTrackId = m_nTrackIndex++;

				m_pCurTrackInfo = pCurrentTrack;

				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case QC_MKBETAG('t','k','h','d'):
			{
				ReadSourceData (nBoxPos + aOffset, &version, sizeof(unsigned char), QCIO_READ_HEAD);
				if(version == 0) {
					nDuration = m_pIOReader->ReadUint32BE(nBoxPos + 20 + aOffset);
					pCurrentTrack->iWidth = m_pIOReader->ReadUint32BE(nBoxPos + 76 + aOffset) >> 16;
					pCurrentTrack->iHeight = m_pIOReader->ReadUint32BE(nBoxPos + 80 + aOffset) >> 16;
				} else if(version == 1)	{
					nDuration = m_pIOReader->ReadUint64BE(nBoxPos + 28 + aOffset);
					pCurrentTrack->iWidth = m_pIOReader->ReadUint32BE(nBoxPos + 84 + aOffset) >> 16;
					pCurrentTrack->iHeight = m_pIOReader->ReadUint32BE(nBoxPos + 88 + aOffset) >> 16;
				}

				pCurrentTrack->iDuration = nDuration;
				pCurrentTrack->iScale = nTimeScale;

				int i, j, e;
				int display_matrix[3][3];
				int res_display_matrix[3][3] = { { 0 } };
				int	nOffset = 48;
				//read in the display matrix (outlined in ISO 14496-12, Section 6.2.2)
				// they're kept in fixed point format through all calculations
				// save u,v,z to store the whole matrix in the AV_PKT_DATA_DISPLAYMATRIX
				// side data, but the scale factor is not needed to calculate aspect ratio
				for (i = 0; i < 3; i++) {
					display_matrix[i][0] = m_pIOReader->ReadUint32BE(nBoxPos + nOffset);   // 16.16 fixed point
					nOffset += 4;
					display_matrix[i][1] = m_pIOReader->ReadUint32BE(nBoxPos + nOffset);   // 16.16 fixed point
					nOffset += 4;
					display_matrix[i][2] = m_pIOReader->ReadUint32BE(nBoxPos + nOffset);   //  2.30 fixed point
					nOffset += 4;
				}
				for (i = 0; i < 3; i++) {
					const int sh[3] = { 16, 16, 30 };
					for (j = 0; j < 3; j++) {
						for (e = 0; e < 3; e++) {
							res_display_matrix[i][j] +=
								((long long)display_matrix[i][e] *
								movie_display_matrix[e][j]) >> sh[e];
						}
					}
				}
				// save the matrix when it is not the default identity
				if (!IS_MATRIX_IDENT(res_display_matrix)) {
					double rotate;
					rotate = av_display_rotation_get((int*)res_display_matrix);
					//if (!isnan(rotate)) 
					{
						rotate = -rotate;
						if (rotate < 0) // for backward compatibility
							rotate += 360;
					}
					if (m_pBaseInst != NULL)
						m_pBaseInst->m_pSetting->g_qcs_nVideoRotateAngle = rotate;
					if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
						m_pBaseInst->m_pMsgMng->Notify(QC_MSG_SNKV_ROTATE, (int)rotate, 0);
				}

				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case QC_MKBETAG('m','d','i','a'):
			{
				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case QC_MKBETAG('m','d','h','d'):
			{
				ReadSourceData (nBoxPos + aOffset, &version, sizeof(unsigned char), QCIO_READ_HEAD);
				unsigned int nScale = 0;
				long long Duration = 0;
				unsigned short lang16;
				unsigned char	 lang[2];

				if(version == 0)
				{
					nScale = m_pIOReader->ReadUint32BE(nBoxPos + 20 + aOffset - 8);
					Duration = m_pIOReader->ReadUint32BE(nBoxPos + 24 + aOffset - 8);
					lang16 = m_pIOReader->ReadUint16BE(nBoxPos + 28 + aOffset - 8);
				}
				else
				{
					nScale = m_pIOReader->ReadUint32BE(nBoxPos + 28 + aOffset - 8);
					Duration = m_pIOReader->ReadUint32BE(nBoxPos + 32 + aOffset - 8);
					lang16 = m_pIOReader->ReadUint16BE(nBoxPos + 40 + aOffset - 8);
				}

				if(nScale && Duration != 0xffffffff) 
					pCurrentTrack->iDuration = Duration * 1000/nScale;

				if(nScale)
					pCurrentTrack->iScale = nScale;
				
				lang[0] = lang16 >> 8; lang[1] = lang16 & 0xff; 
				pCurrentTrack->iLang_code[0] = ((lang[0] >> 2) & 0x1f) + 0x60;
				pCurrentTrack->iLang_code[1] = ((lang[0] & 0x3) << 3 | (lang[1] >> 5)) + 0x60;
				pCurrentTrack->iLang_code[2] = (lang[1] & 0x1f) + 0x60;
				pCurrentTrack->iLang_code[3] = '\0';

				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case QC_MKBETAG('h','d','l','r'):
			{
				int nStreamID = -1;
				unsigned int nSubType = m_pIOReader->ReadUint32BE(nBoxPos + aOffset + 8);

				if (nSubType == QC_MKBETAG('s','o','u','n'))
				{ 
					if(pCurrentTrack->iAudio == 0) {
						nStreamID = EMediaStreamIdAudioL + m_nStrmAudioCount;
						pCurrentTrack->iAudio = 1;
						m_nStrmAudioCount++;
					} else {
						nStreamID = pCurrentTrack->iStreamID;
					}
				} else if (nSubType == QC_MKBETAG('v','i','d','e'))	{ 
					if(pCurrentTrack->iStreamID > 0) {
						nStreamID = pCurrentTrack->iStreamID;
					}else {
						if(m_nStrmVideoCount == 0) {
							nStreamID = EMediaStreamIdVideo + m_nStrmVideoCount;
							m_nStrmVideoCount++;
						}else {
							pCurrentTrack->iErrorTrackInfo = 1;
						}
					}
				} 

				if(nStreamID >= 0) {
					pCurrentTrack->iStreamID = nStreamID;
				}
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case QC_MKBETAG('m','i','n','f'):
			{
				if(pCurrentTrack->iErrorTrackInfo == 1) {
					nBoxPos += nBoxSize;
					nBoxLenLeft -= nBoxSize;
				}else {
					nBoxPos += aOffset;
					nBoxLenLeft -= aOffset;
				}
			}
			break;

		case QC_MKBETAG('s','t','b','l'):
			{
				nBoxPos += aOffset;
				nBoxLenLeft -= aOffset;
			}
			break;

		case QC_MKBETAG('s','t','s','d'):
			{
				ReadBoxStsd(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STSD;
			}
			break;

		case QC_MKBETAG('c','t','t','s'):
			{
				ReadBoxCtts(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_CTTS;
			}
			break;

		case QC_MKBETAG('s','t','t','s'):
			{
				ReadBoxStts(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STTS;
			}
			break;

		case QC_MKBETAG('s','t','s','s'):
			{
				ReadBoxStss(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STSS;
			}
			break;

		case QC_MKBETAG('s','t','s','z'):
			{				
				ReadBoxStsz(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STSZ;
			}
			break;

		case QC_MKBETAG('s','t','s','c'):
			{
				ReadBoxStsc(nBoxPos + aOffset, (int)(nBoxSize - aOffset));				
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STSC;
			}
			break;

		case QC_MKBETAG('s','t','c','o'):
			{
				ReadBoxStco(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STCO;
			}
			break;

		case QC_MKBETAG('c','o','6','4'):
			{
				ReadBoxCo64(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				pCurrentTrack->iReadBoxFlag |= QC_BOX_FLAG_STCO;
			}
			break;

		case QC_MKBETAG('e', 'd', 't', 's'):
			{
				ReadBoxEdts(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		case QC_MKBETAG('m', 'v', 'e', 'x'):
		{
			ReadBoxMvex(nBoxPos + aOffset, (int)(nBoxSize - aOffset));
			nBoxPos += nBoxSize;
			nBoxLenLeft -= nBoxSize;
		}
			break;

		case QC_MKBETAG('m', 'e', 't', 'a'):
			{
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
			}
			break;

		default:
			{
				nBoxPos += nBoxSize;
				nBoxLenLeft -= nBoxSize;
				break;
			}
		}
		// for debug moov
		if (nBoxPos > nBoxSize)
		{
			QCLOGI("MOOVDebug  box: %s    % 8lld     % 8d  UsedTime: Box = % 8d   Moov = % 8d  % 8lld", szBoxID, nBoxPos - nBoxSize, nBoxSize, qcGetSysTime() - nBoxStartTime, qcGetSysTime() - nMoovStartTime, m_fIO->GetDownPos(m_fIO->hIO));
		}
		else
		{
			QCLOGI("MOOVDebug  box: %s    % 8lld     % 8d  UsedTime: Box = % 8d   Moov = % 8d  % 8lld", szBoxID, nBoxPos, nBoxSize, qcGetSysTime() - nBoxStartTime, qcGetSysTime() - nMoovStartTime, m_fIO->GetDownPos(m_fIO->hIO));
		}
		nBoxStartTime = qcGetSysTime();
	}

	UpdateTrackInfo();

	if (m_nStrmAudioCount > 0 || m_nStrmVideoCount > 0)
	{
		if (!m_bBuildSample && m_hMoovThread == NULL && !m_bOpenCache)
		{
			int nID = 0;
			qcThreadCreate(&m_hMoovThread, &nID, MoovWorkProc, this, 0);
		}
		return QC_ERR_NONE;
	}

	QCLOGW ("The box id %s pos = % 8lld, size = % 8d Error!", szBoxID, nBoxPos, nBoxSize);		

	return QC_ERR_UNSUPPORT;
}

int CMP4Parser::UpdateTrackInfo()
{
	if(m_pCurTrackInfo == NULL)
		return QC_ERR_NONE;

	if(m_pCurTrackInfo->iSampleInfoTab != NULL && m_pCurTrackInfo->iSampleCount > 0)
		return QC_ERR_NONE; 

	if(m_pCurTrackInfo->iErrorTrackInfo || m_pCurTrackInfo->iCodecType == 0 || (m_pCurTrackInfo->iReadBoxFlag&0x1f) != 0x1f) 
	{
		if(m_pCurTrackInfo->iStreamID >= 0) 
		{
			if(m_pCurTrackInfo->iAudio && m_pCurTrackInfo->iStreamID >= EMediaStreamIdAudioL)		
				 m_nStrmAudioCount--;
			else if (m_pCurTrackInfo->iStreamID >= EMediaStreamIdVideo)
				 m_nStrmVideoCount--;
		}
		RemoveTrackInfo(m_pCurTrackInfo);
		m_pCurTrackInfo = NULL;
		return QC_ERR_UNSUPPORT;
	}
	if (m_pCurTrackInfo->iAudio == 0)
	{
		if (m_pCurTrackInfo->iSampleCount)
			m_pCurTrackInfo->iFrameTime = (unsigned int)((m_nDuration * 1000) / m_pCurTrackInfo->iSampleCount);
		m_pVideoTrackInfo = m_pCurTrackInfo;
	}
	else
	{
		int nSampleRate = 44100;
		if (m_pCurTrackInfo->iM4AWaveFormat != NULL)
			nSampleRate = m_pCurTrackInfo->iM4AWaveFormat->iSampleRate;
		m_pCurTrackInfo->iFrameTime = (unsigned int)((1024 * 1000000) / nSampleRate);
		m_lstAudioTrackInfo.AddTail(m_pCurTrackInfo);
		if (m_pAudioTrackInfo == NULL)
			m_pAudioTrackInfo = m_pCurTrackInfo;
	}

	BuildSampleTab(m_pCurTrackInfo);

	return QC_ERR_NONE;
}

int CMP4Parser::RemoveTrackInfo(QCMP4TrackInfo*	pTrackInfo)
{
	if(pTrackInfo == NULL)
		return QC_ERR_NONE;

	CAutoLock lock(&m_mtSample);
	if (pTrackInfo->iMP4DecoderSpecificInfo != NULL)
	{
		QC_FREE_P(pTrackInfo->iMP4DecoderSpecificInfo->iData);
		QC_FREE_P(pTrackInfo->iMP4DecoderSpecificInfo);
	}

	if(pTrackInfo->iAVCDecoderSpecificInfo != NULL)
	{
		QC_FREE_P(pTrackInfo->iAVCDecoderSpecificInfo->iData);
		QC_FREE_P(pTrackInfo->iAVCDecoderSpecificInfo->iConfigData);
		QC_FREE_P(pTrackInfo->iAVCDecoderSpecificInfo->iSpsData);
		QC_FREE_P(pTrackInfo->iAVCDecoderSpecificInfo->iPpsData);
		QC_FREE_P(pTrackInfo->iAVCDecoderSpecificInfo);
	}

	QC_FREE_P(pTrackInfo->iM4AWaveFormat);
	QC_DEL_A (pTrackInfo->iComTimeSampleTab);
	QC_DEL_A (pTrackInfo->iTimeToSampleTab);
	QC_DEL_A (pTrackInfo->iVariableSampleSizeTab);
	QC_DEL_A (pTrackInfo->iChunkOffsetTab);
	QC_DEL_A (pTrackInfo->iChunkToSampleTab);
	QC_DEL_A (pTrackInfo->iKeyFrameSampleTab);
	QC_DEL_A (pTrackInfo->iSampleInfoTab);	

	QC_DEL_P (pTrackInfo);

	return QC_ERR_NONE; 
}

int CMP4Parser::BuildSampleTab(QCMP4TrackInfo*	pTrackInfo)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	CAutoLock lock(&m_mtSample);
	TTSampleInfo*	iSampleInfoTab = pTrackInfo->iSampleInfoTab;
	if (iSampleInfoTab == NULL)
	{
		iSampleInfoTab = new TTSampleInfo[pTrackInfo->iSampleCount + 1];
		memset(iSampleInfoTab, 0, sizeof(TTSampleInfo)*(pTrackInfo->iSampleCount + 1));
		iSampleInfoTab[pTrackInfo->iSampleCount].iSampleIdx = 0x7fffffff;
		pTrackInfo->iSampleInfoTab = iSampleInfoTab;
		int nIndex = 0;
		while (nIndex < m_nPreLoadSamples)
		{
			iSampleInfoTab[nIndex].iSampleIdx = nIndex + 1;
			iSampleInfoTab[nIndex].iSampleTimeStamp = pTrackInfo->iFrameTime * nIndex;
			nIndex++;
			if (nIndex >= pTrackInfo->iSampleCount)
				break;
		}
		while (nIndex < pTrackInfo->iSampleCount)
		{
			iSampleInfoTab[nIndex].iSampleIdx = nIndex + 1;
			iSampleInfoTab[nIndex].iSampleTimeStamp = QC_MAX_NUM64_S;
			nIndex++;
		}
	}

	long long*			iChunkOffsetTab = pTrackInfo->iChunkOffsetTab;
	TTChunkToSample*	iChunkToSampleTab = pTrackInfo->iChunkToSampleTab;
	int*				iVariableSampleSizeTab = pTrackInfo->iVariableSampleSizeTab;
	int*				iKeyFrameSampleTab = pTrackInfo->iKeyFrameSampleTab;
	int					iConstantSampleSize = pTrackInfo->iConstantSampleSize;
	TTimeToSample*		iTimeToSampleTab = pTrackInfo->iTimeToSampleTab;
	
	int			nIdx = 0;
	int*		nKeySampleIdx = NULL;
	int			nOldFirstChunkIdx;
	int			nOldSampleIdx ;
	int			nNewFirstChunkIdx;
	int			IsVideo_I_Frame = 0;
	long long	nChunkOffset;
	int			nChunkCount = 0;
	int			entrySizeFront = 0;
	int			k, m;
	bool		bBreak = false;

	if(iKeyFrameSampleTab != NULL)
		nKeySampleIdx = iKeyFrameSampleTab;

	iChunkToSampleTab[pTrackInfo->iChunkToSampleEntryNum].iFirstChunk = pTrackInfo->iChunkOffsetEntryNum + 1;

	// Fill sample start pos and size
	for (k = 1; k <= pTrackInfo->iChunkToSampleEntryNum; k++)
	{
		if (iChunkToSampleTab[k].iFirstChunk == QC_MAX_NUM64)
			break;

		// Only access sample_to_chunk[] if new data is required. 
		nOldFirstChunkIdx = (int)iChunkToSampleTab[k-1].iFirstChunk;
		nOldSampleIdx = iChunkToSampleTab[k-1].iSampleNum;
		nNewFirstChunkIdx = (int)iChunkToSampleTab[k].iFirstChunk;

		for(m = nOldFirstChunkIdx; m < nNewFirstChunkIdx; m++)
		{
			if (m_pBaseInst->m_bForceClose == true)
				return QC_ERR_FAILED;

			nChunkOffset = iChunkOffsetTab[nChunkCount];
			if (nChunkOffset == QC_MAX_NUM64)
			{
				bBreak = true;
				break;
			}

			for(int h = 0; h < nOldSampleIdx; h++)
			{
				IsVideo_I_Frame = 0;
				if(iKeyFrameSampleTab != NULL)
				{   
					if(nIdx == *nKeySampleIdx)
					{
						IsVideo_I_Frame = QCBUFF_KEY_FRAME;
						nKeySampleIdx++;
					}
				}
				if(nIdx >= pTrackInfo->iSampleCount) 
					break;
				if (!iConstantSampleSize && iVariableSampleSizeTab[nIdx] == QC_MAX_NUM32)
				{
					bBreak = true;
					break;
				}

				iSampleInfoTab[nIdx].iFlag |= IsVideo_I_Frame;
				if(iConstantSampleSize) 
				{					
					iSampleInfoTab[nIdx].iSampleEntrySize = iConstantSampleSize;					
					iSampleInfoTab[nIdx].iSampleFileOffset = nChunkOffset + entrySizeFront + m_nMP4KeySize;
					entrySizeFront += iConstantSampleSize;
				}
				else 
				{
					iSampleInfoTab[nIdx].iSampleEntrySize = iVariableSampleSizeTab[nIdx];
					iSampleInfoTab[nIdx].iSampleFileOffset = nChunkOffset + entrySizeFront + +m_nMP4KeySize;
					entrySizeFront += iVariableSampleSizeTab[nIdx];
				}
				nIdx++;
			}
			if (bBreak)
				break;

			nChunkCount++;
			entrySizeFront=0;
			if (nChunkCount >= pTrackInfo->iChunkOffsetEntryNum + 1)
			{
				bBreak = true;
				break;
			}
		}
		if (bBreak)
			break;
	}

	// File sample time
	int			nCurIndex = 0;
	int			nCurCom = 0;
	long long	nTimeStamp = 0;
	int			nScale = pTrackInfo->iScale;
	
	if (nScale == 0)
	{
		nScale = 1000;
	}
	else
	{
		if ((pTrackInfo->iEmptyDuration || pTrackInfo->iStartTime))
		{
			long long llStartTime = 0;
			if (pTrackInfo->iStartTime)
				llStartTime = pTrackInfo->iStartTime * 1000 / nScale;
			pTrackInfo->iTimeOffset = llStartTime - pTrackInfo->iEmptyDuration;

			if (pTrackInfo->iAudio)
			{
				QCLOGI("The EDITLIST INFO Audio Dur = %lld, Start = %lld, Scale = %d, OffsetTime = %lld",
					pTrackInfo->iEmptyDuration, llStartTime, nScale, pTrackInfo->iTimeOffset);
			}
			else
			{
				QCLOGI("The EDITLIST INFO Video Dur = %lld, Start = %lld, Scale = %d, OffsetTime = %lld",
					pTrackInfo->iEmptyDuration, llStartTime, nScale, pTrackInfo->iTimeOffset);
			}
		}
	}
	
	nIdx = 0;
	bBreak = false;
	long long	nSampleDelta = 0;
	int			nComTime = 0;
	
	for (k = 0; k < pTrackInfo->iTimeToSampleEntryNum; k++)
	{
		if (iTimeToSampleTab[k].iSampleCount == QC_MAX_NUM32)
			break;

		int nCount = iTimeToSampleTab[k].iSampleCount;
		nSampleDelta = iTimeToSampleTab[k].iSampleDelta;
		nComTime = 0;
		for(m = 0; m < nCount; m++)
		{
			if(nIdx < pTrackInfo->iSampleCount)
			{
				if (pTrackInfo->iComTimeSampleEntryNum > 0)
				{
					if (pTrackInfo->iComTimeSampleTab[nCurCom].iSampleCount == QC_MAX_NUM32)
					{
						bBreak = true;
						break;
					}
					nComTime = GetCompositionTimeOffset(pTrackInfo, nIdx, nCurIndex, nCurCom);
				}
				
				iSampleInfoTab[nIdx].iSampleTimeStamp = (nTimeStamp + nComTime) * 1000 / nScale - pTrackInfo->iTimeOffset;
				if (iSampleInfoTab[nIdx].iSampleTimeStamp < 0)
					iSampleInfoTab[nIdx].iSampleTimeStamp = 0;
			}
			if (bBreak)
				break;

			nTimeStamp += nSampleDelta;
			nIdx++;
		}		
	}
	
	if (pTrackInfo->iDuration == 0 && pTrackInfo->iSampleCount > 0)
		pTrackInfo->iDuration = iSampleInfoTab[pTrackInfo->iSampleCount - 1].iSampleTimeStamp + nSampleDelta * 1000/nScale;

	// Get the total bitrate number
	int			nBitrate = 0;
	long long	nDuration = m_nDuration;
	if (pTrackInfo->iDuration)
		nDuration = pTrackInfo->iDuration;
	if (nDuration == 0)
	{
		if (pTrackInfo->iSampleCount > 0)
		{
			nDuration = (pTrackInfo->iSampleInfoTab[1].iSampleTimeStamp -
						 pTrackInfo->iSampleInfoTab[0].iSampleTimeStamp)*pTrackInfo->iSampleCount;
		}
	}
	if (nDuration)
		nBitrate = (int)(pTrackInfo->iTotalSize * 8 * 1000 / nDuration);
	m_nTotalBitrate += nBitrate;

	if (pTrackInfo->iSampleCount > 0)
	{
		if (pTrackInfo->iSampleInfoTab[pTrackInfo->iSampleCount - 1].iSampleTimeStamp != QC_MAX_NUM64_S)
		{
			nDuration = pTrackInfo->iSampleInfoTab[pTrackInfo->iSampleCount - 1].iSampleTimeStamp -
				pTrackInfo->iSampleInfoTab[0].iSampleTimeStamp;
			if (pTrackInfo->iDuration == nDuration && pTrackInfo->iSampleInfoTab[0].iSampleTimeStamp > 0)
			{
				pTrackInfo->iDuration += pTrackInfo->iSampleInfoTab[0].iSampleTimeStamp;
				if (m_nStrmVideoCount > 0 && m_pVideoTrackInfo != NULL)
				{
					m_nStrmVideoPlay = 0;
					if (m_pVideoTrackInfo->iDuration > 0)
						m_llDuration = m_pVideoTrackInfo->iDuration;
				}
				if (m_nStrmAudioCount > 0 && m_pAudioTrackInfo != NULL)
				{
					m_nStrmAudioPlay = 0;
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
				if ((m_pCurAudioInfo != NULL || m_pCurVideoInfo != NULL) && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify(QC_MSG_PLAY_DURATION, 0, m_llDuration);
			}
		}
	}

	return QC_ERR_NONE;
}

int CMP4Parser::GetCompositionTimeOffset(QCMP4TrackInfo* pTrackInfo, int aIndex, int& aCurIndex, int& aCurCom)
{
	TCompositionTimeSample*	iComTimeSampleTab = pTrackInfo->iComTimeSampleTab;
	int						iComTimeSampleEntryNum = pTrackInfo->iComTimeSampleEntryNum;

	if (iComTimeSampleEntryNum == 0 || iComTimeSampleTab == NULL) 
        return 0;

    while (aCurCom < iComTimeSampleEntryNum) 
	{
		int sampleCount = iComTimeSampleTab[aCurCom].iSampleCount;
		if (sampleCount > 1)
			sampleCount = sampleCount;
        if (aIndex < aCurIndex + sampleCount) 
			return iComTimeSampleTab[aCurCom].iSampleOffset;

        aCurIndex += sampleCount;
        ++aCurCom;
    }
	return 0;
}

int CMP4Parser::ReadBoxStco(long long aBoxPos, unsigned int aBoxLen, int nBits)
{
	int i = 0;
	int nEntryNum = 0;
	QCLOG_CHECK_FUNC(&nEntryNum, m_pBaseInst, 0);
	nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);
	//audio
	if(m_pCurTrackInfo->iAudio && nEntryNum > 0)
	{
		int nEntrySize = (int)(m_pCurTrackInfo->iTotalSize / nEntryNum);
		int nMaxEntrySize = KMaxALACFrameSize;
		if ((nEntryNum <= 0))// || (nEntrySize > nMaxEntrySize))
		{
			m_pCurTrackInfo->iErrorTrackInfo = 1;
			return QC_ERR_UNSUPPORT;
		}
	}
	//sample to chunkoffset
	long long* nChunkOffsetTab = new long long[nEntryNum+1];
	memset(nChunkOffsetTab, 0xFF, (nEntryNum + 1) * 8);
	m_pCurTrackInfo->iChunkOffsetTab = nChunkOffsetTab;
	m_pCurTrackInfo->iChunkOffsetEntryNum = nEntryNum;
	aBoxPos += 8;

	// get the preload samples
	int nPreLoad = m_nPreLoadSamples;
	int nNeedSampleCount = 0;
	if (m_pCurTrackInfo->iChunkToSampleEntryNum > 0)
	{
		for (i = 1; i <= m_pCurTrackInfo->iChunkToSampleEntryNum; i++)
		{
			nNeedSampleCount += (m_pCurTrackInfo->iChunkToSampleTab[i].iFirstChunk -
							 m_pCurTrackInfo->iChunkToSampleTab[i - 1].iFirstChunk) * m_pCurTrackInfo->iChunkToSampleTab[i - 1].iSampleNum;
			if (nNeedSampleCount > nPreLoad)
			{
				nPreLoad = i;
				break;
			}
		}
	}

	int  nStepSize = 4;
	if (nBits != 32)
		nStepSize = 8;
	long long llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	for (i = 0; i < nEntryNum; i++)
	{
		//if ((nEntryNum - i) * nStepSize > m_nBoxSkipSize && i > nPreLoad && llDownPos < aBoxPos + nStepSize)
		if (i > nPreLoad && llDownPos < aBoxPos + nStepSize)
			break;

		if (nBits == 32)
		{
			nChunkOffsetTab[i] = (long long)m_pIOReader->ReadUint32BE(aBoxPos);
			aBoxPos += 4;
		}
		else
		{
			nChunkOffsetTab[i] = (long long)m_pIOReader->ReadUint64BE(aBoxPos);
			aBoxPos += 8;
		}
		if (nChunkOffsetTab[i] == 0) // for pd didn't finish
			break;
        if (m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
//		llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	}

	QCLOGI("Read entry = % 8d, total % 8d,  downpos = % 8lld   % 8lld    % 8lld", i, nEntryNum, llDownPos, aBoxPos, m_fIO->GetDownPos(m_fIO->hIO));
	if (i < nEntryNum)
	{
		int nRC = 0;
		if (nBits == 32)
		{
			m_pCurTrackInfo->lSTCOStartPos = aBoxPos;
			m_pCurTrackInfo->nSTCOBuffSize = (nEntryNum - i) * 4;
		}
		else
		{
			m_pCurTrackInfo->lCO64StartPos = aBoxPos;
			m_pCurTrackInfo->nCO64BuffSize = (nEntryNum - i) * 8;
		}
		m_bBuildSample = false;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxCo64(long long aBoxPos, unsigned int aBoxLen)
{
	return ReadBoxStco(aBoxPos, aBoxLen, 64);
/*
	int i = 0;
	int nEntryNum = 0;
	QCLOG_CHECK_FUNC(&nEntryNum, m_pBaseInst, 0);
	nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);
	if(m_pCurTrackInfo->iAudio && nEntryNum > 0)
	{
		int nEntrySize = (int)(m_pCurTrackInfo->iTotalSize / nEntryNum);
		int nMaxEntrySize = KMaxALACFrameSize;
		if ((nEntryNum <= 0) || (nEntrySize > nMaxEntrySize))
		{
			m_pCurTrackInfo->iErrorTrackInfo = 1;
			return QC_ERR_UNSUPPORT;
		}
	}

	//sample to chunkoffset
	long long* nChunkOffsetTab = new long long[nEntryNum+1];
	memset(nChunkOffsetTab, 0XFF, (nEntryNum + 1) * 8);
	m_pCurTrackInfo->iChunkOffsetTab = nChunkOffsetTab;
	m_pCurTrackInfo->iChunkOffsetEntryNum = nEntryNum;
	aBoxPos += 8;

	long long llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	for (i = 1; i <= nEntryNum; i++)
	{
		if ((nEntryNum - i) * 8 > m_nBoxSkipSize && i > 50)
		{
//			if (llDownPos < aBoxPos + 8192)
				break;
		}

		nChunkOffsetTab[i] = m_pIOReader->ReadUint64BE(aBoxPos);
		aBoxPos += 8;

		llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	}

#ifdef QC_BOX_MOOV_THREAD
	if (i < nEntryNum + 1)
	{
		int nRC = CreateMoovIO(QC_MKBETAG('c', 'o', '6', '4'), aBoxPos - i * 8, nEntryNum * 8);
		if (nRC == QC_ERR_NONE)
			return nRC;
	}
#endif // QC_BOX_MOOV_THREAD
	return QC_ERR_NONE;
*/
}

int CMP4Parser::ReadBoxStsc(long long aBoxPos, unsigned int aBoxLen)
{
	int i = 0;
	int nEntryNum = 0;
	QCLOG_CHECK_FUNC(&nEntryNum, m_pBaseInst, 0);
	nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);

	TTChunkToSample* ChunkToSampleTab = new TTChunkToSample[nEntryNum+1];
	memset(ChunkToSampleTab, 0XFF, (nEntryNum + 1) * sizeof(TTChunkToSample));
	m_pCurTrackInfo->iChunkToSampleTab = ChunkToSampleTab;
	m_pCurTrackInfo->iChunkToSampleEntryNum = nEntryNum;
	aBoxPos += 8;

	int			nSampleCount = 0;
	long long	llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	for (i = 0; i < nEntryNum; i++)
	{
//		if ((nEntryNum - i) * 12 > m_nBoxSkipSize && nSampleCount > m_nPreLoadSamples && llDownPos < aBoxPos + 12)
		if (nSampleCount > m_nPreLoadSamples && llDownPos < aBoxPos + 12)
			break;

		ChunkToSampleTab[i].iFirstChunk = m_pIOReader->ReadUint32BE(aBoxPos);
		ChunkToSampleTab[i].iSampleNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);
		if (ChunkToSampleTab[i].iFirstChunk == 0 && ChunkToSampleTab[i].iSampleNum == 0) // for pd didn't finish
			break; 
		aBoxPos += 12;

		if (i > 0)
			nSampleCount += (ChunkToSampleTab[i].iFirstChunk - ChunkToSampleTab[i-1].iFirstChunk) * ChunkToSampleTab[i-1].iSampleNum;
//		llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
        if (m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
	}

	QCLOGI("Read entry = % 8d, total % 8d,  downpos = % 8lld   % 8lld    % 8lld", i, nEntryNum, llDownPos, aBoxPos, m_fIO->GetDownPos(m_fIO->hIO));
	if (i < nEntryNum)
	{
		m_pCurTrackInfo->lSTSCStartPos = aBoxPos;
		m_pCurTrackInfo->nSTSCBuffSize = (nEntryNum - i) * 12;
		m_bBuildSample = false;
		return QC_ERR_NONE;
	}

	ChunkToSampleTab[i].iFirstChunk = ChunkToSampleTab[i-1].iFirstChunk + 1;
	ChunkToSampleTab[i].iSampleNum = 0;
	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxCtts(long long aBoxPos, unsigned int aBoxLen)
{
	int i = 0;
	int nEntryNum = 0;
	QCLOG_CHECK_FUNC(&nEntryNum, m_pBaseInst, 0);

 	nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);
	if (nEntryNum == 0)
		return QC_ERR_NONE;

	TCompositionTimeSample*	ComTimeToSampleTab = new TCompositionTimeSample[nEntryNum];
	memset(ComTimeToSampleTab, 0XFF, nEntryNum * sizeof(TCompositionTimeSample));
 	m_pCurTrackInfo->iComTimeSampleTab = ComTimeToSampleTab;
	m_pCurTrackInfo->iComTimeSampleEntryNum = nEntryNum;
	aBoxPos += 8;

	int			nSampleCount = 0;
	long long	llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
 	for (i = 0; i < nEntryNum; i++)
	{
//		if ((nEntryNum - i) * 8 > m_nBoxSkipSize && nSampleCount > m_nPreLoadSamples && llDownPos < aBoxPos + 8)
		if (nSampleCount > m_nPreLoadSamples && llDownPos < aBoxPos + 8)
			break;

 		ComTimeToSampleTab[i].iSampleCount = m_pIOReader->ReadUint32BE(aBoxPos);
 		ComTimeToSampleTab[i].iSampleOffset = m_pIOReader->ReadUint32BE(aBoxPos + 4);
		if (ComTimeToSampleTab[i].iSampleCount == 0 && ComTimeToSampleTab[i].iSampleOffset == 0)  // for pd didn't finish
			break;
		aBoxPos += 8;

		nSampleCount += ComTimeToSampleTab[i].iSampleCount;
//		llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
        if (m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
 	}

	QCLOGI("Read entry = % 8d, total % 8d,  downpos = % 8lld   % 8lld    % 8lld", i, nEntryNum, llDownPos, aBoxPos, m_fIO->GetDownPos(m_fIO->hIO));
	if (i < nEntryNum)
	{
		m_pCurTrackInfo->lCTTSStartPos = aBoxPos;
		m_pCurTrackInfo->nCTTSBuffSize = (nEntryNum - i) * 8;
		m_bBuildSample = false;
	}
	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxStsz(long long aBoxPos, unsigned int aBoxLen)
{
	int			i = 0;
	int			SampleCount = 0;
	QCLOG_CHECK_FUNC(&SampleCount, m_pBaseInst, 0);

	int			MaxFrameSize = 0;
	long long	TotalSize = 0;
	int			ConstantSampleSize = m_pIOReader->ReadUint32BE(aBoxPos + 4);
				SampleCount = m_pIOReader->ReadUint32BE(aBoxPos + 8);
	int*		VariableSampleSizeTab = NULL;
	
	VariableSampleSizeTab = new int[SampleCount+1];
	memset(VariableSampleSizeTab, 0XFF, (SampleCount + 1) * 4);
	m_pCurTrackInfo->iVariableSampleSizeTab = VariableSampleSizeTab;
	m_pCurTrackInfo->iConstantSampleSize = ConstantSampleSize;
	m_pCurTrackInfo->iSampleCount = SampleCount;
	aBoxPos += 12;

	long long llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	if (ConstantSampleSize == 0)
	{
		for(i = 0; i < SampleCount; i++)
		{
//			if ((SampleCount - i) * 4 > m_nBoxSkipSize && i > m_nPreLoadSamples && llDownPos < aBoxPos + 4)
			if (i > m_nPreLoadSamples && llDownPos < aBoxPos + 4)
				break;

			VariableSampleSizeTab[i] = m_pIOReader->ReadUint32BE(aBoxPos);
			if (VariableSampleSizeTab[i] == 0)  // for pd didn't finish
				break;
			if(MaxFrameSize < VariableSampleSizeTab[i])
				MaxFrameSize = VariableSampleSizeTab[i];
			TotalSize += VariableSampleSizeTab[i];
			aBoxPos+=4;

//			llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
            if (m_pBaseInst->m_bForceClose)
                return QC_ERR_FAILED;
		}
	}
	else
	{
		MaxFrameSize = ConstantSampleSize;
		TotalSize = ConstantSampleSize*SampleCount;
	}
	m_pCurTrackInfo->iMaxFrameSize = MaxFrameSize;
	m_pCurTrackInfo->iTotalSize = TotalSize;

	if(!m_pCurTrackInfo->iAudio) 
	{
		if(m_nNALLengthSize < 3) 
		{
			QC_DEL_A (m_pAVCBuffer);
			m_nAVCSize = MaxFrameSize + 512;
			m_pAVCBuffer = new unsigned char[m_nAVCSize];
		}
	}

	QCLOGI("Read entry = % 8d, total % 8d,  downpos = % 8lld   % 8lld    % 8lld", i, SampleCount, llDownPos, aBoxPos, m_fIO->GetDownPos(m_fIO->hIO));
	if (ConstantSampleSize == 0 && i < SampleCount)
	{
		m_pCurTrackInfo->lSTSZStartPos = aBoxPos;
		m_pCurTrackInfo->nSTSZBuffSize = (SampleCount - i) * 4;
		m_bBuildSample = false;
		return QC_ERR_NONE;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxStts(long long aBoxPos, unsigned int aBoxLen)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

 	int TimeToSampleEntryNum = m_pIOReader->ReadUint32BE(aBoxPos + 4);
 	TTimeToSample* TimeToSampleTab = new TTimeToSample[TimeToSampleEntryNum];
	memset(TimeToSampleTab, 0XFF, TimeToSampleEntryNum * sizeof(TTimeToSample));
	m_pCurTrackInfo->iTimeToSampleEntryNum = TimeToSampleEntryNum;
 	m_pCurTrackInfo->iTimeToSampleTab = TimeToSampleTab;

	aBoxPos += 8;
	long long	llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	int			i = 0;
 	for (i = 0; i < TimeToSampleEntryNum; i++)
 	{
		if (i > m_nPreLoadSamples && llDownPos < aBoxPos + 4)
			break;

 		TimeToSampleTab[i].iSampleCount = m_pIOReader->ReadUint32BE(aBoxPos);
 		TimeToSampleTab[i].iSampleDelta = m_pIOReader->ReadUint32BE(aBoxPos + 4);

		if (TimeToSampleTab[i].iSampleCount == 0 && TimeToSampleTab[i].iSampleDelta == 0)  // for pd didn't finish
			break;

		aBoxPos += 8;
        if(m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
 	}

	if (i < TimeToSampleEntryNum)
	{
		m_pCurTrackInfo->lSTTSStartPos = aBoxPos;
		m_pCurTrackInfo->nSTTSBuffSize = (TimeToSampleEntryNum - i) * 8;
		m_bBuildSample = false;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxStss(long long aBoxPos,unsigned int aBoxLen)
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);

	int		nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos+4);
	int*	nKeyFrameSampleTab = new int[nEntryNum + 1];
	memset(nKeyFrameSampleTab, 0XFF, (nEntryNum + 1) * sizeof(int));
	m_pCurTrackInfo->iKeyFrameSampleTab = nKeyFrameSampleTab;
 	m_pCurTrackInfo->iKeyFrameSampleEntryNum = nEntryNum;

	aBoxPos+=8;

	long long	llDownPos = m_fIO->GetDownPos(m_fIO->hIO);
	int			i = 0;
	for(i = 0; i < nEntryNum; i++)
	{
		if (i > m_nPreLoadSamples && llDownPos < aBoxPos + 4)
			break;

		nKeyFrameSampleTab[i] = m_pIOReader->ReadUint32BE(aBoxPos) - 1;
		if (i > 10 && nKeyFrameSampleTab[i] == 0)  // for pd didn't finish
			break;

		aBoxPos+=4;
        if (m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
	}
	nKeyFrameSampleTab[nEntryNum] = 0x7fffffff;

	if (i < nEntryNum)
	{
		m_pCurTrackInfo->lSTSSStartPos = aBoxPos;
		m_pCurTrackInfo->nSTSSBuffSize = (nEntryNum - i) * 4;
		m_bBuildSample = false;
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxStsd(long long aBoxPos, unsigned int aBoxLen)
{
	aBoxPos += (1 + 3);//version + flags

	int nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos);
	aBoxPos += 4;

	int nErr = QC_ERR_NONE;

	for (int i = 0; i < nEntryNum; i++)
	{
		unsigned int nEntrySize = m_pIOReader->ReadUint32BE(aBoxPos);
		unsigned int nForamtId = m_pIOReader->ReadUint32BE(aBoxPos + 4);
		if (nForamtId == QC_MKBETAG('a','v','c','1'))
		{
			m_pCurTrackInfo->iCodecType = QC_CODEC_ID_H264;
			m_pCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != QC_ERR_NONE)
				m_pCurTrackInfo->iErrorTrackInfo = 1;
		} 
		else if (nForamtId == QC_MKBETAG('h','v','c','1') || nForamtId == QC_MKBETAG('h','e','v','1'))
		{
			m_pCurTrackInfo->iCodecType = QC_CODEC_ID_H265;
			m_pCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != QC_ERR_NONE)
				m_pCurTrackInfo->iErrorTrackInfo = 1;
		} else if (nForamtId == QC_MKBETAG('m','p','4','v'))
		{
			m_pCurTrackInfo->iCodecType = QC_CODEC_ID_MPEG4;
			m_pCurTrackInfo->iFourCC = nForamtId;
			nErr = ReadBoxStsdVide(aBoxPos, nEntrySize);
			if(nErr != QC_ERR_NONE)
				m_pCurTrackInfo->iErrorTrackInfo = 1;
		}
		else if (nForamtId == QC_MKBETAG('m','p','4','a'))
		{
			m_pCurTrackInfo->iCodecType = QC_CODEC_ID_AAC;
			m_pCurTrackInfo->iFourCC = QC_MKBETAG('R','A','W',' ');
			nErr = ReadBoxStsdSoun(aBoxPos, nEntrySize);
			if (nErr != QC_ERR_NONE && m_pCurTrackInfo->iM4AWaveFormat == NULL)
				m_pCurTrackInfo->iErrorTrackInfo = 1;
		}
        else if (nForamtId == QC_MKBETAG('u','l','a','w'))
        {
            m_pCurTrackInfo->iCodecType = QC_CODEC_ID_G711U;
            m_pCurTrackInfo->iFourCC = nForamtId;
            nErr = ReadBoxStsdSoun(aBoxPos, nEntrySize);
            if(nErr != QC_ERR_NONE)
                m_pCurTrackInfo->iErrorTrackInfo = 1;
        }
        else if (nForamtId == QC_MKBETAG('a','l','a','w'))
        {
            m_pCurTrackInfo->iCodecType = QC_CODEC_ID_G711A;
            m_pCurTrackInfo->iFourCC = nForamtId;
            nErr = ReadBoxStsdSoun(aBoxPos, nEntrySize);
            if(nErr != QC_ERR_NONE)
                m_pCurTrackInfo->iErrorTrackInfo = 1;
        }
		else
		{
			m_pCurTrackInfo->iErrorTrackInfo = 1;
			return QC_ERR_UNSUPPORT;
		}
        if (m_pBaseInst->m_bForceClose)
            return QC_ERR_FAILED;
	} 

	return nErr;
}

int CMP4Parser::ReadBoxStsdVide(long long aBoxPos, unsigned int aBoxLen)
{
	aBoxPos += 8;
	aBoxLen -= 8;

	int  width = m_pIOReader->ReadUint16BE(aBoxPos+24);
	int  height = m_pIOReader->ReadUint16BE(aBoxPos+26);

	if(width != 0 && height != 0)
	{
		m_pCurTrackInfo->iWidth = width;
		m_pCurTrackInfo->iHeight  = height;
	}

	if(aBoxLen < 78)
		return QC_ERR_UNSUPPORT;

	aBoxPos += 78;
	aBoxLen -= 78;
    
	int			nHeaderSize = 0;
    long long	BoxLen64 = aBoxLen;
	long long	BoxPos64 = aBoxPos;

	nHeaderSize = LocationBox(BoxPos64, BoxLen64, "pasp", true);
	if (nHeaderSize > 0)
	{
		int  nNum = m_pIOReader->ReadUint32BE(BoxPos64 + 8);
		int  nDen = m_pIOReader->ReadUint32BE(BoxPos64 + 12);
		if (nNum > 0 && nDen > 0)
		{
			m_pCurTrackInfo->iNum = nNum;
			m_pCurTrackInfo->iDen = nDen;
		}
	}

	BoxLen64 = aBoxLen;
	BoxPos64 = aBoxPos;
	if(m_pCurTrackInfo->iCodecType == QC_CODEC_ID_H264 && m_pCurTrackInfo->iFourCC == QC_MKBETAG('a','v','c','1'))
	{
		nHeaderSize = LocationBox(aBoxPos, BoxLen64, "avcC", true);
		if(nHeaderSize < 0)
			return QC_ERR_UNSUPPORT;
        
        aBoxLen = (int)BoxLen64;

		return ReadBoxAvcC(aBoxPos, aBoxLen);
	}

	if(m_pCurTrackInfo->iCodecType == QC_CODEC_ID_H265)
	{
		nHeaderSize = LocationBox(aBoxPos, BoxLen64, "hvcC", true);
		if(nHeaderSize < 0)
			return QC_ERR_UNSUPPORT;
        
        aBoxLen = (int)BoxLen64;

		return ReadBoxHevC(aBoxPos, aBoxLen);
	}
	
	if (aBoxLen > 0)
	{
		nHeaderSize = LocationBox(aBoxPos, BoxLen64, "esds", true);
		if(nHeaderSize < 0)
			return QC_ERR_NONE;
        
        aBoxLen = (int)BoxLen64;

		return ReadBoxEsds(aBoxPos + nHeaderSize, aBoxLen - nHeaderSize);
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxStsdSoun(long long aBoxPos,unsigned int nRemainLen)
{
	if (m_pCurTrackInfo->iM4AWaveFormat == NULL)
		m_pCurTrackInfo->iM4AWaveFormat = (QCM4AWaveFormat*)malloc(sizeof(QCM4AWaveFormat));
	
	m_pCurTrackInfo->iM4AWaveFormat->iChannels = m_pIOReader->ReadUint16BE(aBoxPos + 24);
	m_pCurTrackInfo->iM4AWaveFormat->iSampleBit=m_pIOReader->ReadUint16BE(aBoxPos+26);
	m_pCurTrackInfo->iM4AWaveFormat->iSampleRate = m_pIOReader->ReadUint16BE(aBoxPos + 30);
	if (m_pCurTrackInfo->iM4AWaveFormat->iSampleRate == 0) //error compatible
		m_pCurTrackInfo->iM4AWaveFormat->iSampleRate = m_pIOReader->ReadUint16BE(aBoxPos + 32);
	if(m_pCurTrackInfo->iM4AWaveFormat->iSampleRate == 0)
		m_pCurTrackInfo->iM4AWaveFormat->iSampleRate = m_pCurTrackInfo->iScale;

	aBoxPos += 36;// reserved + data reference index + sound version + reserved 
	nRemainLen -= 36;

	if (nRemainLen > 0)
	{
        long long nRemainLen64 = nRemainLen;
		int nHeaderSize = LocationBox(aBoxPos, nRemainLen64, "esds", true);
		if(nHeaderSize < 0)
			return QC_ERR_NONE;
        nRemainLen = (unsigned int)nRemainLen64;
		return ReadBoxEsds(aBoxPos + nHeaderSize, nRemainLen - nHeaderSize);
	}

	return QC_ERR_NONE;
}

int CMP4Parser::ReadBoxAvcC(long long aBoxPos,unsigned int aBoxLen)
{
	int	AvcCSize=aBoxLen - 8;
	aBoxPos += 8;

	QCAVCDecoderSpecificInfo* AVCDecoderSpecificInfo = (QCAVCDecoderSpecificInfo*)malloc(sizeof(QCAVCDecoderSpecificInfo));
	memset(AVCDecoderSpecificInfo, 0, sizeof(QCAVCDecoderSpecificInfo));

	AVCDecoderSpecificInfo->iData = (unsigned char *)malloc(AvcCSize + 128);
	AVCDecoderSpecificInfo->iSpsData = (unsigned char *)malloc(AvcCSize);
	AVCDecoderSpecificInfo->iPpsData = (unsigned char *)malloc(AvcCSize);

	AVCDecoderSpecificInfo->iConfigSize = AvcCSize;
	AVCDecoderSpecificInfo->iConfigData = (unsigned char *)malloc(AvcCSize);

	unsigned char *	pConfigDate = AVCDecoderSpecificInfo->iConfigData;
	unsigned int	nSize = AvcCSize;
	
	ReadSourceData (aBoxPos, pConfigDate, nSize, QCIO_READ_HEAD);

	int nErr = ConvertAVCHead(AVCDecoderSpecificInfo, pConfigDate, nSize);	

	m_pCurTrackInfo->iAVCDecoderSpecificInfo = AVCDecoderSpecificInfo;
	
	return nErr;
}

int CMP4Parser::ReadBoxHevC(long long aBoxPos,unsigned int aBoxLen)
{
	int AvcCSize=aBoxLen - 8;
	aBoxPos += 8;

	QCAVCDecoderSpecificInfo* AVCDecoderSpecificInfo = (QCAVCDecoderSpecificInfo*)malloc(sizeof(QCAVCDecoderSpecificInfo));
	memset(AVCDecoderSpecificInfo, 0, sizeof(QCAVCDecoderSpecificInfo));

	AVCDecoderSpecificInfo->iData = (unsigned char *)malloc(AvcCSize + 128);

	AVCDecoderSpecificInfo->iConfigSize = AvcCSize;
	AVCDecoderSpecificInfo->iConfigData = (unsigned char *)malloc(AvcCSize + 128);

	unsigned char *	pConfigDate = AVCDecoderSpecificInfo->iConfigData;
	unsigned int	nSize = AvcCSize;
	
	ReadSourceData (aBoxPos, pConfigDate, nSize, QCIO_READ_HEAD);

	int nErr = ConvertHEVCHead(AVCDecoderSpecificInfo->iData, AVCDecoderSpecificInfo->iSize, pConfigDate, nSize);	

	m_pCurTrackInfo->iAVCDecoderSpecificInfo = AVCDecoderSpecificInfo;
	
	return nErr;
}

int CMP4Parser::ReadBoxEsds(long long aBoxPos, unsigned int aBoxLen)
{
	SKIP_BYTES(aBoxPos, aBoxLen, 1 + 3);	//version + flags

	unsigned char n8Bits;
	ReadSourceData (aBoxPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
	SKIP_BYTES(aBoxPos, aBoxLen, sizeof(unsigned char));

	if (n8Bits == KTag_ESDescriptor)
	{
		unsigned int nDescriptorLen;
		int nErr = ParseDescriptorLength(aBoxPos, aBoxLen, nDescriptorLen);
		if (nErr == QC_ERR_NONE && nDescriptorLen >= 3)
		{
			return ParseEsDescriptor(aBoxPos, nDescriptorLen);		
		}
	}

	return QC_ERR_UNSUPPORT;
}

int CMP4Parser::ReadBoxEdts(long long aBoxPos, unsigned int aBoxLen)
{
	int nRC = QC_ERR_NONE;
	int nEntryNum = 0;
	int i = 0;
	TTEditListInfo * pElstInfo = NULL;
	unsigned int nForamtId = m_pIOReader->ReadUint32BE(aBoxPos + 4);
	if (nForamtId == QC_MKBETAG('e', 'l', 's', 't'))
	{
		aBoxPos += 8;
		SKIP_BYTES(aBoxPos, aBoxLen, 1 + 3);	//version + flags
		nEntryNum = m_pIOReader->ReadUint32BE(aBoxPos);
		aBoxPos += 4;

		pElstInfo = new TTEditListInfo[nEntryNum];
		for (i = 0; i < nEntryNum; i++)
		{
			pElstInfo[i].iTrackDur = m_pIOReader->ReadUint32BE(aBoxPos);
			pElstInfo[i].iMediaTime = m_pIOReader->ReadUint32BE(aBoxPos + 4);
			pElstInfo[i].iMediaRate = m_pIOReader->ReadUint32BE(aBoxPos + 8);

			aBoxPos += 12;
		}

		int			edit_start_index = 0;
		long long	empty_duration = 0;		// empty duration of the first edit list entry
		long long	start_time = 0;			// start time of the media

		for (i = 0; i < nEntryNum; i++)
		{
			if (i == 0 && pElstInfo[i].iMediaTime == -1)
			{
				// if empty, the first entry is the start time of the stream relative to the presentation itself
				empty_duration = pElstInfo[i].iTrackDur;
				edit_start_index = 1;
				if (m_pCurTrackInfo != NULL)
				{
					if (m_nTimeScale == 1 || m_nTimeScale == 0)
						m_pCurTrackInfo->iEmptyDuration = empty_duration;
					else
						m_pCurTrackInfo->iEmptyDuration = empty_duration * 1000 / m_nTimeScale;
				}
			}
			else if (i == edit_start_index && pElstInfo[i].iMediaTime >= 0)
			{
				start_time = pElstInfo[i].iMediaTime;
				if (m_pCurTrackInfo != NULL)
					m_pCurTrackInfo->iStartTime = start_time;
			}
		}
		// adjust first dts according to edit list 
		//if (empty_duration || start_time) 
		//{
		//	if (m_pCurTrackInfo != NULL)
		//		m_pCurTrackInfo->iTimeOffset = start_time - empty_duration;
		//}
	}

	if (pElstInfo != NULL)
		delete[]pElstInfo;

	return nRC;
}

int CMP4Parser::ParseEsDescriptor(long long aDesPos, unsigned int aDesLen)
{	
	SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned short));	// skip ES_ID

	unsigned char n8Bits;
	ReadSourceData (aDesPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
	SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned char));

	unsigned char nStreamDependenceFlag = n8Bits & 0x80;
	unsigned char nUrlFlag = n8Bits & 0x40;
	unsigned char nOcrStreamFlag = n8Bits & 0x20;

	if (nStreamDependenceFlag)
	{
		SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned short));	// skip dependsOn_ES_ID
	}

	if (nUrlFlag)
	{
		ReadSourceData (aDesPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
		SKIP_BYTES(aDesPos, aDesLen, n8Bits+sizeof(unsigned char));		// skip URLstring
	}

	if (nOcrStreamFlag)
	{
		SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned short));	// skip OCR_ES_Id
	}

	int nErr = QC_ERR_UNSUPPORT;
	while (aDesLen > 1) // > 0 
	{
		ReadSourceData (aDesPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
		SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned char));
		unsigned int nDescriptorLen;
		nErr = ParseDescriptorLength(aDesPos, aDesLen, nDescriptorLen);
		if (nErr == QC_ERR_NONE)
		{
			switch (n8Bits)
			{
			case KTag_DecoderConfigDescriptor:
				nErr = ParseDecoderConfigDescriptor(aDesPos, nDescriptorLen);
				SKIP_BYTES(aDesPos, aDesLen, nDescriptorLen);
				break;

			case KTag_SLConfigDescriptor:
				nErr = ParseSLConfigDescriptor(aDesPos, nDescriptorLen);
				SKIP_BYTES(aDesPos, aDesLen, nDescriptorLen);
				break;

			default:
				break;
			}
		}
	}

	return nErr;
}

int CMP4Parser::ParseDecoderConfigDescriptor(long long aDesPos, unsigned int aDesLen)
{
	if (aDesLen < 13)
		return QC_ERR_UNSUPPORT;

	unsigned char n8Bits;
	unsigned char ObjectTypeIndication = 0;
	ReadSourceData (aDesPos, &ObjectTypeIndication, sizeof(unsigned char), QCIO_READ_HEAD);	
	SKIP_BYTES(aDesPos, aDesLen, 13*sizeof(unsigned char));

	if(m_pCurTrackInfo->iAudio)
	{
		if (ObjectTypeIndication == 0xe1) {
			// it's QCELP 14k...
			return  QC_ERR_UNSUPPORT;
		}

		if (ObjectTypeIndication  == 0x6b || ObjectTypeIndication  == 0x69) {
			m_pCurTrackInfo->iCodecType = QC_CODEC_ID_MP3;
		}
	}

	if (aDesLen > 0)
	{
		ReadSourceData (aDesPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
		SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned char));
		if (n8Bits == KTag_DecoderSpecificInfo)
		{
			unsigned int nDescriptorLen;
			int nErr = ParseDescriptorLength(aDesPos, aDesLen, nDescriptorLen);
			if (nErr == QC_ERR_NONE)
			{
				return ParseDecoderSpecificInfo(aDesPos, aDesLen);
			}
		}
	}

	return QC_ERR_UNSUPPORT;
}

int CMP4Parser::ParseDecoderSpecificInfo(long long aDesPos, unsigned int aDesLen)
{
	if (aDesLen > 0) 
	{
		QCMP4DecoderSpecificInfo* MP4DecoderSpecificInfo = (QCMP4DecoderSpecificInfo*)malloc(sizeof(QCMP4DecoderSpecificInfo));
		MP4DecoderSpecificInfo->iData = (unsigned char*)malloc(aDesLen* sizeof(unsigned char));
		ReadSourceData (aDesPos, MP4DecoderSpecificInfo->iData, aDesLen, QCIO_READ_HEAD);
		MP4DecoderSpecificInfo->iSize = aDesLen;
		m_pCurTrackInfo->iMP4DecoderSpecificInfo = MP4DecoderSpecificInfo;

		if(m_pCurTrackInfo->iAudio)
			return ParseM4AWaveFormat(MP4DecoderSpecificInfo, m_pCurTrackInfo->iM4AWaveFormat);
		else
			return QC_ERR_NONE;
	}

	return QC_ERR_UNSUPPORT;
}

int CMP4Parser::ParseSLConfigDescriptor(long long aDesPos, unsigned int aDesLen)
{
	return QC_ERR_NONE;
}

int CMP4Parser::LocationBox(long long& aLocation, long long& aBoxSize, const char * aBoxType, bool bInBox)
{
	unsigned char	nBoxHeaderBuffer[KBoxHeaderSize];
	long long		nReadPos = aLocation;
	long long		nBoxSize = 0;
	int				nHeadSize = 0;
	while(true)
	{
		if (m_pBaseInst->m_bForceClose == true)
			return QC_ERR_FAILED;

		if (KBoxHeaderSize != ReadSourceData (nReadPos, nBoxHeaderBuffer, KBoxHeaderSize, QCIO_READ_HEAD))
		{
			QCLOGI("QCMP4Parser::LocationBox return QC_ERR_MEMORY");
			return QC_ERR_MEMORY;
		}

		nBoxSize = qcIntReadUint32BE(nBoxHeaderBuffer);
		if (nBoxSize > m_llFileSize)
			return QC_ERR_ARG;
		if(nBoxSize == 1)
		{
			nBoxSize = qcIntReadUint64BE(nBoxHeaderBuffer + 8);
			if(nBoxSize < 16)
				return QC_ERR_ARG;
			nHeadSize = 16;
		}
		else
		{
			if(nBoxSize < 8)
				return QC_ERR_ARG;
			nHeadSize = 8;
		}
		if (memcmp(nBoxHeaderBuffer + 4, aBoxType, 4) == 0)
		{
			break;
		}
		else if (memcmp(nBoxHeaderBuffer + 4, "cpky", 4) == 0)
		{
			if (nBoxSize <= 16)
			{
				m_nMP4KeySize += nBoxSize;
				memset(m_szCompKeyTxt, 0, sizeof(m_szCompKeyTxt));
				int nKeySize = nBoxSize - 8;
				for (int i = 0; i < nKeySize; i++)
					m_szCompKeyTxt[i] = nBoxHeaderBuffer[8+i] - (nKeySize - i);
				QCLOGI("The company key is %s!", m_szCompKeyTxt);
			}
		}
		else if (memcmp(nBoxHeaderBuffer + 4, "flky", 4) == 0)
		{
			if (nBoxSize <= 16)
			{
				m_nMP4KeySize += nBoxSize;
				memset(m_szFileKeyTxt, 0, sizeof(m_szFileKeyTxt));
				memcpy(m_szFileKeyTxt, nBoxHeaderBuffer + 8, nBoxSize - 8);
				QCLOGI("The mp4 file key is %s!", m_szFileKeyTxt);
			}
		}
		else if (memcmp(nBoxHeaderBuffer + 4, "mdat", 4) == 0)
		{
			m_llRawDataBegin = nReadPos + nHeadSize;
			m_llRawDataEnd = nReadPos + nBoxSize;
		}
		nReadPos += nBoxSize;
		if (bInBox && nReadPos >= aLocation + aBoxSize)
			return QC_ERR_ARG;
	}
	aLocation = nReadPos;
	aBoxSize = nBoxSize;
	return nHeadSize;
}

int CMP4Parser::ParseDescriptorLength(long long& aDesPos, unsigned int& aDesLen, unsigned int& aDescriptorLen)
{
	bool			bMore;
	unsigned char	n8Bits;

	aDescriptorLen = 0;
	do 
	{
		if (aDesLen == 0) 
		{
			return QC_ERR_UNSUPPORT;
		}

		ReadSourceData (aDesPos, &n8Bits, sizeof(unsigned char), QCIO_READ_HEAD);
		SKIP_BYTES(aDesPos, aDesLen, sizeof(unsigned char));

		aDescriptorLen = (aDescriptorLen << 7) | (n8Bits & 0x7F);
		bMore = (n8Bits & 0x80) != 0;
	}while (bMore);

	return QC_ERR_NONE;
}

int CMP4Parser::ParseM4AWaveFormat(QCMP4DecoderSpecificInfo* aDecoderSpecificInfo, QCM4AWaveFormat* aWaveFormat)
{
	unsigned char* pData = aDecoderSpecificInfo->iData;

	unsigned int nFreqIndex = (pData[0] & 7) << 1 | (pData[1] >> 7);

	if (nFreqIndex == 15) 
	{
		if (aDecoderSpecificInfo->iSize < 5) 
		{
			return QC_ERR_UNSUPPORT;
		}

		aWaveFormat->iSampleRate = ((pData[1] & 0x7f) << 17)
			| (pData[2] << 9)
			| (pData[3] << 1)
			| (pData[4] >> 7);

		aWaveFormat->iChannels = (pData[4] >> 3) & 15;
	} 
	else 
	{
		static unsigned int KSamplingRate[] = {
			96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
			16000, 12000, 11025, 8000, 7350
		};

		if (nFreqIndex == 13 || nFreqIndex == 14) 
		{
			return QC_ERR_UNSUPPORT;
		}

		aWaveFormat->iSampleRate = KSamplingRate[nFreqIndex];
		aWaveFormat->iChannels = (pData[1] >> 3) & 15;
	}

	if (aWaveFormat->iChannels == 0) 
	{
		return QC_ERR_UNSUPPORT;
	}

	return QC_ERR_NONE;
}

// fixed point to double
#define CONV_FP(x) ((double) (x)) / (1 << 16)

// double to fixed point
#define CONV_DB(x) (int32_t) ((x) * (1 << 16))

#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616

double CMP4Parser::av_display_rotation_get(const int matrix[9])
{
	double rotation, scale[2];

	scale[0] = hypot(CONV_FP(matrix[0]), CONV_FP(matrix[3]));
	scale[1] = hypot(CONV_FP(matrix[1]), CONV_FP(matrix[4]));

	if (scale[0] == 0.0 || scale[1] == 0.0)
		return 0;//NAN;

	rotation = atan2(CONV_FP(matrix[1]) / scale[1],
		CONV_FP(matrix[0]) / scale[0]) * 180 / M_PI;

	return -rotation;
}
