/*******************************************************************************
 File:        CBaseFFMuxer.h
 
 Contains:    CBaseFFMuxer define header file.
 
 Written by:    Jun Lin
 
 Change History (most recent first):
 2018-01-06        Jun            Create file
 
 *******************************************************************************/

#ifndef CBaseFFMuxer_hpp
#define CBaseFFMuxer_hpp

#include "qcMuxer.h"
//#include "CBaseObject.h"
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

class CBaseFFMuxer// : public CBaseObject
{
public:
    CBaseFFMuxer(QCParserFormat nFormat);
    virtual ~CBaseFFMuxer(void);
    
public:
    virtual int Open(const char * pURL);
    virtual int Close();
	virtual int Init(void * pVideoFmt, void * pAudioFmt);
    virtual int Write(QC_DATA_BUFF* pBuffer);
    virtual int GetParam(int nID, void * pParam);
    virtual int SetParam(int nID, void * pParam);

protected:
	int 		AddAudioTrack(QC_AUDIO_FORMAT * pFmt);
	int         AddVideoTrack(QC_VIDEO_FORMAT * pFmt);
    int			WriteAudioSample(QC_DATA_BUFF* pBuff);
    int         WriteVideoSample(QC_DATA_BUFF* pBuff);
    
    int			CodecIdQplayer2FF(int nCodecType);
    long long   ffBaseToTime(long long llBase, AVStream * pStream);
    long long   ffTimeToBase(long long llTime, AVStream * pStream);
    
protected:
    int m_nFileFormat;
    AVFormatContext*	m_hCtx;

    // Define audio stream parameters
    AVStream *          m_pStmAudio;
    // Define video stream parameters
    int                 m_nVideoCount;
    AVStream *          m_pStmVideo;

    char*	   			m_pFileName;
    long long			m_llStartTime;
	bool				m_bInitHeader;

protected:
	virtual int			MuxRead(uint8_t *buf, int buf_size);
	virtual int			MuxWrite(uint8_t *buf, int buf_size);
	virtual int64_t		MuxSeek(int64_t offset, int whence);

protected:
	AVIOContext *		m_pAVIO;
	unsigned char *		m_pBuffer;
	int					m_nBuffSize;
	FILE *				m_hMuxFile;

public:
	static int			QCMUX_Read(void *opaque, uint8_t *buf, int buf_size);
	static int			QCMUX_Write(void *opaque, uint8_t *buf, int buf_size);
	static int64_t		QCMUX_Seek(void *opaque, int64_t offset, int whence);
	static int			QCMUX_Pasue(void *opaque, int pause);

};

#endif /* CBaseFFMuxer_hpp */
