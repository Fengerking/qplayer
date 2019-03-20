/*******************************************************************************
	File:		CAnalJedi.h
 
	Contains:	Pandora analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-28		Jun			Create file
 
 *******************************************************************************/

#ifndef CAnalJedi_H_
#define CAnalJedi_H_

#include "qcAna.h"

#include "CAnalBase.h"
#include "CAnalysisMng.h"

class CAnalJedi : public CAnalBase
{
public:
	CAnalJedi(CBaseInst * pBaseInst);
    virtual ~CAnalJedi();
    
public:
    virtual int ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect=true);
    virtual int onMsg (CMsgItem* pItem);
    virtual int Stop();
protected:
    virtual char*	GetEvtName(int nEvtID);
    
private:
    // specific event
    int			AssembleOpenEvent();
    int			AssembleCloseEvent();
    int			AssembleSeekEvent();
    int			AssemblePauseEvent();
    int			AssembleResumeEvent();
    int			AssembleChangeEvent();
    int			AssembleStartEvent();
    int			AssembleLagEvent();
    int			AssembleDownloadEvent();
    
    // basic
    int			AssembleStart(char* pData, long long llWhen, char* szName);
    int			AssembleEnd(char* pData);
    
    int			AssembleClient(char* pData);
    int			AssembleDeviceInfo(char* pData);
    int			AssembleOSInfo(char* pData);
    int			AssembleAppInfo(char* pData);
    int			AssemblePlayerInfo(char* pData);
    
    int			AssembleResource(char* pData);
    int			AssembleResource(char* pData, char* pszRes, QCANA_SOURCE_INFO* pRes);
    int			AssembleVideoInfo(char* pData, QCANA_SOURCE_INFO* pRes);
    int			AssembleAudioInfo(char* pData, QCANA_SOURCE_INFO* pRes);
    
    int			AssembleError(char* pData);
    
    char*		GetTimeZone();
    
private:
    char	m_szTimeZone[8];
    CAnalDataSender* m_pSender;
    bool	m_bPaused;
    QCANA_SOURCE_INFO* m_pSource;
};

#endif /* CAnalJedi_H_ */
