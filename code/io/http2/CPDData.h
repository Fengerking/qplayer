/*******************************************************************************
	File:		CPDData.h

	Contains:	pd data header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-19		Bangfei			Create file

*******************************************************************************/
#ifndef __CPDData_H__
#define __CPDData_H__

#include "CBaseObject.h"
#include "CFileIO.h"
#include "CNodeList.h"
#include "CThreadWork.h"

typedef struct
{
	long long		llBeg;
	long long		llEnd;
} QCPD_POS_INFO;

class CPDData : public CBaseObject, public CThreadFunc
{
public:
	CPDData(CBaseInst * pBaseInst);
    virtual ~CPDData(void);

	virtual int 		Open (const char * pURL, long long llOffset, int nFlag);
	virtual int 		Close (void);

	virtual int			SetFileSize (long long llSize);
	virtual long long	GetFileSize (void) { return m_llFileSize; }
	virtual long long	GetDownPos(long long llPos);
	virtual bool		IsHadDownLoad(void) { return m_bDownLoad; }
	virtual bool		HadDownload(long long llPos, long long llSize);

	virtual int			ReadData (long long llPos, unsigned char * pBuff, int & nSize, int nFlag);
	virtual int			RecvData (long long llPos, unsigned char * pBuff, int nSize, int nFlag);
	virtual int			SaveData(long long llPos, unsigned char * pBuff, int nSize, int nFlag);

	virtual void		DeleteCacheFile(int nDelete) { m_nDeleteFile = nDelete; }

protected:
	virtual int			ParserInfo (const char * pURL);
	virtual int			CreatePDLFileName (const char * pURL);
	virtual int			OpenCacheFile (void);
	virtual int			SavePDLInfoFile(void);
	virtual int			AdjustSortList(void);

	virtual bool		IsMoovAtEnd(long long llPos);
	virtual void		SaveMoovData(void);
	virtual bool		RecordItem(long long llPos, unsigned char * pBuff, int nSize, int nFlag);

	virtual int			OnWorkItem(void);

protected:
	CFileIO *					m_pIOFile;
	char *						m_pMainURL;
	long long					m_llFileSize;

	bool						m_bDownLoad;
	bool						m_bModified;

	CMutexLock					m_mtLockHead;
	unsigned char *				m_pHeadBuff;
	int							m_nHeadSize;
	QCMP4_MOOV_BUFFER			m_moovData;
	bool						m_bMoovSave;

	char *						m_pPDLFileName;
	CObjectList<QCPD_POS_INFO>	m_lstPos;
	QCPD_POS_INFO *				m_pItem;
	NODEPOS						m_pPos;
	QCPD_POS_INFO **			m_ppPosList;
	int							m_nPosListNum;

	CMutexLock					m_mtLockData;
	CThreadWork *				m_pThreadWork;
	long long					m_llEmptySize;
	unsigned char *				m_pEmptyBuff;
	int							m_nEmptySize;
	int							m_nEmptyTime;

	int							m_nDeleteFile;

	// for debug
	unsigned char *				m_pSourceData;
	bool		CheckData(long long llPos, unsigned char * pBuff, int nSize);


protected:
	static int compareFilePos (const void *arg1, const void *arg2);

};

#endif // __CPDData_H__