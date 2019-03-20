/*******************************************************************************
	File:		CAnalysisMng.cpp
 
	Contains:	Analysis collection implementation code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-03-21		Jun			Create file
 
 *******************************************************************************/
#ifdef __QC_OS_WIN32__
#include <winsock2.h>
#include "Ws2tcpip.h"
#endif // __QC_OS_WIN32__

#ifdef __QC_OS_IOS__
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/sysctl.h>
#import <UIKit/UIKit.h>
#import "NetworkSta.h"
#define NETWORKSTA ((CNetworkSta*)m_pNetworkSta)
#endif

#ifdef __QC_OS_NDK__
#include <stdlib.h>
#include <math.h>
#include <sys/system_properties.h>
#endif // __QC_OS_NDK__
#include "stdio.h"

#include "CAnalysisMng.h"
#include "UUrlParser.h"
#include "ULogFunc.h"
#include "UAVFormatFunc.h"
#include "qcAna.h"
#include "CAnalJedi.h"
#include "CAnalPandora.h"
#include "CAnalPili.h"
#include "COMBoxMng.h"
#include "CMsgMng.h"

#define ANL_REPORT_BEGIN_TIME	5000

CAnalysisMng::CAnalysisMng(CBaseInst * pBaseInst, COMBoxMng* pPlayer)
	: CBaseObject(pBaseInst)
	, m_bSeeked(false)
	, m_bPaused(false)
	, m_pSource(NULL)
	, m_pOldBitrate(NULL)
	, m_pNewBitrate(NULL)
	, m_llDownloadPos(0)
	, m_llDuration(0)
	, m_pThreadWork(NULL)
	, m_bAudioRendered(false)
	, m_bAnalDestroyed(false)
	, m_bAsyncReport(false)
	, m_pPlayer(pPlayer)
	, m_llStopTime(0)
	, m_pszPlayURL(NULL)
	, m_bFirstNetworkInfo(true)
#ifdef __QC_OS_IOS__
	, m_pNetworkSta(NULL)
#endif
{
    SetObjectName("CAnalysisMng");
    
    CreateEvents();
    CreateClientInfo(pPlayer->GetSDKVersion());
    memset(m_szSessionID, 0, 37);
        
    CAnalBase* pAnal = NULL;
//    pAnal = new CAnalPandora;
//    m_listAnal.AddTail(pAnal);
//    pAnal = new CAnalJedi;
//    m_listAnal.AddTail(pAnal);
	pAnal = new CAnalPili(m_pBaseInst);
    m_listAnal.AddTail(pAnal);

	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RegNotify(this);
	if (m_pBaseInst)
		m_pBaseInst->AddListener(this);
    
    m_nTimerTime = qcGetSysTime();
    m_nSendTime = qcGetSysTime();
	m_pThreadWork = new CThreadWork(m_pBaseInst);
    m_pThreadWork->SetOwner (m_szObjName);
    m_pThreadWork->SetWorkProc (this, &CThreadFunc::OnWork);
	m_pThreadWork->Start ();
    
    StartReachability();
}

CAnalysisMng::~CAnalysisMng()
{
	QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
	
    QCLOGI("[ANL]+Exit ANL, position %lld", m_llStopTime);
    m_bAnalDestroyed = true;
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RemNotify(this);
    if(m_pBaseInst)
        m_pBaseInst->RemListener(this);

    int nTime = qcGetSysTime();
    StopReachability();
    AnalStop();
    if(m_pThreadWork)
    {
        m_pThreadWork->Stop();
        delete m_pThreadWork;
        m_pThreadWork = NULL;
    }
    
    CAutoLock lock(&m_mtEvt);
    
    ReleaseEvents();
    ReleaseAnal();
    CAnalBase::ReleaseResource(&m_pSource);
    CAnalBase::ReleaseResource(&m_pOldBitrate);
    CAnalBase::ReleaseResource(&m_pNewBitrate);
    QC_DEL_A(m_pszPlayURL);

    QCLOGI("[ANL][KPI]-Exit ANL, use time %d", qcGetSysTime()-nTime);
}

void CAnalysisMng::OnOpen(const char* pszURL)
{
    CAutoLock lock(&m_mtEvt);
    
    if(!pszURL)
        return;
    
    ResetEvents();

    AnalOpen();
    
    m_llStopTime = 0;
    m_bSeeked = false;
    m_bAudioRendered = false;
	long long llOpenStartTime = qcGetUTC();
    
    //
    QCANA_EVT_STARTUP* pStartup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
    pStartup->llTime = 0;
    pStartup->llOpenStartTime = llOpenStartTime;
    pStartup->llEvtDuration = 0;
    pStartup->nGopTime = 0;
    pStartup->nConnectTime = 0;
    pStartup->nFirstByteTime = 0;
    pStartup->nFirstAudioRenderTime = 0;
    pStartup->nFirstVideoRenderTime = 0;
    
    //
    QCANA_EVT_CLOSE* pClose = (QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE);
    pClose->llOpenStartTime = llOpenStartTime;
    pClose->llWatchDuration = 0;
    pClose->nConnectTime = 0;
    pClose->nFirstByteTime = 0;
    
    //
    CAnalBase::ReleaseResource(&m_pSource);
    CAnalBase::ReleaseResource(&m_pOldBitrate);
    CAnalBase::ReleaseResource(&m_pNewBitrate);

    CreateSessionID();
    QC_DEL_A(m_pszPlayURL);
    m_pszPlayURL = new char[strlen(pszURL)+1];
    strcpy(m_pszPlayURL, pszURL);
    
    //
    QCANA_EVT_BASE* pEvtOpen = GetEvent(QCANA_EVTID_OPEN);
    pEvtOpen->llTime = llOpenStartTime;
    pEvtOpen->llOpenStartTime = llOpenStartTime;
    CAnalBase::ReleaseResource(&pEvtOpen->pSrcInfo);
    QCANA_SOURCE_INFO* pClone = new QCANA_SOURCE_INFO;
    memset(pClone, 0, sizeof(QCANA_SOURCE_INFO));
    pClone->pszURL = new char[strlen(m_pszPlayURL)+1];
    strcpy(pClone->pszURL, m_pszPlayURL);
    pEvtOpen->pSrcInfo = pClone;
    
    //
    QCANA_EVT_STOP* pEvtStop = (QCANA_EVT_STOP*)GetEvent(QCANA_EVTID_STOP);
    pEvtStop->llOpenStartTime = llOpenStartTime;
    pEvtStop->nSystemErrCode = 0;
    pEvtStop->llWatchDuration = 0;
    pEvtStop->nFirstFrameRenderTime = 0;
}

void CAnalysisMng::OnStop(long long llPlayingTime)
{
    CAutoLock lock(&m_mtEvt);
    
    // handle short video
    QCANA_EVT_STARTUP* pStartup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
    // not issue startup event
    if(pStartup->llEvtDuration <= 0)
    {
        // playback begins
        if(pStartup->nFirstAudioRenderTime >0 || pStartup->nFirstVideoRenderTime > 0)
        {
            if(!IsGopTimeReady() && m_pSource)
            {
                pStartup->nGopTime = (int)m_pSource->llDuration;
                ((QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE))->nGopTime = (int)m_pSource->llDuration;
            }
            pStartup->llEvtDuration = qcGetUTC() - pStartup->llOpenStartTime;
            SendAnalData(pStartup);
        }
    }
    
    // handle stop event
    QCANA_EVT_STOP* pEvt = (QCANA_EVT_STOP*)GetEvent(QCANA_EVTID_STOP);
    pEvt->llTime = qcGetUTC();
    pEvt->llWatchDuration = llPlayingTime;
    pEvt->nFirstFrameRenderTime = pStartup->nFirstAudioRenderTime;
    if(pStartup->nFirstVideoRenderTime > 0)
        pEvt->nFirstFrameRenderTime = pStartup->nFirstVideoRenderTime;
    
    CAnalBase::ReleaseResource(&pEvt->pSrcInfo);
    if(m_pSource)
    {
        pEvt->pSrcInfo = CAnalBase::CloneResource(m_pSource);
    }
    else
    {
        // maybe open fail or open was cancelled
        // default duration is 0, live
        QCANA_SOURCE_INFO* pClone = new QCANA_SOURCE_INFO;
        memset(pClone, 0, sizeof(QCANA_SOURCE_INFO));
        if(m_pszPlayURL)
        {
            pClone->pszURL = new char[strlen(m_pszPlayURL)+1];
            strcpy(pClone->pszURL, m_pszPlayURL);
        }
        pEvt->pSrcInfo = pClone;
    }

    SendAnalData(pEvt);
    
    // handle close event
    m_llStopTime = llPlayingTime;
    if(pStartup->llEvtDuration > 0) // make sure startup event has been issued
    {
        QCANA_EVT_BASE* pEvtClose = GetEvent(QCANA_EVTID_CLOSE);
        pEvtClose->llTime = qcGetUTC();
        pEvtClose->llPos = llPlayingTime;
        SendAnalData(pEvtClose);
    }
}

void CAnalysisMng::OnOpenDone(int nErrCode, long long llDuration)
{
    CAutoLock lock(&m_mtEvt);
    
    // send open event
    QCANA_EVT_BASE* pEvtOpen = GetEvent(QCANA_EVTID_OPEN);
    QCANA_SOURCE_INFO* pSrc = pEvtOpen->pSrcInfo;
    if(pSrc)
        pSrc->llDuration = llDuration;
    pEvtOpen->llEvtDuration = qcGetUTC() - pEvtOpen->llTime;
    
    SendAnalData(pEvtOpen);

    // send stop event if open fail
    if(nErrCode != QC_ERR_NONE)
    {
        QCANA_EVT_STOP* pEvt = (QCANA_EVT_STOP*)GetEvent(QCANA_EVTID_STOP);
        pEvt->nSystemErrCode = 0;
        pEvt->nErrCode = nErrCode;
        OnStop(0);
    }
}

void CAnalysisMng::OnEOS()
{
    CAutoLock lock(&m_mtEvt);
    
    // handle GOP time not ready
    if(!IsGopTimeReady() && !IsPureAudio())
    {
        QCANA_EVT_STARTUP* pEvt = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        
        if(m_pSource)
        {
            pEvt->nGopTime = (int)m_pSource->llDuration;
            ((QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE))->nGopTime = (int)m_pSource->llDuration;
        }
        
        SendAnalData(pEvt);
    }
}

#pragma mark Event processing
int CAnalysisMng::onMsg(CMsgItem* pMsg)
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->onMsg(pMsg);
        pAnal = m_listAnal.GetNext(pos);
    }
    
    return QC_ERR_NONE;
}

void CAnalysisMng::AnalPostData(bool bAnlDestroy, long long llPlayingTime)
{
    if(!CAnalBase::IsReportOpenStopEvt())
    {
        // consider network reconnect, playing time would be reset.
        if( ((llPlayingTime <= ANL_REPORT_BEGIN_TIME) && !IsDNSReady())
           || (bAnlDestroy && !IsDNSReady()) )
        {
            QCLOGW("[ANL]Pending, watch time %lld, destroyed %d, ready %d",
                   llPlayingTime, bAnlDestroy, IsDNSReady());
            return;
        }
    }
    
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->PostData();
        pAnal = m_listAnal.GetNext(pos);
    }
}

void CAnalysisMng::SendCacheData(bool bAnlDestroy, long long llPlayingTime)
{
    CAutoLock lock(&m_mtEvtCache);
    
    int nCount = m_EventCache.GetCount();

    if(nCount <= 0)
        return;
    
    if(!CAnalBase::IsReportOpenStopEvt())
    {
        // consider network reconnect, playing time would be reset.
        if( ((llPlayingTime <= ANL_REPORT_BEGIN_TIME) && !IsDNSReady())
           || (bAnlDestroy && !IsDNSReady()) )
        {
            QCLOGW("[ANL]Pending, watch time %lld, destroyed %d, ready %d, count %d",
                   llPlayingTime, bAnlDestroy, IsDNSReady(), nCount);
            return;
        }
    }
    
    if(bAnlDestroy)
        QCLOGI("[ANL][KPI]Left count %d", nCount);

    nCount = 0;
    QCANA_EVT_BASE* pEvt = m_EventCache.RemoveHead();
    
    while (pEvt)
    {
        //QCLOGI("[ANL]+Send No.%d, ID %d", nCount, pEvt->nEventID);
        int nRet = SendAnalDataNow(pEvt, false);
        //QCLOGI("[ANL]-Send No.%d, ID %d", nCount, pEvt->nEventID);
        if(nRet == QC_ERR_RETRY)
        {
            m_EventCache.AddHead(pEvt);
            break;
        }
        
        CAnalBase::ReleaseEvent(pEvt);
        
        if( (nRet != QC_ERR_NONE) && (bAnlDestroy || m_pBaseInst->m_bForceClose) )
            break;
        
        pEvt = m_EventCache.RemoveHead();
        nCount++;
    }
    
    Disconnect();
}

void CAnalysisMng::SendAnalData(QCANA_EVT_BASE* pEvent)
{
    CAutoLock lock(&m_mtEvtCache);
    
    if(m_bAsyncReport)
    {
        QCANA_EVT_BASE* clone = CloneEvent(pEvent);
        if(clone)
        {
            m_EventCache.AddTail(clone);
        }
        return;
    }
    else
        SendAnalDataNow(pEvent, true); // close socket immediately after report successfully
}

int CAnalysisMng::SendAnalDataNow(QCANA_EVT_BASE* pEvent, bool bDisconnect/*=true*/)
{
//    if(!m_pSource && pEvent->nEventID != QCANA_EVTID_OPEN && pEvent->nEventID != QCANA_EVTID_STOP)
//        return QC_ERR_RETRY;
    int nRC = 0;
    if (pEvent != NULL)
        nRC = pEvent->nEventID;
    if(m_pBaseInst)
    {
        if(strlen(m_pBaseInst->m_szAppVer) > 0)
        	sprintf(m_DeviceInfo.szAppVersion, "%s", m_pBaseInst->m_szAppVer);
        if(strlen(m_pBaseInst->m_szSDK_ID) > 0)
            sprintf(m_DeviceInfo.szDeviceID, "%s", m_pBaseInst->m_szSDK_ID);
        if(strlen(m_pBaseInst->m_szAppName) > 0)
            sprintf(m_DeviceInfo.szAppID, "%s", m_pBaseInst->m_szAppName);
    }
    
    m_EventInfo.pDevInfo = &m_DeviceInfo;
    //m_EventInfo.pSrcInfo = m_pSource;
    m_EventInfo.pEvtInfo = pEvent;
    
    int nResult = QC_ERR_NONE;
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        nResult = pAnal->ReportEvent(&m_EventInfo, bDisconnect);
        
        if(nResult != QC_ERR_NONE && m_pBaseInst->m_bForceClose)
        {
            QCLOGW("[ANL]Break issue");
            break;
        }
        pAnal = m_listAnal.GetNext(pos);
    }
    
    return nResult;
}

int CAnalysisMng::ReceiveMsg (CMsgItem* pItem)
{
//    int nTime = qcGetSysTime();
//    QCLOGI("[ANL]+ReceiveMsg %X, %s", pItem->m_nMsgID, pItem->m_szIDName);
    if(m_bAnalDestroyed)
        return QC_ERR_NONE;
    
    if(!IsPlaying())
        return QC_ERR_NONE;
    
    CAutoLock lock(&m_mtEvt);
    
    if(m_Events.GetCount() <= 0)
        return QC_ERR_NONE;
    
    onMsg(pItem);
    
    if(pItem->m_nMsgID == QC_MSG_SNKV_FIRST_FRAME)
    {
        if(m_bSeeked)
        {
            QCANA_EVT_BASE* pEvtSeek 	= GetEvent(QCANA_EVTID_SEEK);
            pEvtSeek->llEvtDuration 	= qcGetUTC() - pEvtSeek->llTime;
            QCLOGI("[ANL][KPI]Seek total use time %lld", pEvtSeek->llEvtDuration);
            SendAnalData(pEvtSeek);
        }
        else
        {
            QCANA_EVT_STARTUP* pEvtStartup	= (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
            if(pEvtStartup->llEvtDuration > 0)
                return QC_ERR_NONE;
            pEvtStartup->llTime = qcGetUTC();
            pEvtStartup->nFirstVideoRenderTime = pItem->m_nValue;
            QCLOGI("[ANL][KPI]Show first video frame use time %d", pItem->m_nValue);
            
            if( (IsPureVideo() || m_bAudioRendered) && IsGopTimeReady())
            {
                pEvtStartup->llEvtDuration = pItem->m_nValue;
                SendAnalData(pEvtStartup);
                
                if(m_bFirstNetworkInfo)
                {
                    m_bFirstNetworkInfo = false;
                    QCANA_EVT_BASE* pEvt = GetEvent(QCANA_EVTID_CONNECT_CHNAGED);
                    pEvt->llTime = qcGetUTC();
                    SendAnalData(pEvt);
                }
            }
        }
        
        m_bSeeked = false;
    }
    else if(pItem->m_nMsgID == QC_MSG_SNKA_FIRST_FRAME)
    {
        if(m_bAudioRendered || m_bSeeked)
            return QC_ERR_NONE;

        m_bAudioRendered = true;
        QCANA_EVT_STARTUP* pEvtStartup    = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        pEvtStartup->nFirstAudioRenderTime = pItem->m_nValue;
        pEvtStartup->llTime = qcGetUTC();

        if( IsPureAudio() || (pEvtStartup->nFirstVideoRenderTime > 0 && IsGopTimeReady()))
        {
            pEvtStartup->llEvtDuration = pItem->m_nValue;
            SendAnalData(pEvtStartup);
            
            if(m_bFirstNetworkInfo)
            {
                m_bFirstNetworkInfo = false;
                QCANA_EVT_BASE* pEvt = GetEvent(QCANA_EVTID_CONNECT_CHNAGED);
                pEvt->llTime = qcGetUTC();
                SendAnalData(pEvt);
            }
        }
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_GOPTIME)
    {
        if(IsGopTimeReady())
            return QC_ERR_NONE;
        
        ((QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE))->nGopTime = pItem->m_nValue;
        QCANA_EVT_STARTUP* pEvtStartup    = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        pEvtStartup->nGopTime = pItem->m_nValue;
        
        if( (pEvtStartup->llEvtDuration > 0 && m_bAudioRendered) || IsPureVideo())
        {
            pEvtStartup->llEvtDuration = qcGetUTC() - pEvtStartup->llOpenStartTime;
            SendAnalData(pEvtStartup);
            
            if(m_bFirstNetworkInfo)
            {
                m_bFirstNetworkInfo = false;
                QCANA_EVT_BASE* pEvt = GetEvent(QCANA_EVTID_CONNECT_CHNAGED);
                pEvt->llTime = qcGetUTC();
                SendAnalData(pEvt);
            }
        }
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_OPEN_START)
    {
//        CAnalBase::ReleaseResource(&m_pSource);
//        CAnalBase::ReleaseResource(&m_pOldBitrate);
//        CAnalBase::ReleaseResource(&m_pNewBitrate);
//        m_llStopTime = 0;
//        m_bSeeked = false;
//        m_bAudioRendered = false;
//        GetEvent(QCANA_EVTID_STARTUP)->llTime = qcGetUTC();
//        GetEvent(QCANA_EVTID_STARTUP)->llEvtDuration = 0;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_OPEN_DONE)
    {
//        QCANA_EVT_BASE* pEvtOpen     = GetEvent(QCANA_EVTID_OPEN);
//        pEvtOpen->llEvtDuration     = qcGetUTC() - pEvtOpen->llTime;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_SEEK_START)
    {
        m_bSeeked = true;
        m_bAudioRendered = false;
        QCANA_EVT_SEEK* pEvtSeek = (QCANA_EVT_SEEK*)GetEvent(QCANA_EVTID_SEEK);
        pEvtSeek->llTime 		= qcGetUTC();
        pEvtSeek->llWhereFrom 	= pItem->m_nValue;
        pEvtSeek->llWhereTo		= pItem->m_llValue;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_SEEK_DONE)
    {
        long long llTime = qcGetUTC() - GetEvent(QCANA_EVTID_SEEK)->llTime;
        
        QCLOGI("[ANL][KPI]Seek done use time %lld", llTime);
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_RUN)
    {
        if(m_bPaused)
        {
            QCANA_EVT_BASE* pEvtPause 	= GetEvent(QCANA_EVTID_PAUSE);
            QCANA_EVT_BASE* pEvtResume	= GetEvent(QCANA_EVTID_RESUME);

            pEvtResume->llTime = qcGetUTC();
            pEvtResume->llPos = pEvtPause->llPos;
            pEvtResume->llEvtDuration = (int)(pEvtResume->llTime - pEvtPause->llTime);
            SendAnalData(pEvtResume);
        }
        m_bPaused = false;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_PAUSE)
    {
        QCANA_EVT_BASE* pEvtPause 	= GetEvent(QCANA_EVTID_PAUSE);
        pEvtPause->llTime = qcGetUTC();
        pEvtPause->llPos = pItem->m_llValue;
        SendAnalData(pEvtPause);
        m_bPaused = true;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_STOP)
    {
//        int nTime = qcGetSysTime();
//        QCLOGI("[ANL][KPI]+Recv stop, %d, pos %lld", nTime, pItem->m_llValue);
//        QCANA_EVT_BASE* pEvtClose     = GetEvent(QCANA_EVTID_CLOSE);
//        pEvtClose->llTime = qcGetUTC();
//        pEvtClose->llPos = pItem->m_llValue;
//        m_llStopTime = pItem->m_llValue;
//        SendAnalData(pEvtClose);
//        QCLOGI("[ANL][KPI]-Recv stop, %d, use time %d", qcGetSysTime(), qcGetSysTime()-nTime);
    }
    else if(pItem->m_nMsgID == QC_MSG_PARSER_NEW_STREAM)
    {
        ProcessNewStream((QC_RESOURCE_INFO*)pItem->m_pInfo, pItem->m_nValue, pItem->m_llValue);
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_DURATION)
    {
        m_llDuration = pItem->m_llValue;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_START_BUFFERING)
    {
        QCANA_EVT_BASE* pEvtLag = GetEvent(QCANA_EVTID_LAG);
        pEvtLag->llTime = qcGetUTC();
        pEvtLag->llPos	= pItem->m_llValue;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_END_BUFFERING)
    {
        QCANA_EVT_BASE* pEvtLag = GetEvent(QCANA_EVTID_LAG);
        pEvtLag->llEvtDuration = qcGetUTC() - pEvtLag->llTime;
        SendAnalData(pEvtLag);
    }
    else if(pItem->m_nMsgID == QC_MSG_HTTP_DOWNLOAD_SPEED
            || pItem->m_nMsgID == QC_MSG_RTMP_DOWNLOAD_SPEED)
    {
        ProcessDownload(pItem->m_nValue, pItem->m_llValue, pItem->m_szValue);
    }
    else if(pItem->m_nMsgID == QC_MSG_RTMP_METADATA)
    {
        //QCLOGI("MetaData: %s", pItem->m_szValue);
    }
    else if(pItem->m_nMsgID == QC_MSG_HTTP_CONNECT_START
            || pItem->m_nMsgID == QC_MSG_RTMP_CONNECT_START)
    {
        QCANA_EVT_STARTUP* startup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        if(startup->nConnectTime == 0)
            startup->nConnectTime = pItem->m_nTime;
    }
    else if(pItem->m_nMsgID == QC_MSG_HTTP_CONNECT_SUCESS
            || pItem->m_nMsgID == QC_MSG_RTMP_CONNECT_SUCESS)
    {
        QCANA_EVT_STARTUP* startup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        if(startup->nConnectTime > 7777777)
        {
            startup->nConnectTime = pItem->m_nTime - startup->nConnectTime;
            ((QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE))->nConnectTime = startup->nConnectTime;
        }
        if(startup->nFirstByteTime == 0)
            startup->nFirstByteTime = pItem->m_nTime;
    }
    else if(pItem->m_nMsgID == QC_MSG_IO_FIRST_BYTE_DONE)
    {
        QCANA_EVT_STARTUP* startup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
        if(startup->nFirstByteTime > 7777777)
        {
            startup->nFirstByteTime = pItem->m_nTime - startup->nFirstByteTime;
            ((QCANA_EVT_CLOSE*)GetEvent(QCANA_EVTID_CLOSE))->nFirstByteTime = startup->nFirstByteTime;
        }
    }

    //QCLOGI("[ANL]-ReceiveMsg %X, %s, Time %d", pItem->m_nMsgID, pItem->m_szIDName, qcGetSysTime()-nTime);
    
    return QC_ERR_NONE;
}

int CAnalysisMng::ProcessNewStream(QC_RESOURCE_INFO* pRes, int nBA, long long llPos)
{
    if(!pRes)
        return QC_ERR_NONE;
    if(!m_pszPlayURL)
    	return QC_ERR_STATUS;
    
    // The first new stream info
    if(!m_pSource)
    {
        m_pSource = CAnalBase::CloneResource(pRes);
        if(m_pszPlayURL)
        {
            QC_DEL_A(m_pSource->pszURL);
            m_pSource->pszURL = new char[strlen(m_pszPlayURL)+1];
            strcpy(m_pSource->pszURL, m_pszPlayURL);
        }
        
        if(m_pOldBitrate)
            CAnalBase::ReleaseResource(&m_pOldBitrate);
        m_pOldBitrate = CAnalBase::CloneResource(pRes);
        if(m_pNewBitrate)
            CAnalBase::ReleaseResource(&m_pNewBitrate);
        m_pNewBitrate = CAnalBase::CloneResource(pRes);
        UpdateSource();
        return QC_ERR_NONE;
    }
    
    QCANA_EVT_BA* pEvtBA = (QCANA_EVT_BA*)GetEvent(QCANA_EVTID_BA);
    
    if(m_pOldBitrate)
    {
        CAnalBase::ReleaseResource(&m_pOldBitrate);
        m_pOldBitrate = m_pNewBitrate;
    }
    
    m_pNewBitrate = CAnalBase::CloneResource(pRes);
    
    pEvtBA->llTime			= qcGetUTC();
    pEvtBA->nBAMode 		= nBA;
    pEvtBA->llPos 			= llPos;
    CAnalBase::ReleaseResource(&pEvtBA->pBitrateFrom);
    pEvtBA->pBitrateFrom	= CAnalBase::CloneResource(m_pOldBitrate);
    CAnalBase::ReleaseResource(&pEvtBA->pBitrateTo);
    pEvtBA->pBitrateTo		= CAnalBase::CloneResource(m_pNewBitrate);
    SendAnalData(pEvtBA);
    return QC_ERR_NONE;
}

int CAnalysisMng::ProcessDownload(int nSpeed, long long llDownloadPos, char* pszNewURL)
{
    if(nSpeed == 0 || llDownloadPos==0)
    {
        m_llDownloadPos = 0;
        return QC_ERR_NONE;
    }
    
    QCANA_EVT_DLD* pEvtDld = (QCANA_EVT_DLD*)GetEvent(QCANA_EVTID_DOWNLOAD);
    
    // speed is byte per sec
    long long size = llDownloadPos - m_llDownloadPos;
    pEvtDld->llDownloadSize		+= size;
    pEvtDld->nDownloadUseTime 	+= size*1000/nSpeed;
    pEvtDld->llEvtDuration		= pEvtDld->nDownloadUseTime;
    m_llDownloadPos = llDownloadPos;
    
    //QCLOGI("[ANL]Pos %lld, speed %d, (%lld, %d)", llDownloadPos, nSpeed, pEvtDld->llDownloadSize, pEvtDld->nDownloadUseTime);
    
    // wait source open complete
    if(!m_pSource)
        return QC_ERR_NONE;
    
    int nRet = QC_ERR_NONE;
    pEvtDld->llTime = qcGetUTC();
    SendAnalData(pEvtDld);
    
    pEvtDld->llDownloadSize 	= 0;
    pEvtDld->nDownloadUseTime 	= 0;

    return nRet;
}

void CAnalysisMng::ResetEvents()
{
    int nEvtID				= 0;
    NODEPOS pos 			= m_Events.GetHeadPosition();
    QCANA_EVT_BASE* pEvt 	= m_Events.GetNext(pos);
    while (pEvt)
    {
        nEvtID = pEvt->nEventID;
        if(pEvt->nEventID == QCANA_EVTID_BA)
        {
            QCANA_EVT_BA* pBA = (QCANA_EVT_BA*)pEvt;
            CAnalBase::ReleaseResource(&pBA->pBitrateFrom);
            CAnalBase::ReleaseResource(&pBA->pBitrateTo);
        }
        CAnalBase::ReleaseResource(&pEvt->pSrcInfo);
        memset(pEvt, 0, sizeof(*pEvt));
        pEvt->nEventID = nEvtID;
        pEvt = m_Events.GetNext(pos);
    }
}


#pragma mark Other
void CAnalysisMng::CreateSessionID()
{
    memset(m_szSessionID, 0, 37);
    
    char utc[16];
    sprintf(utc, "%lld", qcGetUTC());
    char *p = m_szSessionID;
    int n;
    
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;
        
        switch( n )
        {
            case 6:
                sprintf(
                        p,
                        "4%x",
                        b%15 );
                break;
            case 8:
                sprintf(
                        p,
                        "%c%x",
                        utc[rand()%strlen( utc )],
                        b%15 );
                break;
            default:
                sprintf(
                        p,
                        "%02x",
                        b );
                break;
        }
        
        p += 2;
        
        switch( n )
        {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    
    *p = 0;
    
//    NODEPOS pos             = m_Events.GetHeadPosition();
//    QCANA_EVT_BASE* pEvt     = m_Events.GetNext(pos);
//    while (pEvt)
//    {
//        sprintf(pEvt->szSessionID, "%s", m_szSessionID);
//        pEvt = m_Events.GetNext(pos);
//    }
}

void CAnalysisMng::CreateClientInfo(int nVersion)
{
    memset(&m_DeviceInfo, 0, sizeof(m_DeviceInfo));
    
    GetIPAddress(m_DeviceInfo.szDeviceIP, 64);
    //
    sprintf(m_DeviceInfo.szPlayerVersion, "%d.%d.%d.%d",
            (nVersion>>24) & 0xFF, (nVersion>>16) & 0xFF, (nVersion>>8) & 0xFF, nVersion&0xFF);
    //QCLOGI("SDK version %s, %s %s", m_DeviceInfo.szPlayerVersion, __TIME__,  __DATE__);
    
    strcpy(m_DeviceInfo.szAppFamily, UNKNOWN);
    strcpy(m_DeviceInfo.szAppID, UNKNOWN);
    strcpy (m_DeviceInfo.szAppVersion, UNKNOWN);
#ifdef __QC_OS_IOS__
    //
    NSString* appVerion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    if(appVerion)
        strcpy(m_DeviceInfo.szAppVersion, [appVerion UTF8String]);
    else
        strcpy(m_DeviceInfo.szAppVersion, UNKNOWN);

    //
    NSString* appID = [[NSBundle mainBundle] bundleIdentifier];
    if(appID)
        strcpy(m_DeviceInfo.szAppID, [appID UTF8String]);
    else
        strcpy(m_DeviceInfo.szAppID, UNKNOWN);
    
    NSString * uuid = [[UIDevice currentDevice] identifierForVendor].UUIDString;
    if(uuid == nil)
        uuid = @"simulator";
    sprintf(m_DeviceInfo.szDeviceID, "%s", [uuid UTF8String]);

    //
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    sysctlbyname("hw.machine", m_DeviceInfo.szDeviceFamily, &size, NULL, 0);

    //
    strcpy(m_DeviceInfo.szOSFamily, "iOS");
    
    //
    strcpy(m_DeviceInfo.szOSVersion, [[UIDevice currentDevice].systemVersion UTF8String]);
    
#elif defined __QC_OS_NDK__
    char szAppPath[256];
    qcGetAppPath (NULL, szAppPath, sizeof (szAppPath));
    szAppPath[strlen (szAppPath)-1] = 0;
    //strcpy (m_DeviceInfo.szDeviceID, szAppPath + 11);
    //strcpy (m_DeviceInfo.szAppID, szAppPath + 11);
    //
	//strcpy (m_DeviceInfo.szAppVersion, UNKNOWN);

    char prop[PROP_VALUE_MAX];

    //
    memset(prop, 0, PROP_VALUE_MAX);
	__system_property_get ("ro.product.manufacturer", prop);
    if(strlen(prop) >= 64)
        strncpy(m_DeviceInfo.szDeviceFamily, prop, 63);
    else
        strcpy(m_DeviceInfo.szDeviceFamily, prop);
    //QCLOGI("[ANL]Manufactory is %s", m_DeviceInfo.szDeviceFamily);
    
    //
	strcpy (m_DeviceInfo.szOSFamily, "Android");
    
    //
    memset(prop, 0, PROP_VALUE_MAX);
	__system_property_get ("ro.build.version.release", prop);
    if(strlen(prop) >= 16)
        strncpy(m_DeviceInfo.szOSVersion, prop, 15);
    else
        strcpy(m_DeviceInfo.szOSVersion, prop);
    //QCLOGI("[ANL]OS version is %s", m_DeviceInfo.szOSVersion);
    
#elif defined __QC_OS_WIN32__
    //
	strcpy (m_DeviceInfo.szAppVersion, UNKNOWN);

    //
	strcpy (m_DeviceInfo.szDeviceFamily, UNKNOWN);

    //
	strcpy (m_DeviceInfo.szOSFamily, "Windows");

    //
	OSVERSIONINFO osVer;
	memset (&osVer, 0, sizeof (osVer));
	osVer.dwOSVersionInfoSize = sizeof (osVer);
	GetVersionEx (&osVer);
	sprintf (m_DeviceInfo.szOSVersion, "%d.%d", osVer.dwMajorVersion, osVer.dwMinorVersion);
#endif
}

void CAnalysisMng::GetIPAddress(char* pAddress, int nLen)
{
    char* address = pAddress;
    memset(address, 0, nLen);

#ifdef __QC_OS_IOS__
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = getifaddrs(&interfaces);
    if (success == 0)
    {
        //Loop through linked list of interfaces
        temp_addr = interfaces;
        while (temp_addr != NULL)
        {
            if (temp_addr->ifa_addr->sa_family == AF_INET)
            {
                if(!strcmp(temp_addr->ifa_name, "en0"))
                {
                    char* addr = inet_ntoa(((struct sockaddr_in *) temp_addr->ifa_addr)->sin_addr);
                    strcpy(address, inet_ntoa(((struct sockaddr_in *) temp_addr->ifa_addr)->sin_addr));
                    break;
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    freeifaddrs(interfaces);
#elif defined __QC_OS_WIN32__
 	if (SOCKET_ERROR != gethostname(address, 64))  
	{  
		struct hostent* hp;  
		hp = gethostbyname(address);  
		if (hp != NULL && hp->h_addr_list[0] != NULL)  
		{  
			// IPv4: Address is four bytes (32-bit)  
			if ( hp->h_length < 4)  
				return;  

			// Convert address to . format  
			address[0] = 0;  

			// IPv4: Create Address string  
			sprintf(address, "%u.%u.%u.%u",  
					(UINT)(((PBYTE) hp->h_addr_list[0])[0]),  
					(UINT)(((PBYTE) hp->h_addr_list[0])[1]),  
					(UINT)(((PBYTE) hp->h_addr_list[0])[2]),  
					(UINT)(((PBYTE) hp->h_addr_list[0])[3]));  
		}  
	} 
	else
	{ 
		int nErr = WSAGetLastError ();
		QCLOGW ("Get IP error %d", nErr);
	}
#elif defined __QC_OS_NDK__
	__system_property_get ("net.dns1", address);
#endif
}

QCANA_EVT_BASE* CAnalysisMng::AllocEvent(int nEvtID)
{
    QCANA_EVT_BASE* evt = NULL;
    
    if(QCANA_EVTID_CLOSE == nEvtID)
    {
        evt = new QCANA_EVT_CLOSE;
        memset(evt, 0, sizeof(QCANA_EVT_CLOSE));
    }
    else if(QCANA_EVTID_SEEK == nEvtID)
    {
        evt = new QCANA_EVT_SEEK;
        memset(evt, 0, sizeof(QCANA_EVT_SEEK));
    }
    else if(QCANA_EVTID_BA == nEvtID)
    {
        evt = new QCANA_EVT_BA;
        memset(evt, 0, sizeof(QCANA_EVT_BA));
    }
    else if(QCANA_EVTID_DOWNLOAD == nEvtID)
    {
        evt = new QCANA_EVT_DLD;
        memset(evt, 0, sizeof(QCANA_EVT_DLD));
    }
    else if(QCANA_EVTID_STOP == nEvtID)
    {
        evt = new QCANA_EVT_STOP;
        memset(evt, 0, sizeof(QCANA_EVT_STOP));
    }
    else if(QCANA_EVTID_STARTUP == nEvtID)
    {
        evt = new QCANA_EVT_STARTUP;
        memset(evt, 0, sizeof(QCANA_EVT_STARTUP));
    }
    else
    {
        evt = new QCANA_EVT_BASE;
        memset(evt, 0, sizeof(QCANA_EVT_BASE));
    }
    
    if(evt)
    {
        evt->nEventID = nEvtID;
    }
    
    return evt;
}

void CAnalysisMng::CreateEvents()
{
    for(int n=0; n<QCANA_EVTID_MAX; n++)
    {
        QCANA_EVT_BASE* evt = AllocEvent(n);
        if(evt)
        {
            m_Events.AddTail(evt);
        }
    }
}

QCANA_EVT_BASE* CAnalysisMng::CloneEvent(QCANA_EVT_BASE* pEvent)
{
    QCANA_EVT_BASE* evt = AllocEvent(pEvent->nEventID);
    if(!evt)
        return NULL;
    
    evt->llTime			= pEvent->llTime;
    evt->llPos			= pEvent->llPos;
    evt->llOpenStartTime= pEvent->llOpenStartTime;
    evt->llEvtDuration 	= pEvent->llEvtDuration;
    evt->nErrCode		= pEvent->nErrCode;
    strcpy(evt->szSessionID, pEvent->szSessionID);
    
    if(pEvent->pSrcInfo)
        evt->pSrcInfo = CAnalBase::CloneResource(pEvent->pSrcInfo);

    if(QCANA_EVTID_CLOSE == pEvent->nEventID)
    {
        QCANA_EVT_CLOSE* close = (QCANA_EVT_CLOSE*)evt;
        close->nGopTime = ((QCANA_EVT_CLOSE*)pEvent)->nGopTime;
        close->llWatchDuration = ((QCANA_EVT_CLOSE*)pEvent)->llWatchDuration;
        close->nConnectTime = ((QCANA_EVT_CLOSE*)pEvent)->nConnectTime;
        close->nFirstByteTime = ((QCANA_EVT_CLOSE*)pEvent)->nFirstByteTime;
    }
    else if(QCANA_EVTID_STOP == pEvent->nEventID)
    {
        QCANA_EVT_CLOSE* stop = (QCANA_EVT_STOP*)evt;
        stop->llWatchDuration = ((QCANA_EVT_STOP*)pEvent)->llWatchDuration;
    }
    else if(QCANA_EVTID_SEEK == pEvent->nEventID)
    {
        QCANA_EVT_SEEK* seek = (QCANA_EVT_SEEK*)evt;
        seek->llWhereFrom	= ((QCANA_EVT_SEEK*)pEvent)->llWhereFrom;
        seek->llWhereTo		= ((QCANA_EVT_SEEK*)pEvent)->llWhereTo;
    }
    else if(QCANA_EVTID_BA == pEvent->nEventID)
    {
        QCANA_EVT_BA* ba = (QCANA_EVT_BA*)evt;
		ba->llBAPosition = ((QCANA_EVT_BA*)pEvent)->llBAPosition;
		ba->nBAMode = ((QCANA_EVT_BA*)pEvent)->nBAMode;
		ba->pBitrateFrom = CAnalBase::CloneResource(((QCANA_EVT_BA*)pEvent)->pBitrateFrom);
		ba->pBitrateTo = CAnalBase::CloneResource(((QCANA_EVT_BA*)pEvent)->pBitrateTo);
    }
    else if(QCANA_EVTID_DOWNLOAD == pEvent->nEventID)
    {
        QCANA_EVT_DLD* dld = (QCANA_EVT_DLD*)evt;
        dld->llDownloadSize	= ((QCANA_EVT_DLD*)pEvent)->llDownloadSize;
        dld->nDownloadUseTime = ((QCANA_EVT_DLD*)pEvent)->nDownloadUseTime;
    }
    else if(QCANA_EVTID_STARTUP == pEvent->nEventID)
    {
        QCANA_EVT_STARTUP* startup = (QCANA_EVT_STARTUP*)evt;
        startup->nGopTime				= ((QCANA_EVT_STARTUP*)pEvent)->nGopTime;
        startup->nFirstAudioRenderTime	= ((QCANA_EVT_STARTUP*)pEvent)->nFirstAudioRenderTime;
        startup->nFirstVideoRenderTime	= ((QCANA_EVT_STARTUP*)pEvent)->nFirstVideoRenderTime;
        startup->nConnectTime			= ((QCANA_EVT_STARTUP*)pEvent)->nConnectTime;
        startup->nFirstByteTime         = ((QCANA_EVT_STARTUP*)pEvent)->nFirstByteTime;
    }

    return evt;
}

void CAnalysisMng::ReleaseEvents()
{    
    QCANA_EVT_BASE* pEvt 	= m_Events.RemoveHead();
    while (pEvt)
    {
        CAnalBase::ReleaseEvent(pEvt);
        pEvt = m_Events.RemoveHead();
    }
    
    CAutoLock lock(&m_mtEvtCache);
    pEvt = m_EventCache.RemoveHead();
    while (pEvt)
    {
        CAnalBase::ReleaseEvent(pEvt);
        pEvt = m_EventCache.RemoveHead();
    }
}

void CAnalysisMng::ReleaseAnal()
{
    CAnalBase* pAnal = m_listAnal.RemoveHead ();
    while (pAnal != NULL)
    {
        delete pAnal;
        pAnal = m_listAnal.RemoveHead ();
    }
}

void CAnalysisMng::AnalStop()
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->Stop();
        pAnal = m_listAnal.GetNext(pos);
    }
}

void CAnalysisMng::AnalOpen()
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->Open();
        pAnal = m_listAnal.GetNext(pos);
    }
}

QCANA_EVT_BASE* CAnalysisMng::GetEvent(QCANA_EVT_ID nEvtID)
{
    NODEPOS pos 			= m_Events.GetHeadPosition();
    QCANA_EVT_BASE* pEvt 	= m_Events.GetNext(pos);
    while (pEvt)
    {
        if(pEvt->nEventID == nEvtID)
            return pEvt;
        pEvt = m_Events.GetNext(pos);
    }
    
    return NULL;
}

int CAnalysisMng::OnWorkItem (void)
{
    if(qcGetSysTime() - m_nSendTime >= 1000)
    {
        //AnalPostData(false, m_pPlayer->GetPos());
        //SendCacheData(false, m_pPlayer->GetPos());
        m_nSendTime = qcGetSysTime();
    }
    
    if(qcGetSysTime() - m_nTimerTime > 1000)
    {
        CAutoLock lock(&m_mtEvt);
        NODEPOS pos = m_listAnal.GetHeadPosition();
        CAnalBase* pAnal = m_listAnal.GetNext(pos);
        while (pAnal)
        {
            pAnal->onTimer();
            pAnal = m_listAnal.GetNext(pos);
        }
        
        m_nTimerTime = qcGetSysTime();
    }
    
    qcSleep(5000);
    return QC_ERR_NONE;
}

void onNetworkConnectionChanged(void* pUserData, int nConnectionStatus)
{
    CAnalysisMng* pAnl = (CAnalysisMng*)pUserData;
    
    if(pAnl)
        pAnl->OnNetworkConnectChanged(nConnectionStatus);
}

// 0: NotReachable 1: ReachableViaWiFi  2: ReachableViaWWAN
void CAnalysisMng::OnNetworkConnectChanged(int nConnectStatus)
{
    QCLOGI("[ANL]Nework connect chnaged, %d, %d", nConnectStatus, m_bAnalDestroyed?1:0);
    if(0 == nConnectStatus)
    {
    }
    else if(1 == nConnectStatus || 2 == nConnectStatus)
    {
        if(m_pBaseInst)
            m_pBaseInst->NotifyNetChanged();
    }
}

void CAnalysisMng::StartReachability()
{
#ifdef __QC_OS_IOS__
    if(!NETWORKSTA)
    {
    	m_pNetworkSta = [CNetworkSta networkStaForInternetConnection];
        [NETWORKSTA setEvtCallback:onNetworkConnectionChanged userData:this];
    }
    
    [NETWORKSTA startNotifier];
#endif
}

void CAnalysisMng::StopReachability()
{
#ifdef __QC_OS_IOS__
    if(!NETWORKSTA)
        return;
    [NETWORKSTA stopNotifier];
    [NETWORKSTA release];
    m_pNetworkSta = nil;
#endif
}

bool CAnalysisMng::IsPlaying()
{
    return strlen(m_szSessionID) > 0;
}

int CAnalysisMng::RecvEvent(int nEventID)
{
    if(nEventID == QC_BASEINST_EVENT_NETCHANGE)
    {
        if(m_bAnalDestroyed)
            return QC_ERR_NONE;

        QCLOGI("[ANL]Recv network change event");
        CAutoLock lock(&m_mtEvt);
        QCANA_EVT_BASE* pEvt = GetEvent(QCANA_EVTID_CONNECT_CHNAGED);
        pEvt->llTime = qcGetUTC();
        SendAnalData(pEvt);
    }
    
    return QC_ERR_NONE;
}

bool CAnalysisMng::IsDNSReady()
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        if(!pAnal->IsDNSParsed())
            return false;
        pAnal = m_listAnal.GetNext(pos);
    }

    return true;
}

int CAnalysisMng::Disconnect()
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->Disconnect();
        pAnal = m_listAnal.GetNext(pos);
    }
    
    return QC_ERR_NONE;
}

void CAnalysisMng::UpdateSource()
{
    NODEPOS pos = m_Events.GetHeadPosition();
    QCANA_EVT_BASE* pEvt = m_Events.GetNext(pos);
    while (pEvt)
    {
        CAnalBase::ReleaseResource(&pEvt->pSrcInfo);
        pEvt->pSrcInfo = CAnalBase::CloneResource(m_pSource);
        sprintf(pEvt->szSessionID, "%s", m_szSessionID);
        
        pEvt = m_Events.GetNext(pos);
    }
    
    pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->UpdateSource(m_pSource);
        pAnal = m_listAnal.GetNext(pos);
    }
}

bool CAnalysisMng::IsPureAudio()
{
    if(!m_pSource)
        return false;
    if(m_pSource->nWidth <= 0 && m_pSource->nHeight <=0)
        return true;
    return false;
}

bool CAnalysisMng::IsPureVideo()
{
    if(!m_pSource)
        return false;
    if(m_pSource->nAudioCodec == QC_CODEC_ID_NONE || m_pSource->nAudioCodec == QC_CODEC_ID_MAX)
        return true;
    return false;
}

bool CAnalysisMng::IsGopTimeReady()
{
    QCANA_EVT_STARTUP* pStartup = (QCANA_EVT_STARTUP*)GetEvent(QCANA_EVTID_STARTUP);
    if(!pStartup)
        return false;
    
    return pStartup->nGopTime > 0;
}

void CAnalysisMng::UpdateDNSServer(const char* pszServer)
{
    NODEPOS pos = m_listAnal.GetHeadPosition();
    CAnalBase* pAnal = m_listAnal.GetNext(pos);
    while (pAnal)
    {
        pAnal->UpdateDNSServer(pszServer);
        pAnal = m_listAnal.GetNext(pos);
    }
}
