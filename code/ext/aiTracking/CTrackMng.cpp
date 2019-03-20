/*******************************************************************************
	File:		CTrackMng.cpp

	Contains:	The ai track manager implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2018-03-02		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"
#include "qcErr.h"

#include "CTrackMng.h"
#include "CMsgMng.h"
#include "USystemFunc.h"
#include "cJSON.h"

CTrackMng::CTrackMng(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
{
	SetObjectName ("CTrackMng");
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RegNotify(this);
	InitSEICtx();
}

CTrackMng::~CTrackMng(void)
{
	UnInitSEICtx();
	QCLOGI ("Destroy CTrackMng");	
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RemNotify(this);

	if (m_pVideoRndBuff)
	{
		QC_DEL_A(m_pVideoRndBuff->pBuff[0]);
		QC_DEL_A(m_pVideoRndBuff->pBuff[1]);
		QC_DEL_A(m_pVideoRndBuff->pBuff[2]);
		delete m_pVideoRndBuff;
	}
}

int CTrackMng::SendVideoBuff(QC_DATA_BUFF * pVideoBuff)
{
	QC_VIDEO_BUFF * pVideoData = NULL;
	S_SEI_Info*   pSeiInfo = NULL;
	int iRet = 0;
	int h = 0;

	if (pVideoBuff == NULL)
		return QC_ERR_ARG;

	pVideoData = (QC_VIDEO_BUFF *)pVideoBuff->pBuffPtr;
	if (pVideoData != NULL)
	{
		TransFrameVideoBuf(pVideoData, pVideoBuff->llTime);
	}

	pVideoBuff->pData = m_pVideoRndBuff;
	pVideoBuff->nDataType = QC_BUFF_TYPE_Video;
	return QC_ERR_NONE;
}

int	CTrackMng::ReceiveMsg(CMsgItem * pItem)
{
	if (pItem == NULL)
		return QC_ERR_ARG;

	if (pItem->m_nMsgID != QC_MSG_BUFF_SEI_DATA)
		return QC_ERR_NONE;

	QC_DATA_BUFF * pSEIData = (QC_DATA_BUFF *)pItem->m_pInfo;
	AddSEIInfo(pSEIData);

	return QC_ERR_NONE;
}


int  CTrackMng::InitSEICtx()
{
	int iRet = 0;
	int   iIndex = 0;
	m_iBoxCount = 0;

	m_pVideoRndBuff = NULL;
	for (iIndex = 0; iIndex < DEFAULT_MAX_FACE_COUNT; iIndex++)
	{
		InitDrawBoxContext(&(m_sDrawBox[iIndex]));
	}

	m_iCurFrameIdx = 0;
	m_iCurCachedFrameIdx = 0;
	m_illLastDrawTime = 0;

	m_pSEIArray = new S_SEI_Info[AIDRAW_MAX_SEI_ENTRY_COUNT];
	if (m_pSEIArray == NULL)
	{
		iRet = QC_ERR_MEMORY;
	}

	m_iCurInputIndex = m_iCurOutputIndex = m_iCurSeiCount = 0;
	return iRet;
}

int  CTrackMng::UnInitSEICtx()
{
	int iRet = 0;
	if (m_pSEIArray != NULL)
	{
		delete[] m_pSEIArray;
		m_pSEIArray = NULL;
	}

	return iRet;
}

int  CTrackMng::VideoProc(void* pBuffer, char*   pPrivate, long long ullTimeStamp)
{
	int iRet = 0;
	long long  ullLastTime = 0;
	long long  ullCurTime = 0;
	bool    bTrack = false;
	unsigned char*   pYUVData[4] = { 0 };
	int     ilinesizes[4] = { 0 };
	int     iIndex = 0;



	QC_VIDEO_BUFF *  pVideoBuff = (QC_VIDEO_BUFF *)pBuffer;
	int  iFaceCount = 0;

	if (pPrivate != NULL && strstr(pPrivate, "\"pts\":[[") != NULL)
	{
		m_iBoxCount = 0;
		ParsePosInfo(pPrivate);
		QCLOGI("time:%lld , desc:%s", ullTimeStamp, pPrivate);
		m_illLastDrawTime = ullTimeStamp;
	}
	else
	{
		if ((ullTimeStamp - m_illLastDrawTime) > FACE_TRACK_MAX_DELAY_TIME)
		{
			m_iBoxCount = 0;
			m_illLastDrawTime = ullTimeStamp;
		}
	}



	iFaceCount = m_iBoxCount;
	//QCLOGI("face count:%d", iFaceCount);
	for (iIndex = 0; iIndex < 3; iIndex++)
	{
		pYUVData[iIndex] = pVideoBuff->pBuff[iIndex];
		ilinesizes[iIndex] = pVideoBuff->nStride[iIndex];
	}

	for (iIndex = 0; iIndex < iFaceCount; iIndex++)
	{
		DoDrawBoxContext(&m_sDrawBox[iIndex], pYUVData, ilinesizes, pVideoBuff->nWidth, pVideoBuff->nHeight);
	}

	return iRet;
}

int  CTrackMng::TransFrameVideoBuf(QC_VIDEO_BUFF* pVideoBuff, long long ullTimeStamp)
{
	int h = 0;
	S_SEI_Info*  pSeiInfo = NULL;
	unsigned char*   pFindJsonStart = NULL;

	if (m_pVideoRndBuff == NULL)
	{
		m_pVideoRndBuff = new QC_VIDEO_BUFF();
		memset(m_pVideoRndBuff, 0, sizeof(QC_VIDEO_BUFF));
	}

	if (m_pVideoRndBuff->nWidth < pVideoBuff->nWidth || m_pVideoRndBuff->nHeight < pVideoBuff->nHeight)
	{
		QC_DEL_A(m_pVideoRndBuff->pBuff[0]);
		QC_DEL_A(m_pVideoRndBuff->pBuff[1]);
		QC_DEL_A(m_pVideoRndBuff->pBuff[2]);
	}

	m_pVideoRndBuff->nType = pVideoBuff->nType;
	m_pVideoRndBuff->nWidth = pVideoBuff->nWidth;
	m_pVideoRndBuff->nHeight = pVideoBuff->nHeight;
	m_pVideoRndBuff->nRatioDen = pVideoBuff->nRatioDen;
	m_pVideoRndBuff->nRatioNum = pVideoBuff->nRatioNum;
	if (m_pVideoRndBuff->pBuff[0] == NULL)
	{
		m_pVideoRndBuff->pBuff[0] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight];
		m_pVideoRndBuff->pBuff[1] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight / 4];
		m_pVideoRndBuff->pBuff[2] = new unsigned char[pVideoBuff->nWidth * pVideoBuff->nHeight / 4];
		m_pVideoRndBuff->nStride[0] = pVideoBuff->nWidth;
		m_pVideoRndBuff->nStride[1] = m_pVideoRndBuff->nStride[0] / 2;
		m_pVideoRndBuff->nStride[2] = m_pVideoRndBuff->nStride[0] / 2;
	}

	for (h = 0; h < pVideoBuff->nHeight; h++)
	{
		memcpy(m_pVideoRndBuff->pBuff[0] + h * m_pVideoRndBuff->nStride[0], pVideoBuff->pBuff[0] + pVideoBuff->nStride[0] * h, pVideoBuff->nWidth);
	}
	for (h = 0; h < pVideoBuff->nHeight / 2; h++)
	{
		memcpy(m_pVideoRndBuff->pBuff[1] + h * m_pVideoRndBuff->nStride[1], pVideoBuff->pBuff[1] + pVideoBuff->nStride[1] * h, pVideoBuff->nWidth / 2);
		memcpy(m_pVideoRndBuff->pBuff[2] + h * m_pVideoRndBuff->nStride[2], pVideoBuff->pBuff[2] + pVideoBuff->nStride[2] * h, pVideoBuff->nWidth / 2);
	}

	pSeiInfo = GetSEIInfo(ullTimeStamp);
	if (pSeiInfo != NULL)
	{
		ExtractSeiPrivate(pSeiInfo->uSeiInfo, pSeiInfo->iSeiInfoSize, &pFindJsonStart);
		VideoProc(m_pVideoRndBuff, (char* )pFindJsonStart, ullTimeStamp);
	}
	else
	{
		VideoProc(m_pVideoRndBuff, NULL, ullTimeStamp);
	}

	return 0;
}

int  CTrackMng::ParsePosInfo(char*   pPrivate)
{
	int iRet = 0;
	char*   pFindJsonStart = NULL;
	cJSON*   pResult = NULL;
	cJSON*   pJson = NULL;
	cJSON*   pDetectArray = NULL;
	cJSON*   pDetectItem = NULL;
	int      iDetectCount = 0;
	int      iIndex = 0;

	do 
	{
		//Purge the String
		if (pPrivate == NULL || strlen(pPrivate) == 0)
		{
			break;
		}

		//set the last sei data 0x80 to 0
		pPrivate[strlen(pPrivate) - 1] = 0;
		pFindJsonStart = strstr(pPrivate, ":");
		if (pFindJsonStart == NULL)
		{
			break;
		}

		pJson = cJSON_Parse((const char *)(pFindJsonStart+1));
		if (pJson == NULL)
		{
			break;
		}

		pResult = cJSON_GetObjectItem(pJson, "result");
		if (pResult == NULL)
		{
			break;
		}

		pDetectArray = cJSON_GetObjectItem(pResult, "detections");
		if (pDetectArray == NULL)
		{
			break;
		}

		iDetectCount = cJSON_GetArraySize(pDetectArray);
		for (iIndex = 0; iIndex < iDetectCount; iIndex++)
		{
			pDetectItem = cJSON_GetArrayItem(pDetectArray, iIndex);
			
			if (pDetectItem != NULL)
			{
				ParseDetectJsonItem((void*)pDetectItem);
			}
			else
			{
				continue;
			}
		}

	} while (0);

	if (pJson != NULL)
	{
		cJSON_Delete(pJson);
	}

	return iRet;
}


int  CTrackMng::ExtractSeiPrivate(unsigned char*  pFrameData, int iFrameSize, unsigned char**  ppOutput)
{
	int  iRet = 0;
	unsigned char*   pSeiStart = pFrameData + 1;
	int iSeiType = 0;
	int iSeiPayloadSize = 0;
	unsigned char*  pEnd = pFrameData+iFrameSize;

	do 
	{
		while (*pSeiStart == 0xff && pSeiStart < pEnd)
		{
			iSeiType += 255;
			pSeiStart++;
		}

		iSeiType += *(pSeiStart);
		pSeiStart++;

		//Get the SEI Size
		while (*pSeiStart == 0xff && pSeiStart < pEnd)
		{
			iSeiPayloadSize += 255;
			pSeiStart++;
		}

		iSeiPayloadSize += *(pSeiStart);
		pSeiStart++;
		pSeiStart += 16;


		//Sei Data Error
		if (*pSeiStart < 0x20)
		{
			iRet = 1;
			break;
		}

		*ppOutput = pSeiStart;
	} while (0);

	return 0;
}

int  CTrackMng::AddSEIInfo(QC_DATA_BUFF* pBuf)
{
	int iRet = 0;
	int iSeiPlayloadSize = 0;
	S_SEI_Info*   pSeiEntry = NULL;
	unsigned char*  pData = NULL;

	do 
	{
		pData = pBuf->pBuff;
		if (pBuf->uSize > 16 && pData[1] == 5)
		{
			pSeiEntry = &(m_pSEIArray[m_iCurInputIndex]);
			if (iSeiPlayloadSize < 8196 )
			{
				CAutoLock   sAutoLock(&m_mtFunc);
				memset(pSeiEntry->uSeiInfo, 0, 8196);
				pSeiEntry->ullTimeStamp = pBuf->llTime;
				memcpy(pSeiEntry->uSeiInfo, pData, pBuf->uSize);
				pSeiEntry->iSeiInfoSize = pBuf->uSize;
				//QCLOGI("input detect, time:%lld", pSeiEntry->ullTimeStamp);
				m_iCurInputIndex = (m_iCurInputIndex + 1) % AIDRAW_MAX_SEI_ENTRY_COUNT;
				m_iCurSeiCount++;
			}
		}
	} while (0);
	return iRet;
}

S_SEI_Info*   CTrackMng::GetSEIInfo(long long ullTimeStamp)
{
	S_SEI_Info*  pRet = NULL;
	S_SEI_Info*  pCurSEI = NULL;
	int  iIndex = m_iCurOutputIndex;
	int  iSkipCount = 0;
	int  iFindSei = 0;
	long long ullTimeFindMin = 0;
	long long ullTimeFindMax = ullTimeStamp;
	int   iIndexCheck = 0;


	do
	{
		if (m_iCurSeiCount == 0)
		{
			break;
		}

		//fix timestamp
		ullTimeFindMin = (ullTimeStamp < 20) ? 0 : (ullTimeStamp - 20);
		ullTimeFindMax = ullTimeStamp + 20;


		while (iSkipCount < m_iCurSeiCount)
		{
			iIndexCheck = (m_iCurOutputIndex + iSkipCount) % AIDRAW_MAX_SEI_ENTRY_COUNT;
			pCurSEI = &(m_pSEIArray[iIndexCheck]);
			if (pCurSEI->ullTimeStamp < ullTimeFindMin)
			{
				iSkipCount++;
			}
			else
			{
				if (pCurSEI->ullTimeStamp <= ullTimeFindMax)
				{
					iFindSei = 1;
					pRet = pCurSEI;
					iSkipCount++;
				}
				break;
			}
		}

		{
			CAutoLock   sAutoLock(&m_mtFunc);
			m_iCurSeiCount -= iSkipCount;
			m_iCurOutputIndex = (m_iCurOutputIndex + iSkipCount) % AIDRAW_MAX_SEI_ENTRY_COUNT;
		}
	} while (0);

	return pRet;
}

int   CTrackMng::GetDetectInfoType(char*   pDetectInfo)
{
	return 0;
}

int   CTrackMng::ParseDetectJsonItem(void*  pDetectJson)
{
	int iRet = 0;
	cJSON*    pTypeNode = NULL;
	cJSON*    pTypeObject = NULL;
	cJSON*    pJson = NULL;
	cJSON*    pBindingBoxs = NULL;
	cJSON*    pPTS = NULL;
	cJSON*    pGroups = NULL;
	int       iX0, iY0, iX1, iY1;
	int       iGroupCount = 0;
	int       iIndex = 0;
	int       iTypeValue = AI_NORMAL_TYPE_VALUE;

	do 
	{
		pJson = (cJSON*)pDetectJson;
		if (pJson == NULL)
		{
			break;
		}

		pBindingBoxs = cJSON_GetObjectItem(pJson, "boundingBox");
		if (pBindingBoxs == NULL)
		{
			break;
		}

		pPTS = cJSON_GetObjectItem(pBindingBoxs, "pts");
		if (pPTS == NULL)
		{
			break;
		}

		//Get the Position
		iX0 = cJSON_GetArrayItem(pPTS, 0)->child->valueint;
		iY0 = cJSON_GetArrayItem(pPTS, 0)->child->next->valueint;
		iX1 = cJSON_GetArrayItem(pPTS, 2)->child->valueint; 
		iY1 = cJSON_GetArrayItem(pPTS, 2)->child->next->valueint;


		pGroups = cJSON_GetObjectItem(pJson, "groups");
		if (pGroups == NULL)
		{
			break;
		}

		iGroupCount = cJSON_GetArraySize(pGroups);
		for (iIndex = 0; iIndex < iGroupCount; iIndex++)
		{
			pTypeNode = cJSON_GetArrayItem(pGroups, iIndex);
			if (pTypeNode != NULL)
			{
				pTypeObject = cJSON_GetObjectItem(pTypeNode, "type");
				if (pTypeObject != NULL)
				{
					iTypeValue = pTypeObject->valueint;
					if (iTypeValue == AI_WARNING_TYPE_VALUE)
					{
						iTypeValue = AI_WARNING_TYPE_VALUE;
						break;
					}
				}
			}
		}

		m_sDrawBox[m_iBoxCount].x = iX0;
		m_sDrawBox[m_iBoxCount].y = iY0;
		m_sDrawBox[m_iBoxCount].w = iX1-iX0;
		m_sDrawBox[m_iBoxCount].h = iY1-iY0;
		m_sDrawBox[m_iBoxCount].iInfoType = iTypeValue;
		AdjustBackGroundColor(&(m_sDrawBox[m_iBoxCount]));
		m_iBoxCount++;
	} while (0);

	return iRet;
}
