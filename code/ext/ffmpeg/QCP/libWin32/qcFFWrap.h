/*******************************************************************************
	File:		qcFFWrap.h

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#ifndef __qcFFWrap_H__
#define __qcFFWrap_H__

#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavformat/avio.h>

#include "qcData.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
	AVIOContext *		m_pAVIO;
	void *				m_pQCIO;

	unsigned char *		m_pBuffer;
	int					m_nBuffSize;
}QCIOFuncInfo;

typedef struct
{
	AVFormatContext *			m_pFmtCtx;
	int							m_nIdxAudio;
	AVStream *					m_pStmAudio;
	int							m_nAudioNum;
	int							m_nIdxVideo;
	AVStream *					m_pStmVideo;
	int							m_nIdxSubtt;
	AVStream *					m_pStmSubtt;
	int							m_nPacketSize;
	AVDictionary *				m_pFmtOpts;
	AVPacket *					m_pPackData;
	long long					m_llDuration;
	BOOL						m_bEOS;
	BOOL						m_bLive;
	void *						m_pQCParser;
	QCIOFuncInfo *				m_pQCIOInfo;
}QCParser_Context;

typedef struct
{
	char *				m_pFileName;
	int					m_nFileFormat;
	AVFormatContext*	m_hCtx;
	AVStream *          m_pStmAudio;
	int                 m_nVideoCount;
	AVStream *          m_pStmVideo;

	long long			m_llStartTime;
	BOOL				m_bInitHeader;

	AVIOContext *		m_pAVIO;
	unsigned char *		m_pBuffer;
	int					m_nBuffSize;
	FILE *				m_hMuxFile;
}QCMuxInfo;

int		ffIO_Read(void *opaque, uint8_t *buf, int buf_size);
int		ffIO_Write(void *opaque, uint8_t *buf, int buf_size);
int64_t	ffIO_Seek(void *opaque, int64_t offset, int whence);

void	ffFormat_Init(QCParser_Context * pParser);
void	ffFormat_Uninit(QCParser_Context * pParser);

int 	ffFormat_Open(QCParser_Context * pParser, const char * pURL);
int 	ffFormat_Close(QCParser_Context * pParser);

int 	ffFormat_Read(QCParser_Context * pParser, QC_DATA_BUFF * pBuff);
int 	ffFormat_SetPos(QCParser_Context * pParser, long long llPos);

int		ffIO_Open(QCIOFuncInfo * pIOInfo);
int		ffIO_Close(QCIOFuncInfo * pIOInfo);

int		ffMux_Open(QCMuxInfo * pIOInfo);
int		ffMux_Close(QCMuxInfo * pIOInfo);
int		ffMux_AddTrack(QCMuxInfo * pMuxInfo, void * pFormat, QCMediaType nType);
int		ffMux_Write(QCMuxInfo * pMuxInfo, QC_DATA_BUFF* pBuff);

int		QCMUX_Read(void *opaque, uint8_t *buf, int buf_size);
int		QCMUX_Write(void *opaque, uint8_t *buf, int buf_size);
int64_t	QCMUX_Seek(void *opaque, int64_t offset, int whence);

long long	ffBaseToTime(long long llBase, AVStream * pStream);
long long	ffTimeToBase(long long llTime, AVStream * pStream);

int			CodecIdQplayer2FF(int nCodecType);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // __qcFFWrap_H__
