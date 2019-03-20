#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <jni.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/system_properties.h>
#include "TTLog.h"
#include "TTOsalConfig.h"
#include "TTJniEnvUtil.h"
#include "TTTRtmpPush.h"
#include "NativeOpenGl2Proxy.h"

JavaVM* gJVM = NULL;
int g_LogOpenFlag = 1;
static const char* const kClassPathName = "com/ali/music/media/cameracollection/RtmpPublish";
static const char* const kClassFieldName = "mNativeRtmpPara";


class JNITTPushListener: public ITTMsgObserver
{
public:
	JNITTPushListener(jobject thiz, jobject weak_thiz, jmethodID postEvnt, JNIEnv* aEnv);
    ~JNITTPushListener();
	void NotifyEvent(TTPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2);
	long GetMobileTx();

private:
    JNITTPushListener();
    jclass      	mClass;     // Reference to MediaPlayer class
    jobject     	mObject;    // Weak ref to MediaPlayer Java object to call on
	JNIEnv*			mEnv;
	jmethodID	    post_event;

	jmethodID		mTx;
	jclass          mTrafficStats;
};


JNITTPushListener::JNITTPushListener(jobject thiz, jobject weak_thiz, jmethodID postEvnt, JNIEnv* aEnv)
{
    // Hold onto the  class for use in calling the static method
    // that posts events to the application thread.
	mEnv = aEnv;
	post_event = postEvnt;
    jclass clazz = mEnv->GetObjectClass(thiz);
    if (clazz == NULL) {
        LOGE("Can't create JNITTMediaPlayerListener");
        mEnv->ThrowNew(clazz, "Can't create JNITTMsgListener");
        return;
    }
    mClass = (jclass)mEnv->NewGlobalRef(clazz);

	clazz = mEnv->FindClass("android/net/TrafficStats");
	mTrafficStats = (jclass)mEnv->NewGlobalRef(clazz);

	mTx = mEnv->GetStaticMethodID(mTrafficStats, "getMobileTxBytes", "()J");//getTotalTxBytes
	 
    // We use a weak reference so the object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject = mEnv->NewGlobalRef(weak_thiz);
}

JNITTPushListener::~JNITTPushListener()
{
	// remove global references
	CJniEnvUtil	env(gJVM);
	JNIEnv* penv = env.getEnv();
	if (penv!=NULL){
		penv->DeleteGlobalRef(mObject);
		penv->DeleteGlobalRef(mClass);
		penv->DeleteGlobalRef(mTrafficStats);
	}
}


long JNITTPushListener::GetMobileTx()
{
	CJniEnvUtil	env(gJVM);
	JNIEnv* penv = env.getEnv();
	jlong ret = 0;

	if (penv)
	{
		ret = penv->CallStaticLongMethod(mTrafficStats, mTx, NULL);
	}
	return ret;
}

void JNITTPushListener::NotifyEvent(TTPushNotifyMsg aMsg, TTInt aArg1, TTInt aArg2)
{
	CJniEnvUtil	env(gJVM);
	JNIEnv* penv = env.getEnv();
	if (penv)
	{
		penv->CallStaticVoidMethod(mClass, post_event, mObject, aMsg, aArg1, aArg2);
		LOGI("PlayerNotifyEvent to java, msgID = %d", (int)aMsg);
	}
}


typedef struct MediaPara
{
	//jmethodID			 post_event;
	CTTRtmpPush* 	     iPush;
	RTTCritical			 iCriticalVisual;

	JNIEnv*				iCmdEnvPtr;
	JNITTPushListener*	iListener;

	MediaPara()
	{
		iPush = NULL;
		iCmdEnvPtr  = NULL;
		iCriticalVisual.Create();
		iListener = NULL;
	}

	~MediaPara()
	{
		iCriticalVisual.Destroy();
		SAFE_DELETE(iListener);
	}

} MediaPara ;


void NotifyDecThreadAttach()
{
	//gJVM->AttachCurrentThread(&gDecThreadEnvPtr, NULL);
}

void NotifyDecThreadDetach()
{
	gJVM->DetachCurrentThread();
}


//static void alimusic_rtmp_native_init(JNIEnv* env)
//{
//}

static void alimusic_rtmp_native_setup(JNIEnv* env, jobject thiz, jobject weak_this,jint type) {
	LOGI("rtmp native_setup");

	MediaPara* pMediaPara = new MediaPara;
	pMediaPara->iCmdEnvPtr = env;
	env->GetJavaVM(&gJVM);

	jclass className = env->FindClass(kClassPathName);
	jmethodID postEvent = env->GetStaticMethodID(className, "postEventFromNative", "(Ljava/lang/Object;III)V");

   	JNITTPushListener* Listener = new JNITTPushListener(thiz, weak_this, postEvent, env);
    CTTRtmpPush* pPush = new CTTRtmpPush(Listener);

	pPush->SetCollectionType(type);

	pMediaPara->iListener = Listener;
	pMediaPara->iPush = pPush;

	jfieldID MediaPlayPara = env->GetFieldID(className,kClassFieldName, "I"); 
	env->SetIntField(thiz,MediaPlayPara, (int)pMediaPara);

    if (pPush == NULL) {
        LOGI("Create Push Error");
    }
}

static void alimusic_rtmp_native_release(JNIEnv* env, jobject thiz, jint context)
{
	LOGI("native_release");
	//MediaPara* pMediaPara = (MediaPara* )context;
	//if (pMediaPara == NULL)
	//{
	//	return;
	//}
 //
	//if (pMediaPara->iPush != NULL)
	//{
	//	pMediaPara->iPush->Stop();//?
	//	pMediaPara->iPush->Release();
	//	pMediaPara->iPush = NULL;
	//}
	//SAFE_DELETE(pMediaPara);

	//LOGI("native_release Finish");
}

static int alimusic_rtmp_startpublish_native(JNIEnv* env, jobject thiz, jint context, jstring uri)
{
	LOGI("native_startpublish");
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return -1;
	}

	int nErr = -1;
	if (pMediaPara->iPush != NULL && uri != NULL)
	{
		const char *uriStr = env->GetStringUTFChars(uri, NULL);
		nErr = pMediaPara->iPush->SetPublishSource(uriStr);
		env->ReleaseStringUTFChars(uri, uriStr);
	}

	return nErr;
}


static void alimusic_rtmp_stop_native(JNIEnv* env, jobject thiz, jint context)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return;
	}

	if (pMediaPara->iPush != NULL)
	{ 
		int ret = pMediaPara->iPush->Stop();
		if (ret == 0){
			LOGE("_stop_native");
			pMediaPara->iPush->Release();
			SAFE_DELETE(pMediaPara);
		}
	}
	else
	{
		LOGI("Player Not Existed");
	}

}

//static int alimusic_rtmp_getpublishstatus_native(JNIEnv* env, jobject thiz, jint context)
//{
//	MediaPara* pMediaPara = (MediaPara* )context;
//	int nPos = 0;
//	if (pMediaPara == NULL)
//	{
//		return -1;
//	}
//
//	if (pMediaPara->iPush != NULL)
//	{
//		nPos = pMediaPara->iPush->GetStatus();
//	}
//	else
//	{
//		LOGI("Push Not Existed");
//	}
//
//	return nPos;
//}

static int alimusic_rtmp_sendvideopacket(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size, jlong pts)
{
	//LOGE("Video, size = %d",size);
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->TransferVdieoData((TTPBYTE)arrayBody, size, pts);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}

static int alimusic_rtmp_sendviderawdata(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size, jlong pts,jint rotatetype)
{
	//LOGE("Video, size = %d",size);
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->TransferVdieoRawData((TTPBYTE)arrayBody, size, pts,rotatetype);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}

static int alimusic_rtmp_sendaudiopacket(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size, jlong pts)
{
	//LOGE("Audio, size = %d",size);
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->TransferAudioData((TTPBYTE)arrayBody, size, pts);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}

static int alimusic_rtmp_videoconfig(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size)
{
	//LOGE("Audio, size = %d",size);
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->SetVideoConfig((TTPBYTE)arrayBody, size);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}



static int alimusic_rtmp_audioconfig(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size)
{
	LOGI("audioconfig");
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->SetAudioConfig((TTPBYTE)arrayBody, size);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}

static void alimusic_rtmp_videoEncoderInit(JNIEnv* env, jobject thiz, jint context)
{
	LOGE("videoEncoderInit");
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return;
	}

	if (pMediaPara->iPush != NULL)
	{
		pMediaPara->iPush->VideoEncoderInit();
	}
	else
	{
		LOGI("iPush Not Existed");
	}
}

static void alimusic_rtmp_setvideoFpsBitrate(JNIEnv* env, jobject thiz, jint context,jint Fps,jint Bitrate)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return;
	}

	if (pMediaPara->iPush != NULL)
	{
		pMediaPara->iPush->SetVideoFpsBitrate(Fps,Bitrate);
	}
	else
	{
		LOGI("iPush Not Existed");
	}
}

static void alimusic_rtmp_setvideoWidthHeight(JNIEnv* env, jobject thiz, jint context,jint width,jint height)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return;
	}

	if (pMediaPara->iPush != NULL)
	{
		pMediaPara->iPush->SetVideoWidthHeight(width,height);
	}
	else
	{
		LOGI("iPush Not Existed");
	}
}

static void alimusic_rtmp_getlastpic(JNIEnv* env, jobject thiz, jint context, jintArray aArray,jint size)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	jint * arrayBody = env->GetIntArrayElements(aArray,0); 

	//int nArraySize = env->GetArrayLength(aArray);

	pMediaPara->iPush->GetLastPic((int*)arrayBody, size);
	env-> ReleaseIntArrayElements(aArray, arrayBody, 0);

	return ;
}

static void alimusic_rtmp_handlerawdata(JNIEnv* env, jobject thiz, jint context, jbyteArray asrcArray,jbyteArray adstArray,jint with,jint heith,jint type)
{
	//LOGE("Audio, size = %d",size);

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	jbyte * arraySrcBody = env->GetByteArrayElements(asrcArray,0);

	jbyte * arrayDstBody = env->GetByteArrayElements(adstArray,0);

	pMediaPara->iPush->HandleRawdata((TTPBYTE)arraySrcBody, (TTPBYTE)arrayDstBody,with,heith,type);

	env-> ReleaseByteArrayElements(asrcArray, arraySrcBody, 0);
	env-> ReleaseByteArrayElements(adstArray, arrayDstBody, 0);

	return ;
}

static void alimusic_rtmp_setcolorformat(JNIEnv* env, jobject thiz, jint context, jint color)
{
	//LOGE("Audio, size = %d",size);

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	pMediaPara->iPush->SetColorFormat(color);

	return ;
}

static void alimusic_rtmp_setBitrate(JNIEnv* env, jobject thiz, jint context, jint bitrate)
{
	//LOGE("Audio, size = %d",size);

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	pMediaPara->iPush->Setbitrate(bitrate);

	return ;
}

static void alimusic_rtmp_recordstart(JNIEnv* env, jobject thiz, jint context, jint samplerate,jint channel)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	pMediaPara->iPush->RecordStart(samplerate,channel);

	return ;
}

static void alimusic_rtmp_recordpause(JNIEnv* env, jobject thiz, jint context)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	pMediaPara->iPush->RecordPause();

	return ;
}

static void alimusic_rtmp_recordresume(JNIEnv* env, jobject thiz, jint context)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return ;
	}

	pMediaPara->iPush->RecordResume();

	return ;
}

static void alimusic_rtmp_recordclose(JNIEnv* env, jobject thiz, jint context)
{
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{ 
		return ;
	}

	pMediaPara->iPush->RecordClose();
	pMediaPara->iPush->Release();
	SAFE_DELETE(pMediaPara);

	return ;
}

static int alimusic_rtmp_sendauidorawdata(JNIEnv* env, jobject thiz, jint context, jbyteArray aArray,jint size, jlong pts)
{
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	}

	jbyte * arrayBody = env->GetByteArrayElements(aArray,0); 

	pMediaPara->iPush->TransferAudioRawData((TTPBYTE)arrayBody, size, pts);
	env-> ReleaseByteArrayElements(aArray, arrayBody, 0);

	return 0;
}

static int alimusic_rtmp_setfilepath(JNIEnv* env, jobject thiz, jint context, jstring uri)
{
	//LOGI("native_startpublish");
	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL)
	{
		return -1;
	}

	int nErr = -1;
	if (pMediaPara->iPush != NULL && uri != NULL)
	{
		const char *uriStr = env->GetStringUTFChars(uri, NULL);
		pMediaPara->iPush->SetFilePath(uriStr);
		env->ReleaseStringUTFChars(uri, uriStr);
	}

	return nErr;
}

static jint alimusic_CreateOpenGLNativeStatic(JNIEnv * env,
											  jobject,
											  jlong context,
											  jint width,
											  jint height)
{
	NativeOpenGl2Proxy* renderChannel =
		reinterpret_cast<NativeOpenGl2Proxy*> (context);

	return renderChannel->CreateOpenGLNative(width, height);
}

static void alimusic_DrawNativeStatic(JNIEnv * env,jobject, jlong context)
{
	NativeOpenGl2Proxy* renderChannel =
		reinterpret_cast<NativeOpenGl2Proxy*>(context);
	if (renderChannel)
		renderChannel->DrawNative();
}


static int alimusic_GenerateGLRender(JNIEnv* env, jobject thiz, jint context, jobject glSurface)
{
	LOGE(" alimusic_GenerateGLRender ");
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	} 

	NativeOpenGl2Proxy* Proxy = new NativeOpenGl2Proxy(gJVM, glSurface);
	if (Proxy->Init() != 0)
	{
		LOGE("³õÊ¼»¯Ê§°Ü.");
		return -1;
	}

	pMediaPara->iPush->SetRenderProxy(Proxy);

	return 0;
}

static long alimusic_GetRecordTime(JNIEnv* env, jobject thiz, jint context)
{
	//LOGE(" alimusic_GenerateGLRender ");
	int nErr = -1;

	MediaPara* pMediaPara = (MediaPara* )context;
	if (pMediaPara == NULL || pMediaPara->iPush == NULL)
	{
		return -1;
	} 

	return pMediaPara->iPush->GetRecordTime();
}

static JNINativeMethod gMethods[] = {
    {"nativeSetup",        "(Ljava/lang/Object;I)V",    (void *)alimusic_rtmp_native_setup},
    {"nativeRelease",      "(I)V",            		  (void *)alimusic_rtmp_native_release},
	{"nativeStartPublish", 	"(ILjava/lang/String;)I", (void *)alimusic_rtmp_startpublish_native},
	{"nativeStop",      	 "(I)V",       			  (void *)alimusic_rtmp_stop_native},
	//{"nativeGetPublishStatus",  "(I)I",    (void *)alimusic_rtmp_getpublishstatus_native},
	{"nativeSendVideoPacket", 	"(I[BIJ)I", (void *)alimusic_rtmp_sendvideopacket},
	{"nativeSendVideoRawData", 	"(I[BIJI)I", (void *)alimusic_rtmp_sendviderawdata},
	{"nativeSendAudioPacket" ,  "(I[BIJ)I", (void *)alimusic_rtmp_sendaudiopacket},
	{"nativeSendAudioConfig" ,  "(I[BI)I", (void *)alimusic_rtmp_audioconfig},
	{"nativeSendVideoConfig" ,  "(I[BI)I", (void *)alimusic_rtmp_videoconfig},
	{"nativeSetWidthHeight" ,  "(III)V", (void *)alimusic_rtmp_setvideoWidthHeight},
	{"nativeSetFpsBitrate" ,  "(III)V", (void *)alimusic_rtmp_setvideoFpsBitrate},
	{"nativeVideoEncoderInit" ,  "(I)V", (void *)alimusic_rtmp_videoEncoderInit},
	{"nativeGetLastPic", 	"(I[II)V", (void *)alimusic_rtmp_getlastpic},
	{"nativeHandleRawData", 	"(I[B[BIII)V", (void *)alimusic_rtmp_handlerawdata},
	{"nativeSetcolorFormart", 	"(II)V", (void *)alimusic_rtmp_setcolorformat},
	{"nativeSetBitrate" ,  "(II)V", (void *)alimusic_rtmp_setBitrate},

	{"nativeRecordStart" ,  "(III)V", (void *)alimusic_rtmp_recordstart},
	{"nativeRecordPause" ,  "(I)V", (void *)alimusic_rtmp_recordpause},
	{"nativeRecordResume" ,  "(I)V", (void *)alimusic_rtmp_recordresume},
	{"nativeRecordClose" ,  "(I)V", (void *)alimusic_rtmp_recordclose},
	{"nativeSendAudioRawData", 	"(I[BIJ)I", (void *)alimusic_rtmp_sendauidorawdata},
	{"nativeFilePath", 	"(ILjava/lang/String;)I", (void *)alimusic_rtmp_setfilepath},
	{"DrawNative",			"(J)V",      (void*)alimusic_DrawNativeStatic },
	{"CreateOpenGLNative",	"(JII)I", (void*)alimusic_CreateOpenGLNativeStatic},
	{"GenerateGLRenderNative",	"(ILjava/lang/Object;)V", (void*)alimusic_GenerateGLRender},
	{"nativeRecordTime" ,  "(I)J", (void *)alimusic_GetRecordTime},
	 
};

// ----------------------------------------------------------------------------

// This function only registers the native methods
static int register_alimusic_media_RtmpPush(JNIEnv *env)
{
	jclass className = env->FindClass(kClassPathName);
	if (className == NULL) {
		LOGE("Can't find %s\n", kClassPathName);
		return -1;
	}

	if (env->RegisterNatives(className, gMethods, sizeof(gMethods)
			/ sizeof(gMethods[0])) != JNI_OK) {
		LOGE("ERROR: Register mediaplayer jni methods failed\n");
		env->DeleteLocalRef(className);
		return -1;
	}

	env->DeleteLocalRef(className);
	LOGI("register %s succeed\n", kClassPathName);
	return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    LOGI("MediaPlayer: JNI OnLoad\n");
#ifdef JNI_VERSION_1_6
    if (result==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_6\n");
   	    result = JNI_VERSION_1_6;
    }
#endif
#ifdef JNI_VERSION_1_4
    if (result==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_4\n");
   	    result = JNI_VERSION_1_4;
    }
#endif
#ifdef JNI_VERSION_1_2
    if (result==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_2) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_2\n");
   	    result = JNI_VERSION_1_2;
    }
#endif
	
	if(result == -1)
		return result;

    if (register_alimusic_media_RtmpPush(env) < 0) {
        LOGE("ERROR: MediaPlayer native registration failed\n");
        return -1;    
	}

    return result;
}
