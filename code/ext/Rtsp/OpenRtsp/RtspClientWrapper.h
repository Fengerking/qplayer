#ifndef __RTSPCLIENTWRAPPER_H__
#define __RTSPCLIENTWRAPPER_H__

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"

class CRtspClientWrapper
{
public:
	CRtspClientWrapper();
	~CRtspClientWrapper();

	int  Init();
	void UnInit();

	void GetOptions(RTSPClient::responseHandler* afterFunc, void*  pArg);
	void GetSDPDescription(RTSPClient::responseHandler* afterFunc);
	void SetupSubsession(MediaSubsession* subsession, Boolean streamUsingTCP, Boolean forceMulticastOnUnspecified, RTSPClient::responseHandler* afterFunc);
	void StartPlayingSession(MediaSession* session, double start, double end, float scale, RTSPClient::responseHandler* afterFunc);
	void TearDownSession(MediaSession* pSession, RTSPClient::responseHandler* afterFunc);
	void SetUserAgentString(char const* pUserAgentString);

private:
	//Medium* createClient(UsageEnvironment& env, char const* url, int verbosityLevel, char const* applicationName)
	
	int DoPrepare();
	int ContinueAfterClientCreation();
	int CreateClient(char*  pMediaURL);

	//

private:
	RTSPClient* m_pRTSPClient;
	Boolean m_bAllowProxyServers;
	Boolean m_bControlConnectionUsesTCP;
	Boolean m_bSupportCodecSelection;
	
	Medium* m_pClientMedium;
	Authenticator* m_pAuthenticator;

	UsageEnvironment* m_pEnv;
	TaskScheduler* m_pScheduler;

	int        m_iVerbosityLevel;
	
	char*   m_pClientProtocolName;
};


#endif