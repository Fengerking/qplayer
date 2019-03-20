#include "RtspClientWrapper.h"


// Forward function definitions:
void ContinueAfterClientCreation0(CRtspClientWrapper* client, Boolean requestStreamingOverTCP);
void ContinueAfterClientCreation1();
void ContinueAfterOPTIONS(CRtspClientWrapper* client, int resultCode, char* resultString);
void ContinueAfterDESCRIBE(CRtspClientWrapper* client, int resultCode, char* resultString);
void ContinueAfterSETUP(CRtspClientWrapper* client, int resultCode, char* resultString);
void ContinueAfterPLAY(CRtspClientWrapper* client, int resultCode, char* resultString);
void ContinueAfterTEARDOWN(CRtspClientWrapper* client, int resultCode, char* resultString);

void   ContinueAfterOPTIONS(RTSPClient* pClient, int iResultCode, char* pResultString)
{
	if (pResultString != NULL)
	{
		delete[] pResultString;
	}

	// Next, get a SDP description for the stream:
	//pClient->GetSDPDescription(ContinueAfterDESCRIBE);
}

void   ContinueAfterDESCRIBE(RTSPClient* pClient, int iResultCode, char* pResultString)
{
	return;
}

void   ContinueAfterSETUP(RTSPClient* pClient, int iResultCode, char* pResultString)
{
	return;
}

void   ContinueAfterPLAY(RTSPClient* pClient, int iResultCode, char* pResultString)
{
	return;
}

void   ContinueAfterTEARDOWN(RTSPClient* pClient, int iResultCode, char* pResultString)
{
	return;
}


CRtspClientWrapper::CRtspClientWrapper()
{
	Init();
}

CRtspClientWrapper::~CRtspClientWrapper()
{

}

int  CRtspClientWrapper::Init()
{
	m_pClientProtocolName = "RTSP";
	m_bAllowProxyServers = True;
	m_bSupportCodecSelection = True;
	m_bControlConnectionUsesTCP = True;
	m_pRTSPClient = NULL;
	m_pClientMedium = NULL;
	m_pEnv = NULL;
	m_pScheduler = NULL;
	m_iVerbosityLevel = 1;
	return 0;
}

void CRtspClientWrapper::UnInit()
{

}


int CRtspClientWrapper::DoPrepare()
{
	int iRet = 1;

	do 
	{
		m_pScheduler = BasicTaskScheduler::createNew();
		if (m_pScheduler == NULL)
		{
			break;
		}

		m_pEnv = BasicUsageEnvironment::createNew(*m_pScheduler);
		if (m_pEnv == NULL)
		{
			break;
		}

		iRet = 0;
	} while (0);

	return iRet;
}

int CRtspClientWrapper::ContinueAfterClientCreation()
{
	RTSPClient::responseHandler* afterFunc = NULL;
	SetUserAgentString("RtspClient_v0");



	//GetOptions(ContinueAfterOPTIONS, this);


	return 0;
}



int CRtspClientWrapper::CreateClient(char*  pMediaURL)
{
	//Set the program_name NULL, and tunelOverHTTPPortNum 0
	m_pRTSPClient = RTSPClient::createNew(*m_pEnv, pMediaURL, m_iVerbosityLevel, NULL, 0);
	return 0;
}


void CRtspClientWrapper::GetOptions(RTSPClient::responseHandler* afterFunc, void*  pArg)
{
	m_pRTSPClient->sendOptionsCommand(afterFunc, m_pAuthenticator);
}

void CRtspClientWrapper::GetSDPDescription(RTSPClient::responseHandler* afterFunc)
{
	m_pRTSPClient->sendDescribeCommand(afterFunc, m_pAuthenticator);
}

void CRtspClientWrapper::SetupSubsession(MediaSubsession* subsession, Boolean streamUsingTCP, Boolean forceMulticastOnUnspecified, RTSPClient::responseHandler* afterFunc)
{
	m_pRTSPClient->sendSetupCommand(*subsession, afterFunc, False, streamUsingTCP, forceMulticastOnUnspecified, m_pAuthenticator);
}


void  CRtspClientWrapper::StartPlayingSession(MediaSession* session, double start, double end, float scale, RTSPClient::responseHandler* afterFunc) 
{
	m_pRTSPClient->sendPlayCommand(*session, afterFunc, start, end, scale, m_pAuthenticator);
}

void CRtspClientWrapper::TearDownSession(MediaSession* pSession, RTSPClient::responseHandler* afterFunc)
{
	m_pRTSPClient->sendTeardownCommand(*pSession, afterFunc, m_pAuthenticator);
}

void CRtspClientWrapper::SetUserAgentString(char const* pUserAgentString)
{
	m_pRTSPClient->setUserAgentString(pUserAgentString);
}


