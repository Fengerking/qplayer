/*******************************************************************************
	File:		CNDKPlayer.cpp

	Contains:	qpalyer NDK player implement file.

	Written by:	Fenger King

	Change History (most recent first):
	2017-01-15		Fenger			Create file

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include "qcPlayer.h"
#include "CNDKPlayer.h"
#include "COMBoxMng.h"
#include "CMsgMng.h"

#include "USystemFunc.h"
#include "UIntReader.h"
#include "ULogFunc.h"

#define	PARAM_PID_EVENT_DONE		0X100
#define	QC_FLAG_Video_CaptureImage	0x00000010
#define	QC_FLAG_Video_SEIDATA		0x00000020

CNDKPlayer::CNDKPlayer(void)
	: CBaseObject (NULL)
	, m_pjVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPostEvent (NULL)
	, m_fPushVideo (NULL)	
	, m_fPushSubTT (NULL)
	, m_nVersion (0)
	, m_bEventDone (true)
	, m_pView (NULL)	
	, m_nPlayFlag (0)
	, m_pRndAudio (NULL)
	, m_pRndVideo (NULL)
	, m_pRndVidDec (NULL)
	, m_nLatency (-1)
	, m_llSeekPos (0)
	, m_nStartTime (0)
	, m_pEnv (NULL)
	, m_pDataBuff (NULL)
	, m_nDataSize (0)	
	, m_nMsgThread (0)
{
	SetObjectName ("CNDKPlayer");	
	memset (&m_Player, 0, sizeof (QCM_Player));

	char szProp[64];
	memset (szProp, 0, 64);
	__system_property_get ("ro.build.version.release", szProp);
	QCLOGI ("The device propertity is %s", szProp);
	if (szProp[1] == '.')
		szProp[2] = 0;
	else
		szProp[1] = 0;
	gqc_android_devces_ver = m_nVersion = atol (szProp);

	memset(&m_buffAudio, 0, sizeof(m_buffAudio));
	m_buffAudio.nMediaType = QC_MEDIA_Audio;
	m_buffAudio.uBuffType = QC_BUFF_TYPE_Data;
	memset(&m_buffVideo, 0, sizeof(m_buffVideo));
	m_buffVideo.nMediaType = QC_MEDIA_Video;
	m_buffVideo.uBuffType = QC_BUFF_TYPE_Data;
	memset(&m_buffSource, 0, sizeof(m_buffSource));
	m_buffSource.nMediaType = QC_MEDIA_Data;
	m_buffSource.uBuffType = QC_BUFF_TYPE_Data;
}

CNDKPlayer::~CNDKPlayer ()
{
	QC_DEL_A (m_buffAudio.pBuff);
	QC_DEL_A (m_buffVideo.pBuff);
}

int CNDKPlayer::Init (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer, char * pPath, int nFlag)
{
	Uninit (env);

	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;
	
	if (m_pjCls != NULL && m_pjObj != NULL)
	{
		m_fPostEvent = env->GetStaticMethodID (m_pjCls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
		m_fPushVideo = env->GetStaticMethodID (m_pjCls, "videoDataFromNative", "(Ljava/lang/Object;[BIJI)V");
		QCLOGI ("Post event method = %p", m_fPostEvent);
	}

	int nRC = qcCreatePlayer (&m_Player, env);
	if (nRC < 0)
	{	
		QCLOGE ("Create failed %08X", nRC);
		return nRC;
	}
	m_pBaseInst = ((COMBoxMng *)m_Player.hPlayer)->GetBaseInst();

	if ((nFlag & 0X80000000) != 0)
	{
		m_pRndAudio = new COpenSLESRnd (m_pBaseInst, NULL);
	}
	else
	{
//		m_pRndAudio = new COpenSLESRnd (m_pBaseInst, NULL);
//		m_pRndAudio = new CNDKAudioRnd (m_pBaseInst, NULL);
		m_pRndAudio = new CAudioTrack (m_pBaseInst, NULL);
	}
	((CAudioTrack *)m_pRndAudio)->SetNDK (jvm, env, clsPlayer, objPlayer);
	m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_AudioRnd, m_pRndAudio);
		
	m_nPlayFlag = nFlag & 0X0FFFFFFF;
	if ((m_nPlayFlag & QCPLAY_OPEN_VIDDEC_HW) == QCPLAY_OPEN_VIDDEC_HW)
	{
		m_pRndVidDec = new CNDKVDecRnd (m_pBaseInst, NULL);
		m_pRndVidDec->SetNDK (jvm, env, clsPlayer, objPlayer, m_nVersion);	
		m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_VideoRnd, m_pRndVidDec);	
		if (m_pView != NULL)
			m_pRndVidDec->SetSurface (env, m_pView);
	}
	else
	{
		m_pRndVideo = new CNDKVideoRnd (m_pBaseInst, NULL);
		m_pRndVideo->SetNDK (jvm, env, clsPlayer, objPlayer);		
		m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_VideoRnd, m_pRndVideo);	
		if (m_pView != NULL)
			m_pRndVideo->SetSurface (env, m_pView);					
	}
	// version < 5 was not support HEVC decoder with media codec
	if (m_nVersion < 5 && m_pRndVideo == NULL)
	{
		m_pRndVideo = new CNDKVideoRnd (m_pBaseInst, NULL);
		m_pRndVideo->SetNDK (jvm, env, clsPlayer, objPlayer);		
	}	
		
	m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_SetWorkPath, pPath);
	m_Player.SetNotify (m_Player.hPlayer, NotifyEvent, this);	
			
	if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
		m_pBaseInst->m_pMsgMng->RegNotify(this);

	return QC_ERR_NONE;	
}

int CNDKPlayer::Uninit (JNIEnv* env)
{	
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;
	m_Player.SetParam(m_Player.hPlayer, QCPLAY_PID_DEL_Cache, NULL);
	int nTryTimes = 0;
	int nRC = m_Player.Close (m_Player.hPlayer);
	while (nRC != QC_ERR_NONE)
	{
		qcSleep (100000);
		nRC = m_Player.Close (m_Player.hPlayer);
		nTryTimes++;
		if (nTryTimes > 30)
		{
			QCLOGW ("Close Player Failed!");
			break;
		}
	}
	if (m_pRndVideo != NULL)
		m_pRndVideo->ReleaseRnd ();

	//QCMSG_RemNotify (m_pBaseInst, this);
	if (m_Player.hPlayer != NULL)
	{
		qcDestroyPlayer (&m_Player);
		memset (&m_Player, 0, sizeof (QCM_Player));
	}

	if (nRC == QC_ERR_NONE)
	{
		QC_DEL_P (m_pRndAudio);
		QC_DEL_P (m_pRndVideo);
		QC_DEL_P (m_pRndVidDec);
	}	
	
	if (m_pjObj != NULL)
    	env->DeleteGlobalRef(m_pjObj);
    m_pjObj = NULL;
	if (m_pjCls != NULL)
    	env->DeleteGlobalRef(m_pjCls);
    m_pjCls = NULL;  	

	return QC_ERR_NONE;
}

int	CNDKPlayer::SetView (JNIEnv* env, jobject pView)
{
	if (m_pView == pView)
		return QC_ERR_FAILED;
		
	m_pView = pView;

	if (m_pRndVideo != NULL)
		m_pRndVideo->SetSurface (env, m_pView);
	if (m_pRndVidDec != NULL)
		m_pRndVidDec->SetSurface (env, m_pView);

	return QC_ERR_NONE;
}

int CNDKPlayer::Open (const char* szUrl, int nFlag)
{
	m_nStartTime = qcGetSysTime ();
	m_nIndex = 0;
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;

	nFlag |= m_nPlayFlag;

	int nRC = m_Player.Open (m_Player.hPlayer, szUrl, nFlag);
	return nRC;
}

int CNDKPlayer::Play (void)
{
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;
	return m_Player.Run (m_Player.hPlayer);
}

int CNDKPlayer::Pause (void)
{
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;
	return m_Player.Pause (m_Player.hPlayer);
}

int CNDKPlayer::Stop (void)
{
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;
	return m_Player.Stop (m_Player.hPlayer);
}

long long CNDKPlayer::GetDuration ()
{
	if (m_Player.hPlayer == NULL)
		return 0;		
	return m_Player.GetDur (m_Player.hPlayer);
}

int CNDKPlayer::GetPos (long long * pCurPos)
{
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;	
	*pCurPos = m_Player.GetPos (m_Player.hPlayer);
	return QC_ERR_NONE;
}

int CNDKPlayer::SetPos (long long llCurPos)
{
	if (m_Player.hPlayer == NULL)
		return QC_ERR_STATUS;	
	m_llSeekPos = llCurPos;
	int nRC = m_Player.SetPos (m_Player.hPlayer, llCurPos);
	return nRC;
}

int CNDKPlayer::WaitRendTime (long long llTime)
{
	if (llTime < m_llSeekPos)
		return -1;
	int nRC = 1;
	if (m_pRndVidDec != NULL)
		nRC = m_pRndVidDec->WaitRendTime (llTime);
	return nRC;
}

int CNDKPlayer::GetParam (JNIEnv* env, int nID, int nParam, jobject pValue)
{	
	int		nValue = -1;
	jclass 	objClass = NULL;
	if (pValue != NULL)
		objClass = env->GetObjectClass(pValue);
		//jfieldID intID = env->GetFieldID(objClass,"nValue","I");
		//jint nValue = (int)env->GetIntField(obj,intID);
		//jfieldID strID = env->GetFieldID(objClass,"strValue","Ljava/lang/String;");
		//jstring jstr = (jstring)env->GetObjectField(obj,strID);
		//const char *pszStr = env->GetStringUTFChars(jstr,NULL);
		//env->ReleaseStringUTFChars(jstr,pszStr);
		//jstring str = env->NewStringUTF("qweABC123xxXXXX");
		//env->SetObjectField(obj,strID,str);
		//env->SetIntField(obj,intID,222);
	if (nID == QCPLAY_PID_StreamNum)
	{
		if (m_Player.hPlayer != NULL)
			m_Player.GetParam (m_Player.hPlayer, nID, &nValue);
		if (objClass != NULL)
		{
			jfieldID 	intID = env->GetFieldID(objClass,"m_nStreamNum", "I");
			env->SetIntField (pValue, intID, nValue);
		}
		return nValue;
	}
	else if (nID == QCPLAY_PID_StreamPlay)
	{
		int	nStreamPlay = -1;
		if (m_Player.hPlayer != NULL)
			m_Player.GetParam (m_Player.hPlayer, nID, &nValue);
		if (objClass != NULL)
		{
			jfieldID 	intID = env->GetFieldID(objClass,"m_nStreamPlay", "I");
			env->SetIntField (pValue, intID, nValue);
		}
		return nValue;
	}
	else if (nID == QCPLAY_PID_StreamInfo)
	{
		QC_STREAM_FORMAT stmInfo;
		memset (&stmInfo, 0, sizeof (stmInfo));
		stmInfo.nID = nParam;
		if (m_Player.hPlayer != NULL)
			m_Player.GetParam (m_Player.hPlayer, nID, &stmInfo);
		if (objClass != NULL)
		{
			jfieldID 	intID = env->GetFieldID(objClass,"m_nStreamBitrate", "I");
			env->SetIntField (pValue, intID, stmInfo.nBitrate);
			QCLOGI ("Bitrate = %d", stmInfo.nBitrate);
		}
		return stmInfo.nBitrate;
	}		

	long long 	llValue = 0;
	switch (nID)
	{
		case QCPLAY_PID_AudioTrackNum:
		case QCPLAY_PID_AudioTrackPlay:
			nValue = 0;
			if (m_Player.hPlayer != NULL)
				m_Player.GetParam (m_Player.hPlayer, nID, &nValue);
			return nValue;

		case PARAM_PID_AUDIO_VOLUME:
//			if (m_pRndAudio != NULL)
//				return m_pRndAudio->GetVolume ();	
			if (m_Player.hPlayer != NULL)
				return m_Player.GetVolume(m_Player.hPlayer);
			return 100;

		case QCPLAY_PID_Download_Pause:
			if (m_Player.hPlayer != NULL)
				return m_Player.GetParam (m_Player.hPlayer, nID, NULL);
			return 0;

		case QCPLAY_PID_RTMP_AUDIO_MSG_TIMESTAMP:
		case QCPLAY_PID_RTMP_VIDEO_MSG_TIMESTAMP:		
			if (m_Player.hPlayer != NULL)
				m_Player.GetParam (m_Player.hPlayer, nID, &llValue);
			return (int)llValue;

		case PARAM_PID_QPLAYER_VERSION:
			QCLOGI ("The qplayer version is %X", m_Player.nVersion);
			return m_Player.nVersion;

		default:
			break;
	}
	return QC_ERR_NONE;
}

int CNDKPlayer::SetParam (JNIEnv* env, int nID, int nParam, jobject pValue)
{		
	switch (nID)
	{
	case PARAM_PID_EVENT_DONE:
	{
		if (m_pRndVideo != NULL)
			m_pRndVideo->SetEventDone (true);
		if (m_pRndVidDec != NULL)
			m_pRndVidDec->SetEventDone (true);
		QCLOGI ("Event had handled! m_bEventDone = %d", m_bEventDone);
		return QC_ERR_NONE;
	}	

	case QCPLAY_PID_Zoom_Video:
	{
		jintArray zmVideo = (jintArray)pValue;
   		jsize size = env->GetArrayLength (zmVideo);
	    jint * intArray = env->GetIntArrayElements(zmVideo, JNI_FALSE);

		RECT rcZoom;
		rcZoom.left = *(intArray + 0);
 		rcZoom.top = *(intArray + 1);
		rcZoom.right = *(intArray + 2);
		rcZoom.bottom = *(intArray + 3);				    
    	env->ReleaseIntArrayElements(zmVideo, intArray, 0);

		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, &rcZoom);
		return QC_ERR_NONE;
	}	
	case PARAM_PID_AUDIO_VOLUME:
	{
//		if (m_pRndAudio != NULL)
//			m_pRndAudio->SetVolume (nParam);
		if (m_Player.hPlayer != NULL)
			m_Player.SetVolume(m_Player.hPlayer, nParam);
		return QC_ERR_NONE;
	}	
	case QCPLAY_PID_Clock_OffTime:
	{
		int nOffset = 0;
		if (m_nLatency < 0)
			m_nLatency = GetOutputLatency ();
		if (nParam & 0X80000000)
		{
			nParam = nParam & 0X7FFFFFFF;			
			if ((nParam + m_nLatency) < 200)
				nOffset = 220;
		}			
		QCLOGI ("The offset time Java %d, Output: %d, Offset: %d, Total: %d", nParam, m_nLatency, nOffset,  nParam + m_nLatency + nOffset);			
		nParam = nParam + m_nLatency + nOffset;
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_Clock_OffTime, &nParam);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_Capture_Image:
	{
		long long llTime = nParam;
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, &llTime);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_PD_Save_Path:
	case QCPLAY_PID_PD_Save_ExtName:
	{
		jstring strPath = (jstring)pValue;
		char * 	pPath = (char *) env->GetStringUTFChars(strPath, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pPath);
		return QC_ERR_NONE;	
	}
	case QCPLAY_PID_HTTP_HeadReferer:
	{
		jstring strHead = (jstring)pValue;
		char * 	pHeadText = (char *) env->GetStringUTFChars(strHead, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pHeadText);
		return QC_ERR_NONE;
	}
    case QCPLAY_PID_HTTP_HeadUserAgent:
    {
        jstring strHead = (jstring)pValue;
        char *     pHeadText = (char *) env->GetStringUTFChars(strHead, NULL);
        if (m_Player.hPlayer != NULL)
            m_Player.SetParam (m_Player.hPlayer, nID, pHeadText);
        return QC_ERR_NONE;
    }
	case QCPLAY_PID_DNS_SERVER:
	{
		jstring strDNSServer = (jstring)pValue;
		char * 	pDNSText = (char *) env->GetStringUTFChars(strDNSServer, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pDNSText);
		return QC_ERR_NONE;		
	}
	case QCPLAY_PID_DNS_DETECT:
	{
		jstring strDNSHost = (jstring)pValue;
		char * 	pDNSHost = (char *) env->GetStringUTFChars(strDNSHost, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pDNSHost);
		return QC_ERR_NONE;		
	}	
	case QCPLAY_PID_ADD_Cache:
	case QCPLAY_PID_ADD_IOCache:
	{
		jstring strSource = (jstring)pValue;
		char * 	pSource = (char *)env->GetStringUTFChars(strSource, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam(m_Player.hPlayer, nID, pSource);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_DEL_Cache:
	case QCPLAY_PID_DEL_IOCache:
	{
		jstring strSource = NULL;
		char * 	pSource = NULL;
		if (pValue != NULL)
		{
			strSource = (jstring)pValue;
			pSource = (char *)env->GetStringUTFChars(strSource, NULL);
		}
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam(m_Player.hPlayer, nID, pSource);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_DRM_KeyText:
	{
		if (nParam == 0)
		{
			jbyteArray 	byKey = (jbyteArray)pValue;
			jbyte* 		pData = env->GetByteArrayElements(byKey, NULL);	
			if (m_Player.hPlayer != NULL)
				m_Player.SetParam (m_Player.hPlayer, nID, pData);
			env->ReleaseByteArrayElements(byKey, pData, 0);					
		}
		else
		{
			jstring strKey = (jstring)pValue;
			char * 	pKeyText = (char *) env->GetStringUTFChars(strKey, NULL);
			if (m_Player.hPlayer != NULL)
				m_Player.SetParam (m_Player.hPlayer, nID, pKeyText);		
		}
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_FILE_KeyText:
	case QCPLAY_PID_COMP_KeyText:
	{
		jstring strKey = (jstring)pValue;
		char * 	pKeyText = (char *) env->GetStringUTFChars(strKey, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pKeyText);		
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_START_MUX_FILE:
	{
		jstring strMuxFile = (jstring)pValue;
		char * 	pFileText = (char *) env->GetStringUTFChars(strMuxFile, NULL);
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, pFileText);		
		return QC_ERR_NONE;
	}	
	case QCPLAY_PID_Speed:
	{
		int nFZ = (nParam >> 16) & 0XFFFF;
		int nFM = nParam & 0XFFFF;
		double fSpeed = (double)nFZ / nFM;
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, &fSpeed);
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_SendOut_AudioBuff:
	{
		if (m_pRndAudio != NULL)
		{
			m_pRndAudio->SetParam (nID, &nParam);
			((COpenSLESRnd *)m_pRndAudio)->SetNDK (m_pjVM, env, m_pjCls, m_pjObj);
		}
		return QC_ERR_NONE;
	}
	case QCPLAY_PID_SendOut_VideoBuff:
	{
		if (m_pRndVideo != NULL)
		{
			m_pRndVideo->SetParam (env, nID, &nParam);
		}
		return QC_ERR_NONE;
	}

	case QCPLAY_PID_Seek_Mode:
	case QCPLAY_PID_StreamPlay:	
	case QCPLAY_PID_AudioTrackPlay:
	case QCPLAY_PID_Socket_ConnectTimeout:
	case QCPLAY_PID_Socket_ReadTimeout:
	case QCPLAY_PID_PlayBuff_MaxTime:
	case QCPLAY_PID_PlayBuff_MinTime:
	case QCPLAY_PID_Log_Level:
	case QCPLAY_PID_Disable_Video:
	case QCPLAY_PID_Prefer_Protocol:
	case QCPLAY_PID_Prefer_Format:
	case QCPLAY_PID_Flush_Buffer:
	case QCPLAY_PID_Playback_Loop:
	case QCPLAY_PID_Download_Pause:
	case QCPLAY_PID_IOCache_Size:
	case QCPLAY_PID_EXT_VIDEO_CODEC:
	case QCPLAY_PID_EXT_AUDIO_CODEC:
	case QCPLAY_PID_STOP_MUX_FILE:
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, &nParam);
		break;

	case QCPLAY_PID_START_POS:
	{
		long long llStartPos = nParam;
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam (m_Player.hPlayer, nID, &llStartPos);
		break;		
	}

	case QC_FLAG_NETWORK_CHANGED:
		//if (m_pBaseInst != NULL)
		//	m_pBaseInst->NotifyNetChanged ();
		if (m_Player.hPlayer != NULL)
			m_Player.SetParam(m_Player.hPlayer, QCPLAY_PID_NET_CHANGED, NULL);
		QCLOGI("Network QOS Changed!");
		break;
			
	case QC_FLAG_NETWORK_TYPE:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strType = (jstring)pValue;
		char * 	pTypeText = (char *) env->GetStringUTFChars(strType, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szNetType, pTypeText);
		QCLOGI ("Network QOS Type is %s", pTypeText);
	}		
		break;
			
	case QC_FLAG_ISP_NAME:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strISP = (jstring)pValue;
		char * 	pISPText = (char *) env->GetStringUTFChars(strISP, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szISPName, pISPText);
		QCLOGI ("Network QOS ISP Name is %s", pISPText);		
	}		
		break;
			
	case QC_FLAG_WIFI_NAME:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strWifiName = (jstring)pValue;
		char * 	pWifiText = (char *) env->GetStringUTFChars(strWifiName, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szWifiName, pWifiText);
		QCLOGI ("Network QOS Wifi Name is %s", pWifiText);		
	}
		break;
			
	case QC_FLAG_SIGNAL_DB:
		if (m_pBaseInst != NULL)	
			m_pBaseInst->m_nNetSignalDB = nParam;
		QCLOGI ("Network QOS Signal DB is %d", nParam);			
		break;
			
	case QC_FLAG_SIGNAL_LEVEL:
		if (m_pBaseInst != NULL)	
			m_pBaseInst->m_nSignalLevel = nParam;
		QCLOGI ("Network QOS Signal Level is %d", nParam);					
		break;

	case QC_FLAG_APP_VERSION:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strVer = (jstring)pValue;
		char * 	pVerText = (char *) env->GetStringUTFChars(strVer, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szAppVer, pVerText);
		QCLOGI ("Network QOS app version is %s", pVerText);		
	}		
		break;		

	case QC_FLAG_APP_NAME:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strName = (jstring)pValue;
		char * 	pNameText = (char *) env->GetStringUTFChars(strName, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szAppName, pNameText);
		QCLOGI ("Network QOS app name is %s", pNameText);		
	}		
		break;	

	case QC_FLAG_SDK_ID:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strID = (jstring)pValue;
		char * 	pIDText = (char *) env->GetStringUTFChars(strID, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szSDK_ID, pIDText);
		QCLOGI ("Network QOS sdk ID is %s", pIDText);		
	}		
		break;	

	case QC_FLAG_SDK_VERSION:
	{
		if (pValue == NULL)
			return QC_ERR_ARG;
		jstring strVer = (jstring)pValue;
		char * 	pVerText = (char *) env->GetStringUTFChars(strVer, NULL);
		if (m_pBaseInst != NULL)
			strcpy (m_pBaseInst->m_szSDK_Ver, pVerText);
		QCLOGI ("Network QOS SDK version is %s", pVerText);		
	}		
		break;	

	case QCPLAY_PID_EXT_SOURCE_INFO:
	{
		if (m_Player.hPlayer == NULL)
		{
			qcSleep (5000);
			return QC_ERR_RETRY;
		}
		jbyteArray 	byInfo = (jbyteArray)pValue;
		jbyte* 		pData = env->GetByteArrayElements(byInfo, NULL);	
		if (nParam == 0) // Source IO
		{
			m_buffSource.uSize = qcIntReadUint32BE ((const unsigned char *)pData);
			m_buffSource.llTime = qcIntReadUint64BE ((const unsigned char *)pData + 4);
			m_buffSource.uFlag = qcIntReadUint32BE ((const unsigned char *)pData + 12);
			//QCLOGI ("EXT_Source: Size = % 8d, Time: % 8lld, Flag = % 8d", m_buffVideo.uSize, m_buffVideo.llTime, m_buffVideo.uFlag);
		} 
		else if (nParam == 1) // Audio
		{
			m_buffAudio.uSize = qcIntReadUint32BE ((const unsigned char *)pData);
			m_buffAudio.llTime = qcIntReadUint64BE ((const unsigned char *)pData + 4);
			m_buffAudio.uFlag = qcIntReadUint32BE ((const unsigned char *)pData + 12);
		}
		else if (nParam == 2) // Video
		{
			m_buffVideo.uSize = qcIntReadUint32BE ((const unsigned char *)pData);
			m_buffVideo.llTime = qcIntReadUint64BE ((const unsigned char *)pData + 4);
			m_buffVideo.uFlag = qcIntReadUint32BE ((const unsigned char *)pData + 12);
			//QCLOGI ("EXT_Source: Size = % 8d, Time: % 8lld, Flag = % 8d", m_buffVideo.uSize, m_buffVideo.llTime, m_buffVideo.uFlag);
		} 
		env->ReleaseByteArrayElements(byInfo, pData, 0);	
		return QC_ERR_NONE;
	}

	case QCPLAY_PID_EXT_SOURCE_DATA:
	{
		if (m_Player.hPlayer == NULL)
		{
			qcSleep (5000);
			return QC_ERR_RETRY;
		}
		int			nRC = QC_ERR_NONE;
		if (m_pRndVideo != NULL)
			m_pRndVideo->SetParam (env, nID, pValue);
		jbyteArray 	byData = (jbyteArray)pValue;
		jbyte* 		pData = env->GetByteArrayElements(byData, NULL);	
		if (nParam == 0) // IO Source data
		{
			if (m_buffSource.uBuffSize < m_buffSource.uSize)
			{
				QC_DEL_A (m_buffSource.pBuff);
				m_buffSource.uBuffSize = m_buffSource.uSize + 1024;
				if (m_buffSource.uBuffSize < 192000)
					m_buffSource.uBuffSize = 192000;
				m_buffSource.pBuff = new unsigned char[m_buffSource.uBuffSize];
			}
			memcpy (m_buffSource.pBuff, pData, m_buffSource.uSize);
			nRC = m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_SOURCE_DATA, &m_buffSource);
		}	
		else if (nParam == 1) // Audio
		{
			if (m_buffAudio.uBuffSize < m_buffAudio.uSize)
			{
				QC_DEL_A (m_buffAudio.pBuff);
				m_buffAudio.uBuffSize = m_buffAudio.uSize + 1024;
				if (m_buffAudio.uBuffSize < 192000)
					m_buffAudio.uBuffSize = 192000;
				m_buffAudio.pBuff = new unsigned char[m_buffAudio.uBuffSize];
			}
			memcpy (m_buffAudio.pBuff, pData, m_buffAudio.uSize);
			nRC = m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_SOURCE_DATA, &m_buffAudio);
			//QCLOGI ("EXT_Source Audio Data: Size = % 8d, Time: % 8lld, Flag = % 8d", m_buffAudio.uSize, m_buffAudio.llTime, m_buffAudio.uFlag);
		}
		else if (nParam == 2) // Video
		{
			if (m_buffVideo.uBuffSize < m_buffVideo.uSize)
			{
				QC_DEL_A (m_buffVideo.pBuff);
				m_buffVideo.uBuffSize = m_buffVideo.uSize + 1024;
				if (m_buffVideo.uBuffSize < 2048 * 1024)
					m_buffVideo.uBuffSize = 2048 * 1024;
				m_buffVideo.pBuff = new unsigned char[m_buffVideo.uBuffSize];
			}
			memcpy (m_buffVideo.pBuff, pData, m_buffVideo.uSize);
			nRC = m_Player.SetParam (m_Player.hPlayer, QCPLAY_PID_EXT_SOURCE_DATA, &m_buffVideo);
			//QCLOGI ("EXT_Source Video Data: Size = % 8d, Time: % 8lld, Flag = % 8d", m_buffVideo.uSize, m_buffVideo.llTime, m_buffVideo.uFlag);
		} 
		env->ReleaseByteArrayElements(byData, pData, 0);		
		if (nRC != QC_ERR_NONE)
			qcSleep (2000);
		return nRC;
	}

	default:
		break;
	}

	return QC_ERR_ARG;
}

void CNDKPlayer::NotifyEvent (void * pUserData, int nID, void * pV1)
{
	CNDKPlayer * pPlayer = (CNDKPlayer *)pUserData;
	pPlayer->HandleEvent (nID, pV1);	
}

void CNDKPlayer::HandleEvent (int nID, void * pV1)
{
	if (m_fPostEvent == NULL)
		return;
	if (nID == QC_MSG_BUFF_SEI_DATA || nID == QC_MSG_PLAY_CAPTURE_IMAGE)
		return;	
			
	CAutoLock lock (&m_mtNofity);
	JNIEnv * env = NULL;
	if (qcThreadGetCurrentID () == m_nMsgThread)
		env = m_pEnv;
	else
		m_pjVM->AttachCurrentThread (&env, NULL);	
	
	if (nID == QC_MSG_VIDEO_HWDEC_FAILED)
	{
		env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, 0, 0, NULL);	
	}
	else if (nID == QC_MSG_SNKV_NEW_FORMAT)
	{
		// pass
	}
	else if (nID == QC_MSG_SNKA_NEW_FORMAT)
	{
		// pass for jave audio render
		/*
	    QC_AUDIO_FORMAT* pFmt = (QC_AUDIO_FORMAT *)pV1;
		if (pFmt != NULL)
			env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, pFmt->nSampleRate, pFmt->nChannels, NULL);	
		*/	
	}	
	else if (nID == QC_MSG_BUFF_VBUFFTIME || nID == QC_MSG_BUFF_ABUFFTIME ||
			 nID == QC_MSG_SNKA_RENDER || nID == QC_MSG_SNKV_RENDER || 
			 nID == QC_MSG_HTTP_BUFFER_SIZE || nID == QC_MSG_HTTP_CONTENT_SIZE)
	{
		long long llTime = *(long long *)pV1;
		int nBuffTime = (int)llTime;
		int nHighTime = (int)(llTime >> 32);
		env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, nBuffTime, nHighTime, NULL);					
	}
	else if (nID == QC_MSG_RTMP_METADATA)
	{
		if (pV1 != NULL)
		{
			char * 	pMetaData = (char*)pV1;
			int		nLen = strlen (pMetaData);
			QCLOGI ("MetaData = %s,   Size = %d", pMetaData, nLen);			
			for (int i = 0; i < nLen; i++)
			{
				if (pMetaData[i] < 0)
					pMetaData[i] = 32;
			}
			jstring strMetaData = env->NewStringUTF(pMetaData);
			env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, 0, 0, strMetaData);		
		}
	}	
	else if (nID == QC_MSG_PLAY_CACHE_DONE || nID == QC_MSG_PLAY_CACHE_FAILED)
	{
		if (nID == QC_MSG_PLAY_CACHE_DONE) {
			QCLOGI ("Add cache %s done!", (char*)pV1);
		} else {
			QCLOGI ("Add cache %s failed!", (char*)pV1);
		}
		jstring strMetaData = env->NewStringUTF((char *)pV1);
		env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, 0, 0, strMetaData);
	}
    else if (nID == QC_MSG_HTTP_DNS_GET_CACHE || nID == QC_MSG_HTTP_DNS_GET_IPADDR)
    {
        QC_DATA_BUFF* pInfo = (QC_DATA_BUFF*)pV1;
        QCLOGI ("DNS resolved, %s", (char*)pInfo->pBuffPtr);
        jstring strVal = env->NewStringUTF((char*)pInfo->pBuffPtr);
        env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, (int)pInfo->llTime, 0, strVal);
    }
    else if (nID == QC_MSG_HTTP_CONTENT_TYPE)
    {
       	QCLOGI ("Content type, %s", (char*)pV1);
        jstring strVal = env->NewStringUTF((char *)pV1);
        env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, 0, 0, strVal);
    }
	else
	{
		if (pV1 == NULL)
			env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, 0, 0, NULL);	
		else
			env->CallStaticVoidMethod(m_pjCls, m_fPostEvent, m_pjObj, nID, *(int*)pV1, 0, NULL);						
	}

	if (env != m_pEnv)
		m_pjVM->DetachCurrentThread ();	
}

int	CNDKPlayer::ReceiveMsg (CMsgItem * pItem)
{
	if (g_nLogOutLevel < QC_LOG_LEVEL_INFO)
		return 0;	

	CAutoLock lock (&m_mtNofity);
	if (pItem->m_nMsgID == QC_MSG_BUFF_SEI_DATA || pItem->m_nMsgID == QC_MSG_PLAY_CAPTURE_IMAGE)
	{
		QC_DATA_BUFF * 	pImgBuff = (QC_DATA_BUFF*)pItem->m_pInfo;
		//QCLOGI ("SEI Buff = %p, size = % 8d, Data = %d", pImgBuff->pBuff, pImgBuff->uSize, pImgBuff->pBuff[0]);
		bool bFirstTime = false;
		if (m_pEnv == NULL)
		{
			m_nMsgThread = qcThreadGetCurrentID ();			
			m_pjVM->AttachCurrentThread (&m_pEnv, NULL);
			bFirstTime = true;
		}

		if (pImgBuff->uSize > m_nDataSize)
		{
			if (m_pDataBuff != NULL)
				m_pEnv->DeleteLocalRef(m_pDataBuff);
			m_pDataBuff = NULL;
		}
		if (m_pDataBuff == NULL)
		{
			m_nDataSize = pImgBuff->uSize + 1024;
			m_pDataBuff = m_pEnv->NewByteArray(m_nDataSize);	
		}

		if (bFirstTime && pItem->m_nMsgID == QC_MSG_BUFF_SEI_DATA)
		{
			jbyte* 	pData = m_pEnv->GetByteArrayElements (m_pDataBuff, NULL);
			memcpy (pData, pImgBuff->pBuff, pImgBuff->uSize); 
			m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushVideo, m_pjObj, m_pDataBuff, 0, pImgBuff->llTime, QC_FLAG_Video_SEIDATA);
			m_pEnv->ReleaseByteArrayElements(m_pDataBuff, pData, 0);
		}

		jbyte* 	pData = m_pEnv->GetByteArrayElements (m_pDataBuff, NULL);
		memcpy (pData, pImgBuff->pBuff, pImgBuff->uSize); 
		if (pItem->m_nMsgID == QC_MSG_BUFF_SEI_DATA)
			m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushVideo, m_pjObj, m_pDataBuff, pImgBuff->uSize, pImgBuff->llTime, QC_FLAG_Video_SEIDATA);
		else
			m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushVideo, m_pjObj, m_pDataBuff, pImgBuff->uSize, pImgBuff->llTime, QC_FLAG_Video_CaptureImage);
		m_pEnv->ReleaseByteArrayElements(m_pDataBuff, pData, 0);

		return 0;
	}
	else if (pItem->m_nMsgID == QC_MSG_THREAD_EXIT)
	{
		QCLOGI ("SEI Msg Thread exit in NDKPlayer.");
		if (m_pEnv != NULL)
		{
			if (m_pDataBuff != NULL)
			{
				m_pEnv->DeleteLocalRef(m_pDataBuff);
				m_pDataBuff = NULL;		
				m_nDataSize = 0;	
			}
			m_pjVM->DetachCurrentThread ();		
			m_pEnv = NULL;
			m_nMsgThread = 0;
		}
		return 0;
	}
		
	if (pItem->m_nMsgID == QC_MSG_SNKA_RENDER || pItem->m_nMsgID == QC_MSG_SNKV_RENDER)
		return 0;

	int		nStartPos = 0;
	int		tm = (pItem->m_nTime - m_nStartTime) / 1000;

	memset (m_szDebugLine, ' ', 1023);
	m_szDebugLine[1023] = 0;

	sprintf (m_szDebugItem, "QCMSG% 6d  ",	m_nIndex++);
	memcpy (m_szDebugLine + nStartPos, m_szDebugItem, strlen (m_szDebugItem));
	nStartPos += 10;

	memcpy (m_szDebugLine + nStartPos, pItem->m_szIDName, strlen (pItem->m_szIDName));
	nStartPos += 32;

	sprintf (m_szDebugItem, "%02d : %02d : %02d : %03d", tm/3600, (tm%3600)/60, tm%60, (pItem->m_nTime - m_nStartTime)%1000);
	memcpy (m_szDebugLine + nStartPos, m_szDebugItem, strlen (m_szDebugItem));
	nStartPos += 20;

	sprintf (m_szDebugItem, "% 10d", pItem->m_nValue);
	memcpy (m_szDebugLine + nStartPos, m_szDebugItem, strlen (m_szDebugItem));
	nStartPos += 12;

	sprintf (m_szDebugItem, "% 12lld", pItem->m_llValue);
	memcpy (m_szDebugLine + nStartPos, m_szDebugItem, strlen (m_szDebugItem));
	nStartPos += 16;

	int nLen = 0;
	if (pItem->m_szValue != NULL)
	{
		nLen = strlen (pItem->m_szValue);
		if (nLen > 1023 - nStartPos)
			nLen = 1023 - nStartPos;
		memcpy (m_szDebugLine + nStartPos, pItem->m_szValue, nLen);
	}
	m_szDebugLine[nStartPos + nLen + 1] = 0;

	LOGI ("%s", m_szDebugLine);
	return 0;
}


#define		AUDIO_STREAM_MUSIC			3
#define 	AUDIO_FORMAT_PCM_16_BIT		1
#define 	AUDIO_CHANNEL_OUT_MONO		1
#define		AUDIO_CHANNEL_OUT_STEREO	3
#define 	AUDIO_OUTPUT_FLAG_NONE		0
    
typedef void * (* QCGETOUTPUTDEVICE) (int nType, int nSampleRate, int nFormat, int nChannels, int nFlag);                              
typedef int (* QCGETOUTPUTLATENCY) (int * latency, int nType);
typedef int (* QCGETDEVICELATENCY) (void * hDeivce, int nStream, int * pLatency);

int CNDKPlayer::GetOutputLatency (void)
{
	if (gqc_android_devces_ver >= 7)
	{
		return 100;
	}
	else
	{
		FILE * hLibFile = fopen ("/system/lib/libmedia.so", "rb");
		if (hLibFile == NULL)
		{
			QCLOGT ("qcGetOutputLatency", "hLibFile = %p", hLibFile);
			return 0;		
		}
		fseeko (hLibFile, 0LL, SEEK_END);
		int nFileSize = ftello (hLibFile);
		fseeko (hLibFile, 0, SEEK_SET);		
		if (nFileSize <= 0)
			QCLOGT ("qcGetOutputLatency", "nFilesize = %d", nFileSize);		
		char * pFileBuff = new char[nFileSize];
		int nRead = fread (pFileBuff, 1, nFileSize, hLibFile);
		if (nRead != nFileSize)
			QCLOGT ("qcGetOutputLatency", "nRead = %d", nRead);		
		fclose (hLibFile);
		
		char *	pFind = NULL;
		char	szFunGetDevice[256];
		char	szFunGetDevLatency[256];
		char	szFunGetOutLatency[256];		
		char	szFunName[256];
		strcpy (szFunName, "getOutput");
		int		nNameLen = strlen (szFunName);
		char	szFunLatency[256];
		strcpy (szFunLatency, "getLatency");
		int 	nLatencyLen = strlen (szFunLatency);
		
		strcpy (szFunGetDevice, "");
		strcpy (szFunGetDevLatency, "");
		strcpy (szFunGetOutLatency, "");
		
		char * pNameBuff = pFileBuff;
		while (pNameBuff - pFileBuff < nFileSize - nNameLen)
		{
			if (!memcmp (pNameBuff, szFunLatency, nLatencyLen))
			{
				pFind = pNameBuff;
				while (*pFind != 0)
					pFind--;
				pFind++;
				if (strstr (pFind, "AudioSystem") != NULL)
				{
					strcpy (szFunGetDevLatency, pFind);
					//QCLOGT ("qcGetOutputLatency", "szFunGetDevLatency: %s", szFunGetDevLatency);	
				}							
				pNameBuff = pFind + strlen (pFind);
				continue;			
			}
			if (!memcmp (pNameBuff, szFunName, nNameLen))
			{
				pFind = pNameBuff;
				while (*pFind != 0)
					pFind--;
				pFind++;	
				if (strstr (pFind, "audio_output_flags") != NULL)
				{
					strcpy (szFunGetDevice, pFind);
					//QCLOGT ("qcGetOutputLatency", "szFunGetDevice: %s", szFunGetDevice);	
				}
				else if (strstr (pFind, "getOutputLatency") != NULL && strstr (pFind, "AudioSystem") != NULL)
				{
					strcpy (szFunGetOutLatency, pFind);
					//QCLOGT ("qcGetOutputLatency", "szFunGetOutLatency: %s", szFunGetOutLatency);	
				}			
				pNameBuff = pFind + strlen (pFind);
				continue;
			}
			pNameBuff++;
		}
		delete []pFileBuff;
	//	if (strlen (szFunGetDevice) <= 0 || strlen (szFunGetDevLatency) <= 0)
	//		return 0;	
		if (strlen (szFunGetOutLatency) <= 0)
		{
			QCLOGT ("qcGetOutputLatency", "szFunGetOutLatency: %s", szFunGetOutLatency);	
			return 0;
		}
		
		void * hDllMedia = dlopen("/system/lib/libmedia.so", RTLD_NOW);
		if (hDllMedia == NULL)
		{
			QCLOGT ("qcGetOutputLatency", "hDllMedia = %p", hDllMedia);
			return 0;
		}
			
		int nLatency = 0;		
		
		QCGETOUTPUTLATENCY fGetOutLatency = (QCGETOUTPUTLATENCY) dlsym (hDllMedia, szFunGetOutLatency);
		if (fGetOutLatency != NULL)
		{
			fGetOutLatency (&nLatency, -1);
			QCLOGT ("qcGetOutputLatency", "nLatency = %d", nLatency);
		}
		else
		{
			QCLOGT ("qcGetOutputLatency", "fGetOutLatency = %p", fGetOutLatency);
		}
			
		dlclose (hDllMedia);	
	
		return nLatency;
	}				
}
