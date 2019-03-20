/**
* File : TTMP4Parser.h 
* Description : TTMP4Parserþ
*/

#ifndef __TT_MP4_PARSER_H__
#define __TT_MP4_PARSER_H__

#include "TTMediaParser.h"
#include "M4a.h"
//#include "ALACAudioTypes.h"
#include "AVCDecoderTypes.h"

#define  TT_BOX_FLAG_STSD  0x00000001
#define  TT_BOX_FLAG_STTS  0x00000002
#define  TT_BOX_FLAG_STSC  0x00000004
#define  TT_BOX_FLAG_STCO  0x00000008
#define  TT_BOX_FLAG_STSZ  0x00000010
#define  TT_BOX_FLAG_STSS  0x00000020
#define  TT_BOX_FLAG_CTTS  0x00000040

typedef struct 
{
	TTUint iSampleCount;
	TTUint iSampleOffset;
}TCompositionTimeSample;

typedef struct 
{
	TTUint iSampleCount;
	TTUint iSampleDelta;
}TTimeToSample;

typedef struct 
{
	TTUint64 iFirstChunk;
	TTUint iSampleNum;
}TTChunkToSample;

typedef struct  
{
	TTUint iSampleIdx;
	TTUint64 iSampleFileOffset;
	TTUint iSampleEntrySize;
	TTUint iFlag;
	TTInt64 iSampleTimeStamp;
}TTSampleInfo;

typedef struct 
{
	TCompositionTimeSample*				iComTimeSampleTab;
	TTInt								iComTimeSampleEntryNum;

	TTimeToSample*						iTimeToSampleTab;
	TTInt								iTimeToSampleEntryNum;

	TTInt								iConstantSampleSize;
	TTInt								iSampleCount;
	TTInt*								iVariableSampleSizeTab;

	TTChunkToSample*					iChunkToSampleTab;
	TTInt								iChunkToSampleEntryNum;
	
	TTInt64*							iChunkOffsetTab;
	TTInt								iChunkOffsetEntryNum;

	TTInt*								iKeyFrameSampleTab;
	TTInt								iKeyFrameSampleEntryNum;

	TTUint								iFrameTime;
	TTMP4DecoderSpecificInfo*			iMP4DecoderSpecificInfo;
	TTM4AWaveFormat*					iM4AWaveFormat;
	TTAVCDecoderSpecificInfo*			iAVCDecoderSpecificInfo;

	TTSampleInfo*						iSampleInfoTab;

	TTInt64								iDuration;
	TTUint64							iTotalSize;
	TTInt								iScale;
	TTInt								iAudio;
	TTInt								iWidth;
	TTInt								iHeight;
	TTInt								iCodecType;
	TTInt								iFourCC;
	TTInt								iStreamID;
	TTInt								iMaxFrameSize;
	TTUint32							iReadBoxFlag;
	TTUint8								iLang_code[4];
	TTInt								iErrorTrackInfo;
}TTMP4TrackInfo;


class CTTMP4Parser : public CTTMediaParser
{
public: // Constructors and destructor

	/**
	* \fn                               ~CTTMP4Parser.
	* \brief                            C++ destructor.
	*/
	~CTTMP4Parser();

	/**
	* \fn                               CTTAPEParser.
	* \brief                            C++ constructor.
	*/
	CTTMP4Parser(ITTDataReader& aDataReader, ITTMediaParserObserver& aObserver);


public: // Functions from ITTMediaParser

	/**
	* \fn								TTInt Parse(TTMediaInfo& aMediaInfo);
	* \brief							½âÎöÃ½ÌåÔ´
	* \param[out] 	aMediaInfo			½âÎöºÃµÄÃ½ÌåÐÅÏ¢
	* \return							³É¹¦·µ»ØKErrNone£¬·ñÔò·µ»Ø´íÎóÖµ¡£
	*/
	virtual TTInt						Parse(TTMediaInfo& aMediaInfo);

	/**
	* \fn                               TTimeIntervalMicroSeconds32 MediaDuration()
	* \brief                            »ñÈ¡Ã½ÌåÊ±³¤(ºÁÃë)
	* \param[in]						Á÷Id
	* \return                           Ã½ÌåÊ±³¤
	*/
	virtual TTUint						MediaDuration(TTInt aStreamId);

	/**
	* \fn								void Seek(TTUint aPosMS);
	* \brief							Seek²Ù×÷
	* \param[in]	aPosMS				Î»ÖÃ£¬µ¥Î»£ººÁÃë
	* \return							·µ»ØÕæÕýseekµÄÎ»ÖÃ£¬Èç¹ûÊÇ¸ºÊý£¬ÄÇÊÇseek Ê§°Ü¡£
	*/
	virtual TTInt64						Seek(TTUint64 aPosMS, TTInt aOption);

	/**
	* \fn								TInt GetFrameLocation(TInt aStreamId, TInt& aFrmIdx, TTUint aTime)
	* \brief							¸ù¾ÝÊ±¼ä»ñÈ¡Ö¡Ë÷ÒýÎ»ÖÃ
	* \param[in]	aStreamId			Ã½ÌåÁ÷ID
	* \param[out]	aFrmIdx				Ö¡Ë÷Òý
	* \param[in]	aTime				Ê±¼ä
	* \return							³É¹¦·µ»ØKErrNone£¬·ñÔò·µ»Ø´íÎóÖµ¡£
	*/
	virtual TTInt						GetFrameLocation(TTInt aStreamId, TTInt& aFrmIdx, TTUint64 aTime);

	/**
	* \fn								TTMediaInfo GetMediaSample();
	* \brief							»ñµÃaudio£¬video£¬µÈsampleÐÅÏ¢
	* \param[in]	aUrl				ÎÄ¼þÂ·¾¶
	*/
	virtual TTInt			 			GetMediaSample(TTMediaType aStreamType, TTBuffer* pMediaBuffer);

private:

	/**
	* \fn								TTInt SeekWithinFrmPosTab(TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo)
	* \brief							ÔÚÖ¡Ë÷Òý±íÖÐ²éÕÒÖ¡Î»ÖÃ
	* \param[in] 	aFrmIdx				Ö¡ÐòºÅ
	* \param[out] 	aFrameInfo			Ö¡Î»ÖÃ
	* \return							²Ù×÷ÊÇ·ñ³É¹¦
	*/
	virtual TTInt						SeekWithinFrmPosTab(TTInt aStreamId, TTInt aFrmIdx, TTMediaFrameInfo& aFrameInfo);


private:

	TTInt								ReadBoxStco(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxCo64(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStsc(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxCtts(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStsz(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStts(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStss(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStsd(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStsdVide(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxStsdSoun(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ReadBoxAvcC(TTUint64 aBoxPos,TTUint32 aBoxLen);
	TTInt								ReadBoxHevC(TTUint64 aBoxPos,TTUint32 aBoxLen);
	TTInt								ReadBoxEsds(TTUint64 aBoxPos, TTUint32 aBoxLen);
	TTInt								ParseEsDescriptor(TTUint64 aDesPos, TTUint32 aDesLen);
	TTInt								ParseDecoderConfigDescriptor(TTUint64 aDesPos, TTUint32 aDesLen);
	TTInt								ParseDecoderSpecificInfo(TTUint64 aDesPos, TTUint32 aDesLen);
	TTInt								ParseSLConfigDescriptor(TTUint64 aDesPos, TTUint32 aDesLen);
	TTInt								ReadBoxMoov(TTUint64 aBoxPos, TTUint64 aBoxLen);
	TTInt								LocationBox(TTUint64& aLocation, TTUint64& aBoxSize, const TTChar* aBoxType);
	TTInt								updateTrackInfo();
	TTInt								removeTrackInfo(TTMP4TrackInfo*	pTrackInfo);
	TTInt								buildSampleTab(TTMP4TrackInfo*	pTrackInfo);
	TTInt								getCompositionTimeOffset(TTMP4TrackInfo* pTrackInfo, TTInt aIndex, TTInt& aCurIndex, TTInt& aCurCom);

	TTInt								findNextSyncFrame(TTSampleInfo* pTrackInfo, TTSampleInfo* pCurInfo, TTInt64 nTimeStamp);

private:

	TTUint								iDuration;
	TTUint								iTotalBitrate;
	
	RGKPointerArray<TTMP4TrackInfo>		iAudioTrackInfoTab;
	TTMP4TrackInfo*						iVideoTrackInfo;

	TTMP4TrackInfo*						iCurTrackInfo;

	TTSampleInfo*						iCurAudioInfo;
	TTSampleInfo*						iCurVideoInfo;
};

#endif // __TT_MP4_PARSER_H__
