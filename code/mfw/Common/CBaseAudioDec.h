/*******************************************************************************
	File:		CBaseAudioDec.h

	Contains:	The audio dec header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseAudioDec_H__
#define __CBaseAudioDec_H__

#include "CBaseObject.h"
#include "CMutexLock.h"

#include "qcData.h"

class CBaseAudioDec : public CBaseObject
{
public:
	CBaseAudioDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBaseAudioDec(void);

	virtual int		Init (QC_AUDIO_FORMAT * pFmt);
	virtual int		Uninit (void);

	virtual int		SetBuff (QC_DATA_BUFF * pBuff);
	virtual int		GetBuff (QC_DATA_BUFF ** ppBuff);

	virtual int		Start (void);
	virtual int		Pause (void);
	virtual int		Stop (void);
	virtual int		Flush (void);

	virtual int		SetVolume(int nVolume) { m_nVolume = nVolume; return QC_ERR_NONE; }

	virtual QC_AUDIO_FORMAT *	GetAudioFormat (void) {return &m_fmtAudio;}
protected:
    virtual int UpdateAudioVolume(unsigned char* pBuffer, int nSize);

protected:
	void *				m_hInst;
	QC_AUDIO_FORMAT		m_fmtAudio;
	int					m_nVolume;

	CMutexLock			m_mtBuffer;
	unsigned int		m_uBuffFlag;
	QC_DATA_BUFF *		m_pBuffData;
	int					m_nDecCount;
};

#endif // __CBaseAudioDec_H__
