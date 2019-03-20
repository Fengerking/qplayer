/*******************************************************************************
	File:		CBoxSource.h

	Contains:	the source box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxSource_H__
#define __CBoxSource_H__

#include "CBoxBase.h"
#include "CBaseSource.h"
#include "CMutexLock.h"

#include "CNodeList.h"

typedef struct
{
	char *			pSource;
	QCParserFormat	nSrcFormat;
	QC_IO_Func *	pIO;
} QC_SRC_Cache;

class CBoxSource : public CBoxBase
{
public:
	CBoxSource(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxSource(void);

	virtual int			OpenSource (const char * pSource, int nFlag);
	virtual int			Close (void);
	virtual char *		GetSourceName (void);

	virtual int			GetStreamCount (QCMediaType nType);
	virtual int			GetStreamPlay (QCMediaType nType);
	virtual int			SetStreamPlay (QCMediaType nType, int nIndex);
	virtual long long	GetDuration (void);

	virtual int			Start (void);
	virtual int			Pause (void);
	virtual int			Stop (void);

	virtual int			ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);

	virtual int			CanSeek (void);
	virtual long long	SetPos (long long llPos);
	virtual long long	GetPos (void);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

	virtual void		EnableSubtt (bool bEnable) {m_bEnableSubtt = bEnable;}
	CBaseSource *		GetMediaSource (void) {return m_pMediaSource;}

	virtual QC_STREAM_FORMAT *	GetStreamFormat (int nID = -1);
	virtual QC_AUDIO_FORMAT *	GetAudioFormat (int nID = -1);
	virtual QC_VIDEO_FORMAT *	GetVideoFormat (int nID = -1);
	virtual QC_SUBTT_FORMAT *	GetSubttFormat (int nID = -1);

	virtual int			AddCache(const char * pSource, bool bIO);
	virtual int			DelCache(const char * pSource);
	QC_SRC_Cache *		GetCache(const char * pSource);
	virtual void		CancelCache(void);
	virtual int			ReleaseCache(QC_SRC_Cache * pCache);
    
    int					GetIOType();

protected:
	QC_IO_Func			m_fIO;
	CBaseSource *		m_pMediaSource;
	bool				m_bEnableSubtt;

	CMutexLock			m_mtRead;

	CMutexLock					m_mtCache;
	CObjectList<QC_SRC_Cache>	m_lstCache;
	CBaseInst *					m_pInstCache;
};

#endif // __CBoxSource_H__
