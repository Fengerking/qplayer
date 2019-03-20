/*******************************************************************************
	File:		CMediaCodecDec.cpp

	Contains:	The media codec dec implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-03-01		Bangfei			Create file

*******************************************************************************/
#include <sys/system_properties.h>
#include "qcErr.h"

#include "CMediaCodecDec.h"
#include "CJniEnvUtil.h"
#include "CMsgMng.h"

#include "ULogFunc.h"
#include "USystemFunc.h"

#define AVC_MIME              			"video/avc"
#define HEVC_MIME             			"video/hevc"
#define MPEG4_MIME            			"video/mp4v-es"

#define FEATURE_AdaptivePlayback		"adaptive-playback"
#define KEY_MAX_WIDTH					"max-width"
#define KEY_MAX_HEIGHT					"max-height"
#define MAX_ADAPTIVE_PLAYBACK_WIDTH		1920
#define MAX_ADAPTIVE_PLAYBACK_HEIGHT	1080

#define INFO_OUTPUT_BUFFERS_CHANGED 	-3
#define INFO_OUTPUT_FORMAT_CHANGED  	-2
#define INFO_TRY_AGAIN_LATER        	-1

#define MCBUFFER_FLAG_CODEC_CONFIG 		2
#define MCBUFFER_FLAG_END_OF_STREAM 	4
#define MCBUFFER_FLAG_KEY_FRAME 		1

CMediaCodecDec::CMediaCodecDec(CBaseInst * pBaseInst, void * hInst, int nOSVer)
	: CBaseObject (pBaseInst)
	, m_nOSVer (nOSVer)
	, m_pEnv (NULL)
	, m_bCreateDecFailed (false)
{
	SetObjectName ("CMediaCodecDec");
	memset (&m_fmtVideo, 0, sizeof (m_fmtVideo));
	ResetParam ();
	m_pJVM = NULL;
	m_objSurface = NULL;	
}

CMediaCodecDec::~CMediaCodecDec(void)
{
	ReleaseSurface (NULL);
	QC_DEL_A (m_fmtVideo.pHeadData);
}

int CMediaCodecDec::SetSurface (JavaVM * pJVM, JNIEnv* pEnv, jobject pSurface)
{
	CAutoLock lock (&m_mtDec);
	if (pJVM != NULL)
		m_pJVM = pJVM;

	if (pSurface == NULL)
	{
		if (m_objSurface != NULL)
		{
			ReleaseVideoDec (pEnv);
			pEnv->DeleteGlobalRef(m_objSurface);
			m_objSurface = NULL;			
		}
		m_bNewStart = true;
	}
	else
	{
		if (m_objSurface == NULL)
		{
			m_objSurface = pEnv->NewGlobalRef(pSurface);		
		}
		else
		{
			ReleaseVideoDec (pEnv);			
			pEnv->DeleteGlobalRef(m_objSurface);
			m_objSurface = pEnv->NewGlobalRef(pSurface);					
		}
	}
	return QC_ERR_NONE;
}

int CMediaCodecDec::Init (QC_VIDEO_FORMAT * pFmt)
{
	if (pFmt == NULL || m_pJVM == NULL)
		return QC_ERR_ARG;

	memcpy (&m_fmtVideo, pFmt, sizeof (m_fmtVideo));
	QCLOGI ("Init Width = %d, Height = %d", m_fmtVideo.nWidth, m_fmtVideo.nHeight);
	m_fmtVideo.pHeadData = NULL;
	m_fmtVideo.nHeadSize = 0;
	//if(m_fmtVideo.nCodecID != QC_CODEC_ID_H264 && m_fmtVideo.nCodecID != QC_CODEC_ID_H265) 
	//	return QC_ERR_FAILED;	
	
	return QC_ERR_NONE;
}

int CMediaCodecDec::SetInputBuff (QC_DATA_BUFF * pBuff)
{
	CAutoLock lock (&m_mtDec);	
	int nErr = QC_ERR_FAILED;
	if(m_pJVM == NULL || pBuff == NULL || m_bCreateDecFailed)
		return nErr;
	if (m_objSurface == NULL)
		return QC_ERR_STATUS;

	jlong 	nTimeUs = 10000;
	jint	nIndex = -1;
	jint	nFlag = 0;

	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT)
	{
		QC_VIDEO_FORMAT * pFmt = (QC_VIDEO_FORMAT *)pBuff->pFormat;
		if (pFmt != NULL)
		{
			m_fmtVideo.nWidth = pFmt->nWidth;
			m_fmtVideo.nHeight = pFmt->nHeight;
			m_fmtVideo.nCodecID = pFmt->nCodecID;
		}
	}
	if ((pBuff->uFlag & QCBUFF_HEADDATA) == QCBUFF_HEADDATA)
	{
		QC_DEL_A (m_fmtVideo.pHeadData);
		m_fmtVideo.nHeadSize = pBuff->uSize;
		m_fmtVideo.pHeadData = new unsigned char[m_fmtVideo.nHeadSize];
		memcpy (m_fmtVideo.pHeadData, pBuff->pBuff, m_fmtVideo.nHeadSize);
		QCLOGI ("Head Size = %d", m_fmtVideo.nHeadSize);
		nFlag = MCBUFFER_FLAG_CODEC_CONFIG;
	}
	if ((pBuff->uFlag & QCBUFF_KEY_FRAME) == QCBUFF_KEY_FRAME) 
	{
		nFlag |= MCBUFFER_FLAG_KEY_FRAME;
	}
	if ((pBuff->uFlag & QCBUFF_NEW_POS) == QCBUFF_NEW_POS)
	{
		Flush ();
	}
	if ((pBuff->uFlag & QCBUFF_NEW_FORMAT) == QCBUFF_NEW_FORMAT && m_objMediaCodec != NULL)
	{
		QCLOGI ("New format...");	
/*		
		if (m_nOSVer >= 5 && m_mhdDequeueInputBuffer != NULL && m_mhdDequeueOutputBuffer != NULL)
		{
			QCLOGI ("Frame Time = %d", m_nFrameTime);	
			if (m_nFrameTime > 100)
				m_nFrameTime = 100;
			m_nFrameTime = m_nFrameTime * 800;	

			nIndex = m_pEnv->CallIntMethod(m_objMediaCodec, m_mhdDequeueInputBuffer, nTimeUs);
			QCLOGI ("Input index = %d", nIndex);
			if(nIndex >= 0) 
			{		
				m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdQueueInputBuffer, nIndex, 0, 0, 0, MCBUFFER_FLAG_END_OF_STREAM);	

				jobject 	buffer_info = buffer_info = m_pEnv->NewObject(m_clsBufferInfo, m_mhdBufferInfoConstructor);
				jobject   	objBufferInfo = objBufferInfo = m_pEnv->NewGlobalRef(buffer_info);
				m_pEnv->DeleteLocalRef(buffer_info);	
				while (true)
				{	
					nTimeUs = 30000;
					nIndex = m_pEnv->CallIntMethod(m_objMediaCodec, m_mhdDequeueOutputBuffer, objBufferInfo, nTimeUs);	
					QCLOGI ("output index = %d", nIndex);
					if (nIndex < 0)
						break;									
					m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdReleaseOutputBuffer, nIndex, (jboolean)true);
					qcSleep (m_nFrameTime);
				}
				m_pEnv->DeleteGlobalRef(objBufferInfo);			
			}
		}	
*/	
		if (!m_bAdaptivePlayback)
			ReleaseVideoDec (m_pEnv);
		if (m_objMediaCodec != NULL)
			Flush ();	
	}
	if (m_objMediaCodec == NULL)
	{
		if (CreateVideoDec() != QC_ERR_NONE)
		{
			m_bCreateDecFailed = true;
			if (m_pBaseInst != NULL && m_pBaseInst->m_pMsgMng != NULL)
				m_pBaseInst->m_pMsgMng->Notify(QC_MSG_VIDEO_HWDEC_FAILED, 0, 0);
			return QC_ERR_FAILED;
		}
		Start ();
	}
	if(m_mhdDequeueInputBuffer == NULL || m_mhdQueueInputBuffer == NULL || m_mhdGetOutputFormat == NULL) {
		nErr = UpdateBufferFunc();
		if(nErr != QC_ERR_NONE)
			return QC_ERR_FAILED;
	}
	if(m_objInputBuffers == NULL) {
		nErr = UpdateBuffers();
		if(nErr != QC_ERR_NONE)
			return QC_ERR_FAILED;
	}

	//QCLOGI("setInputBuffer Size = % 8d Time = % 8lld, nFlag =  % 8d", pBuff->uSize, pBuff->llTime, pBuff->uFlag);
	if(m_bNewStart && ((pBuff->uFlag & QCBUFF_KEY_FRAME) != QCBUFF_KEY_FRAME) && ((pBuff->uFlag & QCBUFF_HEADDATA) != QCBUFF_HEADDATA)) {
		return QC_ERR_NONE;
	}
	m_bNewStart = false;

	nTimeUs = 10000;
	nIndex = m_pEnv->CallIntMethod(m_objMediaCodec, m_mhdDequeueInputBuffer, nTimeUs);
	if (m_pEnv->ExceptionOccurred()) {
		QCLOGI("Exception in MediaCodec.dequeueInputBuffer. nIndex is %d", nIndex);
		m_pEnv->ExceptionClear();
		return QC_ERR_FAILED;
	}
	if(nIndex >= 0) 
	{		
		jobject buf = m_pEnv->GetObjectArrayElement(m_objInputBuffers, nIndex);
		if(buf == 0) {
			QCLOGI("MediaCodec nIndex buf is null");
			return QC_ERR_FAILED;
		}
        jsize size = m_pEnv->GetDirectBufferCapacity(buf);
        unsigned char *bufptr = ( unsigned char *)m_pEnv->GetDirectBufferAddress(buf);
		if(size < 0 || bufptr == NULL || pBuff->uSize > size) {
			QCLOGI("MediaCodec nIndex buf size %d, InBuffer->nSize %d, bufptr %p", size, pBuff->uSize, bufptr);
			return QC_ERR_FAILED;
		}
		memcpy(bufptr, pBuff->pBuff, pBuff->uSize);
		int nSize = pBuff->uSize;
		nTimeUs = pBuff->llTime * 1000;
		//QCLOGI ("Index = % 8d,  size = % 8d, Time = % 8lld, Flag = % 8d", nIndex, nSize, nTimeUs, nFlag);
		m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdQueueInputBuffer, nIndex, 0, nSize, nTimeUs, nFlag);
		if (m_pEnv->ExceptionOccurred()) {
			QCLOGI("Exception in MediaCodec.dequeueInputBuffer");
			m_pEnv->ExceptionClear();
			m_pEnv->DeleteLocalRef(buf);
			return QC_ERR_FAILED;
		}        
		m_pEnv->DeleteLocalRef(buf);
	} 
	else
	{
		return QC_ERR_RETRY;
	}

	return QC_ERR_NONE;
}

int CMediaCodecDec::SetInputEmpty (void)
{
	if (m_objMediaCodec == NULL)
		return QC_ERR_STATUS;

	jlong nTimeUs = 10000;
	int nIndex = m_pEnv->CallIntMethod(m_objMediaCodec, m_mhdDequeueInputBuffer, nTimeUs);
	if (m_pEnv->ExceptionOccurred()) {
		QCLOGI("Exception in MediaCodec.dequeueInputBuffer");
		m_pEnv->ExceptionClear();
		return QC_ERR_FAILED;
	}
	if(nIndex >= 0) 
	{		
		m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdQueueInputBuffer, nIndex, 0, 0, 0, MCBUFFER_FLAG_END_OF_STREAM);
		if (m_pEnv->ExceptionOccurred()) {
			QCLOGI("Exception in MediaCodec.dequeueInputBuffer");
			m_pEnv->ExceptionClear();
			return QC_ERR_FAILED;
		}        
	} 
	else
	{
		return QC_ERR_RETRY;
	}
	return QC_ERR_NONE;
}

int CMediaCodecDec::RenderOutput (long long * pOutTime, bool bRender)
{
	CAutoLock lock (&m_mtDec);	
	if (m_objMediaCodec == NULL)
		return QC_ERR_STATUS;
	int nErr = QC_ERR_FAILED;
	if(m_mhdDequeueInputBuffer == NULL || m_mhdQueueInputBuffer == NULL || m_mhdGetOutputFormat == NULL) {
		nErr = UpdateBufferFunc();
		if(nErr != QC_ERR_NONE)
			return nErr;
	}
	if(m_objInputBuffers == NULL) {
		nErr = UpdateBuffers();
		if(nErr != QC_ERR_NONE)
			return nErr;
	}

	jlong 	nTimeUs = 60000;
	int 	nIndex = m_pEnv->CallIntMethod(m_objMediaCodec, m_mhdDequeueOutputBuffer, m_objBufferInfo, nTimeUs);
	if (m_pEnv->ExceptionOccurred()) {
		QCLOGI("Exception in MediaCodec.dequeueOutputBuffer");
		m_pEnv->ExceptionClear();
		return QC_ERR_FAILED;
	}
	if(nIndex >= 0) 
	{
		m_mhdPtsField	 = m_pEnv->GetFieldID(m_clsBufferInfo, "presentationTimeUs", "J");
		//m_mhdSizeField	 = m_pEnv->GetFieldID(m_clsBufferInfo, "size", "I");
		//m_mhdOffsetField = m_pEnv->GetFieldID(m_clsBufferInfo, "offset", "I");
		//int nSize = m_pEnv->GetIntField(m_objBufferInfo, m_mhdSizeField);
		nTimeUs = m_pEnv->GetLongField(m_objBufferInfo, m_mhdPtsField) / 1000;
		m_nFrameTime = (int)(nTimeUs - m_llLastTime);
		m_llLastTime = nTimeUs;
		*pOutTime = m_llLastTime;
	} else if(nIndex == INFO_OUTPUT_BUFFERS_CHANGED){
		QCLOGI ("Buffer changed!");
		if (m_objOutputBuffers) {
		    m_pEnv->DeleteGlobalRef(m_objOutputBuffers);
			m_objOutputBuffers = 0;
		}
		m_mhdGetOutputBuffers = m_pEnv->GetMethodID(m_clsMediaCodec, "getOutputBuffers", "()[Ljava/nio/ByteBuffer;");	
		if(m_mhdGetOutputBuffers == NULL) {
			QCLOGI("can not find the getOutputBuffers fucntion \n");
			if (m_pEnv->ExceptionOccurred()) 	{				
				m_pEnv->ExceptionDescribe();
				m_pEnv->ExceptionClear();
			}
			return QC_ERR_FAILED;
		}
		jobjectArray output_buffers = (jobjectArray)m_pEnv->CallObjectMethod(m_objMediaCodec, m_mhdGetOutputBuffers);   
		m_objOutputBuffers = (jobjectArray)m_pEnv->NewGlobalRef(output_buffers);
		m_pEnv->DeleteLocalRef(output_buffers);
		return QC_ERR_FAILED;
	} else if(nIndex == INFO_OUTPUT_FORMAT_CHANGED){
		jobject format = m_pEnv->CallObjectMethod(m_objMediaCodec, m_mhdGetOutputFormat);
		if (m_pEnv->ExceptionOccurred()) {
			QCLOGI("Exception in MediaCodec.getOutputFormat (GetOutput)");
			m_pEnv->ExceptionClear();
			return QC_ERR_FAILED;
		}
		jstring jstr = m_pEnv->NewStringUTF("width");
	//	m_fmtVideo.nWidth = m_pEnv->CallIntMethod(format, m_mhdGetInteger, jstr);
		m_pEnv->DeleteLocalRef(jstr);
		jstr = m_pEnv->NewStringUTF("height");
	//	m_fmtVideo.nHeight = m_pEnv->CallIntMethod(format, m_mhdGetInteger, jstr);
		m_pEnv->DeleteLocalRef(jstr);

		jstr = m_pEnv->NewStringUTF("color-format");
		int ColorFormat = m_pEnv->CallIntMethod(format, m_mhdGetInteger, jstr);
		m_pEnv->DeleteLocalRef(jstr);
		QCLOGI ("Format changed! Size %d  X  %d", m_fmtVideo.nWidth, m_fmtVideo.nHeight);
		return QC_ERR_FORMAT;
	} else if(nIndex == INFO_TRY_AGAIN_LATER){
		return QC_ERR_FAILED;
	} else {
		return QC_ERR_FAILED;
	}

	if(m_mhdReleaseOutputBuffer == NULL) {
		m_mhdReleaseOutputBuffer = m_pEnv->GetMethodID(m_clsMediaCodec, "releaseOutputBuffer", "(IZ)V");
		if(m_mhdReleaseOutputBuffer == NULL) {
			QCLOGI("can not find the releaseOutputBuffer fucntion \n");
			if (m_pEnv->ExceptionOccurred()) 	{				
				m_pEnv->ExceptionDescribe();
				m_pEnv->ExceptionClear();
			}
			QCLOGI("can not find the releaseOutputBuffer fucntion \n");
			return QC_ERR_FAILED;
		}
	}
	if(m_mhdReleaseOutputBuffer != NULL) 
		m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdReleaseOutputBuffer, nIndex, (jboolean)bRender);

	return QC_ERR_NONE;
}

int CMediaCodecDec::RenderRestBuff (JNIEnv* m_pEnv)
{
	return QC_ERR_NONE;
}

int	CMediaCodecDec::CreateVideoDec (void)
{
	if (m_objMediaCodec != NULL)
		return QC_ERR_NONE;

	jclass lclass = m_pEnv->FindClass("android/media/MediaCodec");
	if(lclass == NULL){
		QCLOGI("can not find the android/media/MediaCodec class \n");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_clsMediaCodec = (jclass)m_pEnv->NewGlobalRef(lclass);
	m_pEnv->DeleteLocalRef(lclass);

	lclass = m_pEnv->FindClass("android/media/MediaFormat");
	if(lclass == NULL){
		QCLOGI("can not find the android/media/MediaFormat class \n");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_clsMediaFormat = (jclass)m_pEnv->NewGlobalRef(lclass);
	m_pEnv->DeleteLocalRef(lclass);
	lclass = m_pEnv->FindClass("android/media/MediaCodec$BufferInfo");
	if(lclass == NULL){
		QCLOGI("can not find the android/media/MediaCodec$BufferInfo class \n");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_clsBufferInfo = (jclass)m_pEnv->NewGlobalRef(lclass);
	m_pEnv->DeleteLocalRef(lclass);
	lclass = m_pEnv->FindClass("java/nio/ByteBuffer");
	if(lclass == NULL){
		QCLOGI("can not find the java/nio/ByteBuffer class \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_clsByteBuffer = (jclass)m_pEnv->NewGlobalRef(lclass);
	m_pEnv->DeleteLocalRef(lclass);
	m_mhdCreateByCodecType = m_pEnv->GetStaticMethodID(m_clsMediaCodec, "createDecoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;");	
	if(m_mhdCreateByCodecType == NULL) {
		QCLOGI("can not find the createDecoderByType fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}	
	m_mhdConfigure = m_pEnv->GetMethodID(m_clsMediaCodec, "configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V");	
	if(m_mhdConfigure == NULL) {
		QCLOGI("can not find the configure fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdCreateVideoFormat = m_pEnv->GetStaticMethodID(m_clsMediaFormat, "createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;");	
	if(m_mhdCreateVideoFormat == NULL) {
		QCLOGI("can not find the createVideoFormat fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	const char* Mime; 
	if(m_fmtVideo.nCodecID == QC_CODEC_ID_H264) 
		Mime = AVC_MIME;
	else if(m_fmtVideo.nCodecID  == QC_CODEC_ID_H265) 
		Mime = HEVC_MIME;
	jstring jstr = m_pEnv->NewStringUTF(Mime);
	jobject codecObj = NULL;
	codecObj = m_pEnv->CallStaticObjectMethod(m_clsMediaCodec, m_mhdCreateByCodecType, jstr);
	if(codecObj == NULL){
		if (m_pEnv->ExceptionCheck()) {			
			QCLOGI("Could not create mediacodec for %s", Mime);
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
			m_pEnv->DeleteLocalRef(jstr);
			return QC_ERR_FAILED;
		}
	}
	m_objMediaCodec = m_pEnv->NewGlobalRef(codecObj);
	m_pEnv->DeleteLocalRef(codecObj);

	UpdateBufferFunc ();

	QCLOGI("create videoFormat for with %d height %d.", m_fmtVideo.nWidth, m_fmtVideo.nHeight);

    jobject formatObj = m_pEnv->CallStaticObjectMethod(m_clsMediaFormat, m_mhdCreateVideoFormat, jstr, m_fmtVideo.nWidth, m_fmtVideo.nHeight);
	if(formatObj == NULL){
		if (m_pEnv->ExceptionCheck()) {			
			QCLOGI("Could not create videoFormat for %s", Mime);
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
			m_pEnv->DeleteLocalRef(jstr);
			m_pEnv->DeleteLocalRef(formatObj);
			return QC_ERR_FAILED;
		}
	}
	m_objVideoFormat = m_pEnv->NewGlobalRef(formatObj);	

	IsSupportAdpater(jstr);
	m_pEnv->DeleteLocalRef(jstr);

	if(m_bAdaptivePlayback && m_objVideoFormat != NULL && m_mhdSetInteger != NULL) {
		jstring jstr1 = m_pEnv->NewStringUTF(KEY_MAX_WIDTH);
		m_pEnv->CallVoidMethod(m_objVideoFormat, m_mhdSetInteger, jstr1, MAX_ADAPTIVE_PLAYBACK_WIDTH);
		m_pEnv->DeleteLocalRef(jstr1);

		jstr1 = m_pEnv->NewStringUTF(KEY_MAX_HEIGHT);
		m_pEnv->CallVoidMethod(m_objVideoFormat, m_mhdSetInteger, jstr1, MAX_ADAPTIVE_PLAYBACK_HEIGHT);
		m_pEnv->DeleteLocalRef(jstr1);

		QCLOGI ("Set max width and height");		
	}

	if (m_fmtVideo.pHeadData != NULL && m_fmtVideo.nHeadSize > 0)
	{
		int nErr = SetHeadData(m_fmtVideo.pHeadData, m_fmtVideo.nHeadSize);
		if(nErr != QC_ERR_NONE) 
		{
			m_pEnv->DeleteLocalRef(formatObj);
			return nErr;
		}
	}

	m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdConfigure, m_objVideoFormat, m_objSurface, NULL, 0);
    if (m_pEnv->ExceptionOccurred()) 
	{
		QCLOGI("can not config the video format fucntion \n");		
		m_pEnv->ExceptionClear();
		m_pEnv->DeleteLocalRef(formatObj);
		return QC_ERR_FAILED;
    }
	m_pEnv->DeleteLocalRef(formatObj);

	return QC_ERR_NONE;
}

int	CMediaCodecDec::ReleaseVideoDec (JNIEnv* pEnv)
{
	if(m_pJVM == NULL || m_objMediaCodec == NULL)
		return QC_ERR_FAILED;
	
	if (pEnv == NULL)
	{
		CJniEnvUtil envUtil (m_pJVM);
		pEnv = envUtil.getEnv ();
	}

	Stop(pEnv);

	if(m_mhdRelease == NULL) 
		m_mhdRelease = pEnv->GetMethodID(m_clsMediaCodec, "release", "()V");
	if(m_mhdRelease != NULL) {
		pEnv->CallVoidMethod(m_objMediaCodec, m_mhdRelease);
		if (pEnv->ExceptionOccurred()) {
			QCLOGI("Exception in MediaCodec.release");
			pEnv->ExceptionClear();
		}
	}
	pEnv->DeleteGlobalRef(m_objMediaCodec);
	m_objMediaCodec = NULL;

	if (m_objBufferInfo != NULL) {
        pEnv->DeleteGlobalRef(m_objBufferInfo);
		m_objBufferInfo = NULL;
	}
	if(m_objVideoFormat != NULL) {
        pEnv->DeleteGlobalRef(m_objVideoFormat);
		m_objVideoFormat = NULL;
	}
	if(m_clsMediaCodec != NULL) {
		pEnv->DeleteGlobalRef(m_clsMediaCodec);
		m_clsMediaCodec = NULL;
	}
	if(m_clsMediaFormat != NULL){
		pEnv->DeleteGlobalRef(m_clsMediaFormat);
		m_clsMediaFormat = NULL;
	}
	if(m_clsBufferInfo != NULL){
		pEnv->DeleteGlobalRef(m_clsBufferInfo);
		m_clsBufferInfo = NULL;
	}
	if(m_clsByteBuffer != NULL){
		pEnv->DeleteGlobalRef(m_clsByteBuffer);
		m_clsByteBuffer = NULL;
	}
	if (m_objInputBuffers != NULL) {
        pEnv->DeleteGlobalRef(m_objInputBuffers);
		m_objInputBuffers = NULL;
	}
	if (m_objOutputBuffers != NULL) {
        pEnv->DeleteGlobalRef(m_objOutputBuffers);
		m_objOutputBuffers = NULL;
	}
	return ResetParam ();
}

int CMediaCodecDec::UpdateBufferFunc()
{
	if(m_pJVM == NULL || m_clsMediaCodec == NULL || m_clsBufferInfo == NULL)
		return QC_ERR_FAILED;

	m_mhdGetOutputFormat = m_pEnv->GetMethodID(m_clsMediaCodec, "getOutputFormat", "()Landroid/media/MediaFormat;");
	if(m_mhdGetOutputFormat == NULL) {
		QCLOGI("can not find the getOutputFormat fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdDequeueInputBuffer = m_pEnv->GetMethodID(m_clsMediaCodec, "dequeueInputBuffer", "(J)I");
	if(m_mhdDequeueInputBuffer == NULL) {
		QCLOGI("can not find the dequeueInputBuffer fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdDequeueOutputBuffer = m_pEnv->GetMethodID(m_clsMediaCodec, "dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I");
	if(m_mhdDequeueOutputBuffer == NULL) {
		QCLOGI("can not find the dequeueOutputBuffer fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdQueueInputBuffer = m_pEnv->GetMethodID(m_clsMediaCodec, "queueInputBuffer", "(IIIJI)V");
	if(m_mhdQueueInputBuffer == NULL) {
		QCLOGI("can not find the queueInputBuffer fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdBufferInfoConstructor = m_pEnv->GetMethodID(m_clsBufferInfo, "<init>", "()V");
	if(m_mhdBufferInfoConstructor == NULL) {
		QCLOGI("can not find the bufferinfo construct fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdSetInteger = m_pEnv->GetMethodID(m_clsMediaFormat, "setInteger", "(Ljava/lang/String;I)V");
	if(m_mhdSetInteger == NULL) {
		QCLOGI("can not find the setInteger fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
	}
	m_mhdGetInteger = m_pEnv->GetMethodID(m_clsMediaFormat, "getInteger", "(Ljava/lang/String;)I");
	if(m_mhdGetInteger == NULL) {
		QCLOGI("can not find the getInteger fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	if (m_objBufferInfo) {
        m_pEnv->DeleteGlobalRef(m_objBufferInfo);
		m_objBufferInfo = 0;
	}

	jobject buffer_info = m_pEnv->NewObject(m_clsBufferInfo, m_mhdBufferInfoConstructor);
	m_objBufferInfo = m_pEnv->NewGlobalRef(buffer_info);
	m_pEnv->DeleteLocalRef(buffer_info);

	return QC_ERR_NONE;
}

int CMediaCodecDec::UpdateBuffers()
{
	if(m_bStarted == false  || m_pJVM == NULL || m_objMediaCodec == NULL)
		return QC_ERR_FAILED;

	if (m_objInputBuffers) {
        m_pEnv->DeleteGlobalRef(m_objInputBuffers);
		m_objInputBuffers = 0;
	}
	if (m_objOutputBuffers) {
        m_pEnv->DeleteGlobalRef(m_objOutputBuffers);
		m_objOutputBuffers = 0;
	}

	m_mhdGetInputBuffers = m_pEnv->GetMethodID(m_clsMediaCodec, "getInputBuffers", "()[Ljava/nio/ByteBuffer;");	
	if(m_mhdGetInputBuffers == NULL) {
		QCLOGI("can not find the getInputBuffers fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	m_mhdGetOutputBuffers = m_pEnv->GetMethodID(m_clsMediaCodec, "getOutputBuffers", "()[Ljava/nio/ByteBuffer;");	
	if(m_mhdGetOutputBuffers == NULL) {
		QCLOGI("can not find the getOutputBuffers fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}
	jobjectArray input_buffers = (jobjectArray)m_pEnv->CallObjectMethod(m_objMediaCodec, m_mhdGetInputBuffers);
	jobjectArray output_buffers = (jobjectArray)m_pEnv->CallObjectMethod(m_objMediaCodec, m_mhdGetOutputBuffers);   
	m_objInputBuffers = (jobjectArray)m_pEnv->NewGlobalRef(input_buffers);
	m_objOutputBuffers = (jobjectArray)m_pEnv->NewGlobalRef(output_buffers);
	m_pEnv->DeleteLocalRef(input_buffers);
	m_pEnv->DeleteLocalRef(output_buffers);

	return QC_ERR_NONE;
}

int CMediaCodecDec::SetHeadData(unsigned char *pBuf, int nLen)
{
	if (m_fmtVideo.nCodecID == QC_CODEC_ID_H265)
	{
		SetHeadDataJava (pBuf, nLen, 0);
	}
	else
	{
		int nWorkNal = 0X01000000;
		int nOffset = 0;
		for (int i = 8; i < nLen; i++)
		{
			if (!memcmp (pBuf + i, &nWorkNal, 4))
			{
				nOffset = i;
				break;
			}
		}
		QCLOGI ("Head Size = %d, Offset = %d", nLen, nOffset);
		if (nOffset == 0)
		{
			SetHeadDataJava (pBuf, nLen, 0);
		}
		else
		{
			SetHeadDataJava (pBuf, nOffset, 0);
			SetHeadDataJava (pBuf + nOffset, nLen - nOffset, 1);				
		}	
	}
	return QC_ERR_NONE;
}

int CMediaCodecDec::SetHeadDataJava(unsigned char *pBuf, int nLen, int nIndex)
{
	if(nLen == 0) 
		return QC_ERR_NONE;

	jmethodID	fAllocDirect = m_pEnv->GetStaticMethodID(m_clsByteBuffer, "allocateDirect", "(I)Ljava/nio/ByteBuffer;");	
	if(fAllocDirect == NULL) {
		QCLOGI("can not find the allocateDirect fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	m_mhdSetByteBuffer = m_pEnv->GetMethodID(m_clsMediaFormat, "setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");
	if(m_mhdSetByteBuffer == NULL) {
		QCLOGI("can not find the setByteBuffer fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	jobject bytebuf = m_pEnv->CallStaticObjectMethod(m_clsByteBuffer, fAllocDirect, nLen);
	if(bytebuf == 0) {
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED; 
	}
	unsigned char *ptr = (unsigned char *)m_pEnv->GetDirectBufferAddress(bytebuf);
	memcpy(ptr, pBuf, nLen);
	jstring jstr = NULL;
	if(nIndex == 0) {
		jstr = m_pEnv->NewStringUTF("csd-0");
	} else if(nIndex == 1){
		jstr = m_pEnv->NewStringUTF("csd-1");
	}

	//QCLOGI("set PPS or SPS buffer nLen %d", nLen);
	m_pEnv->CallVoidMethod(m_objVideoFormat, m_mhdSetByteBuffer, jstr, bytebuf);
	m_pEnv->DeleteLocalRef(bytebuf);
	m_pEnv->DeleteLocalRef(jstr);

	return QC_ERR_NONE;	
}

int CMediaCodecDec::Start()
{
	if(m_bStarted) 
		return QC_ERR_NONE;
	if(m_pJVM == NULL || m_objMediaCodec == NULL)
		return QC_ERR_FAILED;

	if(m_mhdStart == NULL) {
		m_mhdStart = m_pEnv->GetMethodID(m_clsMediaCodec, "start", "()V"); 
		if(m_mhdStart == NULL)
			return QC_ERR_FAILED;
	}

	m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdStart);
	if (m_pEnv->ExceptionOccurred()) {
		QCLOGI("Exception in MediaCodec.start");
		m_pEnv->ExceptionClear();
		return QC_ERR_FAILED;
	}

	m_bStarted = true;
	m_bNewStart = true;

	return QC_ERR_NONE;
}

int CMediaCodecDec::Stop(JNIEnv* pEnv)
{
	if(m_pJVM == NULL || m_objMediaCodec == NULL)
		return QC_ERR_FAILED;
	if (pEnv == NULL)
	{
		CJniEnvUtil envUtil (m_pJVM);
		pEnv = envUtil.getEnv ();
	}
	if(m_bStarted && m_objMediaCodec != NULL) {
		if(m_mhdStop == NULL) {
			m_mhdStop = pEnv->GetMethodID(m_clsMediaCodec, "stop", "()V"); 
			if(m_mhdStop == NULL)
				return QC_ERR_FAILED;
		}
		pEnv->CallVoidMethod(m_objMediaCodec, m_mhdStop);
		if (pEnv->ExceptionOccurred()) {
			QCLOGI("Exception in MediaCodec.stop");
			pEnv->ExceptionClear();
		}
	}

	if (m_objInputBuffers) {
        pEnv->DeleteGlobalRef(m_objInputBuffers);
		m_objInputBuffers = NULL;
	}
	if (m_objOutputBuffers) {
        pEnv->DeleteGlobalRef(m_objOutputBuffers);
		m_objOutputBuffers = NULL;
	}

	m_bStarted = false;	
	return QC_ERR_NONE;
}

int CMediaCodecDec::Flush()
{
	if(m_pJVM == NULL || m_objMediaCodec == NULL)
		return QC_ERR_FAILED;

	if(m_mhdFlush == NULL) {
		m_mhdFlush = m_pEnv->GetMethodID(m_clsMediaCodec, "flush", "()V"); 
		if(m_mhdFlush == NULL)
			return QC_ERR_FAILED;
	}
	QCLOGI("call Flush for mediacodec");
	m_pEnv->CallVoidMethod(m_objMediaCodec, m_mhdFlush);

	m_bNewStart = true;
	return QC_ERR_NONE;
}

int CMediaCodecDec::IsSupportAdpater(jstring jstr)
{
	if (m_nOSVer < 5)
		return QC_ERR_FAILED;

	jmethodID mGetCodecInfo = m_pEnv->GetMethodID(m_clsMediaCodec, "getCodecInfo", "()Landroid/media/MediaCodecInfo;");
	if(mGetCodecInfo == NULL) {
		QCLOGI("can not find the getCodecInfo fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	jclass linfoclass = m_pEnv->FindClass("android/media/MediaCodecInfo");
	if(linfoclass == NULL){
		QCLOGI("can not find the android/media/MediaCodecInfo class \n");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}		

	jmethodID mGetCapabilitiesForType = m_pEnv->GetMethodID(linfoclass, "getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;");
	if(mGetCapabilitiesForType == NULL) {
		QCLOGI("can not find the mGetCapabilitiesForType fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	jclass lCapabilitiesclass = m_pEnv->FindClass("android/media/MediaCodecInfo$CodecCapabilities");
	if(lCapabilitiesclass == NULL){
		QCLOGI("can not find the android/media/MediaCodecInfo$CodecCapabilities class \n");
		if (m_pEnv->ExceptionOccurred()) {				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	jmethodID misFeatureSupported = m_pEnv->GetMethodID(lCapabilitiesclass, "isFeatureSupported", "(Ljava/lang/String;)Z");
	if(misFeatureSupported == NULL) {
		QCLOGI("can not find the isFeatureSupported fucntion \n");
		if (m_pEnv->ExceptionOccurred()) 	{				
			m_pEnv->ExceptionDescribe();
			m_pEnv->ExceptionClear();
		}
		return QC_ERR_FAILED;
	}

	jobject info = m_pEnv->CallObjectMethod(m_objMediaCodec, mGetCodecInfo);

	jobject codec_capabilities = m_pEnv->CallObjectMethod(info, mGetCapabilitiesForType, jstr);
	if(codec_capabilities == NULL) {
		return QC_ERR_FAILED;
	}

	jstring jstrad = m_pEnv->NewStringUTF(FEATURE_AdaptivePlayback);
	jboolean bAdaptivePlayback =  m_pEnv->CallBooleanMethod(codec_capabilities, misFeatureSupported, jstrad);
	m_pEnv->DeleteLocalRef(jstrad);

	m_bAdaptivePlayback = (bool)bAdaptivePlayback;
	QCLOGI("m_bAdaptivePlayback %d, bAdaptivePlayback %d, FEATURE_AdaptivePlayback %s", m_bAdaptivePlayback, bAdaptivePlayback, FEATURE_AdaptivePlayback);
	
	return QC_ERR_NONE;	
}

int	CMediaCodecDec::ReleaseSurface (JNIEnv * pEnv)
{
	if (m_objSurface == NULL || m_pJVM == NULL)
		return QC_ERR_FAILED;
	if (pEnv == NULL)
	{
		CJniEnvUtil envUtil (m_pJVM);
		pEnv = envUtil.getEnv ();
	}
	pEnv->DeleteGlobalRef(m_objSurface);
	m_objSurface = NULL;	
	return QC_ERR_NONE;	
}

int CMediaCodecDec::ResetParam (void)
{
	m_llLastTime				= 0;
	m_nFrameTime				= 30;

	m_bStarted					= false;
	m_bNewStart					= true;
	m_bAdaptivePlayback			= false;

	m_objMediaCodec				= NULL;
	m_objBufferInfo				= NULL;
	m_objVideoFormat			= NULL;
	m_objInputBuffers			= NULL;
	m_objOutputBuffers			= NULL;

	m_clsMediaCodec				= NULL;
	m_clsMediaFormat			= NULL;
	m_clsBufferInfo				= NULL; 
	m_clsByteBuffer				= NULL;

	m_mhdToString				= NULL;
	m_mhdCreateByCodecType		= NULL;
	m_mhdConfigure				= NULL;
	m_mhdStart					= NULL;
	m_mhdStop					= NULL;
	m_mhdFlush					= NULL;
	m_mhdRelease				= NULL;
	m_mhdGetOutputFormat		= NULL;
	m_mhdGetInputBuffers		= NULL;
	m_mhdGetOutputBuffers		= NULL;
	m_mhdDequeueInputBuffer		= NULL;
	m_mhdDequeueOutputBuffer	= NULL;
	m_mhdQueueInputBuffer		= NULL;
	m_mhdReleaseOutputBuffer	= NULL;
	m_mhdCreateVideoFormat		= NULL;
	m_mhdSetInteger				= NULL;
	m_mhdSetByteBuffer			= NULL;
	m_mhdGetInteger				= NULL;
	m_mhdBufferInfoConstructor	= NULL;
	m_mhdSizeField				= NULL;
	m_mhdOffsetField			= NULL;
	m_mhdPtsField				= NULL;	

	return QC_ERR_NONE;
}

int CMediaCodecDec::OnStart (JNIEnv * pEnv)
{
	QCLOGI ("OnStart env = %p", pEnv);
	m_pEnv = pEnv;
	return QC_ERR_NONE;
}

int CMediaCodecDec::OnStop (JNIEnv * pEnv)
{
	QCLOGI ("OnStop env = %p", pEnv);
	ReleaseVideoDec (pEnv);
	m_pEnv = NULL;
	return QC_ERR_NONE;
}
