#ifndef _STRUCT_H
#define _STRUCT_H

#include "GKTypedef.h"
#include "ttAACDec.h"
#include "global.h"
#include "bitstream.h"

//#define MAX_CHANNELS		8		
#define MAX_SAMPLES		1024

#define BUFFER_GUARD	8
#define BUFFER_DATA		2048*4 + BUFFER_GUARD

#define SQRT1_2 0x5a82799a	/* sqrt(1/2) in Q31 */

#define NUM_LONG_WIN           1
#define NUM_SHORT_WIN          8

#define FILL_BUF_SIZE           269             /* max count = 15 + 255 - 1*/


#define RND_VAL		(1 << (SCLAE_IMDCT-1))
#define NUM_FFT_SIZES	2

#define MAX_HUFF_BITS                   20
#define HUFFTAB_SPEC_OFFSET             1



#define SF_DQ_OFFSET            6
#define FBITS_OUT_DQ            11
#define FBITS_OUT_DQ_OFF        5   /* number of fraction bits out of dequant, including 2^15 bias */

#define FBITS_IN_IMDCT          FBITS_OUT_DQ_OFF        /* number of fraction bits into IMDCT */
#define GBITS_IN_DCT4           4                                       /* min guard bits in for DCT4 */

#define FBITS_LOST_DCT4         1               /* number of fraction bits lost (>> out) in DCT-IV */
#define FBITS_LOST_WND          1               /* number of fraction bits lost (>> out) in synthesis window (neg = gain frac bits) */
#define FBITS_LOST_IMDCT        2
#define SCLAE_IMDCT         3

#define NUM_IMDCT_SIZES         2


typedef union _U64 {
	TTInt64 w64;
	struct {
		TTUint32  lo32; 
		TTInt32	  hi32;
	} r;
} U64;

typedef struct _HuffInfo {
    int		maxBits;                                                /* number of bits in longest codeword */
    TTUint8 count[MAX_HUFF_BITS];		/* count[i] = number of codes with length i+1 bits */
    int		offset;                                                       /* offset into symbol table */
} HuffInfo;

typedef enum  { 
	ONLY_LONG_SEQUENCE, 
	LONG_START_SEQUENCE, 
	EIGHT_SHORT_SEQUENCE,
	LONG_STOP_SEQUENCE,
	NUM_WIN_SEQ        
} WINDOW_SEQUENCE; 

typedef struct{
    TTUint8  number_pulse;
    TTUint8  pulse_start_sfb;
    TTUint8  pulse_offset[4];
    TTUint8  pulse_amp[4];
} Pulse_Data;

typedef struct{
    TTUint8  n_filt;     
    TTUint8  coef_res;
    TTUint8  length[4];
    TTUint8  order[4];
    TTUint8  direction[4];
	TTUint8  coef_compress[4];
    TTUint8  coef[4][32];              
} TNS_Data;

typedef struct
{
	TTUint8  last_band;
	TTUint8  data_present;
	TTUint16 lag;
	TTUint8  lag_update;
	TTUint8  coef;
	TTUint8  long_used[MAX_SFB];
	TTUint8  short_used[8];
	TTUint8  short_lag_present[8];
	TTUint8  short_lag[8];
}LTP_Data;

typedef struct{
    TTUint8  ics_reserved_bit;
    TTUint8  window_sequence;
    TTUint8  window_shape;
    TTUint8  max_sfb;
    TTUint8  scale_factor_grouping;
    TTUint8  predictor_data_present;

    TTUint8  predictor_reset;
    TTUint8  predictor_reset_group_number;
    TTUint8  prediction_used[MAX_SFB];
	TTUint8  num_window_groups;
    TTUint8  window_group_length[MAX_WINDOW_GROUPS];
} ICS_Data;

typedef struct {
    TTInt32 r[2];
    TTInt64 COR[2];
    TTInt64 VAR[2];
} PRED_State;

typedef struct
{
    TTUint8  element_instance_tag;
    TTUint8  object_type;
    TTUint8  sampling_frequency_index;
    TTUint8  num_front_channel_elements;
    TTUint8  num_side_channel_elements;
    TTUint8  num_back_channel_elements;
    TTUint8  num_lfe_channel_elements;
    TTUint8  num_assoc_data_elements;
    TTUint8  num_valid_cc_elements;
    TTUint8  mono_mixdown_present;
    TTUint8  mono_mixdown_element_number;
    TTUint8  stereo_mixdown_present;
    TTUint8  stereo_mixdown_element_number;
    TTUint8  matrix_mixdown_idx_present;
    TTUint8  matrix_mixdown_idx;
	TTUint8  pseudo_surround_enable;
    TTUint8  front_element_is_cpe[16];
    TTUint8  front_element_tag_select[16];
    TTUint8  side_element_is_cpe[16];
    TTUint8  side_element_tag_select[16];
    TTUint8  back_element_is_cpe[16];
    TTUint8  back_element_tag_select[16];
    TTUint8  lfe_element_tag_select[16];
    TTUint8  assoc_data_element_tag_select[16];
    TTUint8  cc_element_is_ind_sw[16];
    TTUint8  valid_cc_element_tag_select[16];
	
    TTUint8  comment_field_bytes;

	//output value
	TTUint8  num_front_channels;
    TTUint8  num_side_channels;
    TTUint8  num_back_channels;
    TTUint8  num_lfe_channels;
	TTUint8  channels;
	
} program_config;

typedef struct
{
    TTUint16 syncword;
    TTUint8  ID;
    TTUint8  layer;
    TTUint8  protection_absent;
    TTUint8  profile;
    TTUint8  sampling_frequency_index;
    TTUint8  private_bit;
    TTUint8  channel_configuration;
    TTUint8  original;
    TTUint8  home;
    TTUint8  copyright_identification_bit;
    TTUint8  copyright_identification_start;
    TTUint16 frame_length;
    TTUint16 adts_buffer_fullness;
    TTUint8  number_of_raw_data_blocks_in_frame;
    TTUint16 crc_check;	
} adts_header;

#define IS_ADIFHEADER(h)				((h)[0] == 'A' && (h)[1] == 'D' && (h)[2] == 'I' && (h)[3] == 'F')

typedef struct
{
	TTUint32 adif_id;
    TTUint8  copyright_id_present;
    TTUint8  copyright_id[9];//72 bits
    TTUint8  original_copy;
    TTUint8  home;
    TTUint8  bitstream_type;
    TTUint32 bitrate;
    TTUint8  num_program_config_elements;
    TTUint32 adif_buffer_fullness;
    program_config pce[16];
} adif_header;

#define MAXLATMLAYER 4

typedef struct {

	int			  streamId[MAXLATMLAYER];	

	TTUint8		  progSIndex[MAXLATMLAYER];
	TTUint8   	  laySIndex[MAXLATMLAYER];

	TTUint8		  frameLengthTypes[MAXLATMLAYER];
	TTUint8		  latmBufferFullness[MAXLATMLAYER];
	TTUint16	  frameLength[MAXLATMLAYER];
	TTUint16      muxSlotLengthBytes[MAXLATMLAYER];
	TTUint8		  muxSlotLengthCoded[MAXLATMLAYER];
	TTUint8		  progCIndx[MAXLATMLAYER];
	TTUint8		  layCIndx[MAXLATMLAYER];
	TTUint8		  AuEndFlag[MAXLATMLAYER];
	TTUint8		  numLayer;
	TTUint8		  objTypes;
	TTUint8		  audio_mux_version;
	TTUint8		  audio_mux_version_A;
	TTUint8		  all_same_framing;
	TTUint8		  config_crc;
	int			  taraFullness;
	int			  other_data_bits;
	int			  numProgram;
	int			  numSubFrames;
	int			  numChunk;
	int			  samplerate;
	int			  channel;
	int			  muxlength;
} latm_header;

typedef struct{
	unsigned char *start;
	unsigned char *input;
	unsigned char *this_frame;
	unsigned int  uselength;
	unsigned int  inlen;
	unsigned int  length;
	unsigned int  storelength;
	unsigned int  maxLength;
} FrameStream;

typedef struct {

	/*channel configure */
	program_config pce;
	int			pce_set;
	int			channel_position[MAX_CHANNELS];
	int			channel_offsize[MAX_CHANNELS];

    ICS_Data    ICS_Data[MAX_SYNTAX_ELEMENTS];
	int			global_gain;
    int         common_window;
    TTInt16     scaleFactors[MAX_SYNTAX_ELEMENTS][MAX_SFB];
    TTUint8		sfb_cb[MAX_SYNTAX_ELEMENTS][MAX_SFB];
	
	TTUint32		pns_seed;
	int			pns_start_sfb;
	int         pns_data_present[MAX_SYNTAX_ELEMENTS];
	TTUint8		pns_sfb_flag[MAX_SYNTAX_ELEMENTS][MAX_SFB];
	TTUint8		pns_sfb_mode[MAX_SFB];

    int         ms_mask_present;
	TTUint8		ms_used[8][MAX_SFB/2];    
    int         intensity_used;
	TTUint8		is_used[8][MAX_SFB/2];

	int         gbCurrent[MAX_SYNTAX_ELEMENTS];

    int			pulse_data_present[MAX_SYNTAX_ELEMENTS];
	Pulse_Data  pulse_data[MAX_SYNTAX_ELEMENTS];
	
    int			tns_data_present[MAX_SYNTAX_ELEMENTS];
	TNS_Data    tns_data[MAX_SYNTAX_ELEMENTS][MAX_WINDOW_GROUPS];
	int			tns_lpc[48];

#ifdef LTP_DEC	
	LTP_Data	Ltp_Data[MAX_SYNTAX_ELEMENTS];//make it compatible to the LC,keep the data structure for syntax parsing	
	TTInt16		ltp_lag[MAX_CHANNELS];
	int			*ltp_coef[MAX_CHANNELS]; //[MAX_SAMPLES*4];
	int			*t_est_buf;
	int			*f_est_buf;
#endif//LTP_DEC

	/* used to save the prediction state */
#ifdef MAIN_DEC
	PRED_State  *pred_stat[MAX_CHANNELS];
#endif//LTP_DEC

	int         prevWinShape[MAX_CHANNELS];	
	int			*coef[MAX_SYNTAX_ELEMENTS];
	TTInt32		*tmpBuffer;//16KB this is a scratch buffer
	int			*overlap[MAX_CHANNELS];

	/* raw decoded data, before rounding to 16-bit PCM (for postprocessing such as SBR) */
	void		*rawSampleBuf[MAX_CHANNELS];
	int			rawSampleBytes;
	int			rawSampleFBits;

	/* MPEG-4 BSAC decoding */
	void		*bsac; 
	/* MPEG-4 SBR decoding */
	void		*sbr;	

	/* fill data (can be used for processing SBR or other extensions) */
	int			fillExtType;
    int         fillCount;
	TTUint8		fillBuf[FILL_BUF_SIZE];

	BitStream	bs;	
	
	/* frame Header */
	adts_header adts;
	adts_header adts_new;
	adif_header adif;
	latm_header *latm;

	/* block information */
	int			old_id_syn_ele;
	int			id_syn_ele;
	int			first_id_syn_ele;
	
	/*channels info*/
	int			decodedChans;
	int			ChansMode;
	int			elementChans;
	int			decodedSBRChans;
	int			elementSBRChans;
	
	/* frame info */
	int			frame_length;
	int			stereo_mode;
	int			channelNum;
	int         sampRateIdx;
	int			sampleRate;
	int			sampleBits;
	int			profile;
	int			frametype;

	/* decoder parameter */
	int			framenumber;
	int			profilenumber;
	int			Checknumber;
	int			errNumber;
	int			sbrEnabled;
	int			chSpec;
	int			seletedChs;
	int			seletedChDecoded;
	int			seletedSBRChDecoded;
	int			lp_sbr;//low power sbr
	int			forceUpSample;
	int			errorConcealment;
	int			disableSBR;
	int			disablePS;
	int			decoderNum;

	int         nFlushFlag;    //fix #8710

	/* frame buffer  */
	FrameStream	Fstream;

	/* outFormat */
	TTAudioFormat outAudioFormat;
} AACDecoder;

extern const int cos4sin4tab[128 + 1024];
extern const int sinWindow[128 + 1024];
extern const int kbdWindow[128 + 1024];
extern const int twidTabEven[4*6 + 16*6 + 64*6];
extern const int twidTabOdd[8*6 + 32*6 + 128*6];
extern const int cos1sin1tab[514];
extern const int nfftTab[NUM_FFT_SIZES];
extern const int nfftlog2Tab[NUM_FFT_SIZES];
extern const int nmdctTab[NUM_IMDCT_SIZES];
extern const int postSkip[NUM_IMDCT_SIZES];

/* hufftabs.c */
//extern const HuffInfo huffTabSpecInfo[11];
extern const signed short huffTabSpec[1241];
extern const HuffInfo huffTabScaleFactInfo; 
extern const signed short huffTabScaleFact[121];

/* tabs.c */
extern const int cos4sin4tabOffset[NUM_IMDCT_SIZES];
extern const int sinWindowOffset[NUM_IMDCT_SIZES];
extern const int kbdWindowOffset[NUM_IMDCT_SIZES];
extern const TTUint8 BitRevTab[17 + 129];
extern const int bitrevtabOffset[NUM_IMDCT_SIZES];

/* aactabs.c - global ROM tables */
extern const int sampRateTab[NUM_SAMPLE_RATES];
extern const int predSFBMax[NUM_SAMPLE_RATES];
extern const int elementNumChans[ELM_ID_SIZE];
extern const int sfBandTabShortOffset[NUM_SAMPLE_RATES];
extern const short sfBandTabShort[76];
extern const int sfBandTabLongOffset[NUM_SAMPLE_RATES];
extern const short sfBandTabLong[325];

#endif// _STRUCT_H

