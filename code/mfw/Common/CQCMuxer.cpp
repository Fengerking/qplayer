/*******************************************************************************
	File:		CQCMuxer.cpp

	Contains:	Muxer implement code

	Written by:	Jun Lin

	Change History (most recent first):
	2018-01-07		Jun			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CQCMuxer.h"
#include "ULogFunc.h"
#include "UAVFormatFunc.h"
#include "UAVParser.h"
#include "USystemFunc.h"

CQCMuxer::CQCMuxer(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_hLib (NULL)
	, m_nStatus(QCMUX_INIT)
	, m_bWaitKeyFrame(true)
	, m_pFmtVideo(NULL)
	, m_pFmtAudio(NULL)
	, m_nNaluSize (0)
	, m_llTimePause(0)
	, m_llTimeStart(0)
	, m_llTimeStep(0)
	, m_llVideoTime (-1)
{
	SetObjectName ("CQCMuxer");
    memset(&m_fMux, 0, sizeof(m_fMux));
    if (m_pBaseInst)
        m_pBaseInst->AddListener(this);
    
#ifdef __QC_OS_IOS__
    m_pDumpFileAudio = NULL;
    m_pDumpFileVideo = NULL;
#endif // __QC_OS_IOS__
}

CQCMuxer::~CQCMuxer(void)
{
    if(m_pBaseInst)
        m_pBaseInst->RemListener(this);
    Close();
    Uninit();
}

int CQCMuxer::Init (QCParserFormat nFormat)
{
    CAutoLock lock(&m_mtxFunc);
	Uninit ();
#if defined(__QC_OS_IOS__) || defined(__QC_LIB_ONE__)
	int nRC = ffCreateMuxer (&m_fMux, nFormat);
#else
    if (m_pBaseInst->m_hLibCodec == NULL)
        m_hLib = (qcLibHandle)qcLibLoad("qcCodec", 0);
    else
        m_hLib = m_pBaseInst->m_hLibCodec;
    if (m_hLib == NULL)
        return QC_ERR_FAILED;
	FFCREATEMUXER pCreate = (FFCREATEMUXER)qcLibGetAddr(m_hLib, "ffCreateMuxer", 0);
	if (pCreate == NULL)
		return QC_ERR_FAILED;
	int nRC = pCreate (&m_fMux, nFormat);
	if (nRC != QC_ERR_NONE)
	{
		QCLOGW ("Create muxer failed. err = 0X%08X", nRC);
		return nRC;
	}
#endif // __QC_LIB_ONE__

	return nRC;
}

int CQCMuxer::Uninit(void)
{
    CAutoLock lock(&m_mtxFunc);
#if defined(__QC_OS_IOS__) || defined(__QC_LIB_ONE__)
	if (m_fMux.hMuxer != NULL)
		ffDestroyMuxer(&m_fMux);
	m_fMux.hMuxer = NULL;
#else
	if (m_hLib != NULL)
	{
		FFDESTROYMUXER fDestroy = (FFDESTROYMUXER)qcLibGetAddr(m_hLib, "ffDestroyMuxer", 0);
		if (fDestroy != NULL && m_fMux.hMuxer)
			fDestroy (&m_fMux);
        if (m_pBaseInst->m_hLibCodec == NULL)
            qcLibFree(m_hLib, 0);
		m_hLib = NULL;
	}
#endif // __QC_LIB_ONE__
    
	return QC_ERR_NONE;
}

int CQCMuxer::Open(const char * pURL)
{
    QCLOGI("Open %s", pURL);
    CAutoLock lock(&m_mtxFunc);
    if(!m_fMux.hMuxer)
        return QC_ERR_EMPTYPOINTOR;
    Close();
	int nRC = m_fMux.Open(m_fMux.hMuxer, pURL);
	if (nRC != QC_ERR_NONE)
		return nRC;
    m_bWaitKeyFrame = true;
	m_nStatus = QCMUX_OPEN;
	m_llTimePause = 0;
	m_llTimeStart = 0;
	m_llTimeStep = 0;
	m_nNaluSize = 0;
    
#ifdef __QC_OS_IOS__00
    char szTmp[2048], szDump[2048];
    memset(szTmp, 0, 2048);
    qcGetAppPath (NULL, szTmp, 2048);
    
    memset(szDump, 0, 2048);
    sprintf(szDump, "%s%s", szTmp, "audio_dump.dat");
    m_pDumpFileAudio = new CFileIO(m_pBaseInst);
    m_pDumpFileAudio->Open(szDump, 0, QCIO_FLAG_WRITE);

    memset(szDump, 0, 2048);
    sprintf(szDump, "%s%s", szTmp, "video_dump.dat");
    m_pDumpFileVideo = new CFileIO(m_pBaseInst);
    m_pDumpFileVideo->Open(szDump, 0, QCIO_FLAG_WRITE);
#endif // __QC_OS_IOS__

	return nRC;
}

int CQCMuxer::Close()
{
    QCLOGI("Close");
	if (m_nStatus <= QCMUX_CLOSE)
		return QC_ERR_NONE;

    CAutoLock lock(&m_mtxFunc);
    if(!m_fMux.hMuxer)
        return QC_ERR_EMPTYPOINTOR;
    int nRet = m_fMux.Close(m_fMux.hMuxer);
	m_nStatus = QCMUX_CLOSE;
    m_bWaitKeyFrame = true;

	QC_DATA_BUFF * pBuff = m_lstBuff.RemoveHead();
	while (pBuff != NULL)
	{
		delete[]pBuff->pBuff;
		delete pBuff;
		pBuff = m_lstBuff.RemoveHead();
	}
	pBuff = m_lstFree.RemoveHead();
	while (pBuff != NULL)
	{
		delete[]pBuff->pBuff;
		delete pBuff;
		pBuff = m_lstFree.RemoveHead();
	}
	m_llVideoTime = -1;

	qcavfmtDeleteVideoFormat(m_pFmtVideo);
	m_pFmtVideo = NULL;
	qcavfmtDeleteAudioFormat(m_pFmtAudio);
	m_pFmtAudio = NULL;

#ifdef __QC_OS_IOS__
    if (m_pDumpFileAudio != NULL)
    {
        m_pDumpFileAudio->Close();
        delete m_pDumpFileAudio;
    }
    if (m_pDumpFileVideo != NULL)
    {
        m_pDumpFileVideo->Close();
        delete m_pDumpFileVideo;
    }
#endif
    
    return nRet;
}

int CQCMuxer::Pause()
{
    QCLOGI("Pause");
    CAutoLock lock(&m_mtxFunc);
	if (m_nStatus == QCMUX_PAUSE)
		return QC_ERR_NONE;
	m_nStatus = QCMUX_PAUSE;
	m_llTimePause = 0;
	m_bWaitKeyFrame = true;

	QC_DATA_BUFF * pBuff = m_lstBuff.RemoveHead();
	while (pBuff != NULL)
	{
		m_lstFree.AddTail(pBuff);
		pBuff = m_lstBuff.RemoveHead();
	}
	m_llVideoTime = -1;
    return QC_ERR_NONE;
}

int CQCMuxer::Restart()
{
	QCLOGI("Restart");
	CAutoLock lock(&m_mtxFunc);
	if (m_nStatus == QCMUX_RESTART)
		return QC_ERR_NONE;
	m_nStatus = QCMUX_RESTART;
	m_llTimeStart = 0;
	return QC_ERR_NONE;
}

int CQCMuxer::Init(void * pVideoFmt, void * pAudioFmt)
{
	CAutoLock lock(&m_mtxFunc);
	if (!m_fMux.hMuxer)
		return QC_ERR_EMPTYPOINTOR;
    if(pAudioFmt)
    {
        QC_AUDIO_FORMAT* pFmt = (QC_AUDIO_FORMAT*)pAudioFmt;
        QCLOGI("Audio fmt: Samplerate %d, Channel %d, Bits %d, Size %d",
               pFmt->nSampleRate, pFmt->nChannels, pFmt->nBits, pFmt->nHeadSize);
		m_pFmtAudio = qcavfmtCloneAudioFormat(pFmt);
    }
    if(pVideoFmt)
    {
        QC_VIDEO_FORMAT* pFmt = (QC_VIDEO_FORMAT*)pVideoFmt;
        QCLOGI("Video fmt: %d x %d, Size %d",
               pFmt->nWidth, pFmt->nHeight, pFmt->nHeadSize);
		m_pFmtVideo = qcavfmtCloneVideoFormat(pFmt);
#ifdef __QC_OS_IOS__
        QC_DATA_BUFF buf;
        memset(&buf, 0, sizeof(QC_DATA_BUFF));
        buf.llTime = 0;
        buf.uSize = pFmt->nHeadSize;
        buf.pBuff = pFmt->pHeadData;
        dumpFrame(&buf);
#endif
    }

	if (m_pFmtVideo != NULL)
		return QC_ERR_NONE;
	else
		return m_fMux.Init(m_fMux.hMuxer, m_pFmtVideo, m_pFmtAudio);
}

int CQCMuxer::Write(QC_DATA_BUFF* pBuff)
{
    CAutoLock lock(&m_mtxFunc);
	if (!m_fMux.hMuxer || m_nStatus <= QCMUX_CLOSE)
        return QC_ERR_EMPTYPOINTOR;

	if (m_pFmtVideo == NULL && pBuff->nMediaType == QC_MEDIA_Video)
		return QC_ERR_ARG;
	if (m_pFmtAudio == NULL && pBuff->nMediaType == QC_MEDIA_Audio)
		return QC_ERR_ARG;

	if (m_nStatus == QCMUX_PAUSE)
	{
		if (m_llTimePause == 0)
			m_llTimePause = pBuff->llTime;
		return QC_ERR_RETRY;
	}

	if (m_bWaitKeyFrame && m_pFmtVideo != NULL)
	{
		if (pBuff->nMediaType == QC_MEDIA_Video)
		{
			if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			{
				m_bWaitKeyFrame = false;

				bool			bNewFormat = false;
				int				nNalHead = 0X010000;
				unsigned char * pHead = pBuff->pBuff;
				unsigned char * pHeadPos = NULL;
				while (pHead - pBuff->pBuff < pBuff->uSize - 5)
				{
					if (memcmp(pHead, &nNalHead, 3) == 0)
					{
						if (pHeadPos == NULL)
						{
							pHeadPos = pHead;
							if (pHead > pBuff->pBuff && *(pHead - 1) == 0)
								pHeadPos = pHeadPos - 1;
						}

						if ((pHead[3] & 0X1F) == 5)
						{
							if (pHead > pBuff->pBuff && *(pHead - 1) == 0)
								pHead--;

							if (pHead != pHeadPos)
							{
								m_pFmtVideo->nHeadSize = pHead - pHeadPos;
								QC_DEL_A(m_pFmtVideo->pHeadData);
								m_pFmtVideo->pHeadData = new unsigned char[m_pFmtVideo->nHeadSize];
								memcpy(m_pFmtVideo->pHeadData, pHeadPos, m_pFmtVideo->nHeadSize);
								bNewFormat = true;
							}
							break;
						}
						pHead += 4;
					}
					pHead++;
				}
				if (bNewFormat)
				{
					QCLOGI("The MUX video IO format --- Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
					int numRef = 0;
					qcAV_FindAVCDimensions(m_pFmtVideo->pHeadData, m_pFmtVideo->nHeadSize, &m_pFmtVideo->nWidth, &m_pFmtVideo->nHeight, &numRef);
				}
				m_fMux.Init(m_fMux.hMuxer, m_pFmtVideo, m_pFmtAudio);
			}
			else
				return QC_ERR_STATUS;
		}
		else
		{
			return QC_ERR_STATUS;
		}
	}

	if (m_nStatus == QCMUX_RESTART)
	{
		if (m_llTimeStart == 0)
		{
			m_llTimeStart = pBuff->llTime;
			m_llTimeStep = m_llTimeStep + (m_llTimeStart - m_llTimePause);
		}
	}
	long long llBuffTime = pBuff->llTime;
	pBuff->llTime = pBuff->llTime - m_llTimeStep;
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
//		QCLOGI("The MUX video--- Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
	}
	else
	{
//		QCLOGI("The MUX audio+++ Flag = %08X  Size = % 8d, Time = % 8lld", pBuff->uFlag, pBuff->uSize, pBuff->llTime);
	}

	int nRC = QC_ERR_NONE;
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
		if (m_llVideoTime > 0 && pBuff->llTime > m_llVideoTime && m_lstBuff.GetCount () > 0)
		{
			QC_DATA_BUFF * pDataBuff = m_lstBuff.GetHead();
			if (m_lstBuff.GetCount() == 1)
			{
				pDataBuff->llDelay = pDataBuff->llTime;
			}
			else
			{
				int				nBuffIdx = 0;
				int				nBuffNum = m_lstBuff.GetCount();
				QC_DATA_BUFF **	pBuffList = new QC_DATA_BUFF *[nBuffNum];
				NODEPOS pos = m_lstBuff.GetHeadPosition();
				while (pos != NULL)
				{
					pDataBuff = m_lstBuff.GetNext(pos);
					pBuffList[nBuffIdx++] = pDataBuff;
				}
				qsort(pBuffList, nBuffNum, sizeof(QC_DATA_BUFF *), compareBuffTime);

				nBuffIdx = 0;
				pos = m_lstBuff.GetHeadPosition();
				while (pos != NULL)
				{
					pDataBuff = m_lstBuff.GetNext(pos);
					pDataBuff->llDelay = pBuffList[nBuffIdx++]->llTime;
				}
				delete[]pBuffList;
			}
			pDataBuff = m_lstBuff.RemoveHead();
			while (pDataBuff != NULL)
			{
				pDataBuff->llDelay -= 40;
				nRC = muxWrite(pDataBuff);
#ifdef __QC_OS_IOS__
                dumpFrame(pDataBuff);
#endif
				m_lstFree.AddTail(pDataBuff);
				pDataBuff = m_lstBuff.RemoveHead();
			}
		}
		QC_DATA_BUFF * pNewBuff = m_lstFree.RemoveHead();
		pNewBuff = CloneBuff(pBuff, pNewBuff);
		m_lstBuff.AddTail(pNewBuff);
		if (m_llVideoTime < pBuff->llTime)
			m_llVideoTime = pBuff->llTime;
	}
	else
	{
		nRC = muxWrite(pBuff);
#ifdef __QC_OS_IOS__
        dumpFrame(pBuff);
#endif
	}
	pBuff->llTime = llBuffTime;
	return nRC;
}

int CQCMuxer::GetParam(int nID, void * pParam)
{
    if(!m_fMux.hMuxer)
        return QC_ERR_EMPTYPOINTOR;
    return m_fMux.GetParam(m_fMux.hMuxer, nID, pParam);
}

int CQCMuxer::SetParam(int nID, void * pParam)
{
    if(!m_fMux.hMuxer)
        return QC_ERR_EMPTYPOINTOR;

    return m_fMux.SetParam(m_fMux.hMuxer, nID, pParam);
}

int	CQCMuxer::muxWrite(QC_DATA_BUFF* pBuff)
{
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
		//QCLOGI("The MUX video--- Flag = %08X  Size = % 8d, Time = % 8lld, DTS = % 8lld", pDataBuff->uFlag, pDataBuff->uSize, pDataBuff->llTime, pDataBuff->llDelay);
		// for H264
		if (m_pFmtVideo->nCodecID == QC_CODEC_ID_H264 && (pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
		{
			unsigned char * pHead = pBuff->pBuff;
			if (m_nNaluSize == 0)
			{
				m_nNaluWord = 0X010000;
				while (pHead - pBuff->pBuff < pBuff->uSize - 5)
				{
					if (memcmp(pHead, &m_nNaluWord, 3) == 0)
					{
						if (pHead > pBuff->pBuff && *(pHead - 1) == 0)
						{
							m_nNaluWord = 0X01000000;
							m_nNaluSize = 4;
						}
						else
						{
							m_nNaluSize = 3;
							m_nNaluWord = 0X010000;
						}
						break;
					}
					pHead++;
				}
			}

			pHead = pBuff->pBuff;
			while (pHead - pBuff->pBuff < pBuff->uSize - 5)
			{
				if (memcmp(pHead, &m_nNaluWord, m_nNaluSize) == 0)
				{
					if ((pHead[4] & 0X1F) == 5)
					{
						if (pHead != pBuff->pBuff)
						{
							memcpy(&m_dataBuff, pBuff, sizeof(QC_DATA_BUFF));
							m_dataBuff.pBuff = pHead;
							m_dataBuff.uSize = pBuff->uSize - (pHead - pBuff->pBuff);
							pBuff = &m_dataBuff;
						}
						break;
					}
				}
				pHead++;
			}
		}
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		// for AAC FF F9?
		if (m_pFmtAudio->nCodecID == QC_CODEC_ID_AAC)
		{
			if (pBuff->pBuff[0] == 0XFF && pBuff->pBuff[1] == 0XF9)
			{
				memcpy(&m_dataBuff, pBuff, sizeof(QC_DATA_BUFF));
				m_dataBuff.pBuff = pBuff->pBuff + 7;
				m_dataBuff.uSize = pBuff->uSize - 7;
				pBuff = &m_dataBuff;
			}
		}
	}
	int nRC = m_fMux.Write(m_fMux.hMuxer, pBuff);
	return nRC;
}

int CQCMuxer::compareBuffTime(const void *arg1, const void *arg2)
{
	QC_DATA_BUFF * pItem1 = *(QC_DATA_BUFF **)arg1;
	QC_DATA_BUFF * pItem2 = *(QC_DATA_BUFF **)arg2;
	return (int)(pItem1->llTime - pItem2->llTime);
}


#ifdef __QC_OS_IOS__
int CQCMuxer::dumpFrame(QC_DATA_BUFF* pBuff)
{
    if (!pBuff || !pBuff->pBuff)
        return 0;
    if (m_pDumpFileAudio != NULL && pBuff->nMediaType == QC_MEDIA_Audio)
    {
        int nWrite = pBuff->uSize; // szie + time + flag
        switchByteWrite((unsigned char *)&nWrite, 4, m_pDumpFileAudio);
        switchByteWrite((unsigned char *)&(pBuff->llTime), 8, m_pDumpFileAudio);
        switchByteWrite((unsigned char *)&(pBuff->uFlag), 4, m_pDumpFileAudio);
        return m_pDumpFileAudio->Write(pBuff->pBuff, pBuff->uSize);
    }
    else if (m_pDumpFileVideo != NULL && pBuff->nMediaType == QC_MEDIA_Video)
    {
        int nWrite = pBuff->uSize; // szie + time + flag
        switchByteWrite((unsigned char *)&nWrite, 4, m_pDumpFileVideo);
        switchByteWrite((unsigned char *)&(pBuff->llTime), 8, m_pDumpFileVideo);
        switchByteWrite((unsigned char *)&(pBuff->uFlag), 4, m_pDumpFileVideo);
        return m_pDumpFileVideo->Write(pBuff->pBuff, pBuff->uSize);
    }
    return 0;
}

int CQCMuxer::switchByteWrite(unsigned char * pSrc, int nSize, CFileIO * pIO)
{
    unsigned char szDest[32];
    for (int i = 0; i < nSize; i++)
        szDest[nSize - i - 1] = pSrc[i];
    pIO->Write(szDest, nSize);
    return nSize;
}

int CQCMuxer::switchByteRead(unsigned char * pSrc, int nSize, CFileIO * pIO)
{
    unsigned char szDest[32];
    int nRead = nSize;
    pIO->Read(szDest, nRead, true, 0);
    for (int i = 0; i < nSize; i++)
        pSrc[nSize - i - 1] = szDest[i];
    return nSize;
}
#endif
