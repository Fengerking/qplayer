/*******************************************************************************
	File:		CAnalysisMng.h
 
	Contains:	Analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-21		Jun			Create file
 
 *******************************************************************************/

#ifndef CAnalysisMng_hpp
#define CAnalysisMng_hpp

#include "UMsgMng.h"
#include "CNodeList.h"
#include "qcData.h"
#include "CMutexLock.h"
#include "qcAna.h"
#include "CAnalDataSender.h"
#include "CThreadWork.h"

class CAnalBase;
class COMBoxMng;

class CAnalysisMng : public CMsgReceiver, public CBaseObject, public CThreadFunc
{
public:
	CAnalysisMng(CBaseInst * pBaseInst, COMBoxMng* pPlayer);
    virtual ~CAnalysisMng();
    
public:
    void        OnOpen(const char* pszURL);
    void		OnOpenDone(int nErrCode, long long llDuration);
    void		OnStop(long long llPlayingTime);
    void        OnEOS();
    void        OnNetworkConnectChanged(int nConnectStatus);
    void		UpdateDNSServer(const char* pszServer);
    
public:
    virtual int ReceiveMsg (CMsgItem* pItem);
    virtual int	RecvEvent(int nEventID);
    
protected:
    virtual int			OnWorkItem (void);
    
private:
    void		CreateSessionID();
    void		CreateClientInfo(int nVersion);
    int         ProcessNewStream(QC_RESOURCE_INFO* pRes, int nBA, long long llPos);
    int         ProcessDownload(int nSpeed, long long llDownloadPos, char* pszNewURL);
    
    //
    void		GetIPAddress(char* pAddress, int nLen);
    
    //
    void		SendAnalData(QCANA_EVT_BASE* pEvent);
    int         SendAnalDataNow(QCANA_EVT_BASE* pEvent, bool bDisconnect=true);
    void        SendCacheData(bool bAnlDestroy, long long llPlayingTime);
    int			onMsg(CMsgItem* pMsg);
    
    void		CreateEvents();
    void		ReleaseEvents();
    void		ReleaseAnal();
    void		ResetEvents();
    bool		IsPlaying();
    QCANA_EVT_BASE* GetEvent(QCANA_EVT_ID nEvtID);
    QCANA_EVT_BASE* CloneEvent(QCANA_EVT_BASE* pEvent);
    QCANA_EVT_BASE* AllocEvent(int nEvtID);
    
    void        AnalStop();
    void        AnalOpen();
    void		AnalPostData(bool bAnlDestroy, long long llPlayingTime);
    
    void 		StartReachability();
    void 		StopReachability();
    
    bool		IsDNSReady();
    int			Disconnect();
    void		UpdateSource();
    bool		IsPureAudio();
    bool        IsPureVideo();
    bool		IsGopTimeReady();
    
private:
    CMutexLock			m_mtEvt;
    CMutexLock          m_mtEvtCache;
    bool				m_bSeeked;
    bool				m_bPaused;
    
    QCANA_SOURCE_INFO*	m_pSource;
    QCANA_SOURCE_INFO*	m_pOldBitrate;
    QCANA_SOURCE_INFO*	m_pNewBitrate;
    long long			m_llDownloadPos;
    
    char				m_szSessionID[37];
    
    CObjectList<CAnalBase> m_listAnal;
    CObjectList<QCANA_EVT_BASE>	m_Events;
    QCANA_DEVICE_INFO   m_DeviceInfo;
    char*				m_pszPlayURL;
    long long			m_llDuration;
    QCANA_EVENT_INFO	m_EventInfo;
    
    int					m_nTimerTime;
    int					m_nSendTime;
    CThreadWork *		m_pThreadWork;
    bool				m_bAudioRendered;
    bool				m_bAnalDestroyed;
    
    long long			m_llStopTime;
    COMBoxMng*			m_pPlayer;
    bool				m_bAsyncReport;
    CObjectList<QCANA_EVT_BASE>    m_EventCache;
    bool				m_bFirstNetworkInfo;
    bool				m_bGopReady;
    
#ifdef __QC_OS_IOS__
    void*				m_pNetworkSta;
#endif
};

#endif /* CAnalysisMng_hpp */
