/*******************************************************************************
	File:		CAnalBase.cpp
 
	Contains:	Base analysis collection implementation code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-25		Jun			Create file
 
 *******************************************************************************/
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "CAnalBase.h"
#include "qcErr.h"
#include <time.h>

CAnalBase::CAnalBase(CBaseInst * pBaseInst)
	: CBaseObject(pBaseInst)
	, m_pCurrEvtInfo(NULL)
	, m_pCurrSource(NULL)
	, m_nCurrBodySize(0)
	, m_bLive(true)
	, m_pSender(NULL)
{
    SetObjectName("CAnalBase");
    ResetData();
}

CAnalBase::~CAnalBase()
{
    ReleaseResource(&m_pCurrSource);
}

int CAnalBase::ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect/*=true*/)
{
    if(pEvent->pEvtInfo)
    {
        QCANA_SOURCE_INFO* pSrc = pEvent->pEvtInfo->pSrcInfo;
        if(pSrc)
        {
//            if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_OPEN || pEvent->pEvtInfo->nEventID == QCANA_EVTID_STOP)
//                QCLOGI("[ANL]Duration %lld, ID %d, URL %s", pSrc->llDuration, pEvent->pEvtInfo->nEventID, pSrc->pszURL);
            m_bLive = (pSrc->llDuration <= 0);
        }
    }

    return QC_ERR_NONE;
}

int CAnalBase::onMsg (CMsgItem* pItem)
{
    return QC_ERR_IMPLEMENT;
}

int CAnalBase::onTimer()
{
    return QC_ERR_IMPLEMENT;
}

int CAnalBase::ReportEvents(CObjectList<QCANA_EVENT_INFO>* pEvents)
{
    return QC_ERR_IMPLEMENT;
}

int CAnalBase::Stop()
{
    return QC_ERR_IMPLEMENT;
}

void CAnalBase::ResetData()
{
    m_nCurrBodySize = 0;
    memset(m_szBody, 0, ANAL_BODY_LEN);
    memset(m_szHeader, 0, ANAL_HEADER_LEN);
}

void CAnalBase::GetGMTString(char* pszGMT, int nLen)
{
    static char* g_weekstr[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static char* g_monthstr[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    
    time_t now = time(NULL);
    struct tm tt;
#ifdef __QC_OS_WIN32__
	localtime_s ( &tt, &now );
#else
    gmtime_r( &now, &tt );
#endif // __QC_OS_WIN32__
    sprintf(pszGMT,
             "%s, %02d %s %d %02d:%02d:%02d GMT",
             g_weekstr[tt.tm_wday], tt.tm_mday,
             g_monthstr[tt.tm_mon], tt.tm_year + 1900,
             tt.tm_hour, tt.tm_min, tt.tm_sec );
}

char* CAnalBase::GetEvtName(int nEvtID)
{
    if(nEvtID == QCANA_EVTID_OPEN)
        return (char*)"open";
    else if(nEvtID == QCANA_EVTID_CLOSE)
        return (char*)"close";
    else if(nEvtID == QCANA_EVTID_STARTUP)
        return (char*)"startup";
    else if(nEvtID == QCANA_EVTID_LAG)
        return (char*)"lag";
    else if(nEvtID == QCANA_EVTID_SEEK)
        return (char*)"seek";
    else if(nEvtID == QCANA_EVTID_PAUSE)
        return (char*)"pause";
    else if(nEvtID == QCANA_EVTID_BA)
        return (char*)"ba";
    else if(nEvtID == QCANA_EVTID_RESUME)
        return (char*)"resume";
    else if(nEvtID == QCANA_EVTID_DOWNLOAD)
        return (char*)"download";
    
    return (char*)"";
}

bool CAnalBase::IsDNSParsed()
{
    return false;
}

int CAnalBase::Disconnect()
{
    return QC_ERR_IMPLEMENT;
}

int CAnalBase::UpdateSource(QCANA_SOURCE_INFO* pSrc)
{
    if(!pSrc)
        return QC_ERR_EMPTYPOINTOR;
    
    CAutoLock lock(&m_mtReport);
    ReleaseResource(&m_pCurrSource);
    m_pCurrSource = CloneResource(pSrc);
    m_bLive = (m_pCurrSource->nLiveStream == 1);
    
    return QC_ERR_NONE;
}

QCANA_SOURCE_INFO* CAnalBase::CloneResource(QCANA_SOURCE_INFO* pSrc)
{
    if(!pSrc)
        return NULL;
    
    QCANA_SOURCE_INFO* pClone = new QCANA_SOURCE_INFO;
    //memcpy(pClone, pSrc, sizeof(QCANA_SOURCE_INFO));
    memset(pClone, 0, sizeof(QCANA_SOURCE_INFO));
    
    if(pSrc->pszURL)
    {
        pClone->pszURL = new char[strlen(pSrc->pszURL)+1];
        strcpy(pClone->pszURL, pSrc->pszURL);
    }
    if(pSrc->pszFormat)
    {
        pClone->pszFormat = new char[strlen(pSrc->pszFormat)+1];
        strcpy(pClone->pszFormat, pSrc->pszFormat);
    }
    
    pClone->llDuration    = pSrc->llDuration;
    pClone->nVideoCodec    = pSrc->nVideoCodec;
    pClone->nAudioCodec    = pSrc->nAudioCodec;
    pClone->nBitrate    = pSrc->nBitrate;
    pClone->nWidth        = pSrc->nWidth;
    pClone->nHeight        = pSrc->nHeight;
    pClone->nLiveStream    = pSrc->nLiveStream;
    
    return pClone;
}

QCANA_SOURCE_INFO* CAnalBase::CloneResource(QC_RESOURCE_INFO* pSrc)
{
    if(!pSrc)
        return NULL;
    
    QCANA_SOURCE_INFO* pClone = new QCANA_SOURCE_INFO;
    memset(pClone, 0, sizeof(QCANA_SOURCE_INFO));
    
    if(pSrc->pszURL)
    {
        pClone->pszURL = new char[strlen(pSrc->pszURL)+1];
        strcpy(pClone->pszURL, pSrc->pszURL);
    }
    if(pSrc->pszFormat)
    {
        pClone->pszFormat = new char[strlen(pSrc->pszFormat)+1];
        strcpy(pClone->pszFormat, pSrc->pszFormat);
    }
    
    pClone->llDuration    = pSrc->llDuration;
    pClone->nVideoCodec    = pSrc->nVideoCodec;
    pClone->nAudioCodec    = pSrc->nAudioCodec;
    pClone->nBitrate    = pSrc->nBitrate;
    pClone->nWidth        = pSrc->nWidth;
    pClone->nHeight        = pSrc->nHeight;
    pClone->nLiveStream    = (pSrc->llDuration<=0) ? 1 : 0;
    
    return pClone;
}

void CAnalBase::ReleaseResource(QCANA_SOURCE_INFO** pRes)
{
    if(!pRes || !(*pRes))
        return;
    
    QCANA_SOURCE_INFO* res = *pRes;
    QC_DEL_A(res->pszURL);
    QC_DEL_A(res->pszFormat);
    QC_DEL_P(res);
    *pRes = NULL;
}

void CAnalBase::ReleaseEvent(QCANA_EVT_BASE* pEvt)
{
    if(!pEvt)
        return;
    if(pEvt->nEventID == QCANA_EVTID_BA)
    {
        QCANA_EVT_BA* pBA = (QCANA_EVT_BA*)pEvt;
        ReleaseResource(&pBA->pBitrateFrom);
        ReleaseResource(&pBA->pBitrateTo);
    }
    
    ReleaseResource(&pEvt->pSrcInfo);
    QC_DEL_P(pEvt);
}

bool CAnalBase::IsReportOpenStopEvt()
{
    return true;
//    char szAppID[512];
//    memset(szAppID, 0, 512);
//    qcGetAppID(szAppID, 512);
//
//    if(strstr(szAppID, "com.blued.international"))
//        return true;
//    else if(strstr(szAppID, "com.soft.blued"))
//        return true;
//    if(strstr(szAppID, "com.bluecity.blued"))
//        return true;
//    else if(strstr(szAppID, "com.bluecity.blued.hd"))
//        return true;
//    if(strstr(szAppID, "com.bluecity.blued.qy"))
//        return true;
//    else if(strstr(szAppID, "com.blued.international.qy"))
//        return true;
//    if(strstr(szAppID, "com.soft.blued"))
//        return true;
//    else if(strstr(szAppID, "com.blued.internationam"))
//        return true;
//    else if(strstr(szAppID, "com.qiniu."))
//        return true;
//
//    return false;
}

int CAnalBase::Open()
{
    return QC_ERR_NONE;
}

int CAnalBase::PostData()
{
    return QC_ERR_NONE;
}

int CAnalBase::UpdateDNSServer(const char* pszServer)
{
    if(m_pSender)
        m_pSender->UpdateDNSServer(pszServer);
    return QC_ERR_NONE;
}




