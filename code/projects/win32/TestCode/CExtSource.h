/*******************************************************************************
	File:		CExtSource.h

	Contains:	the video decoder and render header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-04		Bangfei			Create file

*******************************************************************************/
#ifndef __CExtSource_H__
#define __CExtSource_H__
#include "qcPlayer.h"
#include "qcParser.h"

#include "CThreadWork.h"
#include "CBuffMng.h"

#include "CFileIO.h"
#include "CMutexLock.h"

class CExtSource;

class CExtBuffMng : public CBuffMng
{
public:
	CExtBuffMng(CBaseInst * pBaseInst, CExtSource * pSource)
		: CBuffMng(pBaseInst)
		, m_pSource(pSource)
	{
	}
	virtual ~CExtBuffMng(void)
	{
	}

	virtual int		Send(QC_DATA_BUFF * pBuff);

protected:
	CExtSource *	m_pSource;
};

class CExtSource : public CThreadFunc
{
public:
	CExtSource(void * hInst);
	virtual ~CExtSource(void);

	virtual int			SetPlayer (QCM_Player * pPlay);
	virtual void		NotifyEvent(void * pUserData, int nID, void * pValue1);
	virtual int			Send(QC_DATA_BUFF * pBuff);
	virtual int			Close(void);
    void				SetSourceType(int nType);
    void				SetURL(char* pszUrl);

protected:
	virtual int			OnWorkItem(void);

	virtual int			ReadVideoData(QC_DATA_BUFF * pBuff);
	virtual int			ReadAudioData(QC_DATA_BUFF * pBuff);

protected:
	void *				m_hInst;
	CBaseInst *			m_pInst;
	char				m_szFile[1024];
	QCM_Player *		m_pPlay;

	QC_IO_Func			m_fIO;
	QC_Parser_Func		m_fParser;
	CThreadWork *		m_pReadThread;

	CExtBuffMng *		m_pBuffMng;
	QC_DATA_BUFF		m_buffAudio;
	QC_DATA_BUFF		m_buffVideo;
	QC_DATA_BUFF		m_buffData;
	int					m_nAudioBuffTime;
	int					m_nVideoBuffTime;

	bool				m_bExtAV;
	QCCodecID			m_bAudioCodecID;
	CMutexLock			m_lock;

	CFileIO *			m_pAudioFile;
	CFileIO *			m_pVideoFile;
	long long			m_llVideoTime;
	long long			m_llAudioTime;

	CFileIO *			m_pDumpFile;

};
#endif //__CExtSource_H__
