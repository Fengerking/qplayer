#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TTPackedaudioParser.h"
#include "TTStreamQueue.h"
#include "TTBufferManager.h"

APackedAudioParser::APackedAudioParser(unsigned int AudioType)
	: mAbsoluteTimeAnchorUs(-1ll),
      mTimeOffsetValid(false),
      mTimeOffsetUs(0ll),
	  mSource(NULL),
	  mQueue(NULL),
      mNumAudioPacketsParsed(0),
	  mAudioType(AudioType)
{
}

APackedAudioParser::~APackedAudioParser() {

}

int APackedAudioParser::feedAudioPacket(char *data, unsigned int size) {
   return 0;
}

void APackedAudioParser::signalDiscontinuity(        
            int type, TTInt64 aTimeOffsetUs) {
    //int64_t mediaTimeUs;
    //if ((type & DISCONTINUITY_TIME)
    //        && extra != NULL
    //        && extra->findInt64(
    //            IStreamListener::kKeyMediaTimeUs, &mediaTimeUs)) {
    //    mAbsoluteTimeAnchorUs = mediaTimeUs;
    //} else if (type == DISCONTINUITY_ABSOLUTE_TIME) {
    //    int64_t timeUs;
    //    CHECK(extra->findInt64("timeUs", &timeUs));

    //    CHECK(mPrograms.empty());
    //    mAbsoluteTimeAnchorUs = timeUs;
    //    return;
    //} else if (type == DISCONTINUITY_TIME_OFFSET) {
    //    int64_t offset;
    //    CHECK(extra->findInt64("offset", &offset));

    //    mTimeOffsetValid = true;
    //    mTimeOffsetUs = offset;
    //    return;
    //}

    //for (size_t i = 0; i < mPrograms.size(); ++i) {
    //    mPrograms.editItemAt(i)->signalDiscontinuity(type, extra);
    //}
}

void APackedAudioParser::signalEOS(int finalResult) 
{

}

TTBufferManager* APackedAudioParser::getStreamSource() 
{
	return mSource;
}

bool APackedAudioParser::PTSTimeDeltaEstablished() 
{
	return false;
}
