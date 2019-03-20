/*******************************************************************************
	File:		CAnalJedi.cpp
 
	Contains:	CAnalJedi analysis collection implementation code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-28		Jun			Create file
 
 *******************************************************************************/
#include "CAnalJedi.h"
#include "qcErr.h"
#include "ULogFunc.h"
#include "UUrlParser.h"

#ifdef __QC_OS_NDK__
#include <stdlib.h>
#include <math.h>
#include <sys/system_properties.h>
#endif // __QC_OS_NDK__

CAnalJedi::CAnalJedi(CBaseInst * pBaseInst)
	: CAnalBase(pBaseInst)
	, m_bPaused(false)
	, m_pSource(NULL)
{
    SetObjectName("CAnalJedi");
    GetTimeZone();
    sprintf(m_szAuth, "%s", (char*)"A94B881E5C7749A48231F9C0");
    sprintf(m_szServer, "%s", (char*)"lightsaber.qiniuapi.com");
    m_pSender = new CAnalDataSender(m_pBaseInst, NULL, m_szServer);
}

CAnalJedi::~CAnalJedi()
{
    QC_DEL_P(m_pSender);
}

int CAnalJedi::Stop()
{
    if(m_pSender)
        m_pSender->Stop();
    return QC_ERR_NONE;
}

char* CAnalJedi::GetEvtName(int nEvtID)
{
    if(nEvtID == QCANA_EVTID_OPEN)
        return (char*)"vopen";
    else if(nEvtID == QCANA_EVTID_CLOSE)
        return (char*)"vclose";
    else if(nEvtID == QCANA_EVTID_STARTUP)
        return (char*)"vstart";
    else if(nEvtID == QCANA_EVTID_LAG)
        return (char*)"vlag";
    else if(nEvtID == QCANA_EVTID_SEEK)
        return (char*)"vseek";
    else if(nEvtID == QCANA_EVTID_PAUSE)
        return (char*)"vpause";
    else if(nEvtID == QCANA_EVTID_BA)
        return (char*)"vchange";
    else if(nEvtID == QCANA_EVTID_RESUME)
        return (char*)"vresume";
    else if(nEvtID == QCANA_EVTID_DOWNLOAD)
        return (char*)"vdownload";
    
    return (char*)"";
}

int CAnalJedi::ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect/*=true*/)
{
    if(!pEvent || !pEvent->pDevInfo || !pEvent->pEvtInfo || !pEvent->pEvtInfo->pSrcInfo)
        return QC_ERR_EMPTYPOINTOR;
    
    CAnalBase::ReportEvent(pEvent, bDisconnect);

    m_pCurrEvtInfo = pEvent;
    
    if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_OPEN)
        AssembleOpenEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_CLOSE)
        AssembleCloseEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_STARTUP)
        AssembleStartEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_LAG)
        AssembleLagEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_SEEK)
        AssembleSeekEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_PAUSE)
        AssemblePauseEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_BA)
        AssembleChangeEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_RESUME)
        AssembleResumeEvent();
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_DOWNLOAD)
        AssembleDownloadEvent();
    else
        return QC_ERR_FAILED;
    
    if(m_pSender)
    {
        char GMT[32];
        GetGMTString(GMT, 32);
        int nLen = sprintf(m_szHeader, "POST %s%s HTTP/1.1\r\nHost: %s\r\nDate: %s\r\nContent-Type: %s\r\nX-Token: %s\r\nX-Data-Version: %s\r\nContent-Length: %d\r\n\r\n",
                 "/v2/event/", GetEvtName(pEvent->pEvtInfo->nEventID), m_szServer, GMT, "application/json", m_szAuth, "0.5", m_nCurrBodySize);
        m_pSender->PostData(m_szHeader, nLen);
        m_pSender->PostData(m_szBody, m_nCurrBodySize);
        nLen = ANAL_BODY_LEN;
        m_pSender->ReadResponse(m_szBody, nLen);
    }
    
    return QC_ERR_NONE;
}

int CAnalJedi::onMsg (CMsgItem* pItem)
{
    if(pItem->m_nMsgID == QC_MSG_PLAY_PAUSE)
    {
        m_bPaused = true;
        return QC_ERR_NONE;
    }
    else if(pItem->m_nMsgID == QC_MSG_PLAY_OPEN_START)
    {
        return QC_ERR_NONE;
    }
    return QC_ERR_IMPLEMENT;
}

#pragma mark Basic assemble
int CAnalJedi::AssembleStart(char* pData, long long llWhen, char* szName)
{
    int nCurrLength = 0;
    // json start
    nCurrLength += sprintf(pData+nCurrLength, "{\"session\":\"%s\",\"when\":%lld", m_pCurrEvtInfo->pEvtInfo->szSessionID, llWhen);
    
    return nCurrLength;
}

int CAnalJedi::AssembleEnd(char* pData)
{
    int nCurrLength = 0;
    
    // json end
    nCurrLength += sprintf(pData+nCurrLength, "%s", "}");
    
    return nCurrLength;
}

int	CAnalJedi::AssembleClient(char* pData)
{
    int nCurrLength = 0;
    
    nCurrLength += sprintf(pData, ",\"client\":{\"zone\":%s,\"ip\":\"%s\"",
                           "0",
                           m_pCurrEvtInfo->pDevInfo->szDeviceIP);
    
    nCurrLength += AssembleDeviceInfo(pData+nCurrLength);
    nCurrLength += AssembleOSInfo(pData+nCurrLength);
    nCurrLength += AssembleAppInfo(pData+nCurrLength);
    nCurrLength += AssemblePlayerInfo(pData+nCurrLength);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    
    nCurrLength += sprintf(pData+nCurrLength, ",\"refer\":\"%s\"", UNKNOWN);
    
    return nCurrLength;
}

int	CAnalJedi::AssembleDeviceInfo(char* pData)
{
    int nCurrLength = 0;
    nCurrLength += sprintf(pData, ",\"device\":{\"family\":\"%s\",\"version\":\"%s\"",
                           m_pCurrEvtInfo->pDevInfo->szDeviceFamily,
                           UNKNOWN);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    return nCurrLength;
}

int	CAnalJedi::AssembleOSInfo(char* pData)
{
    int nCurrLength = 0;
    nCurrLength += sprintf(pData, ",\"os\":{\"family\":\"%s\",\"version\":\"%s\"",
                           m_pCurrEvtInfo->pDevInfo->szOSFamily,
                           m_pCurrEvtInfo->pDevInfo->szOSVersion);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    return nCurrLength;
}

int	CAnalJedi::AssembleAppInfo(char* pData)
{
    int nCurrLength = 0;
    nCurrLength += sprintf(pData, ",\"app\":{\"family\":\"%s\",\"version\":\"%s\"",
                           m_pCurrEvtInfo->pDevInfo->szAppFamily,
                           m_pCurrEvtInfo->pDevInfo->szAppVersion);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    return nCurrLength;
}

int	CAnalJedi::AssemblePlayerInfo(char* pData)
{
    int nCurrLength = 0;
    nCurrLength += sprintf(pData, ",\"player\":{\"family\":\"%s\",\"version\":\"%s\"}",
                           "corePlayer",
                           m_pCurrEvtInfo->pDevInfo->szPlayerVersion);
    return nCurrLength;
}

int	CAnalJedi::AssembleResource(char* pData)
{
    return AssembleResource(pData, (char*)"resource", m_pCurrEvtInfo->pEvtInfo->pSrcInfo);
}

int	CAnalJedi::AssembleResource(char* pData, char* pszRes, QCANA_SOURCE_INFO* pRes)
{
    if(!pRes)
        return 0;
    
    int nPort = 0;
    char szHost[1024];
    char szPath[1024];
    qcUrlParseUrl(pRes->pszURL, szHost, szPath, nPort, NULL);
    
    int nCurrLength = 0;
    nCurrLength += sprintf(pData, ",\"%s\":{\"url\":\"%s\",\"domain\":\"%s\",\"format\":\"%s\",\"duration\":%lld",
                           pszRes, pRes->pszURL, szHost, pRes->pszFormat, pRes->llDuration);
    
    nCurrLength += AssembleVideoInfo(pData+nCurrLength, pRes);
    nCurrLength += AssembleAudioInfo(pData+nCurrLength, pRes);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    
    return nCurrLength;
}

int	CAnalJedi::AssembleVideoInfo(char* pData, QCANA_SOURCE_INFO* pRes)
{
    if(!pRes)
        return 0;
    
    int nCurrLength = 0;
    char* codec = (pRes->nVideoCodec==QC_CODEC_ID_H264) ? (char*)"h264":(char*)"h265";
    nCurrLength += sprintf(pData, ",\"video\":{\"codec\":\"%s\",\"frame_rate\":%d,\"bit_rate\":%d,\"width\":%d,\"height\":%d", codec, 0, pRes->nBitrate, pRes->nWidth, pRes->nHeight);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    
    return nCurrLength;
}

int	CAnalJedi::AssembleAudioInfo(char* pData, QCANA_SOURCE_INFO* pRes)
{
    if(!pRes)
        return 0;
    
    int nCurrLength = 0;
    char* codec = (pRes->nAudioCodec==QC_CODEC_ID_AAC) ? (char*)"aac":(char*)UNKNOWN;
    nCurrLength += sprintf(pData, ",\"audio\":{\"codec\":\"%s\"", codec);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    
    return nCurrLength;
}

int	CAnalJedi::AssembleError(char* pData)
{
    int nCurrLength = 0;
    
    nCurrLength += sprintf(pData, ",\"error\":{\"code\":\"%d\",\"message\":\"%s\"", 0, UNKNOWN);
    nCurrLength += AssembleEnd(pData+nCurrLength);
    
    return nCurrLength;
}


#pragma mark Assemble specific event
int CAnalJedi::AssembleOpenEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
    //QCLOGI("[KPI][vopen]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    //Dump(m_szData, nCurrLength);
    
    return nCurrLength;
}

int CAnalJedi::AssembleCloseEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_CLOSE* pEvt = (QCANA_EVT_CLOSE*)m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset\":%lld,\"duration_stay\":%lld,\"duration_watch\":%d",
                           pEvt->llPos, qcGetUTC()-pEvt->llOpenStartTime, (int)pEvt->llWatchDuration);
    
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize = nCurrLength;
    
    //QCLOGI("[KPI][vclose]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    
    return nCurrLength;
}

int CAnalJedi::AssembleSeekEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_SEEK* pEvt = (QCANA_EVT_SEEK*)m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset_start\":%lld,\"offset_end\":%lld,\"wait\":%lld",
                           pEvt->llWhereFrom, pEvt->llWhereTo, pEvt->llEvtDuration);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
    //QCLOGI("[KPI][vseek]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssemblePauseEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset\":%lld", pEvt->llPos);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
    //QCLOGI("[KPI][vpause]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssembleResumeEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset\":%lld,\"duration\":%lld,\"wait\":%d",
                           pEvt->llPos, pEvt->llEvtDuration, 0);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    //QCLOGI("[KPI][vresume]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssembleChangeEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BA* pEvt = (QCANA_EVT_BA*)m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset\":%lld", pEvt->llPos);
    nCurrLength += AssembleResource(pStart+nCurrLength, (char*)"resource_from", pEvt->pBitrateFrom);
    nCurrLength += AssembleResource(pStart+nCurrLength, (char*)"resource_to", pEvt->pBitrateTo);
    nCurrLength += sprintf(pStart+nCurrLength, ",\"adapt\":%d,\"wait\":%d", pEvt->nBAMode, 0);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
	m_nCurrBodySize	= nCurrLength;
    //QCLOGI("[KPI][vchange]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssembleStartEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    
    nCurrLength += sprintf(pStart+nCurrLength, ",\"wait\":%lld", pEvt->llEvtDuration);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
    //QCLOGI("[KPI][vstart]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssembleLagEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    
    nCurrLength += sprintf(pStart+nCurrLength, ",\"offset\":%lld,\"wait\":%lld", pEvt->llPos, pEvt->llEvtDuration);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
    QCLOGI("[KPI][vlag]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szBody), pStart);
    return nCurrLength;
}

int	CAnalJedi::AssembleDownloadEvent()
{
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_EVT_DLD* pEvt = (QCANA_EVT_DLD*)m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += AssembleStart(pStart+nCurrLength, pEvt->llTime, GetEvtName(pEvt->nEventID));
    nCurrLength += AssembleClient(pStart+nCurrLength);
    nCurrLength += AssembleResource(pStart+nCurrLength);
    nCurrLength += AssembleError(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, ",\"size\":%lld,\"duration\":%lld", pEvt->llDownloadSize, pEvt->llEvtDuration);
    nCurrLength += AssembleEnd(pStart+nCurrLength);
    
	m_nCurrBodySize	= nCurrLength;
    
    //QCLOGI("[KPI][vdownload]%d(%d), %s\n\n", nCurrLength, (int)strlen(m_szData), pStart);
    return nCurrLength;
}

char* CAnalJedi::GetTimeZone()
{
#ifndef __QC_OS_WIN32__
    time_t time_utc;
    struct tm tm_local;
    
    // Get the UTC time
    time(&time_utc);
    
    // Get the local time
    // Use localtime_r for threads safe
    localtime_r(&time_utc, &tm_local);
    
    time_t time_local;
    struct tm tm_gmt;
    
    // Change tm to time_t
    time_local = mktime(&tm_local);
    
    // Change it to GMT tm
    gmtime_r(&time_utc, &tm_gmt);
    
    int time_zone = tm_local.tm_hour - tm_gmt.tm_hour;
    if (time_zone < -12)
    {
        time_zone += 24;
    }
    else if (time_zone > 12)
    {
        time_zone -= 24;
    }
    
    sprintf(m_szTimeZone, "%s%02d00", time_zone>=0?"+":"", time_zone);
#else
    TIME_ZONE_INFORMATION tzInfo;
    GetTimeZoneInformation (&tzInfo);
    sprintf (m_szTimeZone, "+%02d00", -tzInfo.Bias / 60);
#endif
    
    return m_szTimeZone;
}





