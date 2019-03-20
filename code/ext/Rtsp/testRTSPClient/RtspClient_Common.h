#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "common_def.h"

// Forward function definitions:


void CallbackCurState(RTSPSourceCallBack   m_pCallback, void*  pUserInfo, int iState, void* pParams);

void FillTrackInfo(S_Media_Track_Info*  pTrackInfo, MediaSubsession*  pMediaSub);
// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")


// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

void shutdownStream(RTSPClient* rtspClient, int exitCode=0);

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient);
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession);

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient : public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);
	void SetClientInsInfo(void*  pUser);
	void StopEventLoop();
	void SetUserClientDisable();
	void SetCallback(RTSPSourceCallBack   m_pCallback);
protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;
	int m_nID;
	void*   m_pClientInsInfo;
	RTSPSourceCallBack   m_pCallback;
};

class DummySink : public MediaSink {
public:
	static DummySink* createNew(UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)
	void SetCallback(RTSPSourceCallBack   pCallback, void*  pUser);
	void SetMediaTrackInfo(S_Media_Track_Info*  pMediaInfo);
private:
	DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes,
	struct timeval presentationTime,
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds);

public:
	u_int8_t*  GetDataBuffer(){ return fReceiveBuffer; }
private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
	RTSPSourceCallBack   m_pCallback;
	S_Media_Track_Info   m_sMediaTrackInfo;
	void*                m_pUser;
};