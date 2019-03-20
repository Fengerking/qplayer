/*******************************************************************************
	File:		CQCVideoEnc.cpp

	Contains:	qc Video encoder implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CQCVideoEnc.h"
#include "ULogFunc.h"

CQCVideoEnc::CQCVideoEnc(CBaseInst * pBaseInst, void * hInst)
	: CBaseObject (pBaseInst)
	, m_hInst (hInst)
	, m_hLib (NULL)
	, m_hEnc (NULL)
	, m_fEncImage (NULL)
{
	SetObjectName ("CQCVideoEnc");
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
}

CQCVideoEnc::~CQCVideoEnc(void)
{
	Uninit ();
}

int CQCVideoEnc::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();
#ifdef __QC_LIB_ONE__
	int nRC = qcCreateEncoder (&m_hEnc, pFmt);
	m_fEncImage = qcEncodeImage;
#else
	if (m_pBaseInst->m_hLibCodec == NULL)
		m_hLib = (qcLibHandle)qcLibLoad("qcCodec", 0);
	else
		m_hLib = m_pBaseInst->m_hLibCodec;
	if (m_hLib == NULL)
		return QC_ERR_FAILED;
	QCCREATEENCODER pCreate = (QCCREATEENCODER)qcLibGetAddr(m_hLib, "qcCreateEncoder", 0);
	if (pCreate == NULL)
		return QC_ERR_FAILED;
	int nRC = pCreate (&m_hEnc, pFmt);
	if (nRC != QC_ERR_NONE)
	{
		QCLOGW ("Create QC video enc failed. err = 0X%08X", nRC);
		return nRC;
	}
	m_fEncImage = (QCENCODEIMAGE)qcLibGetAddr(m_hLib, "qcEncodeImage", 0);
	if (m_fEncImage == NULL)
		return QC_ERR_FAILED;
#endif // __QC_LIB_ONE__

	memcpy(&m_fmtVideo, pFmt, sizeof(m_fmtVideo));

	return QC_ERR_NONE;
}

int CQCVideoEnc::Uninit(void)
{
#ifdef __QC_LIB_ONE__
	if (m_hEnc != NULL)
	{
		qcDestroyEncoder(m_hEnc);
		m_hEnc = NULL;
	}
#else
	if (m_hLib != NULL)
	{
		QCDESTROYENCODER fDestroy = (QCDESTROYENCODER)qcLibGetAddr(m_hLib, "qcDestroyEncoder", 0);
		if (fDestroy != NULL)
			fDestroy (m_hEnc);
		if (m_pBaseInst->m_hLibCodec == NULL)
			qcLibFree(m_hLib, 0);
		m_hLib = NULL;
	}
#endif // __QC_LIB_ONE__
	return QC_ERR_NONE;
}

int CQCVideoEnc::EncodeImage(QC_VIDEO_BUFF * pVideo, QC_DATA_BUFF * pData)
{
	if (m_fEncImage == NULL)
		return QC_ERR_STATUS;

	int nRC = m_fEncImage(m_hEnc, pVideo, pData);

	return nRC;
}
