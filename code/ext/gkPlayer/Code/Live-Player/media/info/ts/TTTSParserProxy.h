#ifndef _TT_TS_PARSER_PROXY_H_
#define _TT_TS_PARSER_PROXY_H_

#include "TTBitReader.h"
#include "GKTypedef.h"
#include "TTList.h"
#include "TTMediainfoDef.h"

class TTBufferManager;

class ATSParser
{
public:
	enum DiscontinuityType {
        DISCONTINUITY_NONE              = 0,
        DISCONTINUITY_SEGMENT_SKIP      = 1,
		DISCONTINUITY_NEW_PROGRAM_FLAG	= 2,
    };

	enum Flags {
         TS_TIMESTAMPS_ARE_ABSOLUTE = 1,
    };


    ATSParser(unsigned int nFlags = 0);
	virtual ~ATSParser();

    int feedTSPacket(unsigned char *data, unsigned int size);

    void signalDiscontinuity(
            int type, TTInt64 aTimeOffsetUs);

    void signalEOS(int finalResult);

	int	 isHeadReady();

    bool PTSTimeDeltaEstablished();

	int	 getProgramNum();
	int  getProgramStreamNum(int nProgram);
	TTBufferManager* getStreamSource(int nProgram, int nStream);

    enum {
        // From ISO/IEC 13818-1: 2000 (E), Table 2-29
        STREAMTYPE_RESERVED             = 0x00,
        STREAMTYPE_MPEG1_VIDEO          = 0x01,
        STREAMTYPE_MPEG2_VIDEO          = 0x02,
        STREAMTYPE_MPEG1_AUDIO          = 0x03,
        STREAMTYPE_MPEG2_AUDIO          = 0x04,
        STREAMTYPE_MPEG2_AUDIO_ADTS     = 0x0f,
        STREAMTYPE_MPEG4_VIDEO          = 0x10,
        STREAMTYPE_H264                 = 0x1b,

        // From ATSC A/53 Part 3:2009, 6.7.1
        STREAMTYPE_AC3                  = 0x81,

        // Stream type 0x83 is non-standard,
        // it could be LPCM or TrueHD AC3
        STREAMTYPE_LPCM_AC3             = 0x83,
    };

private:
    class Program;
    class Stream;
    class PSISection;

	unsigned int mFlags;

	int	mPMTParser;

	List<Program *>	mPrograms;
    List<PSISection *> mPSISections;
    TTInt64 mAbsoluteTimeAnchorUs;

    bool mTimeOffsetValid;
    TTInt64 mTimeOffsetUs;

	unsigned int mNumTSPacketsParsed;

    void parseProgramAssociationTable(TTBitReader *br);
    void parseProgramMap(TTBitReader *br);
    void parsePES(TTBitReader *br);

    int parsePID(
        TTBitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator);

    void parseAdaptationField(TTBitReader *br, unsigned PID);
    int parseTS(TTBitReader *br);
    void updatePCR(unsigned PID, TTUint64 PCR, unsigned int byteOffsetFromStart);

    TTUint64 mPCR[2];
    unsigned int mPCRBytes[2];
    TTInt64 mSystemTimeUs[2];
    unsigned int mNumPCRs;
};

#endif  // _TT_TS_PARSER_PROXY_H_
