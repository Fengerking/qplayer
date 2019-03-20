#ifndef __TTPOD_TT_AVCODEC_H_
#define __TTPOD_TT_AVCODEC_H_

#include <errno.h>
#include "config.h"
#include "ttAttributes.h"
#include "ttBuffer.h"
#include "ttFrame.h"
#include "ttRational.h"
#include "ttH264Log.h"

enum TTCodecID {
    TTV_CODEC_ID_NONE,

    /* video codecs */
    TTV_CODEC_ID_MPEG1VIDEO,
    TTV_CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
#if TT_API_XVMC
    TTV_CODEC_ID_MPEG2VIDEO_XVMC,
#endif /* TT_API_XVMC */
    TTV_CODEC_ID_H261,
    TTV_CODEC_ID_H263,
    TTV_CODEC_ID_RV10,
    TTV_CODEC_ID_RV20,
    TTV_CODEC_ID_MJPEG,
    TTV_CODEC_ID_MJPEGB,
    TTV_CODEC_ID_LJPEG,
    TTV_CODEC_ID_SP5X,
    TTV_CODEC_ID_JPEGLS,
    TTV_CODEC_ID_MPEG4,
    TTV_CODEC_ID_RAWVIDEO,
    TTV_CODEC_ID_MSMPEG4V1,
    TTV_CODEC_ID_MSMPEG4V2,
    TTV_CODEC_ID_MSMPEG4V3,
    TTV_CODEC_ID_WMV1,
    TTV_CODEC_ID_WMV2,
    TTV_CODEC_ID_H263P,
    TTV_CODEC_ID_H263I,
    TTV_CODEC_ID_FLV1,
    TTV_CODEC_ID_SVQ1,
    TTV_CODEC_ID_SVQ3,
    TTV_CODEC_ID_DVVIDEO,
    TTV_CODEC_ID_HUFFYUV,
    TTV_CODEC_ID_CYUV,
    TTV_CODEC_ID_H264,
    TTV_CODEC_ID_INDEO3,
    TTV_CODEC_ID_VP3,
    TTV_CODEC_ID_THEORA,
    TTV_CODEC_ID_ASV1,
    TTV_CODEC_ID_ASV2,
    TTV_CODEC_ID_FFV1,
    TTV_CODEC_ID_4XM,
    TTV_CODEC_ID_VCR1,
    TTV_CODEC_ID_CLJR,
    TTV_CODEC_ID_MDEC,
    TTV_CODEC_ID_ROQ,
    TTV_CODEC_ID_INTERPLAY_VIDEO,
    TTV_CODEC_ID_XAN_WC3,
    TTV_CODEC_ID_XAN_WC4,
    TTV_CODEC_ID_RPZA,
    TTV_CODEC_ID_CINEPAK,
    TTV_CODEC_ID_WS_VQA,
    TTV_CODEC_ID_MSRLE,
    TTV_CODEC_ID_MSVIDEO1,
    TTV_CODEC_ID_IDCIN,
    TTV_CODEC_ID_8BPS,
    TTV_CODEC_ID_SMC,
    TTV_CODEC_ID_FLIC,
    TTV_CODEC_ID_TRUEMOTION1,
    TTV_CODEC_ID_VMDVIDEO,
    TTV_CODEC_ID_MSZH,
    TTV_CODEC_ID_ZLIB,
    TTV_CODEC_ID_QTRLE,
    TTV_CODEC_ID_TSCC,
    TTV_CODEC_ID_ULTI,
    TTV_CODEC_ID_QDRAW,
    TTV_CODEC_ID_VIXL,
    TTV_CODEC_ID_QPEG,
    TTV_CODEC_ID_PNG,
    TTV_CODEC_ID_PPM,
    TTV_CODEC_ID_PBM,
    TTV_CODEC_ID_PGM,
    TTV_CODEC_ID_PGMYUV,
    TTV_CODEC_ID_PAM,
    TTV_CODEC_ID_FFVHUFF,
    TTV_CODEC_ID_RV30,
    TTV_CODEC_ID_RV40,
    TTV_CODEC_ID_VC1,
    TTV_CODEC_ID_WMV3,
    TTV_CODEC_ID_LOCO,
    TTV_CODEC_ID_WNV1,
    TTV_CODEC_ID_AASC,
    TTV_CODEC_ID_INDEO2,
    TTV_CODEC_ID_FRAPS,
    TTV_CODEC_ID_TRUEMOTION2,
    TTV_CODEC_ID_BMP,
    TTV_CODEC_ID_CSCD,
    TTV_CODEC_ID_MMVIDEO,
    TTV_CODEC_ID_ZMBV,
    TTV_CODEC_ID_AVS,
    TTV_CODEC_ID_SMACKVIDEO,
    TTV_CODEC_ID_NUV,
    TTV_CODEC_ID_KMVC,
    TTV_CODEC_ID_FLASHSV,
    TTV_CODEC_ID_CAVS,
    TTV_CODEC_ID_JPEG2000,
    TTV_CODEC_ID_VMNC,
    TTV_CODEC_ID_VP5,
    TTV_CODEC_ID_VP6,
    TTV_CODEC_ID_VP6F,
    TTV_CODEC_ID_TARGA,
    TTV_CODEC_ID_DSICINVIDEO,
    TTV_CODEC_ID_TIERTEXSEQVIDEO,
    TTV_CODEC_ID_TIFF,
    TTV_CODEC_ID_GIF,
    TTV_CODEC_ID_DXA,
    TTV_CODEC_ID_DNXHD,
    TTV_CODEC_ID_THP,
    TTV_CODEC_ID_SGI,
    TTV_CODEC_ID_C93,
    TTV_CODEC_ID_BETHSOFTVID,
    TTV_CODEC_ID_PTX,
    TTV_CODEC_ID_TXD,
    TTV_CODEC_ID_VP6A,
    TTV_CODEC_ID_AMV,
    TTV_CODEC_ID_VB,
    TTV_CODEC_ID_PCX,
    TTV_CODEC_ID_SUNRAST,
    TTV_CODEC_ID_INDEO4,
    TTV_CODEC_ID_INDEO5,
    TTV_CODEC_ID_MIMIC,
    TTV_CODEC_ID_RL2,
    TTV_CODEC_ID_ESCAPE124,
    TTV_CODEC_ID_DIRAC,
    TTV_CODEC_ID_BFI,
    TTV_CODEC_ID_CMV,
    TTV_CODEC_ID_MOTIONPIXELS,
    TTV_CODEC_ID_TGV,
    TTV_CODEC_ID_TGQ,
    TTV_CODEC_ID_TQI,
    TTV_CODEC_ID_AURA,
    TTV_CODEC_ID_AURA2,
    TTV_CODEC_ID_V210X,
    TTV_CODEC_ID_TMV,
    TTV_CODEC_ID_V210,
    TTV_CODEC_ID_DPX,
    TTV_CODEC_ID_MAD,
    TTV_CODEC_ID_FRWU,
    TTV_CODEC_ID_FLASHSV2,
    TTV_CODEC_ID_CDGRAPHICS,
    TTV_CODEC_ID_R210,
    TTV_CODEC_ID_ANM,
    TTV_CODEC_ID_BINKVIDEO,
    TTV_CODEC_ID_ITT_ILBM,
    TTV_CODEC_ID_ITT_BYTERUN1,
    TTV_CODEC_ID_KGV1,
    TTV_CODEC_ID_YOP,
    TTV_CODEC_ID_VP8,
    TTV_CODEC_ID_PICTOR,
    TTV_CODEC_ID_ANSI,
    TTV_CODEC_ID_A64_MULTI,
    TTV_CODEC_ID_A64_MULTI5,
    TTV_CODEC_ID_R10K,
    TTV_CODEC_ID_MXPEG,
    TTV_CODEC_ID_LAGARITH,
    TTV_CODEC_ID_PRORES,
    TTV_CODEC_ID_JV,
    TTV_CODEC_ID_DFA,
    TTV_CODEC_ID_WMV3IMAGE,
    TTV_CODEC_ID_VC1IMAGE,
    TTV_CODEC_ID_UTVIDEO,
    TTV_CODEC_ID_BMV_VIDEO,
    TTV_CODEC_ID_VBLE,
    TTV_CODEC_ID_DXTORY,
    TTV_CODEC_ID_V410,
    TTV_CODEC_ID_XWD,
    TTV_CODEC_ID_CDXL,
    TTV_CODEC_ID_XBM,
    TTV_CODEC_ID_ZEROCODEC,
    TTV_CODEC_ID_MSS1,
    TTV_CODEC_ID_MSA1,
    TTV_CODEC_ID_TSCC2,
    TTV_CODEC_ID_MTS2,
    TTV_CODEC_ID_CLLC,
    TTV_CODEC_ID_MSS2,
    TTV_CODEC_ID_VP9,
    TTV_CODEC_ID_AIC,
    TTV_CODEC_ID_ESCAPE130_DEPRECATED,
    TTV_CODEC_ID_G2M_DEPRECATED,
    TTV_CODEC_ID_WEBP_DEPRECATED,
    TTV_CODEC_ID_HNM4_VIDEO,
    TTV_CODEC_ID_HEVC_DEPRECATED,
    TTV_CODEC_ID_FIC,
    TTV_CODEC_ID_ALIAS_PIX,
    TTV_CODEC_ID_BRENDER_PIX_DEPRECATED,
    TTV_CODEC_ID_PAF_VIDEO_DEPRECATED,
    TTV_CODEC_ID_EXR_DEPRECATED,
    TTV_CODEC_ID_VP7_DEPRECATED,
    TTV_CODEC_ID_SANM_DEPRECATED,
    TTV_CODEC_ID_SGIRLE_DEPRECATED,
    TTV_CODEC_ID_MVC1_DEPRECATED,
    TTV_CODEC_ID_MVC2_DEPRECATED,

    TTV_CODEC_ID_BRENDER_PIX= MKBETAG('B','P','I','X'),
    TTV_CODEC_ID_Y41P       = MKBETAG('Y','4','1','P'),
    TTV_CODEC_ID_ESCAPE130  = MKBETAG('E','1','3','0'),
    TTV_CODEC_ID_EXR        = MKBETAG('0','E','X','R'),
    TTV_CODEC_ID_AVRP       = MKBETAG('A','V','R','P'),

    TTV_CODEC_ID_012V       = MKBETAG('0','1','2','V'),
    TTV_CODEC_ID_G2M        = MKBETAG( 0 ,'G','2','M'),
    TTV_CODEC_ID_AVUI       = MKBETAG('A','V','U','I'),
    TTV_CODEC_ID_AYUV       = MKBETAG('A','Y','U','V'),
    TTV_CODEC_ID_TARGA_Y216 = MKBETAG('T','2','1','6'),
    TTV_CODEC_ID_V308       = MKBETAG('V','3','0','8'),
    TTV_CODEC_ID_V408       = MKBETAG('V','4','0','8'),
    TTV_CODEC_ID_YUV4       = MKBETAG('Y','U','V','4'),
    TTV_CODEC_ID_SANM       = MKBETAG('S','A','N','M'),
    TTV_CODEC_ID_PAF_VIDEO  = MKBETAG('P','A','F','V'),
    TTV_CODEC_ID_AVRN       = MKBETAG('A','V','R','n'),
    TTV_CODEC_ID_CPIA       = MKBETAG('C','P','I','A'),
    TTV_CODEC_ID_XFACE      = MKBETAG('X','F','A','C'),
    TTV_CODEC_ID_SGIRLE     = MKBETAG('S','G','I','R'),
    TTV_CODEC_ID_MVC1       = MKBETAG('M','V','C','1'),
    TTV_CODEC_ID_MVC2       = MKBETAG('M','V','C','2'),
    TTV_CODEC_ID_SNOW       = MKBETAG('S','N','O','W'),
    TTV_CODEC_ID_WEBP       = MKBETAG('W','E','B','P'),
    TTV_CODEC_ID_SMVJPEG    = MKBETAG('S','M','V','J'),
    TTV_CODEC_ID_HEVC       = MKBETAG('H','2','6','5'),
#define TTV_CODEC_ID_H265 TTV_CODEC_ID_HEVC
    TTV_CODEC_ID_VP7        = MKBETAG('V','P','7','0'),

    /* various PCM "codecs" */
    TTV_CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
    TTV_CODEC_ID_PCM_S16LE = 0x10000,
    TTV_CODEC_ID_PCM_S16BE,
    TTV_CODEC_ID_PCM_U16LE,
    TTV_CODEC_ID_PCM_U16BE,
    TTV_CODEC_ID_PCM_S8,
    TTV_CODEC_ID_PCM_U8,
    TTV_CODEC_ID_PCM_MULAW,
    TTV_CODEC_ID_PCM_ALAW,
    TTV_CODEC_ID_PCM_S32LE,
    TTV_CODEC_ID_PCM_S32BE,
    TTV_CODEC_ID_PCM_U32LE,
    TTV_CODEC_ID_PCM_U32BE,
    TTV_CODEC_ID_PCM_S24LE,
    TTV_CODEC_ID_PCM_S24BE,
    TTV_CODEC_ID_PCM_U24LE,
    TTV_CODEC_ID_PCM_U24BE,
    TTV_CODEC_ID_PCM_S24DAUD,
    TTV_CODEC_ID_PCM_ZORK,
    TTV_CODEC_ID_PCM_S16LE_PLANAR,
    TTV_CODEC_ID_PCM_DVD,
    TTV_CODEC_ID_PCM_F32BE,
    TTV_CODEC_ID_PCM_F32LE,
    TTV_CODEC_ID_PCM_F64BE,
    TTV_CODEC_ID_PCM_F64LE,
    TTV_CODEC_ID_PCM_BLURAY,
    TTV_CODEC_ID_PCM_LXF,
    TTV_CODEC_ID_S302M,
    TTV_CODEC_ID_PCM_S8_PLANAR,
    TTV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED,
    TTV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED,
    TTV_CODEC_ID_PCM_S24LE_PLANAR = MKBETAG(24,'P','S','P'),
    TTV_CODEC_ID_PCM_S32LE_PLANAR = MKBETAG(32,'P','S','P'),
    TTV_CODEC_ID_PCM_S16BE_PLANAR = MKBETAG('P','S','P',16),

    /* various ADPCM codecs */
    TTV_CODEC_ID_ADPCM_IMA_QT = 0x11000,
    TTV_CODEC_ID_ADPCM_IMA_WAV,
    TTV_CODEC_ID_ADPCM_IMA_DK3,
    TTV_CODEC_ID_ADPCM_IMA_DK4,
    TTV_CODEC_ID_ADPCM_IMA_WS,
    TTV_CODEC_ID_ADPCM_IMA_SMJPEG,
    TTV_CODEC_ID_ADPCM_MS,
    TTV_CODEC_ID_ADPCM_4XM,
    TTV_CODEC_ID_ADPCM_XA,
    TTV_CODEC_ID_ADPCM_ADX,
    TTV_CODEC_ID_ADPCM_EA,
    TTV_CODEC_ID_ADPCM_G726,
    TTV_CODEC_ID_ADPCM_CT,
    TTV_CODEC_ID_ADPCM_SWF,
    TTV_CODEC_ID_ADPCM_YAMAHA,
    TTV_CODEC_ID_ADPCM_SBPRO_4,
    TTV_CODEC_ID_ADPCM_SBPRO_3,
    TTV_CODEC_ID_ADPCM_SBPRO_2,
    TTV_CODEC_ID_ADPCM_THP,
    TTV_CODEC_ID_ADPCM_IMA_AMV,
    TTV_CODEC_ID_ADPCM_EA_R1,
    TTV_CODEC_ID_ADPCM_EA_R3,
    TTV_CODEC_ID_ADPCM_EA_R2,
    TTV_CODEC_ID_ADPCM_IMA_EA_SEAD,
    TTV_CODEC_ID_ADPCM_IMA_EA_EACS,
    TTV_CODEC_ID_ADPCM_EA_XAS,
    TTV_CODEC_ID_ADPCM_EA_MAXIS_XA,
    TTV_CODEC_ID_ADPCM_IMA_ISS,
    TTV_CODEC_ID_ADPCM_G722,
    TTV_CODEC_ID_ADPCM_IMA_APC,
    TTV_CODEC_ID_ADPCM_VIMA_DEPRECATED,
    TTV_CODEC_ID_ADPCM_VIMA = MKBETAG('V','I','M','A'),
    TTV_CODEC_ID_VIMA       = MKBETAG('V','I','M','A'),
    TTV_CODEC_ID_ADPCM_AFC  = MKBETAG('A','F','C',' '),
    TTV_CODEC_ID_ADPCM_IMA_OKI = MKBETAG('O','K','I',' '),
    TTV_CODEC_ID_ADPCM_DTK  = MKBETAG('D','T','K',' '),
    TTV_CODEC_ID_ADPCM_IMA_RAD = MKBETAG('R','A','D',' '),
    TTV_CODEC_ID_ADPCM_G726LE = MKBETAG('6','2','7','G'),

    /* AMR */
    TTV_CODEC_ID_AMR_NB = 0x12000,
    TTV_CODEC_ID_AMR_WB,

    /* RealAudio codecs*/
    TTV_CODEC_ID_RA_144 = 0x13000,
    TTV_CODEC_ID_RA_288,

    /* various DPCM codecs */
    TTV_CODEC_ID_ROQ_DPCM = 0x14000,
    TTV_CODEC_ID_INTERPLAY_DPCM,
    TTV_CODEC_ID_XAN_DPCM,
    TTV_CODEC_ID_SOL_DPCM,

    /* audio codecs */
    TTV_CODEC_ID_MP2 = 0x15000,
    TTV_CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    TTV_CODEC_ID_AAC,
    TTV_CODEC_ID_AC3,
    TTV_CODEC_ID_DTS,
    TTV_CODEC_ID_VORBIS,
    TTV_CODEC_ID_DVAUDIO,
    TTV_CODEC_ID_WMAV1,
    TTV_CODEC_ID_WMAV2,
    TTV_CODEC_ID_MACE3,
    TTV_CODEC_ID_MACE6,
    TTV_CODEC_ID_VMDAUDIO,
    TTV_CODEC_ID_FLAC,
    TTV_CODEC_ID_MP3ADU,
    TTV_CODEC_ID_MP3ON4,
    TTV_CODEC_ID_SHORTEN,
    TTV_CODEC_ID_ALAC,
    TTV_CODEC_ID_WESTWOOD_SND1,
    TTV_CODEC_ID_GSM, ///< as in Berlin toast format
    TTV_CODEC_ID_QDM2,
    TTV_CODEC_ID_COOK,
    TTV_CODEC_ID_TRUESPEECH,
    TTV_CODEC_ID_TTA,
    TTV_CODEC_ID_SMACKAUDIO,
    TTV_CODEC_ID_QCELP,
    TTV_CODEC_ID_WAVPACK,
    TTV_CODEC_ID_DSICINAUDIO,
    TTV_CODEC_ID_IMC,
    TTV_CODEC_ID_MUSEPACK7,
    TTV_CODEC_ID_MLP,
    TTV_CODEC_ID_GSM_MS, /* as found in WAV */
    TTV_CODEC_ID_ATRAC3,
#if TT_API_VOXWARE
    TTV_CODEC_ID_VOXWARE,
#endif
    TTV_CODEC_ID_APE,
    TTV_CODEC_ID_NELLYMOSER,
    TTV_CODEC_ID_MUSEPACK8,
    TTV_CODEC_ID_SPEEX,
    TTV_CODEC_ID_WMAVOICE,
    TTV_CODEC_ID_WMAPRO,
    TTV_CODEC_ID_WMALOSSLESS,
    TTV_CODEC_ID_ATRAC3P,
    TTV_CODEC_ID_EAC3,
    TTV_CODEC_ID_SIPR,
    TTV_CODEC_ID_MP1,
    TTV_CODEC_ID_TWINVQ,
    TTV_CODEC_ID_TRUEHD,
    TTV_CODEC_ID_MP4ALS,
    TTV_CODEC_ID_ATRAC1,
    TTV_CODEC_ID_BINKAUDIO_RDFT,
    TTV_CODEC_ID_BINKAUDIO_DCT,
    TTV_CODEC_ID_AAC_LATM,
    TTV_CODEC_ID_QDMC,
    TTV_CODEC_ID_CELT,
    TTV_CODEC_ID_G723_1,
    TTV_CODEC_ID_G729,
    TTV_CODEC_ID_8SVX_EXP,
    TTV_CODEC_ID_8SVX_FIB,
    TTV_CODEC_ID_BMV_AUDIO,
    TTV_CODEC_ID_RALF,
    TTV_CODEC_ID_IAC,
    TTV_CODEC_ID_ILBC,
    TTV_CODEC_ID_OPUS_DEPRECATED,
    TTV_CODEC_ID_COMFORT_NOISE,
    TTV_CODEC_ID_TAK_DEPRECATED,
    TTV_CODEC_ID_METASOUND,
    TTV_CODEC_ID_PAF_AUDIO_DEPRECATED,
    TTV_CODEC_ID_ON2AVC,
    TTV_CODEC_ID_FFWAVESYNTH = MKBETAG('F','F','W','S'),
    TTV_CODEC_ID_SONIC       = MKBETAG('S','O','N','C'),
    TTV_CODEC_ID_SONIC_LS    = MKBETAG('S','O','N','L'),
    TTV_CODEC_ID_PAF_AUDIO   = MKBETAG('P','A','F','A'),
    TTV_CODEC_ID_OPUS        = MKBETAG('O','P','U','S'),
    TTV_CODEC_ID_TAK         = MKBETAG('t','B','a','K'),
    TTV_CODEC_ID_EVRC        = MKBETAG('s','e','v','c'),
    TTV_CODEC_ID_SMV         = MKBETAG('s','s','m','v'),
    TTV_CODEC_ID_DSD_LSBF    = MKBETAG('D','S','D','L'),
    TTV_CODEC_ID_DSD_MSBF    = MKBETAG('D','S','D','M'),
    TTV_CODEC_ID_DSD_LSBF_PLANAR = MKBETAG('D','S','D','1'),
    TTV_CODEC_ID_DSD_MSBF_PLANAR = MKBETAG('D','S','D','8'),

    /* subtitle codecs */
    TTV_CODEC_ID_FIRST_SUBTITLE = 0x17000,          ///< A dummy ID pointing at the start of subtitle codecs.
    TTV_CODEC_ID_DVD_SUBTITLE = 0x17000,
    TTV_CODEC_ID_DVB_SUBTITLE,
    TTV_CODEC_ID_TEXT,  ///< raw UTF-8 text
    TTV_CODEC_ID_XSUB,
    TTV_CODEC_ID_SSA,
    TTV_CODEC_ID_MOV_TEXT,
    TTV_CODEC_ID_HDMV_PGS_SUBTITLE,
    TTV_CODEC_ID_DVB_TELETEXT,
    TTV_CODEC_ID_SRT,
    TTV_CODEC_ID_MICRODVD   = MKBETAG('m','D','V','D'),
    TTV_CODEC_ID_EIA_608    = MKBETAG('c','6','0','8'),
    TTV_CODEC_ID_JACOSUB    = MKBETAG('J','S','U','B'),
    TTV_CODEC_ID_SAMI       = MKBETAG('S','A','M','I'),
    TTV_CODEC_ID_REALTEXT   = MKBETAG('R','T','X','T'),
    TTV_CODEC_ID_STL        = MKBETAG('S','p','T','L'),
    TTV_CODEC_ID_SUBVIEWER1 = MKBETAG('S','b','V','1'),
    TTV_CODEC_ID_SUBVIEWER  = MKBETAG('S','u','b','V'),
    TTV_CODEC_ID_SUBRIP     = MKBETAG('S','R','i','p'),
    TTV_CODEC_ID_WEBVTT     = MKBETAG('W','V','T','T'),
    TTV_CODEC_ID_MPL2       = MKBETAG('M','P','L','2'),
    TTV_CODEC_ID_VPLAYER    = MKBETAG('V','P','l','r'),
    TTV_CODEC_ID_PJS        = MKBETAG('P','h','J','S'),
    TTV_CODEC_ID_ASS        = MKBETAG('A','S','S',' '),  ///< ASS as defined in Matroska

    /* other specific kind of codecs (generally used for attachments) */
    TTV_CODEC_ID_FIRST_UNKNOWN = 0x18000,           ///< A dummy ID pointing at the start of various fake codecs.
    TTV_CODEC_ID_TTF = 0x18000,
    TTV_CODEC_ID_BINTEXT    = MKBETAG('B','T','X','T'),
    TTV_CODEC_ID_XBIN       = MKBETAG('X','B','I','N'),
    TTV_CODEC_ID_IDF        = MKBETAG( 0 ,'I','D','F'),
    TTV_CODEC_ID_OTF        = MKBETAG( 0 ,'O','T','F'),
    TTV_CODEC_ID_SMPTE_KLV  = MKBETAG('K','L','V','A'),
    TTV_CODEC_ID_DVD_NAV    = MKBETAG('D','N','A','V'),
    TTV_CODEC_ID_TIMED_ID3  = MKBETAG('T','I','D','3'),
    TTV_CODEC_ID_BIN_DATA   = MKBETAG('D','A','T','A'),


    TTV_CODEC_ID_PROBE = 0x19000, ///< codec_id is not known (like TTV_CODEC_ID_NONE) but lavf should attempt to identify it

    TTV_CODEC_ID_MPEG2TS = 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
    TTV_CODEC_ID_MPEG4SYSTEMS = 0x20001, /**< _FAKE_ codec to indicate a MPEG-4 Systems
                                * stream (only used by libavformat) */
    TTV_CODEC_ID_FFMETADATA = 0x21000,   ///< Dummy codec for streams containing only metadata information.

// #if TT_API_CODEC_ID
// #include "old_codec_ids.h"
// #endif
};


typedef struct TTCodecDescriptor {
    enum TTCodecID     id;
    enum AVMediaType type;
    const char      *name;
    const char *long_name;
    int             props;
	const char *const mime_types[5];
} TTCodecDescriptor;

#define TTV_CODEC_PROP_INTRA_ONLY    (1 << 0)
#define TTV_CODEC_PROP_LOSSY         (1 << 1)
#define TTV_CODEC_PROP_LOSSLESS      (1 << 2)
#define TTV_CODEC_PROP_REORDER       (1 << 3)
#define TTV_CODEC_PROP_BITMAP_SUB    (1 << 16)
#define TTV_CODEC_PROP_TEXT_SUB      (1 << 17)
#define TT_INPUT_BUFFER_PADDING_SIZE 32
#define TT_MIN_BUFFER_SIZE 16384

enum Motion_Est_ID {
    ME_ZERO = 1,    ///< no search, that is use 0,0 vector whenever one is needed
    ME_FULL,
    ME_LOG,
    ME_PHODS,
    ME_EPZS,        ///< enhanced predictive zonal search
    ME_X1,          ///< reserved for experiments
    ME_HEX,         ///< hexagon based search
    ME_UMH,         ///< uneven multi-hexagon search
    ME_TESA,        ///< transformed exhaustive search algorithm
    ME_ITER=50,     ///< iterative search
};

enum TTDiscard{
    AVDISCARD_NONE    =-16, ///< discard nothing
    AVDISCARD_DEFAULT =  0, ///< discard useless packets like 0 size packets in avi
    AVDISCARD_NONREF  =  8, ///< discard all non reference
    AVDISCARD_BIDIR   = 16, ///< discard all bidirectional frames
    AVDISCARD_NONINTRA= 24, ///< discard all non intra frames
    AVDISCARD_NONKEY  = 32, ///< discard all frames except keyframes
    AVDISCARD_ALL     = 48, ///< discard all
};

enum TTAudioServiceType {
    TTV_AUDIO_SERVICE_TYPE_MAIN              = 0,
    TTV_AUDIO_SERVICE_TYPE_EFFECTS           = 1,
    TTV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED = 2,
    TTV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED  = 3,
    TTV_AUDIO_SERVICE_TYPE_DIALOGUE          = 4,
    TTV_AUDIO_SERVICE_TYPE_COMMENTARY        = 5,
    TTV_AUDIO_SERVICE_TYPE_EMERGENCY         = 6,
    TTV_AUDIO_SERVICE_TYPE_VOICE_OVER        = 7,
    TTV_AUDIO_SERVICE_TYPE_KARAOKE           = 8,
    TTV_AUDIO_SERVICE_TYPE_NB                   , ///< Not part of ABI
};

typedef struct RcOverride{
    int start_frame;
    int end_frame;
    int qscale; // If this is 0 then quality_factor will be used instead.
    float quality_factor;
} RcOverride;


#define CODEC_FLAG_UNALIGNED 0x0001
#define CODEC_FLAG_QSCALE 0x0002  ///< Use fixed qscale.
#define CODEC_FLAG_4MV    0x0004  ///< 4 MV per MB allowed / advanced prediction for H.263.
#define CODEC_FLAG_OUTPUT_CORRUPT 0x0008 ///< Output even those frames that might be corrupted
#define CODEC_FLAG_QPEL   0x0010  ///< Use qpel MC.
#if TT_API_GMC
#define CODEC_FLAG_GMC    0x0020  ///< Use GMC.
#endif
#if TT_API_MV0
#define CODEC_FLAG_MV0    0x0040
#endif
#if TT_API_INPUT_PRESERVED
#define CODEC_FLAG_INPUT_PRESERVED 0x0100
#endif
#define CODEC_FLAG_PASS1           0x0200   ///< Use internal 2pass ratecontrol in first pass mode.
#define CODEC_FLAG_PASS2           0x0400   ///< Use internal 2pass ratecontrol in second pass mode.
#define CODEC_FLAG_GRAY            0x2000   ///< Only decode/encode grayscale.
#if TT_API_EMU_EDGE
/**
 * @deprecated edges are not used/required anymore. I.e. this flag is now always
 * set.
 */
#define CODEC_FLAG_EMU_EDGE        0x4000
#endif
#define CODEC_FLAG_PSNR            0x8000   ///< error[?] variables will be set during encoding.
#define CODEC_FLAG_TRUNCATED       0x00010000 /** Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
#if TT_API_NORMALIZE_AQP
/**
 * @deprecated use the flag "naq" in the "mpv_flags" private option of the
 * mpegvideo encoders
 */
#define CODEC_FLAG_NORMALIZE_AQP  0x00020000
#endif
#define CODEC_FLAG_INTERLACED_DCT 0x00040000 ///< Use interlaced DCT.
#define CODEC_FLAG_LOW_DELAY      0x00080000 ///< Force low delay.
#define CODEC_FLAG_GLOBAL_HEADER  0x00400000 ///< Place global headers in extradata instead of every keyframe.
#define CODEC_FLAG_BITEXACT       0x00800000 ///< Use only bitexact stuff (except (I)DCT).
/* Fx : Flag for h263+ extra options */
#define CODEC_FLAG_AC_PRED        0x01000000 ///< H.263 advanced intra coding / MPEG-4 AC prediction
#define CODEC_FLAG_LOOP_FILTER    0x00000800 ///< loop filter
#define CODEC_FLAG_INTERLACED_ME  0x20000000 ///< interlaced motion estimation
#define CODEC_FLAG_CLOSED_GOP     0x80000000
#define CODEC_FLAG2_FAST          0x00000001 ///< Allow non spec compliant speedup tricks.
#define CODEC_FLAG2_NO_OUTPUT     0x00000004 ///< Skip bitstream encoding.
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008 ///< Place global headers at every keyframe instead of in extradata.
#define CODEC_FLAG2_DROP_FRAME_TIMECODE 0x00002000 ///< timecode is in drop frame format. DEPRECATED!!!!
#define CODEC_FLAG2_IGNORE_CROP   0x00010000 ///< Discard cropping information from SPS.

#define CODEC_FLAG2_CHUNKS        0x00008000 ///< Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.
#define CODEC_FLAG2_SHOW_ALL      0x00400000 ///< Show all frames before the first keyframe
#define CODEC_FLAG2_EXPORT_MVS    0x10000000 ///< Export motion vectors through frame side data
#define CODEC_FLAG2_SKIP_MANUAL   0x20000000 ///< Do not skip samples and export skip information as frame side data

/* Unsupported options :
 *              Syntax Arithmetic coding (SAC)
 *              Reference Picture Selection
 *              Independent Segment Decoding */
/* /Fx */
/* codec capabilities */

#define CODEC_CAP_DRAW_HORIZ_BAND 0x0001 ///< Decoder can use draw_horiz_band callback.
/**
 * Codec uses get_buffer() for allocating buffers and supports custom allocators.
 * If not set, it might not use get_buffer() at all or use operations that
 * assume the buffer was allocated by ttcodec_default_get_buffer.
 */
#define CODEC_CAP_DR1             0x0002
#define CODEC_CAP_TRUNCATED       0x0008
#if TT_API_XVMC
/* Codec can export data for HW decoding. This flag indicates that
 * the codec would call get_format() with list that might contain HW accelerated
 * pixel formats (XvMC, VDPAU, VAAPI, etc). The application can pick any of them
 * including raw image format.
 * The application can use the passed context to determine bitstream version,
 * chroma format, resolution etc.
 */
#define CODEC_CAP_HWACCEL         0x0010
#endif /* TT_API_XVMC */
/**
 * Encoder or decoder requires flushing with NULL input at the end in order to
 * give the complete and correct output.
 *
 * NOTE: If this flag is not set, the codec is guaranteed to never be fed with
 *       with NULL data. The user can still send NULL data to the public encode
 *       or decode function, but libavcodec will not pass it along to the codec
 *       unless this flag is set.
 *
 * Decoders:
 * The decoder has a non-zero delay and needs to be fed with avpkt->data=NULL,
 * avpkt->size=0 at the end to get the delayed data until the decoder no longer
 * returns frames.
 *
 * Encoders:
 * The encoder needs to be fed with NULL data at the end of encoding until the
 * encoder no longer returns data.
 *
 * NOTE: For encoders implementing the TTCodec.encode2() function, setting this
 *       flag also means that the encoder must set the pts and duration for
 *       each output packet. If this flag is not set, the pts and duration will
 *       be determined by libavcodec from the input frame.
 */
#define CODEC_CAP_DELAY           0x0020
/**
 * Codec can be fed a final frame with a smaller size.
 * This can be used to prevent truncation of the last audio samples.
 */
#define CODEC_CAP_SMALL_LAST_FRAME 0x0040
#if TT_API_CAP_VDPAU
/**
 * Codec can export data for HW decoding (VDPAU).
 */
#define CODEC_CAP_HWACCEL_VDPAU    0x0080
#endif
/**
 * Codec can output multiple frames per TTPacket
 * Normally demuxers return one frame at a time, demuxers which do not do
 * are connected to a parser to split what they return into proper frames.
 * This flag is reserved to the very rare category of codecs which have a
 * bitstream that cannot be split into frames without timeconsuming
 * operations like full decoding. Demuxers carring such bitstreams thus
 * may return multiple frames in a packet. This has many disadvantages like
 * prohibiting stream copy in many cases thus it should only be considered
 * as a last resort.
 */
#define CODEC_CAP_SUBFRAMES        0x0100
/**
 * Codec is experimental and is thus avoided in favor of non experimental
 * encoders
 */
#define CODEC_CAP_EXPERIMENTAL     0x0200
/**
 * Codec should fill in channel configuration and samplerate instead of container
 */
#define CODEC_CAP_CHANNEL_CONF     0x0400
#if TT_API_NEG_LINESIZES
/**
 * @deprecated no codecs use this capability
 */
#define CODEC_CAP_NEG_LINESIZES    0x0800
#endif
/**
 * Codec supports frame-level multithreading.
 */
#define CODEC_CAP_FRAME_THREADS    0x1000
/**
 * Codec supports slice-based (or partition-based) multithreading.
 */
#define CODEC_CAP_SLICE_THREADS    0x2000
/**
 * Codec supports changed parameters at any point.
 */
#define CODEC_CAP_PARAM_CHANGE     0x4000
/**
 * Codec supports avctx->thread_count == 0 (auto).
 */
#define CODEC_CAP_AUTO_THREADS     0x8000
/**
 * Audio encoder supports receiving a different number of samples in each call.
 */
#define CODEC_CAP_VARIABLE_FRAME_SIZE 0x10000
/**
 * Codec is intra only.
 */
#define CODEC_CAP_INTRA_ONLY       0x40000000
/**
 * Codec is lossless.
 */
#define CODEC_CAP_LOSSLESS         0x80000000

#if TT_API_MB_TYPE
//The following defines may change, don't expect compatibility if you use them.
#define MB_TYPE_INTRA4x4   0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME H.264-specific
#define MB_TYPE_INTRA_PCM  0x0004 //FIXME H.264-specific
#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040
#define MB_TYPE_INTERLACED 0x0080
#define MB_TYPE_DIRECT2    0x0100 //FIXME
#define MB_TYPE_ACPRED     0x0200
#define MB_TYPE_GMC        0x0400
#define MB_TYPE_SKIP       0x0800
#define MB_TYPE_P0L0       0x1000
#define MB_TYPE_P1L0       0x2000
#define MB_TYPE_P0L1       0x4000
#define MB_TYPE_P1L1       0x8000
#define MB_TYPE_L0         (MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1         (MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1       (MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT      0x00010000
#define MB_TYPE_CBP        0x00020000
//Note bits 24-31 are reserved for codec specific use (h264 ref0, mpeg1 0mv, ...)
#endif



#if TT_API_QSCALE_TYPE
#define TT_QSCALE_TYPE_MPEG1 0
#define TT_QSCALE_TYPE_MPEG2 1
#define TT_QSCALE_TYPE_H264  2
#define TT_QSCALE_TYPE_VP56  3
#endif

#if TT_API_GET_BUFFER
#define TT_BUFFER_TYPE_INTERNAL 1
#define TT_BUFFER_TYPE_USER     2 ///< direct rendering buffers (image is (de)allocated by user)
#define TT_BUFFER_TYPE_SHARED   4 ///< Buffer from somewhere else; don't deallocate image (data/base), all other tables are not shared.
#define TT_BUFFER_TYPE_COPY     8 ///< Just a (modified) copy of some other buffer, don't deallocate anything.

#define TT_BUFFER_HINTS_VALID    0x01 // Buffer hints value is meaningful (if 0 ignore).
#define TT_BUFFER_HINTS_READABLE 0x02 // Codec will read from buffer.
#define TT_BUFFER_HINTS_PRESERVE 0x04 // User must not alter buffer content.
#define TT_BUFFER_HINTS_REUSABLE 0x08 // Codec will reuse the buffer (update).
#endif

/**
 * The decoder will keep a reference to the frame and may reuse it later.
 */
#define TTV_GET_BUFFER_FLAG_REF (1 << 0)

/**
 * @defgroup lavc_packet TTPacket
 *
 * Types and functions for working with TTPacket.
 * @{
 */
enum TTPacketSideDataType {
    TTV_PKT_DATA_PALETTE,
    TTV_PKT_DATA_NEW_EXTRADATA,

    /**
     * An TTV_PKT_DATA_PARAM_CHANGE side data packet is laid out as follows:
     * @code
     * u32le param_flags
     * if (param_flags & TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT)
     *     s32le channel_count
     * if (param_flags & TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT)
     *     u64le channel_layout
     * if (param_flags & TTV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE)
     *     s32le sample_rate
     * if (param_flags & TTV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS)
     *     s32le width
     *     s32le height
     * @endcode
     */
    TTV_PKT_DATA_PARAM_CHANGE,

    /**
     * An TTV_PKT_DATA_H263_MB_INFO side data packet contains a number of
     * structures with info about macroblocks relevant to splitting the
     * packet into smaller packets on macroblock edges (e.g. as for RFC 2190).
     * That is, it does not necessarily contain info about all macroblocks,
     * as long as the distance between macroblocks in the info is smaller
     * than the target payload size.
     * Each MB info structure is 12 bytes, and is laid out as follows:
     * @code
     * u32le bit offset from the start of the packet
     * u8    current quantizer at the start of the macroblock
     * u8    GOB number
     * u16le macroblock address within the GOB
     * u8    horizontal MV predictor
     * u8    vertical MV predictor
     * u8    horizontal MV predictor for block number 3
     * u8    vertical MV predictor for block number 3
     * @endcode
     */
    TTV_PKT_DATA_H263_MB_INFO,

    /**
     * This side data should be associated with an audio stream and contains
     * ReplayGain information in form of the AVReplayGain struct.
     */
    TTV_PKT_DATA_REPLAYGAIN,

    /**
     * This side data contains a 3x3 transformation matrix describing an affine
     * transformation that needs to be applied to the decoded video frames for
     * correct presentation.
     *
     * See ttDisplay.h for a detailed description of the data.
     */
    TTV_PKT_DATA_DISPLAYMATRIX,

    /**
     * This side data should be associated with a video stream and contains
     * Stereoscopic 3D information in form of the AVStereo3D struct.
     */
    TTV_PKT_DATA_STEREO3D,

    /**
     * Recommmends skipping the specified number of samples
     * @code
     * u32le number of samples to skip from start of this packet
     * u32le number of samples to skip from end of this packet
     * u8    reason for start skip
     * u8    reason for end   skip (0=padding silence, 1=convergence)
     * @endcode
     */
    TTV_PKT_DATA_SKIP_SAMPLES=70,

    /**
     * An TTV_PKT_DATA_JP_DUALMONO side data packet indicates that
     * the packet may contain "dual mono" audio specific to Japanese DTV
     * and if it is true, recommends only the selected channel to be used.
     * @code
     * u8    selected channels (0=mail/left, 1=sub/right, 2=both)
     * @endcode
     */
    TTV_PKT_DATA_JP_DUALMONO,

    /**
     * A list of zero terminated key/value strings. There is no end marker for
     * the list, so it is required to rely on the side data size to stop.
     */
    TTV_PKT_DATA_STRINGS_METADATA,

    /**
     * Subtitle event position
     * @code
     * u32le x1
     * u32le y1
     * u32le x2
     * u32le y2
     * @endcode
     */
    TTV_PKT_DATA_SUBTITLE_POSITION,

    /**
     * Data found in BlockAdditional element of matroska container. There is
     * no end marker for the data, so it is required to rely on the side data
     * size to recognize the end. 8 byte id (as found in BlockAddId) followed
     * by data.
     */
    TTV_PKT_DATA_MATROSKA_BLOCKADDITIONAL,

    /**
     * The optional first identifier line of a WebVTT cue.
     */
    TTV_PKT_DATA_WEBVTT_IDENTIFIER,

    /**
     * The optional settings (rendering instructions) that immediately
     * follow the timestamp specifier of a WebVTT cue.
     */
    TTV_PKT_DATA_WEBVTT_SETTINGS,

    /**
     * A list of zero terminated key/value strings. There is no end marker for
     * the list, so it is required to rely on the side data size to stop. This
     * side data includes updated metadata which appeared in the stream.
     */
    TTV_PKT_DATA_METADATA_UPDATE,
};

typedef struct TTPacketSideData {
    uint8_t *data;
    int      size;
    enum TTPacketSideDataType type;
} TTPacketSideData;

/**
 * This structure stores compressed data. It is typically exported by demuxers
 * and then passed as input to decoders, or received as output from encoders and
 * then passed to muxers.
 *
 * For video, it should typically contain one compressed frame. For audio it may
 * contain several compressed frames.
 *
 * TTPacket is one of the few structs in FFmpeg, whose size is a part of public
 * ABI. Thus it may be allocated on stack and no new fields can be added to it
 * without libavcodec and libavformat major bump.
 *
 * The semantics of data ownership depends on the buf or destruct (deprecated)
 * fields. If either is set, the packet data is dynamically allocated and is
 * valid indefinitely until tt_free_packet() is called (which in turn calls
 * ttv_buffer_unref()/the destruct callback to free the data). If neither is set,
 * the packet data is typically backed by some static buffer somewhere and is
 * only valid for a limited time (e.g. until the next read call when demuxing).
 *
 * The side data is always allocated with ttv_malloc() and is freed in
 * tt_free_packet().
 */
typedef struct TTPacket {
    /**
     * A reference to the reference-counted buffer where the packet data is
     * stored.
     * May be NULL, then the packet data is not reference-counted.
     */
    TTBufferRef *buf;
    /**
     * Presentation timestamp in AVStream->time_base units; the time at which
     * the decompressed packet will be presented to the user.
     * Can be TTV_NOPTS_VALUE if it is not stored in the file.
     * pts MUST be larger or equal to dts as presentation cannot happen before
     * decompression, unless one wants to view hex dumps. Some formats misuse
     * the terms dts and pts/cts to mean something different. Such timestamps
     * must be converted to true pts/dts before they are stored in TTPacket.
     */
    int64_t pts;
    /**
     * Decompression timestamp in AVStream->time_base units; the time at which
     * the packet is decompressed.
     * Can be TTV_NOPTS_VALUE if it is not stored in the file.
     */
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    /**
     * A combination of TTV_PKT_FLAG values
     */
    int   flags;
    /**
     * Additional packet data that can be provided by the container.
     * Packet can contain several types of side information.
     */
    TTPacketSideData *side_data;
    int side_data_elems;

    /**
     * Duration of this packet in AVStream->time_base units, 0 if unknown.
     * Equals next_pts - this_pts in presentation order.
     */
    int   duration;
#if TT_API_DESTRUCT_PACKET
    attribute_deprecated
    void  (*destruct)(struct TTPacket *);
    attribute_deprecated
    void  *priv;
#endif
    int64_t pos;                            ///< byte position in stream, -1 if unknown

    /**
     * Time difference in AVStream->time_base units from the pts of this
     * packet to the point at which the output from the decoder has converged
     * independent from the availability of previous frames. That is, the
     * frames are virtually identical no matter if decoding started from
     * the very first frame or from this keyframe.
     * Is TTV_NOPTS_VALUE if unknown.
     * This field is not the display duration of the current packet.
     * This field has no meaning if the packet does not have TTV_PKT_FLAG_KEY
     * set.
     *
     * The purpose of this field is to allow seeking in streams that have no
     * keyframes in the conventional sense. It corresponds to the
     * recovery point SEI in H.264 and match_time_delta in NUT. It is also
     * essential for some types of subtitle streams to ensure that all
     * subtitles are correctly displayed after seeking.
     */
    int64_t convergence_duration;
} TTPacket;
#define TTV_PKT_FLAG_KEY     0x0001 ///< The packet contains a keyframe
#define TTV_PKT_FLAG_CORRUPT 0x0002 ///< The packet content is corrupted

enum TTSideDataParamChangeFlags {
    TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT  = 0x0001,
    TTV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,
    TTV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE    = 0x0004,
    TTV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS     = 0x0008,
};
/**
 * @}
 */

struct TTCodecInternal;

enum TTFieldOrder {
    TTV_FIELD_UNKNOWN,
    TTV_FIELD_PROGRESSIVE,
    TTV_FIELD_TT,          //< Top coded_first, top displayed first
    TTV_FIELD_BB,          //< Bottom coded first, bottom displayed first
    TTV_FIELD_TB,          //< Top coded first, bottom displayed first
    TTV_FIELD_BT,          //< Bottom coded first, top displayed first
};

/**
 * main external API structure.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * Please use AVOptions (ttv_opt* / ttv_set/get*()) to access these fields from user
 * applications.
 * sizeof(TTCodecContext) must not be used outside libav*.
 */
typedef struct TTCodecContext {
    /**
     * information on struct for ttv_log
     * - set by ttcodec_alloc_context3
     */
    const AVClass *ttv_class;
    int log_level_offset;

    enum AVMediaType codec_type; /* see AVMEDIA_TYPE_xxx */
    const struct TTCodec  *codec;
#if TT_API_CODEC_NAME
    /**
     * @deprecated this field is not used for anything in libavcodec
     */
    attribute_deprecated
    char             codec_name[32];
#endif
    enum TTCodecID     codec_id; /* see TTV_CODEC_ID_xxx */

    /**
     * fourcc (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * A demuxer should set this to what is stored in the field used to identify the codec.
     * If there are multiple such fields in a container then the demuxer should choose the one
     * which maximizes the information about the used codec.
     * If the codec tag field in a container is larger than 32 bits then the demuxer should
     * remap the longer ID to 32 bits with a table or other structure. Alternatively a new
     * extra_codec_tag + size could be added but for this a clear advantage must be demonstrated
     * first.
     * - encoding: Set by user, if not then the default based on codec_id will be used.
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
    unsigned int codec_tag;

    /**
     * fourcc from the AVI stream header (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * - encoding: unused
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
    unsigned int stream_codec_tag;

    void *priv_data;

    /**
     * Private context used for internal data.
     *
     * Unlike priv_data, this is not codec-specific. It is used in general
     * libavcodec functions.
     */
    struct TTCodecInternal *internal;

    /**
     * Private data of the user, can be used to carry app specific stuff.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    void *opaque;

    /**
     * the average bitrate
     * - encoding: Set by user; unused for constant quantizer encoding.
     * - decoding: Set by libavcodec. 0 or some bitrate if this info is available in the stream.
     */
    int bit_rate;

    /**
     * number of bits the bitstream is allowed to diverge from the reference.
     *           the reference can be CBR (for CBR pass1) or VBR (for pass2)
     * - encoding: Set by user; unused for constant quantizer encoding.
     * - decoding: unused
     */
    int bit_rate_tolerance;

    /**
     * Global quality for codecs which cannot change it per frame.
     * This should be proportional to MPEG-1/2/4 qscale.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int global_quality;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int compression_level;
#define TT_COMPRESSION_DEFAULT -1

    /**
     * CODEC_FLAG_*.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int flags;

    /**
     * CODEC_FLAG2_*
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int flags2;

    /**
     * some codecs need / can use extradata like Huffman tables.
     * mjpeg: Huffman tables
     * rv10: additional flags
     * mpeg4: global headers (they can be in the bitstream or here)
     * The allocated memory should be TT_INPUT_BUFFER_PADDING_SIZE bytes larger
     * than extradata_size to avoid problems if it is read with the bitstream reader.
     * The bytewise contents of extradata must not depend on the architecture or CPU endianness.
     * - encoding: Set/allocated/freed by libavcodec.
     * - decoding: Set/allocated/freed by user.
     */
    uint8_t *extradata;
    int extradata_size;

    /**
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identically 1.
     * This often, but not always is the inverse of the frame rate or field rate
     * for video.
     * - encoding: MUST be set by user.
     * - decoding: the use of this field for decoding is deprecated.
     *             Use framerate instead.
     */
    TTRational time_base;

    /**
     * For some codecs, the time base is closer to the field rate than the frame rate.
     * Most notably, H.264 and MPEG-2 specify time_base as half of frame duration
     * if no telecine is used ...
     *
     * Set to time_base ticks per frame. Default 1, e.g., H.264/MPEG-2 set it to 2.
     */
    int ticks_per_frame;

    /**
     * Codec delay.
     *
     * Encoding: Number of frames delay there will be from the encoder input to
     *           the decoder output. (we assume the decoder matches the spec)
     * Decoding: Number of frames delay in addition to what a standard decoder
     *           as specified in the spec would produce.
     *
     * Video:
     *   Number of frames the decoded output will be delayed relative to the
     *   encoded input.
     *
     * Audio:
     *   For encoding, this field is unused (see initial_padding).
     *
     *   For decoding, this is the number of samples the decoder needs to
     *   output before the decoder's output is valid. When seeking, you should
     *   start decoding this many samples prior to your desired seek point.
     *
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int delay;


    /* video only */
    /**
     * picture width / height.
     * - encoding: MUST be set by user.
     * - decoding: May be set by the user before opening the decoder if known e.g.
     *             from the container. Some decoders will require the dimensions
     *             to be set by the caller. During decoding, the decoder may
     *             overwrite those values as required.
     */
    int width, height;

    /**
     * Bitstream width / height, may be different from width/height e.g. when
     * the decoded frame is cropped before being output or lowres is enabled.
     * - encoding: unused
     * - decoding: May be set by the user before opening the decoder if known
     *             e.g. from the container. During decoding, the decoder may
     *             overwrite those values as required.
     */
    int coded_width, coded_height;



    /**
     * the number of pictures in a group of pictures, or 0 for intra_only
     * - encoding: Set by user.
     * - decoding: unused
     */
    int gop_size;

    /**
     * Pixel format, see TTV_PIX_FMT_xxx.
     * May be set by the demuxer if known from headers.
     * May be overridden by the decoder if it knows better.
     * - encoding: Set by user.
     * - decoding: Set by user if known, overridden by libavcodec if known
     */
    enum TTPixelFormat pix_fmt;

    /**
     * Motion estimation algorithm used for video coding.
     * 1 (zero), 2 (full), 3 (log), 4 (phods), 5 (epzs), 6 (x1), 7 (hex),
     * 8 (umh), 9 (iter), 10 (tesa) [7, 8, 10 are x264 specific, 9 is snow specific]
     * - encoding: MUST be set by user.
     * - decoding: unused
     */
    int me_method;

    /**
     * If non NULL, 'draw_horiz_band' is called by the libavcodec
     * decoder to draw a horizontal band. It improves cache usage. Not
     * all codecs can do that. You must check the codec capabilities
     * beforehand.
     * When multithreading is used, it may be called from multiple threads
     * at the same time; threads might draw different parts of the same TTFrame,
     * or multiple TTFrames, and there is no guarantee that slices will be drawn
     * in order.
     * The function is also used by hardware acceleration APIs.
     * It is called at least once during frame decoding to pass
     * the data needed for hardware render.
     * In that mode instead of pixel data, TTFrame points to
     * a structure specific to the acceleration API. The application
     * reads the structure and can change some fields to indicate progress
     * or mark state.
     * - encoding: unused
     * - decoding: Set by user.
     * @param height the height of the slice
     * @param y the y position of the slice
     * @param type 1->top field, 2->bottom field, 3->frame
     * @param offset offset into the TTFrame.data from which the slice should be read
     */
    void (*draw_horiz_band)(struct TTCodecContext *s,
                            const TTFrame *src, int offset[TTV_NUM_DATA_POINTERS],
                            int y, int type, int height);

    /**
     * callback to negotiate the pixelFormat
     * @param fmt is the list of formats which are supported by the codec,
     * it is terminated by -1 as 0 is a valid format, the formats are ordered by quality.
     * The first is always the native one.
     * @note The callback may be called again immediately if initialization for
     * the selected (hardware-accelerated) pixel format failed.
     * @warning Behavior is undefined if the callback returns a value not
     * in the fmt list of formats.
     * @return the chosen format
     * - encoding: unused
     * - decoding: Set by user, if not set the native format will be chosen.
     */
    enum TTPixelFormat (*get_format)(struct TTCodecContext *s, const enum TTPixelFormat * fmt);

    /**
     * maximum number of B-frames between non-B-frames
     * Note: The output will be delayed by max_b_frames+1 relative to the input.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_b_frames;

    /**
     * qscale factor between IP and B-frames
     * If > 0 then the last P-frame quantizer will be used (q= lastp_q*factor+offset).
     * If < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset).
     * - encoding: Set by user.
     * - decoding: unused
     */
    float b_quant_factor;

    /** obsolete FIXME remove */
    int rc_strategy;
#define TT_RC_STRATEGY_XVID 1

    int b_frame_strategy;

    /**
     * qscale offset between IP and B-frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    float b_quant_offset;

    /**
     * Size of the frame reordering buffer in the decoder.
     * For MPEG-2 it is 1 IPB or 0 low delay IP.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int has_b_frames;

    /**
     * 0-> h263 quant 1-> mpeg quant
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mpeg_quant;

    /**
     * qscale factor between P and I-frames
     * If > 0 then the last p frame quantizer will be used (q= lastp_q*factor+offset).
     * If < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset).
     * - encoding: Set by user.
     * - decoding: unused
     */
    float i_quant_factor;

    /**
     * qscale offset between P and I-frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    float i_quant_offset;

    /**
     * luminance masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float lumi_masking;

    /**
     * temporary complexity masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float temporal_cplx_masking;

    /**
     * spatial complexity masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float spatial_cplx_masking;

    /**
     * p block masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float p_masking;

    /**
     * darkness masking (0-> disabled)
     * - encoding: Set by user.
     * - decoding: unused
     */
    float dark_masking;

    /**
     * slice count
     * - encoding: Set by libavcodec.
     * - decoding: Set by user (or 0).
     */
    int slice_count;
    /**
     * prediction method (needed for huffyuv)
     * - encoding: Set by user.
     * - decoding: unused
     */
     int prediction_method;
#define TT_PRED_LEFT   0
#define TT_PRED_PLANE  1
#define TT_PRED_MEDIAN 2

    /**
     * slice offsets in the frame in bytes
     * - encoding: Set/allocated by libavcodec.
     * - decoding: Set/allocated by user (or NULL).
     */
    int *slice_offset;

    /**
     * sample aspect ratio (0 if unknown)
     * That is the width of a pixel divided by the height of the pixel.
     * Numerator and denominator must be relatively prime and smaller than 256 for some video standards.
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    TTRational sample_aspect_ratio;

    /**
     * motion estimation comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_cmp;
    /**
     * subpixel motion estimation comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_sub_cmp;
    /**
     * macroblock comparison function (not supported yet)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_cmp;
    /**
     * interlaced DCT comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int ildct_cmp;
#define TT_CMP_SAD    0
#define TT_CMP_SSE    1
#define TT_CMP_SATD   2
#define TT_CMP_DCT    3
#define TT_CMP_PSNR   4
#define TT_CMP_BIT    5
#define TT_CMP_RD     6
#define TT_CMP_ZERO   7
#define TT_CMP_VSAD   8
#define TT_CMP_VSSE   9
#define TT_CMP_NSSE   10
#define TT_CMP_W53    11
#define TT_CMP_W97    12
#define TT_CMP_DCTMAX 13
#define TT_CMP_DCT264 14
#define TT_CMP_CHROMA 256

    /**
     * ME diamond size & shape
     * - encoding: Set by user.
     * - decoding: unused
     */
    int dia_size;

    /**
     * amount of previous MV predictors (2a+1 x 2a+1 square)
     * - encoding: Set by user.
     * - decoding: unused
     */
    int last_predictor_count;

    /**
     * prepass for motion estimation
     * - encoding: Set by user.
     * - decoding: unused
     */
    int pre_me;

    /**
     * motion estimation prepass comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_pre_cmp;

    /**
     * ME prepass diamond size & shape
     * - encoding: Set by user.
     * - decoding: unused
     */
    int pre_dia_size;

    /**
     * subpel ME quality
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_subpel_quality;

#if TT_API_AFD
    /**
     * DTG active format information (additional aspect ratio
     * information only used in DVB MPEG-2 transport streams)
     * 0 if not set.
     *
     * - encoding: unused
     * - decoding: Set by decoder.
     * @deprecated Deprecated in favor of AVSideData
     */
    attribute_deprecated int dtg_active_format;
#define TT_DTG_AFD_SAME         8
#define TT_DTG_AFD_4_3          9
#define TT_DTG_AFD_16_9         10
#define TT_DTG_AFD_14_9         11
#define TT_DTG_AFD_4_3_SP_14_9  13
#define TT_DTG_AFD_16_9_SP_14_9 14
#define TT_DTG_AFD_SP_4_3       15
#endif /* TT_API_AFD */

    /**
     * maximum motion estimation search range in subpel units
     * If 0 then no limit.
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_range;

    /**
     * intra quantizer bias
     * - encoding: Set by user.
     * - decoding: unused
     */
    int intra_quant_bias;
#define TT_DEFAULT_QUANT_BIAS 999999

    /**
     * inter quantizer bias
     * - encoding: Set by user.
     * - decoding: unused
     */
    int inter_quant_bias;

    /**
     * slice flags
     * - encoding: unused
     * - decoding: Set by user.
     */
    int slice_flags;
#define SLICE_FLAG_CODED_ORDER    0x0001 ///< draw_horiz_band() is called in coded order instead of display
#define SLICE_FLAG_ALLOW_FIELD    0x0002 ///< allow draw_horiz_band() with field slices (MPEG2 field pics)
#define SLICE_FLAG_ALLOW_PLANE    0x0004 ///< allow draw_horiz_band() with 1 component at a time (SVQ1)

#if TT_API_XVMC
    /**
     * XVideo Motion Acceleration
     * - encoding: forbidden
     * - decoding: set by decoder
     * @deprecated XvMC doesn't need it anymore.
     */
    attribute_deprecated int xvmc_acceleration;
#endif /* TT_API_XVMC */

    /**
     * macroblock decision mode
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_decision;
#define TT_MB_DECISION_SIMPLE 0        ///< uses mb_cmp
#define TT_MB_DECISION_BITS   1        ///< chooses the one which needs the fewest bits
#define TT_MB_DECISION_RD     2        ///< rate distortion

    /**
     * custom intra quantization matrix
     * - encoding: Set by user, can be NULL.
     * - decoding: Set by libavcodec.
     */
    uint16_t *intra_matrix;

    /**
     * custom inter quantization matrix
     * - encoding: Set by user, can be NULL.
     * - decoding: Set by libavcodec.
     */
    uint16_t *inter_matrix;

    /**
     * scene change detection threshold
     * 0 is default, larger means fewer detected scene changes.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int scenechange_threshold;

    /**
     * noise reduction strength
     * - encoding: Set by user.
     * - decoding: unused
     */
    int noise_reduction;

#if TT_API_MPV_OPT
    /**
     * @deprecated this field is unused
     */
    attribute_deprecated
    int me_threshold;

    /**
     * @deprecated this field is unused
     */
    attribute_deprecated
    int mb_threshold;
#endif

    /**
     * precision of the intra DC coefficient - 8
     * - encoding: Set by user.
     * - decoding: unused
     */
    int intra_dc_precision;

    /**
     * Number of macroblock rows at the top which are skipped.
     * - encoding: unused
     * - decoding: Set by user.
     */
    int skip_top;

    /**
     * Number of macroblock rows at the bottom which are skipped.
     * - encoding: unused
     * - decoding: Set by user.
     */
    int skip_bottom;

#if TT_API_MPV_OPT
    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    float border_masking;
#endif

    /**
     * minimum MB lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_lmin;

    /**
     * maximum MB lagrange multipler
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mb_lmax;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int me_penalty_compensation;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int bidir_refine;

    /**
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int brd_scale;

    /**
     * minimum GOP size
     * - encoding: Set by user.
     * - decoding: unused
     */
    int keyint_min;

    /**
     * number of reference frames
     * - encoding: Set by user.
     * - decoding: Set by lavc.
     */
    int refs;

    /**
     * chroma qp offset from luma
     * - encoding: Set by user.
     * - decoding: unused
     */
    int chromaoffset;

#if TT_API_UNUSED_MEMBERS
    /**
     * Multiplied by qscale for each frame and added to scene_change_score.
     * - encoding: Set by user.
     * - decoding: unused
     */
    attribute_deprecated int scenechange_factor;
#endif

    /**
     *
     * Note: Value depends upon the compare function used for fullpel ME.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int mv0_threshold;

    /**
     * Adjust sensitivity of b_frame_strategy 1.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int b_sensitivity;

    /**
     * Chromaticity coordinates of the source primaries.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorPrimaries color_primaries;

    /**
     * Color Transfer Characteristic.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorTransferCharacteristic color_trc;

    /**
     * YUV colorspace type.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorSpace colorspace;

    /**
     * MPEG vs JPEG YUV range.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTColorRange color_range;

    /**
     * This defines the location of chroma samples.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum TTChromaLocation chroma_sample_location;

    /**
     * Number of slices.
     * Indicates number of picture subdivisions. Used for parallelized
     * decoding.
     * - encoding: Set by user
     * - decoding: unused
     */
    int slices;

    /** Field order
     * - encoding: set by libavcodec
     * - decoding: Set by user.
     */
    enum TTFieldOrder field_order;

    /* audio only */
    int sample_rate; ///< samples per second
    int channels;    ///< number of audio channels

    /**
     * audio sample format
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
//    enum AVSampleFormat sample_fmt;  ///< sample format

    /* The following data should not be initialized. */
    /**
     * Number of samples per channel in an audio frame.
     *
     * - encoding: set by libavcodec in ttcodec_open2(). Each submitted frame
     *   except the last must contain exactly frame_size samples per channel.
     *   May be 0 when the codec has CODEC_CAP_VARIABLE_FRAME_SIZE set, then the
     *   frame size is not restricted.
     * - decoding: may be set by some decoders to indicate constant frame size
     */
    int frame_size;

    /**
     * Frame counter, set by libavcodec.
     *
     * - decoding: total number of frames returned from the decoder so far.
     * - encoding: total number of frames passed to the encoder so far.
     *
     *   @note the counter is not incremented if encoding/decoding resulted in
     *   an error.
     */
    int frame_number;

    /**
     * number of bytes per packet if constant and known or 0
     * Used by some WAV based audio codecs.
     */
    int block_align;

    /**
     * Audio cutoff bandwidth (0 means "automatic")
     * - encoding: Set by user.
     * - decoding: unused
     */
    int cutoff;

#if TT_API_REQUEST_CHANNELS
    /**
     * Decoder should decode to this many channels if it can (0 for default)
     * - encoding: unused
     * - decoding: Set by user.
     * @deprecated Deprecated in favor of request_channel_layout.
     */
    attribute_deprecated int request_channels;
#endif

    /**
     * Audio channel layout.
     * - encoding: set by user.
     * - decoding: set by user, may be overwritten by libavcodec.
     */
    uint64_t channel_layout;

    /**
     * Request decoder to use this channel layout if it can (0 for default)
     * - encoding: unused
     * - decoding: Set by user.
     */
    uint64_t request_channel_layout;

    /**
     * Type of service that the audio stream conveys.
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    enum TTAudioServiceType audio_service_type;

    /**
     * desired sample format
     * - encoding: Not used.
     * - decoding: Set by user.
     * Decoder will decode to this format if it can.
     */
//    enum AVSampleFormat request_sample_fmt;

#if TT_API_GET_BUFFER
    /**
     * Called at the beginning of each frame to get a buffer for it.
     *
     * The function will set TTFrame.data[], TTFrame.linesize[].
     * TTFrame.extended_data[] must also be set, but it should be the same as
     * TTFrame.data[] except for planar audio with more channels than can fit
     * in TTFrame.data[]. In that case, TTFrame.data[] shall still contain as
     * many data pointers as it can hold.
     *
     * if CODEC_CAP_DR1 is not set then get_buffer() must call
     * ttcodec_default_get_buffer() instead of providing buffers allocated by
     * some other means.
     *
     * TTFrame.data[] should be 32- or 16-byte-aligned unless the CPU doesn't
     * need it. ttcodec_default_get_buffer() aligns the output buffer properly,
     * but if get_buffer() is overridden then alignment considerations should
     * be taken into account.
     *
     * @see ttcodec_default_get_buffer()
     *
     * Video:
     *
     * If pic.reference is set then the frame will be read later by libavcodec.
     * ttcodec_align_dimensions2() should be used to find the required width and
     * height, as they normally need to be rounded up to the next multiple of 16.
     *
     * If frame multithreading is used and thread_safe_callbacks is set,
     * it may be called from a different thread, but not from more than one at
     * once. Does not need to be reentrant.
     *
     * @see release_buffer(), reget_buffer()
     * @see ttcodec_align_dimensions2()
     *
     * Audio:
     *
     * Decoders request a buffer of a particular size by setting
     * TTFrame.nb_samples prior to calling get_buffer(). The decoder may,
     * however, utilize only part of the buffer by setting TTFrame.nb_samples
     * to a smaller value in the output frame.
     *
     * Decoders cannot use the buffer after returning from
     * ttcodec_decode_audio4(), so they will not call release_buffer(), as it
     * is assumed to be released immediately upon return. In some rare cases,
     * a decoder may need to call get_buffer() more than once in a single
     * call to ttcodec_decode_audio4(). In that case, when get_buffer() is
     * called again after it has already been called once, the previously
     * acquired buffer is assumed to be released at that time and may not be
     * reused by the decoder.
     *
     * As a convenience, ttv_samples_get_buffer_size() and
     * ttv_samples_fill_arrays() in libavutil may be used by custom get_buffer()
     * functions to find the required data size and to fill data pointers and
     * linesize. In TTFrame.linesize, only linesize[0] may be set for audio
     * since all planes must be the same size.
     *
     * @see ttv_samples_get_buffer_size(), ttv_samples_fill_arrays()
     *
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     *
     * @deprecated use get_buffer2()
     */
    attribute_deprecated
    int (*get_buffer)(struct TTCodecContext *c, TTFrame *pic);

    /**
     * Called to release buffers which were allocated with get_buffer.
     * A released buffer can be reused in get_buffer().
     * pic.data[*] must be set to NULL.
     * May be called from a different thread if frame multithreading is used,
     * but not by more than one thread at once, so does not need to be reentrant.
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     *
     * @deprecated custom freeing callbacks should be set from get_buffer2()
     */
    attribute_deprecated
    void (*release_buffer)(struct TTCodecContext *c, TTFrame *pic);

    /**
     * Called at the beginning of a frame to get cr buffer for it.
     * Buffer type (size, hints) must be the same. libavcodec won't check it.
     * libavcodec will pass previous buffer in pic, function should return
     * same buffer or new buffer with old frame "painted" into it.
     * If pic.data[0] == NULL must behave like get_buffer().
     * if CODEC_CAP_DR1 is not set then reget_buffer() must call
     * ttcodec_default_reget_buffer() instead of providing buffers allocated by
     * some other means.
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     */
    attribute_deprecated
    int (*reget_buffer)(struct TTCodecContext *c, TTFrame *pic);
#endif

    /**
     * This callback is called at the beginning of each frame to get data
     * buffer(s) for it. There may be one contiguous buffer for all the data or
     * there may be a buffer per each data plane or anything in between. What
     * this means is, you may set however many entries in buf[] you feel necessary.
     * Each buffer must be reference-counted using the AVBuffer API (see description
     * of buf[] below).
     *
     * The following fields will be set in the frame before this callback is
     * called:
     * - format
     * - width, height (video only)
     * - sample_rate, channel_layout, nb_samples (audio only)
     * Their values may differ from the corresponding values in
     * TTCodecContext. This callback must use the frame values, not the codec
     * context values, to calculate the required buffer size.
     *
     * This callback must fill the following fields in the frame:
     * - data[]
     * - linesize[]
     * - extended_data:
     *   * if the data is planar audio with more than 8 channels, then this
     *     callback must allocate and fill extended_data to contain all pointers
     *     to all data planes. data[] must hold as many pointers as it can.
     *     extended_data must be allocated with ttv_malloc() and will be freed in
     *     ttv_frame_unref().
     *   * otherwise exended_data must point to data
     * - buf[] must contain one or more pointers to TTBufferRef structures. Each of
     *   the frame's data and extended_data pointers must be contained in these. That
     *   is, one TTBufferRef for each allocated chunk of memory, not necessarily one
     *   TTBufferRef per data[] entry. See: ttv_buffer_create(), ttv_buffer_alloc(),
     *   and ttv_buffer_ref().
     * - extended_buf and nb_extended_buf must be allocated with ttv_malloc() by
     *   this callback and filled with the extra buffers if there are more
     *   buffers than buf[] can hold. extended_buf will be freed in
     *   ttv_frame_unref().
     *
     * If CODEC_CAP_DR1 is not set then get_buffer2() must call
     * ttcodec_default_get_buffer2() instead of providing buffers allocated by
     * some other means.
     *
     * Each data plane must be aligned to the maximum required by the target
     * CPU.
     *
     * @see ttcodec_default_get_buffer2()
     *
     * Video:
     *
     * If TTV_GET_BUFFER_FLAG_REF is set in flags then the frame may be reused
     * (read and/or written to if it is writable) later by libavcodec.
     *
     * ttcodec_align_dimensions2() should be used to find the required width and
     * height, as they normally need to be rounded up to the next multiple of 16.
     *
     * Some decoders do not support linesizes changing between frames.
     *
     * If frame multithreading is used and thread_safe_callbacks is set,
     * this callback may be called from a different thread, but not from more
     * than one at once. Does not need to be reentrant.
     *
     * @see ttcodec_align_dimensions2()
     *
     * Audio:
     *
     * Decoders request a buffer of a particular size by setting
     * TTFrame.nb_samples prior to calling get_buffer2(). The decoder may,
     * however, utilize only part of the buffer by setting TTFrame.nb_samples
     * to a smaller value in the output frame.
     *
     * As a convenience, ttv_samples_get_buffer_size() and
     * ttv_samples_fill_arrays() in libavutil may be used by custom get_buffer2()
     * functions to find the required data size and to fill data pointers and
     * linesize. In TTFrame.linesize, only linesize[0] may be set for audio
     * since all planes must be the same size.
     *
     * @see ttv_samples_get_buffer_size(), ttv_samples_fill_arrays()
     *
     * - encoding: unused
     * - decoding: Set by libavcodec, user can override.
     */
    int (*get_buffer2)(struct TTCodecContext *s, TTFrame *frame, int flags);

    /**
     * If non-zero, the decoded audio and video frames returned from
     * ttcodec_decode_video2() and ttcodec_decode_audio4() are reference-counted
     * and are valid indefinitely. The caller must free them with
     * ttv_frame_unref() when they are not needed anymore.
     * Otherwise, the decoded frames must not be freed by the caller and are
     * only valid until the next decode call.
     *
     * - encoding: unused
     * - decoding: set by the caller before ttcodec_open2().
     */
    int refcounted_frames;

    /* - encoding parameters */
    float qcompress;  ///< amount of qscale change between easy & hard scenes (0.0-1.0)
    float qblur;      ///< amount of qscale smoothing over time (0.0-1.0)

    /**
     * minimum quantizer
     * - encoding: Set by user.
     * - decoding: unused
     */
    int qmin;

    /**
     * maximum quantizer
     * - encoding: Set by user.
     * - decoding: unused
     */
    int qmax;

    /**
     * maximum quantizer difference between frames
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_qdiff;

#if TT_API_MPV_OPT
    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    float rc_qsquish;

    attribute_deprecated
    float rc_qmod_amp;
    attribute_deprecated
    int rc_qmod_freq;
#endif

    /**
     * decoder bitstream buffer size
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_buffer_size;

    /**
     * ratecontrol override, see RcOverride
     * - encoding: Allocated/set/freed by user.
     * - decoding: unused
     */
    int rc_override_count;
    RcOverride *rc_override;

#if TT_API_MPV_OPT
    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    const char *rc_eq;
#endif

    /**
     * maximum bitrate
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
    int rc_max_rate;

    /**
     * minimum bitrate
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_min_rate;

#if TT_API_MPV_OPT
    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    float rc_buffer_aggressivity;

    attribute_deprecated
    float rc_initial_cplx;
#endif

    /**
     * Ratecontrol attempt to use, at maximum, <value> of what can be used without an underflow.
     * - encoding: Set by user.
     * - decoding: unused.
     */
    float rc_max_available_vbv_use;

    /**
     * Ratecontrol attempt to use, at least, <value> times the amount needed to prevent a vbv overflow.
     * - encoding: Set by user.
     * - decoding: unused.
     */
    float rc_min_vbv_overflow_use;

    /**
     * Number of bits which should be loaded into the rc buffer before decoding starts.
     * - encoding: Set by user.
     * - decoding: unused
     */
    int rc_initial_buffer_occupancy;

#define TT_CODER_TYPE_VLC       0
#define TT_CODER_TYPE_AC        1
#define TT_CODER_TYPE_RAW       2
#define TT_CODER_TYPE_RLE       3
#if TT_API_UNUSED_MEMBERS
#define TT_CODER_TYPE_DEFLATE   4
#endif /* TT_API_UNUSED_MEMBERS */
    /**
     * coder type
     * - encoding: Set by user.
     * - decoding: unused
     */
    int coder_type;

    /**
     * context model
     * - encoding: Set by user.
     * - decoding: unused
     */
    int context_model;

#if TT_API_MPV_OPT
    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    int lmin;

    /**
     * @deprecated use encoder private options instead
     */
    attribute_deprecated
    int lmax;
#endif

    /**
     * frame skip threshold
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_threshold;

    /**
     * frame skip factor
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_factor;

    /**
     * frame skip exponent
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_exp;

    /**
     * frame skip comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
    int frame_skip_cmp;

    /**
     * trellis RD quantization
     * - encoding: Set by user.
     * - decoding: unused
     */
    int trellis;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int min_prediction_order;

    /**
     * - encoding: Set by user.
     * - decoding: unused
     */
    int max_prediction_order;

    /**
     * GOP timecode frame start number
     * - encoding: Set by user, in non drop frame format
     * - decoding: Set by libavcodec (timecode in the 25 bits format, -1 if unset)
     */
    int64_t timecode_frame_start;

    /* The RTP callback: This function is called    */
    /* every time the encoder has a packet to send. */
    /* It depends on the encoder if the data starts */
    /* with a Start Code (it should). H.263 does.   */
    /* mb_nb contains the number of macroblocks     */
    /* encoded in the RTP payload.                  */
    void (*rtp_callback)(struct TTCodecContext *avctx, void *data, int size, int mb_nb);

    int rtp_payload_size;   /* The size of the RTP payload: the coder will  */
                            /* do its best to deliver a chunk with size     */
                            /* below rtp_payload_size, the chunk will start */
                            /* with a start code on some codecs like H.263. */
                            /* This doesn't take account of any particular  */
                            /* headers inside the transmitted RTP payload.  */

    /* statistics, used for 2-pass encoding */
    int mv_bits;
    int header_bits;
    int i_tex_bits;
    int p_tex_bits;
    int i_count;
    int p_count;
    int skip_count;
    int misc_bits;

    /**
     * number of bits used for the previously encoded frame
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    int frame_bits;

    /**
     * pass1 encoding statistics output buffer
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    char *stats_out;

    /**
     * pass2 encoding statistics input buffer
     * Concatenated stuff from stats_out of pass1 should be placed here.
     * - encoding: Allocated/set/freed by user.
     * - decoding: unused
     */
    char *stats_in;

    /**
     * Work around bugs in encoders which sometimes cannot be detected automatically.
     * - encoding: Set by user
     * - decoding: Set by user
     */
    int workaround_bugs;
#define TT_BUG_AUTODETECT       1  ///< autodetection
#if TT_API_OLD_MSMPEG4
#define TT_BUG_OLD_MSMPEG4      2
#endif
#define TT_BUG_XVID_ILACE       4
#define TT_BUG_UMP4             8
#define TT_BUG_NO_PADDING       16
#define TT_BUG_AMV              32
#if TT_API_AC_VLC
#define TT_BUG_AC_VLC           0  ///< Will be removed, libavcodec can now handle these non-compliant files by default.
#endif
#define TT_BUG_QPEL_CHROMA      64
#define TT_BUG_STD_QPEL         128
#define TT_BUG_QPEL_CHROMA2     256
#define TT_BUG_DIRECT_BLOCKSIZE 512
#define TT_BUG_EDGE             1024
#define TT_BUG_HPEL_CHROMA      2048
#define TT_BUG_DC_CLIP          4096
#define TT_BUG_MS               8192 ///< Work around various bugs in Microsoft's broken decoders.
#define TT_BUG_TRUNCATED       16384

    /**
     * strictly follow the standard (MPEG4, ...).
     * - encoding: Set by user.
     * - decoding: Set by user.
     * Setting this to STRICT or higher means the encoder and decoder will
     * generally do stupid things, whereas setting it to unofficial or lower
     * will mean the encoder might produce output that is not supported by all
     * spec-compliant decoders. Decoders don't differentiate between normal,
     * unofficial and experimental (that is, they always try to decode things
     * when they can) unless they are explicitly asked to behave stupidly
     * (=strictly conform to the specs)
     */
    int strict_std_compliance;
#define TT_COMPLIANCE_VERY_STRICT   2 ///< Strictly conform to an older more strict version of the spec or reference software.
#define TT_COMPLIANCE_STRICT        1 ///< Strictly conform to all the things in the spec no matter what consequences.
#define TT_COMPLIANCE_NORMAL        0
#define TT_COMPLIANCE_UNOFFICIAL   -1 ///< Allow unofficial extensions
#define TT_COMPLIANCE_EXPERIMENTAL -2 ///< Allow nonstandardized experimental things.

    /**
     * error concealment flags
     * - encoding: unused
     * - decoding: Set by user.
     */
    int error_concealment;
#define TT_EC_GUESS_MVS   1
#define TT_EC_DEBLOCK     2
#define TT_EC_FAVOR_INTER 256

    /**
     * debug
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int debug;
#define TT_DEBUG_PICT_INFO   1
#define TT_DEBUG_RC          2
#define TT_DEBUG_BITSTREAM   4
#define TT_DEBUG_MB_TYPE     8
#define TT_DEBUG_QP          16
#if TT_API_DEBUG_MV
/**
 * @deprecated this option does nothing
 */
#define TT_DEBUG_MV          32
#endif
#define TT_DEBUG_DCT_COEFF   0x00000040
#define TT_DEBUG_SKIP        0x00000080
#define TT_DEBUG_STARTCODE   0x00000100
#if TT_API_UNUSED_MEMBERS
#define TT_DEBUG_PTS         0x00000200
#endif /* TT_API_UNUSED_MEMBERS */
#define TT_DEBUG_ER          0x00000400
#define TT_DEBUG_MMCO        0x00000800
#define TT_DEBUG_BUGS        0x00001000
#if TT_API_DEBUG_MV
#define TT_DEBUG_VIS_QP      0x00002000 ///< only access through AVOptions from outside libavcodec
#define TT_DEBUG_VIS_MB_TYPE 0x00004000 ///< only access through AVOptions from outside libavcodec
#endif
#define TT_DEBUG_BUFFERS     0x00008000
#define TT_DEBUG_THREADS     0x00010000
#define TT_DEBUG_NOMC        0x01000000

#if TT_API_DEBUG_MV
    /**
     * debug
     * Code outside libavcodec should access this field using AVOptions
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int debug_mv;
#define TT_DEBUG_VIS_MV_P_FOR  0x00000001 //visualize forward predicted MVs of P frames
#define TT_DEBUG_VIS_MV_B_FOR  0x00000002 //visualize forward predicted MVs of B frames
#define TT_DEBUG_VIS_MV_B_BACK 0x00000004 //visualize backward predicted MVs of B frames
#endif

    /**
     * Error recognition; may misdetect some more or less valid parts as errors.
     * - encoding: unused
     * - decoding: Set by user.
     */
    int err_recognition;

/**
 * Verify checksums embedded in the bitstream (could be of either encoded or
 * decoded data, depending on the codec) and print an error message on mismatch.
 * If TTV_EF_EXPLODE is also set, a mismatching checksum will result in the
 * decoder returning an error.
 */
#define TTV_EF_CRCCHECK  (1<<0)
#define TTV_EF_BITSTREAM (1<<1)          ///< detect bitstream specification deviations
#define TTV_EF_BUFFER    (1<<2)          ///< detect improper bitstream length
#define TTV_EF_EXPLODE   (1<<3)          ///< abort decoding on minor error detection

#define TTV_EF_IGNORE_ERR (1<<15)        ///< ignore errors and continue
#define TTV_EF_CAREFUL    (1<<16)        ///< consider things that violate the spec, are fast to calculate and have not been seen in the wild as errors
#define TTV_EF_COMPLIANT  (1<<17)        ///< consider all spec non compliances as errors
#define TTV_EF_AGGRESSIVE (1<<18)        ///< consider things that a sane encoder should not do as an error


    /**
     * opaque 64bit number (generally a PTS) that will be reordered and
     * output in TTFrame.reordered_opaque
     * - encoding: unused
     * - decoding: Set by user.
     */
    int64_t reordered_opaque;

    /**
     * Hardware accelerator in use
     * - encoding: unused.
     * - decoding: Set by libavcodec
     */
//     struct AVHWAccel *hwaccel;

    /**
     * Hardware accelerator context.
     * For some hardware accelerators, a global context needs to be
     * provided by the user. In that case, this holds display-dependent
     * data FFmpeg cannot instantiate itself. Please refer to the
     * FFmpeg HW accelerator documentation to know how to fill this
     * is. e.g. for VA API, this is a struct vaapi_context.
     * - encoding: unused
     * - decoding: Set by user
     */
    void *hwaccel_context;

    /**
     * error
     * - encoding: Set by libavcodec if flags&CODEC_FLAG_PSNR.
     * - decoding: unused
     */
    uint64_t error[TTV_NUM_DATA_POINTERS];

    /**
     * DCT algorithm, see TT_DCT_* below
     * - encoding: Set by user.
     * - decoding: unused
     */
    int dct_algo;
#define TT_DCT_AUTO    0
#define TT_DCT_FASTINT 1
#if TT_API_UNUSED_MEMBERS
#define TT_DCT_INT     2
#endif /* TT_API_UNUSED_MEMBERS */
#define TT_DCT_MMX     3
#define TT_DCT_ALTIVEC 5
#define TT_DCT_FAAN    6

    /**
     * IDCT algorithm, see TT_IDCT_* below.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int idct_algo;
#define TT_IDCT_AUTO          0
#define TT_IDCT_INT           1
#define TT_IDCT_SIMPLE        2
#define TT_IDCT_SIMPLEMMX     3
#define TT_IDCT_ARM           7
#define TT_IDCT_ALTIVEC       8
#if TT_API_ARCH_SH4
#define TT_IDCT_SH4           9
#endif
#define TT_IDCT_SIMPLEARM     10
#if TT_API_UNUSED_MEMBERS
#define TT_IDCT_IPP           13
#endif /* TT_API_UNUSED_MEMBERS */
#define TT_IDCT_XVID          14
#if TT_API_IDCT_XVIDMMX
#define TT_IDCT_XVIDMMX       14
#endif /* TT_API_IDCT_XVIDMMX */
#define TT_IDCT_SIMPLEARMV5TE 16
#define TT_IDCT_SIMPLEARMV6   17
#if TT_API_ARCH_SPARC
#define TT_IDCT_SIMPLEVIS     18
#endif
#define TT_IDCT_FAAN          20
#define TT_IDCT_SIMPLENEON    22
#if TT_API_ARCH_ALPHA
#define TT_IDCT_SIMPLEALPHA   23
#endif
#define TT_IDCT_SIMPLEAUTO    128

    /**
     * bits per sample/pixel from the demuxer (needed for huffyuv).
     * - encoding: Set by libavcodec.
     * - decoding: Set by user.
     */
     int bits_per_coded_sample;

    /**
     * Bits per sample/pixel of internal libavcodec pixel/sample format.
     * - encoding: set by user.
     * - decoding: set by libavcodec.
     */
    int bits_per_raw_sample;

#if TT_API_LOWRES
    /**
     * low resolution decoding, 1-> 1/2 size, 2->1/4 size
     * - encoding: unused
     * - decoding: Set by user.
     * Code outside libavcodec should access this field using:
     * ttv_codec_{get,set}_lowres(avctx)
     */
     int lowres;
#endif

    /**
     * the picture in the bitstream
     * - encoding: Set by libavcodec.
     * - decoding: unused
     */
    TTFrame *coded_frame;

    /**
     * thread count
     * is used to decide how many independent tasks should be passed to execute()
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int thread_count;

    /**
     * Which multithreading methods to use.
     * Use of TT_THREAD_FRAME will increase decoding delay by one frame per thread,
     * so clients which cannot provide future frames should not use it.
     *
     * - encoding: Set by user, otherwise the default is used.
     * - decoding: Set by user, otherwise the default is used.
     */
    int thread_type;
#define TT_THREAD_FRAME   1 ///< Decode more than one frame at once
#define TT_THREAD_SLICE   2 ///< Decode more than one part of a single frame at once

    /**
     * Which multithreading methods are in use by the codec.
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int active_thread_type;

    /**
     * Set by the client if its custom get_buffer() callback can be called
     * synchronously from another thread, which allows faster multithreaded decoding.
     * draw_horiz_band() will be called from other threads regardless of this setting.
     * Ignored if the default get_buffer() is used.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int thread_safe_callbacks;

    /**
     * The codec may call this to execute several independent things.
     * It will return only after finishing all tasks.
     * The user may replace this with some multithreaded implementation,
     * the default implementation will execute the parts serially.
     * @param count the number of things to execute
     * - encoding: Set by libavcodec, user can override.
     * - decoding: Set by libavcodec, user can override.
     */
    int (*execute)(struct TTCodecContext *c, int (*func)(struct TTCodecContext *c2, void *arg), void *arg2, int *ret, int count, int size);

    /**
     * The codec may call this to execute several independent things.
     * It will return only after finishing all tasks.
     * The user may replace this with some multithreaded implementation,
     * the default implementation will execute the parts serially.
     * Also see ttcodec_thread_init and e.g. the --enable-pthread configure option.
     * @param c context passed also to func
     * @param count the number of things to execute
     * @param arg2 argument passed unchanged to func
     * @param ret return values of executed functions, must have space for "count" values. May be NULL.
     * @param func function that will be called count times, with jobnr from 0 to count-1.
     *             threadnr will be in the range 0 to c->thread_count-1 < MAX_THREADS and so that no
     *             two instances of func executing at the same time will have the same threadnr.
     * @return always 0 currently, but code should handle a future improvement where when any call to func
     *         returns < 0 no further calls to func may be done and < 0 is returned.
     * - encoding: Set by libavcodec, user can override.
     * - decoding: Set by libavcodec, user can override.
     */
    int (*execute2)(struct TTCodecContext *c, int (*func)(struct TTCodecContext *c2, void *arg, int jobnr, int threadnr), void *arg2, int *ret, int count);

#if TT_API_THREAD_OPAQUE
    /**
     * @deprecated this field should not be used from outside of lavc
     */
    attribute_deprecated
    void *thread_opaque;
#endif

    /**
     * noise vs. sse weight for the nsse comparison function
     * - encoding: Set by user.
     * - decoding: unused
     */
     int nsse_weight;

    /**
     * profile
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
     int profile;
#define TT_PROFILE_UNKNOWN -99
#define TT_PROFILE_RESERVED -100

#define TT_PROFILE_AAC_MAIN 0
#define TT_PROFILE_AAC_LOW  1
#define TT_PROFILE_AAC_SSR  2
#define TT_PROFILE_AAC_LTP  3
#define TT_PROFILE_AAC_HE   4
#define TT_PROFILE_AAC_HE_V2 28
#define TT_PROFILE_AAC_LD   22
#define TT_PROFILE_AAC_ELD  38
#define TT_PROFILE_MPEG2_AAC_LOW 128
#define TT_PROFILE_MPEG2_AAC_HE  131

#define TT_PROFILE_DTS         20
#define TT_PROFILE_DTS_ES      30
#define TT_PROFILE_DTS_96_24   40
#define TT_PROFILE_DTS_HD_HRA  50
#define TT_PROFILE_DTS_HD_MA   60

#define TT_PROFILE_MPEG2_422    0
#define TT_PROFILE_MPEG2_HIGH   1
#define TT_PROFILE_MPEG2_SS     2
#define TT_PROFILE_MPEG2_SNR_SCALABLE  3
#define TT_PROFILE_MPEG2_MAIN   4
#define TT_PROFILE_MPEG2_SIMPLE 5

#define TT_PROFILE_H264_CONSTRAINED  (1<<9)  // 8+1; constraint_set1_flag
#define TT_PROFILE_H264_INTRA        (1<<11) // 8+3; constraint_set3_flag

#define TT_PROFILE_H264_BASELINE             66
#define TT_PROFILE_H264_CONSTRAINED_BASELINE (66|TT_PROFILE_H264_CONSTRAINED)
#define TT_PROFILE_H264_MAIN                 77
#define TT_PROFILE_H264_EXTENDED             88
#define TT_PROFILE_H264_HIGH                 100
#define TT_PROFILE_H264_HIGH_10              110
#define TT_PROFILE_H264_HIGH_10_INTRA        (110|TT_PROFILE_H264_INTRA)
#define TT_PROFILE_H264_HIGH_422             122
#define TT_PROFILE_H264_HIGH_422_INTRA       (122|TT_PROFILE_H264_INTRA)
#define TT_PROFILE_H264_HIGH_444             144
#define TT_PROFILE_H264_HIGH_444_PREDICTIVE  244
#define TT_PROFILE_H264_HIGH_444_INTRA       (244|TT_PROFILE_H264_INTRA)
#define TT_PROFILE_H264_CAVLC_444            44

#define TT_PROFILE_VC1_SIMPLE   0
#define TT_PROFILE_VC1_MAIN     1
#define TT_PROFILE_VC1_COMPLEX  2
#define TT_PROFILE_VC1_ADVANCED 3

#define TT_PROFILE_MPEG4_SIMPLE                     0
#define TT_PROFILE_MPEG4_SIMPLE_SCALABLE            1
#define TT_PROFILE_MPEG4_CORE                       2
#define TT_PROFILE_MPEG4_MAIN                       3
#define TT_PROFILE_MPEG4_N_BIT                      4
#define TT_PROFILE_MPEG4_SCALABLE_TEXTURE           5
#define TT_PROFILE_MPEG4_SIMPLE_FACE_ANIMATION      6
#define TT_PROFILE_MPEG4_BASIC_ANIMATED_TEXTURE     7
#define TT_PROFILE_MPEG4_HYBRID                     8
#define TT_PROFILE_MPEG4_ADVANCED_REAL_TIME         9
#define TT_PROFILE_MPEG4_CORE_SCALABLE             10
#define TT_PROFILE_MPEG4_ADVANCED_CODING           11
#define TT_PROFILE_MPEG4_ADVANCED_CORE             12
#define TT_PROFILE_MPEG4_ADVANCED_SCALABLE_TEXTURE 13
#define TT_PROFILE_MPEG4_SIMPLE_STUDIO             14
#define TT_PROFILE_MPEG4_ADVANCED_SIMPLE           15

#define TT_PROFILE_JPEG2000_CSTREAM_RESTRICTION_0   0
#define TT_PROFILE_JPEG2000_CSTREAM_RESTRICTION_1   1
#define TT_PROFILE_JPEG2000_CSTREAM_NO_RESTRICTION  2
#define TT_PROFILE_JPEG2000_DCINEMA_2K              3
#define TT_PROFILE_JPEG2000_DCINEMA_4K              4


#define TT_PROFILE_HEVC_MAIN                        1
#define TT_PROFILE_HEVC_MAIN_10                     2
#define TT_PROFILE_HEVC_MAIN_STILL_PICTURE          3
#define TT_PROFILE_HEVC_REXT                        4

    /**
     * level
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
     int level;
#define TT_LEVEL_UNKNOWN -99

    /**
     * Skip loop filtering for selected frames.
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum TTDiscard skip_loop_filter;

    /**
     * Skip IDCT/dequantization for selected frames.
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum TTDiscard skip_idct;

    /**
     * Skip decoding for selected frames.
     * - encoding: unused
     * - decoding: Set by user.
     */
    enum TTDiscard skip_frame;

    /**
     * Header containing style information for text subtitles.
     * For SUBTITLE_ASS subtitle type, it should contain the whole ASS
     * [Script Info] and [V4+ Styles] section, plus the [Events] line and
     * the Format line following. It shouldn't include any Dialogue line.
     * - encoding: Set/allocated/freed by user (before ttcodec_open2())
     * - decoding: Set/allocated/freed by libavcodec (by ttcodec_open2())
     */
    uint8_t *subtitle_header;
    int subtitle_header_size;

#if TT_API_ERROR_RATE
    /**
     * @deprecated use the 'error_rate' private AVOption of the mpegvideo
     * encoders
     */
    attribute_deprecated
    int error_rate;
#endif

#if TT_API_CODEC_PKT
    /**
     * @deprecated this field is not supposed to be accessed from outside lavc
     */
    attribute_deprecated
    TTPacket *pkt;
#endif

    /**
     * VBV delay coded in the last frame (in periods of a 27 MHz clock).
     * Used for compliant TS muxing.
     * - encoding: Set by libavcodec.
     * - decoding: unused.
     */
    uint64_t vbv_delay;

    /**
     * Encoding only. Allow encoders to output packets that do not contain any
     * encoded data, only side data.
     *
     * Some encoders need to output such packets, e.g. to update some stream
     * parameters at the end of encoding.
     *
     * All callers are strongly recommended to set this option to 1 and update
     * their code to deal with such packets, since this behaviour may become
     * always enabled in the future (then this option will be deprecated and
     * later removed). To avoid ABI issues when this happens, the callers should
     * use AVOptions to set this field.
     */
    int side_data_only_packets;

    /**
     * Audio only. The number of "priming" samples (padding) inserted by the
     * encoder at the beginning of the audio. I.e. this number of leading
     * decoded samples must be discarded by the caller to get the original audio
     * without leading padding.
     *
     * - decoding: unused
     * - encoding: Set by libavcodec. The timestamps on the output packets are
     *             adjusted by the encoder so that they always refer to the
     *             first sample of the data actually contained in the packet,
     *             including any added padding.  E.g. if the timebase is
     *             1/samplerate and the timestamp of the first input sample is
     *             0, the timestamp of the first output packet will be
     *             -initial_padding.
     */
    int initial_padding;

    /**
     * - decoding: For codecs that store a framerate value in the compressed
     *             bitstream, the decoder may export it here. { 0, 1} when
     *             unknown.
     * - encoding: unused
     */
    TTRational framerate;

    /**
     * Timebase in which pkt_dts/pts and TTPacket.dts/pts are.
     * Code outside libavcodec should access this field using:
     * ttv_codec_{get,set}_pkt_timebase(avctx)
     * - encoding unused.
     * - decoding set by user.
     */
    TTRational pkt_timebase;

    /**
     * TTCodecDescriptor
     * Code outside libavcodec should access this field using:
     * ttv_codec_{get,set}_codec_descriptor(avctx)
     * - encoding: unused.
     * - decoding: set by libavcodec.
     */
 //   const TTCodecDescriptor *codec_descriptor;

#if !TT_API_LOWRES
    /**
     * low resolution decoding, 1-> 1/2 size, 2->1/4 size
     * - encoding: unused
     * - decoding: Set by user.
     * Code outside libavcodec should access this field using:
     * ttv_codec_{get,set}_lowres(avctx)
     */
     int lowres;
#endif

    /**
     * Current statistics for PTS correction.
     * - decoding: maintained and used by libavcodec, not intended to be used by user apps
     * - encoding: unused
     */
    int64_t pts_correction_num_faulty_pts; /// Number of incorrect PTS values so far
    int64_t pts_correction_num_faulty_dts; /// Number of incorrect DTS values so far
    int64_t pts_correction_last_pts;       /// PTS of the last frame
    int64_t pts_correction_last_dts;       /// DTS of the last frame

    /**
     * Character encoding of the input subtitles file.
     * - decoding: set by user
     * - encoding: unused
     */
    char *sub_charenc;

    /**
     * Subtitles character encoding mode. Formats or codecs might be adjusting
     * this setting (if they are doing the conversion themselves for instance).
     * - decoding: set by libavcodec
     * - encoding: unused
     */
    int sub_charenc_mode;
#define TT_SUB_CHARENC_MODE_DO_NOTHING  -1  ///< do nothing (demuxer outputs a stream supposed to be already in UTF-8, or the codec is bitmap for instance)
#define TT_SUB_CHARENC_MODE_AUTOMATIC    0  ///< libavcodec will select the mode itself
#define TT_SUB_CHARENC_MODE_PRE_DECODER  1  ///< the TTPacket data needs to be recoded to UTF-8 before being fed to the decoder, requires iconv

    /**
     * Skip processing alpha if supported by codec.
     * Note that if the format uses pre-multiplied alpha (common with VP6,
     * and recommended due to better video quality/compression)
     * the image will look as if alpha-blended onto a black background.
     * However for formats that do not use pre-multiplied alpha
     * there might be serious artefacts (though e.g. libswscale currently
     * assumes pre-multiplied alpha anyway).
     * Code outside libavcodec should access this field using AVOptions
     *
     * - decoding: set by user
     * - encoding: unused
     */
    int skip_alpha;

    /**
     * Number of samples to skip after a discontinuity
     * - decoding: unused
     * - encoding: set by libavcodec
     */
    int seek_preroll;

#if !TT_API_DEBUG_MV
    /**
     * debug motion vectors
     * Code outside libavcodec should access this field using AVOptions
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int debug_mv;
#define TT_DEBUG_VIS_MV_P_FOR  0x00000001 //visualize forward predicted MVs of P frames
#define TT_DEBUG_VIS_MV_B_FOR  0x00000002 //visualize forward predicted MVs of B frames
#define TT_DEBUG_VIS_MV_B_BACK 0x00000004 //visualize backward predicted MVs of B frames
#endif

    /**
     * custom intra quantization matrix
     * Code outside libavcodec should access this field using ttv_codec_g/set_chroma_intra_matrix()
     * - encoding: Set by user, can be NULL.
     * - decoding: unused.
     */
    uint16_t *chroma_intra_matrix;

    /**
     * dump format separator.
     * can be ", " or "\n      " or anything else
     * Code outside libavcodec should access this field using AVOptions
     * (NO direct access).
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    uint8_t *dump_separator;

    /**
     * ',' seperated list of allowed decoders.
     * If NULL then all are allowed
     * - encoding: unused
     * - decoding: set by user through AVOPtions (NO direct access)
     */
    char *codec_whitelist;
} TTCodecContext;

typedef struct TTProfile {
    int profile;
    const char *name; ///< short name for the profile
} TTProfile;

typedef struct TTCodecDefault TTCodecDefault;

struct TTSubtitle;


typedef struct TTCodec {
	const char *name;
	const char *long_name;
	enum AVMediaType type;
	enum TTCodecID id;
	int priv_data_size;
	int capabilities;
	const enum TTPixelFormat *pix_fmts;  
	const AVClass *priv_class;

	int (*init)(TTCodecContext *);
	int (*decode)(TTCodecContext *, void *outdata, int *outdata_size, TTPacket *avpkt);
	int (*close)(TTCodecContext *);
	int (*encode_sub)(TTCodecContext *, uint8_t *buf, int buf_size, const struct TTSubtitle *sub);
	int (*encode2)(TTCodecContext *avctx, TTPacket *avpkt, const TTFrame *frame, int *got_packet_ptr);
	void (*flush)(TTCodecContext *);

	const TTRational *supported_framerates;
	const int *supported_samplerates;      
	const enum AVSampleFormat *sample_fmts;
	const uint64_t *channel_layouts;       
	const TTProfile *profiles;  

	struct TTCodec *next;
	int (*init_thread_copy)(TTCodecContext *);
	int (*update_thread_context)(TTCodecContext *dst, const TTCodecContext *src);
	const TTCodecDefault *defaults;
	void (*init_static_data)(struct TTCodec *codec);

#if TT_API_LOWRES
	uint8_t max_lowres;                    
#endif
} TTCodec;




/**
 * Hardware acceleration should be used for decoding even if the codec level
 * used is unknown or higher than the maximum supported level reported by the
 * hardware driver.
 */
#define TTV_HWACCEL_FLAG_IGNORE_LEVEL (1 << 0)


typedef struct TTPicture {
    uint8_t *data[TTV_NUM_DATA_POINTERS];    ///< pointers to the image data planes
    int linesize[TTV_NUM_DATA_POINTERS];     ///< number of bytes per line
} TTPicture;

/**
 * @}
 */

enum TTSubtitleType {
    SUBTITLE_NONE,

    SUBTITLE_BITMAP,                ///< A bitmap, pict will be set

    /**
     * Plain text, the text field must be set by the decoder and is
     * authoritative. ass and pict fields may contain approximations.
     */
    SUBTITLE_TEXT,

    /**
     * Formatted text, the ass field must be set by the decoder and is
     * authoritative. pict and text fields may contain approximations.
     */
    SUBTITLE_ASS,
};

#define TTV_SUBTITLE_FLAG_FORCED 0x00000001

typedef struct TTSubtitleRect {
    int x;         ///< top left corner  of pict, undefined when pict is not set
    int y;         ///< top left corner  of pict, undefined when pict is not set
    int w;         ///< width            of pict, undefined when pict is not set
    int h;         ///< height           of pict, undefined when pict is not set
    int nb_colors; ///< number of colors in pict, undefined when pict is not set

    /**
     * data+linesize for the bitmap of this subtitle.
     * can be set for text/ass as well once they where rendered
     */
    TTPicture pict;
    enum TTSubtitleType type;

    char *text;                     ///< 0 terminated plain UTF-8 text

    /**
     * 0 terminated ASS/SSA compatible event line.
     * The presentation of this is unaffected by the other values in this
     * struct.
     */
    char *ass;

    int flags;
} TTSubtitleRect;

typedef struct TTSubtitle {
    uint16_t format; /* 0 = graphics */
    uint32_t start_display_time; /* relative to packet pts, in ms */
    uint32_t end_display_time; /* relative to packet pts, in ms */
    unsigned num_rects;
    TTSubtitleRect **rects;
    int64_t pts;    ///< Same as packet pts, in TTV_TIME_BASE
} TTSubtitle;

/**
 * If c is NULL, returns the first registered codec,
 * if c is non-NULL, returns the next registered codec after c,
 * or NULL if c is the last one.
 */
TTCodec *ttv_codec_next(const TTCodec *c);

/**
 * Register the codec codec and initialize libavcodec.
 *
 * @warning either this function or ttcodec_register() must be called
 * before any other libavcodec functions.
 *
 * @see ttcodec_register()
 */
void ttcodec_register_c(TTCodec *codec);

/**
 * Register all the codecs, parsers and bitstream filters which were enabled at
 * configuration time. If you do not call this function you can select exactly
 * which formats you want to support, by using the individual registration
 * functions.
 *
 * @see ttcodec_register_c
 * @see ttv_register_codec_parser
 * @see ttv_register_bitstream_filter
 */
void ttcodec_register(void);

/**
 * Allocate an TTCodecContext and set its fields to default values. The
 * resulting struct should be freed with ttcodec_free_context().
 *
 * @param codec if non-NULL, allocate private data and initialize defaults
 *              for the given codec. It is illegal to then call ttcodec_open2()
 *              with a different codec.
 *              If NULL, then the codec-specific defaults won't be initialized,
 *              which may result in suboptimal default settings (this is
 *              important mainly for encoders, e.g. libx264).
 *
 * @return An TTCodecContext filled with default values or NULL on failure.
 * @see ttcodec_get_context_defaults
 */
TTCodecContext *ttcodec_alloc_context3(const TTCodec *codec);


/**
 * Set the fields of the given TTCodecContext to default values corresponding
 * to the given codec (defaults may be codec-dependent).
 *
 * Do not call this function if a non-NULL codec has been passed
 * to ttcodec_alloc_context3() that allocated this TTCodecContext.
 * If codec is non-NULL, it is illegal to call ttcodec_open2() with a
 * different codec on this TTCodecContext.
 */
int ttcodec_get_context_defaults3(TTCodecContext *s, const TTCodec *codec);




int ttcodec_open2(TTCodecContext *avctx, const TTCodec *codec, TTDictionary **options);

/**
 * Close a given TTCodecContext and free all the data associated with it
 * (but not the TTCodecContext itself).
 *
 * Calling this function on an TTCodecContext that hasn't been opened will free
 * the codec-specific data allocated in ttcodec_alloc_context3() /
 * ttcodec_get_context_defaults3() with a non-NULL codec. Subsequent calls will
 * do nothing.
 */
int ttcodec_close(TTCodecContext *avctx);


/**
 * Initialize optional fields of a packet with default values.
 *
 * Note, this does not touch the data and size members, which have to be
 * initialized separately.
 *
 * @param pkt packet
 */
void tt_init_packet(TTPacket *pkt);



/**
 * Free a packet.
 *
 * @param pkt packet to free
 */
void tt_free_packet(TTPacket *pkt);

/**
 * Allocate new information of a packet.
 *
 * @param pkt packet
 * @param type side information type
 * @param size side information size
 * @return pointer to fresh allocated data or NULL otherwise
 */
uint8_t* ttv_packet_new_side_data(TTPacket *pkt, enum TTPacketSideDataType type,
                                 int size);



/**
 * Get side information from packet.
 *
 * @param pkt packet
 * @param type desired side information type
 * @param size pointer for side information size to store (optional)
 * @return pointer to data if present or NULL otherwise
 */
uint8_t* ttv_packet_get_side_data(TTPacket *pkt, enum TTPacketSideDataType type,
                                 int *size);



int ttv_packet_split_side_data(TTPacket *pkt);

/**
 * Unpack a dictionary from side_data.
 *
 * @param data data from side_data
 * @param size size of the data
 * @param dict the metadata storage dictionary
 * @return 0 on success, < 0 on failure
 */
int ttv_packet_unpack_dictionary(const uint8_t *data, int size, TTDictionary **dict);


/**
 * Convenience function to free all the side data stored.
 * All the other fields stay untouched.
 *
 * @param pkt packet
 */
void ttv_packet_free_side_data(TTPacket *pkt);

/**
 * Setup a new reference to the data described by a given packet
 *
 * If src is reference-counted, setup dst as a new reference to the
 * buffer in src. Otherwise allocate a new buffer in dst and copy the
 * data from src into it.
 *
 * All the other fields are copied from src.
 *
 * @see ttv_packet_unref
 *
 * @param dst Destination packet
 * @param src Source packet
 *
 * @return 0 on success, a negative AVERROR on error.
 */
int ttv_packet_ref(TTPacket *dst, const TTPacket *src);

/**
 * Wipe the packet.
 *
 * Unreference the buffer referenced by the packet and reset the
 * remaining packet fields to their default values.
 *
 * @param pkt The packet to be unreferenced.
 */
void ttv_packet_unref(TTPacket *pkt);


/**
 * Copy only "properties" fields from src to dst.
 *
 * Properties for the purpose of this function are all the fields
 * beside those related to the packet data (buf, data, size)
 *
 * @param dst Destination packet
 * @param src Source packet
 *
 * @return 0 on success AVERROR on failure.
 *
 */
int ttv_packet_copy_props(TTPacket *dst, const TTPacket *src);



/**
 * Find a registered decoder with a matching codec ID.
 *
 * @param id TTCodecID of the requested decoder
 * @return A decoder if one was found, NULL otherwise.
 */
TTCodec *ttcodec_find_decoder(enum TTCodecID id);


/**
 * The default callback for TTCodecContext.get_buffer2(). It is made public so
 * it can be called by custom get_buffer2() implementations for decoders without
 * CODEC_CAP_DR1 set.
 */
int ttcodec_default_get_buffer2(TTCodecContext *s, TTFrame *frame, int flags);



/**
 * Modify width and height values so that they will result in a memory
 * buffer that is acceptable for the codec if you also ensure that all
 * line sizes are a multiple of the respective linesize_align[i].
 *
 * May only be used if a codec with CODEC_CAP_DR1 has been opened.
 */
void ttcodec_align_dimensions2(TTCodecContext *s, int *width, int *height,
                               int linesize_align[TTV_NUM_DATA_POINTERS]);




/**
 * Decode the video frame of size avpkt->size from avpkt->data into picture.
 * Some decoders may support multiple frames in a single TTPacket, such
 * decoders would then just decode the first frame.
 *
 * @warning The input buffer must be TT_INPUT_BUFFER_PADDING_SIZE larger than
 * the actual read bytes because some optimized bitstream readers read 32 or 64
 * bits at once and could read over the end.
 *
 * @warning The end of the input buffer buf should be set to 0 to ensure that
 * no overreading happens for damaged MPEG streams.
 *
 * @note Codecs which have the CODEC_CAP_DELAY capability set have a delay
 * between input and output, these need to be fed with avpkt->data=NULL,
 * avpkt->size=0 at the end to return the remaining frames.
 *
 * @param avctx the codec context
 * @param[out] picture The TTFrame in which the decoded video frame will be stored.
 *             Use ttv_frame_alloc() to get an TTFrame. The codec will
 *             allocate memory for the actual bitmap by calling the
 *             TTCodecContext.get_buffer2() callback.
 *             When TTCodecContext.refcounted_frames is set to 1, the frame is
 *             reference counted and the returned reference belongs to the
 *             caller. The caller must release the frame using ttv_frame_unref()
 *             when the frame is no longer needed. The caller may safely write
 *             to the frame if ttv_frame_is_writable() returns 1.
 *             When TTCodecContext.refcounted_frames is set to 0, the returned
 *             reference belongs to the decoder and is valid only until the
 *             next call to this function or until closing or flushing the
 *             decoder. The caller may not write to it.
 *
 * @param[in] avpkt The input TTPacket containing the input buffer.
 *            You can create such packet with tt_init_packet() and by then setting
 *            data and size, some decoders might in addition need other fields like
 *            flags&TTV_PKT_FLAG_KEY. All decoders are designed to use the least
 *            fields possible.
 * @param[in,out] got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero.
 * @return On error a negative value is returned, otherwise the number of bytes
 * used or zero if no frame could be decompressed.
 */
int ttcodec_decode_video2(TTCodecContext *avctx, TTFrame *picture,
                         int *got_picture_ptr,
                         const TTPacket *avpkt);





enum TTPixelFormat ttcodec_default_get_format(struct TTCodecContext *s, const enum TTPixelFormat * fmt);


int ttcodec_default_execute(TTCodecContext *c, int (*func)(TTCodecContext *c2, void *arg2),void *arg, int *ret, int count, int size);
int ttcodec_default_execute2(TTCodecContext *c, int (*func)(TTCodecContext *c2, void *arg2, int, int),void *arg, int *ret, int count);


/**
 * Reset the internal decoder state / flush internal buffers. Should be called
 * e.g. when seeking or when switching to a different stream.
 *
 * @note when refcounted frames are not used (i.e. avctx->refcounted_frames is 0),
 * this invalidates the frames previously returned from the decoder. When
 * refcounted frames are used, the decoder just releases any references it might
 * keep internally, but the caller's reference remains valid.
 */
void ttcodec_flush_buffers(TTCodecContext *avctx);

void ttv_fast_padded_malloc(void *ptr, unsigned int *size, size_t min_size);


/**
 * Lock operation used by lockmgr
 */
enum AVLockOp {
  TTV_LOCK_CREATE,  ///< Create a mutex
  TTV_LOCK_OBTAIN,  ///< Lock the mutex
  TTV_LOCK_RELEASE, ///< Unlock the mutex
  TTV_LOCK_DESTROY, ///< Free mutex resources
};


/**
 * @return a positive value if s is open (i.e. ttcodec_open2() was called on it
 * with no corresponding ttcodec_close()), 0 otherwise.
 */
int ttcodec_is_open(TTCodecContext *s);



/**
 * @return a non-zero number if codec is a decoder, zero otherwise
 */
int ttv_codec_is_decoder(const TTCodec *codec);



#endif /* __TTPOD_TT_AVCODEC_H_ */
