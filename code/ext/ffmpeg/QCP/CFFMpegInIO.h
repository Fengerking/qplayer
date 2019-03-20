/*******************************************************************************
	File:		CFFMpegInIO.h

	Contains:	the ffmpeg io header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-07		Bangfei			Create file

*******************************************************************************/
#ifndef __CFFMpegInIO_H__
#define __CFFMpegInIO_H__
#include "qcIO.h"
#include "qcData.h"

#include <libavformat/avio.h>

class CFFMpegInIO
{
public:
    CFFMpegInIO(void);
    virtual ~CFFMpegInIO(void);

	virtual int		Open (QC_IO_Func * pQCIO, const char * pSource);
	virtual int		Close (void);

	AVIOContext *	GetAVIIO (void) { return m_pAVIO; }

protected:
	virtual int		Read (uint8_t *buf, int buf_size);
	virtual int		Write (uint8_t *buf, int buf_size, long long llPos);
	virtual int64_t	Seek(int64_t offset, int whence);

protected:
	AVIOContext *		m_pAVIO;
	QC_IO_Func *		m_pQCIO;

	unsigned char *		m_pBuffer;
	int					m_nBuffSize;

public:
	static int		QCFF_Read (void *opaque, uint8_t *buf, int buf_size);
	static int		QCFF_Write (void *opaque, uint8_t *buf, int buf_size);
	static int64_t	QCFF_Seek (void *opaque, int64_t offset, int whence);
	static int		QCFF_Pasue (void *opaque, int pause);

};
/*/
int(*read_packet)(void *opaque, uint8_t *buf, int buf_size);
int(*write_packet)(void *opaque, uint8_t *buf, int buf_size);
int64_t(*seek)(void *opaque, int64_t offset, int whence);
int(*read_pause)(void *opaque, int pause);
*/

#endif // __CFFMpegInIO_H__
