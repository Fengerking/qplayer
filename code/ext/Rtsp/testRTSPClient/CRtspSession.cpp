#include "CRtspSession.h"
#include "RtspClient_Common.h"
// A function that outputs a string that identifies each subsession (for debugging output). Modify this if you wish:




CRtspSession::CRtspSession()
{
	m_pRtspClient = NULL;
	m_running = false;
	m_EventLoopWatchVariable = 0;
	memset(m_strProgName, 0, 1024);
	memset(m_strRtspURL, 0, 1024);
	m_hTreandHandle = NULL;
	m_ulThreadId = 0;
	m_iStatus = RTSP_STATE_NONE;
	m_iDebugLevel = 1;
	m_bClientDisable = false;
	m_pCallback = NULL;
	m_iCurErrCode = 0;
	memset(m_strError, 0, 1024);
}

CRtspSession::~CRtspSession()
{
}

void CRtspSession::SetCallback(RTSPSourceCallBack   pCallback)
{
	m_pCallback = pCallback;
}


int CRtspSession::StartRtspClient(char const* pProgName, char const* pRtspURL, int iDebugLevel)
{
	int iRet = 0;
	strcpy(m_strProgName, pProgName);
	strcpy(m_strRtspURL, pRtspURL);

	m_iDebugLevel = iDebugLevel;
	m_EventLoopWatchVariable = 0;

	m_hTreandHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RtspThreadFun, this, 0, (LPDWORD)&m_ulThreadId);
	if (m_hTreandHandle == NULL)
	{
		perror("pthread_create()");
		iRet = 1;
	}
	return iRet;
}
int CRtspSession::StopRTSPClient()
{
	m_EventLoopWatchVariable = 1;
	WaitForSingleObject(m_hTreandHandle, INFINITE);

	if (m_hTreandHandle != NULL)
	{
		CloseHandle(m_hTreandHandle);
		m_hTreandHandle = NULL;
	}
	return 0;
}
void *CRtspSession::RtspThreadFun(void *param)
{
	CRtspSession *pThis = (CRtspSession*)param;
	pThis->RtspFunc();
	return NULL;
}
void CRtspSession::RtspFunc()
{
	//::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	if (OpenURL(*env, m_strProgName, m_strRtspURL, m_iDebugLevel, m_pCallback) == 0)
	{
		m_iStatus = RTSP_STATE_SENDING_DESCRIBE_REQ;
		env->taskScheduler().doEventLoop(&m_EventLoopWatchVariable);

		//
		printf("quit from event loop\n");
		//

		m_running = false;
		m_EventLoopWatchVariable = 0;

		if (m_pRtspClient && m_bClientDisable == false)
		{
			shutdownStream(m_pRtspClient);
		}
		printf("shutdown Stream\n");
	}

	env->reclaim();
	env = NULL;
	delete scheduler;
	scheduler = NULL;
	m_pRtspClient = NULL;
	m_iStatus = 2;
	printf("quit all\n");
}
int CRtspSession::OpenURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel, RTSPSourceCallBack   pCallback)
{
	m_pRtspClient = ourRTSPClient::createNew(env, rtspURL, debugLevel, progName);
	if (m_pRtspClient == NULL)
	{
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return -1;
	}


	m_bClientDisable = false;
	((ourRTSPClient*)m_pRtspClient)->m_nID = m_nID;
	((ourRTSPClient*)m_pRtspClient)->SetClientInsInfo(this);
	((ourRTSPClient*)m_pRtspClient)->SetCallback(pCallback);
	m_pRtspClient->sendDescribeCommand(continueAfterDESCRIBE);
	return 0;
}

void CRtspSession::SetErrorInfo(int iErrorCode, char*  pErrorStr)
{
	memset(m_strError, 0, 1024);
	m_iCurErrCode = iErrorCode;
}


void CRtspSession::SetState(int iStateValue)
{
	m_iStatus = iStateValue;
}

int CRtspSession::SetUserForCallback(void*  pUser)
{
	m_pUserData = pUser;
	return 0;
}

void* CRtspSession::GetUserData()
{
	return m_pUserData;
}