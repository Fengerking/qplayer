/*******************************************************************************
	File:		CAnalPandora.cpp
 
	Contains:	CAnalPandora analysis collection implementation code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-27		Jun			Create file
 
 *******************************************************************************/
#include "CAnalPandora.h"
#include "qcErr.h"
#include "ULogFunc.h"

#ifdef __QC_OS_IOS__
#include "urlsafe_b64.h"
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
//#import <UIKit/UIKit.h>
#endif


const char* CONTENT_TYPE = "text/plain";
const char* SK = (char*)"XQJ_BFPsNOKDP_2zkx5BksmgPs-QaEsoKbVxViN9";
const char* AK = (char*)"d9c4iEdjoQsotipK0Ncy16OkCK9N0BealyAVIQ-P";


CAnalPandora::CAnalPandora(CBaseInst * pBaseInst)
	: CAnalBase(pBaseInst)
	, m_nAuthUpdateTime(0)
{
    SetObjectName("CAnalPandora");
    sprintf(m_szServer, "%s", (char*)"pipeline.qiniu.com");
	m_pSender = new CAnalDataSender(m_pBaseInst, NULL, m_szServer);
}

CAnalPandora::~CAnalPandora()
{
    QC_DEL_P(m_pSender);
}

int CAnalPandora::Stop()
{
    if(m_pSender)
        m_pSender->Stop();
    return QC_ERR_NONE;
}

char* CAnalPandora::GetWorkflowName(int nID)
{
    if(nID == PDR_WF_ID_DEVICES)
        return (char*)"devices";
    else if(nID == PDR_WF_ID_SOURCES)
        return (char*)"sources";
    else if(nID == PDR_WF_ID_EVENTS)
        return (char*)"events";
    
    return (char*)"";
}

int CAnalPandora::ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect/*=true*/)
{
#ifdef __QC_OS_IOS__
    
    ResetData();
    
    char GMT[64];
    GetGMTString(GMT, 64);
    PrepareAuth(GMT, PDR_WF_ID_EVENTS);
    
    QCANA_EVT_BASE* pEvt = pEvent->pEvtInfo;
    QCANA_DEVICE_INFO* pDevice = pEvent->pDevInfo;
    
    m_nCurrBodySize = sprintf(m_szBody,"name=%s\tposition=%lld\tevent_duration=%lld\ttime=%lld\tevent_id=%d\tsession_id=%s\terror=%d\tdevice_id=%s\tapp_id=%s\tplay_url=%s",
                       GetEvtName(pEvt->nEventID), pEvt->llPos, pEvt->llEvtDuration, pEvt->llTime, pEvt->nEventID, pEvt->szSessionID, pEvt->nErrCode, pDevice->szDeviceID, pDevice->szAppID, "");
    
    // note: it must include Date in POST header, otherwise auth will fail
    int nLen = sprintf(m_szHeader, "POST /v2/repos/%s/data HTTP/1.1\r\nHost: %s\r\nDate: %s\r\nContent-Type: %s\r\nAuthorization: %s\r\nContent-length: %d\r\n\r\n",
        GetWorkflowName(PDR_WF_ID_EVENTS), m_szServer, GMT, CONTENT_TYPE, m_szAuth, m_nCurrBodySize);
    
    if(m_pSender)
    {
        m_pSender->PostData(m_szHeader, nLen);
        m_pSender->PostData(m_szBody, m_nCurrBodySize);
        nLen = ANAL_BODY_LEN;
        m_pSender->ReadResponse(m_szBody, nLen);
    }
#endif
    
    return QC_ERR_NONE;
}

void CAnalPandora::UpdateWorkflow(int nWorkflowID)
{
#ifdef __QC_OS_IOS__
//    ResetData();
//    
//    char GMT[64];
//    GetGMTString(GMT, 64);
//    PrepareAuth(GMT, PDR_WF_ID_EVENTS);
#endif
}

int CAnalPandora::onMsg (CMsgItem* pItem)
{
    return QC_ERR_IMPLEMENT;
}

void CAnalPandora::PrepareAuth(char* GMT, int nWorkflowID)
{
#ifdef __QC_OS_IOS__
//    if((qcGetSysTime() - m_nAuthUpdateTime)<= 60*1000*1.5)
//        return;
//    m_nAuthUpdateTime = qcGetSysTime();
    
    // fromat refer to https://qiniu.github.io/pandora-docs/#/api/apiReady?id=%e5%88%b6%e4%bd%9c%e7%ad%be%e5%90%8d
    /*
    strToSign = <method> + "\n"
    + <content-md5> + "\n"
    + <content-type> + "\n"
    + <date> + "\n"
    + <canonicalizedqiniuheaders>
    + <canonicalizedresource>
    */
    char szToSign[128];
    sprintf(szToSign, "%s\n%s\n%s\n%s\n%s/v2/repos/%s/data",
            "POST",
            "",
            CONTENT_TYPE,
            GMT,
            "",
            GetWorkflowName(nWorkflowID));
    //QCLOGI("toSign, %s", szToSign);
    
    unsigned char result[CC_SHA1_DIGEST_LENGTH];
    memset(result, 0, CC_SHA1_DIGEST_LENGTH);
    CCHmac(kCCHmacAlgSHA1, SK, strlen(SK), szToSign, strlen(szToSign), result);
    
    char base64[32];
    memset(base64, 0, 32);
    urlsafe_b64_encode(result, CC_SHA1_DIGEST_LENGTH, base64, 32);
    sprintf(m_szAuth, "Pandora %s:%s", AK, base64);
    //QCLOGI("AUTH:\n%s\n", m_szAuth);
#endif
}





