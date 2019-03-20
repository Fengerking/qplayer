/*******************************************************************************
	File:		COpenHEVCDec.cpp

	Contains:	The openHEVC video dec wrap implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "COpenHEVCDec.h"

#include "USystemFunc.h"
#include "ULibFunc.h"
#include "ULogFunc.h"

COpenHEVCDec::COpenHEVCDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoDec(pBaseInst, hInst)
	, m_hDll (NULL)
	, m_hDec (NULL)
	, m_fInit (NULL)
	, m_fStart (NULL)
	, m_fDec (NULL)
	, m_fGetPicInfo (NULL)
	, m_fCopyExtData (NULL)
	, m_fGetPicSize2 (NULL)
	, m_fGetOutput (NULL)
	, m_fGetOutputCopy (NULL)
	, m_fSetCheckMD5 (NULL)
	, m_fSetDebugMode (NULL)
	, m_fSetTempLayer (NULL)
	, m_fSetNoCrop (NULL)
	, m_fSetActiveDec (NULL)
	, m_fClose (NULL)
	, m_fFlush (NULL)
	, m_fGetVer (NULL)
	, m_llNewPos (0)
{
	SetObjectName ("COpenHEVCDec");
}

COpenHEVCDec::~COpenHEVCDec(void)
{
	Uninit ();
}

int COpenHEVCDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();

	m_hDll = qcLibLoad (("H265Dec"), 0);
	if (m_hDll == NULL)
		return QC_ERR_FAILED;

	m_fInit			= (LIBOPENHEVCINIT) qcLibGetAddr (m_hDll, "libOpenHevcInit", 0);
	m_fStart		= (LIBOPENHEVCSTARTDECODER) qcLibGetAddr (m_hDll, "libOpenHevcStartDecoder", 0);
	m_fDec			= (LIBOPENHEVCDECODE) qcLibGetAddr (m_hDll, "libOpenHevcDecode", 0);
	m_fGetPicInfo	= (LIBOPENHEVCGETPICTUREINFO) qcLibGetAddr (m_hDll, "libOpenHevcGetPictureInfo", 0);
	m_fCopyExtData	= (LIBOPENHEVCCOPYEXTRADATA) qcLibGetAddr (m_hDll, "libOpenHevcCopyExtraData", 0);
	m_fGetPicSize2	= (LIBOPENHEVCGETPICTURESIZE2) qcLibGetAddr (m_hDll, "libOpenHevcGetPictureSize2", 0);
	m_fGetOutput	= (LIBOPENHEVCGETOUTPUT) qcLibGetAddr (m_hDll, "libOpenHevcGetOutput", 0);
	m_fGetOutputCopy = (LIBOPENHEVCGETOUTPUTCPY) qcLibGetAddr (m_hDll, "libOpenHevcGetOutputCpy", 0);
	m_fSetCheckMD5	= (LIBOPENHEVCSETCHECKMD5) qcLibGetAddr (m_hDll, "libOpenHevcSetCheckMD5", 0);
	m_fSetDebugMode = (LIBOPENHEVCSETDEBUGMODE) qcLibGetAddr (m_hDll, "libOpenHevcSetDebugMode", 0);
	m_fSetTempLayer = (LIBOPENHEVCSETTEMPORALLAYER_ID) qcLibGetAddr (m_hDll, "libOpenHevcSetTemporalLayer_id", 0);
	m_fSetNoCrop	= (LIBOPENHEVCSETNOCROPPING) qcLibGetAddr (m_hDll, "libOpenHevcSetNoCropping", 0);
	m_fSetActiveDec = (LIBOPENHEVCSETACTIVEDECODERS) qcLibGetAddr (m_hDll, "libOpenHevcSetActiveDecoders", 0);
	m_fClose		= (LIBOPENHEVCCLOSE) qcLibGetAddr (m_hDll, "libOpenHevcClose", 0);
	m_fFlush		= (LIBOPENHEVCFLUSH) qcLibGetAddr (m_hDll, "libOpenHevcFlush", 0);
	m_fGetVer		= (LIBOPENHEVCVERSION) qcLibGetAddr (m_hDll, "libOpenHevcVersion", 0);
	if (m_fInit == NULL || m_fClose == NULL)
		return QC_ERR_FAILED;

	QCLOGI ("00000");

	int	nRC = 0;
	int nCPUNum = qcGetCPUNum ();
	m_hDec = m_fInit (1, 0); // FF_THREAD_FRAME
	if (m_hDec == NULL)
		return QC_ERR_FAILED;
	QCLOGI ("111111");
//	m_fSetActiveDec (m_hDec, 1);
//	nRC = m_fStart (m_hDec);

	CBaseVideoDec::Init (pFmt);

	if (pFmt->nHeadSize > 0 && pFmt->pHeadData != NULL)
		nRC = m_fDec (m_hDec, pFmt->pHeadData, pFmt->nHeadSize, 0);
	memset (&m_frmCopy, 0, sizeof (m_frmCopy));
	m_frmCopy.frameInfo.nBitDepth = 8;
	m_frmCopy.frameInfo.nYPitch = m_fmtVideo.nWidth;
	m_frmCopy.frameInfo.nUPitch = m_frmCopy.frameInfo.nYPitch / 2;
	m_frmCopy.frameInfo.nVPitch = m_frmCopy.frameInfo.nYPitch / 2;
	m_frmCopy.frameInfo.nWidth = m_fmtVideo.nWidth;
	m_frmCopy.frameInfo.nHeight = m_fmtVideo.nHeight;
	m_frmCopy.pvY = new unsigned char [m_fmtVideo.nWidth * m_fmtVideo.nHeight];
	m_frmCopy.pvU = new unsigned char [m_fmtVideo.nWidth * m_fmtVideo.nHeight / 4];
	m_frmCopy.pvV = new unsigned char [m_fmtVideo.nWidth * m_fmtVideo.nHeight / 4];

	return QC_ERR_NONE;
}

int COpenHEVCDec::Uninit (void)
{
	if (m_hDec == NULL)
		return QC_ERR_NONE;

	m_fClose (m_hDec);
	m_hDec = NULL;

	qcLibFree (m_hDll, 0);
	m_hDll = NULL;

	CBaseVideoDec::Uninit ();
	return QC_ERR_NONE;
}

int COpenHEVCDec::Flush (void)
{
	if (m_hDec == NULL)
		return QC_ERR_FAILED;

	CAutoLock lock (&m_mtBuffer);

	m_fFlush (m_hDec);

	return QC_ERR_NONE;
}

int COpenHEVCDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseVideoDec::SetBuff (pBuff);
	QCLOGI ("22222 Flag = % 8d size % 8d,  Time % 8d", pBuff->uFlag, pBuff->uSize, (int)pBuff->llTime);	
	int nRC = QC_ERR_NONE;
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
			Flush ();
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && pFmt->pHeadData != NULL)
			nRC = m_fDec (m_hDec, pFmt->pHeadData, pFmt->nHeadSize, 0);
	}	
	if ((pBuff->uBuffType == QC_BUFF_TYPE_Data))
		nRC = m_fDec (m_hDec, pBuff->pBuff, pBuff->uSize, pBuff->llTime);
	else
		return QC_ERR_UNSUPPORT;
	QCLOGI ("333333  == %d", nRC);
	if (nRC == 0)
		return QC_ERR_NEEDMORE;
	else if (nRC == 1)
		return QC_ERR_NONE;

	return QC_ERR_NONE;
}

int COpenHEVCDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	CAutoLock lock (&m_mtBuffer);
//	memset (&m_frmVideo, 0, sizeof (m_frmVideo));
//	int nRC = m_fGetOutput (m_hDec, 1, &m_frmVideo);
    if (m_pBuffData != NULL)
        m_pBuffData->uFlag = 0;

	QCLOGI ("444444");
	m_frmCopy.frameInfo.nYPitch = 0;
	int nRC = m_fGetOutputCopy (m_hDec, 1, &m_frmCopy);

	QCLOGI ("55555  == %d", nRC);

	if (m_frmCopy.frameInfo.nYPitch == 0)
		return QC_ERR_RETRY;

//	if (m_llNewPos > 0 && abs ((int)(m_frmCopy.frameInfo.nTimeStamp - m_llNewPos)) > 1800)
//		return QC_ERR_RETRY;
//	m_llNewPos = 0;

	if (m_frmCopy.frameInfo.nWidth != m_fmtVideo.nWidth || m_frmCopy.frameInfo.nHeight != m_fmtVideo.nHeight)
	{
		m_fmtVideo.nWidth = m_frmCopy.frameInfo.nWidth;
		m_fmtVideo.nHeight = m_frmCopy.frameInfo.nHeight;
		m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
		m_pBuffData->pFormat = &m_fmtVideo;
	}

	m_buffVideo.pBuff[0] = (unsigned char *)m_frmCopy.pvY;
	m_buffVideo.pBuff[1] = (unsigned char *)m_frmCopy.pvU;
	m_buffVideo.pBuff[2] = (unsigned char *)m_frmCopy.pvV;
	m_buffVideo.nStride[0] = m_frmCopy.frameInfo.nYPitch;
	m_buffVideo.nStride[1] = m_frmCopy.frameInfo.nUPitch;
	m_buffVideo.nStride[2] = m_frmCopy.frameInfo.nVPitch;

	m_buffVideo.nWidth = m_frmCopy.frameInfo.nWidth;
	m_buffVideo.nHeight = m_frmCopy.frameInfo.nHeight;
	m_buffVideo.nType = QC_VDT_YUV420_P;

	m_pBuffData->llTime = m_frmCopy.frameInfo.nTimeStamp;
	m_pBuffData->uBuffType = QC_BUFF_TYPE_Video;

	CBaseVideoDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;

	return QC_ERR_NONE;
}

