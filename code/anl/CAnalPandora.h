/*******************************************************************************
	File:		CAnalPandora.h
 
	Contains:	Pandora analysis collection header code
 
	Written by:	Jun Lin
 
	Change History (most recent first):
	2017-04-27		Jun			Create file
 
 *******************************************************************************/

#ifndef CAnalPandora_H_
#define CAnalPandora_H_

#include "qcAna.h"

#include "CAnalBase.h"
#include "CAnalysisMng.h"

typedef enum
{
    PDR_WF_ID_DEVICES	= 0,
    PDR_WF_ID_SOURCES	= 1,
    PDR_WF_ID_EVENTS 	= 2
}PANDORA_WORKFLOW_ID;

class CAnalPandora : public CAnalBase
{
public:
	CAnalPandora(CBaseInst * pBaseInst);
    virtual ~CAnalPandora();
    
public:
    virtual int ReportEvent(QCANA_EVENT_INFO* pEvent, bool bDisconnect=true);
    virtual int onMsg (CMsgItem* pItem);
    virtual int Stop();
    
private:
    void	UpdateWorkflow(int nWorkflowID);
    void 	PrepareAuth(char* GMT, int nWorkflowID);
    char*	GetWorkflowName(int nID);
    
private:
    int		m_nAuthUpdateTime;
    CAnalDataSender* m_pSender;
};

#endif /* CAnalPandora_H_ */
