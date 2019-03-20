/*******************************************************************************
	File:		CQCFFSource.h

	Contains:	The qc ffmpeg source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-16		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCFFSource_H__
#define __CQCFFSource_H__

#include "CQCSource.h"

#include "ULibFunc.h"

class CQCFFSource : public CQCSource
{
public:
	CQCFFSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCFFSource(void);

    virtual int         Open (const char * pSource, int nType);
    virtual int         OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close(void);

	virtual int			ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

protected:
	virtual int			CreateParser (QCIOProtocol nProtocol, QCParserFormat	nFormat);
	virtual int			DestroyParser (void);

	virtual int			ReadParserBuff (QC_DATA_BUFF * pBuffInfo);

	virtual int			CreateHeadBuff(QC_DATA_BUFF * pBuffInfo);
    
    virtual void        OnOpenDone(const char * pURL);
    virtual void        ReleaseResInfo();


protected:
	qcLibHandle			m_hLib;

	QC_DATA_BUFF		m_datBuff;
	QC_DATA_BUFF *		m_pVideoBuff;
	long long			m_llLastVideoTime;

	bool				m_bReadVideoHead;
	bool				m_bReadAudioHead;

	bool				m_bFindKeyFrame;
    QC_RESOURCE_INFO    m_sResInfo;

};

#endif // __CQCFFSource_H__
