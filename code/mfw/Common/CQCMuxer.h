/*******************************************************************************
	File:		CQCMuxer.h

	Contains:	The muxer wrap header file.

	Written by:	Jun Lin

	Change History (most recent first):
	2018-01-07		Jun			Create file

*******************************************************************************/
#ifndef __CQCMuxer_H__
#define __CQCMuxer_H__
#include "qcCodec.h"
#include "CBaseObject.h"
#include "ULibFunc.h"
#include "qcMuxer.h"
#include "CMutexLock.h"

#ifdef __QC_OS_IOS__
#include "CFileIO.h"
#endif // __QC_OS_IOS__

#define	QCMUX_INIT		0
#define	QCMUX_CLOSE		1
#define	QCMUX_OPEN		2
#define	QCMUX_PAUSE		3
#define	QCMUX_RESTART	4

class CQCMuxer : public CBaseObject
{
public:
	CQCMuxer(CBaseInst * pBaseInst, void * hInst);
	virtual ~CQCMuxer(void);

    int Init (QCParserFormat nFormat);
    int Uninit(void);
    virtual int Open(const char * pURL);
    virtual int Close();
    virtual int Pause();
	virtual int Restart();
	virtual int Init(void * pVideoFmt, void * pAudioFmt);
	virtual int Write(QC_DATA_BUFF* pBuffer);
    virtual int GetParam(int nID, void * pParam);
    virtual int SetParam(int nID, void * pParam);

protected:
	void *				m_hInst;
	qcLibHandle			m_hLib;
	QC_Muxer_Func	    m_fMux;
    
    CMutexLock          m_mtxFunc;
	int					m_nStatus;
    bool				m_bWaitKeyFrame;
	QC_VIDEO_FORMAT *	m_pFmtVideo;
	QC_AUDIO_FORMAT *	m_pFmtAudio;
	int					m_nNaluSize;
	int					m_nNaluWord;
	QC_DATA_BUFF		m_dataBuff;

	long long			m_llTimePause;
	long long			m_llTimeStart;
	long long			m_llTimeStep;

	CObjectList<QC_DATA_BUFF>	m_lstBuff;
	CObjectList<QC_DATA_BUFF>	m_lstFree;
	long long					m_llVideoTime;

	virtual int	muxWrite(QC_DATA_BUFF* pBuffer);
	static int compareBuffTime(const void *arg1, const void *arg2);
    
#ifdef __QC_OS_IOS__
    int dumpFrame(QC_DATA_BUFF* pBuff);
    int switchByteWrite(unsigned char * pSrc, int nSize, CFileIO * pIO);
    int switchByteRead(unsigned char * pSrc, int nSize, CFileIO * pIO);
    CFileIO*            m_pDumpFileAudio;
    CFileIO*            m_pDumpFileVideo;
#endif // __QC_OS_IOS__
    
};

#endif // __CQCMuxer_H__
