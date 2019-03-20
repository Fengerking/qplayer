/*******************************************************************************
	File:		CBaseSource.h

	Contains:	The base source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseSource_H__
#define __CBaseSource_H__
#include "qcData.h"
#include "qcParser.h"

#include "CBaseObject.h"
#include "CBuffMng.h"
#include "CThreadWork.h"
#include "CMutexLock.h"

class CBaseSource : public CBaseObject , public CThreadFunc
{
public:
	CBaseSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBaseSource(void);

	virtual int			Open (const char * pSource, int nType);
	virtual int			OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close (void);

	virtual int			Start (void);
	virtual int			Pause (void);
	virtual int			Stop (void);

	virtual int			ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);

	virtual int			CanSeek (void);
	virtual long long	SetPos (long long llPos);
	virtual long long	GetPos (void);

	virtual long long	GetBuffTime (bool bAudio);
	virtual int			SetBuffTimer (long long llBuffTime);
	virtual void		EnableSubtt (bool bEnable) {m_bSubttEnable = bEnable;}
	virtual char *		GetSourceName (void) {return m_pSourceName;}

	virtual int			GetStreamCount (QCMediaType nType);
	virtual int			GetStreamPlay (QCMediaType nType);
	virtual int			SetStreamPlay (QCMediaType nType, int nStream);
	virtual long long	GetDuration (void);
	virtual bool		IsEOS (void);
	virtual bool		IsLive (void);
	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

	virtual QC_STREAM_FORMAT *	GetStreamFormat (int nID = 0);
	virtual QC_AUDIO_FORMAT *	GetAudioFormat (int nID = 0);
	virtual QC_VIDEO_FORMAT *	GetVideoFormat (int nID = 0);
	virtual QC_SUBTT_FORMAT *	GetSubttFormat (int nID = 0);

	virtual QCParserFormat		GetSourceFormat (const char * pSource);
	virtual QCParserFormat		GetParserFormat(void) { return QC_PARSER_NONE; }
    virtual QCIOType			GetIOType(){return QC_IOTYPE_NONE;};

	virtual void				SetOpenCache(int nOpenCache) { m_nOpenCache = nOpenCache; }

protected:
	virtual int			UpdateInfo (void);

protected:
	void *				m_hInst;
	char *				m_pSourceName;
	int					m_nSourceType;
	QC_IO_Func *		m_pIO;
	QC_IO_Func			m_fIO;
	CMutexLock			m_lckParser;
	CMutexLock			m_lckReader;

	int					m_nStmSourceNum;
	int					m_nStmVideoNum;
	int					m_nStmAudioNum;
	int					m_nStmSubttNum;
	int					m_nStmSourcePlay;
	int					m_nStmSourceSel;
	int					m_nStmVideoPlay;
	int					m_nStmAudioPlay;
	int					m_nStmSubttPlay;

	QC_STREAM_FORMAT *	m_pFmtStream;
	QC_AUDIO_FORMAT *	m_pFmtAudio;
	QC_VIDEO_FORMAT *	m_pFmtVideo;
	QC_SUBTT_FORMAT *	m_pFmtSubtt;

	long long			m_llDuration;
	long long			m_llMaxBuffTime;
	long long			m_llMinBuffTime;
	bool				m_bNeedBuffing;

	long long			m_llSeekPos;
	bool				m_bVideoNewPos;
	bool				m_bAudioNewPos;
	bool				m_bSubttNewPos;

	bool				m_bEOS;
	bool				m_bEOA;
	bool				m_bEOV;
	int					m_nBuffRead;
	
	bool				m_bSourceLive;
	bool				m_bSubttEnable;
	int					m_nOpenCache;

	CBuffMng *			m_pBuffMng;
	QC_DATA_BUFF		m_buffInfo;

	CThreadWork *		m_pReadThread;
};

#endif // __CBaseSource_H__
