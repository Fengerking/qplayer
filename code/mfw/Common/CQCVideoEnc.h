/*******************************************************************************
	File:		CQCVideoEnc.h

	Contains:	The qc video enc wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-29		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCVideoEnc_H__
#define __CQCVideoEnc_H__
#include "qcCodec.h"
#include "CBaseObject.h"
#include "ULibFunc.h"

class CQCVideoEnc : public CBaseObject
{
public:
	CQCVideoEnc(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCVideoEnc(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		EncodeImage(QC_VIDEO_BUFF * pVideo, QC_DATA_BUFF * pData);

protected:
	void *				m_hInst;
	qcLibHandle			m_hLib;
	void *				m_hEnc;

	QCENCODEIMAGE		m_fEncImage;

	QC_VIDEO_FORMAT		m_fmtVideo;
};

#endif // __CQCVideoEnc_H__
