/*******************************************************************************
	File:		CQCFFConcat.h

	Contains:	The qc CQCFFConcat source header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-05-16		Bangfei			Create file

*******************************************************************************/
#ifndef __CQCFFConcat_H__
#define __CQCFFConcat_H__

#include "CBaseSource.h"
#include "CNodeList.h"

#include "ULibFunc.h"

typedef struct
{
	char *		pURL;
	long long	llDur;
	long long	llPos;
	long long	llStart;
	long long	llStop;
	long long	llSeek;
	bool		bEOA;
	bool		bEOV;
} QCFF_CONCAT_ITEM;

class CQCSource;
class CQCFFConcat;

class CFFCatBuffMng : public CBuffMng
{
public:
	CFFCatBuffMng(CBaseInst * pBaseInst, CQCFFConcat * pConcat);
	virtual ~CFFCatBuffMng(void);

	virtual int		Send(QC_DATA_BUFF * pBuff);

protected:
	CQCFFConcat *	m_pConcat;
};

class CQCFFConcat : public CBaseSource
{
public:
	CQCFFConcat(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCFFConcat(void);

	virtual int			OpenIO(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			Close(void);

	virtual int			CanSeek(void);
	virtual long long	SetPos(long long llPos);

	virtual int			GetStreamCount(QCMediaType nType);
	virtual int			GetStreamPlay(QCMediaType nType);
	virtual int			SetStreamPlay(QCMediaType nType, int nStream);

	virtual QC_STREAM_FORMAT *	GetStreamFormat(int nID = 0);
	virtual QC_AUDIO_FORMAT *	GetAudioFormat(int nID = 0);
	virtual QC_VIDEO_FORMAT *	GetVideoFormat(int nID = 0);
	virtual QC_SUBTT_FORMAT *	GetSubttFormat(int nID = 0);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

	virtual int			SendBuff(QC_DATA_BUFF * pBuff);

protected:
	virtual int			OpenConcat(QC_IO_Func * pIO, int nType, QCParserFormat nFormat, const char * pSource);
	virtual int			OpenItemSource(QCFF_CONCAT_ITEM * pItem);
	virtual int			UpdateInfo(void);

	virtual int			OnWorkItem(void);
	virtual int			ReadParserBuff(QC_DATA_BUFF * pBuffInfo);

protected:
	char *								m_pFolder;
	CObjectList<QCFF_CONCAT_ITEM>		m_lstItem;

	QCFF_CONCAT_ITEM *					m_pCurItem;
	CQCSource *							m_pSource;
	int									m_nOpenFlag;

	long long							m_llItemTime;
	long long							m_llBuffTime;
	long long							m_llPlayTime;

	int									m_nPreloadTimeDef;
};

#endif // __CQCFFConcat_H__
