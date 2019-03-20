/*******************************************************************************
	File:		qcColorCvtR.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2018-03-13		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcCodec.h"

#ifdef __QC_OS_NDK__
#include <unistd.h>
#include <android/log.h>
#endif // __QC_OS_NDK__

#if defined(__QC_OS_IOS__) || defined(__QC_OS_MACOS__)
#include <sys/sysctl.h>
#endif

#include "libyuv.h"

#include "qcFFLog.h"

int	qcColorCvtRotate(QC_VIDEO_BUFF * pSrcVideo, QC_VIDEO_BUFF * pDstVideo, int nAngle)
{
	if (pSrcVideo == NULL || pDstVideo == NULL)
		return QC_ERR_ARG;

	int nRC = QC_ERR_NONE;
	if (nAngle != 0)
	{
		RotationModeEnum nRTTMode = kRotate0;
		if (nAngle == 90)
			nRTTMode = kRotate90;
		else if (nAngle == 180)
			nRTTMode = kRotate180;
		else if (nAngle == 270)
			nRTTMode = kRotate270;

		if (pSrcVideo->nType == QC_VDT_YUV420_P)
		{
			nRC = I420Rotate(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->pBuff[1], pDstVideo->nStride[1],
							 pDstVideo->pBuff[2], pDstVideo->nStride[2],
							 pSrcVideo->nWidth, pSrcVideo->nHeight, nRTTMode);
		}
        else if (pSrcVideo->nType == QC_VDT_ARGB)
        {
            nRC = ARGBRotate(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
                       pDstVideo->pBuff[0], pDstVideo->nStride[0],
                       pSrcVideo->nWidth, pSrcVideo->nHeight, nRTTMode);
        }

		return nRC;
	}

	if (pSrcVideo->nType == QC_VDT_YUV420_P)
	{
		if (pDstVideo->nType == QC_VDT_ARGB)
		{
			nRC = I420ToARGB(pSrcVideo->pBuff[0], pSrcVideo->nStride[0], 
							 pSrcVideo->pBuff[2], pSrcVideo->nStride[2], 
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
		if (pDstVideo->nType == QC_VDT_RGBA)
		{
			nRC = I420ToARGB(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
		else if (pDstVideo->nType == QC_VDT_RGB24)
		{
			nRC = I420ToRGB24(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							  pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							  pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							  pDstVideo->pBuff[0], pDstVideo->nStride[0],
							  pDstVideo->nWidth, pDstVideo->nHeight);
		}
		else if (pDstVideo->nType == QC_VDT_RGB565)
		{
			nRC = I420ToRGB565(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							   pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							   pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							   pDstVideo->pBuff[0], pDstVideo->nStride[0],
							   pDstVideo->nWidth, pDstVideo->nHeight);
		}
	}
	else if (pDstVideo->nType == QC_VDT_YUV420_P)
	{
		if (pSrcVideo->nType == QC_VDT_YUV422_P)
		{
			nRC = I422ToI420(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->pBuff[1], pDstVideo->nStride[1],
							 pDstVideo->pBuff[2], pDstVideo->nStride[2],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
		else if (pSrcVideo->nType == QC_VDT_YUV444_P)
		{
			nRC = I444ToI420(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pSrcVideo->pBuff[2], pSrcVideo->nStride[2],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->pBuff[1], pDstVideo->nStride[1],
							 pDstVideo->pBuff[2], pDstVideo->nStride[2],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
		else if (pSrcVideo->nType == QC_VDT_NV12)
		{
			nRC = NV12ToI420(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->pBuff[1], pDstVideo->nStride[1],
							 pDstVideo->pBuff[2], pDstVideo->nStride[2],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
		else if (pSrcVideo->nType == QC_VDT_YUYV422)
		{
			nRC = YUY2ToI420(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
							 pDstVideo->pBuff[0], pDstVideo->nStride[0],
							 pDstVideo->pBuff[1], pDstVideo->nStride[1],
							 pDstVideo->pBuff[2], pDstVideo->nStride[2],
							 pDstVideo->nWidth, pDstVideo->nHeight);
		}
	}
    else if (pSrcVideo->nType == QC_VDT_NV12 && pDstVideo->nType == QC_VDT_ARGB)
    {
        nRC = NV12ToARGB(pSrcVideo->pBuff[0], pSrcVideo->nStride[0],
                         pSrcVideo->pBuff[1], pSrcVideo->nStride[1],
                         pDstVideo->pBuff[0], pDstVideo->nStride[0],
                         pSrcVideo->nWidth, pSrcVideo->nHeight);
    }

	return nRC;
}
