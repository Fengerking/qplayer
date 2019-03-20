/*******************************************************************************
	File:		CFLVTag.h

	Contains:	the flv tag parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-07		Bangfei			Create file

*******************************************************************************/
#ifndef __CFLVTag_H__
#define __CFLVTag_H__
#include "qcIO.h"
#include "qcData.h"
#include "qcParser.h"
#include "CBaseObject.h"

#include "CBuffMng.h"

#define FLV_HEAD_SIZE				 9
#define	FLV_PREVTAG_SIZE			 4
#define	FLV_TAG_HEAD_SIZE			 11

// offsets for packed values 
#define FLV_AUDIO_SAMPLESSIZE_OFFSET 1
#define FLV_AUDIO_SAMPLERATE_OFFSET  2
#define FLV_AUDIO_CODECID_OFFSET     4

#define FLV_VIDEO_FRAMETYPE_OFFSET   4

// bitmasks to isolate specific values
#define FLV_AUDIO_CHANNEL_MASK    0x01
#define FLV_AUDIO_SAMPLESIZE_MASK 0x02
#define FLV_AUDIO_SAMPLERATE_MASK 0x0c
#define FLV_AUDIO_CODECID_MASK    0xf0

#define FLV_VIDEO_CODECID_MASK    0x0f
#define FLV_VIDEO_FRAMETYPE_MASK  0xf0

#define AMF_END_OF_OBJECT         0x09

#define KEYFRAMES_TAG            "keyframes"
#define KEYFRAMES_TIMESTAMP_TAG  "times"
#define KEYFRAMES_BYTEOFFSET_TAG "filepositions"

enum {
    FLV_HEADER_FLAG_HASVIDEO = 1,
    FLV_HEADER_FLAG_HASAUDIO = 4,
};

enum {
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};

enum {
    FLV_STREAM_TYPE_VIDEO,
    FLV_STREAM_TYPE_AUDIO,
    FLV_STREAM_TYPE_DATA,
    FLV_STREAM_TYPE_NB,
};

enum {
    FLV_MONO   = 0,
    FLV_STEREO = 1,
};

enum {
    FLV_SAMPLESSIZE_8BIT  = 0,
    FLV_SAMPLESSIZE_16BIT = 1 << FLV_AUDIO_SAMPLESSIZE_OFFSET,
};

enum {
    FLV_SAMPLERATE_SPECIAL = 0, // signifies 5512Hz and 8000Hz in the case of NELLYMOSER 
    FLV_SAMPLERATE_11025HZ = 1 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_22050HZ = 2 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_44100HZ = 3 << FLV_AUDIO_SAMPLERATE_OFFSET,
};

enum {
    FLV_CODECID_PCM                  = 0,
    FLV_CODECID_ADPCM                = 1 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_MP3                  = 2 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_LE               = 3 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_16KHZ_MONO = 4 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_8KHZ_MONO = 5 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER           = 6 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_ALAW             = 7 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_MULAW            = 8 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_AAC                  = 10<< FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_SPEEX                = 11<< FLV_AUDIO_CODECID_OFFSET,
};

enum {
    FLV_CODECID_H263    = 2,
    FLV_CODECID_SCREEN  = 3,
    FLV_CODECID_VP6     = 4,
    FLV_CODECID_VP6A    = 5,
    FLV_CODECID_SCREEN2 = 6,
    FLV_CODECID_H264    = 7,
    FLV_CODECID_REALH263= 8,
    FLV_CODECID_MPEG4   = 9,
	FLV_CODECID_HEVC   = 12,
};

enum {
    FLV_FRAME_KEY            = 1 << FLV_VIDEO_FRAMETYPE_OFFSET, ///< key frame (for AVC, a seekable frame)
    FLV_FRAME_INTER          = 2 << FLV_VIDEO_FRAMETYPE_OFFSET, ///< inter frame (for AVC, a non-seekable frame)
    FLV_FRAME_DISP_INTER     = 3 << FLV_VIDEO_FRAMETYPE_OFFSET, ///< disposable inter frame (H.263 only)
    FLV_FRAME_GENERATED_KEY  = 4 << FLV_VIDEO_FRAMETYPE_OFFSET, ///< generated key frame (reserved for server use only)
    FLV_FRAME_VIDEO_INFO_CMD = 5 << FLV_VIDEO_FRAMETYPE_OFFSET, ///< video info/command frame
};

typedef enum {
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
} AMFDataType;

class CFLVParser;

class CFLVTag : public CBaseObject
{
public:
	CFLVTag(CBaseInst * pBaseInst, CBuffMng * pBuffMng, unsigned int streamType);
	virtual ~CFLVTag();

	void			SetParser(CFLVParser * pParser) { m_pParser = pParser; }
	int				AddTag (unsigned char *pData, unsigned int nSize, long long llTime);
	int				FillAudioFormat (QC_AUDIO_FORMAT * pFmt);
	int				FillVideoFormat (QC_VIDEO_FORMAT * pFmt);
    int				OnDisconnect();

    unsigned int	Type() const { return m_nStreamType; }


private:
	int		AddAudioTag(unsigned char * pData, unsigned int nSize, long long llTime);
	int		AddVideoTag(unsigned char * pData, unsigned int nSize, long long llTime);

private:
	CFLVParser *	m_pParser;
	CBuffMng *		m_pBuffMng;
	unsigned int	m_nStreamType;

	int				m_nNalLength;
	int				m_nVideoCodec;
	int				m_nWidth;
	int				m_nHeight;
	int				m_nNumRef;
	int				m_nSarWidth;
	int				m_nSarHeight;
	unsigned char *	m_pHeadVideoData;
	int				m_nHeadVideoSize;
	unsigned int	m_nSyncWord;
    QC_VIDEO_FORMAT	m_FmtVideo;

	int				m_nAudioCodec;
	int				m_nSampleRate;
	int				m_nChannel;
	int				m_nSampleBits;
	unsigned char *	m_pHeadAudioData;
	int				m_nHeadAudioSize;
    QC_AUDIO_FORMAT m_FmtAudio;

    bool			m_bDisconnect;

public:
	unsigned char *	m_pExtVideoData;
	int				m_nExtVideoSize;

public:
	static int	TagHeader (unsigned char *pData, unsigned int nSize, int &nStreamType, int &nDataSize, int &nTime);


};

#endif // __CFLVTag_H__
