/*******************************************************************************
 File:        CiOSPlayer.h
 
 Contains:    The iOS player wrapper implement file.
 
 Written by:    Jun Lin
 
 Change History (most recent first):
 2018-05-16        Jun Lin            Create file
 
 *******************************************************************************/
#include "CiOSPlayer.h"
#include "CVideoRndFactory.h"
#include "CBaseVideoRnd.h"
#include "CMsgMng.h"

#import <UIKit/UIKit.h>

#define GIT_LATEST_COMMIT_SHA "uknowncommit"

CiOSPlayer::CiOSPlayer(CBaseInst * pBaseInst)
:CBaseObject(pBaseInst)
,m_nStartTime(0)
,m_nIndex(0)
{
    SetObjectName("CiOSPlayer");
    if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
        m_pBaseInst->m_pMsgMng->RegNotify(this);
//    if(pBaseInst)
//        pBaseInst->AddListener(this);
    QCLOGI("%s", GIT_LATEST_COMMIT_SHA);
}

CiOSPlayer::~CiOSPlayer()
{
    QCLOG_CHECK_FUNC(NULL, m_pBaseInst, 0);
    
    if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
        m_pBaseInst->m_pMsgMng->RemNotify(this);
//    if(m_pBaseInst)
//        m_pBaseInst->RemListener(this);
}

int CiOSPlayer::Open(const char* pURL)
{
    m_nIndex        = 0;
    m_nStartTime     = qcGetSysTime();
    return QC_ERR_NONE;
}

CBaseVideoRnd* CiOSPlayer::CreateRender(void* pView, RECT* pRect)
{
    if([NSRunLoop mainRunLoop] != [NSRunLoop currentRunLoop])
    {
        QCLOGE("Create render from a non-main thread!");
        return NULL;
    }
    
    CBaseVideoRnd* pRnd = CVideoRndFactory::Create(m_pBaseInst);
    pRnd->SetView(pView, pRect);
    QC_VIDEO_FORMAT fmt;
    memset(&fmt, 0, sizeof(QC_VIDEO_FORMAT));
    fmt.nWidth = 640;
    fmt.nHeight = 320;
    pRnd->Init(&fmt);
    return pRnd;
}

void CiOSPlayer::DestroyRender(CBaseVideoRnd** pRnd)
{
    CVideoRndFactory::Destroy(pRnd);
}

int CiOSPlayer::ReceiveMsg (CMsgItem* pItem)
{
    // print log
    if(pItem->m_nMsgID==QC_MSG_SNKA_RENDER || pItem->m_nMsgID==QC_MSG_SNKV_RENDER || pItem->m_nMsgID==QC_MSG_BUFF_SEI_DATA)
        return 0;
    char    szLine[4096];
    char    szItem[4096];
    int        nStartPos = 0;
    int        tm = (pItem->m_nTime - m_nStartTime) / 1000;
    
    memset (szLine, 0, 4096);
    memset (szLine, ' ', 4095);
    
    sprintf (szItem, "QCMSG% 6d  ",    m_nIndex++);
    memcpy (szLine + nStartPos, szItem, strlen (szItem));
    nStartPos += 12;
    
    memcpy (szLine + nStartPos, pItem->m_szIDName, strlen (pItem->m_szIDName));
    nStartPos += 32;
    
    sprintf (szItem, "%02d : %02d : %02d : %03d", tm/3600, (tm%3600)/60, tm%60, (pItem->m_nTime - m_nStartTime)%1000);
    memcpy (szLine + nStartPos, szItem, strlen (szItem));
    nStartPos += 20;
    
    sprintf (szItem, "% 10d", pItem->m_nValue);
    memcpy (szLine + nStartPos, szItem, strlen (szItem));
    nStartPos += 12;
    
    sprintf (szItem, "% 12lld", pItem->m_llValue);
    memcpy (szLine + nStartPos, szItem, strlen (szItem));
    nStartPos += 16;
    
    int nLen = 0;
    if (pItem->m_szValue != NULL)
    {
        nLen = (int)strlen (pItem->m_szValue);
        if (nLen > 4095 - nStartPos)
            nLen = 4095 - nStartPos;
        memcpy (szLine + nStartPos, pItem->m_szValue, nLen);
    }
    szLine[nStartPos + nLen + 1] = 0;
    
    QCLOGI ("%s", szLine);

    return QC_ERR_NONE;
}
