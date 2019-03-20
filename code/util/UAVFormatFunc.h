/*******************************************************************************
	File:		UAVFormatFunc.h

	Contains:	The audio video format func header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#ifndef __UAVFormatFunc_H__
#define __UAVFormatFunc_H__

#include "qcType.h"
#include "qcData.h"

QC_VIDEO_FORMAT *		qcavfmtCloneVideoFormat (QC_VIDEO_FORMAT * pFmt);
int						qcavfmtDeleteVideoFormat (QC_VIDEO_FORMAT * pFmt);

QC_AUDIO_FORMAT *		qcavfmtCloneAudioFormat (QC_AUDIO_FORMAT * pFmt);
int						qcavfmtDeleteAudioFormat (QC_AUDIO_FORMAT * pFmt);

QC_DATA_BUFF *			CloneBuff(QC_DATA_BUFF * pSrcBuff, QC_DATA_BUFF * pDstBuff);

#endif // __UAVFormatFunc_H__
