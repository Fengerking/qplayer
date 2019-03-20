/*******************************************************************************
	File:		UMsgMng.cpp

	Contains:	The message manager implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "UMsgMng.h"
#include "qcData.h"
#include "CMsgMng.h"

/*
int	QCMSG_Init(CBaseInst * pBaseInst)
{
	if (pBaseInst == NULL)
		return QC_ERR_ARG;
	if (pBaseInst->m_pMsgMng != NULL)
		return QC_ERR_NONE;
	pBaseInst->m_pMsgMng = new CMsgMng(pBaseInst);
	return QC_ERR_NONE;
}

int	QCMSG_Close(CBaseInst * pBaseInst)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	QC_DEL_P(pMsgMng);
	return QC_ERR_NONE;
}

int	QCMSG_RegNotify(CBaseInst * pBaseInst, CMsgReceiver * pReceiver)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->RegNotify(pReceiver);
}

int	QCMSG_RemNotify(CBaseInst * pBaseInst, CMsgReceiver * pReceiver)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->RemNotify(pReceiver);
}

int	QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Notify(nMsg, nValue, llValue);
}

int	QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Notify(nMsg, nValue, llValue, pValue);
}

int	QCMSG_Notify(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue, void * pInfo)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Notify(nMsg, nValue, llValue, pValue, pInfo);
}

int	QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Send(nMsg, nValue, llValue);
}

int	QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Send(nMsg, nValue, llValue, pValue);
}

int	QCMSG_Send(CBaseInst * pBaseInst, int nMsg, int nValue, long long llValue, const char * pValue, void * pInfo)
{
	if (pBaseInst == NULL || pBaseInst->m_pMsgMng == NULL)
		return QC_ERR_ARG;
	CMsgMng * pMsgMng = (CMsgMng *)pBaseInst->m_pMsgMng;
	return pMsgMng->Send(nMsg, nValue, llValue, pValue, pInfo);
}
*/
int	QCMSG_ConvertName(int nMsg, char * pName, int nSize)
{
	switch (nMsg)
	{
	case QC_MSG_HTTP_CONNECT_START:
		strcpy (pName, "QC_MSG_HTTP_CONNECT_START");
		break;
	case QC_MSG_HTTP_CONNECT_FAILED:
		strcpy (pName, "QC_MSG_HTTP_CONNECT_FAILED");
		break;
	case QC_MSG_HTTP_CONNECT_SUCESS:
		strcpy (pName, "QC_MSG_HTTP_CONNECT_SUCESS");
		break;
    case QC_MSG_HTTP_RECONNECT_SUCESS:
        strcpy (pName, "QC_MSG_HTTP_RECONNECT_SUCESS");
        break;
	case QC_MSG_HTTP_DOWNLOAD_FINISH:
		strcpy(pName, "QC_MSG_HTTP_DOWNLOAD_FINISH");
		break;
	case QC_MSG_HTTP_DOWNLOAD_PERCENT:
		strcpy(pName, "QC_MSG_HTTP_DOWNLOAD_PERCENT");
		break;
	case QC_MSG_HTTP_CONTENT_SIZE:
		strcpy(pName, "QC_MSG_HTTP_CONTENT_SIZE");
		break;    
    case QC_MSG_HTTP_CONTENT_TYPE:
        strcpy(pName, "QC_MSG_HTTP_CONTENT_TYPE");
        break;
	case QC_MSG_HTTP_BUFFER_SIZE:
		strcpy(pName, "QC_MSG_HTTP_BUFFER_SIZE");
		break;
	case QC_MSG_HTTP_RECONNECT_FAILED:
        strcpy (pName, "QC_MSG_HTTP_RECONNECT_FAILED");
        break;
	case QC_MSG_HTTP_DNS_START:
		strcpy (pName, "QC_MSG_HTTP_DNS_START");
		break;
	case QC_MSG_HTTP_DNS_GET_CACHE:
		strcpy (pName, "QC_MSG_HTTP_DNS_GET_CACHE");
		break;
	case QC_MSG_HTTP_DNS_GET_IPADDR:
		strcpy (pName, "QC_MSG_HTTP_DNS_GET_IPADDR");
		break;
    case QC_MSG_HTTP_SEND_BYTE:
        strcpy (pName, "QC_MSG_HTTP_SEND_BYTE");
        break;
	case QC_MSG_HTTP_GET_HEADDATA:
		strcpy (pName, "QC_MSG_HTTP_GET_HEADDATA");
		break;
	case QC_MSG_HTTP_CONTENT_LEN:
		strcpy (pName, "QC_MSG_HTTP_CONTENT_LEN");
		break;
	case QC_MSG_HTTP_REDIRECT:
		strcpy (pName, "QC_MSG_HTTP_REDIRECT");
		break;
	case QC_MSG_HTTP_DISCONNECT_START:
		strcpy (pName, "QC_MSG_HTTP_DISCONNECT_START");
		break;
	case QC_MSG_HTTP_DISCONNECT_DONE:
		strcpy (pName, "QC_MSG_HTTP_DISCONNECT_DONE");
		break;
	case QC_MSG_HTTP_RETURN_CODE:
		strcpy (pName, "QC_MSG_HTTP_RETURN_CODE");
		break;	
	case QC_MSG_HTTP_DOWNLOAD_SPEED:
		strcpy (pName, "QC_MSG_HTTP_DOWNLOAD_SPEED");
		break;
	case QC_MSG_HTTP_DISCONNECTED:
		strcpy (pName, "QC_MSG_HTTP_DISCONNECTED");
		break;
	case QC_MSG_IO_FIRST_BYTE_DONE:
		strcpy(pName, "QC_MSG_IO_FIRST_BYTE_DONE");
		break;
	case QC_MSG_IO_SEEK_SOURCE_TYPE:
		strcpy(pName, "QC_MSG_IO_SEEK_SOURCE_TYPE");
		break;
    case QC_MSG_IO_HANDSHAKE_START:
        strcpy(pName, "QC_MSG_IO_HANDSHAKE_START");
        break;
    case QC_MSG_IO_HANDSHAKE_FAILED:
        strcpy(pName, "QC_MSG_IO_HANDSHAKE_FAILED");
        break;
    case QC_MSG_IO_HANDSHAKE_SUCESS:
        strcpy(pName, "QC_MSG_IO_HANDSHAKE_SUCESS");
        break;
	case QC_MSG_PARSER_NEW_STREAM:
		strcpy (pName, "QC_MSG_PARSER_NEW_STREAM");
		break;

	case QC_MSG_PARSER_M3U8_ERROR:
		strcpy(pName, "QC_MSG_PARSER_M3U8_ERROR");
		break;

	case QC_MSG_PARSER_FLV_ERROR:
		strcpy(pName, "QC_MSG_PARSER_FLV_ERROR");
		break;

	case QC_MSG_PARSER_MP4_ERROR:
		strcpy(pName, "QC_MSG_PARSER_MP4_ERROR");
		break;

	case QC_MSG_SNKA_FIRST_FRAME:
		strcpy (pName, "QC_MSG_SNKA_FIRST_FRAME");
		break;
	case QC_MSG_SNKA_EOS:
		strcpy (pName, "QC_MSG_SNKA_EOS");
		break;
	case QC_MSG_SNKA_NEW_FORMAT:
		strcpy(pName, "QC_MSG_SNKA_NEW_FORMAT");
		break;
	case QC_MSG_SNKA_RENDER:
		strcpy(pName, "QC_MSG_SNKA_RENDER");
		break;

	case QC_MSG_SNKV_FIRST_FRAME:
		strcpy (pName, "QC_MSG_SNKV_FIRST_FRAME");
		break;
	case QC_MSG_SNKV_EOS:
		strcpy (pName, "QC_MSG_SNKV_EOS");
		break;
	case QC_MSG_SNKV_NEW_FORMAT:
		strcpy (pName, "QC_MSG_SNKV_NEW_FORMAT");
		break;
	case QC_MSG_SNKV_RENDER:
		strcpy(pName, "QC_MSG_SNKV_RENDER");
		break;
	case QC_MSG_SNKV_ROTATE:
		strcpy(pName, "QC_MSG_SNKV_ROTATE");
		break;

	case QC_MSG_PLAY_OPEN_DONE:
		strcpy (pName, "QC_MSG_PLAY_OPEN_DONE");
		break;
	case QC_MSG_PLAY_OPEN_FAILED:
		strcpy (pName, "QC_MSG_PLAY_OPEN_FAILED");
		break;
	case QC_MSG_PLAY_CLOSE_DONE:
		strcpy (pName, "QC_MSG_PLAY_CLOSE_DONE");
		break;
	case QC_MSG_PLAY_CLOSE_FAILED:
		strcpy (pName, "QC_MSG_PLAY_CLOSE_FAILED");
		break;
	case QC_MSG_PLAY_SEEK_DONE:
		strcpy (pName, "QC_MSG_PLAY_SEEK_DONE");
		break;
	case QC_MSG_PLAY_SEEK_FAILED:
		strcpy (pName, "QC_MSG_PLAY_SEEK_FAILED");
		break;
	case QC_MSG_PLAY_COMPLETE:
		strcpy (pName, "QC_MSG_PLAY_COMPLETE");
		break;
	case QC_MSG_PLAY_STATUS:
		strcpy (pName, "QC_MSG_PLAY_STATUS");
		break;
	case QC_MSG_PLAY_DURATION:
		strcpy(pName, "QC_MSG_PLAY_DURATION");
		break;
	case QC_MSG_PLAY_OPEN_START:
		strcpy(pName, "QC_MSG_PLAY_OPEN_START");
		break;
	case QC_MSG_PLAY_SEEK_START:
		strcpy(pName, "QC_MSG_PLAY_SEEK_START");
		break;
	case QC_MSG_PLAY_RUN:
		strcpy(pName, "QC_MSG_PLAY_RUN");
		break;
	case QC_MSG_PLAY_PAUSE:
		strcpy(pName, "QC_MSG_PLAY_PAUSE");
		break;
	case QC_MSG_PLAY_STOP:
		strcpy(pName, "QC_MSG_PLAY_STOP");
		break;

	case QC_MSG_PLAY_LOOP_TIMES:
		strcpy(pName, "QC_MSG_PLAY_LOOP_TIMES");
		break;

	case QC_MSG_BUFF_VBUFFTIME:
		strcpy (pName, "QC_MSG_BUFF_VIDEO_TIME");
		break;
	case QC_MSG_BUFF_ABUFFTIME:
		strcpy (pName, "QC_MSG_BUFF_AUDIO_TIME");
		break;
	case QC_MSG_BUFF_START_BUFFERING:
		strcpy (pName, "QC_MSG_BUFF_START_BUFFERING");
		break;
	case QC_MSG_BUFF_END_BUFFERING:
		strcpy (pName, "QC_MSG_BUFF_END_BUFFERING");
		break;
	case QC_MSG_BUFF_NEWSTREAM:
		strcpy (pName, "QC_MSG_BUFF_NEWSTREAM");
		break;
	case QC_MSG_BUFF_VFPS:
		strcpy (pName, "QC_MSG_BUFF_VFPS");
		break;
	case QC_MSG_BUFF_AFPS:
		strcpy (pName, "QC_MSG_BUFF_AFPS");
		break;
	case QC_MSG_RENDER_VIDEO_FPS:
		strcpy (pName, "QC_MSG_RENDER_VIDEO_FPS");
		break;
	case QC_MSG_RENDER_AUDIO_FPS:
		strcpy (pName, "QC_MSG_RENDER_AUDIO_FPS");
		break;
	case QC_MSG_BUFF_VBITRATE:
		strcpy(pName, "QC_MSG_BUFF_VBITRATE");
		break;
	case QC_MSG_BUFF_ABITRATE:
		strcpy(pName, "QC_MSG_BUFF_ABITRATE");
		break;
	case QC_MSG_BUFF_SEI_DATA:
		strcpy(pName, "QC_MSG_BUFF_SEI_DATA");
		break;

	case QC_MSG_LOG_TEXT:
		strcpy (pName, "QC_MSG_LOG_TEXT");
		break;
    case QC_MSG_RTMP_DOWNLOAD_SPEED:
        strcpy (pName, "QC_MSG_RTMP_DOWNLOAD_SPEED");
        break;
    case QC_MSG_RTMP_CONNECT_START:
            strcpy (pName, "QC_MSG_RTMP_CONNECT_START");
            break;
    case QC_MSG_RTMP_CONNECT_FAILED:
        strcpy (pName, "QC_MSG_RTMP_CONNECT_FAILED");
        break;
    case QC_MSG_RTMP_CONNECT_SUCESS:
        strcpy (pName, "QC_MSG_RTMP_CONNECT_SUCESS");
        break;
    case QC_MSG_RTMP_DNS_GET_IPADDR:
        strcpy (pName, "QC_MSG_RTMP_DNS_GET_IPADDR");
        break;
    case QC_MSG_RTMP_DNS_GET_CACHE:
        strcpy (pName, "QC_MSG_RTMP_DNS_GET_CACHE");
        break;
    case QC_MSG_RTMP_DISCONNECTED:
        strcpy (pName, "QC_MSG_RTMP_DISCONNECTED");
        break;
    case QC_MSG_RTMP_RECONNECT_FAILED:
        strcpy (pName, "QC_MSG_RTMP_RECONNECT_FAILED");
        break;
    case QC_MSG_RTMP_RECONNECT_SUCESS:
        strcpy (pName, "QC_MSG_RTMP_RECONNECT_SUCESS");
        break;
	
	case QC_MSG_RTMP_METADATA:
        strcpy (pName, "QC_MSG_RTMP_METADATA");
		break;

	case QC_MSG_BUFF_GOPTIME:
        strcpy (pName, "QC_MSG_BUFF_GOPTIME");
		break;

	default:
		sprintf (pName, "Unknow ID 0X%08X", nMsg);
		break;
	}
	return 0;
}


void * QCMSG_InfoClone(int nMsg, void * pInfo)
{
	void * pNewInfo = NULL;
	switch (nMsg)
	{
	case QC_MOD_IO_HTTP:
		break;

	case QC_MOD_IO_RTMP:
		break;

	case QC_MOD_PARSER_MP4:
		break;

	case QC_MOD_PARSER_FLV:
		break;

	case QC_MOD_PARSER_TS:
		break;

	case QC_MOD_PARSER_M3U8:
		break;

	case QC_MOD_AUDIO_DEC_AAC:
		break;

	case QC_MOD_VIDEO_DEC_H264:
		break;

	case QC_MOD_SINK_AUDIO:
		break;

	case QC_MOD_SINK_VIDEO:
		break;

	case QC_MOD_SINK_DATA:
		break;

	case QC_MOD_MFW_PLAY:
		break;
	case QC_MSG_PLAY_CAPTURE_IMAGE:
		return pInfo;
    case QC_MSG_BUFF_SEI_DATA:
    {
        QC_DATA_BUFF* pClone = new QC_DATA_BUFF;
        memset(pClone, 0, sizeof(QC_DATA_BUFF));
        QC_DATA_BUFF* pSEI = (QC_DATA_BUFF*)pInfo;
        if(pSEI)
        {
			pClone->llTime = pSEI->llTime;
            if(pSEI->pBuff && pSEI->uSize>0)
            {
                pClone->pBuff = new unsigned char[pSEI->uSize];
                memcpy(pClone->pBuff, pSEI->pBuff, pSEI->uSize);
                pClone->uSize = pSEI->uSize;
            }
        }
        pNewInfo = pClone;
    }
    break;
	case QC_MSG_PARSER_NEW_STREAM:
	{
		QC_RESOURCE_INFO*   pResInfo = new QC_RESOURCE_INFO;
		if (pResInfo != NULL)
		{
			memset(pResInfo, 0, sizeof(QC_RESOURCE_INFO));
            char* tmp = ((QC_RESOURCE_INFO*)pInfo)->pszFormat;
            unsigned long nLen = 0;
            if(tmp)
            {
                nLen = strlen(tmp) + 1;
                pResInfo->pszFormat = new char[nLen];
                memset(pResInfo->pszFormat, 0, nLen);
                strcpy(pResInfo->pszFormat, tmp);
            }
            //pResInfo->pszFormat = ((QC_RESOURCE_INFO*)pInfo)->pszFormat;
            
            tmp = ((QC_RESOURCE_INFO*)pInfo)->pszDomain;
            if(tmp)
            {
                nLen = strlen(tmp) + 1;
                pResInfo->pszDomain = new char[nLen];
                memset(pResInfo->pszDomain, 0, nLen);
                strcpy(pResInfo->pszDomain, tmp);
            }
			//pResInfo->pszDomain = ((QC_RESOURCE_INFO*)pInfo)->pszDomain;
            
            tmp = ((QC_RESOURCE_INFO*)pInfo)->pszURL;
            if(tmp)
            {
                nLen = strlen(tmp) + 1;
                pResInfo->pszURL = new char[nLen];
                memset(pResInfo->pszURL, 0, nLen);
                strcpy(pResInfo->pszURL, tmp);
            }
			//pResInfo->pszURL = ((QC_RESOURCE_INFO*)pInfo)->pszURL;
            
			pResInfo->llDuration = ((QC_RESOURCE_INFO*)pInfo)->llDuration;
			pResInfo->nBitrate = ((QC_RESOURCE_INFO*)pInfo)->nBitrate;
			pResInfo->nAudioCodec = ((QC_RESOURCE_INFO*)pInfo)->nAudioCodec;
			pResInfo->nVideoCodec = ((QC_RESOURCE_INFO*)pInfo)->nVideoCodec;
			pResInfo->nHeight = ((QC_RESOURCE_INFO*)pInfo)->nHeight;
			pResInfo->nWidth = ((QC_RESOURCE_INFO*)pInfo)->nWidth;
		}

		pNewInfo = (void*)pResInfo;
		break;
	}
	default:
		break;
	}

	return pNewInfo;
}	

int	QCMSG_InfoRelase(int nMsg, void * pInfo)
{
	switch (nMsg)
	{
	case QC_MOD_IO_HTTP:
		break;

	case QC_MOD_IO_RTMP:
		break;

	case QC_MOD_PARSER_MP4:
		break;

	case QC_MOD_PARSER_FLV:
		break;

	case QC_MOD_PARSER_TS:
		break;

	case QC_MOD_PARSER_M3U8:
		break;

	case QC_MOD_AUDIO_DEC_AAC:
		break;

	case QC_MOD_VIDEO_DEC_H264:
		break;

	case QC_MOD_SINK_AUDIO:
		break;

	case QC_MOD_SINK_VIDEO:
		break;

	case QC_MOD_SINK_DATA:
		break;

	case QC_MOD_MFW_PLAY:
		break;
	case QC_MSG_PARSER_NEW_STREAM:
	{
		QC_RESOURCE_INFO*   pResInfo = (QC_RESOURCE_INFO*)pInfo;
		if (pResInfo != NULL)
		{
            QC_DEL_A(pResInfo->pszFormat);
            QC_DEL_A(pResInfo->pszURL);
            QC_DEL_A(pResInfo->pszDomain);
			QC_DEL_P(pResInfo);
		}
		break;
	}
    case QC_MSG_BUFF_SEI_DATA:
    {
        QC_DATA_BUFF* pSEI = (QC_DATA_BUFF*)pInfo;
        if(pSEI)
        {
            pSEI->uSize = 0;
            QC_DEL_A(pSEI->pBuff);
            QC_DEL_P(pSEI);
        }
    }
    break;
	default:
		break;
	}
	return QC_ERR_NONE;
}
