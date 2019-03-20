/*******************************************************************************
	File:		CBuffMng.cpp

	Contains:	message manager class implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-08		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"
#include "CBuffMng.h"
#include "CMsgMng.h"
#include "CQCMuxer.h"

#include "UAVFormatFunc.h"
#include "ULogFunc.h"
#include "UAVParser.h"

#define	QCBA_KEEP_BUTTIIME	3000

CBuffMng::CBuffMng(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_bSourceLive(false)
	, m_llLastTime(0)
	, m_nSysTime(0)
	, m_llVideoFrameSize(0)
	, m_llAudioFrameSize(0)
	, m_pFmtVideo(NULL)
	, m_pFmtAudio(NULL)
{
	SetObjectName ("CBuffMng");
	ResetParam();

	m_llStartTime = -1;
	m_llGopTime = -1;
	m_nNewEmptyNum = 0;

	if (m_pBaseInst != NULL)
		m_pBaseInst->m_pBuffMng = this;
}

CBuffMng::~CBuffMng(void)
{
	ReleaseBuff (true);
	if (m_pBaseInst != NULL)
		m_pBaseInst->m_pBuffMng = NULL;
}

int CBuffMng::Read (QCMediaType nType, long long llPos, QC_DATA_BUFF ** ppBuff)
{
	if (ppBuff == NULL)
		return QC_ERR_ARG;
	*ppBuff = NULL;

	CAutoLock lock (&m_lckList);
	NotifyBuffTime();

	QC_DATA_BUFF * pBuff = NULL;
	if (nType == QC_MEDIA_Video && m_lstNewVideo.GetCount() > 0)
	{
		pBuff = m_lstVideo.GetHead ();
		SwitchNewStream (pBuff);	
	}	

	// remove the first audio buffs if early than video at begin
	if (m_nNumRead == 0 && !m_bEOS && m_llSeekPos != 0 && (m_lstVideo.GetCount () < 100 && m_lstAudio.GetCount () < 100))
	{
		if ((m_lstVideo.GetCount () < 2 && !m_bEOV) || (m_lstAudio.GetCount () < 2 && !m_bEOA))
			return QC_ERR_RETRY;
		pBuff = m_lstVideo.GetHead ();
		QC_DATA_BUFF * pBuffAudio = m_lstAudio.GetHead ();
		while (pBuffAudio != NULL && pBuff)
		{
			if (pBuffAudio->llTime >= pBuff->llTime)
				break;
			pBuffAudio = m_lstAudio.RemoveHead ();
			m_lstEmpty.AddHead (pBuffAudio);
			pBuffAudio = m_lstAudio.GetHead ();
		}
		if (pBuffAudio == NULL)
			return QC_ERR_RETRY;
	}
	if (nType == QC_MEDIA_Video)
	{
		pBuff = ReadVideo (llPos);
		if (pBuff != NULL)
			m_llLastVideoTime = pBuff->llTime;
	}
	else if (nType == QC_MEDIA_Audio)
		pBuff = m_lstAudio.RemoveHead();
	else if (nType == QC_MEDIA_Subtt)
		pBuff = m_lstSubtt.RemoveHead();
	else
		return QC_ERR_FAILED;

	if (pBuff == NULL)
	{	
		if (m_bEOS)
		{
			if (nType == QC_MEDIA_Video && m_lstVideo.GetCount () == 0)
				return QC_ERR_FINISH;
			else if (nType == QC_MEDIA_Audio && m_lstAudio.GetCount () == 0)
				return QC_ERR_FINISH;
			else if (nType == QC_MEDIA_Subtt && m_lstSubtt.GetCount () == 0)
				return QC_ERR_FINISH;
			else
				return QC_ERR_RETRY;
		}
		else
		{
			return QC_ERR_RETRY;
		}
	}
	pBuff->fReturn = &CBuffMng::Return;
	pBuff->pBuffMng = this;
	pBuff->nUsed++;
	*ppBuff = pBuff;

	if (pBuff->nMediaType == QC_MEDIA_Video && pBuff->uFlag != 0)
	{
//		QCLOGI ("VideoTime: % 8lld Size % 8d, Flag % 8d  Step: % 8lld", pBuff->llTime, pBuff->uSize, pBuff->uFlag, pBuff->llTime - m_llLastTime);
//		m_llLastTime = pBuff->llTime;
	}
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
        //QCLOGI ("###Video Time: % 8lld,  Size: % 8d, Flag: % 8d     Pos = % 8lld, count %d", pBuff->llTime, pBuff->uSize, pBuff->uFlag, llPos, GetBuffCount(QC_MEDIA_Video));
	}
	else
	{
        //QCLOGI ("***Audio Time: % 8lld,  Size: % 8d, Flag: % 8d, count %d", pBuff->llTime, pBuff->uSize, pBuff->uFlag, GetBuffCount(QC_MEDIA_Audio));
	}

	m_nNumRead++;
	return QC_ERR_NONE;
}

QC_DATA_BUFF * CBuffMng::ReadVideo (long long llPos)
{
	QC_DATA_BUFF *	pBuff = m_lstVideo.GetHead ();
	if (pBuff == NULL)
		return NULL;

	if (pBuff->llTime >= llPos || llPos < m_llNextKeyTime || ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA))
	{
		pBuff = m_lstVideo.RemoveHead ();
		return pBuff;
	}
	if (m_llNextKeyTime == 0)
	{
		NODEPOS pPos = m_lstVideo.GetHeadPosition ();
		while (pPos != NULL)
		{
			pBuff = m_lstVideo.GetNext (pPos);
			if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
			{
				m_llNextKeyTime = pBuff->llTime;
				if (pBuff->llTime >= llPos)
					break;
			}
		}
	}
	if (m_llNextKeyTime > 0 && m_llNextKeyTime > llPos)
	{
		pBuff = m_lstVideo.RemoveHead ();
		return pBuff;
	}
	else
	{
		int				nDropFrames = 0;
		pBuff = m_lstVideo.RemoveHead ();
		while (pBuff != NULL)
		{
			if (pBuff->llTime < m_llNextKeyTime)
			{
				m_lstEmpty.AddHead(pBuff);
				nDropFrames++;
			}
			else
			{
				QC_DATA_BUFF *	pTmpBuff = NULL;
				if (nDropFrames > 0)
				{
					pTmpBuff = m_lstVideo.GetHead();
					while (pTmpBuff->llTime < pBuff->llTime)
					{
						pTmpBuff = m_lstVideo.RemoveHead();
						m_lstEmpty.AddTail(pTmpBuff);
						pTmpBuff = m_lstVideo.GetHead();
					}
				}
				break;
			}
			pBuff = m_lstVideo.RemoveHead ();
		}
	}
	m_llNextKeyTime = 0;

	return pBuff;
}

bool CBuffMng::SwitchNewStream (QC_DATA_BUFF * pPlayBuff)
{
	QC_DATA_BUFF *	pKeyBuff = NULL;
	QC_DATA_BUFF *	pTmpBuff = NULL;
	QC_DATA_BUFF *	pKeyAudio = NULL;
	long long		llNewBuff = 0;
	long long		llOldBuff = 0;
	bool			bSwitchStream = false;
	NODEPOS			pos = NULL;

	if (pPlayBuff == NULL)
	{
		pTmpBuff = m_lstNewVideo.GetTail ();
		if (pTmpBuff != NULL && pTmpBuff->llTime < m_llLastVideoTime)
				return false;

		pos = m_lstNewVideo.GetHeadPosition ();
		while (pos != NULL)
		{
			pTmpBuff = m_lstNewVideo.GetNext (pos);
			if ((pTmpBuff->uFlag & QCBUFF_KEY_FRAME) != 0 && pTmpBuff->llTime >= m_llLastVideoTime)
			{
				pKeyBuff = pTmpBuff;
				break;
			}
		}
		if (pKeyBuff == NULL)
			return false;

		pKeyAudio = pKeyBuff;
		if (pKeyBuff != NULL)
		{
			pTmpBuff = m_lstAudio.GetTail ();
			if (pTmpBuff != NULL && pTmpBuff->llTime < pKeyAudio->llTime)
				pKeyAudio = pTmpBuff;
		}
		SwitchBuffList (pKeyBuff, &m_lstNewVideo, &m_lstVideo);
		SwitchBuffList (pKeyAudio, &m_lstNewAudio, &m_lstAudio);
		m_pLstSendVideo = &m_lstVideo;
		m_pLstSendAudio = &m_lstAudio;
		return true;
	}

	if (pPlayBuff == NULL || m_lstNewVideo.GetCount () < 2)
		return false;

	if (m_bNewBetter)
	{
		pos = m_lstNewVideo.GetHeadPosition ();
		while (pos != NULL)
		{
			pTmpBuff = m_lstNewVideo.GetNext (pos);
			if ((pTmpBuff->uFlag & QCBUFF_KEY_FRAME) != 0)
			{
				if (pTmpBuff->llTime < pPlayBuff->llTime)
					continue;
				pKeyBuff = pTmpBuff;
				break;
			}
		}
		if (pKeyBuff == NULL)
			return false;
		if (abs ((int)(pKeyBuff->llTime - pPlayBuff->llTime)) > 20)
			return false;

		llNewBuff = m_lstNewVideo.GetTail ()->llTime - pKeyBuff->llTime;
		llOldBuff = m_lstVideo.GetTail ()->llTime - pKeyBuff->llTime;
		if (llNewBuff > 2000)
			bSwitchStream = true;
		if (!bSwitchStream && llNewBuff >= llOldBuff)
			bSwitchStream = true;
		if (!bSwitchStream)
			return false;

		pKeyAudio = pKeyBuff;
		pTmpBuff = m_lstAudio.GetHead ();
		if (pTmpBuff != NULL && pTmpBuff->llTime > pKeyAudio->llTime)
			pKeyAudio = pTmpBuff;
	}
	else
	{
		pPlayBuff = m_lstVideo.GetTail ();
		if (m_lstNewVideo.GetTail ()->llTime <= pPlayBuff->llTime)
			return false;
		pos = m_lstNewVideo.GetTailPosition ();
		while (pos != NULL)
		{
			pTmpBuff = m_lstNewVideo.GetPrev (pos);
			if ((pTmpBuff->uFlag & QCBUFF_KEY_FRAME) != 0)
			{
				if (pTmpBuff->llTime < pPlayBuff->llTime)
					break;
				pKeyBuff = pTmpBuff;
			}
		}
		if (pKeyBuff == NULL)
			return false;

		pKeyAudio = pKeyBuff;
		pTmpBuff = m_lstAudio.GetTail ();
		if (pTmpBuff != NULL && pTmpBuff->llTime < pKeyAudio->llTime)
			pKeyAudio = pTmpBuff;
	}

	QCLOGI("###Switch to new stream at % 8d, Size = %d,  Flag = %0X  LastVideoTime = % 8lld", (int)pKeyBuff->llTime, pKeyBuff->uSize, pKeyBuff->uFlag, m_llLastVideoTime);
	SwitchBuffList (pKeyBuff, &m_lstNewVideo, &m_lstVideo);
	SwitchBuffList (pKeyAudio, &m_lstNewAudio, &m_lstAudio);
	m_pLstSendVideo = &m_lstVideo;
	m_pLstSendAudio = &m_lstAudio;
	return true;
}

int CBuffMng::Send(QC_DATA_BUFF * pBuff)
{
	CAutoLock lock(&m_lckList);
	if (pBuff == NULL)
		return QC_ERR_ARG;

	if (m_pBaseInst->m_bAudioDecErr && pBuff->nMediaType == QC_MEDIA_Audio)
	{
		m_lstEmpty.AddHead(pBuff);
		return QC_ERR_NONE;
	}
	if (m_pBaseInst->m_bVideoDecErr && pBuff->nMediaType == QC_MEDIA_Video)
	{
		m_lstEmpty.AddHead(pBuff);
		return QC_ERR_NONE;
	}
	if (pBuff->nMediaType != QC_MEDIA_Video && pBuff->nMediaType != QC_MEDIA_Audio)
	{
		m_lstEmpty.AddHead(pBuff);
		return QC_ERR_NONE;
	}

	if (pBuff->nMediaType == QC_MEDIA_Video && (pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		if (m_pBaseInst != NULL)
			m_pBaseInst->SetSettingParam(QC_BASEINST_EVENT_NEWFORMAT_V, 0, pBuff->pFormat);
	}

	if (m_bFlush && pBuff->nMediaType == QC_MEDIA_Video)
	{
		if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == 0)
		{
			if ((pBuff->uFlag & QCBUFF_HEADDATA) == 0)
			{
				m_lstEmpty.AddHead(pBuff);
				return QC_ERR_NONE;
			}
		}
		else
		{
			m_bFlush = false;
		}
	}

	NotifyBuffTime();

	AnlBufferInfo(pBuff);

	if (m_llStartTime == -1 && ((pBuff->uFlag & QCBUFF_HEADDATA) == 0) && pBuff->llTime >= 0)
	{
		if (m_pBaseInst->m_llFAudioTime != m_pBaseInst->m_llFVideoTime)
		{
			m_llSeekPos = 0;
			m_llStartTime = 0;
		}
		else
		{
			if (m_llSeekPos == -1)
				m_llSeekPos = pBuff->llTime;
			m_llStartTime = pBuff->llTime - m_llSeekPos;
		}
	}
	if (pBuff->llTime >= 0)
	{
		pBuff->llTime -= m_llStartTime;
		if (pBuff->llTime < 0)
			m_llStartTime = m_llStartTime + pBuff->llTime - 33;
		m_llLastSendTime = pBuff->llTime;
	}
    
	if (pBuff->nMediaType == QC_MEDIA_Video)
	{
//                QCLOGI ("++++Time: % 8lld Size % 8d, Flag % 8d  Step: % 8lld, buff %d, count %d", pBuff->llTime, pBuff->uSize, pBuff->uFlag, pBuff->llTime - m_llLastTime, (int)GetBuffTime(QC_MEDIA_Video), GetBuffCount(QC_MEDIA_Video));
//                if (pBuff->llTime - m_llLastTime > 200)
//                    QCLOGI ("Buff Time: % 8d   Last  % 8d, Step % 8d", (int)pBuff->llTime, (int)m_llLastTime, (int)(pBuff->llTime - m_llLastTime));
//                m_llLastTime = pBuff->llTime;

        //		if (m_nSysTime == 0)
		//			m_nSysTime = qcGetSysTime();
		//		QCLOGI("++++Time: % 8lld Size % 8d, Flag % 8d  Step: % 8d", pBuff->llTime, pBuff->uSize, pBuff->uFlag, (int)(pBuff->llTime - (qcGetSysTime() - m_nSysTime)));
//        QCLOGI("[S]1 Send video %lld", pBuff->llTime);
	}
	if (pBuff->nMediaType == QC_MEDIA_Audio)
	{
//                QCLOGI ("-----Time: % 8lld Size % 8d, Flag % 8d, buff %d, count %d", pBuff->llTime, pBuff->uSize, pBuff->uFlag, (int)GetBuffTime(QC_MEDIA_Audio), GetBuffCount(QC_MEDIA_Audio));
	}

	QC_DATA_BUFF *					pTempBuff = NULL;
	CObjectList<QC_DATA_BUFF> *		pLstSend = NULL;
	if ((pBuff->uFlag & QCBUFF_NEWSTREAM) == QCBUFF_NEWSTREAM)
	{
		QCLOGI("There is new % 8d stream!", pBuff->nMediaType);
		if (pBuff->nMediaType == QC_MEDIA_Video)
		{
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify (QC_MSG_BUFF_NEWSTREAM, 0, pBuff->llTime);
			if (m_pLstSendVideo == NULL)
				m_pLstSendVideo = &m_lstVideo;
			else
				m_pLstSendVideo = &m_lstNewVideo;
			pLstSend = m_pLstSendVideo;
		}
		else if (pBuff->nMediaType == QC_MEDIA_Audio)
		{
			if (m_pLstSendAudio == NULL)
				m_pLstSendAudio = &m_lstAudio;
			else
				m_pLstSendAudio = &m_lstNewAudio;
			pLstSend = m_pLstSendAudio;
		}

		pTempBuff = pLstSend->GetTail();
		while (pTempBuff != NULL)
		{
			if (pTempBuff->llTime > pBuff->llTime || pTempBuff->llTime < 0)
			{
				pTempBuff = pLstSend->RemoveTail();
				m_lstEmpty.AddHead(pTempBuff);
				pTempBuff = pLstSend->GetTail();
			}
			else
			{
				break;
			}
		}
	}
	// Save the format data with buffer
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
		CopyNewFormat (pBuff);

	// For mux data
	WriteFrame(pBuff);

	if (pLstSend == NULL)
	{
		if (pBuff->nMediaType == QC_MEDIA_Video)
		{
			if (m_pLstSendVideo == NULL)
				m_pLstSendVideo = &m_lstVideo;
			pLstSend = m_pLstSendVideo;
		}
		else if (pBuff->nMediaType == QC_MEDIA_Audio)
		{
			if (m_pLstSendAudio == NULL)
				m_pLstSendAudio = &m_lstAudio;
			pLstSend = m_pLstSendAudio;
		}
	}

	if ((pBuff->uFlag & QCBUFF_HEADDATA) == 0)
	{
		pTempBuff = pLstSend->GetTail();
		if (pTempBuff != NULL && (pTempBuff->uFlag & QCBUFF_HEADDATA) != 0)
			pTempBuff->llTime = pBuff->llTime;
	}
	pLstSend->AddTail (pBuff);

	m_nNumSend++;
	return QC_ERR_NONE;
}

QC_DATA_BUFF * CBuffMng::GetEmpty (QCMediaType nType, int nSize)
{
	CAutoLock lock (&m_lckList);
	QC_DATA_BUFF *	pNewBuff = NULL;
	QC_DATA_BUFF *	pTmpBuff = NULL;
	int				nBufCount = 0;
	int				nIndex = 0;
	int				nKeepFrames = 2;
	NODEPOS			pPos = m_lstEmpty.GetHeadPosition ();
	while (pPos != NULL)
	{
		pTmpBuff = m_lstEmpty.GetNext (pPos);
		if (pTmpBuff->nMediaType == nType)
			nBufCount++;
	}
	if (nBufCount > nKeepFrames)
	{
		nIndex = 0;
		pPos = m_lstEmpty.GetHeadPosition ();
		while (pPos != NULL)
		{
			pTmpBuff = m_lstEmpty.GetNext (pPos);
			if (pTmpBuff->nMediaType == nType)
				nIndex++;
			if (nIndex + nKeepFrames >= nBufCount)
				break;
			if (pTmpBuff->nMediaType == nType && pTmpBuff->uBuffSize >= (unsigned int)nSize)
			{
				if (pNewBuff == NULL)
					pNewBuff = pTmpBuff;
				else if (pNewBuff->uBuffSize > pTmpBuff->uBuffSize)
					pNewBuff = pTmpBuff;
			}
		}

		if (pNewBuff != NULL)
		{
			m_lstEmpty.Remove(pNewBuff);
		}
		else
		{
			pPos = m_lstEmpty.GetHeadPosition ();
			while (pPos != NULL)
			{
				pTmpBuff = m_lstEmpty.GetNext (pPos);
				if (pTmpBuff->nMediaType == nType)
				{
					m_lstEmpty.Remove (pTmpBuff);
					pNewBuff = pTmpBuff;
					break;
				}
			}
		}
	}

	if (pNewBuff == NULL)
		pNewBuff = NewEmptyBuff ();
	else if (pNewBuff->pUserData != NULL && pNewBuff->fFreeBuff != NULL)
		pNewBuff->fFreeBuff(pNewBuff->pUserData, pNewBuff);
	if (pNewBuff != NULL)
	{
		pNewBuff->nMediaType = nType;
		pNewBuff->pFormat = NULL;
		pNewBuff->uFlag = 0;
		pNewBuff->uSize = 0;
		m_nNumGet++;
	}
	//QCLOGI("BuffMemSize = %lld", GetTotalMemSize());
	return pNewBuff;
}

int	CBuffMng::Return (void * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	CAutoLock lock (&m_lckList);
	m_lstEmpty.AddTail ((QC_DATA_BUFF*)pBuff);
	m_nNumReturn++;
	return QC_ERR_NONE;
}

int CBuffMng::SetPos (long long llPos)
{
	CAutoLock lock (&m_lckList);
	QC_DATA_BUFF *	pHeadVideo = NULL;
	QC_DATA_BUFF *	pHeadAudio = NULL;
	QC_DATA_BUFF *	pTmpBuff = NULL;
	QC_DATA_BUFF *	pKeyBuff = NULL;
	NODEPOS			pos = NULL;
	bool			bInBuff = false;

	if (m_lstNewVideo.GetCount () > 0)
	{
		pos = m_lstNewVideo.GetHeadPosition();
		while (pos != NULL)
		{
			pTmpBuff = m_lstNewVideo.GetNext(pos);
			if ((pTmpBuff->uFlag & QCBUFF_HEADDATA) != 0)
				pHeadVideo = pTmpBuff;
			if ((pTmpBuff->uFlag & QCBUFF_KEY_FRAME) != 0)
				pKeyBuff = pTmpBuff;
			if (pTmpBuff->llTime >= llPos)
			{
				if (pKeyBuff != NULL)
					bInBuff = true;
				break;
			}
		}

		if (!bInBuff)
		{
			EmptyBuffList(&m_lstNewVideo);
			EmptyBuffList(&m_lstNewAudio);
		}
		else
		{
			EmptyBuffList(&m_lstVideo);
			EmptyBuffList(&m_lstAudio);
			QC_DATA_BUFF * pBuff = m_lstNewVideo.RemoveHead();
			while (pBuff != NULL)
			{
				m_lstVideo.AddTail(pBuff);
				pBuff = m_lstNewVideo.RemoveHead();
			}
			pBuff = m_lstNewAudio.RemoveHead();
			while (pBuff != NULL)
			{
				m_lstAudio.AddTail(pBuff);
				pBuff = m_lstNewAudio.RemoveHead();
			}
		}
	}

	pHeadVideo = NULL;
	pKeyBuff = NULL;
	bInBuff = false;
	pos = m_lstVideo.GetHeadPosition();
	while (pos != NULL)
	{
		pTmpBuff = m_lstVideo.GetNext(pos);
		if ((pTmpBuff->uFlag & QCBUFF_HEADDATA) != 0)
			pHeadVideo = pTmpBuff;
		if ((pTmpBuff->uFlag & QCBUFF_KEY_FRAME) != 0)
			pKeyBuff = pTmpBuff;
		if (pTmpBuff->llTime >= llPos)
		{
			if (pKeyBuff != NULL && pKeyBuff->llTime <= llPos)
				bInBuff = true;
			break;
		}
	}

	if (bInBuff)
	{
		pHeadVideo = NULL;
		pTmpBuff = m_lstVideo.RemoveHead();
		while (pTmpBuff != NULL)
		{
			if (pTmpBuff == pKeyBuff)
				break;
			if ((pTmpBuff->uFlag & QCBUFF_HEADDATA) != 0)
				pHeadVideo = pTmpBuff;
			m_lstEmpty.AddHead(pTmpBuff);
			pTmpBuff = m_lstVideo.RemoveHead();
		}
		m_lstVideo.AddHead(pKeyBuff);
		if (pHeadVideo != NULL && pHeadVideo != pKeyBuff)
		{
			m_lstVideo.AddHead(pHeadVideo);
			m_lstEmpty.Remove(pHeadVideo);
		}

		pTmpBuff = m_lstAudio.RemoveHead();
		while (pTmpBuff != NULL)
		{
			if (pTmpBuff->llTime >= llPos)
			{
				m_lstAudio.AddHead(pTmpBuff);
				break;
			}
			if ((pTmpBuff->uFlag & QCBUFF_HEADDATA) != 0)
				pHeadAudio = pTmpBuff;
			m_lstEmpty.AddHead(pTmpBuff);
			pTmpBuff = m_lstAudio.RemoveHead();
		}
		if (pHeadAudio != NULL && pHeadAudio != pTmpBuff)
		{
			m_lstAudio.AddHead(pHeadAudio);
			m_lstEmpty.Remove(pHeadAudio);
		}
	}
	else
	{
		EmptyBuffList(&m_lstVideo);
		EmptyBuffList(&m_lstAudio);
	}

	ResetParam();

	if (m_llStartTime == -1)
		m_llSeekPos = -1;

	return bInBuff ? QC_ERR_NONE : QC_ERR_NEEDMORE;
}

void CBuffMng::SetEOS (bool bEOS)
{
	m_bEOS = bEOS;
}

void CBuffMng::SetAVEOS(bool bEOA, bool bEOV)
{
	m_bEOA = bEOA;
	m_bEOV = bEOV;
}

int	CBuffMng::SetStreamPlay (QCMediaType nType, int nStream)
{
	if (nType == QC_MEDIA_Source)
	{
		m_nSelStream = nStream;
		QCLOGI("Select new stream % 8d", nStream);
	}
	return QC_ERR_NONE;
}

int CBuffMng::SetSourceLive (bool bLive)
{
	m_bSourceLive = bLive;
	return QC_ERR_NONE;
}

long long CBuffMng::GetLastTime(QCMediaType nType)
{
	CAutoLock lock(&m_lckList);
	if (!SetCurrentList(nType, true))
		return 0;

	QC_DATA_BUFF * pBuff = m_pCurList->GetTail();
	if (pBuff == NULL)
	{
		return 0;
//		if (nType == QC_MEDIA_Video)
//			return m_llGetLastVTime;
//		else
//			return m_llGetLastATime;
	}
	if (nType == QC_MEDIA_Video)
		m_llGetLastVTime = pBuff->llTime;
	else
		m_llGetLastATime = pBuff->llTime;
	return pBuff->llTime;
}

int	CBuffMng::GetBuffCount(QCMediaType nType)
{
	CAutoLock lock(&m_lckList);
	if (!SetCurrentList(nType, true))
		return 0;
	return m_pCurList->GetCount();
}

long long CBuffMng::GetPlayTime (QCMediaType nType)
{
	CAutoLock lock (&m_lckList);
	if (!SetCurrentList (nType, true))
		return -1;

	QC_DATA_BUFF * pBuff = m_pCurList->GetHead ();
	if (pBuff == NULL)
		return -1;
	return pBuff->llTime;
}

unsigned int CBuffMng::GetBuffM3u8Pos()
{
	CAutoLock lock(&m_lckList);
	if (!SetCurrentList(QC_MEDIA_Video, true))
		return 0;
	QC_DATA_BUFF * pBuff = m_pCurList->GetHead();
	if (pBuff == NULL)
		return 0;
	return pBuff->nValue;
}

int CBuffMng::HaveNewStreamBuff (void)
{
	CAutoLock lock(&m_lckList);
	if (m_lstNewVideo.GetCount() > 0 || m_lstNewAudio.GetCount() > 0)
		return 1;
	else
		return 0;
}

int CBuffMng::InSwitching()
{
	CAutoLock lock(&m_lckList);
	return m_lstNewVideo.GetCount() > 0;
}

long long CBuffMng::GetTotalMemSize(void)
{
	CAutoLock lock(&m_lckList);
	long long llMemSize = 0;
	NODEPOS			pPos = NULL;
	QC_DATA_BUFF *	pBuff = NULL;

	CObjectList<QC_DATA_BUFF> *	aBuffList[6];
	aBuffList[0] = &m_lstVideo;
	aBuffList[1] = &m_lstAudio;
	aBuffList[2] = &m_lstEmpty;
	aBuffList[3] = &m_lstSubtt;
	aBuffList[4] = &m_lstNewVideo;
	aBuffList[5] = &m_lstNewAudio;

	for (int i = 0; i < 6; i++)
	{
		pPos = aBuffList[i]->GetHeadPosition();
		while (pPos != NULL)
		{
			pBuff = aBuffList[i]->GetNext(pPos);
			llMemSize += pBuff->uBuffSize;
		}
	}
	return llMemSize;
}

long long CBuffMng::GetBuffTime(QCMediaType nType)
{
	CAutoLock lock (&m_lckList);
	if (!SetCurrentList (nType, true))
		return 0;

	long long		llHeadTime = 0;
	long long		llLastTime = 0;
	QC_DATA_BUFF *	pHeadBuff = NULL;
	QC_DATA_BUFF *	pLastBuff = NULL;
	NODEPOS			pPos = NULL;

	pHeadBuff = m_pCurList->GetHead ();
	pLastBuff = m_pCurList->GetTail();
	if (m_pCurList->GetCount () > 1)
	{
		pPos = m_pCurList->GetHeadPosition();
		while (pPos != NULL)
		{
			pHeadBuff = m_pCurList->GetNext(pPos);
			if ((pHeadBuff->uFlag & QCBUFF_HEADDATA) !=  QCBUFF_HEADDATA)
			{
				llHeadTime = pHeadBuff->llTime;
				break;
			}
		}
		pPos = m_pCurList->GetTailPosition();
		while (pPos != NULL)
		{
			pLastBuff = m_pCurList->GetPrev(pPos);
			if ((pHeadBuff->uFlag & QCBUFF_HEADDATA) !=  QCBUFF_HEADDATA)
			{
				llLastTime = pLastBuff->llTime;
				break;
			}
		}
	}
	else if (pHeadBuff != NULL)
	{
		llHeadTime = pHeadBuff->llTime;
		llLastTime = pLastBuff->llTime;
	}

	if (nType == QC_MEDIA_Video && m_lstNewVideo.GetCount () > 1)
		pLastBuff = m_lstNewVideo.GetTail ();
	if (nType == QC_MEDIA_Audio && m_lstNewAudio.GetCount () > 1)
		pLastBuff = m_lstNewAudio.GetTail ();
	if (pLastBuff != NULL && pLastBuff->llTime > llLastTime)
		llLastTime = pLastBuff->llTime;

	if (llLastTime > llHeadTime)
		return llLastTime - llHeadTime;
	else
	{
		long long llBuffTime = 0;
		llHeadTime = 0;
		llLastTime = 0;
		pPos = m_pCurList->GetHeadPosition();
		while (pPos != NULL)
		{
			pHeadBuff = m_pCurList->GetNext(pPos);
			if ((pHeadBuff->uFlag & QCBUFF_HEADDATA) ==  QCBUFF_HEADDATA)
				continue;
			if (llHeadTime == 0)
				llHeadTime = pHeadBuff->llTime;
			if (pHeadBuff->llTime < llHeadTime)
			{
				llBuffTime = llBuffTime + (llLastTime - llHeadTime);
				llHeadTime = pHeadBuff->llTime;
			}
			llLastTime = pHeadBuff->llTime;
		}
		llBuffTime = llBuffTime + (llLastTime - llHeadTime);
		return llBuffTime;
	}
}

bool CBuffMng::SetCurrentList (QCMediaType nType, bool bRead)
{
	switch (nType)
	{
	case QC_MEDIA_Video:
		m_pCurList = &m_lstVideo;
		break;
	case QC_MEDIA_Audio:
		m_pCurList = &m_lstAudio;
		break;
	case QC_MEDIA_Subtt:
		m_pCurList = &m_lstSubtt;
		break;
	default:
		return false;
	}
	if (!bRead && m_lstNewVideo.GetCount () > 0)
	{
		switch (nType)
		{
		case QC_MEDIA_Video:
			m_pCurList = &m_lstNewVideo;
			break;
		case QC_MEDIA_Audio:
			m_pCurList = &m_lstNewAudio;
			break;
		default:
			return false;
		}
	}
	return true;
}

int	CBuffMng::CopyNewFormat (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	if (pBuff->nMediaType == QC_MEDIA_Video && pBuff->pFormat != NULL)
	{
		if (m_pFmtVideo == NULL)
			m_pFmtVideo = qcavfmtCloneVideoFormat((QC_VIDEO_FORMAT *)pBuff->pFormat);

		QC_VIDEO_FORMAT * pFmtVideo = qcavfmtCloneVideoFormat ((QC_VIDEO_FORMAT *)pBuff->pFormat);
		if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
		{
			QC_DEL_A (pFmtVideo->pHeadData);
			pFmtVideo->nHeadSize = 0;
		}
		pBuff->pFormat = pFmtVideo;

		if (m_lstFmtVideo.GetCount () > 0)
		{
			QC_VIDEO_FORMAT * pFmtOld = m_lstFmtVideo.GetTail ();
			m_bNewBetter = pFmtVideo->nWidth > pFmtOld->nWidth ? true : false;
			if (m_nSelStream >= 0)
				m_bNewBetter = true;
		}

		m_lstFmtVideo.AddTail (pFmtVideo);
		QCLOGI ("New video format % 8d X % 8d!", pFmtVideo->nWidth, pFmtVideo->nHeight);
	}
	else if (pBuff->nMediaType == QC_MEDIA_Audio && pBuff->pFormat != NULL)
	{
		if (m_pFmtAudio == NULL)
			m_pFmtAudio = qcavfmtCloneAudioFormat((QC_AUDIO_FORMAT *)pBuff->pFormat);

		QC_AUDIO_FORMAT * pFmtAudio = qcavfmtCloneAudioFormat ((QC_AUDIO_FORMAT *)pBuff->pFormat);
		if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
		{
			QC_DEL_A (pFmtAudio->pHeadData);
			pFmtAudio->nHeadSize = 0;
		}
		pBuff->pFormat = pFmtAudio;
		m_lstFmtAudio.AddTail (pFmtAudio);
		QCLOGI ("New Audio format % 8d X % 8d", pFmtAudio->nSampleRate, pFmtAudio->nChannels);
	}
	return QC_ERR_NONE;
}

void CBuffMng::ReleaseBuff (bool bFree)
{
	CAutoLock lock (&m_lckList);

	if (!bFree)
	{
		EmptyBuffList (&m_lstVideo);
		EmptyBuffList (&m_lstAudio);
		EmptyBuffList (&m_lstSubtt);
		EmptyBuffList (&m_lstNewAudio);
		EmptyBuffList (&m_lstNewVideo);
	}
	else
	{
		int nBuffNum = m_lstEmpty.GetCount();
		nBuffNum += m_lstVideo.GetCount();
		nBuffNum += m_lstAudio.GetCount();
		nBuffNum += m_lstSubtt.GetCount();
		nBuffNum += m_lstNewAudio.GetCount();
		nBuffNum += m_lstNewVideo.GetCount();
		if (nBuffNum != m_nNewEmptyNum)
			QCLOGW("There is memory leak in buffer manager! Empty = %d, Used = %d", m_nNewEmptyNum, nBuffNum);

		FreeListBuff (&m_lstEmpty);
		FreeListBuff (&m_lstVideo);
		FreeListBuff (&m_lstAudio);
		FreeListBuff (&m_lstSubtt);
		FreeListBuff (&m_lstNewAudio);
		FreeListBuff (&m_lstNewVideo);
		m_nNewEmptyNum = 0;
	}

	QC_VIDEO_FORMAT * pFmtVideo = m_lstFmtVideo.RemoveHead ();
	while (pFmtVideo != NULL)
	{
		qcavfmtDeleteVideoFormat (pFmtVideo);
		pFmtVideo = m_lstFmtVideo.RemoveHead ();
	}
	QC_AUDIO_FORMAT * pFmtAudio = m_lstFmtAudio.RemoveHead ();
	while (pFmtAudio != NULL)
	{
		qcavfmtDeleteAudioFormat (pFmtAudio);
		pFmtAudio = m_lstFmtAudio.RemoveHead ();
	}

	if (m_pFmtVideo != NULL)
	{
		qcavfmtDeleteVideoFormat(m_pFmtVideo);
		m_pFmtVideo = NULL;
	}
	if (m_pFmtAudio != NULL)
	{
		qcavfmtDeleteAudioFormat(m_pFmtAudio);
		m_pFmtAudio = NULL;
	}
	ResetParam();
}

void CBuffMng::FlushBuff(void)
{
	CAutoLock lock(&m_lckList);

	EmptyBuffList(&m_lstVideo);
	EmptyBuffList(&m_lstAudio);
	EmptyBuffList(&m_lstSubtt);
	EmptyBuffList(&m_lstNewAudio);
	EmptyBuffList(&m_lstNewVideo);

	m_llNextKeyTime = 0;

	m_bFlush = true;
}

void CBuffMng::EmptyBuff(QCMediaType nType)
{
	CAutoLock lock(&m_lckList);
	if (nType == QC_MEDIA_Audio)
	{
		EmptyBuffList(&m_lstAudio);
		EmptyBuffList(&m_lstNewAudio);
	}
	else if (nType == QC_MEDIA_Video)
	{
		EmptyBuffList(&m_lstVideo);
		EmptyBuffList(&m_lstNewVideo);
	}
	else
	{
		EmptyBuffList(&m_lstSubtt);
	}
}

void CBuffMng::EmptyBuffList (CObjectList<QC_DATA_BUFF> * pList)
{
	QC_DATA_BUFF * pBuff = pList->RemoveHead ();
	while (pBuff != NULL)
	{
		m_lstEmpty.AddHead (pBuff);
		pBuff = pList->RemoveHead ();
	}
}

void CBuffMng::FreeListBuff (CObjectList<QC_DATA_BUFF> * pList)
{
	if (pList == NULL)
		return;
	CAutoLock lock (&m_lckList);
	QC_DATA_BUFF * pBuff = pList->RemoveHead ();
	while (pBuff != NULL)
	{
		DeleteBuff (pBuff, true);
		pBuff = pList->RemoveHead ();
	}
	return;
}

void CBuffMng::EmptyListBuff (CObjectList<QC_DATA_BUFF> * pList)
{
	if (pList == NULL)
		return;
	CAutoLock lock (&m_lckList);
	QC_DATA_BUFF * pBuff = pList->RemoveHead ();
	while (pBuff != NULL)
	{
		m_lstEmpty.AddHead (pBuff);
		pBuff = pList->RemoveHead ();
	}
	return;
}

void CBuffMng::SwitchBuffList (QC_DATA_BUFF * pKeyBuff, CObjectList<QC_DATA_BUFF> * pListNew, CObjectList<QC_DATA_BUFF> * pListPlay)
{
	if (pListNew == NULL || pListPlay == NULL)
		return;

	QC_DATA_BUFF * pTempBuff = NULL;
	QC_DATA_BUFF * pHeadBuff = NULL;
	if (pKeyBuff == NULL)
	{
		pTempBuff = pListNew->RemoveHead ();
		while (pTempBuff != NULL)
			pListPlay->AddTail (pTempBuff);
		return;
	}
			
	pTempBuff = pListPlay->GetTail ();
	while (pTempBuff != NULL)
	{
		if (pTempBuff->llTime > 0 && pTempBuff->llTime < pKeyBuff->llTime)
			break;
		pTempBuff = pListPlay->RemoveTail ();	
		m_lstEmpty.AddHead (pTempBuff);
		pTempBuff = pListPlay->GetTail ();
	}

	pTempBuff = pListNew->RemoveHead ();
	while (pTempBuff != NULL)
	{
		if ((pTempBuff->uFlag & QCBUFF_HEADDATA) != 0)
		{
			pHeadBuff = pTempBuff;
			pTempBuff->llTime = pKeyBuff->llTime;
			pListPlay->AddTail (pTempBuff);
		}
		else
		{
			if (pTempBuff->llTime < pKeyBuff->llTime)
			{
				m_lstEmpty.AddHead(pTempBuff);
			}
			else
			{
				if (pHeadBuff != NULL)
				{
					pHeadBuff->llTime = pTempBuff->llTime;
					pHeadBuff = NULL;
				}
				pListPlay->AddTail(pTempBuff);
			}
		}
		pTempBuff = pListNew->RemoveHead ();
	}
	return;
}

QC_DATA_BUFF * CBuffMng::NewEmptyBuff (void)
{
	m_nNewEmptyNum++;

	QC_DATA_BUFF * pBuff = new QC_DATA_BUFF ();
	memset (pBuff, 0, sizeof (QC_DATA_BUFF));
	pBuff->uBuffType = QC_BUFF_TYPE_Data;
	return pBuff;
}

int	CBuffMng::DeleteBuff (QC_DATA_BUFF * pBuff, bool bForce)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	if (pBuff->nUsed > 0 && !bForce)
		return QC_ERR_STATUS;

	int nErr = FreeBuffData (pBuff, bForce);
	QC_ERR_CHECK (nErr);

	delete pBuff;
	return QC_ERR_NONE;
}

int CBuffMng::FreeBuffData (QC_DATA_BUFF * pBuff, bool bForce)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;
	if (pBuff->nUsed > 0 && !bForce)
		return QC_ERR_STATUS;

	switch (pBuff->uBuffType)
	{
	case QC_BUFF_TYPE_Data:
		if (pBuff->pBuff != NULL)
			QC_DEL_A (pBuff->pBuff);
		pBuff->uSize = 0;
		break;

	case QC_BUFF_TYPE_Video:
		if (pBuff->pBuffPtr != NULL)
		{
			QC_VIDEO_BUFF * pVideo = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
			QC_DEL_A (pVideo->pBuff[0]);
			QC_DEL_A (pVideo->pBuff[1]);
			QC_DEL_A (pVideo->pBuff[2]);
			QC_DEL_P (pVideo);
			pBuff->pBuffPtr = NULL;
		}
		pBuff->uBuffType = QC_BUFF_TYPE_MAX;
		pBuff->uSize = 0;
		break;

	default:
		break;
	}

	return QC_ERR_NONE;
}

QC_DATA_BUFF * CBuffMng::CloneBuff (QC_DATA_BUFF * pBuff)
{
	return NULL;
}

void CBuffMng::AnlBufferInfo(QC_DATA_BUFF* pBuff)
{
    if(!pBuff || !pBuff->pBuff || pBuff->uSize<=0)
        return;
    
    if (pBuff->nMediaType == QC_MEDIA_Video && (pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME)
    {
        if(m_llGopTime == -1)
            m_llGopTime = pBuff->llTime;
        else if(m_llGopTime != -2)
        {
            if(pBuff->llTime - m_llGopTime <= 0)
                m_llGopTime = -1;
            else
            {
				if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
					m_pBaseInst->m_pMsgMng->Notify (QC_MSG_BUFF_GOPTIME, (int)(pBuff->llTime - m_llGopTime), 0);
                m_llGopTime = -2;
            }
        }
    }

    int nInterval = 5000;
    if (pBuff->nMediaType == QC_MEDIA_Video)
    {
        m_nVideoFrameCount++;
        if(pBuff->uSize > 0)
        	m_llVideoFrameSize += pBuff->uSize;
        if(m_llVideoRecTime == -1)
            m_llVideoRecTime = pBuff->llTime;
        if((pBuff->llTime - m_llVideoRecTime) >= nInterval)
        {
            long long llTime = pBuff->llTime - m_llVideoRecTime;
            //QCLOGI("play.v5, total %lld, time %lld, bitrate %lld", m_nVideoFrameSize, pBuff->llTime - m_llVideoRecTime, (m_nVideoFrameSize*8*1000)/(pBuff->llTime - m_llVideoRecTime))
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
			{
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_VBITRATE, (int)(m_llVideoFrameSize * 8 * 1000 / llTime), 0);
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_VFPS, m_nVideoFrameCount * 1000 / (int)llTime, 0);
			}
            m_nVideoFrameCount = 0;
            m_llVideoRecTime = -1;
            m_llVideoFrameSize = 0;
        }
    }
    if (pBuff->nMediaType == QC_MEDIA_Audio)
    {
        m_nAudioFrameCount++;
        if(pBuff->uSize > 0)
        	m_llAudioFrameSize += pBuff->uSize;
        if(m_llAudioRecTime == -1)
            m_llAudioRecTime = pBuff->llTime;
        if((pBuff->llTime - m_llAudioRecTime) >= nInterval)
        {
            long long llTime = pBuff->llTime - m_llAudioRecTime;
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL) {
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_ABITRATE, (int)(m_llAudioFrameSize * 8 * 1000 / llTime), 0);
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_AFPS, m_nAudioFrameCount * 1000 / (int)llTime, 0);
			}
            m_nAudioFrameCount = 0;
            m_llAudioRecTime = -1;
            m_llAudioFrameSize = 0;
        }
    }
    
    // parse SEI
    if (pBuff->nMediaType == QC_MEDIA_Video && QC_CODEC_ID_H264 == GetCodecType(pBuff->nMediaType))
    {
        int startCodeLen = qcAV_IsNalUnit(pBuff->pBuff, pBuff->uSize);
        
        if(startCodeLen > 0)
        {
            unsigned char* pSEI = NULL;
            unsigned char* pCurrPos = pBuff->pBuff;
            
            while ((int)(pCurrPos - pBuff->pBuff) < (int)pBuff->uSize)
            {
                if (XRAW_IS_ANNEXB2(pCurrPos) || XRAW_IS_ANNEXB(pCurrPos))
                {
                    int type = (*(pCurrPos + startCodeLen) & 0x1f);
                    if (6 == type)
                    {
						if (pSEI != NULL)
							NotifySEIData(pSEI, (int)(pCurrPos - pSEI), pBuff->llTime);
                        pSEI = pCurrPos + startCodeLen;
                    }
					else if (pSEI != NULL)
                    {
						NotifySEIData(pSEI, (int)(pCurrPos - pSEI), pBuff->llTime);
						pSEI = NULL;
                    }
                    pCurrPos += startCodeLen;
                }
                else
                {
                    pCurrPos++;
                }
            } // end of while
            
            // not found next nal unit
            if(pSEI != NULL)
				NotifySEIData(pSEI, (int)(pCurrPos - pSEI), pBuff->llTime);
        }
    }
}

void CBuffMng::NotifySEIData(unsigned char * pData, int nSize, long long llTime)
{
	QC_DATA_BUFF buf;
	buf.llTime = llTime;
	buf.pBuff = pData;
	buf.uSize = nSize;
	if (buf.uSize > 0 && m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
        m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_SEI_DATA, 0, 0, NULL, &buf);
}

void CBuffMng::NotifyBuffTime(void)
{
	if (m_nLastNotifyTime == 0)
		m_nLastNotifyTime = qcGetSysTime();
	if (qcGetSysTime() >= m_nLastNotifyTime + 1000)
	{
		if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		{
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_VBUFFTIME, (int)GetBuffTime(QC_MEDIA_Video), m_lstVideo.GetCount());
			m_pBaseInst->m_pMsgMng->Notify(QC_MSG_BUFF_ABUFFTIME, (int)GetBuffTime(QC_MEDIA_Audio), m_lstAudio.GetCount());
		}
		m_nLastNotifyTime = qcGetSysTime();
	}
}

void CBuffMng::ResetParam(void)
{
	m_pCurList = NULL;
	m_pLstSendVideo = NULL;
	m_pLstSendAudio = NULL;
	m_nSelStream = -1;
	m_bNewBetter = false;
	m_bFlush = false;
	m_llLastSendTime = -1;

	m_bEOS = false;
	m_bEOA = false;
	m_bEOV = false;
	m_llNextKeyTime = 0;
	m_llSeekPos = 0;
	m_llVideoRecTime = -1;
	m_llAudioRecTime = -1;
	m_nVideoFrameCount = 0;
	m_nAudioFrameCount = 0;

	m_llLastVideoTime = 0;
	m_nLastNotifyTime = 0;

	m_llGetLastVTime = 0;
	m_llGetLastATime = 0;

	m_nNumGet = 0;
	m_nNumSend = 0;
	m_nNumRead = 0;
	m_nNumReturn = 0;
}

int CBuffMng::GetCodecType(QCMediaType nType)
{
    if (nType == QC_MEDIA_Video)
    {
        if (m_lstFmtVideo.GetCount () > 0)
        {
            QC_VIDEO_FORMAT * pFmt = m_lstFmtVideo.GetTail ();
            if(pFmt)
                return pFmt->nCodecID;
        }
    }
    else if (nType == QC_MEDIA_Audio)
    {
        if (m_lstFmtAudio.GetCount () > 0)
        {
            QC_AUDIO_FORMAT * pFmt = m_lstFmtAudio.GetTail ();
            if(pFmt)
                return pFmt->nCodecID;
        }
    }
    
    return QC_CODEC_ID_NONE;
}

int CBuffMng::SetFormat(QCMediaType nType, void * pFormat)
{
	if (pFormat == NULL)
		return QC_ERR_ARG;
	if (nType == QC_MEDIA_Video)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pFormat;
		if (pFmt->nCodecID < QC_CODEC_ID_H264 || pFmt->nCodecID > QC_CODEC_ID_MJPEG)
			return QC_ERR_FORMAT;
		if (pFmt->pHeadData == NULL && pFmt->nCodecID != QC_CODEC_ID_MJPEG)
			return QC_ERR_FORMAT;
		if (m_pFmtVideo != NULL)
			qcavfmtDeleteVideoFormat(m_pFmtVideo);
		m_pFmtVideo = qcavfmtCloneVideoFormat(pFmt);
	}
	else if (nType == QC_MEDIA_Audio)
	{
		QC_AUDIO_FORMAT * pFmt = (QC_AUDIO_FORMAT *)pFormat;
		if (pFmt->nCodecID < QC_CODEC_ID_AAC || pFmt->nCodecID > QC_CODEC_ID_G726)
			return QC_ERR_FORMAT;
		if (m_pFmtAudio != NULL)
			qcavfmtDeleteAudioFormat(m_pFmtAudio);
		m_pFmtAudio = qcavfmtCloneAudioFormat(pFmt);
	}
	else
	{
		return QC_ERR_FORMAT;
	}
	return QC_ERR_NONE;
}

int CBuffMng::WriteFrame(QC_DATA_BUFF *pBuff)
{
    if (m_pBaseInst && m_pBaseInst->m_pMuxer)
    {
		if (m_pFmtVideo == NULL || m_pFmtAudio == NULL)
		{
			if (m_nNumSend <= 100)
				return QC_ERR_STATUS;
		}

        if (m_pBaseInst->m_bBeginMux)
        {
            m_pBaseInst->m_bBeginMux = false;
			m_pBaseInst->m_pMuxer->Init(m_pFmtVideo, m_pFmtAudio);

			QC_DATA_BUFF *	pBuffVideo = NULL;
			QC_DATA_BUFF *	pBuffAudio = NULL;

			NODEPOS pPosVideo = m_lstVideo.GetHeadPosition();
			NODEPOS pPosAudio = m_lstAudio.GetHeadPosition();
			while (pPosVideo != NULL)
			{
				pBuffVideo = m_lstVideo.GetNext(pPosVideo);
				m_pBaseInst->m_pMuxer->Write(pBuffVideo);

				while (pPosAudio != NULL)
				{
					if (pBuffAudio == NULL)
						pBuffAudio = m_lstAudio.GetNext(pPosAudio);
					if (pBuffAudio->llTime > pBuffVideo->llTime)
						break;
					m_pBaseInst->m_pMuxer->Write(pBuffAudio);
					pBuffAudio = NULL;
				}
			}

			// write the rest audio buffer
			if (pBuffAudio != NULL)
				m_pBaseInst->m_pMuxer->Write(pBuffAudio);
			while (pPosAudio != NULL)
			{
				pBuffAudio = m_lstAudio.GetNext(pPosAudio);
				m_pBaseInst->m_pMuxer->Write(pBuffAudio);
			}
        }
        
        m_pBaseInst->m_pMuxer->Write(pBuff);
    }
    return QC_ERR_NONE;
}
