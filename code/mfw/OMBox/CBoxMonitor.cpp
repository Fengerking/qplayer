/*******************************************************************************
	File:		CBoxMonitor.cpp

	Contains:	base box implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "CBoxMonitor.h"
#include "CBoxSource.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

static CBoxMonitor * g_pBoxMonitor = NULL;

CBoxRecOne::CBoxRecOne(CBoxBase * pBox, QC_DATA_BUFF * pBuffer, int * pRC)
	: m_pBox (pBox)
	, m_pBuffer (pBuffer)
	, m_pRC (pRC)
{
	m_nThdUsed = qcGetThreadTime (NULL);
	m_nSysUsed = qcGetSysTime ();

	g_pBoxMonitor->StartRead (this);
}

CBoxRecOne::~CBoxRecOne(void)
{
	m_nThdUsed = qcGetThreadTime (NULL) - m_nThdUsed;
	m_nSysUsed = qcGetSysTime() - m_nSysUsed;

	g_pBoxMonitor->EndRead (this);
}

CBoxMonitor::CBoxMonitor(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_pClock (NULL)
	, m_hFile (0)
{
	SetObjectName ("CBoxMonitor");
	g_pBoxMonitor = this;
	char szFile[1024];
#ifdef _OS_WIN32
	qcGetAppPath (NULL, szFile, sizeof (szFile));
#else
	strcpy (szFile, "/sdcard/");
#endif // _OS_win32
	strcat (szFile, ("qcLog.txt"));
//	m_hFile = qcFileOpen (szFile, QCFILE_WRITE);
}

CBoxMonitor::~CBoxMonitor(void)
{
	ReleaseItems ();
	if (m_hFile != 0)
		qcFileClose (m_hFile);
	g_pBoxMonitor = NULL;
}

int CBoxMonitor::ReleaseItems (void)
{
	SBoxReadItem * pRead = m_lstRead.RemoveHead ();
	while (pRead != NULL)
	{
		delete pRead;
		pRead = m_lstRead.RemoveHead ();
	}

	return 0;
}

int CBoxMonitor::StartRead (CBoxRecOne * pOne)
{
	return 0;
}

int CBoxMonitor::EndRead (CBoxRecOne * pOne)
{
	CAutoLock lock (&m_mtRead);

	SBoxReadItem * pRead = new SBoxReadItem ();
	pRead->m_pBox = pOne->m_pBox;
	pRead->m_nSysUsed = pOne->m_nSysUsed;
	pRead->m_nThdUsed = pOne->m_nThdUsed;
	pRead->m_tmSys = qcGetSysTime ();
	pRead->m_nType = pOne->m_pBuffer->nMediaType;
	pRead->m_tmBuf = (int)pOne->m_pBuffer->llTime;
	pRead->m_tmClock = (int)m_pClock->GetTime ();
	pRead->m_nRC = *pOne->m_pRC;

	m_lstRead.AddTail (pRead);

	return 0;
}

SBoxReadItem * CBoxMonitor::GetLastItem (CBoxBase * pBox, QCMediaType nType, int nRC)
{
	CAutoLock lock (&m_mtRead);

	SBoxReadItem * pItem = NULL;
	SBoxReadItem * pLast = NULL;
	NODEPOS pos = m_lstRead.GetTailPositionI ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetPrev (pos);
		if (pItem->m_nRC != nRC)
			continue;

		if (pItem->m_pBox == pBox && pItem->m_nType == nType)
		{
			pLast = pItem;
			break;
		}
	}

	return pLast;
}

int CBoxMonitor::ShowResult (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_SOURCE, QC_MEDIA_Audio);
	if (pBox == NULL)
		pBox = GetBox (OMB_TYPE_SOURCE, QC_MEDIA_Video);
	if (pBox != NULL)
	{
		if (((CBoxSource *)pBox)->GetSourceName () != NULL)
		{
			QCLOGI ("Source Name: %s", ((CBoxSource *)pBox)->GetSourceName ());
		}
	}
#ifndef BOX_MONITOR_DISABLE_ALL
//	ShowAudioSrc ();
//	ShowVideoSrc ();
//	ShowAudioDec ();
//	ShowVideoDec ();
//	ShowAudioRnd ();
//	ShowVideoRnd ();

//	ShowPerformInfo ();

#endif // BOX_MONITOR_DISABLE_ALL
	if (m_hFile != 0)
		qcFileClose (m_hFile);
	m_hFile = 0;
	return 0;
}

void CBoxMonitor::ShowAudioSrc (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_SOURCE, QC_MEDIA_Audio);
	if (pBox != NULL)
	{
		QCLOGI ("Box Source read audio info:");
		ShowBoxInfo (pBox, QC_MEDIA_Audio, true);
	}
}

void CBoxMonitor::ShowVideoSrc (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_SOURCE, QC_MEDIA_Video);
	if (pBox != NULL)
	{
		QCLOGI ("Box Source read video info:");
		ShowBoxInfo (pBox, QC_MEDIA_Video, true);
	}
}

void CBoxMonitor::ShowAudioDec (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_FILTER, QC_MEDIA_Audio);
	if (pBox != NULL)
	{
		QCLOGI ("Box Audio Dec info:");
		ShowBoxInfo (pBox, QC_MEDIA_Audio, true);
	}
}

void CBoxMonitor::ShowVideoDec (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_FILTER, QC_MEDIA_Video);
	if (pBox != NULL)
	{
		QCLOGI ("Box Video Dec info:");
		ShowBoxInfo (pBox, QC_MEDIA_Video, true);
	}
}

void CBoxMonitor::ShowAudioRnd (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_RENDER, QC_MEDIA_Audio);
	if (pBox != NULL)
	{
		QCLOGI ("Box Audio Rnd info:");
		ShowBoxInfo (pBox, QC_MEDIA_Audio, true);
	}
}

void CBoxMonitor::ShowVideoRnd (void)
{
	CBoxBase * pBox = GetBox (OMB_TYPE_RENDER, QC_MEDIA_Video);
	if (pBox != NULL)
	{
		QCLOGI ("Box Video Rnd info:");
		ShowBoxInfo (pBox, QC_MEDIA_Video, true);
	}
}

void CBoxMonitor::ShowBoxInfo (CBoxBase * pBox, QCMediaType nType, bool bSuccess)
{
	int				nIndex = 0;
	int				nStartTime = 0;
	SBoxReadItem *	pItem = NULL;
	SBoxReadItem *	pPrev = NULL;
	NODEPOS		pos = m_lstRead.GetHeadPosition ();

	QCLOGI ("Index  UseSys  UseThd    Buffer    Step   Buf-Clk   SysTime   Sys-Clk  Sys-Step");
	if (m_hFile != 0)
	{
		qcFileWrite (m_hFile, (unsigned char *)pBox->GetName (), strlen (pBox->GetName ()));
		qcFileWrite (m_hFile, (unsigned char *)"\r\n\r\n", 4);
	}
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox != pBox || pItem->m_nType != nType)
			continue;
		if (bSuccess && pItem->m_nRC != QC_ERR_NONE)
			continue;

		if (nIndex == 0)
		{
			nStartTime = pItem->m_tmSys;// - pItem->m_tmClock;
			QCLOGI ("% 5d  % 6d  % 6d  % 8d  % 6d  % 8d  % 8d  % 8d    % 6d",
					nIndex, pItem->m_nSysUsed, pItem->m_nThdUsed,
					pItem->m_tmBuf, 0, pItem->m_tmBuf - pItem->m_tmClock,
					0, pItem->m_tmSys - nStartTime - pItem->m_tmClock, 0);
			if (m_hFile != 0)
			{
				char szLogText[1024];
				sprintf (szLogText, "% 5d  % 6d  % 6d  % 8d  % 6d  % 8d  % 8d  % 8d    % 6d\r\n",
						nIndex, pItem->m_nSysUsed, pItem->m_nThdUsed,
						pItem->m_tmBuf, 0, pItem->m_tmBuf - pItem->m_tmClock,
						0, pItem->m_tmSys - nStartTime - pItem->m_tmClock, 0);
				qcFileWrite (m_hFile, (unsigned char *)szLogText, strlen (szLogText));
			}
		}
		else
		{
//			if (pItem->m_tmBuf - pPrev->m_tmBuf != 40)
			{
			QCLOGI ("% 5d  % 6d  % 6d  % 8d  % 6d  % 8d  % 8d  % 8d    % 6d",
					nIndex, pItem->m_nSysUsed, pItem->m_nThdUsed,
					pItem->m_tmBuf, pItem->m_tmBuf - pPrev->m_tmBuf, pItem->m_tmBuf - pItem->m_tmClock,
					pItem->m_tmSys - nStartTime, pItem->m_tmSys - nStartTime - pItem->m_tmClock, pItem->m_tmSys - pPrev->m_tmSys);
			}
			if (m_hFile != 0)
			{
				char szLogText[1024];
				sprintf (szLogText, "% 5d  % 6d  % 6d  % 8d  % 6d  % 8d  % 8d  % 8d    % 6d\r\n",
						nIndex, pItem->m_nSysUsed, pItem->m_nThdUsed,
						pItem->m_tmBuf, pItem->m_tmBuf - pPrev->m_tmBuf, pItem->m_tmBuf - pItem->m_tmClock,
						pItem->m_tmSys - nStartTime, pItem->m_tmSys - nStartTime - pItem->m_tmClock, pItem->m_tmSys - pPrev->m_tmSys);
				qcFileWrite (m_hFile, (unsigned char *)szLogText, strlen (szLogText));
			}
		}

		pPrev = pItem;
		nIndex++;
	}
	if (m_hFile != 0)
		qcFileWrite (m_hFile, (unsigned char *)"\r\n\r\n\r\n\r\n", 8);
}

CBoxBase * CBoxMonitor::GetBox (OMBOX_TYPE nBoxType, QCMediaType nMediaType)
{
	CBoxBase *		pBox = NULL;
	SBoxReadItem *	pItem = NULL;
	NODEPOS		pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == nBoxType && pItem->m_nType == nMediaType)
		{
			pBox = pItem->m_pBox;
			break;
		}
	}

	return pBox;
}

void CBoxMonitor::ShowPerformInfo (void)
{
	int				nCount = 0;
	int				nTotal = 0;
	int				nThdTime = 0;
	int				nSttTime = 0;
	int				nEndTime = 0;

	QCLOGI ("Show performance info:");

	SBoxReadItem *	pItem = NULL;
	NODEPOS		pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_SOURCE && pItem->m_nType == QC_MEDIA_Audio)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Audio Read: Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%", nCount, nTotal, nThdTime, (nThdTime * 100.0) / (nEndTime - nSttTime));

	nCount = 0;
	nTotal = 0;
	nThdTime = 0;
	pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_FILTER && pItem->m_nType == QC_MEDIA_Audio)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Audio Dec:  Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%", nCount, nTotal, nThdTime, (nThdTime * 100.0) / (nEndTime - nSttTime));

	nCount = 0;
	nTotal = 0;
	nThdTime = 0;
	pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_RENDER && pItem->m_nType == QC_MEDIA_Audio)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Audio Rnd:  Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%", nCount, nTotal, nThdTime, (nThdTime * 100.0) / (nEndTime - nSttTime));


	// Start to analyse video info.
	nCount = 0;
	nTotal = 0;
	nThdTime = 0;
	pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_SOURCE && pItem->m_nType == QC_MEDIA_Video)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Video Read: Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%   Speed  % 4.2f F/S", nCount, nTotal, nThdTime, 
			(nThdTime * 100.0) / (nEndTime - nSttTime), (nCount * 1000.0) / (nEndTime - nSttTime));

	nCount = 0;
	nTotal = 0;
	nThdTime = 0;
	pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_FILTER && pItem->m_nType == QC_MEDIA_Video)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Video Dec:  Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%   Speed  % 4.2f F/S", nCount, nTotal, nThdTime, 
			(nThdTime * 100.0) / (nEndTime - nSttTime), (nCount * 1000.0) / (nEndTime - nSttTime));

	nCount = 0;
	nTotal = 0;
	nThdTime = 0;
	pos = m_lstRead.GetHeadPosition ();
	while (pos != NULL)
	{
		pItem = m_lstRead.GetNext (pos);
		if (pItem->m_pBox->GetType () == OMB_TYPE_RENDER && pItem->m_nType == QC_MEDIA_Video)
		{
			if (pItem->m_nRC != QC_ERR_NONE)
			{
				nTotal++;
				continue;
			}

			if (nCount == 0)
				nSttTime = pItem->m_tmSys;
			nCount++;
			nTotal++;
			nThdTime += pItem->m_nThdUsed;
			nEndTime = pItem->m_tmSys;
		}
	}
	QCLOGI ("Video Rnd:  Num % 6d / % 6d,    Thd % 8d,   % 4.2f%%   Speed  % 4.2f F/S", nCount, nTotal, nThdTime, 
			(nThdTime * 100.0) / (nEndTime - nSttTime), (nCount * 1000.0) / (nEndTime - nSttTime));
}