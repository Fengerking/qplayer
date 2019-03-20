/*******************************************************************************
	File:		CBuffMng.h

	Contains:	the buffer trace class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-08		Bangfei			Create file

*******************************************************************************/
#ifndef __CBuffMng_H__
#define __CBuffMng_H__
#include "qcData.h"
#include "CBaseObject.h"

#include "CNodeList.h"
#include "CMutexLock.h"


class CBuffMng : public CBaseObject
{
public:
	CBuffMng(CBaseInst * pBaseInst);
    virtual ~CBuffMng(void);

	virtual int				Read (QCMediaType nType, long long llPos, QC_DATA_BUFF ** ppBuff);
	virtual QC_DATA_BUFF *	ReadVideo (long long llPos);

	virtual QC_DATA_BUFF *	GetEmpty (QCMediaType nType, int nSize);
	virtual int				Send (QC_DATA_BUFF * pBuff);
	virtual int				Return (void * pBuff);
	virtual void			ReleaseBuff (bool bFree);
	virtual void			FlushBuff(void);
	virtual void			EmptyBuff(QCMediaType nType);

	virtual int				SetPos (long long llPos);
	virtual void			SetEOS (bool bEOS);
	virtual void			SetAVEOS(bool bEOA, bool bEOV);
	virtual int				SetStreamPlay(QCMediaType nType, int nStream);
	virtual int				SetSourceLive (bool bLive);

	virtual long long		GetLastTime(QCMediaType nType);
	virtual int				GetBuffCount(QCMediaType nType);

	virtual long long		GetPlayTime (QCMediaType nType);
	virtual long long		GetBuffTime (QCMediaType nType);
	virtual int				SetBuffTime (long long llTime) {return QC_ERR_NONE;}
	virtual unsigned int	GetBuffM3u8Pos();
	virtual int				HaveNewStreamBuff (void);
	virtual int             InSwitching();

	virtual long long		GetTotalMemSize(void);

	virtual int				SetFormat(QCMediaType nType, void * pFormat);

protected:
	virtual	bool			SetCurrentList (QCMediaType nType, bool bRead);
	virtual bool			SwitchNewStream (QC_DATA_BUFF * pPlayBuff);
	virtual int				CopyNewFormat (QC_DATA_BUFF * pBuff);

	virtual void			EmptyBuffList (CObjectList<QC_DATA_BUFF> * pList);
	virtual void			FreeListBuff (CObjectList<QC_DATA_BUFF>	* pList);
	virtual void			EmptyListBuff (CObjectList<QC_DATA_BUFF> * pList);
	virtual void			SwitchBuffList (QC_DATA_BUFF * pKeyBuff, CObjectList<QC_DATA_BUFF> * pListNew, CObjectList<QC_DATA_BUFF> * pListPlay);

    virtual void			AnlBufferInfo(QC_DATA_BUFF* pBuff);
	virtual void			NotifySEIData(unsigned char * pData, int nSize, long long llTime);
	virtual void			NotifyBuffTime(void);

	virtual void			ResetParam(void);
    
    int						GetCodecType(QCMediaType nType);

protected:
	CMutexLock						m_lckList;
	CObjectList<QC_DATA_BUFF> *		m_pCurList;
	CObjectList<QC_DATA_BUFF>		m_lstAudio;
	CObjectList<QC_DATA_BUFF>		m_lstVideo;
	CObjectList<QC_DATA_BUFF>		m_lstSubtt;
	CObjectList<QC_DATA_BUFF>		m_lstEmpty;

	CObjectList<QC_DATA_BUFF>		m_lstNewAudio;
	CObjectList<QC_DATA_BUFF>		m_lstNewVideo;
	CObjectList<QC_DATA_BUFF> *		m_pLstSendVideo;
	CObjectList<QC_DATA_BUFF> *		m_pLstSendAudio;

	int								m_nSelStream;
	bool							m_bNewBetter;
	bool							m_bEOS;
	bool							m_bEOA;
	bool							m_bEOV;
	bool							m_bFlush;
	long long						m_llNextKeyTime;
	long long						m_llStartTime;
	long long						m_llSeekPos;
	long long						m_llLastVideoTime;

	long long						m_llGetLastVTime;
	long long						m_llGetLastATime;

	// same video and audio clone 
	CObjectList<QC_VIDEO_FORMAT>	m_lstFmtVideo;
	CObjectList<QC_AUDIO_FORMAT>	m_lstFmtAudio;
	QC_VIDEO_FORMAT	*				m_pFmtVideo;
	QC_AUDIO_FORMAT	*				m_pFmtAudio;

	bool							m_bSourceLive;
	long long						m_llLastSendTime;

// check the buff status
protected:
	int								m_nNumGet;
	int								m_nNumSend;
	int								m_nNumRead;
	int								m_nNumReturn;
	long long						m_llLastTime;
	int								m_nSysTime;
    int								m_nLastNotifyTime;
    
// detect the buff info
private:
    long long						m_llGopTime;
    long long						m_llVideoRecTime;
    long long						m_llAudioRecTime;
    long long						m_llVideoFrameSize;
    long long						m_llAudioFrameSize;
    int								m_nVideoFrameCount;
    int								m_nAudioFrameCount;
    
// muxer
    virtual int			WriteFrame(QC_DATA_BUFF* pBuff);

//  buffer utility
public:
	QC_DATA_BUFF *		NewEmptyBuff (void);
	int					DeleteBuff (QC_DATA_BUFF * pBuff, bool bForce = false);
	int					FreeBuffData (QC_DATA_BUFF * pBuff, bool bForce = false);
	QC_DATA_BUFF *		CloneBuff (QC_DATA_BUFF * pBuff); // it will support later

	int					m_nNewEmptyNum;

};

#endif //__CBuffMng_H__
