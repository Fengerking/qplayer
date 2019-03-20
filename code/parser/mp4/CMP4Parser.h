/*******************************************************************************
	File:		CMP4Parser.h

	Contains:	the base parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-04		Bangfei			Create file

*******************************************************************************/
#ifndef __CMP4Parser_H__
#define __CMP4Parser_H__

#include "../CBaseParser.h"
#include "CBaseIO.h"
#include "CIOReader.h"
#include "CNodeList.h"

#define  QC_BOX_FLAG_STSD  0x00000001
#define  QC_BOX_FLAG_STTS  0x00000002
#define  QC_BOX_FLAG_STSC  0x00000004
#define  QC_BOX_FLAG_STCO  0x00000008
#define  QC_BOX_FLAG_STSZ  0x00000010
#define  QC_BOX_FLAG_STSS  0x00000020
#define  QC_BOX_FLAG_CTTS  0x00000040

#define	 QC_MP4DATA_INTERLACE	0
#define	 QC_MP4DATA_AUDIO_VIDEO	1
#define	 QC_MP4DATA_VIDEO_AUDIO	2

#define  QC_BOX_MOOV_THREAD

#define SKIP_BYTES(pos, len, skips)	{	\
	(pos) += (skips);					\
	(len) -= (skips);					\
		}

enum QCMediaStreamId
{
	EMediaStreamIdNone		= -1
	, EMediaStreamIdAudioL	= 10	//���������KAudioIdElementIdMap[0]
	, EMediaStreamIdAudioR	= 11	//���������KAudioIdElementIdMap[1]
	, EMediaStreamIdVideo	= 100	//���������KAudioIdElementIdMap[2]
	, EMediaStreamMaxCount	= 0x7fffffff
};

typedef struct 
{
	unsigned int	iSampleCount;
	unsigned int	iSampleOffset;
}TCompositionTimeSample;

typedef struct 
{
	unsigned int	iSampleCount;
	unsigned int	iSampleDelta;
}TTimeToSample;

typedef struct 
{
	long long		iFirstChunk;
	unsigned int	iSampleNum;
}TTChunkToSample;

typedef struct
{
	int				iTrackDur;
	int				iMediaTime;
	int				iMediaRate;
}TTEditListInfo;

typedef struct  
{
	unsigned int	iSampleIdx;
	long long		iSampleFileOffset;
	unsigned int	iSampleEntrySize;
	unsigned int	iFlag;
	long long		iSampleTimeStamp;
}TTSampleInfo;

typedef struct
{
	long long		llMoofOffset;
	long long		llMoofTime;
}TTMoofIndexInfo;

typedef struct 
{
	TTimeToSample*					iTimeToSampleTab;
	int								iTimeToSampleEntryNum;

	TCompositionTimeSample*			iComTimeSampleTab;
	int								iComTimeSampleEntryNum;

	TTChunkToSample*				iChunkToSampleTab;
	int								iChunkToSampleEntryNum;
	
	long long*						iChunkOffsetTab;
	int								iChunkOffsetEntryNum;

	int								iConstantSampleSize;
	int*							iVariableSampleSizeTab;

	int*							iKeyFrameSampleTab;
	int								iKeyFrameSampleEntryNum;

	unsigned int					iFrameTime;
	QCMP4DecoderSpecificInfo*		iMP4DecoderSpecificInfo;
	QCM4AWaveFormat*				iM4AWaveFormat;
	QCAVCDecoderSpecificInfo*		iAVCDecoderSpecificInfo;

	int								iSampleCount;
	int								iSampleBuild;
	TTSampleInfo*					iSampleInfoTab;

	long long						iDuration;
	long long						iTotalSize;
	int								iScale;
	int								iAudio;
	int								iWidth;
	int								iHeight;
	int								iNum;
	int								iDen;
	int								iCodecType;
	int								iFourCC;
	int								iStreamID;
	int								iMaxFrameSize;
	unsigned int					iReadBoxFlag;
	unsigned char					iLang_code[4];
	int								iErrorTrackInfo;

	// empty duration of the first edit list entry
	long long						iEmptyDuration;
	// start time of the media
	long long						iStartTime;
	long long						iTimeOffset;

	// for improve open speed with long time
	long long						lCTTSStartPos;
	int								nCTTSBuffSize;
	long long						lSTSCStartPos;
	int								nSTSCBuffSize;
	long long						lSTCOStartPos;
	int								nSTCOBuffSize;
	long long						lCO64StartPos;
	int								nCO64BuffSize;
	long long						lSTSZStartPos;
	int								nSTSZBuffSize;
	long long						lSTTSStartPos;
	int								nSTTSBuffSize;
	long long						lSTSSStartPos;
	int								nSTSSBuffSize;

	// for fragement mp4
	int								nTrackId;
	long long						llTrackEnd;
}QCMP4TrackInfo;

typedef struct QCMP4Fragment {
	unsigned	track_id;
	long long	base_data_offset;
	long long	moof_offset;
	long long	implicit_offset;
	unsigned	stsd_id;
	unsigned	duration;
	unsigned	size;
	unsigned	flags;
	long long	time;
	QCMP4TrackInfo * pTrackInfo;
} QCMP4Fragment;

typedef struct QCMP4TrackExt {
	unsigned		track_id;
	unsigned		stsd_id;
	unsigned		duration;
	unsigned		size;
	unsigned		flags;
} QCMP4TrackExt;

class CMP4Parser : public CBaseParser, public CMP4IOReader
{
public:
    CMP4Parser(CBaseInst * pBaseInst);
    virtual ~CMP4Parser(void);

	virtual int			Open (QC_IO_Func * pIO, const char * pURL, int nFlag);
	virtual int 		Close (void);

	virtual int			SetStreamPlay (QCMediaType nType, int nStream);

	virtual int			Read (QC_DATA_BUFF * pBuff);

	virtual int		 	CanSeek (void);
	virtual long long 	GetPos (void);
	virtual long long 	SetPos (long long llPos);

	virtual int 		GetParam (int nID, void * pParam);
	virtual int 		SetParam (int nID, void * pParam);

	virtual int			MP4ReadAt(long long llPos, unsigned char * pBuff, int nSize, int nFlag);

protected:
	virtual int			ReadSourceData (long long llPos, unsigned char * pBuff, int nSize, int nFlag);
	virtual int			CreateNewIO(void);
	virtual int			UpdateFormat(bool bAudio);
	virtual int			CheckDataInterlace(void);
	virtual int			CreateHeadDataBuff (QC_DATA_BUFF * pBuff);

	int					ReadBoxMoov(long long aBoxPos, long long aBoxLen);
	int					UpdateTrackInfo(void);
	int					RemoveTrackInfo(QCMP4TrackInfo*	pTrackInfo);
	int					BuildSampleTab(QCMP4TrackInfo*	pTrackInfo);
	int					GetCompositionTimeOffset(QCMP4TrackInfo* pTrackInfo, int aIndex, int& aCurIndex, int& aCurCom);

	int					ReadBoxStco(long long aBoxPos, unsigned int aBoxLen, int nBits = 32);
	int					ReadBoxCo64(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStsc(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxCtts(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStsz(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStts(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStss(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStsd(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStsdVide(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxStsdSoun(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxAvcC(long long aBoxPos,unsigned int aBoxLen);
	int					ReadBoxHevC(long long aBoxPos,unsigned int aBoxLen);
	int					ReadBoxEsds(long long aBoxPos, unsigned int aBoxLen);

	// for fragement mp4
	int					ReadBoxMvex(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxMoof(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxMfhd(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxTraf(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxTfhd(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxTfdt(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxTrun(long long aBoxPos, unsigned int aBoxLen);
	int					ReadBoxSidx(long long aBoxPos, unsigned int aBoxLen);
	QCMP4TrackInfo *	GetTrackInfo(int nTrackID);
	int					ReadSampleData(TTSampleInfo * pSampleInfo, QC_DATA_BUFF * pBuff);
	int					SetFragPos(long long llPos);


	// Add Bang fix issue av sync.
	int					ReadBoxEdts(long long aBoxPos, unsigned int aBoxLen);

	int					ParseEsDescriptor(long long aDesPos, unsigned int aDesLen);
	int					ParseDecoderConfigDescriptor(long long aDesPos, unsigned int aDesLen);
	int					ParseDecoderSpecificInfo(long long aDesPos, unsigned int aDesLen);
	int					ParseSLConfigDescriptor(long long aDesPos, unsigned int aDesLen);

	int					LocationBox(long long& aLocation, long long& aBoxSize, const char * aBoxType, bool bInBox);
	int					ParseDescriptorLength(long long& aDesPos, unsigned int& aDesLen, unsigned int& aDescriptorLen);
	int					ParseM4AWaveFormat(QCMP4DecoderSpecificInfo* aDecoderSpecificInfo, QCM4AWaveFormat* aWaveFormat);

	double				av_display_rotation_get(const int matrix[9]);

protected:
	char *								m_pSourceURL;
	CIOReader *							m_pIOReader;
	unsigned int						m_nTimeScale;
	unsigned int						m_nDuration;
	unsigned int						m_nTotalBitrate;
	unsigned int						m_nDataInterlace; // 0 interlace 1 audio video  2 video audio

	CObjectList<QCMP4TrackInfo>			m_lstAudioTrackInfo;
	QCMP4TrackInfo*						m_pAudioTrackInfo;
	QCMP4TrackInfo*						m_pVideoTrackInfo;
	QCMP4TrackInfo*						m_pCurTrackInfo;
	TTSampleInfo*						m_pCurAudioInfo;
	TTSampleInfo*						m_pCurVideoInfo;

	bool								m_bADTSHeader;
	long long							m_llRawDataBegin;
	long long							m_llRawDataEnd;

	bool								m_bReadVideoHead;
	bool								m_bReadAudioHead;

	int									movie_display_matrix[3][3]; // display matrix from mvhd

	// the audio and video data pos is not in same pos
	QC_IO_Func *						m_fIONew;

	int									m_nAudioLoopTimes;
	int									m_nVideoLoopTimes;

	bool								m_bNotifyDLPercent;

	// for improve open speed
	CMutexLock					m_mtSample;
	QCIOProtocol				m_nIOProtocol;
	int							m_nBoxSkipSize;
	int							m_nPreLoadSamples;
	int							m_nConnectTime;
	bool						m_bBuildSample;

	// for encrype data
	int							m_nMP4KeySize;
	char						m_szCompKeyTxt[16];
	char						m_szFileKeyTxt[16];

	// for fragement mp4 file
	QCMP4Fragment *				m_pFragment;
	QCMP4TrackExt *				m_pTrackExt;
	int							m_nTrexCount;
	int							m_nTrackIndex;
	long long					m_llMoofFirstPos;
	long long					m_llMoofNextPos;
	bool						m_bMoofIndexTab;

	CObjectList<TTMoofIndexInfo>	m_lstMoofInfo;
	CObjectList<TTSampleInfo>		m_lstSampleInfo;
	long long						m_llLoopTime;

protected:
	static	int			MoovWorkProc(void * pParam);
	virtual int			ReadMoovData(QCMP4TrackInfo * pTrackInfo);
	virtual int			ReadHttpData(void * pIO, long long llStartPos, char * pBuff, int nSize);

	qcThreadHandle		m_hMoovThread;
};

#endif // __CBaseParser_H__