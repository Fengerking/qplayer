#ifndef _SBR_H
#define _SBR_H

#include "global.h"
#include "bitstream.h"
#ifdef PS_DEC
#include "ps_dec.h"
#endif
#include "struct.h"

#define NUM_TIME_SLOTS	    16
#define TT_SAMP_PER_SLOT		2	/* RATE in spec */
#define TT_SAMP_RATES_NUM 	9	/* downsampled (single-rate) mode unsupported, so only use Fs_sbr >= 16 kHz */

#define TT_MAX_NUM_ENV					5
#define MAX_NUM_NOISE_FLOORS		2
#define MAX_NUM_NOISE_FLOOR_BANDS	5	/* max Nq, see 4.6.18.3.6 */
#define MAX_NUM_PATCHES				5
#define MAX_NUM_SMOOTH_COEFS		5

#define SBR_HF_GEN			8
#define SBR_HF_ADJ			2

/* max QMF subbands covered by SBR (4.6.18.3.6) */
#define MQ_BANDS	48		

#define SBR_IN_QMFA	14
#define SBR_LOST_QMFA	(1 + 2 + 3 + 2 + 1)	/* 1 from cTab, 2 in premul, 3 in FFT, 2 in postmul, 1 for implicit scaling by 2.0 */
#define SBR_OUT_QMFA	(SBR_IN_QMFA - SBR_LOST_QMFA)//5

#define SBR_GBITS_IN_QMFS		2
#define SBR_IN_QMFS			    SBR_OUT_QMFA    //5
#define SBR_LOST_DCT4_64		(2 + 3 + 2)		//7 /* 2 in premul, 3 in FFT, 2 in postmul */

#define SBR_OUT_DQ_ENV	            29	     /* dequantized env scalefactors are Q(29 - envDataDequantScale) */
#define SBR_OUT_DQ_NOISE	        24	     /* range of Q_orig = [2^-24, 2^6] */
#define SBR_NOISE_FLOOR_OFFSET	    6

/* see comments in ApplyBoost() */
#define SBR_GLIM_BOOST	    24
#define SBR_QLIM_BOOST	    14

#define MAX_HUFF_BITS		20
#define NUM_QMF_DELAY_BUFS	10
#define DELAY_SAMPS_QMFA	(NUM_QMF_DELAY_BUFS * 32)
#define DELAY_SAMPS_QMFS	(NUM_QMF_DELAY_BUFS * 128)


/* Huffman Table Index */
#define ttHuffTab_t_Env15      0
#define ttHuffTab_f_Env15      1
#define ttHuffTab_t_Env15b     2
#define ttHuffTab_f_Env15b     3
#define ttHuffTab_t_Env30      4
#define ttHuffTab_f_Env30      5
#define ttHuffTab_t_Env30b     6
#define ttHuffTab_f_Env30b     7
#define ttHuffTab_t_Noise30    8
#define ttHuffTab_f_Noise30    5
#define ttHuffTab_t_Noise30b   9
#define ttHuffTab_f_Noise30b   7

/* Grid control */
#define ttSBR_CLA_BITS  2
#define ttSBR_ABS_BITS  2
#define ttSBR_RES_BITS  1
#define ttSBR_REL_BITS  2
#define ttSBR_ENV_BITS  2
#define ttSBR_NUM_BITS  2

/* four different SBR frame classes */
#define ttFIXFIX  0
#define ttFIXVAR  1
#define ttVARFIX  2
#define ttVARVAR  3

#define SBR_FREQ_SCALE_DEFAULT                  2
#define SBR_ALTER_SCALE_DEFAULT                 1
#define SBR_NOISE_BANDS_DEFAULT                 2
#define SBR_LIMITER_BANDS_DEFAULT               2
#define SBR_LIMITER_GAINS_DEFAULT               2
#define SBR_INTERPOL_FREQ_DEFAULT               1
#define SBR_SMOOTHING_LENGTH_DEFAULT            1  /* 0: on  1: off */

/* header */
#define SI_SBR_AMP_RES_BITS                     1
#define SI_SBR_START_FREQ_BITS                  4
#define SI_SBR_STOP_FREQ_BITS                   4
#define SI_SBR_XOVER_BAND_BITS                  3
#define SI_SBR_DATA_EXTRA_BITS                  1
#define SI_SBR_HEADER_EXTRA_1_BITS              1
#define SI_SBR_HEADER_EXTRA_2_BITS              1
#define SI_SBR_PSEUDO_STEREO_MODE_BITS          2
#define SI_SBR_FREQ_SCALE_BITS                  2
#define SI_SBR_ALTER_SCALE_BITS                 1
#define SI_SBR_NOISE_BANDS_BITS                 2
#define SI_SBR_LIMITER_BANDS_BITS               2
#define SI_SBR_LIMITER_GAINS_BITS               2
#define SI_SBR_INTERPOL_FREQ_BITS               1
#define SI_SBR_SMOOTHING_LENGTH_BITS            1
#define SI_SBR_RESERVED_HE2_BITS                1
#define SI_SBR_RESERVED_BITS_HDR                2
#define SI_SBR_RESERVED_BITS_DATA               4

/* need one SBRHeader per element (SCE/CPE), updated only on new header */
typedef struct _SBRHeader {
	TTInt32		  count;

	TTUint8		  ampRes;
	TTUint8       startFreq;
	TTUint8       stopFreq;
	TTUint8       crossOverBand;
	TTUint8       resBitsHdr;
	TTUint8       hdrExtra1;
	TTUint8       hdrExtra2;

	TTUint8       freqScale;
	TTUint8       alterScale;
	TTUint8       noiseBands;
	
	TTUint8       limiterBands;
	TTUint8       limiterGains;
	TTUint8       interpFreq;
	TTUint8       smoothMode;
} SBRHeader;

/* need one SBRGrid per channel, updated every frame */
typedef struct _SBRGrid {
	TTUint8       FrameType;
	TTUint8       SBR_AmpRes_30;
	TTUint8       ptr;

	TTUint8       L_E;						                 /* L_E */
	TTUint8       t_E[TT_MAX_NUM_ENV+1];	                     /* t_E */
	TTUint8       freqRes[TT_MAX_NUM_ENV];			             /* r */

	TTUint8       L_Q;							             /* L_Q */
	TTUint8       t_Q[MAX_NUM_NOISE_FLOORS+1];	             /* t_Q */

	TTUint8       numEnvPrev;
	TTUint8       numNoiseFloorsPrev;
	TTUint8       freqResPrev;
} SBRGrid;

/* need one SBRFreq per element (SCE/CPE/LFE), updated only on header reset */
typedef struct _SBRFreq {
	int                   kStart;				/* k_x */
	int                   nMaster;
	int                   nHigh;
	int                   nLow;
	int                   nLimiter;				/* N_l */
	int                   numQMFBands;			/* M */
	int                   NQ;	/* Nq */

	int                   kStartPrev;
	int                   numQMFBandsPrev;

	TTUint8				  freqBandMaster[MQ_BANDS + 1];	/* not necessary to save this  after derived tables are generated */
	TTUint8				  freqBandHigh[MQ_BANDS + 1];
	TTUint8				  freqBandLow[MQ_BANDS / 2 + 1];	/* nLow = nHigh - (nHigh >> 1) */
	TTUint8				  freqBandNoise[MAX_NUM_NOISE_FLOOR_BANDS+1];
	TTUint8				  freqBandLim[MQ_BANDS / 2 + MAX_NUM_PATCHES];		/* max (intermediate) size = nLow + numPatches - 1 */

	TTUint8				  numPatches;
	TTUint8				  patchNumSubbands[MAX_NUM_PATCHES + 1];
	TTUint8				  patchStartSubband[MAX_NUM_PATCHES + 1];
} SBRFreq;

typedef struct _SBRChan {
	int                   reset;
	TTUint8				  deltaFlagEnv[TT_MAX_NUM_ENV];
	TTUint8				  deltaFlagNoise[MAX_NUM_NOISE_FLOORS];

	signed char           envDataQuant[TT_MAX_NUM_ENV][MQ_BANDS];		/* range = [0, 127] */
	signed char           noiseDataQuant[MAX_NUM_NOISE_FLOORS][MAX_NUM_NOISE_FLOOR_BANDS];

	TTUint8				  invfMode[2][MAX_NUM_NOISE_FLOOR_BANDS];	/* invfMode[0/1][band] = prev/curr */
	int                   chirpFact[MAX_NUM_NOISE_FLOOR_BANDS];		/* bwArray */
	TTUint8				  addHarmonicFlag[2];						/* addHarmonicFlag[0/1] = prev/curr */
	TTUint8               addHarmonic[2][64];						/* addHarmonic[0/1][band] = prev/curr */
	
	int                   gbMask[2];	/* gbMask[0/1] = XBuf[0-31]/XBuf[32-39] */
	signed char           laPrev;

	int                   fIndexNoise;
	int                   fIndexSine;
	int                   gIndexRing;
	int                   G_Temp[MAX_NUM_SMOOTH_COEFS][MQ_BANDS];
	int                   Q_Temp[MAX_NUM_SMOOTH_COEFS][MQ_BANDS];

} SBRChan;

#ifdef WMMX
#define ALIGN	__declspec(align(8))
#else//WMMX
#define ALIGN
#endif//WMMX
typedef struct{
	/* save for entire file */
	int                   number;
	int                   sampRateIdx;

	/* SBR bit stream header information */
	TTUint8               bs_ampRes;
	TTUint8               bs_startFreq;
	TTUint8               bs_stopFreq;
	TTUint8               bs_crossOverBand;

	/* state info that must be saved for each channel */
	SBRHeader             sbrHdr[MAX_CHANNELS];
	SBRHeader             sbrHdrPrev[MAX_CHANNELS];
	SBRHeader             sbrHdrPrevOK[MAX_CHANNELS];
	SBRGrid               sbrGrid[MAX_CHANNELS];
	SBRFreq               *sbrFreq[MAX_CHANNELS];
	SBRChan               *sbrChan[MAX_CHANNELS];

	int					  sbrError;
	/* temp variables, no need to save between blocks */
	TTUint8				  dataExtra;
	TTUint8				  resBitsData;
	TTUint8				  extendedDataPresent;
	int                   extendedDataSize;

	signed char           envDataDequantScale[MAX_SYNTAX_ELEMENTS][TT_MAX_NUM_ENV];
	int                   envDataDequant[MAX_SYNTAX_ELEMENTS][TT_MAX_NUM_ENV][MQ_BANDS];
	int                   noiseDataDequant[MAX_SYNTAX_ELEMENTS][MAX_NUM_NOISE_FLOORS][MAX_NUM_NOISE_FLOOR_BANDS];

	int                   eCurr[MQ_BANDS];
	TTUint8				  eCurrExp[MQ_BANDS];
	TTUint8				  eCurrExpMax;
	signed char           la;

	int                   crcCheckWord;
	int                   couplingFlag;
	int                   envBand;
	int                   eOMGainMax;
	int                   gainMax;
	int                   gainMaxFBits;
	int                   noiseFloorBand;
	int                   qp1Inv;
	int                   qqp1Inv;
	int                   sMapped;
	int                   sBand;
	int                   highBand;

	int                   sumEOrigMapped;
	int                   sumECurrGLim;
	int                   sumSM;
	int                   sumQM;
	int                   G_LimBoost[MQ_BANDS];
	int                   Qm_LimBoost[MQ_BANDS];
	int                   Sm_Boost[MQ_BANDS];

	int                   Sm_Buf[MQ_BANDS];
	int                   Qm_LimBuf[MQ_BANDS];
	int                   G_LimBuf[MQ_BANDS];
	int                   G_LimFbits[MQ_BANDS];

	int                   G_FiltLast[MQ_BANDS];
	int                   Q_FiltLast[MQ_BANDS];

	/* large buffers */
	int                   delayIdxQMFA[MAX_CHANNELS];
	int                   delayIdxQMFS[MAX_CHANNELS];
	
	int		              *delayQMFA[MAX_CHANNELS];  //[DELAY_SAMPS_QMFA];
	int				      *delayQMFS[MAX_CHANNELS];  //[DELAY_SAMPS_QMFS];
	int					  *XBufDelay[MAX_CHANNELS];  //[HF_GEN][64][2];
	ALIGN int             XBuf[32+8][64][2];
#ifdef PS_DEC
	ps_info				  *ps;
#endif
	int					  ps_used;
	int					  last_syn_ele;

} sbr_info;

/* ttLog2[i] = ceil(log2(i)) (disregard i == 0) */
static const unsigned char ttLog2[9] = {0, 0, 1, 2, 2, 3, 3, 3, 3};


#define TT_Multi32(A,B) (TTInt32)(((TTInt64)(A)*(TTInt64)(B)) >> 32)

static __inline int SBRGetBits(BitStream * const bsi,
				              const unsigned int nBits)
{
	unsigned int data, lowBits;

	data = bsi->iCache >> (32 - nBits);		
	bsi->iCache <<= nBits;					
	bsi->cachedBits -= nBits;				

	if (bsi->cachedBits < 0) {
		lowBits = -bsi->cachedBits;
		RefillBitstreamCache(bsi);
		data |= bsi->iCache >> (32 - lowBits);		
	
		bsi->cachedBits -= lowBits;			
		bsi->iCache <<= lowBits;			
	}
	return data;
}
/* sbrtabs.c */
extern const TTUint8 k0Tab[TT_SAMP_RATES_NUM ][16];
extern const TTUint8 k2Tab[TT_SAMP_RATES_NUM ][14];
extern const HuffInfo huffTabSBRInfo[10];
extern const signed char huffTabSBR[604];
extern const int log2Tab[65];
extern const int noiseTab[512*2];
extern const int cTabA[165];
extern const int cTabS[640];

int get_sr_index(const int sampleRate);

int ttSBRDecUpdateFreqTables(AACDecoder* decoder,
							 SBRHeader *sbrHdr, 
							 SBRFreq *sbrFreq, 
							 int sampRateIdx);

int ttSBR_Single_Channel_Element(AACDecoder* decoder,
						         BitStream *bs, 
						         int chBase
						         );

int ttSBR_Channel_Pair_Element(AACDecoder* decoder,
							   BitStream *bs,
							   int chBase
							   );

int QMFAnalysis(int *inbuf, 
				int *delay, 
				int *XBuf, 
				int fBitsIn, 
				int *delayIdx, 
				int qmfaBands
				);


int RMAACDecodePS(AACDecoder* decoder,
			      sbr_info* psi,
			      SBRGrid *sbrGrid,
			      SBRFreq *sbrFreq
				  );
void QMFSynthesisAfterPS(int *inbuf, int *delay, int *delayIdx, int qmfsBands, short *outbuf, int channelNum);
void QMFSynthesis(int *inbuf, 
				  int *delay, 
				  int *delayIdx, 
				  int qmfsBands, 
				  short *outbuf, 
				  int channelNum);


int ttHFGen(sbr_info *psi, 
			SBRGrid *sbrGrid, 
			SBRFreq *sbrFreq, 
			SBRChan *sbrChan
            );

int ttHFAdj(AACDecoder* decoder, 
			SBRHeader *sbrHdr, 
			SBRGrid *sbrGrid, 
			SBRFreq *sbrFreq, 
			SBRChan *sbrChan, 
			int ch);

void ReSetSBRDate(sbr_info *psi);

int ttSBR_Envelope(AACDecoder* decoder,
				   BitStream *bs, 
				   SBRGrid *sbrGrid, 
				   SBRFreq *sbrFreq, 
				   SBRChan *sbrChan, 
				   int ch);

int ttSBR_Noise(AACDecoder* decoder,
				BitStream *bs, 
				SBRGrid *sbrGrid, 
				SBRFreq *sbrFreq, 
				SBRChan *sbrChan, 
				int ch
				);

int RMAACReadPSData(AACDecoder *decoder, 
			   BitStream *bs, 
			   sbr_info *sbr, 
			   int bitsLeft);

void UncoupleSBREnvelope(sbr_info *psi, 
						 SBRGrid *sbrGrid, 
						 SBRFreq *sbrFreq, 
						 SBRChan *sbrChanR
						 );

void UncoupleSBRNoise(sbr_info *psi, 
					  SBRGrid *sbrGrid, 
					  SBRFreq *sbrFreq, 
					  SBRChan *sbrChanR);

void Radix4_FFT(int *x);
int  ttInvRNormal(int r);
int  SqrtFix(int q, int fBitsIn, int *fBitsOut);

#endif	/* _SBR_H */
