/*******************************************************************************
File:		cmbasetype.h

Contains:	ts base parse header file.

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/
#ifndef __QP_TS_BASE_H__
#define __QP_TS_BASE_H__


#include "bit_t.h"

const uint32 PESPacketStartCodePrefix = 0x000001;


//Stream ID
const uint32 SID_METADATA_PES = 0x0d;
const uint32 SID_ProgramStreamMap = 0xbc;
const uint32 SID_PrivateStream1 = 0xbd;
const uint32 SID_PaddingStream = 0xbe;
const uint32 SID_PrivateStream2 = 0xbf;
const uint32 SID_AudioMin = 0xc0;
const uint32 SID_AudioMax = 0xdf;
const uint32 SID_VideoMin = 0xe0;
const uint32 SID_VideoMax = 0xef;
const uint32 SID_ECMStream = 0xf0;
const uint32 SID_EMMStream = 0xf1;
const uint32 SID_DSMCCStream = 0xf2;
const uint32 SID_13522Stream = 0xf3;
const uint32 SID_H2221A = 0xf4;
const uint32 SID_H2221B = 0xf5;
const uint32 SID_H2221C = 0xf6;
const uint32 SID_H2221D = 0xf7;
const uint32 SID_H2221E = 0xf8;
const uint32 SID_AncillaryStream = 0xf9;
const uint32 SID_ReservedDataStreamMin = 0xfa;
const uint32 SID_ReservedDataStreamMax = 0xfe;
const uint32 SID_ProgramStreamDirectory = 0xff;

//define the media type in ts
#define MEDIA_AUDIO_IN_TS     0
#define MEDIA_VIDEO_IN_TS     1
#define MEDIA_SUBTITLE_IN_TS  2


//define the media codec type in ts
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24
#define STREAM_TYPE_AUDIO_G711_A    0x8d
#define STREAM_TYPE_AUDIO_G711_U    0x8e

//define the sample flag
#define KEY_SAMPLE_FLAG       1

typedef struct 
{
	uint16   usMediaType;
	uint16   usTrackId;
	uint32   ulMediaCodecId;
	uint64   ullTimeStamp;
	uint8*   pSampleBuffer;
	uint32   ulSampleBufferSize;
	uint32   ulSampleFlag;
	void*    pUserData;
} S_Ts_Media_Sample;

typedef struct
{
	int iMediaType;
	int iCodecID;
	int iSampleRate;
	int iChannelCount;
	int iWidth;
	int iHeight;
} S_Track_Info_From_Desc;

struct RawPacket
{
public:
	uint32 index;

	bit1 transport_error_indicator;
	bit1 payload_unit_start_indicator;
	bit1 transport_priority;
	bit13 PID;
	bit2 transport_scrambling_control;
	bit2 adaptation_field_control;
	bit4 continuity_counter;

	uint8* head;
	uint8* data;
	uint32 datasize;
};

bool ParseOnePacket(RawPacket* pRawPacket, uint8* pData, uint32 cbData);
uint8* ParseAdaptationField(uint8* pData);


struct PESPacket
{
    uint8 stream_id;
    uint16 PES_packet_length;

	bit2 PES_scrambling_control;
	bit1 PES_priority;
	bit1 data_alignment_indicator;
	bit1 copyright;
	bit1 original_or_copy;
	bit2 PTS_DTS_flags;
	bit1 ESCR_flag;
	bit1 ES_rate_flag;
	bit1 DSM_trick_mode_flag;
	bit1 additional_copy_info_flag;
	bit1 PES_CRC_flag;
	bit1 PES_extension_flag;
	uint8 PES_header_data_length;

	bit33 PTS;
	bit33 DTS;
	bit33 ESCR_base;
	bit10 ESCR_extension;

	bit1 PES_private_data_flag;
	bit1 pack_header_field_flag;
	bit1 program_packet_sequence_counter_flag;
	bit1 PSTD_buffer_flag;
	bit1 PES_extension_flag_2;

	uint8* head;
	uint8* data;
	uint16 datasize; //current valid data size in TS packet
	uint16 payloadsize; //whole PES payload


	bool Load(uint8* pData, uint32 cbData);
};

#endif