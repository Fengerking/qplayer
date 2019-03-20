/*******************************************************************************
	File:		CBoxAudioDec.h

	Contains:	the audio dec box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxAudioDec_H__
#define __CBoxAudioDec_H__

#include "CBoxBase.h"
#include "CBaseAudioDec.h"

#include "CFileIO.h"

class CBoxAudioDec : public CBoxBase
{
public:
	CBoxAudioDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxAudioDec(void);

	virtual int			SetSource (CBoxBase * pSource);
	virtual long long	SetPos (long long llPos);
	virtual void		Flush(void);
	virtual int			ReadBuff(QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);

	virtual int 		SetParam(int nID, void * pParam);

	virtual QC_AUDIO_FORMAT *	GetAudioFormat (int nID = -1);

protected:
	CBaseAudioDec *		m_pDec;
	QC_DATA_BUFF *		m_pCurrBuff;

	CFileIO *			m_pFile;
};

#endif // __CBoxAudioDec_H__
