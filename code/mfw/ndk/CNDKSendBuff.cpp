/*******************************************************************************
	File:		CNDKSendBuff.cpp

	Contains:	The ndk audio render implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-12		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "qcMsg.h"
#include "CNDKSendBuff.h"

#include "USystemFunc.h"
#include "ULogFunc.h"

CNDKSendBuff::CNDKSendBuff(CBaseInst * pBaseInst)
	: CBaseObject (pBaseInst)
	, m_pjVM (NULL)
	, m_pjCls (NULL)
	, m_pjObj (NULL)
	, m_fPushAudio (NULL)
    , m_fPushVideo (NULL)
	, m_pEnv (NULL)
	, m_pDataBuff (NULL)
	, m_nDataSize (0)
	, m_nBuffSize (0)
{
	SetObjectName ("CNDKSendBuff");
}

CNDKSendBuff::~CNDKSendBuff(void)
{

}

int CNDKSendBuff::SetNDK (JavaVM * jvm, JNIEnv* env, jclass clsPlayer, jobject objPlayer)
{
	m_pjVM = jvm;
	m_pjCls = clsPlayer;
	m_pjObj = objPlayer;
	
	m_fPushAudio = env->GetStaticMethodID (m_pjCls, "audioDataFromNative", "(Ljava/lang/Object;[BIJ)V");
	m_fPushVideo = env->GetStaticMethodID (m_pjCls, "videoDataFromNative", "(Ljava/lang/Object;[BIJI)V");

	return QC_ERR_NONE;
}

int	CNDKSendBuff::OnStop (void)
{
	if (m_pjVM == NULL)
		return QC_ERR_NONE;

    QCLOGI ("OnStop!");

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);
	if (m_pDataBuff != NULL)
		m_pEnv->DeleteLocalRef(m_pDataBuff);
	m_pDataBuff = NULL;		
	m_pjVM->DetachCurrentThread ();		
	m_pEnv = NULL;

	return QC_ERR_NONE;
}

int CNDKSendBuff::SendBuff (QC_DATA_BUFF * pBuff)
{
	if (pBuff == NULL)
		return QC_ERR_ARG;

	if (m_pEnv == NULL)
		m_pjVM->AttachCurrentThread (&m_pEnv, NULL);

    int nBuffSize = 0;
    if (pBuff->nMediaType == QC_MEDIA_Audio)
    {
        nBuffSize = pBuff->uSize * 2;  
    }
    else if (pBuff->nMediaType == QC_MEDIA_Video)
    {
		m_pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;
        nBuffSize = m_pVideoBuff->nWidth * m_pVideoBuff->nHeight * 3;
    }
    if (m_nBuffSize < nBuffSize)
    {
        if (m_pDataBuff != NULL)
            m_pEnv->DeleteLocalRef(m_pDataBuff);   
        m_pDataBuff = NULL;
        m_nBuffSize = nBuffSize;
    } 
	if (m_pDataBuff == NULL)
    {
		m_nDataSize = 0;
		m_pDataBuff = m_pEnv->NewByteArray(m_nBuffSize);
    }

	jbyte* pData = m_pEnv->GetByteArrayElements(m_pDataBuff, NULL);

    if (pBuff->nMediaType == QC_MEDIA_Audio)
    {
        m_nDataSize = pBuff->uSize;
        memcpy (pData, pBuff->pBuff, m_nDataSize); 
        m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushAudio, m_pjObj, m_pDataBuff, m_nDataSize, pBuff->llTime);	
    }
    else if (pBuff->nMediaType == QC_MEDIA_Video)
    {
        int	i = 0;       
        unsigned char *	pDataY = (unsigned char *)pData;
        unsigned char *	pDataU = NULL;
        unsigned char *	pDataV = NULL;    
		m_pVideoBuff = (QC_VIDEO_BUFF *)pBuff->pBuffPtr;  
		if (m_pVideoBuff->nType == QC_VDT_YUV420_P)
        {
            m_nDataSize = m_pVideoBuff->nWidth * m_pVideoBuff->nHeight * 3 / 2;
            pDataU = pDataY + m_pVideoBuff->nWidth * m_pVideoBuff->nHeight;
            pDataV = pDataY + m_pVideoBuff->nWidth * m_pVideoBuff->nHeight * 5 / 4;

            for (i = 0; i < m_pVideoBuff->nHeight; i++)
                memcpy (pDataY + i * m_pVideoBuff->nWidth, m_pVideoBuff->pBuff[0] + m_pVideoBuff->nStride[0] * i, m_pVideoBuff->nWidth);
            for (i = 0; i < m_pVideoBuff->nHeight / 2; i++)
                memcpy (pDataU + i * m_pVideoBuff->nWidth / 2, m_pVideoBuff->pBuff[1] + m_pVideoBuff->nStride[1] * i, m_pVideoBuff->nWidth / 2);
            for (i = 0; i < m_pVideoBuff->nHeight / 2; i++)
                memcpy (pDataV + i * m_pVideoBuff->nWidth / 2, m_pVideoBuff->pBuff[2] + m_pVideoBuff->nStride[2] * i, m_pVideoBuff->nWidth / 2);          
        }  
        m_pEnv->CallStaticVoidMethod(m_pjCls, m_fPushVideo, m_pjObj, m_pDataBuff, m_nDataSize, pBuff->llTime, m_pVideoBuff->nType);			      
    }
	
	m_pEnv->ReleaseByteArrayElements(m_pDataBuff, pData, 0);

	return QC_ERR_NONE;
}
