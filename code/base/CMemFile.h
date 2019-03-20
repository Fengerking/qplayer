/*******************************************************************************
	File:		CMemFile.h

	Contains:	the memory file class header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#ifndef __CBuffTrace_H__
#define __CBuffTrace_H__
#include "qcDef.h"
#include "qcData.h"

#include "CBaseObject.h"

#include "CFileIO.h"
#include "CMutexLock.h"
#include "CNodeList.h"

#define MEM_BUFF_SIZE	32768

class CMemItem
{
public:
	CMemItem (void)
	{
		m_llPos = -1;
		m_pBuff = NULL;
		m_nDataSize = 0;
		m_nBuffSize = 0;
		m_bRead = false;
		m_nFlag = QCIO_READ_DATA;
	}
	virtual ~CMemItem (void)
	{
		QC_DEL_A (m_pBuff);
	}

public:
	long long		m_llPos;
	char *			m_pBuff;
	int				m_nDataSize;
	int				m_nBuffSize;
	bool			m_bRead;
	int				m_nFlag;
};

class CMemFile : public CBaseObject
{
public:
	CMemFile(CBaseInst * pBaseInst);
    virtual ~CMemFile(void);

	virtual int			ReadBuff (long long llPos, char * pBuff, int nSize, bool bFull, int nFlag);
	virtual int			FillBuff (long long llPos, char * pBuff, int nSize);
	virtual long long	SetPos (long long llPos);

	virtual int			GetBuffSize (long long llPos);
	virtual long long	GetStartPos (void);
	virtual long long	GetDownPos(void);

	virtual int			Reset (void);
	virtual int			CheckBuffSize (void);

	virtual int			SetMoovPos(long long llMoovPos);
	virtual int			SetDataPos(long long llDataPos);
	virtual void		ShowStatus(void);

	virtual void		SetOpenCache(bool bCache) { m_bOpenCache = bCache; }
	virtual void		SortFullList(void);
	virtual int			CopyOtherMem(void * pMemFile, unsigned char ** ppBuff, int * pSize);
	CObjectList<CMemItem> * GetFullList(void) { return &m_lstFull; }

protected:
	virtual CMemItem *	GetItem (int nSize = MEM_BUFF_SIZE);
	CMemItem *			FindItem (long long llPos);

	virtual int			CheckFreeItem (void);
	virtual int			CheckFullList (void);


protected:
	CMutexLock				m_mtLock;
	bool					m_bOpenCache;
	CObjectList<CMemItem>	m_lstFull;
	CObjectList<CMemItem>	m_lstFree;
	CObjectList<CMemItem>	m_lstHead;

	long long				m_llFillPos;
	long long				m_llReadPos;

	long long				m_llKeepSize;
	long long				m_llMoovPos;
	long long				m_llDataPos;

	long long				m_llPosAudio;
	long long				m_llPosVideo;
	long long				m_llBufAudio;
	long long				m_llBufVideo;

protected:
	static int compareMemPos(const void *arg1, const void *arg2);

};

#endif //__CBuffTrace_H__
