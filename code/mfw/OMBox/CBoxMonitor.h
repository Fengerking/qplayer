/*******************************************************************************
	File:		CBoxMonitor.h

	Contains:	the base box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxMonitor_H__
#define __CBoxMonitor_H__

#include "qcData.h"

#include "CBaseObject.h"
#include "CMutexLock.h"
#include "CBaseClock.h"
#include "CBoxBase.h"
#include "CNodeList.h"

#include "UFileFunc.h"

#define BOX_MONITOR_DISABLE_ALL

#ifdef BOX_MONITOR_DISABLE_ALL
#define	BOX_READ_BUFFER_REC_SOURCE
#define	BOX_READ_BUFFER_REC_AUDIODEC
#define	BOX_READ_BUFFER_REC_VIDEODEC
#define	BOX_READ_BUFFER_REC_AUDIORND
#define	BOX_READ_BUFFER_REC_VIDEORND
#else
#define	BOX_READ_BUFFER_REC_SOURCE CBoxRecOne boxRecRead(this, m_pBuffInfo, &nRC);
//#define	BOX_READ_BUFFER_REC_SOURCE

#define	BOX_READ_BUFFER_REC_AUDIODEC CBoxRecOne boxRecRead(this, m_pBuffInfo, &nRC);
//#define	BOX_READ_BUFFER_REC_AUDIODEC

#define	BOX_READ_BUFFER_REC_VIDEODEC CBoxRecOne boxRecRead(this, m_pBuffInfo, &nRC);
//#define	BOX_READ_BUFFER_REC_VIDEODEC

#define	BOX_READ_BUFFER_REC_AUDIORND CBoxRecOne boxRecRead(this, m_pBuffInfo, &nRC);
//#define	BOX_READ_BUFFER_REC_AUDIORND

#define	BOX_READ_BUFFER_REC_VIDEORND CBoxRecOne boxRecRead(this, m_pBuffInfo, &nRC);
//#define	BOX_READ_BUFFER_REC_VIDEORND
#endif // BOX_MONITOR_DISABLE_ALL

class CBoxRecOne
{
public:
	CBoxRecOne (CBoxBase * pBox, QC_DATA_BUFF * pBuffInfo, int * pRC);
	virtual ~CBoxRecOne(void);

public:
	CBoxBase *		m_pBox;
	QC_DATA_BUFF *	m_pBuffer;
	int *			m_pRC;

	int				m_nThdUsed;
	int				m_nSysUsed;
};

struct SBoxReadItem
{
	CBoxBase *		m_pBox;
	QCMediaType		m_nType;
	int				m_nThdUsed;
	int				m_nSysUsed;

	int				m_tmBuf;
	int				m_tmClock;
	int				m_tmSys;

	int				m_nRC;
};

class CBoxMonitor : public CBaseObject
{
public:
	CBoxMonitor(CBaseInst * pBaseInst);
	virtual ~CBoxMonitor(void);

	void		SetClock (CBaseClock * pClock) {m_pClock = pClock;}

	int			StartRead (CBoxRecOne * pOne);
	int			EndRead (CBoxRecOne * pOne);

	int			ReleaseItems (void);

	int			ShowResult (void);

protected:
	SBoxReadItem *	GetLastItem (CBoxBase * pBox, QCMediaType nType, int nRC);

	void			ShowAudioSrc (void);
	void			ShowVideoSrc (void);
	void			ShowAudioDec (void);
	void			ShowVideoDec (void);
	void			ShowAudioRnd (void);
	void			ShowVideoRnd (void);

	void			ShowPerformInfo (void);

	void			ShowBoxInfo (CBoxBase * pBox, QCMediaType nType, bool bSuccess);

	CBoxBase *		GetBox (OMBOX_TYPE nBoxType, QCMediaType nMediaType);

protected:
	CBaseClock *				m_pClock;
	CMutexLock					m_mtRead;
	CObjectList<SBoxReadItem>	m_lstRead;

	qcFile						m_hFile;

};

#endif // __CBoxMonitor_H__
