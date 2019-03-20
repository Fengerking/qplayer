/*******************************************************************************
	File:		COpenHEVCDec.h

	Contains:	The openHEVC dec wrap header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-11		Bangfei			Create file

*******************************************************************************/
#ifndef __COpenHEVCDec_H__
#define __COpenHEVCDec_H__

#include "openHevcWrapper.h"
#include "CBaseVideoDec.h"

typedef OpenHevc_Handle (* LIBOPENHEVCINIT) (int nb_pthreads, int thread_type);
typedef int (* LIBOPENHEVCSTARTDECODER) (OpenHevc_Handle openHevcHandle);
typedef int (* LIBOPENHEVCDECODE) (OpenHevc_Handle openHevcHandle, const unsigned char *buff, int nal_len, int64_t pts);
typedef void (* LIBOPENHEVCGETPICTUREINFO) (OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo);
typedef void (* LIBOPENHEVCCOPYEXTRADATA) (OpenHevc_Handle openHevcHandle, unsigned char *extra_data, int extra_size_alloc);
typedef void (* LIBOPENHEVCGETPICTURESIZE2) (OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo);
typedef int  (* LIBOPENHEVCGETOUTPUT) (OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame *openHevcFrame);
typedef int (*LIBOPENHEVCGETOUTPUTCPY)(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame_cpy *openHevcFrame);
typedef void (* LIBOPENHEVCSETCHECKMD5) (OpenHevc_Handle openHevcHandle, int val);
typedef void (* LIBOPENHEVCSETDEBUGMODE) (OpenHevc_Handle openHevcHandle, int val);
typedef void (* LIBOPENHEVCSETTEMPORALLAYER_ID) (OpenHevc_Handle openHevcHandle, int val);
typedef void (* LIBOPENHEVCSETNOCROPPING) (OpenHevc_Handle openHevcHandle, int val);
typedef void (* LIBOPENHEVCSETACTIVEDECODERS) (OpenHevc_Handle openHevcHandle, int val);
typedef void (* LIBOPENHEVCCLOSE) (OpenHevc_Handle openHevcHandle);
typedef void (* LIBOPENHEVCFLUSH) (OpenHevc_Handle openHevcHandle);
typedef const char * (* LIBOPENHEVCVERSION) (OpenHevc_Handle openHevcHandle);

class COpenHEVCDec : public CBaseVideoDec
{
public:
	COpenHEVCDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~COpenHEVCDec(void);

	virtual int		Init (QC_VIDEO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		Flush (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

protected:
	void *							m_hDll;
	OpenHevc_Handle					m_hDec;

	LIBOPENHEVCINIT					m_fInit;
	LIBOPENHEVCSTARTDECODER			m_fStart;
	LIBOPENHEVCDECODE				m_fDec;
	LIBOPENHEVCGETPICTUREINFO		m_fGetPicInfo;
	LIBOPENHEVCCOPYEXTRADATA		m_fCopyExtData;
	LIBOPENHEVCGETPICTURESIZE2		m_fGetPicSize2;
	LIBOPENHEVCGETOUTPUT			m_fGetOutput;
	LIBOPENHEVCGETOUTPUTCPY			m_fGetOutputCopy;
	LIBOPENHEVCSETCHECKMD5			m_fSetCheckMD5;
	LIBOPENHEVCSETDEBUGMODE			m_fSetDebugMode;
	LIBOPENHEVCSETTEMPORALLAYER_ID	m_fSetTempLayer;
	LIBOPENHEVCSETNOCROPPING		m_fSetNoCrop;
	LIBOPENHEVCSETACTIVEDECODERS	m_fSetActiveDec;
	LIBOPENHEVCCLOSE				m_fClose;
	LIBOPENHEVCFLUSH				m_fFlush;
	LIBOPENHEVCVERSION				m_fGetVer;

	OpenHevc_Frame					m_frmVideo;
	OpenHevc_Frame_cpy				m_frmCopy;

	long long						m_llNewPos;
};

#endif // __COpenHEVCDec_H__
