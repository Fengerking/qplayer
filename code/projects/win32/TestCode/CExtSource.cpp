/*******************************************************************************
	File:		CExtSource.cpp

	Contains:	Window base implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#ifdef __QC_OS_WIN32__
#include "windows.h"
#include "tchar.h"
#endif

#include "CExtSource.h"

#include "USystemFunc.h"
#include "ULogFunc.h"
#include "UUrlParser.h"

int	CExtBuffMng::Send(QC_DATA_BUFF * pBuff)
{
	m_pSource->Send(pBuff);
	return QC_ERR_NONE;
}

CExtSource::CExtSource(void * hInst)
	: m_hInst (hInst)
	, m_pPlay (NULL)
	, m_pReadThread(NULL)
	, m_pAudioFile(NULL)
	, m_pVideoFile(NULL)
	, m_llAudioTime(0)
	, m_llVideoTime(0)
	, m_pDumpFile(NULL)
{
	m_pInst = new CBaseInst();
	m_pInst->m_pSetting->g_qcs_nPlaybackLoop = 1;
	memset(&m_fIO, 0, sizeof(m_fIO));
	m_fIO.pBaseInst = m_pInst;
	memset(&m_fParser, 0, sizeof(m_fParser));
	m_fParser.pBaseInst = m_pInst;

	memset(&m_buffAudio, 0, sizeof(m_buffAudio));
	m_buffAudio.nMediaType = QC_MEDIA_Audio;
	memset(&m_buffVideo, 0, sizeof(m_buffVideo));
	m_buffVideo.nMediaType = QC_MEDIA_Video;
	memset(&m_buffData, 0, sizeof(m_buffData));
#ifdef __QC_OS_WIN32__
	strcpy(m_szFile, "c:/temp/15346515019356_origin.mp4");
	strcpy(m_szFile, "C:\\temp\\s033d_0.mp4");
#elif defined __QC_OS_IOS__
    char szTmp[2048];
    memset(szTmp, 0, 2048);
    qcGetAppPath (NULL, szTmp, 2048);
    sprintf(m_szFile, "%s%s", szTmp, "15346515019356_origin.mp4");
#endif
    
    SetSourceType(1);

#ifdef __QC_OS_WIN32__0
	m_pDumpFile = new CFileIO(m_pInst);
	m_pDumpFile->Open("c:\\work\\temp\\AAC.dat", 0, QCIO_FLAG_WRITE);
#endif // __QC_OS_WIN32__
}

CExtSource::~CExtSource(void)
{
	Close();
	if (m_buffData.pBuff != NULL)
		delete[]m_buffData.pBuff;

	if (m_pDumpFile != NULL)
	{
		m_pDumpFile->Close();
		delete m_pDumpFile;
	}
	if (m_pAudioFile != NULL)
	{
		m_pAudioFile->Close();
		delete m_pAudioFile;
	}
	if (m_pVideoFile != NULL)
	{
		m_pVideoFile->Close();
		delete m_pVideoFile;
	}
	delete m_pInst;
}

int CExtSource::SetPlayer (QCM_Player * pPlay)
{
	Close();

	m_pPlay = pPlay;

	m_pReadThread = new CThreadWork(m_pInst);
	m_pReadThread->SetOwner("CExtSource");
	m_pReadThread->SetWorkProc(this, &CThreadFunc::OnWork);
	m_pReadThread->Start();

	CAutoLock lock(&m_lock);
	int nRC = 0;
	m_fIO.pBaseInst = m_pInst;
    if(!strncmp(m_szFile, "http", 4))
        nRC = qcCreateIO(&m_fIO, QC_IOPROTOCOL_HTTP);
    else if(!strncmp(m_szFile, "rtmp", 4))
        nRC = qcCreateIO(&m_fIO, QC_IOPROTOCOL_RTMP);
    else
        nRC = qcCreateIO(&m_fIO, QC_IOPROTOCOL_FILE);
	nRC = m_fIO.Open(m_fIO.hIO, m_szFile, 0, QCIO_FLAG_READ);

	if (m_bExtAV)
	{
		QCCodecID nCodecID = QC_CODEC_ID_H264;
		m_pPlay->SetParam(m_pPlay->hPlayer, QCPLAY_PID_EXT_VIDEO_CODEC, &nCodecID);
		m_pPlay->SetParam(m_pPlay->hPlayer, QCPLAY_PID_EXT_AUDIO_CODEC, &m_bAudioCodecID);
		nRC = m_pPlay->Open(m_pPlay->hPlayer, "EXT_AV", QCPLAY_OPEN_EXT_SOURCE_AV);

		m_pBuffMng = new CExtBuffMng(m_pInst, this);
		m_fParser.pBuffMng = m_pBuffMng;

		if (!strncmp(m_szFile, "rtmp", 4))
			nRC = qcCreateParser(&m_fParser, QC_PARSER_FLV);
		else
        {
            char ext[12];
            memset(ext, 0, 12);
            qcUrlParseExtension(m_szFile, ext, 12);
            if(!strcmp(ext, "TS"))
            	nRC = qcCreateParser(&m_fParser, QC_PARSER_TS);
            else if(!strcmp(ext, "M3U8"))
                nRC = qcCreateParser(&m_fParser, QC_PARSER_M3U8);
            else if(!strcmp(ext, "MP4"))
                nRC = qcCreateParser(&m_fParser, QC_PARSER_MP4);
            else if(!strcmp(ext, "FLV"))
                nRC = qcCreateParser(&m_fParser, QC_PARSER_FLV);
        }
		nRC = m_fParser.Open(m_fParser.hParser, &m_fIO, m_szFile, 0);
	}
	else
	{
		QCParserFormat nParser = QC_PARSER_TS;
		nRC = m_pPlay->SetParam(m_pPlay->hPlayer, QCPLAY_PID_Prefer_Format, &nParser);
		nRC = m_pPlay->Open(m_pPlay->hPlayer, "EXT_IO", QCPLAY_OPEN_EXT_SOURCE_IO);
	}
	return QC_ERR_NONE;
}

static int switchByteWrite(unsigned char * pSrc, int nSize, CFileIO * pIO)
{
	unsigned char szDest[32];
	for (int i = 0; i < nSize; i++)
		szDest[nSize - i - 1] = pSrc[i];
	pIO->Write(szDest, nSize);
	return nSize;
}

static int switchByteRead(unsigned char * pSrc, int nSize, CFileIO * pIO)
{
	unsigned char szDest[32];
	int nRead = nSize;
	pIO->Read(szDest, nRead, true, 0);
	for (int i = 0; i < nSize; i++)
		pSrc[nSize - i - 1] = szDest[i];
	return nSize;
}

int	CExtSource::Send(QC_DATA_BUFF * pBuff)
{
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
		if (ReadVideoData(pBuff) <= 0)
			memcpy(&m_buffVideo, pBuff, sizeof(QC_DATA_BUFF));
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
		qcSleep(30000);
		return QC_ERR_NONE;
		if (ReadAudioData(pBuff) <= 0)
			memcpy(&m_buffAudio, pBuff, sizeof(QC_DATA_BUFF));
	}

	int nRC = QC_ERR_RETRY;
	while (nRC != QC_ERR_NONE)
	{
		nRC = m_pPlay->SetParam(m_pPlay->hPlayer, QCPLAY_PID_EXT_SOURCE_DATA, pBuff);
		qcSleep(2000);
	}

	m_pBuffMng->Return(pBuff);

	return QC_ERR_NONE;
}

int	CExtSource::ReadVideoData(QC_DATA_BUFF * pBuff)
{
    //return 0;
	int nRead = 0;
	if (m_pVideoFile == NULL)
	{
		m_llVideoTime = 0;
		m_pVideoFile = new CFileIO(m_pInst);
#ifdef __QC_OS_WIN32__
		m_pVideoFile->Open("c:\\temp\\video.dat", 0, QCIO_FLAG_READ);
#elif defined __QC_OS_IOS__
		char szTmp[2048];
		char szFile[2048];
		memset(szTmp, 0, 2048);
		qcGetAppPath(NULL, szTmp, 2048);
		sprintf(szFile, "%s%s", szTmp, "video.dat");
		m_pVideoFile->Open(szFile, 0, QCIO_FLAG_READ);
#endif
	}

	pBuff->uFlag = 0;
	nRead = pBuff->uBuffSize;
	int			nSize = 0;
	long long	llTime = 0;
	int			nFlag = 0;
//	switchByteRead ((unsigned char *)&nSize, 4, m_pVideoFile);
//	switchByteRead((unsigned char *)&llTime, 8, m_pVideoFile);
//	switchByteRead((unsigned char *)&nFlag, 4, m_pVideoFile);
	nRead = 4;
	m_pVideoFile->Read ((unsigned char *)&nSize, nRead, true, 0);
	nRead = 8;
	m_pVideoFile->Read((unsigned char *)&llTime, nRead, true, 0);
	nRead = 4;
	m_pVideoFile->Read((unsigned char *)&nFlag, nRead, true, 0);

	pBuff->uSize = nSize;
	pBuff->uFlag = nFlag;
	pBuff->llTime = m_llVideoTime;// llTime;
	m_llVideoTime += 33;
	if (pBuff->uBuffSize < nSize)
	{
		QC_DEL_A(pBuff->pBuff);
		pBuff->uBuffSize = nSize + 1024;
		pBuff->pBuff = new unsigned char[pBuff->uBuffSize];
	}
	m_pVideoFile->Read(pBuff->pBuff, nSize, true, 0);
    
	return nRead;
}

int	CExtSource::ReadAudioData(QC_DATA_BUFF * pBuff)
{
	return 0;

	if (m_pDumpFile != NULL)
	{
		int nWrite = pBuff->uSize; // szie + time + flag
		switchByteWrite((unsigned char *)&nWrite, 4, m_pDumpFile);
		switchByteWrite((unsigned char *)&(pBuff->llTime), 8, m_pDumpFile);
		switchByteWrite((unsigned char *)&(pBuff->uFlag), 4, m_pDumpFile);
		m_pDumpFile->Write(pBuff->pBuff, pBuff->uSize);
	}

	int nRead = 0;
	if (m_bAudioCodecID == QC_CODEC_ID_G711A || m_bAudioCodecID == QC_CODEC_ID_G711U)
	{
		if (m_pAudioFile == NULL)
		{
			m_pAudioFile = new CFileIO(m_pInst);
#ifdef __QC_OS_WIN32__
			if (m_bAudioCodecID == QC_CODEC_ID_G711A)
				m_pAudioFile->Open("c:\\work\\temp\\audio.alaw", 0, QCIO_FLAG_READ);
			else
				m_pAudioFile->Open("c:\\work\\temp\\test.ulaw", 0, QCIO_FLAG_READ);
#elif defined __QC_OS_IOS__
			char szTmp[2048];
			char szFile[2048];
			memset(szTmp, 0, 2048);
			qcGetAppPath(NULL, szTmp, 2048);
            if (m_bAudioCodecID == QC_CODEC_ID_G711A)
                sprintf(szFile, "%s%s", szTmp, "audio.alaw");
            else
                sprintf(szFile, "%s%s", szTmp, "audio.ulaw");
			m_pAudioFile->Open(szFile, 0, QCIO_FLAG_READ);
#endif
		}

		pBuff->uFlag = 0;
		nRead = pBuff->uBuffSize;
#if 0
		int			nSize = 0;
		long long	llTime = 0;
		int			nFlag = 0;
		nRead = 4;
		m_pAudioFile->Read((unsigned char *)&nSize, nRead, true, 0);
		nRead = 8;
		m_pAudioFile->Read((unsigned char *)&llTime, nRead, true, 0);
		nRead = 4;
		m_pAudioFile->Read((unsigned char *)&nFlag, nRead, true, 0);

		pBuff->uSize = nSize;
		pBuff->uFlag = nFlag;
		pBuff->llTime = llTime;
		if (pBuff->uBuffSize < nSize)
		{
			QC_DEL_A(pBuff->pBuff);
			pBuff->uBuffSize = nSize + 1024;
			pBuff->pBuff = new unsigned char[pBuff->uBuffSize];
		}
#endif // 0
		m_pAudioFile->Read(pBuff->pBuff, nRead, true, 0);
		pBuff->llTime = m_llAudioTime;
		m_llAudioTime += 20;
	}
	return nRead;
}

int CExtSource::Close(void)
{
	if (m_pReadThread != NULL)
	{
		m_pReadThread->Stop();
		delete m_pReadThread;
		m_pReadThread = NULL;
	}

	if (m_fParser.hParser != NULL)
	{
		m_fParser.Close(m_fParser.hParser);
		qcDestroyParser(&m_fParser);
	}
	if (m_fIO.hIO != NULL)
		qcDestroyIO(&m_fIO);

	m_nAudioBuffTime = 0;
	m_nVideoBuffTime = 0;

	return QC_ERR_NONE;
}

void CExtSource::NotifyEvent(void * pUserData, int nID, void * pValue1)
{
	int nRC = 0;
	switch (nID)
	{
	case QC_MSG_PLAY_OPEN_DONE:
		break;

	case QC_MSG_BUFF_VBUFFTIME:
		m_nVideoBuffTime = *(int *)pValue1;
		break;

	case QC_MSG_BUFF_ABUFFTIME:
		m_nAudioBuffTime = *(int *)pValue1;
		break;

	default:
		break;
	}
}

int	CExtSource::OnWorkItem(void)
{
	int nRC = QC_ERR_NONE;
	CAutoLock lock(&m_lock);
	if (m_bExtAV)
	{
		if (m_fParser.hParser == NULL)
		{
			qcSleep(5000);
			return QC_ERR_RETRY;
		}
		QC_DATA_BUFF * pBuff = NULL;

		if (m_nVideoBuffTime > 10000 && m_nAudioBuffTime > 10000)
		{
            qcSleep(10000);
			return QC_ERR_RETRY;
		}
		if (m_nVideoBuffTime <= m_nAudioBuffTime)
			pBuff = &m_buffVideo;
		else
			pBuff = &m_buffAudio;
		nRC = m_fParser.Read(m_fParser.hParser, pBuff);
        qcSleep(5000);
	}
	else
	{
		if (m_fIO.hIO == NULL)
		{
			qcSleep(5000);
			return QC_ERR_RETRY;
		}
		int nRead = m_buffData.uBuffSize;
		nRC = m_fIO.Read(m_fIO.hIO, m_buffData.pBuff, nRead, false, QCIO_READ_DATA);
		if (nRC == QC_ERR_NONE && nRead > 0)
		{
			m_buffData.uSize = nRead;
			while (true)
			{
				nRC = m_pPlay->SetParam(m_pPlay->hPlayer, QCPLAY_PID_EXT_SOURCE_DATA, &m_buffData);
				if (nRC == QC_ERR_NONE)
					break;
				qcSleep(10*1000);
			}
		}
		else
		{
			m_fIO.SetPos(m_fIO.hIO, 0, QCIO_SEEK_BEGIN);
		}
		qcSleep(2000);
	}

	return nRC;
}

void CExtSource::SetSourceType(int nType)
{
    m_bExtAV = nType>0?true:false;
	//m_bAudioCodecID = QC_CODEC_ID_G711A;
	m_bAudioCodecID = QC_CODEC_ID_G711U;
    //m_bAudioCodecID = QC_CODEC_ID_AAC;

    if (!m_bExtAV && !m_buffData.pBuff)
    {
        m_buffData.uBuffSize = 32 * 1024;
		m_buffData.uBuffSize = 32 * 188;
        m_buffData.pBuff = new unsigned char[m_buffData.uBuffSize];
    }
}

void CExtSource::SetURL(char* pszUrl)
{
#ifdef __QC_OS_WIN32__
    strcpy(m_szFile, pszUrl);
#elif defined __QC_OS_IOS__
    if(!strncmp(pszUrl, "http", 4) || !strncmp(pszUrl, "rtmp", 4))
    {
        sprintf(m_szFile, "%s", pszUrl);
    }
    else
    {
        char szTmp[2048];
        memset(szTmp, 0, 2048);
        qcGetAppPath (NULL, szTmp, 2048);
        sprintf(m_szFile, "%s%s", szTmp, pszUrl);
    }
#endif
}
