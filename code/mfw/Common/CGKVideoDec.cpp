/*******************************************************************************
	File:		CGKVideoDec.cpp

	Contains:	The gk video dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "AVCDecoderTypes.h"

#include "CGKVideoDec.h"
#include "ULogFunc.h"

CGKVideoDec::CGKVideoDec(CBaseInst * pBaseInst, void * hInst)
	: CBaseVideoDec(pBaseInst, hInst)
	, m_hLib (NULL)
	, m_hDec (NULL)
{
	SetObjectName ("CGKVideoDec");
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
	memset (&m_fAPI, 0, sizeof (m_fAPI));
	memset (&m_Input, 0, sizeof (m_Input));

	memset (&m_Input, 0, sizeof (m_Input));
	memset (&m_Output, 0, sizeof (m_Output));
	memset (&m_OutputInfo, 0, sizeof (m_OutputInfo));

}

CGKVideoDec::~CGKVideoDec(void)
{
	Uninit ();
}

int CGKVideoDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;

	Uninit ();

	m_ttFmt.Width = pFmt->nWidth;
	m_ttFmt.Height = pFmt->nHeight;
	if (pFmt->nCodecID == QC_CODEC_ID_H264)
	{
		m_hLib = (qcLibHandle)qcLibLoad ("qcH264Dec", 0);
		if (m_hLib == NULL)
			return QC_ERR_FAILED;
		__GetVideoDECAPI pGetAPI = (__GetVideoDECAPI)qcLibGetAddr (m_hLib, "ttGetH264DecAPI", 0);
		pGetAPI (&m_fAPI);
	}
	if (m_fAPI.Open == NULL)
		return QC_ERR_FAILED;

	int nRC = m_fAPI.Open (&m_hDec);
	if (m_hDec == NULL)
		return QC_ERR_FAILED;

	if(pFmt->nCodecID == QC_CODEC_ID_H264)
	{
		nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_FORMAT, &m_ttFmt);
		if (pFmt->pPrivateData != NULL && pFmt->nPrivateFlag == 1)
		{
			TTAVCDecoderSpecificInfo * pDecInfo = (TTAVCDecoderSpecificInfo *)pFmt->pPrivateData;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_DECODER_INFO, pDecInfo);
		}
		else if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
		{
			TTAVCDecoderSpecificInfo DecInfo;
			memset (&DecInfo, 0, sizeof (DecInfo));
			DecInfo.iData = pFmt->pHeadData;
			DecInfo.iSize = pFmt->nHeadSize;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_DECODER_INFO, &DecInfo);
		}
	}
	memset (&m_Input, 0, sizeof (m_Input));
	CBaseVideoDec::Init (pFmt);
    
    if (m_pBuffData == NULL)
    {
        m_pBuffData = new QC_DATA_BUFF ();
        memset (m_pBuffData, 0, sizeof (QC_DATA_BUFF));
        m_pBuffData->uFlag = QC_BUFF_TYPE_Video;
        m_pBuffData->uBuffType = QC_BUFF_TYPE_Video;
        m_pBuffData->pBuffPtr = &m_buffVideo;
    }

	return QC_ERR_NONE;
}

int CGKVideoDec::Uninit (void)
{
	if (m_hDec != NULL)
		m_fAPI.Close (m_hDec);
	m_hDec = NULL;
	if (m_hLib != NULL)
	{
		qcLibFree (m_hLib, 0);
		m_hLib = NULL;
	}
    QC_DEL_P(m_pBuffData);
	CBaseVideoDec::Uninit ();
	return QC_ERR_NONE;
}

int CGKVideoDec::Flush (void)
{
	CAutoLock lock (&m_mtBuffer);
	if (m_hDec != NULL)
	{
		unsigned int	nFlush = 1;	
		m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_FLUSH, &nFlush);
	}
	memset (&m_Input, 0, sizeof (m_Input));
	return QC_ERR_NONE;
}

int CGKVideoDec::PushRestOut (void)
{
	CAutoLock lock (&m_mtBuffer);
	if (m_hDec != NULL)
	{
		unsigned int	nFlush = 1;	
		m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_FLUSHALL, &nFlush);
	}
	memset (&m_Input, 0, sizeof (m_Input));
	return QC_ERR_NONE;
}

int CGKVideoDec::SetBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	CBaseVideoDec::SetBuff (pBuff);

	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		if (m_Input.pBuffer != NULL)
			Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL && pFmt->pHeadData != NULL)
			InitNewFormat (pFmt);
	}	
	if (pBuff->uBuffType == QC_BUFF_TYPE_Data)
	{
		m_Input.pBuffer = pBuff->pBuff;
		m_Input.nSize = pBuff->uSize;
		m_Input.llTime = pBuff->llTime;
	}
	else
	{
		return QC_ERR_UNSUPPORT;
	}

	int nEnDeblock = 1;
	if (pBuff->llDelay >= 30)
		nEnDeblock = 0;
//	m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_ENDEBLOCK, &nEnDeblock);
	
	int nRC = m_fAPI.SetInput (m_hDec, &m_Input);
	if (nRC != TTKErrNone)
		return QC_ERR_RETRY;

	return QC_ERR_NONE;
}

int CGKVideoDec::GetBuff (QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL || m_hDec == NULL)
		return QC_ERR_ARG;

	CAutoLock lock (&m_mtBuffer);
	if (m_Input.nSize <= 0)
		return QC_ERR_RETRY;
    if (m_pBuffData != NULL)
        m_pBuffData->uFlag = 0;

	int nRC = m_fAPI.Process (m_hDec, &m_Output, &m_OutputInfo);
	if (nRC != TTKErrNone)
	{
		m_Input.nSize = 0;
		return QC_ERR_FAILED;
	}
	m_buffVideo.pBuff[0] = m_Output.Buffer[0];
	m_buffVideo.pBuff[1] = m_Output.Buffer[1];
	m_buffVideo.pBuff[2] = m_Output.Buffer[2];
	m_buffVideo.nStride[0] = m_Output.Stride[0];
	m_buffVideo.nStride[1] = m_Output.Stride[1];
	m_buffVideo.nStride[2] = m_Output.Stride[2];
	m_buffVideo.nType = QC_VDT_YUV420_P;

	m_pBuffData->llTime = m_Output.Time;
	m_pBuffData->uBuffType = QC_BUFF_TYPE_Video;
	if (m_fmtVideo.nWidth != m_OutputInfo.Width || m_fmtVideo.nHeight !=m_OutputInfo.Height)
	{
		m_fmtVideo.nWidth = m_OutputInfo.Width;
		m_fmtVideo.nHeight =m_OutputInfo.Height;
		m_pBuffData->uFlag |= QCBUFF_NEW_FORMAT;
		m_pBuffData->pFormat = &m_fmtVideo;
	}

	CBaseVideoDec::GetBuff (&m_pBuffData);
	*ppBuff = m_pBuffData;

	return QC_ERR_NONE;
}

int CGKVideoDec::InitNewFormat (QC_VIDEO_FORMAT * pFmt)
{
	if (m_hLib == NULL)
		return Init (pFmt);

	if (m_hDec != NULL)
		m_fAPI.Close (m_hDec);
	int nRC = m_fAPI.Open (&m_hDec);
	if (m_hDec == NULL)
		return QC_ERR_FAILED;
	m_ttFmt.Width = pFmt->nWidth;
	m_ttFmt.Height = pFmt->nHeight;
	if(pFmt->nCodecID == QC_CODEC_ID_H264)
	{
		nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_FORMAT, &m_ttFmt);
		if (pFmt->pPrivateData != NULL && pFmt->nPrivateFlag == 1)
		{
			TTAVCDecoderSpecificInfo * pDecInfo = (TTAVCDecoderSpecificInfo *)pFmt->pPrivateData;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_DECODER_INFO, pDecInfo);
		}
		else if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
		{
			TTAVCDecoderSpecificInfo DecInfo;
			memset (&DecInfo, 0, sizeof (DecInfo));
			DecInfo.iData = pFmt->pHeadData;
			DecInfo.iSize = pFmt->nHeadSize;
			nRC = m_fAPI.SetParam (m_hDec, TT_PID_VIDEO_DECODER_INFO, &DecInfo);
		}
	}
	return QC_ERR_NONE;
}
