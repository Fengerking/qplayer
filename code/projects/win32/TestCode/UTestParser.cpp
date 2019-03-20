/*******************************************************************************
	File:		UTestParser.cpp

	Contains:	The message manager implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "UTestParser.h"

#include "USourceFormat.h"
#include "USystemFunc.h"

CTestParser::CTestParser(void)
	: CBaseObject (NULL)
	, m_pBuffMng (NULL)
	, m_pReadThread (NULL)
	, m_pSource (NULL)
	, m_pBoxSource (NULL)
	, m_pBoxMng (NULL)
{
	SetObjectName ("CTestParser");
	memset (&m_fParser, 0, sizeof (QC_Parser_Func));
	memset (&m_fIO, 0, sizeof (QC_IO_Func));
	memset (&m_buffInfo, 0, sizeof (m_buffInfo));
}

CTestParser::~CTestParser(void)
{
	Close ();
}

int	CTestParser::Open (const char * pURL)
{
	m_pBoxMng = new COMBoxMng (NULL);
	if (m_pBoxMng == NULL)
	{
		m_pBoxSource = new CBoxSource (NULL, NULL);
		if (m_pBoxSource == NULL)
		{
			m_pSource = new CQCSource (NULL, NULL);
			if (m_pSource == NULL)
				m_pBuffMng = new CBuffMng (NULL);
		}
	}

	if (m_pBoxMng != NULL)
	{
		m_pBoxMng->Open (pURL, 0);
		m_pBoxMng->Start ();
		return 0;
	} 
	if (m_pBoxSource != NULL)
	{
		m_pBoxSource->OpenSource (pURL, 0);
		m_pBoxSource->Start ();
		return 0;
	}
	if (m_pSource != NULL)
	{
		m_pSource->Open (pURL, 0);
		m_pSource->Start ();
		return 0;
	}
	int nRC = qcCreateIO (&m_fIO, qcGetSourceProtocol (pURL));
	if (nRC < 0)
		return nRC;

	m_fParser.pBuffMng = m_pBuffMng;
	qcCreateParser (&m_fParser, qcGetSourceFormat (pURL));
	if (m_fParser.hParser == NULL)
		return QC_ERR_FORMAT;

	nRC = m_fParser.Open (m_fParser.hParser, &m_fIO, pURL, 0);
	if (nRC < 0)
		return nRC;

	if (m_pReadThread == NULL)
	{
		m_pReadThread = new CThreadWork (NULL);
		m_pReadThread->SetOwner ("Test Parser");
		m_pReadThread->SetWorkProc (this, &CThreadFunc::OnWork);
	}
	m_pReadThread->Start ();

	return 0;
}

int	CTestParser::Close (void)
{
	if (m_pBoxMng != NULL)
	{
		m_pBoxMng->Close ();
		QC_DEL_P (m_pBoxMng);
	}
	if (m_pBoxSource != NULL)
	{
		m_pBoxSource->Close ();
		QC_DEL_P (m_pBoxSource);
	}
	if (m_pSource != NULL)
	{
		m_pSource->Close ();
		QC_DEL_P (m_pSource);
	}
	if (m_pReadThread != NULL)
	{
		m_pReadThread->Stop ();
		QC_DEL_P (m_pReadThread);
	}
	if (m_fParser.hParser != NULL)
	{
		m_fParser.Close (m_fParser.hParser);
		qcDestroyParser (&m_fParser);
		m_fParser.hParser = NULL;
	}
	if (m_fIO.hIO != NULL)
		qcDestroyIO (&m_fIO);

	QC_DEL_P (m_pBuffMng);

	return 0;
}

int CTestParser::OnWorkItem (void)
{
	if (m_fParser.hParser == NULL || m_pBuffMng == NULL)
		return QC_ERR_STATUS;

	int nRC = QC_ERR_NONE;
	long long llBuffAudio = m_pBuffMng->GetBuffTime (QC_MEDIA_Audio);
	long long llBuffVideo = m_pBuffMng->GetBuffTime (QC_MEDIA_Video);
	if (llBuffVideo > 100000 && llBuffAudio > 10000)
	{
//		qcSleep (2000);
//		return QC_ERR_RETRY;
	}
	if (llBuffVideo > llBuffAudio)
		m_buffInfo.nMediaType = QC_MEDIA_Audio;
	else
		m_buffInfo.nMediaType = QC_MEDIA_Video;
	nRC = m_fParser.Read (m_fParser.hParser, &m_buffInfo);
	if (nRC == QC_ERR_FINISH)
		qcSleep (2000);

	return QC_ERR_NONE;
}

int	qcTestParserOpen (void)
{
	char				szSource[256];
	QC_Parser_Func		m_fParser;
	QC_IO_Func			m_fIO;
	CBuffMng *			m_pBuffMng;
	QC_DATA_BUFF		m_buffInfo;

	memset (&m_fParser, 0, sizeof (QC_Parser_Func));
	memset (&m_fIO, 0, sizeof (QC_IO_Func));
	memset (&m_buffInfo, 0, sizeof (m_buffInfo));

	strcpy (szSource, "D:\\Work\\TestClips\\YouTube_0.flv");
	m_pBuffMng = new CBuffMng (NULL);

	int nRC = qcCreateIO (&m_fIO, qcGetSourceProtocol (szSource));
	if (nRC < 0)
		return nRC;

	m_fParser.pBuffMng = m_pBuffMng;
	qcCreateParser (&m_fParser, qcGetSourceFormat (szSource));
	if (m_fParser.hParser == NULL)
		return QC_ERR_FORMAT;

	nRC = m_fParser.Open (m_fParser.hParser, &m_fIO, szSource, 0);
	if (nRC < 0)
		return nRC;

	for (int i = 0; i < 1000; i++)
	{
		m_buffInfo.nMediaType = QC_MEDIA_Audio;
		nRC = m_fParser.Read (m_fParser.hParser, &m_buffInfo);
		m_buffInfo.nMediaType = QC_MEDIA_Video;
		nRC = m_fParser.Read (m_fParser.hParser, &m_buffInfo);
	}


	if (m_fParser.hParser != NULL)
	{
		m_fParser.Close (m_fParser.hParser);
		qcDestroyParser (&m_fParser);
	}

	if (m_fIO.hIO != NULL)
	{
		qcDestroyIO (&m_fIO);
	}

	QC_DEL_P (m_pBuffMng);

	return 0;
}

int qcTestParserClose (void)
{
	return 0;
}