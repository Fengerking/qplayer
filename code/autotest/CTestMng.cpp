/*******************************************************************************
	File:		CTestMng.cpp

	Contains:	the buffer trace r class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-01		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CTestMng.h"

#include "CFileIO.h"
#include "CHTTPIO2.h"
#include "USystemFunc.h"
#include "USourceFormat.h"
#include "ULogFunc.h"

#if defined __QC_OS_IOS__
#include "CTestAdp.h"
#endif

CTestMng::CTestMng(void)
	: CTestBase(NULL)
	, m_pPlayer(NULL)
	, m_pThreadWork(NULL)
{
	m_pInst = new CTestInst();
	m_pInst->m_pTestMng = this;

	m_nStartTime = 0;
	m_nGetPosTime = 0;
}

CTestMng::~CTestMng(void)
{
	ExitTest();

	if (m_pThreadWork != NULL)
		m_pThreadWork->Stop();
	QC_DEL_P(m_pThreadWork);

	CTestTask * pTask = m_lstTask.RemoveHead();
	while (pTask != NULL)
	{
		delete pTask;
		pTask = m_lstTask.RemoveHead();
	}

	QC_DEL_P(m_pInst);

	QCLOGT ("qcAutotest", "Finished auto test and safe exit!");
}

int CTestMng::ExitTest(void)
{
	CAutoLock lock(&m_pInst->m_mtMain);
	QC_DEL_P(m_pPlayer);
	return QC_ERR_NONE;
}

int CTestMng::StartTest(void)
{
    CAutoLock lock(&m_pInst->m_mtMain);
    
    m_pCurTask = m_lstTask.GetHead();
    if (m_pCurTask == NULL)
        return QC_ERR_FAILED;
    
    if (m_pPlayer == NULL)
        m_pPlayer = new CTestPlayer(m_pInst);
    
    if (m_pThreadWork == NULL)
    {
        m_pThreadWork = new CThreadWork(NULL);
        m_pThreadWork->SetOwner("CTestMng");
        m_pThreadWork->SetWorkProc(this, &CThreadFunc::OnWork);
    }
    m_pThreadWork->Start();
    
    PostTask(QCTEST_TASK_START, 10, 0, 0, NULL);
    
    return QC_ERR_NONE;
}

int CTestMng::AddTestFile(const char * pFile)
{
    CBaseIO *    pIO = NULL;
    CBaseInst    baseInst;
    
	if (!strncmp(pFile, "http:", 5))
		pIO = new CHTTPIO2(&baseInst);
    else
        pIO = new CFileIO(&baseInst);
    if (pIO->Open(pFile, 0, QCIO_FLAG_READ) != QC_ERR_NONE)
    {
        delete pIO;
        return QC_ERR_FAILED;
    }
    int nFileSize = (int)pIO->GetSize();
    if (nFileSize < 8)
    {
        delete pIO;
        return QC_ERR_FAILED;
    }
    char * pTxtFile = new char[nFileSize + 1];
    pIO->Read((unsigned char *)pTxtFile, nFileSize, true, QCIO_READ_DATA);
    pTxtFile[nFileSize] = 0;
    pIO->Close();
    delete pIO;
    
    char *        pTxtTask = pTxtFile;
    int            nTaskSize = 0;
    CTestTask * pTask = NULL;
    while (pTxtTask - pTxtFile < nFileSize)
    {
        pTask = new CTestTask(m_pInst);
        nTaskSize = pTask->FillTask(pTxtTask);
        if (nTaskSize <= 0)
        {
            delete pTask;
            delete[]pTxtFile;
            return QC_ERR_FAILED;
        }
        m_lstTask.AddTail(pTask);
        pTxtTask += nTaskSize;
    }
    delete[]pTxtFile;

    return QC_ERR_NONE;
}

int	CTestMng::OpenTestFile(const char * pFile)
{
    int nRC = AddTestFile(pFile);
    if (nRC != QC_ERR_NONE)
        return nRC;
    
    nRC = StartTest();

#ifdef __QC_OS_WIN32__
    SetWindowText(m_pInst->m_hWndMain, pFile);
#endif // __QC_OS_WIN32__

	return nRC;
}

int	CTestMng::PostTask(int nTaskID, int nDelay, int nValue, long long llValue, char * pName)
{
	if (m_pThreadWork == NULL)
		return QC_ERR_STATUS;

	CThreadEvent * pEvent = m_pThreadWork->GetFree();
	if (pEvent == NULL)
	{
		pEvent = new CThreadEvent(nTaskID, nValue, llValue, pName);
		pEvent->SetEventFunc(this, &CThreadFunc::OnEvent);
	}
	else
	{
		pEvent->m_nID = nTaskID;
		pEvent->m_nValue = nValue;
		pEvent->m_llValue = llValue;
		pEvent->SetName(pName);
	}

	m_pThreadWork->PostEvent(pEvent, nDelay);
	return QC_ERR_NONE;
}

int	CTestMng::ResetTask(void)
{
	if (m_pThreadWork == NULL)
		return QC_ERR_STATUS;
	m_pThreadWork->ResetEvent();
	return QC_ERR_NONE;
}

int	CTestMng::OnHandleEvent(CThreadEvent * pEvent)
{
	CAutoLock lock(&m_pInst->m_mtMain);
	if (m_pPlayer == NULL)
		return QC_ERR_STATUS;

	switch (pEvent->m_nID)
	{
	case QCTEST_TASK_START:
		if (m_pCurTask == NULL)
			return QC_ERR_STATUS;
		m_pCurTask->Start(m_pPlayer);
		break;

	case QCTEST_TASK_EXIT:
	{
		if (m_pCurTask != NULL)
			m_pCurTask->Stop();
		NODEPOS pos = m_lstTask.GetHeadPosition();
		CTestTask * pTask = NULL;
		while (pos != NULL)
		{
			pTask = m_lstTask.GetNext(pos);
			if (pTask == m_pCurTask && pos != NULL)
			{
				pTask = m_lstTask.GetNext(pos);
				m_pCurTask = pTask;
				PostTask(QCTEST_TASK_START, 10, 0, 0, NULL);
				return QC_ERR_NONE;
			}
		}
#ifdef __QC_OS_NDK__	
		QCLOGT ("qcAutotest   ", "The autotest had finished.");
#elif defined __QC_OS_IOS__
        QCLOGT ("qcAutotest   ", "The autotest had finished.");
#else	
		m_pCurTask = m_lstTask.GetHead();
		PostTask(QCTEST_TASK_START, 10, 0, 0, NULL);
#endif // __QC_OS_NDK__
		break;
	}

	case QCTEST_TASK_ITEM:
		if (m_pCurTask != NULL)
			m_pCurTask->ExcuteItem(pEvent->m_nValue);
		break;

	default:
		break;
	}
	return QC_ERR_NONE;
}

int CTestMng::OnWorkItem(void)
{
    if (m_pInst->m_bExitTest)
        return QC_ERR_NONE;
	if (m_nStartTime == 0)
		m_nStartTime = qcGetSysTime();
	if (m_nGetPosTime == 0)
		m_nGetPosTime = qcGetSysTime();
	if (qcGetSysTime() - m_nGetPosTime > 500)
	{
		m_nGetPosTime = qcGetSysTime();
#ifdef __QC_OS_WIN32__		
		if (m_pPlayer != NULL)
			m_pInst->m_pSlidePos->SetPos((int)m_pPlayer->GetPos());
#elif defined __QC_OS_IOS__
        if (m_pPlayer)
            m_pInst->m_pAdp->SetPos(m_pPlayer->GetPos(), m_pPlayer->GetDur());
#endif // __QC_OS_WIN32__			
	}
    m_pInst->ShowInfoItem();
	qcSleep(1000);

	if (m_pCurTask != NULL)
		m_pCurTask->CheckStatus();

	return QC_ERR_NONE;
}

int CTestMng::SetPlayer(CTestPlayer* pPlayer)
{
    if(!m_pPlayer)
        m_pPlayer = pPlayer;
    return QC_ERR_NONE;
}
