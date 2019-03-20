#include "TTAvUtils.h"
#include "TTBitReader.h"

unsigned int parseUE(TTBitReader *br) {
    unsigned int numZeroes = 0;
    while (br->getBits(1) == 0) {
        ++numZeroes;
    }

    unsigned int x = br->getBits(numZeroes);

    return x + (1u << numZeroes) - 1;
}

int parseSE(TTBitReader *br) {
    int codeNum = parseUE(br);

    return (codeNum & 1) ? (codeNum + 1) / 2 : -(codeNum / 2);
}

static void skipScalingList(TTBitReader *br, unsigned int sizeOfScalingList) {
    unsigned int lastScale = 8;
    unsigned int nextScale = 8;
    for (unsigned int j = 0; j < sizeOfScalingList; ++j) {
        if (nextScale != 0) {
            signed int delta_scale = parseSE(br);
            nextScale = (lastScale + delta_scale + 256) % 256;
        }

        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
}

// Determine video dimensions from the sequence parameterset.
void FindAVCDimensions(
        TTBuffer* pInBuffer,
        int *width, int *height, int* numRef,
        int *sarWidth, int *sarHeight) {
	unsigned char* buffer = pInBuffer->pBuffer;
	unsigned int size = pInBuffer->nSize;
	if (buffer[2]==0 && buffer[3]==1) {
		buffer+=5;
		size -= 5;
	} else {
		buffer+=4;
		size -= 4;
	}

    TTBitReader br(buffer, size);
    unsigned int profile_idc = br.getBits(8);
    br.skipBits(16);
    parseUE(&br);  // seq_parameter_set_id

    unsigned int chroma_format_idc = 1;  // 4:2:0 chroma format

    if (profile_idc == 100 || profile_idc == 110
            || profile_idc == 122 || profile_idc == 244
            || profile_idc == 44 || profile_idc == 83 || profile_idc == 86) {
        chroma_format_idc = parseUE(&br);
        if (chroma_format_idc == 3) {
            br.skipBits(1);  // residual_colour_transform_flag
        }
        parseUE(&br);  // bit_depth_luma_minus8
        parseUE(&br);  // bit_depth_chroma_minus8
        br.skipBits(1);  // qpprime_y_zero_transform_bypass_flag

        if (br.getBits(1)) {  // seq_scaling_matrix_present_flag
            for (size_t i = 0; i < 8; ++i) {
                if (br.getBits(1)) {  // seq_scaling_list_present_flag[i]

                    // WARNING: the code below has not ever been exercised...
                    // need a real-world example.

                    if (i < 6) {
                        // ScalingList4x4[i],16,...
                        skipScalingList(&br, 16);
                    } else {
                        // ScalingList8x8[i-6],64,...
                        skipScalingList(&br, 64);
                    }
                }
            }
        }
    }

    parseUE(&br);  // log2_max_frame_num_minus4
    unsigned int pic_order_cnt_type = parseUE(&br);

    if (pic_order_cnt_type == 0) {
        parseUE(&br);  // log2_max_pic_order_cnt_lsb_minus4
    } else if (pic_order_cnt_type == 1) {
        // offset_for_non_ref_pic, offset_for_top_to_bottom_field and
        // offset_for_ref_frame are technically se(v), but since we are
        // just skipping over them the midpoint does not matter.

        br.getBits(1);  // delta_pic_order_always_zero_flag
        parseUE(&br);  // offset_for_non_ref_pic
        parseUE(&br);  // offset_for_top_to_bottom_field

        unsigned int num_ref_frames_in_pic_order_cnt_cycle = parseUE(&br);
        for (unsigned int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            parseUE(&br);  // offset_for_ref_frame
        }
    }

    unsigned int nRef = parseUE(&br);  // num_ref_frames
	if(numRef) {
		*numRef = nRef;
	}

    br.getBits(1);  // gaps_in_frame_num_value_allowed_flag

    unsigned int pic_width_in_mbs_minus1 = parseUE(&br);
    unsigned int pic_height_in_map_units_minus1 = parseUE(&br);
    unsigned int frame_mbs_only_flag = br.getBits(1);

    *width = pic_width_in_mbs_minus1 * 16 + 16;

    *height = (2 - frame_mbs_only_flag)
        * (pic_height_in_map_units_minus1 * 16 + 16);

    if (!frame_mbs_only_flag) {
        br.getBits(1);  // mb_adaptive_frame_field_flag
    }

    br.getBits(1);  // direct_8x8_inference_flag

    if (br.getBits(1)) {  // frame_cropping_flag
        unsigned int frame_crop_left_offset = parseUE(&br);
        unsigned int frame_crop_right_offset = parseUE(&br);
        unsigned int frame_crop_top_offset = parseUE(&br);
        unsigned int frame_crop_bottom_offset = parseUE(&br);

        unsigned int cropUnitX, cropUnitY;
        if (chroma_format_idc == 0  /* monochrome */) {
            cropUnitX = 1;
            cropUnitY = 2 - frame_mbs_only_flag;
        } else {
            unsigned int subWidthC = (chroma_format_idc == 3) ? 1 : 2;
            unsigned int subHeightC = (chroma_format_idc == 1) ? 2 : 1;

            cropUnitX = subWidthC;
            cropUnitY = subHeightC * (2 - frame_mbs_only_flag);
        }

        *width -=
            (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
        *height -=
            (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;
    }

    if (sarWidth != NULL) {
        *sarWidth = 0;
    }

    if (sarHeight != NULL) {
        *sarHeight = 0;
    }

    if (br.getBits(1)) {  // vui_parameters_present_flag
        unsigned int sar_width = 0, sar_height = 0;

        if (br.getBits(1)) {  // aspect_ratio_info_present_flag
            unsigned int aspect_ratio_idc = br.getBits(8);

            if (aspect_ratio_idc == 255 /* extendedSAR */) {
                sar_width = br.getBits(16);
                sar_height = br.getBits(16);
            } else if (aspect_ratio_idc > 0 && aspect_ratio_idc < 14) {
                static const int kFixedSARWidth[] = {
                    1, 12, 10, 16, 40, 24, 20, 32, 80, 18, 15, 64, 160
                };

                static const int kFixedSARHeight[] = {
                    1, 11, 11, 11, 33, 11, 11, 11, 33, 11, 11, 33, 99
                };

                sar_width = kFixedSARWidth[aspect_ratio_idc - 1];
                sar_height = kFixedSARHeight[aspect_ratio_idc - 1];
            }
        }

		if (sarWidth != NULL) {
            *sarWidth = sar_width;
        }

        if (sarHeight != NULL) {
            *sarHeight = sar_height;
        }
    }
}

static int adjustSPS(unsigned char *sps, unsigned int*spsLen) {
    unsigned char *data = sps;
    unsigned int  size = *spsLen;
    unsigned int  offset = 0;

    while (offset + 2 <= size) {
        if (data[offset] == 0x00 && data[offset+1] == 0x00 && data[offset+2] == 0x03) {
            //found 00 00 03
            if (offset + 2 == size) {//00 00 03 as suffix
                *spsLen -=1;
                return 0;
            }

            offset += 2; //point to 0x03
            memcpy(data+offset, data+(offset+1), size - offset);//cover ox03

            size -= 1;
            *spsLen -= 1;
            continue;
        }
        ++offset;
    }

    return 0;
}

static void HEVC_parse_ptl(TTBitReader &br,
                           unsigned int max_sub_layers_minus1)
{
    unsigned int i;
    unsigned char sub_layer_profile_present_flag[8];
    unsigned char sub_layer_level_present_flag[8];

    br.skipBits(2);
    br.skipBits(1);
    br.skipBits(5);
    br.skipBits(32);
    br.skipBits(48);
    br.skipBits(8);

    for (i = 0; i < max_sub_layers_minus1; i++) {
        sub_layer_profile_present_flag[i] = br.getBits(1);
        sub_layer_level_present_flag[i]   = br.getBits(1);
    }

	if (max_sub_layers_minus1 > 0) {
        for (i = max_sub_layers_minus1; i < 8; i++)
            br.getBits(2); // reserved_zero_2bits[i]
	}

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            /*
             * sub_layer_profile_space[i]                     u(2)
             * sub_layer_tier_flag[i]                         u(1)
             * sub_layer_profile_idc[i]                       u(5)
             * sub_layer_profile_compatibility_flag[i][0..31] u(32)
             * sub_layer_progressive_source_flag[i]           u(1)
             * sub_layer_interlaced_source_flag[i]            u(1)
             * sub_layer_non_packed_constraint_flag[i]        u(1)
             * sub_layer_frame_only_constraint_flag[i]        u(1)
             * sub_layer_reserved_zero_44bits[i]              u(44)
             */
            br.skipBits(32);
            br.skipBits(32);
            br.skipBits(24);
        }

        if (sub_layer_level_present_flag[i])
            br.skipBits(8);
    }
}


void FindHEVCDimensions(
        unsigned char* buffer, unsigned int size,
        int *width, int *height)
{
	unsigned int i, sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
	if (buffer[2]==0 && buffer[3]==1) {
		buffer+=6;
		size -= 6;
	} else if(buffer[1]==0 && buffer[2]==1){
		buffer+=5;
		size -= 5;
	} else {
		buffer+=2;
		size -= 2;
	}

	adjustSPS(buffer, &size);

    TTBitReader br(buffer, size);

    br.skipBits(4); // sps_video_parameter_set_id
    sps_max_sub_layers_minus1 = br.getBits(3);

    br.getBits(1); //sps_temporal_id_nesting_flag
    HEVC_parse_ptl(br, sps_max_sub_layers_minus1);

    parseUE(&br); // sps_seq_parameter_set_id

    int chroma_format_idc = parseUE(&br);
	int separate_colour_plane_flag = 0;

	if (chroma_format_idc == 3) {
        separate_colour_plane_flag = br.getBits(1); 
	}

    int pic_width_in_luma_samples = parseUE(&br); // pic_width_in_luma_samples
    int pic_height_in_luma_samples = parseUE(&br); // pic_height_in_luma_samples

    int conformance_window_flag = br.getBits(1);
	int conf_win_left_offset = 0;
    int conf_win_right_offset = 0;
    int conf_win_top_offset = 0;
    int conf_win_bottom_offset = 0;
	if (conformance_window_flag) {        // conformance_window_flag
        conf_win_left_offset = parseUE(&br); // conf_win_left_offset
        conf_win_right_offset = parseUE(&br); // conf_win_right_offset
        conf_win_top_offset = parseUE(&br); // conf_win_top_offset
        conf_win_bottom_offset = parseUE(&br); // conf_win_bottom_offset
    }

    //int bitDepthLumaMinus8          = parseUE(&br);
    //int bitDepthChromaMinus8        = parseUE(&br);
    //log2_max_pic_order_cnt_lsb_minus4 = parseUE(&br);

    /* sps_sub_layer_ordering_info_present_flag */
    //i = br.getBits(1) ? 0 : sps_max_sub_layers_minus1;
	//for (; i <= sps_max_sub_layers_minus1; i++) {
    //    parseUE(&br); // max_dec_pic_buffering_minus1
	//	parseUE(&br); // max_num_reorder_pics
	//	parseUE(&br); // max_latency_increase_plus1
	//}

    //parseUE(&br); // log2_min_luma_coding_block_size_minus3
    //parseUE(&br); // log2_diff_max_min_luma_coding_block_size
    //parseUE(&br); // log2_min_transform_block_size_minus2
    //parseUE(&br); // log2_diff_max_min_transform_block_size
    //parseUE(&br); // max_transform_hierarchy_depth_inter
    //parseUE(&br); // max_transform_hierarchy_depth_intra

	int sub_width_c  = ((1==chroma_format_idc)||(2 == chroma_format_idc))&&(0==separate_colour_plane_flag)?2:1;
	int sub_height_c = (1==chroma_format_idc)&& (0 == separate_colour_plane_flag)?2:1;
	int nWidth  = pic_width_in_luma_samples;
	int nHeight = pic_height_in_luma_samples;

	nWidth  -= (sub_width_c*conf_win_right_offset + sub_width_c*conf_win_left_offset);
	nHeight -= (sub_height_c*conf_win_bottom_offset + sub_height_c*conf_win_top_offset);

	if(width) {
		*width = nWidth;
		*height = nHeight;
	}

}

#define XRAW_IS_ANNEXB(p) ( !(*((p)+0)) && !(*((p)+1)) && (*((p)+2)==1))
#define XRAW_IS_ANNEXB2(p) ( !(*((p)+0)) && !(*((p)+1)) && !(*((p)+2))&& (*((p)+3)==1))
bool IsAVCReferenceFrame(TTBuffer* pInBuffer)
{
	TTPBYTE pBuffer = pInBuffer->pBuffer;
	int size = pInBuffer->nSize;
	TTPBYTE buffer = pBuffer;
	if (buffer[2]==0 && buffer[3]==1) {
		buffer+=4;
		size -= 4;
	} else {
		buffer+=3;
		size -= 3;
	}

	TTInt naluType = buffer[0]&0x0f;
	TTInt isRef	 = 1;
	while(naluType!=1 && naluType!=5)//find next NALU
	{
		TTPBYTE p = buffer;  
		TTPBYTE endPos = buffer+size;
		for (; p < endPos; p++) {
			if (XRAW_IS_ANNEXB(p))	{
				size  -= p-buffer;
				buffer = p+3;
				naluType = buffer[0]&0x0f;
				break;
			}

			if (XRAW_IS_ANNEXB2(p))	{
				size  -= p-buffer;
				buffer = p+4;
				naluType = buffer[0]&0x0f;
				break;
			}
		}

		if(p>=endPos)
			return false; 
	}
	
	if(naluType == 5)
		return true;

	if(naluType==1)	{
		isRef = (buffer[0]>>5) & 3;
	}

	return (isRef != 0);
}

//
//bool ExtractDimensionsFromVOLHeader(
//        const uint8_t *data, size_t size, int32_t *width, int32_t *height) {
//    ABitReader br(&data[4], size - 4);
//    br.skipBits(1);  // random_accessible_vol
//    unsigned video_object_type_indication = br.getBits(8);
//
//    CHECK_NE(video_object_type_indication,
//             0x21u /* Fine Granularity Scalable */);
//
//    unsigned video_object_layer_verid;
//    unsigned video_object_layer_priority;
//    if (br.getBits(1)) {
//        video_object_layer_verid = br.getBits(4);
//        video_object_layer_priority = br.getBits(3);
//    }
//    unsigned aspect_ratio_info = br.getBits(4);
//    if (aspect_ratio_info == 0x0f /* extended PAR */) {
//        br.skipBits(8);  // par_width
//        br.skipBits(8);  // par_height
//    }
//    if (br.getBits(1)) {  // vol_control_parameters
//        br.skipBits(2);  // chroma_format
//        br.skipBits(1);  // low_delay
//        if (br.getBits(1)) {  // vbv_parameters
//            br.skipBits(15);  // first_half_bit_rate
//            CHECK(br.getBits(1));  // marker_bit
//            br.skipBits(15);  // latter_half_bit_rate
//            CHECK(br.getBits(1));  // marker_bit
//            br.skipBits(15);  // first_half_vbv_buffer_size
//            CHECK(br.getBits(1));  // marker_bit
//            br.skipBits(3);  // latter_half_vbv_buffer_size
//            br.skipBits(11);  // first_half_vbv_occupancy
//            CHECK(br.getBits(1));  // marker_bit
//            br.skipBits(15);  // latter_half_vbv_occupancy
//            CHECK(br.getBits(1));  // marker_bit
//        }
//    }
//    unsigned video_object_layer_shape = br.getBits(2);
//    CHECK_EQ(video_object_layer_shape, 0x00u /* rectangular */);
//
//    br.getBits(1);  // marker_bit
//    unsigned vop_time_increment_resolution = br.getBits(16);
//    br.getBits(1);  // marker_bit
//
//    if (br.getBits(1)) {  // fixed_vop_rate
//        // range [0..vop_time_increment_resolution)
//
//        // vop_time_increment_resolution
//        // 2 => 0..1, 1 bit
//        // 3 => 0..2, 2 bits
//        // 4 => 0..3, 2 bits
//        // 5 => 0..4, 3 bits
//        // ...
//
//        CHECK_GT(vop_time_increment_resolution, 0u);
//        --vop_time_increment_resolution;
//
//        unsigned numBits = 0;
//        while (vop_time_increment_resolution > 0) {
//            ++numBits;
//            vop_time_increment_resolution >>= 1;
//        }
//
//        br.skipBits(numBits);  // fixed_vop_time_increment
//    }
//
//    br.getBits(1);  // marker_bit
//    unsigned video_object_layer_width = br.getBits(13);
//    br.getBits(1);  // marker_bit
//    unsigned video_object_layer_height = br.getBits(13);
//    br.getBits(1);  // marker_bit
//
//    unsigned interlaced = br.getBits(1);
//
//    *width = video_object_layer_width;
//    *height = video_object_layer_height;
//
//    return true;
//}


static const int kAACSampleRate[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000};

int GetAACFrameSize(
        unsigned char* pBuffer, unsigned int size, int *frame_size,
        int *out_sampling_rate, int *out_channels)
{
	int framelen = 0;
	int sampleIndex, profile, channel;
	int maxFrameLen = 2048;
	int aSync = 1;
	unsigned char *pBuf = pBuffer;

	if (out_sampling_rate) {
        *out_sampling_rate = 0;
    }

    if (out_channels) {
        *out_channels = 0;
    }

	if(frame_size) {
		*frame_size = 0;
	}

	if(pBuf == NULL) {
		return -1;
	}

	do {
		int  i;
		int  inLen = size;

		for (i = 0; i < inLen - 1; i++) {			
			if ( (pBuf[0] & 0xFF) == 0xFF && (pBuf[1] & 0xF0) == 0xF0 )
				break;

			pBuf++;
			inLen--;
			if (inLen <= 7)	{
				return -1;
			}
		}

		framelen = ((pBuf[3] & 0x3) << 11) + (pBuf[4] << 3) + (pBuf[5] >> 5);
		sampleIndex = (pBuf[2] >> 2) &0xF;
		profile = (pBuf[2] >> 6) + 1;
		channel = ((pBuf[2]&0x01) << 2) | (pBuf[3] >> 6);

		if(framelen > maxFrameLen || profile > 2 || channel > 6 || sampleIndex > 12) {
			pBuf++;
			inLen--;
			continue;
		}

		if(framelen > inLen || inLen == 0){
			return -1;
		}

		if(framelen > inLen) {
			pBuf++;
			inLen--;
			continue;		
		}

		if(framelen + 2 < inLen) {
			if(pBuf[framelen] == 0xFF && (pBuf[framelen + 1] & 0xF0) == 0xF0) {
				aSync = 0;
			}				
		}

		if(framelen == inLen){
			aSync = 0;
		}
	}while(aSync);

	if(frame_size) {
		*frame_size = pBuf - pBuffer + framelen;
	}

	if (out_sampling_rate) {
        *out_sampling_rate = kAACSampleRate[sampleIndex];
    }

    if (out_channels) {
       *out_channels = channel;
    }

	return 0;
}

int ConvertAVCNalHead(unsigned char* pOutBuffer, int& nOutSize, unsigned char* pInBuffer, int nInSize, int &nNalLength)
{
	if (pOutBuffer == NULL || pInBuffer == NULL)
		return -1;
	
	if (nInSize < 12)
		return -1;

	//char configurationVersion = pInBuffer[0];
	//char AVCProfileIndication = pInBuffer[1];
	//char profile_compatibility = pInBuffer[2];
	//char AVCLevelIndication  = pInBuffer[3];

	nNalLength =  (pInBuffer[4]&0x03)+1;
	int nNalWord = 0x01000000;
	if (nNalLength == 3)
		nNalWord = 0X010000;

	TTInt nNalLen = nNalLength;
	if (nNalLength < 3)	{
		nNalLen = 4;
	}

	int HeadSize = 0;
	int i = 0;

	int nSPSNum = pInBuffer[5]&0x1f;
	unsigned char* pBuffer = pInBuffer + 6;

	for (i = 0; i< nSPSNum; i++)
	{
		TTUint32 nSPSLength = (pBuffer[0]<<8)| pBuffer[1];
		pBuffer += 2;

		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;

		if(nSPSLength > (nInSize - (pBuffer - pInBuffer))){
			return -1;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nSPSLength);
		HeadSize += nSPSLength;
		pBuffer += nSPSLength;
	}

	int nPPSNum = *pBuffer++;
	for (i=0; i< nPPSNum; i++)
	{
		TTUint32 nPPSLength = (pBuffer[0]<<8) | pBuffer[1];
		pBuffer += 2;
		
		memcpy (pOutBuffer + HeadSize, &nNalWord, nNalLen);
		HeadSize += nNalLen;
		
		if(nPPSLength > (nInSize - (pBuffer - pInBuffer))){
			return -1;
		}

		memcpy (pOutBuffer + HeadSize, pBuffer, nPPSLength);
		HeadSize += nPPSLength;
		pBuffer += nPPSLength;
	}

	nOutSize = HeadSize;

	return 0;
}

int ConvertHEVCNalHead(unsigned char* pOutBuffer, int& nOutSize, unsigned char* pInBuffer, int nInSize, int &nNalLength)
{
	if (pOutBuffer == NULL || pInBuffer == NULL)
		return -1;

	if (nInSize < 22)
		return -1;

	TTPBYTE pData = pInBuffer;
	nNalLength =  (pData[21]&0x03)+1;
	TTInt nNalLen = nNalLength;
	if (nNalLength < 3)	{
		nNalLen = 4;
	}

	TTUint32 nNalWord = 0x01000000;
	if (nNalLength == 3)
		nNalWord = 0X010000;

	TTInt nHeadSize = 0;
	TTPBYTE pBuffer = pOutBuffer;
	TTInt nArrays = pData[22];
	TTInt nNum = 0;;

	pData += 23;
	if(nArrays)
	{
		for(nNum = 0; nNum < nArrays; nNum++)
		{
			unsigned char nal_type = 0;
			nal_type = pData[0]&0x3F;
			pData += 1;
			switch(nal_type)
			{
			case 33://sps
				{
					TTUint32 nSPSNum = (pData[0] << 8)|pData[1];
					pData += 2;
					for(int i = 0; i < nSPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						TTInt nSPSLength = (pData[0] << 8)|pData[1];
						pData += 2;
						if(nSPSLength > (nInSize - (pData - pInBuffer))){
							nOutSize = 0;
							return -1;
						}

						memcpy (pBuffer + nHeadSize, pData, nSPSLength);
						nHeadSize += nSPSLength;
						pData += nSPSLength;
					}
				}
				break;
			case 34://pps
				{
					TTUint32 nPPSNum = (pData[0] << 8) | pData[1];
					pData += 2;
					for(int i = 0; i < nPPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						TTInt nPPSLength = (pData[0] << 8)| pData[1];
						pData += 2;
						if(nPPSLength > (nInSize - (pData - pInBuffer))){
							nOutSize = 0;
							return -1;
						}
						memcpy (pBuffer + nHeadSize, pData, nPPSLength);
						nHeadSize += nPPSLength;
						pData += nPPSLength;
					}
				}
				break;
			case 32: //vps
				{
					TTUint32 nVPSNum = (pData[0] << 8 )| pData[1] ;
					pData += 2;
					for(int i = 0; i < nVPSNum; i++)
					{
						memcpy (pBuffer + nHeadSize, &nNalWord, nNalLen);
						nHeadSize += nNalLen;
						TTInt nVPSLength = (pData[0] << 8 )|pData[1];
						pData += 2;
						if(nVPSLength > (nInSize - (pData - pInBuffer))){
							nOutSize = 0;
							return -1;
						}
						memcpy (pBuffer + nHeadSize, pData, nVPSLength);
						nHeadSize += nVPSLength;
						pData += nVPSLength;
					}
				}
				break;
			default://just skip the data block
				{
					TTUint32 nSKP = (pData[0] << 8 )|pData[1];
					pData += 2;
					for(int i = 0; i < nSKP; i++)
					{
						TTInt nAKPLength = (pData[0] << 8) | pData[1];
						if(nAKPLength > (nInSize - (pData - pInBuffer))){
							nOutSize = 0;
							return -1;
						}
						pData += 2;
						pData += nAKPLength;
					}

				}
				break;
			}
		}
	}

	nOutSize = nHeadSize;

	return 0;
}

int ConvertAVCNalFrame(unsigned char* pOutBuffer, int& nOutSize, unsigned char* pInBuffer, int nInSize, int nNalLength, int &IsKeyFrame, int nType)
{
	unsigned char*  pBuffer = pInBuffer;
	int	nNalLen = 0;
	int	nNalType = 0;	
	int nNalWord = 0x01000000;
	if (nNalLength == 3)
		nNalWord = 0X010000;

	if(nNalLength == 0) {
		return -1;
	}

	int i = 0;
	int leftSize = nInSize;
	nOutSize = 0;

	while (pBuffer - pInBuffer + 4 < nInSize)
	{
		nNalLen = *pBuffer++;
		for (i = 0; i < (int)nNalLength - 1; i++)
		{
			nNalLen = nNalLen << 8;
			nNalLen += *pBuffer++;
		}

		if(nNalType != 1 && nNalType != 5) {
			if(nType == 12) {
				nNalType = (pBuffer[0] >> 1)&0x3f;
			} else {
				nNalType = pBuffer[0]&0x0f;
			}
		}

		leftSize -= nNalLength;

		if(nNalLen > leftSize)
		{
			nOutSize = 0;
			return -1;
		}

		if (nNalLength == 3 || nNalLength == 4)
		{
			memcpy ((pBuffer - nNalLength), &nNalWord, nNalLength);
		}
		else
		{
			memcpy (pOutBuffer + nOutSize, &nNalWord, 4);
			nOutSize += 4;
			memcpy (pOutBuffer + nOutSize, pBuffer, nNalLen);
			nOutSize += nNalLen;
		}

		leftSize -= nNalLen;
		pBuffer += nNalLen;
	}

	if(nType == 12) {
		if(nNalType >= 19 && nNalType <= 21)
			IsKeyFrame = 1;
	} else {
		if(nNalType == 5)
			IsKeyFrame = 1;
	}

	return 0;
}

int ParseAACConfig(
        unsigned char* pBuffer, unsigned int size,
        int *out_sampling_rate, int *out_channels)
{
	if(pBuffer == NULL || size < 2) {
		return -1;
	}

	int sampleIndex = -1;
	int sampFreq = 0;
	int channel = 0;

	//int object = pBuffer[0] >> 3;
	sampleIndex = ((pBuffer[0] & 7) << 1) | (pBuffer[1] >> 7);
	if(sampleIndex == 0x0f) {
		if(size < 5)
			return -1;

		sampFreq = ((pBuffer[1]&0x7f) << 17) | (pBuffer[2] << 9) | ((pBuffer[3] << 1)) | (pBuffer[4] >> 7);

		channel = (pBuffer[4]&0x78) >> 3;
	} else {
		channel = (pBuffer[1]&0x78) >> 3;
		sampFreq = kAACSampleRate[sampleIndex];
	}

	if(out_sampling_rate) {
		*out_sampling_rate = sampFreq;
	}

	if(out_channels) {
		*out_channels = channel;
	}

	return 0;
}

int ConstructAACHeader(
        unsigned char* pBuffer, unsigned int size,
        int in_sampling_rate, int in_channels, int in_framesize)
{
	if(pBuffer == NULL || size < 7) {
		return -1;
	}

	int sampleIndex = -1;
	int i;
	for (i = 0; i < 12; i++) {
		if (in_sampling_rate == kAACSampleRate[i]) {
			sampleIndex = i;
			break;
		}	
	}

	if(sampleIndex == -1) {
		return -1;
	}
	
	pBuffer[0] = 0xFF;
	pBuffer[1] = 0xF9;

	pBuffer[2] = 0x40 | (unsigned char)(sampleIndex << 2) |((unsigned char)(in_channels >> 2) & 0x01);
	pBuffer[3] = (unsigned char)((in_channels << 6) & 0xc0) | (0x01 << 3) | (unsigned char)(((in_framesize + 7) >> 11) & 0x03); 
	pBuffer[4] = (unsigned char)((unsigned short)((in_framesize + 7) >> 3) & 0x00ff);
	pBuffer[5] = (unsigned char)((unsigned char)((in_framesize + 7) & 0x07) << 5) | 0x1f;
	pBuffer[6] = 0xF8;

	return 7;
}

int GetMPEGAudioFrameSize(
        unsigned char* pBuf, unsigned int *frame_size,
        int *out_sampling_rate, int *out_channels,
        int *out_bitrate, int *out_num_samples) 
{
	if(frame_size) {
		*frame_size = 0;
	}

    if (out_sampling_rate) {
        *out_sampling_rate = 0;
    }

    if (out_channels) {
        *out_channels = 0;
    }

    if (out_bitrate) {
        *out_bitrate = 0;
    }

    if (out_num_samples) {
        *out_num_samples = 1152;
    }

	if(pBuf == NULL) {
		return -1;
	}

	unsigned int verIdx		= (pBuf[1] >> 3) & 0x03;
	unsigned int version    = (pBuf[1] >> 3) & 0x03;
	unsigned int layer		=  4 - ((pBuf[1] >> 1) & 0x03);     
	unsigned int brIdx		= (pBuf[2] >> 4) & 0x0f;
	unsigned int srIdx		= (pBuf[2] >> 2) & 0x03;
	unsigned int paddingBit	= (pBuf[2] >> 1) & 0x01;
	unsigned int mode	    = (pBuf[3] >> 6) & 0x03;

	if (srIdx == 3 || brIdx == 15 || verIdx == 1)
		return -1;

    if (layer == 0x04) {
        layer -= 1;
    }

    static const int kSamplingRateV1[] = { 44100, 48000, 32000 };
    int sampling_rate = kSamplingRateV1[srIdx];
    if (version == 2 /* V2 */) {
        sampling_rate /= 2;
    } else if (version == 0 /* V2.5 */) {
        sampling_rate /= 4;
    }

     if (layer == 1) {
        // layer I
        static const int kBitrateV1[] = {
            0, 32, 64, 96, 128, 160, 192, 224, 256,
            288, 320, 352, 384, 416, 448
        };

        static const int kBitrateV2[] = {
            0, 32, 48, 56, 64, 80, 96, 112, 128,
            144, 160, 176, 192, 224, 256
        };

        int bitrate =
            (version == 3 /* V1 */)
                ? kBitrateV1[brIdx]
                : kBitrateV2[brIdx];

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        *frame_size = (12000 * bitrate / sampling_rate + paddingBit) * 4;

        if (out_num_samples) {
            *out_num_samples = 384;
        }
    } else {
        // layer II or III
        static const int kBitrateV1L2[] = {
            0, 32, 48, 56, 64, 80, 96, 112, 128,
            160, 192, 224, 256, 320, 384
        };

        static const int kBitrateV1L3[] = {
            0, 32, 40, 48, 56, 64, 80, 96, 112,
            128, 160, 192, 224, 256, 320
        };

        static const int kBitrateV2[] = {
            0, 8, 16, 24, 32, 40, 48, 56, 64,
            80, 96, 112, 128, 144, 160
        };

        int bitrate;
        if (version == 3 /* V1 */) {
            bitrate = (layer == 2 /* L2 */)
                ? kBitrateV1L2[brIdx]
                : kBitrateV1L3[brIdx];

            if (out_num_samples) {
                *out_num_samples = 1152;
            }
        } else {
            // V2 (or 2.5)
            bitrate = kBitrateV2[brIdx];
            if (out_num_samples) {
                *out_num_samples = (layer == 3 /* L3 */) ? 576 : 1152;
            }
        }

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        if (version == 3 /* V1 */) {
            *frame_size = 144000 * bitrate / sampling_rate + paddingBit;
        } else {
            // V2 or V2.5
            unsigned int tmp = (layer == 1 /* L3 */) ? 72000 : 144000;
            *frame_size = tmp * bitrate / sampling_rate + paddingBit;
        }
    }

    if (out_sampling_rate) {
        *out_sampling_rate = sampling_rate;
    }

    if (out_channels) {
        *out_channels = (mode == 3) ? 1 : 2;
    }

    return 0;
}


