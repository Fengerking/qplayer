/*******************************************************************************
File:		tsbase.cpp

Contains:	ts base parse implement

Written by:	Qichao Shen

Change History (most recent first):
2016-12-22		Qichao			Create file

*******************************************************************************/

#include "tsbase.h"
#include "string.h"

uint8* ParseAdaptationField(uint8* pData)
{
	uint8 adaptation_field_length = *pData++;
	pData += adaptation_field_length;
	return pData;
}

bool ParseOnePacket(RawPacket* pRawPacket, uint8* pData, uint32 cbData)
{
	memset(pRawPacket, 0, sizeof(RawPacket));
	pRawPacket->head = pData;
	uint8* p = pData;
	++p; //already checked sync byte


	pRawPacket->transport_error_indicator = R1B7(*p);
	pRawPacket->payload_unit_start_indicator = R1B6(*p);
	pRawPacket->transport_priority = R1B5(*p);
	R13B(p, pRawPacket->PID);

	pRawPacket->transport_scrambling_control = R2B7(*p);
	pRawPacket->adaptation_field_control = R2B5(*p);
	pRawPacket->continuity_counter = R4B3(*p);
	++p;


	if (pRawPacket->adaptation_field_control & 0x2)
	{
		p = ParseAdaptationField(p);
	}

	pRawPacket->data = p;
	uint32 cbHead = (p - pData);
	if (cbHead >= cbData)
	{
		return false;
	}
	pRawPacket->datasize = cbData - cbHead;

	if (pRawPacket->PID == 0x1fff) //ignore padding packets
    {
		return true;
	}

	return true;
}

bool PESPacket::Load(uint8* pData, uint32 cbData)
{
	uint8* p = pData;
	bit24 packet_start_code_prefix;
	R24B(p, packet_start_code_prefix);
	if (packet_start_code_prefix != PESPacketStartCodePrefix)
		return false;
	head = p;
	stream_id = *p++;
	R16B(p, PES_packet_length);

	if (stream_id == SID_ProgramStreamMap ||
		stream_id == SID_PrivateStream2 ||
		stream_id == SID_ECMStream ||
		stream_id == SID_EMMStream ||
		stream_id == SID_ProgramStreamDirectory ||
		stream_id == SID_DSMCCStream ||
		stream_id == SID_H2221E)
	{
		data = p;
		datasize = (uint16)(cbData - (uint32)(p - pData));
		if (datasize > cbData)
		{
			return false;
		}

		payloadsize = PES_packet_length;
		//set the Max Value of PTS, something wrong
		PTS =  0x1FFFFFFFFLL;
		DTS =  0x1FFFFFFFFLL;
		return true;
	}
	else if (stream_id == SID_PaddingStream)
	{
		data = p;
		datasize = (uint16)(cbData - (uint32)(p - pData));
		if (datasize > cbData)
		{
			return false;
		}

		payloadsize = 0;
		return true;
	}

	PES_scrambling_control = R2B5(*p);

	PES_priority = R1B3(*p);
	data_alignment_indicator = R1B2(*p);
	copyright = R1B1(*p);
	original_or_copy = R1B0(*p++);
	PTS_DTS_flags = R2B7(*p);
	ESCR_flag = R1B5(*p);
	ES_rate_flag = R1B4(*p);
	DSM_trick_mode_flag = R1B3(*p);
	additional_copy_info_flag = R1B2(*p);
	PES_CRC_flag = R1B1(*p);
	PES_extension_flag = R1B0(*p++);
	PES_header_data_length = *p++;
	if (PES_header_data_length > cbData)
		return false;
	uint8* pmark = p; //bookmark

	if (PTS_DTS_flags & 0x02)
	{
		R33B(p, PTS);
	}

	if (PTS_DTS_flags & 0x01)
	{	
		R33B(p, DTS);
	}
	
	if (ESCR_flag)
	{
		R33n10B(p, ESCR_base, ESCR_extension);
	}

	if (ES_rate_flag)
	{
		uint32 ES_rate;
		R22B(p, ES_rate);
	}
	if (DSM_trick_mode_flag)
	{
		p++; //TODO
	}
	if (additional_copy_info_flag)
	{
		p++; //TODO
	}
	if (PES_CRC_flag)
	{
		p += 2; //TODO
	}

	if (PES_extension_flag)
	{
		bit1 PES_private_data_flag = R1B7(*p);
		bit1 pack_header_field_flag = R1B6(*p);
		bit1 program_packet_sequence_counter_flag = R1B5(*p);
		bit1 PSTD_buffer_flag = R1B4(*p);
		bit1 PES_extension_flag_2 = R1B0(*p++);
		if (PES_private_data_flag)
		{
			p += 8; //TODO
		}
		if (pack_header_field_flag)
		{
			uint8 pack_field_length = *p++;
			p += pack_field_length; //TODO
		}
		if (program_packet_sequence_counter_flag)
		{
			p += 2; //TODO
		}
		if (PSTD_buffer_flag)
		{
			p+= 2; //TODO
		}
		if (PES_extension_flag_2)
		{
			uint8 PES_extension_field_length = R7B6(*p++);
			p += PES_extension_field_length;
		}
	}

	int stuffing_len = PES_header_data_length;
	stuffing_len -= (int)(p - pmark);
	if (stuffing_len < 0)
	{
		return false;
	}

	p += stuffing_len;

	data = p;
	datasize = (uint16)(cbData - (p - pData));
	if (datasize > cbData)
		return false;

	if (PES_packet_length > 0)
	{
		payloadsize = PES_packet_length - (uint16) (p - pData - 6);
	}
	else
	{
		payloadsize = 0;
	}
	return true;
}

