#ifndef __CRTSP_SESSION_H__
#define __CRTSP_SESSION_H__

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "common_def.h"


class CRtspSession
{
public:
	CRtspSession();
	virtual ~CRtspSession();

	void SetCallback(RTSPSourceCallBack   pCallback);
	int StartRtspClient(char const* pProgName, char const* pRtspURL, int iDebugLevel);
	int StopRTSPClient();
	int SetUserForCallback(void*  pUser);
	void* GetUserData();
	int OpenURL(UsageEnvironment& env, char const* pProgName, char const* pRtspURL, int iDebugLevel, RTSPSourceCallBack   pCallback);
	void SetErrorInfo(int iErrorCode, char*  pErrorStr);
	void SetState(int iStateValue);

	RTSPClient* m_pRtspClient;
	char m_EventLoopWatchVariable;
	bool m_running;
	bool m_bClientDisable;
	cm_ThreadHandle m_hTreandHandle;
	int             m_iStatus;
	unsigned int    m_ulThreadId;
	char  m_strRtspURL[1024];
	char  m_strProgName[1024];
	int m_iDebugLevel;
	int m_nID;
	static void *RtspThreadFun(void *param);
	void RtspFunc();
	RTSPSourceCallBack   m_pCallback;
	void*       m_pUserData;
	int         m_iCurErrCode;
	char        m_strError[1024];
};

#endif