/*******************************************************************************
	File:		CFFMpegVideoDec.h

	Contains:	The ffmpeg video dec header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-15		Bangfei			Create file

*******************************************************************************/
#ifndef __CFFMpegVideoDec_H__
#define __CFFMpegVideoDec_H__
#include "qcFF.h"

#include "CBaseVideoDec.h"

#include "ULibFunc.h"

class CFFMpegVideoDec : public CBaseVideoDec
{
public:
	CFFMpegVideoDec(void * hInst);
	virtual ~CFFMpegVideoDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);
	virtual int		FlushAll (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	qcLibHandle				m_hLib;
    AVCodecContext *		m_pDecCtx;
    AVCodecContext *		m_pNewCtx;
	AVCodec *				m_pDecVideo;
	AVFrame *				m_pFrmVideo;

	AVPacket				m_pktData;
	AVPacket *				m_pPacket;
};

#endif // __CFFMpegVideoDec_H__
