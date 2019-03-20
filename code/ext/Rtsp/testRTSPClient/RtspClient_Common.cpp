#include "RtspClient_Common.h"
#include "CRtspSession.h"

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP TRUE


void FillTrackInfo(S_Media_Track_Info*  pTrackInfo, MediaSubsession*  pMediaSub)
{
	char*  pMediaName = NULL;
	char*  pMediaCodec = NULL;
	char*  pMediaCodecConfig = NULL;

	if (pTrackInfo == NULL && pMediaSub == NULL)
	{
		return;
	}

	pMediaName = (char *)pMediaSub->mediumName();
	pMediaCodec = (char *)pMediaSub->codecName();
	pMediaCodecConfig = (char *)pMediaSub->fmtp_config();

	if (strcmp(pMediaName, "video") == 0)
	{
		pTrackInfo->iMediaType = MEDIA_TYPE_VIDEO;
		if (strcmp(pMediaCodec, "H264") == 0)
		{
			pTrackInfo->iCodecId = MEDIA_VIDEO_CODEC_H264;
			strcat(pTrackInfo->sVideoInfo.aCodecConfig, pMediaSub->fmtp_spropparametersets());
			pTrackInfo->sVideoInfo.iCodecConfigSize = strlen(pMediaSub->fmtp_spropparametersets());
		}
	}
	else if (strcmp(pMediaName, "audio") == 0)
	{
		pTrackInfo->iMediaType = MEDIA_TYPE_AUDIO;

		if (strcmp(pMediaCodec, "MPEG4-GENERIC") == 0)
		{
			pTrackInfo->iCodecId = MEDIA_AUDIO_CODEC_AAC;
			strcpy(pTrackInfo->sAudioInfo.aCodecConfig, pMediaCodecConfig);
			pTrackInfo->sAudioInfo.iCodecConfigSize = strlen(pMediaCodecConfig);
		}
	}
}

void CallbackCurState(RTSPSourceCallBack   pCallback, void*  pUserInfo, int iState, void* pParams)
{
	S_Media_Frame_Info  sMediaFrameInfo = { 0 };
	S_Media_Track_Info*  pTrackInfo = NULL;
	if (pCallback == NULL || pUserInfo == NULL)
	{
		return;
	}


	sMediaFrameInfo.iMediaType = MEDIA_TYPE_INFO;
	sMediaFrameInfo.iInfoData = iState;
	pCallback(0, pUserInfo, FRAME_TYPE_INFO, pParams, &sMediaFrameInfo);
}


void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {		
	char  strError[1024] = { 0 };

	do {
		ourRTSPClient*  pourRtspClient = (ourRTSPClient*)(rtspClient);
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
			sprintf(strError, "Failed to get a SDP description: %s, error code:%d\n", resultString, resultCode);
			CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_RequestFailed, strError);
			delete[] resultString;
			break;
		}

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			sprintf(strError, "Failed to create a MediaSession object from the SDP description : %s\n", env.getResultMsg());
			CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_BadArgument, strError);
			break;
		}
		else if (!scs.session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_BadArgument, "This session has no media subsessions (i.e., no \"m=\" lines)\n");
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)

		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
	ourRTSPClient*  pourRtspClient = (ourRTSPClient*)(rtspClient);
	char  strError[1024] = { 0 };


	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		sprintf(strError, "%s RTSP_STATE_SENDING_SETUP_REQ", scs.subsession->mediumName());
		CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_STATE_SENDING_SETUP_REQ, strError);
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
			if (scs.subsession->rtcpIsMuxed()) {
				env << "client port " << scs.subsession->clientPortNum();
			}
			else {
				env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
			}
			env << ")\n";

			
			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
	}
	else {
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	char  strError[1024] = { 0 };

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
		ourRTSPClient*  pourRtspClient = (ourRTSPClient*)(rtspClient);
		S_Media_Track_Info   sMediaTrackInfo = { 0 };

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
			sprintf(strError, "Failed to set up the  %s %s ,error code:%d\n", *scs.subsession->mediumName(), resultString, resultCode);
			((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->SetErrorInfo(resultCode, "This session has no media subsessions (i.e., no \"m=\" lines)\n");
			CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_SendError, strError);
			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
		if (scs.subsession->rtcpIsMuxed()) {
			env << "client port " << scs.subsession->clientPortNum();
		}
		else {
			env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
		}
		env << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
		// perhaps use your own custom "MediaSink" subclass instead
		if (scs.subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
				<< "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		FillTrackInfo(&sMediaTrackInfo, scs.subsession);
		CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_STATE_TRACK_INFO_READY, (void*)(&sMediaTrackInfo));

		((DummySink*)(scs.subsession->sink))->SetCallback(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData());
		((DummySink*)(scs.subsession->sink))->SetMediaTrackInfo(&sMediaTrackInfo);
		env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
		scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
			subsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
		}
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	Boolean success = False;
	ourRTSPClient*  pourRtspClient = (ourRTSPClient*)(rtspClient);
	char  strError[1024] = { 0 };

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
			sprintf(strError, "Failed to start playing session, error info:%s, error code:%d \n", resultString, resultCode);
			CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_RequestFailed, strError);
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (scs.duration > 0) {
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	}
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) 
{
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
	ourRTSPClient*  pourRtspClient = (ourRTSPClient*)(rtspClient);

	pourRtspClient->StopEventLoop();
	pourRtspClient->SetUserClientDisable();

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	env << *rtspClient << "Closing the stream.\n";
	CallbackCurState(pourRtspClient->m_pCallback, ((CRtspSession*)(pourRtspClient->m_pClientInsInfo))->GetUserData(), RTSP_STATE_PLAY_END, "Closing the stream.\n");
	Medium::close(rtspClient);

	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

	//if (--rtspClientCount == 0) {
	//	// The final stream has ended, so exit the application now.
	//	// (Of course, if you're embedding this code into your own application, you might want to comment this out,
	//	// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
	//	exit(exitCode);
	//}
}

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}

// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
	return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
	m_pClientInsInfo = NULL;
}

void ourRTSPClient::SetClientInsInfo(void*  pUser)
{
	m_pClientInsInfo = pUser;
}

void ourRTSPClient::StopEventLoop()
{
	CRtspSession*   pSession = (CRtspSession*)m_pClientInsInfo;
	if (pSession != NULL)
	{
		pSession->m_EventLoopWatchVariable = 1;
	}
}

void ourRTSPClient::SetUserClientDisable()
{
	CRtspSession*   pSession = (CRtspSession*)m_pClientInsInfo;
	if (pSession != NULL)
	{
		pSession->m_bClientDisable = true;
	}
}

void ourRTSPClient::SetCallback(RTSPSourceCallBack   pCallback)
{
	m_pCallback = pCallback;
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
	: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 2000000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
	return new DummySink(env, subsession, streamId);
}



void DummySink::SetCallback(RTSPSourceCallBack   pCallback, void*  pUser)
{
	m_pCallback = pCallback;
	m_pUser = pUser;
}

void DummySink::SetMediaTrackInfo(S_Media_Track_Info*  pMediaInfo)
{
	memcpy(&m_sMediaTrackInfo, pMediaInfo, sizeof(S_Media_Track_Info));
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env),
	fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
struct timeval presentationTime, unsigned durationInMicroseconds) {
	DummySink* sink = (DummySink*)clientData;
	//if (pFileDump == NULL)
	//{
	//	pFileDump = fopen("video_dump.h264", "wb");
	//}

	//if (pFileDump != NULL)
	//{
	//	fwrite(sink->GetDataBuffer(), 1, frameSize, pFileDump);
	//	fflush(pFileDump);
	//}

	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {

	S_Media_Frame_Info  sMediaFrameInfo = { 0 };

//	// We've just received a frame of data.  (Optionally) print out information about it:
//#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
//	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
//	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
//	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
//	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
//	sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
//	envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
//	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
//		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
//	}
//#ifdef DEBUG_PRINT_NPT
//	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
//#endif
//	envir() << "\n";
//#endif

	// Then continue, to request the next frame of data:

	sMediaFrameInfo.iFrameDataSize = frameSize;
	sMediaFrameInfo.illTimeStamp = presentationTime.tv_sec * 1000 + presentationTime.tv_usec / 1000;
	sMediaFrameInfo.iMediaType = m_sMediaTrackInfo.iMediaType;
	sMediaFrameInfo.iMediaCodec = m_sMediaTrackInfo.iCodecId;
	if (m_pCallback != NULL)
	{
		m_pCallback(0, m_pUser, FRAME_TYPE_MEDIA_DATA, (char*)fReceiveBuffer, &sMediaFrameInfo);
	}
	continuePlaying();
}

Boolean DummySink::continuePlaying() {
	if (fSource == NULL) return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);
	return True;
}

