/*******************************************************************************
	File:		CTestInst.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CTestInst.h"

#include "CTestBase.h"

#ifdef __QC_OS_IOS__
#include "CTestAdp.h"
#endif


CTestInst::CTestInst(void)
{
	m_bExitTest = true;
	m_pTestMng = NULL;
	m_nItemIndex = 1000;

#ifdef __QC_OS_WIN32__
	m_hWndMain = NULL;
	m_hWndVideo = NULL;
	m_hLBItem = NULL;
	m_hLBFunc = NULL;
	m_hLBMsg = NULL;
	m_hLBErr = NULL;
#elif defined __QC_OS_NDK__
	m_pJVM = NULL;
	m_pENV = NULL;
	m_hWndVideo = NULL;

	m_pRndAudio = NULL;
	m_pRndVideo = NULL;
	m_pRndVidDec = NULL;
#elif defined __QC_OS_IOS__
    m_hWndVideo = NULL;
    m_pAdp = NULL;
#endif // __QC_OS_WIN32__
}

CTestInst::~CTestInst(void)
{
	CAutoLock lock(&m_mtInfo);
	QCTEST_InfoItem * pItem = m_lstInfo.RemoveHead();
	while (pItem != NULL)
	{
		delete[]pItem->pInfoText;
		delete pItem;
		pItem = m_lstInfo.RemoveHead();
	}
	pItem = m_lstHist.RemoveHead();
	while (pItem != NULL)
	{
		delete[]pItem->pInfoText;
		delete pItem;
		pItem = m_lstHist.RemoveHead();
	}

#ifdef __QC_OS_NDK__
	QC_DEL_P (m_pRndAudio);
	QC_DEL_P (m_pRndVideo);
	QC_DEL_P (m_pRndVidDec);
#endif // __QC_OS_NDK__	
}

int	CTestInst::AddInfoItem(void * pTask, int nType, char * pText)
{
	CAutoLock lock(&m_mtInfo);

	QCTEST_InfoItem * pItem = new QCTEST_InfoItem();
	pItem->pTask = pTask;
	pItem->nInfoType = nType;
	pItem->pInfoText = new char[strlen(pText) + 1];
	strcpy(pItem->pInfoText, pText);
	m_lstInfo.AddTail(pItem);

	return QC_ERR_NONE;
}

int	CTestInst::ShowInfoItem(void)
{
	QCTEST_InfoItem * pItem = NULL;
	m_mtInfo.Lock();
	pItem = m_lstInfo.RemoveHead();
	m_mtInfo.Unlock();
	if (pItem == NULL)
		return QC_ERR_STATUS;

	if (pItem->nInfoType == QCTEST_INFO_Item)
	{
#ifdef __QC_OS_WIN32__
		SendMessage(m_hLBItem, LB_ADDSTRING, 0, (LPARAM)pItem->pInfoText);
#else
		QCLOGT ("qcAutotest ITEM  ", "%s", pItem->pInfoText);
#endif // __QC_OS_WIN32__
	}
	else if (pItem->nInfoType == QCTEST_INFO_Func)
	{
#ifdef __QC_OS_WIN32__
		if (strcmp(pItem->pInfoText, "RESET") == 0)
			SendMessage(m_hLBFunc, LB_RESETCONTENT, 0, 0);
		else
			SendMessage(m_hLBFunc, LB_ADDSTRING, 0, (LPARAM)pItem->pInfoText);
#else
		QCLOGT ("qcAutotest FUNC  ", "%s", pItem->pInfoText);			
#endif // __QC_OS_WIN32__
	}
	else if (pItem->nInfoType == QCTEST_INFO_Msg)
	{
#ifdef __QC_OS_WIN32__
		if (strcmp(pItem->pInfoText, "RESET") == 0)
			SendMessage(m_hLBMsg, LB_RESETCONTENT, 0, 0);
		else
			SendMessage(m_hLBMsg, LB_INSERTSTRING, 0, (LPARAM)pItem->pInfoText);
#else
		QCLOGT ("qcAutotest MSG   ", "%s", pItem->pInfoText);			
#endif // __QC_OS_WIN32__
	}
	else if (pItem->nInfoType == QCTEST_INFO_Err)
	{
#ifdef __QC_OS_WIN32__
		if (strcmp(pItem->pInfoText, "RESET") == 0)
			SendMessage(m_hLBErr, LB_RESETCONTENT, 0, 0);
		else
			SendMessage(m_hLBErr, LB_INSERTSTRING, 0, (LPARAM)pItem->pInfoText);
#else
		QCLOGT ("qcAutotest ERR   ", "%s", pItem->pInfoText);			
#endif // __QC_OS_WIN32__
	}
    
#ifdef __QC_OS_IOS__
    if(m_pAdp)
        m_pAdp->ShowInfo(pItem);
#endif
    
	m_mtInfo.Lock();
//	m_lstHist.AddTail(pItem);
	delete[]pItem->pInfoText;
	delete pItem;
	m_mtInfo.Unlock();

	return QC_ERR_NONE;
}
