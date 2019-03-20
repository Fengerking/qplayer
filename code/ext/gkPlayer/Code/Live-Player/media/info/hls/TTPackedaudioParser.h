#ifndef _TT_PACKED_AUDIO_PARSER_H_
#define _TT_PACKED_AUDIO_PARSER_H_

#include "TTBitReader.h"
#include "GKTypedef.h"
#include "TTList.h"
#include "TTMediainfoDef.h"

class TTBufferManager;
class TTStreamQueue;

class APackedAudioParser
{
public:
    APackedAudioParser(unsigned int AudioType);
	virtual ~APackedAudioParser();

    int feedAudioPacket(char *data, unsigned int size);

    void signalDiscontinuity(
            int type, TTInt64 aTimeOffsetUs);

    void signalEOS(int finalResult);

    bool PTSTimeDeltaEstablished();

	TTBufferManager* getStreamSource();

private:
    TTInt64 mAbsoluteTimeAnchorUs;

    bool mTimeOffsetValid;
    TTInt64 mTimeOffsetUs;

	TTBufferManager* mSource;
	TTStreamQueue *mQueue;

    unsigned int mNumAudioPacketsParsed;
	unsigned int mAudioType;
};

#endif  // _TT_PACKED_AUDIO_PARSER_H_
