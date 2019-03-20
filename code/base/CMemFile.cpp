/*******************************************************************************
	File:		CMemFile.cpp

	Contains:	the memory file class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcIO.h"
#include <stdlib.h>

#include "CMemFile.h"

#include "CPDData.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CMemFile::CMemFile(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_bOpenCache(false)
	, m_llReadPos (-1)
	, m_llFillPos (-1)
	, m_llPosAudio (0)
	, m_llPosVideo (0)
	, m_llBufAudio (0)
	, m_llBufVideo (0)
{
	SetObjectName ("CMemFile");
	m_llKeepSize = 1024 * 1024 * 8;
	m_llMoovPos = 0;
	m_llDataPos = 0;
}

CMemFile::~CMemFile(void)
{
	CAutoLock lock (&m_mtLock);
	CMemItem * pItem = m_lstFree.RemoveHead ();
	while (pItem != NULL)
	{
		QC_DEL_A (pItem->m_pBuff);
		QC_DEL_P (pItem);
		pItem = m_lstFree.RemoveHead ();
	}
	pItem = m_lstFull.RemoveHead ();
	while (pItem != NULL)
	{
		QC_DEL_A (pItem->m_pBuff);
		QC_DEL_P (pItem);
		pItem = m_lstFull.RemoveHead ();
	}
}

int CMemFile::ReadBuff (long long llPos, char * pBuff, int nSize, bool bFull, int nFlag)
{
	CAutoLock lock (&m_mtLock);
	if (bFull)
	{
		if (GetBuffSize(llPos) < nSize)
			return 0;
	}

	long long	llReadPos = 0;
	int			nCopySize = 0;
	int			nReadSize = 0;
	int			nRestSize = nSize;
	CMemItem *	pItem = NULL;
	NODEPOS		pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (llReadPos > 0)
		{
			if (pItem->m_llPos == llReadPos)
			{
				nCopySize = nRestSize;
				if (llReadPos + nCopySize > pItem->m_llPos + pItem->m_nDataSize)
					nCopySize = (int)(pItem->m_llPos + pItem->m_nDataSize - llReadPos);
				memcpy(pBuff + nReadSize, pItem->m_pBuff + (int)(llReadPos - pItem->m_llPos), nCopySize);
				nRestSize -= nCopySize;
				llReadPos += nCopySize;
				nReadSize += nCopySize;
				if (nRestSize <= 0)
					break;
			}
			else
			{
				break;
			}
		}
		if (llPos >= pItem->m_llPos && llPos < pItem->m_llPos + pItem->m_nDataSize)
		{
			llReadPos = llPos;
			nCopySize = nRestSize;
			if (llReadPos + nCopySize > pItem->m_llPos + pItem->m_nDataSize)
				nCopySize = (int)(pItem->m_llPos + pItem->m_nDataSize  - llReadPos);
			memcpy(pBuff + nReadSize, pItem->m_pBuff + (int)(llReadPos - pItem->m_llPos), nCopySize);
			nRestSize -= nCopySize;
			llReadPos += nCopySize;
			nReadSize += nCopySize;
			if (nRestSize <= 0)
				break;
		}
	}
	if (nReadSize <= 0)
		return 0;

	m_llReadPos = llPos + nReadSize;
	if ((nFlag & QCIO_READ_AUDIO) != 0)
	{
		m_llPosAudio = m_llReadPos;
		m_llBufAudio += nReadSize;
	}
	else if ((nFlag & QCIO_READ_VIDEO) != 0)
	{
		m_llPosVideo = m_llReadPos;
		m_llBufVideo += nReadSize;
	}

	// For keep more buff in full list.
	//CheckFreeItem ();

	return nReadSize;
}

int	CMemFile::FillBuff(long long llPos, char * pBuff, int nSize)
{
	CAutoLock lock(&m_mtLock);
	int			nCopySize = nSize;
	int			nRestSize = nSize;
	CMemItem *	pItem = NULL;
	CMemItem *	pFind = NULL;
	NODEPOS		pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (llPos >= pItem->m_llPos && llPos < pItem->m_llPos + pItem->m_nBuffSize)
		{
			pFind = pItem;
			break;
		}
	}
	if (pFind == NULL)
	{
		pItem = GetItem(MEM_BUFF_SIZE);
		m_lstFull.AddTail(pItem);
		pItem->m_llPos = llPos;
		SortFullList();
	}

	while (nRestSize > 0)
	{
		nCopySize = nRestSize;
		if (pItem->m_nDataSize >= pItem->m_nBuffSize)
		{
			pItem = GetItem(MEM_BUFF_SIZE);
			m_lstFull.AddTail(pItem);
			pItem->m_llPos = llPos + (nSize - nRestSize);
			SortFullList();
		}
		if (nCopySize > pItem->m_nBuffSize - pItem->m_nDataSize)
			nCopySize = pItem->m_nBuffSize - pItem->m_nDataSize;
		memcpy(pItem->m_pBuff + pItem->m_nDataSize, pBuff + (nSize - nRestSize), nCopySize);
		pItem->m_nDataSize += nCopySize;
		nRestSize -= nCopySize;
	}
	m_llFillPos = llPos + nSize;
	return QC_ERR_NONE;
}

long long CMemFile::SetPos (long long llPos)
{
	if (m_bOpenCache)
		return 0;

	CAutoLock lock (&m_mtLock);
	NODEPOS		pPos = NULL;
	CMemItem *	pItem = NULL;
	CMemItem *	pFind = NULL;
	long long	llEndPos = 0;

	// Check pos is in full or not
	pPos = m_lstFull.GetHeadPosition ();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext (pPos);
		if (llPos >= pItem->m_llPos && llPos < pItem->m_llPos + pItem->m_nDataSize)
		{
			pFind = pItem;
			break;
		}
	}
	if (pFind == NULL)
	{
		Reset();
		m_llFillPos = llPos;
	}
	else
	{
		CMemItem *	pPrev = pFind;
		if (pFind != m_lstFull.GetHead())
		{
			long long llStartPos = pFind->m_llPos;

			pItem = NULL;
			pPos = m_lstFull.GetTailPosition();
			while (pPos != NULL && pItem != pFind)
				pItem = m_lstFull.GetPrev(pPos);

			while (pPos != NULL)
			{
				pItem = m_lstFull.GetPrev(pPos);
				if (pItem->m_llPos + pItem->m_nDataSize != llStartPos)
					break;
				llStartPos = pItem->m_llPos;
				pPrev = pItem;
			}
			if (pPos == NULL)
				pPrev = m_lstFull.GetHead();
		}
		// Release previous items
		pItem = m_lstFull.GetHead();
		while (pItem != pPrev)
		{
			pItem = m_lstFull.RemoveHead();
			m_lstFree.AddTail(pItem);
			pItem = m_lstFull.GetHead();
		}

		// Find the last continue item
		pFind = NULL;
		llEndPos = llPos;
		pPos = m_lstFull.GetHeadPosition();
		while (pPos != NULL)
		{
			pItem = m_lstFull.GetNext(pPos);
			if (llEndPos == llPos)
			{
				llEndPos = pItem->m_llPos + pItem->m_nDataSize;
				continue;
			}
			if (llEndPos != pItem->m_llPos)
			{
				pFind = pItem;
				break;
			}
			llEndPos = pItem->m_llPos + pItem->m_nDataSize;
		}

		// Release last items.
		if (pFind != NULL)
		{
			pItem = m_lstFull.GetTail();
			while (pItem != pFind)
			{
				m_lstFull.RemoveTail();
				m_lstFree.AddTail(pItem);
				pItem = m_lstFull.GetTail();
			}
		}
		m_llFillPos = llEndPos;
	}

	m_llPosAudio = llPos;
	m_llPosVideo = llPos;
	m_llBufAudio = 0;
	m_llBufVideo = 0;

	return QC_ERR_NONE;
}

CMemItem * CMemFile::GetItem (int nSize)
{
	CAutoLock lock (&m_mtLock);
	CMemItem * pItem = m_lstFree.RemoveHead();
	if (pItem == NULL)
	{
		if (CheckFullList() == QC_ERR_NONE)
			pItem = m_lstFull.RemoveHead();
	}
	if (pItem == NULL)
	{
		pItem = new CMemItem();
		pItem->m_nBuffSize = nSize;
	}
	if (pItem->m_nBuffSize < nSize)
	{
		pItem->m_nBuffSize = nSize;
		QC_DEL_P (pItem->m_pBuff);
	}
	if (pItem->m_pBuff == NULL)
		pItem->m_pBuff = new char[pItem->m_nBuffSize];
	pItem->m_llPos = -1;
	pItem->m_nDataSize = 0;

	return pItem;
}

int CMemFile::Reset (void)
{
	CAutoLock lock (&m_mtLock);
	CMemItem * pItem = m_lstFull.RemoveHead ();
	while (pItem != NULL)
	{
		m_lstFree.AddTail (pItem);
		pItem = m_lstFull.RemoveHead ();
	}
	m_llReadPos = 0;
	m_llFillPos = 0;

	m_llPosAudio = 0;
	m_llPosVideo = 0;
	m_llBufAudio = 0;
	m_llBufVideo = 0;

	return QC_ERR_NONE;
}

int CMemFile::GetBuffSize (long long llPos)
{
	if (m_llFillPos == llPos)
		return 0;

	CAutoLock lock(&m_mtLock);
	int			nBuffSize = 0;
	long long	llPrevPos = 0;
	CMemItem *	pItem = NULL;
	NODEPOS		pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (llPrevPos > 0)
		{
			if (pItem->m_llPos == llPrevPos)
			{
				llPrevPos = pItem->m_llPos + pItem->m_nDataSize;
				nBuffSize = (int)(llPrevPos - llPos);
			}
		}
		else if (llPos >= pItem->m_llPos && llPos < pItem->m_llPos + pItem->m_nDataSize)
		{
			llPrevPos = pItem->m_llPos + pItem->m_nDataSize;
			nBuffSize = (int)(llPrevPos - llPos);
		}
	}

	return nBuffSize;
}

long long CMemFile::GetStartPos (void)
{
	CAutoLock lock (&m_mtLock);
	CMemItem *	pItem = m_lstFull.GetHead ();
	if (pItem != NULL)
		return pItem->m_llPos;
	else
		return 0;
}

long long CMemFile::GetDownPos(void)
{
	CAutoLock lock(&m_mtLock);
	CMemItem *	pItem = m_lstFull.GetHead();
	if (pItem == NULL)
		return 0;

	long long	llPos = 0;
	NODEPOS		pPos = NULL;
	pPos = m_lstFull.GetHeadPosition();
	pItem = m_lstFull.GetNext(pPos);
	llPos = pItem->m_llPos + pItem->m_nDataSize;
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (pItem->m_llPos != llPos)
			break;
		llPos = pItem->m_llPos + pItem->m_nDataSize;
	}
	return llPos;
}

CMemItem * CMemFile::FindItem (long long llPos)
{
	CAutoLock lock (&m_mtLock);
	CMemItem *	pItem = NULL;
	NODEPOS		pPos = m_lstFull.GetHeadPosition ();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext (pPos);
		if (llPos >= pItem->m_llPos && llPos < pItem->m_llPos + pItem->m_nDataSize)
			return pItem;
	}
	return NULL;
}

int CMemFile::CheckFreeItem (void)
{
	CAutoLock lock (&m_mtLock);
	if (m_lstFull.GetCount() <= 2)
		return QC_ERR_NEEDMORE;
	CMemItem * pItemFirst = m_lstFull.GetHead();
	CMemItem * pItemLast = m_lstFull.GetTail();
	if (pItemFirst != NULL && pItemLast != NULL)
	{
		long long llDataSize = pItemLast->m_llPos + pItemLast->m_nDataSize - pItemFirst->m_llPos;
		if (llDataSize < 1024 * 1024 * 8)
			return QC_ERR_NEEDMORE;
	}

	long long llPosLast = QC_MIN (m_llPosAudio, m_llPosVideo);
	if (m_llPosAudio == 0 || m_llPosVideo == 0)
	{
		if (m_llPosAudio == 0 && m_llBufVideo > 1024 * 1024 * 4)
			llPosLast = m_llPosVideo;
		if (m_llPosVideo == 0 && m_llBufAudio > 1024 * 512)
			llPosLast = m_llPosAudio;
	}
	llPosLast = llPosLast - 1024 * 1024 * 4;
	if (llPosLast <= 0)
		return QC_ERR_NEEDMORE;
	CMemItem * pItem = m_lstFull.GetHead ();
	while (pItem != NULL)
	{
		if (pItem->m_llPos + pItem->m_nDataSize  < llPosLast)
		{
			pItem = m_lstFull.RemoveHead ();
			m_lstFree.AddTail (pItem);
		}
		else
		{
			break;
		}
		pItem = m_lstFull.GetHead ();
	}

	return QC_ERR_NONE;
}

int CMemFile::CheckFullList(void)
{
	CAutoLock lock(&m_mtLock);
	if (m_lstFull.GetCount() <= 2)
		return QC_ERR_NEEDMORE;
	CMemItem * pItemFirst = m_lstFull.GetHead();
	CMemItem * pItemLast = m_lstFull.GetTail();
	long long llDataSize = pItemLast->m_llPos + pItemLast->m_nDataSize - pItemFirst->m_llPos;
    //QCLOGI("Full size is %f, A %lld, V %lld, First %lld, Offset %lld", (double)llDataSize / 1024.0 / 1024.0, m_llPosAudio, m_llPosVideo, pItemFirst->m_llPos, (m_llPosVideo-pItemFirst->m_llPos) - 1024 * 1024 * 4);
	if (llDataSize < 1024 * 1024 * 8)
		return QC_ERR_NEEDMORE;

	long long llPosLast = QC_MIN(m_llPosAudio, m_llPosVideo);
	llPosLast = (llPosLast - pItemFirst->m_llPos) - 1024 * 1024 * 4;
	if (llPosLast <= 0)
		return QC_ERR_NEEDMORE;

	return QC_ERR_NONE;
}

int CMemFile::CheckBuffSize(void)
{
	int nSleepTime = 10000;
	if (m_llFillPos - m_llReadPos > m_llKeepSize)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 2)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 3)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 4)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 5)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 6)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 7)
		qcSleep(nSleepTime);
	if (m_llFillPos - m_llReadPos > m_llKeepSize * 8)
		qcSleep(nSleepTime);
	return QC_ERR_NONE;
}

int	CMemFile::SetMoovPos(long long llMoovPos)
{
	m_llMoovPos = llMoovPos;
	return QC_ERR_NONE;
}

int	CMemFile::SetDataPos(long long llDataPos)
{
	CAutoLock lock(&m_mtLock);
	m_llDataPos = llDataPos;

	NODEPOS		pPos = NULL;
	CMemItem *	pItem = NULL;

	pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (pItem->m_llPos < llDataPos)
			pItem->m_nFlag = QCIO_READ_HEAD;
		if (m_llMoovPos > llDataPos)
		{
			if (pItem->m_llPos + pItem->m_nDataSize > m_llMoovPos)
				pItem->m_nFlag = QCIO_READ_HEAD;
		}
	}
	return QC_ERR_NONE;
}

void CMemFile::SortFullList(void)
{
	CAutoLock lock(&m_mtLock);
	int nCount = m_lstFull.GetCount();
	if (nCount <= 1)
		return;

	CMemItem ** ppItems = new CMemItem*[nCount];
	int			nIndex = 0;
	NODEPOS		pPos = NULL;
	pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
		ppItems[nIndex++] = m_lstFull.GetNext(pPos);

	qsort(ppItems, nCount, sizeof(CMemItem *), compareMemPos);

	m_lstFull.RemoveAll();
	for (int i = 0; i < nCount; i++)
		m_lstFull.AddTail(ppItems[i]);

	delete[]ppItems;
}

void CMemFile::ShowStatus(void)
{
	CAutoLock lock(&m_mtLock);
	QCLOGI("The Pos MOOV: % 8lld, Data: % 8lld", m_llMoovPos, m_llDataPos);

	long long	llStart = -1;
	long long	llEnd = -1;
	NODEPOS		pPos = NULL;
	CMemItem *	pItem = NULL;
	pPos = m_lstFull.GetHeadPosition();
	while (pPos != NULL)
	{
		pItem = m_lstFull.GetNext(pPos);
		if (llStart < 0)
			llStart = pItem->m_llPos;
		if (llEnd < 0)
		{
			llEnd = pItem->m_llPos + pItem->m_nDataSize;
			continue;
		}
		if (pItem->m_llPos != llEnd)
		{
			QCLOGI("The Pos  Start: % 8lld, End: % 8lld", llStart, llEnd);
			llStart = -1;
			llEnd = -1;
		}
		llEnd = pItem->m_llPos + pItem->m_nDataSize;
	}
	QCLOGI("The Pos  Start: % 8lld, End: % 8lld", llStart, llEnd);
}

int	CMemFile::CopyOtherMem(void * pMemFile, unsigned char ** ppBuff, int * pSize)
{
	CMemFile *				pSrcMem = (CMemFile *)pMemFile;
	CObjectList<CMemItem> * pSrcFull = &pSrcMem->m_lstFull;
	CMemItem *				pSrcItem = NULL;
	CMemItem *				pDstItem = NULL;
	NODEPOS 				pPos = NULL;
	int						nPos = 0;

	if (ppBuff != NULL)
	{
		*pSize = pSrcMem->GetBuffSize(0);
		if (*pSize > 0)
			*ppBuff = new unsigned char[*pSize];
	}
	pPos = pSrcFull->GetHeadPosition();
	while (pPos != NULL)
	{
		pSrcItem = pSrcFull->GetNext(pPos);
		pDstItem = GetItem();
		m_lstFull.AddTail(pDstItem);
		pDstItem->m_nFlag = pSrcItem->m_nFlag;
		pDstItem->m_llPos = pSrcItem->m_llPos;
		pDstItem->m_nDataSize = pSrcItem->m_nDataSize;
		memcpy(pDstItem->m_pBuff, pSrcItem->m_pBuff, pDstItem->m_nDataSize);
		if (ppBuff != NULL)
		{
			memcpy(*ppBuff + nPos, pSrcItem->m_pBuff, pSrcItem->m_nDataSize);
			nPos += pSrcItem->m_nDataSize;
		}
	}
	if (pDstItem != NULL)
		m_llFillPos = pDstItem->m_llPos + pDstItem->m_nDataSize;
	return QC_ERR_NONE;
}

//	qsort(ppPriceItems, nItemNum, sizeof(QCPD_POS_INFO *), compareFilePos);
int CMemFile::compareMemPos(const void *arg1, const void *arg2)
{
	CMemItem * pItem1 = *(CMemItem **)arg1;
	CMemItem * pItem2 = *(CMemItem **)arg2;
	return (int)(pItem1->m_llPos - pItem2->m_llPos);
}
