#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TTTSParserProxy.h"
#include "TTStreamQueue.h"
#include "TTBufferManager.h"

static const size_t kTSPacketSize = 188;

class ATSParser::Program {
public:
    Program(ATSParser *parser, unsigned int programNumber, unsigned int programMapPID);
	virtual ~Program();

    bool parsePSISection(
            unsigned int pid, TTBitReader *br, int *err);

    bool parsePID(
            unsigned int pid, unsigned int continuity_counter,
            unsigned int payload_unit_start_indicator,
            TTBitReader *br, int *err);

    void signalDiscontinuity(
            int type, TTInt64 aTimeOffsetUs);

    void signalEOS(int finalResult);

	TTBufferManager* getStreamSource(int nNum);

    TTInt64 convertPTSToTimestamp(TTUint64 PTS);

	bool PTSTimeDeltaEstablished() const {
        return mFirstPTSValid;
    }

    unsigned int number() const { return mProgramNumber; }

    void updateProgramMapPID(unsigned int programMapPID) {
        mProgramMapPID = programMapPID;
    }

    unsigned int getPID() const {
        return mProgramMapPID;
    }

	unsigned int getStreamNum() const {
		return mStreams.size();
	}

private:
    ATSParser *mParser;
    unsigned int mProgramNumber;
    unsigned int mProgramMapPID;
	List<Stream *>	mStreams;
    bool mFirstPTSValid;
    TTUint64 mFirstPTS;

	Stream *getPIDStream(unsigned int streamPID);
    int parseProgramMap(TTBitReader *br);
};

class ATSParser::Stream {
public:
    Stream(Program *program,
           unsigned int elementaryPID,
           unsigned int streamType,
           unsigned int PCR_PID);
	virtual ~Stream();

    unsigned int type() const { return mStreamType; }
    unsigned int pid() const { return mElementaryPID; }
    void setPID(unsigned int pid) { mElementaryPID = pid; }

    int parse(
            unsigned int continuity_counter,
            unsigned int payload_unit_start_indicator,
            TTBitReader *br);

    void signalDiscontinuity(            
            int type, TTInt64 aTimeOffsetUs);

    void signalEOS(int finalResult);

	TTBufferManager* getSource();

	unsigned int getPID() const { return mElementaryPID;  }

private:
    Program *mProgram;
    unsigned int mElementaryPID;
    unsigned int mStreamType;
    unsigned int mPCR_PID;
    int mExpectedContinuityCounter;

	int  mDiscontinueType;

	unsigned int    mSize;       
    unsigned char*	mBuffer;  
	unsigned int    mCapacity;    
    TTBufferManager* mSource;
    bool mPayloadStarted;
	bool mIsVideo;

   TTStreamQueue *mQueue;

    int flush();
    int parsePES(TTBitReader *br);

    void onPayloadData(
            unsigned int PTS_DTS_flags, TTUint64 PTS, TTUint64 DTS,
            unsigned char *data, unsigned int size);
};

class ATSParser::PSISection{
public:
    PSISection(unsigned int nPID);

    int append(void *data, unsigned int size);
    void clear();

    bool isComplete() const;
    bool isEmpty() const;

    unsigned char *data() const;
    unsigned int size() const;

	unsigned int getPID() const {
        return mPsiPID;
    }

    virtual ~PSISection();

private:
	unsigned int mPsiPID;
    unsigned int mSize;       
    unsigned char* mBuffer;  
	unsigned int mCapacity;  
};

////////////////////////////////////////////////////////////////////////////////

ATSParser::Program::Program(
        ATSParser *parser, unsigned int programNumber, unsigned int programMapPID)
    : mParser(parser),
      mProgramNumber(programNumber),
      mProgramMapPID(programMapPID),
      mFirstPTSValid(false),
      mFirstPTS(0) {
}

ATSParser::Program::~Program() {
	List<Stream *>::iterator it = mStreams.begin();
	while (it != mStreams.end()) {
		Stream *pStream = *it;
		delete pStream;
		it = mStreams.erase(it);
    }
}

bool ATSParser::Program::parsePSISection(
        unsigned int pid, TTBitReader *br, int *err) {
    *err = 0;

    if (pid != mProgramMapPID) {
        return false;
    }

    *err = parseProgramMap(br);

    return true;
}

ATSParser::Stream* ATSParser::Program::getPIDStream(
        unsigned int streamPID) {

	List<Stream *>::iterator it = mStreams.begin();
	while (it != mStreams.end()) {
        if((*it)->pid() == streamPID)	{
			return *it;
		}
		++it;
    }

    return NULL;
}

bool ATSParser::Program::parsePID(
        unsigned int pid, unsigned int continuity_counter,
        unsigned int payload_unit_start_indicator,
        TTBitReader *br, int *err) {
    *err = 0;

    Stream *curStream = getPIDStream(pid);
    if (curStream == NULL) {
        return false;
    }

    *err = curStream->parse(continuity_counter, payload_unit_start_indicator, br);

    return true;
}

void ATSParser::Program::signalDiscontinuity(        
            int type, TTInt64 aTimeOffsetUs) {
	List<Stream *>::iterator it = mStreams.begin();
	while (it != mStreams.end()) {
		(*it)->signalDiscontinuity(type, aTimeOffsetUs);
		++it;
	}
}

void ATSParser::Program::signalEOS(int finalResult) 
{
	List<Stream *>::iterator it = mStreams.begin();
	while (it != mStreams.end()) {
		(*it)->signalEOS(finalResult);
		++it;
	} 
}

TTBufferManager* ATSParser::Program::getStreamSource(int nNum)
{
    int nIndex = 0;
	List<Stream *>::iterator it = mStreams.begin();
	while (it != mStreams.end()) {
		if(nIndex == nNum) {
			return (*it)->getSource();
		}
		++nIndex;
		++it;
	} 

    return NULL;
}

struct StreamInfo {
    unsigned int mType;
    unsigned int mPID;
};

int ATSParser::Program::parseProgramMap(TTBitReader *br) {
    unsigned int table_id = br->getBits(8);
    unsigned int section_syntax_indicator = br->getBits(1);
	br->getBits(1);
	br->getBits(2);   //reserved;
    unsigned int section_length = br->getBits(12);

    br->getBits(16);//program_number
    br->getBits(2);	//reserved
    br->getBits(5);	//version_number
    br->getBits(1);	//current_next_indicator
    br->getBits(8);	//section_number
    br->getBits(8);	//last_section_number
    br->getBits(3);	//reserved

    unsigned int PCR_PID = br->getBits(13);
	br->getBits(4);	//reserved

    unsigned int program_info_length = br->getBits(12);
    br->skipBits(program_info_length * 8);  // skip descriptors

    List<StreamInfo>	infos;
    // infoBytesRemaining is the number of bytes that make up the
    // variable length section of ES_infos. It does not include the
    // final CRC.
    unsigned int infoBytesRemaining = section_length - 9 - program_info_length - 4;

	while (infoBytesRemaining > 0) {
        unsigned int streamType = br->getBits(8);
		br->getBits(3);	//reserved

        unsigned int elementaryPID = br->getBits(13);
		br->getBits(4);	//reserved

        unsigned int ES_info_length = br->getBits(12);
        unsigned int info_bytes_remaining = ES_info_length;
        while (info_bytes_remaining >= 2) {
			br->getBits(8);	//tag 
            unsigned int descLength = br->getBits(8);
            br->skipBits(descLength * 8);
            info_bytes_remaining -= descLength + 2;
        }

        StreamInfo info;
        info.mType = streamType;
        info.mPID = elementaryPID;
        infos.push_back(info);

        infoBytesRemaining -= 5 + ES_info_length;
    }

	if(infoBytesRemaining != 0u) {
	}

	br->getBits(32); //CRC
	List<StreamInfo>::iterator it = infos.begin();
	while (it != infos.end()) {
		StreamInfo* pInfo = &(*it);
		Stream *curStream = getPIDStream(pInfo->mPID);
		if(curStream == NULL) {
			Stream *stream = new Stream(
                    this, pInfo->mPID, pInfo->mType, PCR_PID);

            mStreams.push_back(stream);
		}
		++it;
	}

    return 0;
}

TTInt64 ATSParser::Program::convertPTSToTimestamp(TTUint64 PTS) {
	if (!(mParser->mFlags & TS_TIMESTAMPS_ARE_ABSOLUTE)) {
		if (!mFirstPTSValid) {
			mFirstPTSValid = true;
			mFirstPTS = PTS;
			PTS = 0;
		} else if (PTS < mFirstPTS) {
			PTS = 0;
		} else {
			PTS -= mFirstPTS;
		}
	}

    TTInt64 timeUs = PTS / 90;

    if (mParser->mAbsoluteTimeAnchorUs >= 0ll) {
        timeUs += mParser->mAbsoluteTimeAnchorUs;
    }

    if (mParser->mTimeOffsetValid) {
        timeUs += mParser->mTimeOffsetUs;
    }

    return timeUs;
}

////////////////////////////////////////////////////////////////////////////////

ATSParser::Stream::Stream(
        Program *program,
        unsigned int elementaryPID,
        unsigned int streamType,
        unsigned int PCR_PID)
    : mProgram(program),
      mElementaryPID(elementaryPID),
      mStreamType(streamType),
      mPCR_PID(PCR_PID),
      mExpectedContinuityCounter(-1),
	  mDiscontinueType(0),
	  mSource(NULL),
      mPayloadStarted(false),
	  mSize(0),
	  mCapacity(192 * 1024)
{
	switch (mStreamType) {
		case STREAMTYPE_H264:
			mQueue = new TTStreamQueue(TTStreamQueue::H264, mElementaryPID);
			mSource = new TTBufferManager(TTBufferManager::MODE_H264, mElementaryPID);
			mIsVideo = true;
			break;
		//case STREAMTYPE_HEVC:
		//	mQueue = new TTStreamQueue(TTStreamQueue::HEVC, mElementaryPID);
		//	mSource = new TTBufferManager(TTBufferManager::MODE_HEVC, mElementaryPID);
		//	mIsVideo = true;
		//	break;
		case STREAMTYPE_MPEG2_AUDIO_ADTS:
			mQueue = new TTStreamQueue(TTStreamQueue::AAC, mElementaryPID);
			mSource = new TTBufferManager(TTBufferManager::MODE_MPEG2_AUDIO_ADTS, mElementaryPID);
			mIsVideo = false;
			break;
		case STREAMTYPE_MPEG1_AUDIO:
		case STREAMTYPE_MPEG2_AUDIO:
			mQueue = new TTStreamQueue(TTStreamQueue::MPEG_AUDIO, mElementaryPID);
			mSource = new TTBufferManager(TTBufferManager::MODE_MPEG2_AUDIO, mElementaryPID);
			mIsVideo = false;
			break;
		default:
			break;
	}

	//mSource = new TTBufferManager(mStreamType, mElementaryPID);
	mBuffer = (unsigned char *)malloc(mCapacity);
}

ATSParser::Stream::~Stream() {
	if(mBuffer) {
		free(mBuffer);
		mBuffer = NULL;
	}

	if(mQueue) {
		delete mQueue;
		mQueue = NULL;
	}

	if(mSource) {
		delete mSource;
		mSource = NULL;
	}
}

int ATSParser::Stream::parse(
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator, TTBitReader *br) {
    if (mExpectedContinuityCounter >= 0
            && (unsigned int)mExpectedContinuityCounter != continuity_counter) {

        mPayloadStarted = false;
        mSize = 0;
        mExpectedContinuityCounter = -1;

        if (!payload_unit_start_indicator) {
            return 0;
        }
    }

    mExpectedContinuityCounter = (continuity_counter + 1) & 0x0f;

    if (payload_unit_start_indicator) {
        if (mPayloadStarted) {
            // Otherwise we run the danger of receiving the trailing bytes
            // of a PES packet that we never saw the start of and assuming
            // we have a a complete PES packet.
            int err = flush();

            if (err != 0) {
                return err;
            }
        }

        mPayloadStarted = true;
    }

    if (!mPayloadStarted) {
        return 0;
    }

    unsigned int payloadSizeBytes = br->numBitsLeft()/8;
    unsigned int neededSize = mSize + payloadSizeBytes;
    if (mCapacity < neededSize) {
        neededSize = (neededSize + 65535) & ~65535;
		mCapacity = neededSize;
        unsigned char* newBuffer = (unsigned char*)malloc(mCapacity);
        memcpy(newBuffer, mBuffer, mSize);
		free(mBuffer);
        mBuffer = newBuffer;
    }

    memcpy(mBuffer + mSize, br->data(), payloadSizeBytes);
	mSize += payloadSizeBytes;
    return 0;
}

void ATSParser::Stream::signalDiscontinuity(        
            int type, TTInt64 aTimeOffsetUs) {
    mExpectedContinuityCounter = -1;
    mPayloadStarted = false;
	mSize = 0;
	
	if(type == ATSParser::DISCONTINUITY_NEW_PROGRAM_FLAG){
		mQueue->clear(true);
	} else {
		mQueue->clear();
	}

	
	mDiscontinueType = type;
}

void ATSParser::Stream::signalEOS(int finalResult) {
    if (mSource != NULL) {
        mSource->signalEOS(finalResult);
    }
}

TTBufferManager* ATSParser::Stream::getSource()
{
	return mSource;
}

int ATSParser::Stream::parsePES(TTBitReader *br) {
    unsigned int packet_startcode_prefix = br->getBits(24);

    if (packet_startcode_prefix != 1) {
        return -1;
    }

    unsigned int stream_id = br->getBits(8);
    unsigned int PES_packet_length = br->getBits(16);

    if (stream_id != 0xbc  // program_stream_map
            && stream_id != 0xbe  // padding_stream
            && stream_id != 0xbf  // private_stream_2
            && stream_id != 0xf0  // ECM
            && stream_id != 0xf1  // EMM
            && stream_id != 0xff  // program_stream_directory
            && stream_id != 0xf2  // DSMCC
            && stream_id != 0xf8) {  // H.222.1 type E
		br->getBits(2);

		br->getBits(2); //PES_scrambling_control
        br->getBits(1); //PES_priority
		br->getBits(1); //data_alignment_indicator
        br->getBits(1); //copyright
        br->getBits(1); //original_or_copy

        unsigned int PTS_DTS_flags = br->getBits(2);
        unsigned int ESCR_flag = br->getBits(1);
        unsigned int ES_rate_flag = br->getBits(1);
        unsigned int DSM_trick_mode_flag = br->getBits(1);
        unsigned int additional_copy_info_flag = br->getBits(1);

        br->getBits(1); //PES_CRC_flag
        br->getBits(1); //PES_extension_flag

        unsigned int PES_header_data_length = br->getBits(8);

        unsigned int optional_bytes_remaining = PES_header_data_length;

        TTUint64 PTS = 0, DTS = 0;

        if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {

            br->getBits(4);
            PTS = ((TTUint64)br->getBits(3)) << 30;
            br->getBits(1);
            PTS |= ((TTUint64)br->getBits(15)) << 15;
            br->getBits(1);
            PTS |= br->getBits(15);
            br->getBits(1);

			optional_bytes_remaining -= 5;

            if (PTS_DTS_flags == 3) {
                br->getBits(4);

                DTS = ((TTUint64)br->getBits(3)) << 30;
                br->getBits(1);   //1u
                DTS |= ((TTUint64)br->getBits(15)) << 15;
                br->getBits(1); //1u
                DTS |= br->getBits(15);
                br->getBits(1); //1u

                optional_bytes_remaining -= 5;
            }
        }

        if (ESCR_flag) {
            br->getBits(2);
            TTUint64 ESCR = ((TTUint64)br->getBits(3)) << 30;
            br->getBits(1);  //1u
			ESCR |= ((TTUint64)br->getBits(15)) << 15;
            br->getBits(1); //1u
            ESCR |= br->getBits(15);
            br->getBits(1); //1u

            br->getBits(9); //ESCR_extension
            br->getBits(1); //1u

            optional_bytes_remaining -= 6;
        }

        if (ES_rate_flag) {
            br->getBits(1);  //1u
            br->getBits(22); //ES_rate
            br->getBits(1);  //1u

            optional_bytes_remaining -= 3;
        }

        br->skipBits(optional_bytes_remaining * 8);

        // ES data follows.
        if (PES_packet_length != 0) {
            unsigned int dataLength =
                PES_packet_length - 3 - PES_header_data_length;

            if (br->numBitsLeft() < dataLength * 8) {
                return -1;
            }

            onPayloadData(
                    PTS_DTS_flags, PTS, DTS, br->data(), dataLength);

            br->skipBits(dataLength * 8);
        } else {
            onPayloadData(
                    PTS_DTS_flags, PTS, DTS,
                    br->data(), br->numBitsLeft() / 8);

            unsigned int payloadSizeBits = br->numBitsLeft();
        }
    } else if (stream_id == 0xbe) {  // padding_stream
        br->skipBits(PES_packet_length * 8);
    } else {
        br->skipBits(PES_packet_length * 8);
    }

    return 0;
}

int ATSParser::Stream::flush() {
    if (mSize == 0) {
        return 0;
    }
	
	TTBitReader br(mBuffer, mSize);
    int err = parsePES(&br);
    mSize = 0;
    return err;
}

void ATSParser::Stream::onPayloadData(
        unsigned int PTS_DTS_flags, TTUint64 PTS, TTUint64 /* DTS */,
        unsigned char *data, unsigned int size) {

    TTInt64 timeUs = 0; 
    if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {
        timeUs = mProgram->convertPTSToTimestamp(PTS);
    }

    int err = mQueue->appendData(data, size, timeUs, 0);
    if (err != 0) {
        return;
    }

	TTBuffer*  accessUnit = NULL;
    while ((accessUnit = mQueue->dequeueAccessUnit()) != NULL) {        
		if(mDiscontinueType == DISCONTINUITY_SEGMENT_SKIP) {
			accessUnit->nFlag |= TT_FLAG_BUFFER_FLUSH;
			mDiscontinueType = DISCONTINUITY_NONE;
		}
		mSource->queueAccessUnit(accessUnit);
    }
}

////////////////////////////////////////////////////////////////////////////////

ATSParser::ATSParser(unsigned int nFlags)
	: mFlags(nFlags),
	  mPMTParser(0),
	  mAbsoluteTimeAnchorUs(-1ll),
      mTimeOffsetValid(false),
      mTimeOffsetUs(0ll),
      mNumTSPacketsParsed(0),
      mNumPCRs(0) {
	memset(mPCRBytes, 0, sizeof(int)*2);
	mPSISections.push_back(new PSISection(0));
}

ATSParser::~ATSParser() {
	List<PSISection * >::iterator it = mPSISections.begin();
	while (it != mPSISections.end()) {
		PSISection *pSection = *it;
		delete pSection;
		it = mPSISections.erase(it);
    }

	List<Program *>::iterator itp = mPrograms.begin();
	while (itp != mPrograms.end()) {
		Program *pProgram = *itp;
		delete pProgram;
		itp = mPrograms.erase(itp);
    }
}

int ATSParser::feedTSPacket(unsigned char *data, unsigned int size) {
    TTBitReader br(data, size);
   return parseTS(&br);
}

void ATSParser::signalDiscontinuity(        
            int type, TTInt64 aTimeOffsetUs) {
	List<Program *>::iterator it = mPrograms.begin();
	while (it != mPrograms.end()) {
		Program *pProgram = *it;
		pProgram->signalDiscontinuity(type, aTimeOffsetUs);		
		++it;
    }
}

void ATSParser::signalEOS(int finalResult) {
    List<Program *>::iterator it = mPrograms.begin();
	while (it != mPrograms.end()) {
		(*it)->signalEOS(finalResult);
		++it;
	}
}

void ATSParser::parseProgramAssociationTable(TTBitReader *br) {
    unsigned int table_id = br->getBits(8);
    unsigned int section_syntax_indictor = br->getBits(1);

	br->getBits(1); //CHECK_EQ(br->getBits(1), 0u);
	br->getBits(2); //reserved

    unsigned int section_length = br->getBits(12);

    br->getBits(16); //transport_stream_id
    br->getBits(2);  //reserved 
    br->getBits(5);  //version_number
    br->getBits(1);  //current_next_indicator
    br->getBits(8);  //section_number
    br->getBits(8);  //last_section_number

    unsigned int numProgramBytes = (section_length - 5 /* header */ - 4 /* crc */);

    for (unsigned int i = 0; i < numProgramBytes / 4; ++i) {
        unsigned int program_number = br->getBits(16);

        br->getBits(3); //reserved

        if (program_number == 0) {
            br->getBits(13); //network_PID 
        } else {
            unsigned int programMapPID = br->getBits(13);

            bool found = false;
			List<Program *>::iterator itprg = mPrograms.begin();
			while (itprg != mPrograms.end()) {
				Program* program = *itprg;
				if (program->number() == program_number) {
                    program->updateProgramMapPID(programMapPID);
                    found = true;
                    break;
                }
				++itprg;
			}

            if (!found) {
                mPrograms.push_back(
                        new Program(this, program_number, programMapPID));
            }

			found = false;
			List<PSISection *>::iterator it = mPSISections.begin();
			while (it != mPSISections.end()) {
				if((*it)->getPID() == programMapPID) {
					found = true;
					break;
				}
				++it;
			}

            if (!found) {
                mPSISections.push_back(new PSISection(programMapPID));
            }
        }
    }

    br->getBits(32); 
}

int ATSParser::parsePID(
        TTBitReader *br, unsigned int PID,
        unsigned int continuity_counter,
        unsigned int payload_unit_start_indicator) {

	PSISection* section = NULL;	
	List<PSISection *>::iterator it = mPSISections.begin();
	while (it != mPSISections.end()) {
		if((*it)->getPID() == PID) {
			section = (*it);
			break;
		}
		++it;
	}

    if (section != NULL) {
        if (payload_unit_start_indicator) {
            unsigned int skip = br->getBits(8);
            br->skipBits(skip * 8);
        }

        int err = section->append(br->data(), br->numBitsLeft() / 8);
        if (err != 0) {
            return err;
        }

        if (!section->isComplete()) {
            return 0;
        }

        TTBitReader sectionBits(section->data(), section->size());

        if (PID == 0) {
            parseProgramAssociationTable(&sectionBits);
        } else {
            bool handled = false;
            List<Program *>::iterator itprg = mPrograms.begin();
			while (itprg != mPrograms.end()) {
				int err;
				Program *pProgram = *itprg;
				if(!(pProgram->parsePSISection(PID, &sectionBits, &err))) {
					++itprg;	
					continue;
				}

				if (err != 0) {
                    return err;
                }

                handled = true;
				mPMTParser = 1;
                break;				
			}

            if (!handled) {                
				mPSISections.erase(it);                
				section->clear();
				delete section;
            }
        }

        if (section != NULL) {
            section->clear();
        }

        return 0;
    }

    bool handled = false;
	List<Program *>::iterator itprg = mPrograms.begin();
	while (itprg != mPrograms.end()) {
		int err;
		Program *pProgram = *itprg;
        if (pProgram->parsePID(
                    PID, continuity_counter, payload_unit_start_indicator,
                    br, &err)) {
            if (err != 0) {
                return err;
            }

            handled = true;
            break;
        }
		++itprg;
	}

    return 0;
}

void ATSParser::parseAdaptationField(TTBitReader *br, unsigned int PID) {
    unsigned int adaptation_field_length = br->getBits(8);

    if (adaptation_field_length > 0) {
        unsigned int discontinuity_indicator = br->getBits(1);
        br->skipBits(2);
        unsigned int PCR_flag = br->getBits(1);

        unsigned int numBitsRead = 4;

        if (PCR_flag) {
            br->getBits(4);
            TTUint64 PCR_base = br->getBits(32);
            PCR_base = (PCR_base << 1) | br->getBits(1);

            br->skipBits(6);
            unsigned int PCR_ext = br->getBits(9);

            // The number of bytes from the start of the current
            // MPEG2 transport stream packet up and including
            // the final byte of this PCR_ext field.
            unsigned int byteOffsetFromStartOfTSPacket =
                (188 - br->numBitsLeft() / 8);

            TTUint64 PCR = PCR_base * 300 + PCR_ext;

            // The number of bytes received by this parser up to and
            // including the final byte of this PCR_ext field.
            unsigned int byteOffsetFromStart =
                mNumTSPacketsParsed * 188 + byteOffsetFromStartOfTSPacket;

            for (unsigned int i = 0; i < mPrograms.size(); ++i) {
                updatePCR(PID, PCR, byteOffsetFromStart);
            }

            numBitsRead += 52;
        }
		
		br->skipBits(adaptation_field_length * 8 - numBitsRead);
    }
}

int ATSParser::parseTS(TTBitReader *br) {
    unsigned int sync_byte = br->getBits(8);

	if (br->getBits(1)) {  // transport_error_indicator
        // silently ignore.
        return 0;
    }

    unsigned int payload_unit_start_indicator = br->getBits(1);
    br->getBits(1); //transport_priority
    unsigned int PID = br->getBits(13);
    br->getBits(2);  //transport_scrambling_control
    unsigned int adaptation_field_control = br->getBits(2);
    unsigned int continuity_counter = br->getBits(4);

	if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        parseAdaptationField(br, PID);
    }

    int err = 0;

    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        err = parsePID(
                br, PID, continuity_counter, payload_unit_start_indicator);
    }

    ++mNumTSPacketsParsed;

    return err;
}

int ATSParser::getProgramNum()
{
	return mPrograms.size();
}

int ATSParser::getProgramStreamNum(int nNum)
{
	List<Program *>::iterator it = mPrograms.begin();
	while (it != mPrograms.end()) {
		Program *program = *it;
		if(program->number() == nNum) {
			return program->getStreamNum();
		}
		++it;
	}

	return 0;
}

TTBufferManager* ATSParser::getStreamSource(int nProgram, int nStream) 
{
	List<Program *>::iterator it = mPrograms.begin();
	while (it != mPrograms.end()) {
		Program *program = *it;
		if(program->number() == nProgram) {
			return program->getStreamSource(nStream);
		}
		++it;
	}

	return NULL;
}

int ATSParser::isHeadReady()
{
	return mPMTParser;
}

bool ATSParser::PTSTimeDeltaEstablished() {
    if (mPrograms.empty()) {
        return false;
    }

    return (*(mPrograms.begin()))->PTSTimeDeltaEstablished();
}

void ATSParser::updatePCR(
        unsigned int PID , TTUint64 PCR, unsigned int byteOffsetFromStart) {

    if (mNumPCRs == 2) {
        mPCR[0] = mPCR[1];
        mPCRBytes[0] = mPCRBytes[1];
        mSystemTimeUs[0] = mSystemTimeUs[1];
        mNumPCRs = 1;
    }

    mPCR[mNumPCRs] = PCR;
    mPCRBytes[mNumPCRs] = byteOffsetFromStart;
    mSystemTimeUs[mNumPCRs] = 0; 

    ++mNumPCRs;

    if (mNumPCRs == 2) {
        double transportRate =
            (mPCRBytes[1] - mPCRBytes[0]) * 27E6 / (mPCR[1] - mPCR[0]);
    }
}

////////////////////////////////////////////////////////////////////////////////

ATSParser::PSISection::PSISection(unsigned int nPID) 
:mPsiPID(nPID)
,mSize(0)
,mBuffer(NULL)
,mCapacity(0){
}

ATSParser::PSISection::~PSISection() {
	if(mBuffer) {
		free(mBuffer);
		mBuffer = NULL;
	}
}

int ATSParser::PSISection::append(void *data, unsigned int size) {
    if (mBuffer == NULL || mSize + size > mCapacity) {
        unsigned int newCapacity =
            (mBuffer == NULL) ? size : mCapacity + size;

        newCapacity = (newCapacity + 1023) & ~1023;

		mCapacity = newCapacity;
		unsigned char* newBuffer = (unsigned char*)malloc(mCapacity);
        if (mBuffer != NULL) {
            memcpy(newBuffer, mBuffer, mSize);
			free(mBuffer);
        } else {
            mSize = 0;
        }

        mBuffer = newBuffer;
    }

    memcpy(mBuffer + mSize, data, size);
	mSize += size;
    return 0;
}

void ATSParser::PSISection::clear() {
       mSize = 0;
}

bool ATSParser::PSISection::isComplete() const {
    if (mBuffer == NULL || mSize < 3) {
        return false;
    }

    unsigned int sectionLength = *(mBuffer + 1);
	sectionLength = (sectionLength << 8) + *(mBuffer + 2);
	sectionLength = sectionLength & 0xfff;

    return mSize >= sectionLength + 3;
}

bool ATSParser::PSISection::isEmpty() const {
    return mBuffer == NULL || mSize == 0;
}

unsigned char *ATSParser::PSISection::data() const {
    return mBuffer;
}

unsigned int ATSParser::PSISection::size() const {
    return mSize;
}
