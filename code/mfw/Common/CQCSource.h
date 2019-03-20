/*******************************************************************************
	File:		CQCSource.h

	Contains:	The qc own source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-18		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCSource_H__
#define __CQCSource_H__

#include "CBaseSource.h"

class CQCSource : public CBaseSource
{
public:
	CQCSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCSource(void);

	virtual int			Open (const char * pSource, int nType);
	virtual int			OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close (void);

	virtual int			Start (void);
	virtual int			Pause (void);
	virtual int			Stop (void);

	virtual int			ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);
	virtual int			ReadParserBuff(QC_DATA_BUFF * pBuffInfo);

	virtual int			CanSeek (void);
	virtual long long	SetPos (long long llPos);
	virtual int			SetStreamPlay (QCMediaType nType, int nStream);

	virtual void		EnableSubtt (bool bEnable) {m_bSubttEnable = bEnable;}

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

	virtual QC_STREAM_FORMAT *	GetStreamFormat (int nID = 0);
	virtual QC_AUDIO_FORMAT *	GetAudioFormat (int nID = 0);
	virtual QC_VIDEO_FORMAT *	GetVideoFormat (int nID = 0);
	virtual QC_SUBTT_FORMAT *	GetSubttFormat (int nID = 0);

	virtual QCParserFormat		GetParserFormat(void) { return m_nFormat; }
    virtual QCIOType            GetIOType();
	QC_Parser_Func *			GetParserFunc(void) { return &m_fParser; }

protected:
	virtual int			OpenSource(const char * pSource, int nType);
	virtual int			OpenSameSource(const char * pSource, int nType);

	virtual int			CreateParser (QCIOProtocol nProtocol, QCParserFormat nFormat);
	virtual int			DestroyParser (void);

	virtual int			UpdateProtocolFormat(const char * pSource);
	virtual int			UpdateInfo (void);

	virtual int			OnWorkItem (void);

protected:
	QC_Parser_Func		m_fParser;

	QC_STREAM_FORMAT *	m_pFmtStreamGet;
	QC_AUDIO_FORMAT *	m_pFmtAudioGet;
	QC_VIDEO_FORMAT *	m_pFmtVideoGet;
	QC_SUBTT_FORMAT *	m_pFmtSubttGet;

	CMutexLock			m_lckSeek;
	bool				m_bNewStream;
	int					m_nHadReadVideo;

	QCIOProtocol		m_nProtocol;
	QCParserFormat		m_nFormat;

	char				m_szQiniuDrmKey[128];
};

#endif // __CQCSource_H__
