/*******************************************************************************
	File:		CAnalPili.cpp
 
	Contains:	CAnalPili analysis collection implementation code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-05-04		Jun			Create file
 
 *******************************************************************************/
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "CAnalPili.h"
#include "qcErr.h"
#include "ULogFunc.h"
#include "UUrlParser.h"

#if defined __QC_OS_IOS__
#import <UIKit/UIKit.h>
#import <SystemConfiguration/CaptiveNetwork.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <CoreTelephony/CTCarrier.h>


#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <sys/socket.h>


#import <sys/types.h>
#import <sys/sysctl.h>
#import <mach/mach.h>
#import <mach/host_info.h>
#import <mach/mach_host.h>
#import <mach/task_info.h>
#import <mach/task.h>
#endif

const char* PILI_CONTENT_TYPE = "application/octet-stream";
const char* PILI_REPORT_URL_PLAY = "play-pili-qos-report.qiniuapi.com";	// for live
const char* PILI_REPORT_URL_MISC = "misc-pili-qos-report.qiniuapi.com"; // for live
const char* PILI_REPORT_URL_PLAY_VOD = "play-vod-qos-report.qiniuapi.com"; // for VOD
const char* PILI_REPORT_URL_MISC_VOD = "misc-vod-qos-report.qiniuapi.com"; // for VOD

//#define _PRINTF_DETAILS_

CAnalPili::CAnalPili(CBaseInst * pBaseInst)
	: CAnalBase(pBaseInst)
	, m_nReportInterval(120)
	, m_nSampleInterval(60)
	, m_nBufferingCount(0)
	, m_nBufferingTime(0)
	, m_nCpuLoadCount(0)
	, m_fCpuLoadSys(0.0)
	, m_fCpuLoadApp(0.0)
	, m_nMemoryUsageCount(0)
	, m_fMemoryUsageSys(0.0)
	, m_fMemoryUsageApp(0.0)
	, m_bUpdateSampleTime(false)
{
    memset(m_szResolveIP, 0, 64);
    memset(&m_sRecord, 0, sizeof(m_sRecord));
    SetObjectName("CAnalPili");
    sprintf(m_szServer, "%s", PILI_REPORT_URL_MISC);

#ifdef __QC_OS_WIN32__
	m_nLastSysTime = 0;
	memset(&m_ftKernelTime, 0, sizeof(m_ftKernelTime));
	memset(&m_ftUserTime, 0, sizeof(m_ftUserTime));
#endif // __QC_OS_WIN32__
    
    CBaseInst* pInst = new CBaseInst();
    m_pSender = new CAnalDataSender(pInst, pInst->m_pDNSCache, m_szServer);
}

CAnalPili::~CAnalPili()
{
    QC_DEL_P(m_pSender);
}

char* CAnalPili::GetEvtName(int nEvtID)
{
    if(nEvtID == QCANA_EVTID_CLOSE)
        return (char*)"play_end.v5";
    else if(nEvtID == QCANA_EVTID_STARTUP)
        return (char*)"play_start.v5";
    else if(nEvtID == QCANA_EVTID_CONNECT_CHNAGED)
        return (char*)"network_change.v5";
    else if(nEvtID == QCANA_EVTID_OPEN)
        return (char*)"play_start_op.v5";
    else if(nEvtID == QCANA_EVTID_STOP)
        return (char*)"play_end_op.v5";
    else
        return (char*)"play.v5";
    
    return (char*)"";
}

int CAnalPili::Stop()
{
//    if(m_pSender)
//        m_pSender->Stop();
    return QC_ERR_NONE;
}

int CAnalPili::ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect/*=true*/)
{
    if(!pEvent || !pEvent->pDevInfo || !pEvent->pEvtInfo || !pEvent->pEvtInfo->pSrcInfo)
        return QC_ERR_EMPTYPOINTOR;
    
    CAnalBase::ReportEvent(pEvent, bDisconnect);

    m_pCurrEvtInfo = pEvent;
    
    if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_STARTUP)
    {
        EncStartupEvent();
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_LAG)
    {
        m_nBufferingTime += (int)pEvent->pEvtInfo->llEvtDuration;
        m_sRecord.nBufferingCount++;
        m_nBufferingCount++;
        return QC_ERR_NONE;
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_DOWNLOAD)
    {
        QCANA_EVT_DLD* pEvt = (QCANA_EVT_DLD*)pEvent->pEvtInfo;
        m_llDownloadBytes += pEvt->llDownloadSize;
        m_sRecord.llDownloadSize += pEvt->llDownloadSize;
        m_sRecord.nDownloadTime += pEvt->nDownloadUseTime;
        return QC_ERR_NONE;
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_CLOSE)
    {
//        if(!IsDNSParsed())
//        {
//            QCLOGW("[ANL]Ingore stop");
//            return QC_ERR_FAILED;
//        }
        EncCloseEvent();
        m_sRecord.llStartTime = 0;
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_CONNECT_CHNAGED)
    {
        EncConnectChangedEvent();
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_OPEN)
    {
        if(!IsReportOpenStopEvt())
            return QC_ERR_NONE;
        EncOpenEvent();
    }
    else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_STOP)
    {
        if(!IsReportOpenStopEvt())
            return QC_ERR_NONE;
        EncStopEvent();
    }
    else
        return QC_ERR_NONE;
    
	if (m_pSender)
    {
        int nLen = CreateHeader(true);
        int nHeadLen = nLen;
#if 0
        m_pSender->UpdateServer(GetReportURL(true));
        int nRet = m_pSender->PostData(m_szHeader, nLen);
        if(nRet != QC_ERR_NONE)
            return nRet;
        m_pSender->PostData(m_szBody, m_nCurrBodySize, false);
        
        if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_STARTUP)
        {
            if(!m_pBaseInst->m_bForceClose && !m_bUpdateSampleTime)
            {
                nLen = ANAL_RESPONSE_LEN;
                nLen = m_pSender->ReadResponse(m_szResponse, nLen);
                UpdateTrackParam(m_szResponse, nLen);
                m_bUpdateSampleTime = true;
            }
        }
        else if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_CLOSE)
        {
            QCLOGI("[ANL]Report CLOSE event complete");
        }
        if(bDisconnect)
        	m_pSender->Disconnect();
#endif
        m_pSender->Save(GetReportURL(true), m_szHeader, nHeadLen, m_szBody, m_nCurrBodySize);
        if(pEvent->pEvtInfo->nEventID == QCANA_EVTID_STARTUP)
        	m_pSender->GetReportParam(m_nSampleInterval, m_nReportInterval);
    }
    
    return QC_ERR_NONE;
}

int CAnalPili::CreateHeader(bool bPersistent)
{
    char szGMT[32];
    GetGMTString(szGMT, 32);
    int nLen = sprintf(m_szHeader, "POST /raw/log/%s-v5 HTTP/1.1\r\nHost: %s\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", bPersistent?"misc":"play", GetReportURL(bPersistent),
                       szGMT, PILI_CONTENT_TYPE, m_nCurrBodySize);

    return nLen;
}

int CAnalPili::onMsg (CMsgItem* pItem)
{
    if(pItem->m_nMsgID == QC_MSG_PLAY_OPEN_START)
    {
//        memset(&m_sRecord, 0, sizeof(m_sRecord));
//        m_sRecord.llStartTime = qcGetUTC();
//        m_sRecord.llReportTime = qcGetUTC();
//        m_nBufferingCount = 0;
//        m_nBufferingTime = 0;
//        m_llDownloadBytes = 0;
//        m_nMemoryUsageCount = 0;
//        m_fMemoryUsageApp = 0.0;
//        m_fMemoryUsageSys = 0.0;
//        m_nCpuLoadCount = 0;
//        m_fCpuLoadApp = 0.0;
//        m_fCpuLoadSys = 0.0;
//        memset(m_szResolveIP, 0, 64);
//#if defined __QC_OS_NDK__
//        m_ndkCPUInfo.GetUsedCpu();
//#endif // end of __QC_OS_NDK__
        
        return QC_ERR_NONE;
    }
    else if(pItem->m_nMsgID == QC_MSG_HTTP_DNS_GET_CACHE
            || pItem->m_nMsgID == QC_MSG_HTTP_DNS_GET_IPADDR
            || pItem->m_nMsgID == QC_MSG_RTMP_DNS_GET_IPADDR
            || pItem->m_nMsgID == QC_MSG_RTMP_DNS_GET_CACHE)
    {
        if(pItem->m_szValue)
        {
            sprintf(m_szResolveIP, "%s", pItem->m_szValue);
        }
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_VBUFFTIME)
    {
        m_sRecord.nVideoBuffSize = pItem->m_nValue;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_ABUFFTIME)
    {
        m_sRecord.nAudioBuffSize = pItem->m_nValue;
    }
    else if(pItem->m_nMsgID == QC_MSG_RENDER_VIDEO_FPS)
    {
        if(m_sRecord.fVideoRenderFPS == 0)
            m_sRecord.fVideoRenderFPS = pItem->m_nValue;
        m_sRecord.fVideoRenderFPS += pItem->m_nValue;
        m_sRecord.fVideoRenderFPS = m_sRecord.fVideoRenderFPS / 2;
    }
    else if(pItem->m_nMsgID == QC_MSG_RENDER_AUDIO_FPS)
    {
        if(m_sRecord.fAudioRenderFPS == 0)
            m_sRecord.fAudioRenderFPS = pItem->m_nValue;
        m_sRecord.fAudioRenderFPS += pItem->m_nValue;
        m_sRecord.fAudioRenderFPS = m_sRecord.fAudioRenderFPS / 2;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_VFPS)
    {
        if(m_sRecord.fVideoFPS == 0)
            m_sRecord.fVideoFPS = pItem->m_nValue;
        m_sRecord.fVideoFPS += pItem->m_nValue;
        m_sRecord.fVideoFPS /= 2;
        //QCLOGI("V FPS %d, average %f", pItem->m_nValue, m_sRecord.fVideoFPS);
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_AFPS)
    {
        if(m_sRecord.fAudioFPS == 0)
            m_sRecord.fAudioFPS = pItem->m_nValue;
        m_sRecord.fAudioFPS += pItem->m_nValue;
        m_sRecord.fAudioFPS /= 2;
        //QCLOGI("A FPS %d, average %f", pItem->m_nValue, m_sRecord.fAudioFPS);
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_VBITRATE)
    {
        if(m_sRecord.nVideoBitrte == 0)
            m_sRecord.nVideoBitrte = pItem->m_nValue;
        m_sRecord.nVideoBitrte += pItem->m_nValue;
        m_sRecord.nVideoBitrte /= 2;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_ABITRATE)
    {
        if(m_sRecord.nAudioBitrte == 0)
            m_sRecord.nAudioBitrte = pItem->m_nValue;
        m_sRecord.nAudioBitrte += pItem->m_nValue;
        m_sRecord.nAudioBitrte /= 2;
    }
    else if(pItem->m_nMsgID == QC_MSG_BUFF_START_BUFFERING)
    {
        m_sRecord.bBuffering = true;
    }
    else if (pItem->m_nMsgID == QC_MSG_BUFF_END_BUFFERING)
    {
        m_sRecord.bBuffering = false;
    }

    return QC_ERR_IMPLEMENT;
}

int CAnalPili::onTimer()
{
    //QCLOGI("[ANL]Now %lld, Last %lld, interval %lld, max %d", qcGetUTC(), m_sRecord.llStartTime, qcGetUTC()-m_sRecord.llStartTime, m_nSampleInterval*1000);
    
    if(m_sRecord.llReportTime != 0 && (qcGetUTC()-m_sRecord.llReportTime) >= m_nReportInterval*1000)
    {
        ReportPlayEvent();
        m_sRecord.llReportTime = qcGetUTC();
    }
    
    if(m_sRecord.llStartTime != 0 && (qcGetUTC()-m_sRecord.llStartTime) >= m_nSampleInterval*1000)
    {
        MeasureUsage();
        
//        m_nCpuLoadCount++;
//        m_fCpuLoadApp += GetCpuLoad();
//        //QCLOGI("[ANL]App CPU usage %f, count %d", m_fCpuLoadApp, m_nCpuLoadCount);
//        m_nMemoryUsageCount++;
//        m_fMemoryUsageApp += GetMemoryUsage(false);
//        m_fMemoryUsageSys += GetMemoryUsage(true);

        bool bStillBuffering = m_sRecord.bBuffering;
        long long llReportTime = m_sRecord.llReportTime;
        memset(&m_sRecord, 0, sizeof(m_sRecord));
        if(bStillBuffering)
            m_sRecord.nBufferingCount = 1;
        m_sRecord.bBuffering = bStillBuffering;
        m_sRecord.llStartTime = qcGetUTC();
        m_sRecord.llReportTime = llReportTime;
    }
    
    //QCLOGI("Memory usage %f, CPU load %f", GetMemoryUsage()/1024.0/1024.0, GetCpuLoad());
    return QC_ERR_NONE;
}

void CAnalPili::MeasureUsage()
{
    m_nCpuLoadCount++;
    m_fCpuLoadApp += GetCpuLoad();
    //QCLOGI("[ANL]App CPU usage %f, count %d", m_fCpuLoadApp, m_nCpuLoadCount);
    m_nMemoryUsageCount++;
    m_fMemoryUsageApp += GetMemoryUsage(false);
    m_fMemoryUsageSys += GetMemoryUsage(true);
    
//    bool bStillBuffering = m_sRecord.bBuffering;
//    long long llReportTime = m_sRecord.llReportTime;
//    memset(&m_sRecord, 0, sizeof(m_sRecord));
//    if(bStillBuffering)
//        m_sRecord.nBufferingCount = 1;
//    m_sRecord.bBuffering = bStillBuffering;
//    m_sRecord.llStartTime = qcGetUTC();
//    m_sRecord.llReportTime = llReportTime;
}

int CAnalPili::ReportPlayEvent()
{
    CAutoLock lock(&m_mtReport);
    
//    m_nCpuLoadCount++;
//    m_fCpuLoadApp += GetCpuLoad();
//    //QCLOGI("[ANL]App CPU usage %f, count %d", m_fCpuLoadApp, m_nCpuLoadCount);
//    m_nMemoryUsageCount++;
//    m_fMemoryUsageApp += GetMemoryUsage(false);
//    m_fMemoryUsageSys += GetMemoryUsage(true);
    if(!m_pCurrEvtInfo)
        return QC_ERR_STATUS;
    m_sRecord.llEndTime = qcGetUTC();

    EncPlayEvent();
    int nLen = CreateHeader(false);
    if(m_pSender)
    {
#if 0
        m_pSender->UpdateServer(GetReportURL(false));
        m_pSender->PostData(m_szHeader, nLen);
        m_pSender->PostData(m_szBody, m_nCurrBodySize, false);
        
        if(!m_pBaseInst->m_bForceClose && !m_bUpdateSampleTime)
        {
            nLen = ANAL_RESPONSE_LEN;
            nLen = m_pSender->ReadResponse(m_szResponse, nLen);
            UpdateTrackParam(m_szResponse, nLen);
            m_bUpdateSampleTime = true;
        }

        m_pSender->Disconnect();
#endif
        m_pSender->Save(GetReportURL(false), m_szHeader, nLen, m_szBody, m_nCurrBodySize);
        m_pSender->GetReportParam(m_nSampleInterval, m_nReportInterval);
    }
    
    return QC_ERR_NONE;
}


#pragma mark Basic assemble
/*
base.v5 {
    type
    timestamp
    sdk_id
    sdk_version
}
*/
int	CAnalPili::EncBase(char* pData, char* pszEvtName)
{
    if(!m_pCurrEvtInfo)
        return 0;
    int nCurrLength = 0;
 	QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    
    nCurrLength += sprintf(pData, "%s\t%lld\t%s\t%s\t", pszEvtName?pszEvtName:GetEvtName(pEvt->nEventID), pEvt->llTime, pDev->szDeviceID, pDev->szPlayerVersion);
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#] type            : %s\n[#] timestamp       : %lld\n[#] sdk_id          : %s\n[#] sdk_version     : %s",
           "------------------------------ base.v5 --------------------------------",
           pszEvtName?pszEvtName:GetEvtName(pEvt->nEventID),
           pEvt->llTime,
           pDev->szDeviceID,
           pDev->szPlayerVersion);
#endif
    
    return nCurrLength;
}

/*
media_base.v5 {
    base.v5 {}
    scheme
    domain
    path
    reqid
    remote_ip
}
 */
int	CAnalPili::EncMediaBase(char* pData, QCANA_SOURCE_INFO* pSource/*=NULL*/)
{
    if(!m_pCurrEvtInfo)
        return 0;
    int nCurrLength = 0;
    int nPort = 0;
    char szHost[1024];
    memset(szHost, 0, 1024);
    char szPath[1024*6];
    memset(szPath, 0, 1024*6);
    char szProtocal[128];
    memset(szProtocal, 0, 128);
    QCANA_SOURCE_INFO* pSrc = NULL;
    if(pSource)
        pSrc = pSource;
    else
        pSrc = m_pCurrEvtInfo->pEvtInfo->pSrcInfo;
    
    if(pSrc && pSrc->pszURL && strlen(pSrc->pszURL)>0)
    {
        qcUrlParseUrl(pSrc->pszURL, szHost, szPath, nPort, NULL);
        qcUrlParseProtocal(pSrc->pszURL, szProtocal);
        if(strlen(szProtocal) <= 0)
            strcpy(szProtocal, "local");
        if(strlen(szHost) <= 0)
            strcpy(szHost, "local");
    }
    else
    {
        strcpy(szProtocal, "-");
        strcpy(szHost, "-");
        strcpy(szPath, "-");
    }

    nCurrLength += sprintf(pData, "%s\t%s\t%s\t%s\t%s\t", szProtocal, szHost, szPath, "-", strlen(m_szResolveIP)>0?m_szResolveIP:"-");// remote server
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#] scheme          : %s\n[#] domain          : %s\n[#] path            : %s\n[#] reqid           : %s\n[#] remote_ip       : %s\n[#] %s",
           "--------------------------- media_base.v5 -----------------------------",
           szProtocal,
           szHost,
           szPath,
           "-",
           strlen(m_szResolveIP)>0?m_szResolveIP:"-",
           "-----------------------------------------------------------------------");
#endif

    return nCurrLength;
}

int	CAnalPili::EncEndBase(char* pData)
{
    if(!m_pCurrEvtInfo)
        return 0;
    int nCurrLength = 0;
    
    QCANA_EVT_CLOSE* pClose = (QCANA_EVT_CLOSE*)m_pCurrEvtInfo->pEvtInfo;
    m_fCpuLoadSys = m_fCpuLoadSys / (m_nCpuLoadCount>0?m_nCpuLoadCount:1);
    m_fCpuLoadApp = m_fCpuLoadApp / (m_nCpuLoadCount>0?m_nCpuLoadCount:1);
    m_fMemoryUsageSys = m_fMemoryUsageSys / (m_nMemoryUsageCount>0?m_nMemoryUsageCount:1);
    m_fMemoryUsageApp = m_fMemoryUsageApp / (m_nMemoryUsageCount>0?m_nMemoryUsageCount:1);
    //QCLOGI("[ANL]System memory usgae %f, app %f. System CPU usage %f, app %f", m_fMemoryUsageSys, m_fMemoryUsageApp, m_fCpuLoadSys, m_fCpuLoadApp)

    nCurrLength += EncDeviceInfoBase(pData+nCurrLength);
    nCurrLength += sprintf(pData+nCurrLength, "%0.2f\t%0.2f\t%0.2f\t%0.2f\t%s\t%s\t",
                           m_fCpuLoadSys, m_fCpuLoadApp, m_fMemoryUsageSys, m_fMemoryUsageApp, "ffmpeg-3.0", "-");
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] sys_cpu_usage   : %0.2f\n[#] app_cpu_usage   : %0.2f\n[#] sys_memory_usage: %0.2f\n[#] app_memory_usage: %0.2f\n[#] componentsVersion: %s\n[#] ui_fps          : %s",
           m_fCpuLoadSys, m_fCpuLoadApp, m_fMemoryUsageSys, m_fMemoryUsageApp, "ffmpeg-3.0", "-");
#endif

    nCurrLength += EncNetworkInfoBase(pData+nCurrLength);
    nCurrLength += sprintf(pData+nCurrLength, "%d\t%d\t%d\t%s\t%s\t%s\t",
                           pClose->nConnectTime, pClose->nConnectTime, pClose->nFirstByteTime, "-", "-", "-");
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#] tcp_connect     : %d\n[#] rtmp_connect    : %d\n[#] first_byte      : %d\n[#] ping            : %s\n[#] error_code      : %s\n[#] error_os_code   : %s",
           "------------------------- connect indicator ---------------------------",
           pClose->nConnectTime, pClose->nConnectTime, pClose->nFirstByteTime, "-", "-", "-");
#endif

    return nCurrLength;
}

int CAnalPili::EncNetworkInfoBase(char* pData)
{
    if(!m_pCurrEvtInfo)
        return 0;
    int nCurrLength = 0;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    
    char szWifiName[128];
    memset(szWifiName, 0, 128);
    GetWifiName(szWifiName);
    bool bHasWifi = (strcmp(szWifiName, "-") && strcmp(szWifiName, ""));
    
    int nSignalStrength = GetSignalStrength();
    char szSignalLevel[8];
    if(nSignalStrength > 0)
        sprintf(szSignalLevel, "%s", "-");
    else
        sprintf(szSignalLevel, "%d", GetSignalLevel());
    
    nCurrLength += sprintf(pData, "%s\t%s\t%s\t%s\t%s\t%d\t%s\t",
                           GetNetworkType(), pDev->szDeviceIP, m_szResolveIP, szWifiName, bHasWifi?"-":GetISP(),
                           nSignalStrength, szSignalLevel);
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#] network_type    : %s\n[#] local_ip        : %s\n[#] resolver_ip     : %s\n[#] wifi_name       : %s\n[#] isp_name        : %s\n[#] signal_db       : %d\n[#] signal_level    : %s",
           "--------------------------- network info ------------------------------",
           GetNetworkType(),
           pDev->szDeviceIP,
           m_szResolveIP,
           szWifiName,
           bHasWifi?"-":GetISP(),
           nSignalStrength,
           szSignalLevel);
#endif

    return nCurrLength;
}

int CAnalPili::EncDeviceInfoBase(char* pData)
{
    if(!m_pCurrEvtInfo)
        return 0;
    int nCurrLength = 0;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    
    nCurrLength += sprintf(pData, "%s\t%s\t%s\t%s\t%s\t",
                           pDev->szDeviceFamily, pDev->szOSFamily, pDev->szOSVersion, pDev->szAppID, pDev->szAppVersion);
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#] device_model    : %s\n[#] os_platform     : %s\n[#] os_version      : %s\n[#] app_name        : %s\n[#] app_version     : %s",
           "--------------------------- device info -------------------------------",
           pDev->szDeviceFamily,
           pDev->szOSFamily,
           pDev->szOSVersion,
           pDev->szAppID,
           pDev->szAppVersion);
#endif

    return nCurrLength;
}

#pragma mark Assemble specific event
int CAnalPili::EncStartupEvent()
{
    if(!m_pCurrEvtInfo)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    
    QCANA_EVT_STARTUP* pEvt = (QCANA_EVT_STARTUP*)m_pCurrEvtInfo->pEvtInfo;
    QCANA_SOURCE_INFO* pSrc = m_pCurrEvtInfo->pEvtInfo->pSrcInfo;
    
    nCurrLength += EncBase(pStart+nCurrLength);
    nCurrLength += EncMediaBase(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, "%d\t%d\t%d\t%s\t%s\t%d\t%d\n",
                           pEvt->nFirstVideoRenderTime, pEvt->nFirstAudioRenderTime, pEvt->nGopTime, GetCodecName(pSrc->nVideoCodec), GetCodecName(pSrc->nAudioCodec), pEvt->nConnectTime, pEvt->nFirstByteTime);// connect, first byte

    m_nCurrBodySize	= nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] first_video_time: %d\n[#] first_audio_time: %d\n[#] gop_time        : %d\n[#] video_decode_type: %s\n[#] audio_decode_type: %s\n[#] tcp_connect     : %d\n[#] first_byte      : %d\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]",
           pEvt->nFirstVideoRenderTime,
           pEvt->nFirstAudioRenderTime,
           pEvt->nGopTime,
           GetCodecName(pSrc->nVideoCodec),
           GetCodecName(pSrc->nAudioCodec),
           pEvt->nConnectTime,
           pEvt->nFirstByteTime,
           "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}

int CAnalPili::EncPlayEvent()
{
    if(!m_pCurrEvtInfo || !m_pCurrSource)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    
    nCurrLength += sprintf(pStart+nCurrLength, "%s\t%lld\t%s\t%s\t", "play.v5", m_sRecord.llEndTime, pDev->szDeviceID, pDev->szPlayerVersion);
    nCurrLength += EncMediaBase(pStart+nCurrLength, m_pCurrSource);
    nCurrLength += sprintf(pStart+nCurrLength, "%lld\t%lld\t%d\t%.2f\t%d\t%.2f\t%d\t%.2f\t%.2f\t%d\t%d\t%d\t%d\n",
                           m_sRecord.llStartTime, m_sRecord.llEndTime, m_sRecord.nBufferingCount>0?1:0, m_sRecord.fVideoFPS, m_sRecord.nVideoDrop,
                           m_sRecord.fAudioFPS, m_sRecord.nAudioDrop, m_sRecord.fVideoRenderFPS, m_sRecord.fAudioRenderFPS, m_sRecord.nVideoBuffSize,
                           m_sRecord.nAudioBuffSize, m_sRecord.nAudioBitrte, m_sRecord.nVideoBitrte);
    m_nCurrBodySize	= nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] begin_at        : %lld\n[#] end_at          : %lld\n[#] buffering       : %d\n[#] video_source_fps: %.2f\n[#] video_drop_frame: %d\n[#] audio_source_fps: %.2f\n[#] audio_drop_frame: %d\n[#] video_render_fps: %.2f\n[#] audio_render_fps: %.2f\n[#] video_buff_size : %d\n[#] audio_buff_size : %d\n[#] audio_bitrate   : %d\n[#] video_bitrate   : %d\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]",
           m_sRecord.llStartTime,m_sRecord.llEndTime, m_sRecord.nBufferingCount>0?1:0, m_sRecord.fVideoFPS, m_sRecord.nVideoDrop,
           m_sRecord.fAudioFPS, m_sRecord.nAudioDrop, m_sRecord.fVideoRenderFPS, m_sRecord.fAudioRenderFPS, m_sRecord.nVideoBuffSize,
           m_sRecord.nAudioBuffSize, m_sRecord.nAudioBitrte, m_sRecord.nVideoBitrte,
           "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}


int CAnalPili::EncCloseEvent()
{
    if(!m_pCurrEvtInfo)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    
    QCANA_EVT_CLOSE* pEvt = (QCANA_EVT_CLOSE*)m_pCurrEvtInfo->pEvtInfo;
    
    nCurrLength += EncBase(pStart+nCurrLength);
    nCurrLength += EncMediaBase(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, "%lld\t%lld\t%d\t%d\t%lld\t%d\t%d\t",
                           pEvt->llOpenStartTime, pEvt->llTime, m_nBufferingCount, m_nBufferingTime, m_llDownloadBytes, 0, pEvt->nGopTime);
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] begin_at        : %lld\n[#] end_at          : %lld\n[#] buffering_count : %d\n[#] buffering_time  : %d\n[#] total_recv_bytes: %lld\n[#] end_buffering_time: %d\n[#] gop_time        : %d",
           pEvt->llOpenStartTime, pEvt->llTime, m_nBufferingCount, m_nBufferingTime, m_llDownloadBytes, 0, pEvt->nGopTime);
#endif

    nCurrLength += EncEndBase(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]", "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}

int CAnalPili::EncConnectChangedEvent()
{
    if(!m_pCurrEvtInfo)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    
    nCurrLength += EncBase(pStart+nCurrLength, (char*)"network_change.v5");
    nCurrLength += EncDeviceInfoBase(pStart+nCurrLength);
    nCurrLength += EncNetworkInfoBase(pStart+nCurrLength);
    
    m_nCurrBodySize	= nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]", "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}

int CAnalPili::EncOpenEvent()
{
    if(!m_pCurrEvtInfo)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    
    //QCANA_EVT_BASE* pEvt = m_pCurrEvtInfo->pEvtInfo;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    
    nCurrLength += EncBase(pStart+nCurrLength);
    nCurrLength += EncMediaBase(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, "%s\t%s\t%s\t%s\t%s\t",
                           pDev->szDeviceFamily, pDev->szOSFamily, pDev->szOSVersion, pDev->szAppID, pDev->szAppVersion);

    m_nCurrBodySize    = nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] device_model    : %s\n[#] os_platform     : %s\n[#] os_version      : %s\n[#] app_name        : %s\n[#] app_version     : %s\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]",
           pDev->szDeviceFamily, pDev->szOSFamily, pDev->szOSVersion, pDev->szAppID, pDev->szAppVersion,
           "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}

int CAnalPili::EncStopEvent()
{
    if(!m_pCurrEvtInfo)
        return 0;
    ResetData();
    int nCurrLength = 0;
    char* pStart = m_szBody;
    
    char szWifiName[128];
    memset(szWifiName, 0, 128);
    GetWifiName(szWifiName);
    bool bHasWifi = (strcmp(szWifiName, "-") && strcmp(szWifiName, ""));

    QCANA_EVT_STOP* pEvt = (QCANA_EVT_STOP*)m_pCurrEvtInfo->pEvtInfo;
    QCANA_DEVICE_INFO* pDev = m_pCurrEvtInfo->pDevInfo;
    QCANA_SOURCE_INFO* pSrc = m_pCurrEvtInfo->pEvtInfo->pSrcInfo;
    
    nCurrLength += EncBase(pStart+nCurrLength);
    nCurrLength += EncMediaBase(pStart+nCurrLength);
    nCurrLength += sprintf(pStart+nCurrLength, "%s\t%s\t%s\t%s\t%s\t%lld\t%lld\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t",
                           pDev->szDeviceFamily, pDev->szOSFamily, pDev->szOSVersion, pDev->szAppID, pDev->szAppVersion,
                           pEvt->llWatchDuration, pSrc->llDuration, GetCodecName(pSrc->nVideoCodec), GetCodecName(pSrc->nAudioCodec), pEvt->nFirstFrameRenderTime,
                           bHasWifi?1:0, (int)m_sRecord.fVideoFPS, (int)m_sRecord.fAudioFPS, m_sRecord.nAudioBitrte, m_sRecord.nVideoBitrte,
                           pEvt->nErrCode, pEvt->nSystemErrCode);
    
    m_nCurrBodySize    = nCurrLength;
    
#ifdef _PRINTF_DETAILS_
    QCLOGI("\n[#] device_model    : %s\n[#] os_platform     : %s\n[#] os_version      : %s\n[#] app_name        : %s\n[#] app_version     : %s\n[#] duration        : %lld\n[#] total_duration  : %lld\n[#] video_enc_type  : %s\n[#] audio_enc_type  : %s\n[#] first_play      : %d\n[#] is_wifi         : %d\n[#] video_fps       : %d\n[#] audio_fps       : %d\n[#] audio_bitrate   : %d\n[#] video_bitrate   : %d\n[#] error_code      : %d\n[#] error_os_code   : %d\n[#] %s\n[#]\n[#]\n[#]\n[#]\n[#]\n[#]",
           pDev->szDeviceFamily, pDev->szOSFamily, pDev->szOSVersion, pDev->szAppID, pDev->szAppVersion,
           pEvt->llWatchDuration, pSrc->llDuration, GetCodecName(pSrc->nVideoCodec), GetCodecName(pSrc->nAudioCodec), pEvt->nFirstFrameRenderTime,
           bHasWifi?1:0, (int)m_sRecord.fVideoFPS, (int)m_sRecord.fAudioFPS, m_sRecord.nAudioBitrte, m_sRecord.nVideoBitrte,
           pEvt->nErrCode, pEvt->nSystemErrCode,
           "--------------------------- ENCODE END !!! ----------------------------");
#endif

    return nCurrLength;
}


void CAnalPili::UpdateTrackParam(char* pData, int nLen)
{
    if(nLen <= 0)
        return;
 
    char*	pBuff = const_cast<char*>(pData);
    int		nLength = nLen;
    int		nLineSize = 0;
    char*	pLine = NULL;
    
    while(GetLine (&pBuff, &nLength, &pLine, &nLineSize) == true)
    {
        //format:    {"reportInterval":120,"sampleInterval":60}
        if(pLine)
        {
            char* pStart = pLine + 2;
            if(!strncmp(pStart, "reportInterval", strlen("reportInterval")))
            {
                char szTmp[12];
                char* end = strchr(pStart, ',');
                char* start = strchr(pStart, ':');
                if(start && end)
                {
                    start++;
                    memcpy(szTmp, start, end-start);
                    szTmp[end-start] = '\0';
                    m_nReportInterval = atoi(szTmp);
                    
                    start = strchr(end, ':');
                    end = strchr(end, '}');
                    
                    if(start && end)
                    {
                        start++;
                        memcpy(szTmp, start, end-start);
                        szTmp[end-start] = '\0';
                        m_nSampleInterval = atoi(szTmp);
                    }
                    QCLOGI("[ANL]Update time, %d, %d", m_nSampleInterval, m_nReportInterval);
                }
                break;
            }
        }
    }
}

bool CAnalPili::GetLine (char ** pBuffer, int* nLen, char** pLine, int* nLineSize)
{
    char *	pBegin = *pBuffer;
    char *	pEnd = *pBuffer;
    int		nChars = 0;
    
    *pLine = NULL;
    *nLineSize = 0;
    
    if(*nLen<=0)
        return false;
    
    while(*nLen > 0 && *pBegin != 0 && (*pBegin =='\r' || *pBegin=='\n' || *pBegin==' ' || *pBegin == '\t'))
    {
        pBegin++;
        (*nLen)--;
    }
    
    pEnd = pBegin;
    
    while((*nLen) > 0 && *pEnd!=0 && *pEnd!='\r' && *pEnd!='\n')
    {
        pEnd++;
        (*nLen)--;
        nChars++;
    }
    
    *pBuffer = pEnd;
    if(*nLen > 0)
    {
        (*pBuffer)++;
        (*nLen)--;
    }
    
    if(nChars > 0)
    {
        *pEnd = 0;
        *pLine = pBegin;
        *nLineSize = nChars;
        return true;
    }
    else
    {
        return false;
    }
}

float CAnalPili::GetMemoryUsage(bool bSystem)
{
#if defined __QC_OS_IOS__
    int mib[6];
    mib[0] = CTL_HW;
    mib[1] = HW_PAGESIZE;
    int pagesize;
    size_t length;
    length = sizeof (pagesize);
    sysctl (mib, 2, &pagesize, &length, NULL, 0);

    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    host_statistics (mach_host_self (), HOST_VM_INFO, (host_info_t) &vmstat, &count);

    //float total = (vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count) * pagesize;
    float total = [NSProcessInfo processInfo].physicalMemory;
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
    
    //QCLOGI("[ANL]Memory total %f, app use %f, system use %f", total/1024./1024., info.resident_size/total, (total-vmstat.free_count*pagesize)/total);
    if(bSystem)
    	return (total-vmstat.free_count*pagesize) / total;
    else
        return info.resident_size/total;
#elif defined __QC_OS_WIN32__
	MEMORYSTATUS memStatus;
	memset(&memStatus, 0, sizeof(memStatus));
	memStatus.dwLength = sizeof(memStatus);
	GlobalMemoryStatus(&memStatus);
	return (float)(memStatus.dwTotalPhys - memStatus.dwAvailPhys) / memStatus.dwTotalPhys;
#elif defined __QC_OS_NDK__
    m_ndkMemInfo.GetTotalPhys ();
    m_ndkMemInfo.GetAvailPhys ();  
    if (m_ndkMemInfo.ullTotalPhys > 0)
    {
//        QCLOGI("[ANL]Used memory %lld, available %lld, total %lld, percent %f",
//               (m_ndkMemInfo.ullTotalPhys - m_ndkMemInfo.ullAvailPhys), m_ndkMemInfo.ullAvailPhys, m_ndkMemInfo.ullTotalPhys,
//               (float)(m_ndkMemInfo.ullTotalPhys - m_ndkMemInfo.ullAvailPhys) / (m_ndkMemInfo.ullTotalPhys));
        
        return (float)(m_ndkMemInfo.ullTotalPhys - m_ndkMemInfo.ullAvailPhys) / (m_ndkMemInfo.ullTotalPhys);
    }
#endif //__QC_OS_IOS__
    return 0.0;
}

float CAnalPili::GetCpuLoad()
{
#ifdef __QC_OS_IOS__
    kern_return_t kr;
    task_info_data_t tinfo;
    mach_msg_type_number_t task_info_count;
    
    task_info_count = TASK_INFO_MAX;
    kr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)tinfo, &task_info_count);
    if (kr != KERN_SUCCESS)
    {
        return -1;
    }
    
    task_basic_info_t      basic_info;
    thread_array_t         thread_list;
    mach_msg_type_number_t thread_count;
    
    thread_info_data_t     thinfo;
    mach_msg_type_number_t thread_info_count;
    
    thread_basic_info_t basic_info_th;
    uint32_t stat_thread = 0; // Mach threads
    
    basic_info = (task_basic_info_t)tinfo;
    
    // get threads in the task
    kr = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    if (thread_count > 0)
        stat_thread += thread_count;
    
    long tot_sec = 0;
    long tot_usec = 0;
    float tot_cpu = 0;
    
    for (size_t j = 0; j < thread_count; j++)
    {
        thread_info_count = THREAD_INFO_MAX;
        kr = thread_info(thread_list[j], THREAD_BASIC_INFO,
                         (thread_info_t)thinfo, &thread_info_count);
        if (kr != KERN_SUCCESS) {
            return -1;
        }
        
        basic_info_th = (thread_basic_info_t)thinfo;
        
        if (!(basic_info_th->flags & TH_FLAGS_IDLE)) {
            tot_sec = tot_sec + basic_info_th->user_time.seconds + basic_info_th->system_time.seconds;
            tot_usec = tot_usec + basic_info_th->system_time.microseconds + basic_info_th->system_time.microseconds;
            tot_cpu = tot_cpu + basic_info_th->cpu_usage / (float)TH_USAGE_SCALE * 100.0;
        }
        
    } // for each thread
    
    kr = vm_deallocate(mach_task_self(), (vm_offset_t)thread_list, thread_count * sizeof(thread_t));
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    
    return tot_cpu / 100.0;

#elif defined __QC_OS_WIN32__
	float	fLoad = 0.0;
	FILETIME CreationTime;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;
	BOOL	 BReturnCode = GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime);
	if (BReturnCode)
	{
		if (m_nLastSysTime != 0)
		{
			long long llOldTime = m_ftKernelTime.dwHighDateTime + m_ftUserTime.dwHighDateTime;
			llOldTime = ((llOldTime << 32) & 0XFFFFFFFF00000000) + m_ftKernelTime.dwLowDateTime + m_ftUserTime.dwLowDateTime;

			long long llNewTime = KernelTime.dwHighDateTime + UserTime.dwHighDateTime;
			llNewTime = ((llNewTime << 32) & 0XFFFFFFFF00000000) + KernelTime.dwLowDateTime + UserTime.dwLowDateTime;

			fLoad = ((llNewTime - llOldTime) / 100) / (qcGetSysTime () - m_nLastSysTime);
            fLoad = fLoad / 100;
		}
		m_nLastSysTime = qcGetSysTime();
		m_ftKernelTime = KernelTime;
		m_ftUserTime = UserTime;
	}
	return fLoad / 100.0;
#elif defined __QC_OS_NDK__
    return m_ndkCPUInfo.GetUsedCpu () / 100.0;
#endif // end of __QC_OS_IOS__
    
    return 0.0;
}

const char* CAnalPili::GetNetworkType()
{
#if defined __QC_OS_IOS__
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;
    SCNetworkReachabilityRef defaultRouteReachability = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityGetFlags(defaultRouteReachability, &flags);
    CFRelease(defaultRouteReachability);
    
    if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
        return "-";
    }
    
    NSString *network = nil;
    if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
        network = @"WIFI";
    }
    
    if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
         (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
        if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
            network = @"WIFI";
        }
    }
    if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN) {
        if((flags & kSCNetworkReachabilityFlagsReachable) == kSCNetworkReachabilityFlagsReachable) {
            if ((flags & kSCNetworkReachabilityFlagsTransientConnection) == kSCNetworkReachabilityFlagsTransientConnection)
            {
                CTTelephonyNetworkInfo *telephonyInfo = [[CTTelephonyNetworkInfo alloc] init];
                NSString *radioAccessTechnology = telephonyInfo.currentRadioAccessTechnology;
                
                NSRange range = [radioAccessTechnology rangeOfString:@"CTRadioAccessTechnology"];
                if (NSNotFound != range.location) {
                    NSString *network = [radioAccessTechnology substringFromIndex:range.location + range.length];
                    return [network UTF8String];
                } else {
                    return "-";
                }

                //network = CurrentRadioAccessTechnology();
            }
        }
    }
    
    return [network UTF8String];
    /*
    UIApplication *app = [UIApplication sharedApplication];
    if ([[app valueForKeyPath:@"_statusBar"] isKindOfClass:NSClassFromString(@"UIStatusBar_Modern")])
        return "-";

    NSArray *subviews = [[[app valueForKeyPath:@"statusBar"] valueForKeyPath:@"foregroundView"] subviews];
    for (id subview in subviews) {
        if ([subview isKindOfClass:NSClassFromString(@"UIStatusBarDataNetworkItemView")]) {
            int networkType = [[subview valueForKeyPath:@"dataNetworkType"] intValue];
            switch (networkType) {
                case 0:
                    return "-";
                case 1:
                    return "2G";
                case 2:
                    return "3G";
                case 3:
                    return "4G";
                case 5:
                    return "WIFI";
                default:
                    break;
            }
        }
    }
     */
#elif defined __QC_OS_WIN32__
	return "WIFI";
#elif defined __QC_OS_NDK__
    if(m_pBaseInst)
        return m_pBaseInst->m_szNetType;
#endif // __QC_OS_IOS__
    
    return (char*)"-";
}

const char* CAnalPili::GetISP()
{
#if defined __QC_OS_IOS__
    
    CTTelephonyNetworkInfo *telephonyInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier *carrier = [telephonyInfo subscriberCellularProvider];
    if (!carrier)
    {
        [telephonyInfo release];
        return "-";
    }
    
    NSString *currentCountry = [carrier carrierName];
    if (!carrier.isoCountryCode || !currentCountry)
    {
        [telephonyInfo release];
        return "-";
    }
    
    NSString* isp = [NSString stringWithFormat:@"%@", currentCountry];
    [telephonyInfo release];
    return [isp UTF8String];

    /*
    UIApplication *app = [UIApplication sharedApplication];
    // iPhone X
    if ([[app valueForKeyPath:@"_statusBar"] isKindOfClass:NSClassFromString(@"UIStatusBar_Modern")])
        return "-";

    NSArray *subviews = [[[app valueForKeyPath:@"statusBar"] valueForKeyPath:@"foregroundView"] subviews];
    for (id subview in subviews)
    {
        if ([subview isKindOfClass:NSClassFromString(@"UIStatusBarServiceItemView")])
        {
            NSString *serviceString = [subview valueForKeyPath:@"serviceString"];
            //QCLOGI("ISP: %s", [serviceString UTF8String]);
            return [serviceString UTF8String];
        }
    }
     */
#elif defined __QC_OS_WIN32__
	return "unknown";
#elif defined __QC_OS_NDK__
    if(m_pBaseInst)
        return m_pBaseInst->m_szISPName;
#endif // __QC_OS_IOS__
    
    return (char*)"";
}

// Wifi or 3G/4G signal
int CAnalPili::GetSignalStrength()
{
#if defined __QC_OS_IOS__
/*
    UIApplication *app = [UIApplication sharedApplication];
    
    // iPhone X
    if ([[app valueForKeyPath:@"_statusBar"] isKindOfClass:NSClassFromString(@"UIStatusBar_Modern")])
        return 0;
    
    NSArray *subviews = [[[app valueForKey:@"statusBar"] valueForKey:@"foregroundView"] subviews];
    UIView *dataNetworkItemView = nil;
    
    for (UIView * subview in subviews)
    {
        if([subview isKindOfClass:[NSClassFromString(@"UIStatusBarDataNetworkItemView") class]]) {
            dataNetworkItemView = subview;
            break;
        }
    }
    
    return [[dataNetworkItemView valueForKey:@"_wifiStrengthBars"] intValue];
*/
#elif defined __QC_OS_WIN32__
    return 3;
#elif defined __QC_OS_NDK__
    if(m_pBaseInst)
        return m_pBaseInst->m_nNetSignalDB;
#endif // __QC_OS_IOS__
    
    return 0;
}

int CAnalPili::GetSignalLevel()
{
#if defined __QC_OS_IOS__
    return 0;
#elif defined __QC_OS_NDK__
    if(m_pBaseInst)
        return m_pBaseInst->m_nSignalLevel;
#endif
    return 0;
}


void CAnalPili::GetWifiName(char* pszWifiName)
{
    if(!pszWifiName)
        return;
#if defined __QC_OS_IOS__

    CFArrayRef wifiInterfaces = CNCopySupportedInterfaces();
    
    if (!wifiInterfaces)
        return;
    
    NSArray* interfaces = (__bridge NSArray *)wifiInterfaces;
    for (NSString* interfaceName in interfaces)
    {
        NSDictionary* networkInfo = (__bridge_transfer NSDictionary *)CNCopyCurrentNetworkInfo((__bridge CFStringRef)(interfaceName));
        
        if (networkInfo)
        {
            //NSLog(@"network info -> %@", networkInfo);
            NSString* wifiName = [networkInfo objectForKey:(__bridge NSString *)kCNNetworkInfoKeySSID];
            
            sprintf(pszWifiName, "%s", [wifiName UTF8String]);
            //networkInfo = nil;
            //wifiInterfaces = nil;
            [networkInfo release];
            CFRelease(wifiInterfaces);
            return;
        }
    }
    CFRelease(wifiInterfaces);
#elif defined __QC_OS_NDK__
    if(m_pBaseInst)
    {
        sprintf(pszWifiName, "%s", m_pBaseInst->m_szWifiName);
        return;
    }
#else
    sprintf(pszWifiName, "-");
    return;
#endif
    sprintf(pszWifiName, "-");
}

bool CAnalPili::IsDNSParsed()
{
    if(!m_pSender)
        return false;
    
    return m_pSender->IsDNSParsed();
}

int CAnalPili::Disconnect()
{
    if(m_pSender)
        return m_pSender->Disconnect();
    return QC_ERR_STATUS;
}

const char* CAnalPili::GetReportURL(bool bPersistent)
{
    if(bPersistent)
        return m_bLive?PILI_REPORT_URL_MISC:PILI_REPORT_URL_MISC_VOD;
    
    return m_bLive?PILI_REPORT_URL_PLAY:PILI_REPORT_URL_PLAY_VOD;
}

const char* CAnalPili::GetCodecName(int nCodec)
{
    if(QC_CODEC_ID_H264 == nCodec)
        return "h264";
    else if(QC_CODEC_ID_H265 == nCodec)
        return "h265";
    else if(QC_CODEC_ID_MPEG4 == nCodec)
        return "mpeg4";
    else if(QC_CODEC_ID_AAC == nCodec)
        return "aac";
    else if(QC_CODEC_ID_MP3 == nCodec)
        return "mp3";
    else if(QC_CODEC_ID_MP2 == nCodec)
        return "mp2";
    else if(QC_CODEC_ID_SPEEX == nCodec)
        return "speex";
    
    return "-";
}

int CAnalPili::Open()
{
    memset(&m_sRecord, 0, sizeof(m_sRecord));
    m_sRecord.llStartTime = qcGetUTC();
    m_sRecord.llReportTime = qcGetUTC();
    m_nBufferingCount = 0;
    m_nBufferingTime = 0;
    m_llDownloadBytes = 0;
    m_nMemoryUsageCount = 0;
    m_fMemoryUsageApp = 0.0;
    m_fMemoryUsageSys = 0.0;
    m_nCpuLoadCount = 0;
    m_fCpuLoadApp = 0.0;
    m_fCpuLoadSys = 0.0;
    memset(m_szResolveIP, 0, 64);
#if defined __QC_OS_NDK__
    m_ndkCPUInfo.GetUsedCpu();
#endif // end of __QC_OS_NDK__

    MeasureUsage();
    
    return QC_ERR_NONE;
}

int CAnalPili::PostData()
{
    if(m_pSender)
    	return m_pSender->PostData();
    return -1;
}

