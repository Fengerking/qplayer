/*******************************************************************************
	File:		UAVFormatFunc.cpp

	Contains:	The audio video format func implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "qcDef.h"
#include "UAVFormatFunc.h"

QC_VIDEO_FORMAT * qcavfmtCloneVideoFormat (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return NULL;
	QC_VIDEO_FORMAT * pNewFmt = new QC_VIDEO_FORMAT ();
	memcpy (pNewFmt, pFmt, sizeof (QC_VIDEO_FORMAT));
	if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
	{
		pNewFmt->pHeadData = new unsigned char[pFmt->nHeadSize];
		memcpy (pNewFmt->pHeadData, pFmt->pHeadData, pFmt->nHeadSize);
		pNewFmt->nHeadSize = pFmt->nHeadSize;
	}
	return pNewFmt;
}

int	qcavfmtDeleteVideoFormat (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	if (pFmt->pHeadData != NULL)
		delete []pFmt->pHeadData;
	delete pFmt;
	return QC_ERR_NONE;
}

QC_AUDIO_FORMAT * qcavfmtCloneAudioFormat (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return NULL;
	QC_AUDIO_FORMAT * pNewFmt = new QC_AUDIO_FORMAT ();
	memcpy (pNewFmt, pFmt, sizeof (QC_AUDIO_FORMAT));
	if (pFmt->pHeadData != NULL && pFmt->nHeadSize > 0)
	{
		pNewFmt->pHeadData = new unsigned char[pFmt->nHeadSize];
		memcpy (pNewFmt->pHeadData, pFmt->pHeadData, pFmt->nHeadSize);
		pNewFmt->nHeadSize = pFmt->nHeadSize;
	}
	return pNewFmt;
}

int	qcavfmtDeleteAudioFormat (QC_AUDIO_FORMAT * pFmt)
{
	if (pFmt == NULL)
		return QC_ERR_ARG;
	if (pFmt->pHeadData != NULL)
		delete []pFmt->pHeadData;
	delete pFmt;
	return QC_ERR_NONE;
}

QC_DATA_BUFF *	CloneBuff(QC_DATA_BUFF * pSrcBuff, QC_DATA_BUFF * pDstBuff)
{
	if (pSrcBuff == NULL)
		return NULL;

	if (pDstBuff == NULL)
	{
		pDstBuff = new QC_DATA_BUFF();
		memset(pDstBuff, 0, sizeof(QC_DATA_BUFF));
	}
	if (pDstBuff->uBuffSize < pSrcBuff->uSize)
	{
		QC_DEL_A(pDstBuff->pBuff);
		pDstBuff->uBuffSize = 0;
	}
	if (pDstBuff->pBuff == NULL)
	{
		pDstBuff->uBuffSize = pSrcBuff->uSize + 1024;
		pDstBuff->pBuff = new unsigned char[pDstBuff->uBuffSize];
	}
	unsigned char * pDstData = pDstBuff->pBuff;
	unsigned int	nDstSize = pDstBuff->uBuffSize;
	memcpy(pDstBuff, pSrcBuff, sizeof (QC_DATA_BUFF));
	memcpy(pDstData, pSrcBuff->pBuff, pSrcBuff->uSize);
	pDstBuff->uBuffSize = nDstSize;
	pDstBuff->pBuff = pDstData;

	return pDstBuff;
}