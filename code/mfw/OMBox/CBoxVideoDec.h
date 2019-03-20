/*******************************************************************************
	File:		CBoxVideoDec.h

	Contains:	The video dec box header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-13		Bangfei			Create file

*******************************************************************************/
#ifndef __CBoxVideoDec_H__
#define __CBoxVideoDec_H__

#include "CBoxBase.h"
#include "CBaseVideoDec.h"
#include "CNodeList.h"

#include "UThreadFunc.h"

class CBoxVideoDec : public CBoxBase
{
public:
	CBoxVideoDec(CBaseInst * pBaseInst, void * hInst);
	virtual ~CBoxVideoDec(void);

	virtual int			SetSource (CBoxBase * pSource);
	virtual long long	SetPos (long long llPos);
	virtual void		Flush(void);
	virtual int			ReadBuff(QC_DATA_BUFF * pBuffInfo, QC_DATA_BUFF ** ppBuffData, bool bWait);

	virtual QC_VIDEO_FORMAT *	GetVideoFormat (int nID = -1);

protected:
	virtual int			CreateDec (QC_VIDEO_FORMAT * pFmt);

protected:
	CMutexLock					m_mtBuffer;
	CBaseVideoDec *				m_pDec;
	int							m_nOutCount;	

	QC_DATA_BUFF *				m_pNewBuff;

	QC_VIDEO_FORMAT				m_fmtVideo;


};

#endif // __CBoxVideoDec_H__
