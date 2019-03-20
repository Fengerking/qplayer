/*******************************************************************************
	File:		CBoxBase.h

	Contains:	the base box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxBase_H__
#define __CBoxBase_H__

#include "qcData.h"

#include "CBaseObject.h"
#include "CMutexLock.h"
#include "CBaseClock.h"

typedef enum  {
	OMB_TYPE_BASE		= 0,
	OMB_TYPE_SOURCE		= 10,
	OMB_TYPE_FILTER		= 20,
	OMB_TYPE_RENDER		= 30,
	OMB_TYPE_RND_EXT	= 31,
	OMB_TYPE_MAX		= 0X7FFFFFFF
} OMBOX_TYPE;

typedef enum  {
	OMB_STATUS_INIT		= 0,
	OMB_STATUS_RUN		= 1,
	OMB_STATUS_PAUSE	= 2,
	OMB_STATUS_STOP		= 3,
	OMB_STATUS_MAX		= 0X7FFFFFFF
} OMBOX_STATUS;

// return the audio buffer time from source box
#define	BOX_GET_AudioBuffTime		1001
#define	BOX_GET_SourceType			1002
#define	BOX_GET_VideoBuffTime		1003
#define	BOX_SET_AudioVolume			1010

class CBoxBase : public CBaseObject
{
public:
	CBoxBase(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxBase(void);

	virtual int				SetSource (CBoxBase * pSource);
	virtual CBoxBase *		GetSource (void) {return m_pBoxSource;}

	virtual int				Start (void);
	virtual int				Pause (void);
	virtual int				Stop (void);

	virtual int				ReadBuff (QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);
	virtual int				RendBuff (QC_DATA_BUFF * pBuffer, bool bRnd);

	virtual long long		SetPos (long long llPos);
	virtual void			Flush(void);
	virtual bool			IsEOS(void);

	virtual void			SetClock (CBaseClock * pClock);
	virtual CBaseClock *	GetClock (void);
	virtual char *			GetName (void) {return m_szBoxName;}
	virtual OMBOX_TYPE		GetType (void) {return m_nBoxType;}

	virtual int 			SetParam (int nID, void * pParam);
	virtual int				GetParam (int nID, void * pParam);

	virtual QC_STREAM_FORMAT *	GetStreamFormat (int nID = -1);
	virtual QC_AUDIO_FORMAT *	GetAudioFormat (int nID = -1);
	virtual QC_VIDEO_FORMAT *	GetVideoFormat (int nID = -1);
	virtual QC_SUBTT_FORMAT *	GetSubttFormat (int nID = -1);

protected:
	void *				m_hInst;
	char				m_szBoxName[32];
	OMBOX_TYPE			m_nBoxType;
	OMBOX_STATUS		m_nStatus;
	CBaseClock *		m_pClock;
	CBoxBase *			m_pBoxSource;

	QC_DATA_BUFF *		m_pBuffInfo;
	QC_DATA_BUFF *		m_pBuffData;

	long long			m_llSeekPos;
	bool				m_bEOS;
};

#endif // __CBoxBase_H__
