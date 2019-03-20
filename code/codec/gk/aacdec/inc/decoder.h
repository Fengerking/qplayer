#ifndef _AACDECODER_H__
#define _AACDECODER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "struct.h"

/* private implementation-specific functions */

/* decframe.c */
int DecodeOneFrame(AACDecoder *decoder, TTInt16* outbuf);

/* unit.c*/
int updateSampleRate(AACDecoder *decoder,int sampleRate);
int updateProfile(AACDecoder *decoder,int profile);
int EnableDecodeCurrChannel(AACDecoder *decoder,int ch);
int EnableDecodeCurrSBRChannel(AACDecoder *decoder,int ch);
void UpdateSeletedChDecoded(AACDecoder *decoder,int channels);
void UpdateSeletedSBRChDecoded(AACDecoder *decoder,int channels);
int SignedDivide(AACDecoder* decoder,int divisor,int dividend);
unsigned int UnsignedDivide(AACDecoder* decoder,unsigned int divisor,unsigned int dividend);
int error(AACDecoder *decoder,char *text, int code);
void Channelconfig(AACDecoder *decoder);
int tns_analysis_filter(AACDecoder* decoder,ICS_Data *ics,TNS_Data* tns, int* spec);

int raw_data_block(AACDecoder* decoder);
int dequant_rescale(AACDecoder* decoder,int channels);
int mi_decode(AACDecoder* decoder,int channels);
int pns_decode(AACDecoder* decoder,int channels);
int ltp_decode(AACDecoder* decoder,int channels);
int tns_decode(AACDecoder* decoder,int channels);
int filter_bank(AACDecoder* decoder,short *outbuf);
int ltp_update(AACDecoder* decoder,int channels);
int ic_prediction(AACDecoder* decoder,int channels);

int ttSBRExtData(AACDecoder *decoder, int chBase);
int DecodeSBRData(AACDecoder *decoder, int chBase, short *outbuf);

/* Header.c */
int program_config_element(BitStream *bs,program_config* pce);

/* huffman.c */
int DecodeHuffmanScalar(const signed short *huffTab, const HuffInfo *huffTabInfo, unsigned int bitBuf, signed int *val);

/* downMatrix.c */
int DownMixto2Chs(AACDecoder* decoder,int chans,short* outbuf);
int Selectto2Chs(AACDecoder* decoder,int chans,short* outbuf);
int Stereo2Mono(AACDecoder* decoder,short *outbuf,int sampleSize);
int Mono2Stereo(AACDecoder* decoder,short *outbuf,int sampleSize);
int PostChannelProcess(AACDecoder* decoder,short *outbuf,int sampleSize);

/* Header.c */
int ParseADIFHeader(AACDecoder* decoder, BitStream *bs);
int ParseADTSHeader(AACDecoder* decoder);
int program_config_element(BitStream *bs,program_config* pce);

/* latmheader.c */
int ParserLatm(AACDecoder*	decoder);
int ReadMUXConfig(AACDecoder*	decoder, BitStream *bs);

/* lc_syntax.c */
int ltp_data(AACDecoder* decoder,ICS_Data *ics,LTP_Data* ltp);

/* lc_imdct.c */
void BitReverse(int *inout, int tabidx);
void ttIMDCT(int tabidx, int *coef, int gb);
void R4Core(int *x, int bg, int gp, int *wtab);
void ttRadix4FFT(int tabidx, int *x);

/* lc_mdct.c */
void MDCT(int tabidx, int *coef);

void *RMAACDecAlignedMalloc(int size);
void  RMAACDecAlignedFree(void *alignedPt);
#define SafeAlignedFree(a) {RMAACDecAlignedFree(a);(a)=NULL;}

int spectral_data(AACDecoder* decoder,ICS_Data *ics,int ch);
int sbr_init(AACDecoder* decoder);
void sbr_free(AACDecoder* decoder);
void tns_data(AACDecoder* decoder,BitStream *bs, int window_sequence, int ch);


#ifdef __cplusplus
}
#endif

#endif//DECODER
