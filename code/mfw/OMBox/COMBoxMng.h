/*******************************************************************************
	File:		COMBoxMng.h

	Contains:	The media engine header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-15		Bangfei			Create file

*******************************************************************************/
#ifndef __COMBoxMng_H__
#define __COMBoxMng_H__
#include "qcPlayer.h"

#include "CBaseObject.h"
#include "CMutexLock.h"
#include "CThreadWork.h"

#include "CNodeList.h"
#include "CBoxVideoRnd.h"
#include "CBoxAudioRnd.h"
#include "CBoxVDecRnd.h"
#include "CBoxSource.h"
#include "CBoxMonitor.h"

#include "CBaseAudioRnd.h"
#include "CBaseVideoRnd.h"

#include "CAnalysisMng.h"

#ifdef __QC_OS_IOS__
class CiOSPlayer;
#endif

#define	QC_TASK_OPEN		0X70000001
#define	QC_TASK_SEEK		0X70000002
#define	QC_TASK_REOPEN		0X70000003
#define	QC_TASK_CHECK		0X70000004
#define	QC_TASK_NETCHANGE	0X70000005
#define	QC_TASK_IPDETECT	0X70000006
#define	QC_TASK_ADDCACHE	0X70000007
#define	QC_TASK_DELCACHE	0X70000008
#define	QC_TASK_ADDIOCACHE	0X70000009
#define	QC_TASK_DELIOCACHE	0X7000000A

class COMBoxMng : public CBaseObject, public CThreadFunc, public CMsgReceiver
{
public:
	COMBoxMng(void * hInst);
	virtual ~COMBoxMng(void);

	virtual void		SetNotifyFunc (QCPlayerNotifyEvent pFunc, void * pUserData);
	virtual void		SetView (void * hView, RECT * pRect);

	virtual int			Open (const char * pURL, int nFlag);
	virtual int			Close (void);
	
	virtual int			Start (void);
	virtual int			Pause (void);
	virtual int			Stop (void);

	virtual long long	SetPos (long long llPos);
	virtual long long	GetPos (void);
	virtual long long	GetDuration (void);
	QCPLAY_STATUS		GetStatus (void) {return m_stsPlay;}

	virtual int			SetVolume (int nVolume);
	virtual int			GetVolume (void);

	virtual int 		SetParam (int nID, void * pParam);
	virtual int			GetParam (int nID, void * pParam);

	int					GetBoxCount (void);
	CBoxBase *			GetBox (int nIndex);
	CBaseClock *		GetClock (void) {return m_pClock;}
    int					GetSDKVersion();

protected:
	virtual	int		DoOpen (const char * pSource, int nFlag);
	virtual int		DoSeek (const long long llPos, bool bPause);
    virtual	int		DoFastOpen (const char * pSource, int nFlag);
	virtual int		DoReopen (void);
	virtual int		DoCheckStatus (void);

	virtual int		CheckOpenStatus (int nWaitTime);
	virtual int		WaitAudioRender (int nWaitTime, bool bCheckStatus);
    virtual int		PostAsyncTask(int nID, int nValue, long long llValue, const char * pName, int nDelay = 0);
    
private:
    void UpdateAnal();

protected:
	void *						m_hInst;
	QCPlayerNotifyEvent			m_fNotifyEvent;
	void *						m_pUserData;
	bool						m_bExit;

	CMutexLock					m_mtFunc;
    CMutexLock                  m_mtView;
	QCPLAY_STATUS				m_stsPlay;

	char						m_szPreName[32];
	int							m_nOpenFlag;
	long long					m_llDur;
	bool						m_bOpening;
	bool						m_bClosed;
	long long					m_llStartTime;
	// for reopen
	QCParserFormat				m_nParserFormat;
	long long					m_llReopenPos;
	int							m_nTotalRndCount;
	bool						m_bEOS;

// video render parameters
	void *						m_hView;
	RECT						m_rcView;
	int							m_nDisVideoLevel;
	int							m_nAudioVolume;

// Seek parameter
	int							m_nSeekMode;
	long long					m_llSeekPos;
	bool						m_bSeeking;
	int							m_nLastSeekTime;

// for mp4 preload time when seek before play
	long long					m_llStartPos;
	int							m_nPreloadTimeSet;
	int							m_nPreloadTimeDef;

// the box and clock parameres
	CObjectList<CBoxBase>		m_lstBox;
	CBoxSource *				m_pBoxSource;
	CBoxRender *				m_pRndAudio;
	CBoxRender *				m_pRndVideo;
	CBoxMonitor *				m_pBoxMonitor;
	CBaseClock *				m_pClock;
	CBaseClock *				m_pClockMng;

	CBaseAudioRnd *				m_pExtRndAudio;
	CBaseVideoRnd *				m_pExtRndVideo;
    
 // Analysis collection for player behavior
    CAnalysisMng *				m_pAnlMng;

	char						m_szQiniuDrmKey[128];
    bool						m_bFirstFrameRendered;
    char*						m_pURL;
	char						m_szCacheURL[2048];
    
#ifdef __QC_OS_IOS__
    CiOSPlayer*					m_pPlayer;
#endif
	
// define the event parameters
protected:
	virtual int			OnHandleEvent (CThreadEvent * pEvent);
	CThreadWork	*		m_pThreadWork;

public:
	virtual int			ReceiveMsg (CMsgItem * pItem);
	bool				m_bSendIPAddr;

};

#endif // __COMBoxMng_H__
