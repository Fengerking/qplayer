/*******************************************************************************
	File:		CAnalBase.h
 
	Contains:	Base analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-25		Jun			Create file
 
 *******************************************************************************/

#ifndef CBaseAnal_hpp
#define CBaseAnal_hpp

#include "CBaseObject.h"
#include "qcAna.h"
#include "UMsgMng.h"
#include "CNodeList.h"
#include "CMutexLock.h"
#include "CAnalDataSender.h"

#define UNKNOWN				"unknown"
#define ANAL_BODY_LEN 		8*1024
#define ANAL_RESPONSE_LEN 	8*1024
#define ANAL_HEADER_LEN 	512

class CAnalBase : public CBaseObject
{
public:
	CAnalBase(CBaseInst * pBaseInst);
    virtual ~CAnalBase();
    
public:
    virtual int ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect=true);
    virtual int ReportEvents(CObjectList<QCANA_EVENT_INFO>* pEvents);
    virtual int onMsg(CMsgItem* pItem);
    virtual int onTimer();
    virtual int Stop();
    virtual int Open();
    virtual bool IsDNSParsed();
    virtual int Disconnect();
    virtual int UpdateSource(QCANA_SOURCE_INFO* pSrc);
    virtual int PostData();
    virtual int SetSender(CAnalDataSender* pSnd){m_pSender = pSnd;return QC_ERR_NONE;}
    virtual int UpdateDNSServer(const char* pszServer);
    
    
public:
    static QCANA_SOURCE_INFO* CloneResource(QC_RESOURCE_INFO* pSrc);
    static QCANA_SOURCE_INFO* CloneResource(QCANA_SOURCE_INFO* pSrc);
    static void ReleaseResource(QCANA_SOURCE_INFO** pRes);
    static void ReleaseEvent(QCANA_EVT_BASE* pEvent);
    static bool IsReportOpenStopEvt();

protected:
    virtual char*	GetEvtName(int nEvtID);
    void			Save(char* pServerURL, char* pHeader, int nHeaderLen, char* pBody, int nBodyLen){};
protected:
    void ResetData();
    void GetGMTString(char* pszGMT, int nLen);
    
protected:
    int					m_nCurrBodySize;
    char				m_szBody[ANAL_BODY_LEN];
    char				m_szResponse[ANAL_RESPONSE_LEN];
    char				m_szHeader[ANAL_HEADER_LEN];
    char 				m_szAuth[128];
    char 				m_szServer[1024];
    QCANA_EVENT_INFO*	m_pCurrEvtInfo;
    QCANA_SOURCE_INFO*  m_pCurrSource;
    CMutexLock          m_mtReport;
    bool				m_bLive;
    CAnalDataSender*	m_pSender;
};

#endif /* CBaseAnal_hpp */
