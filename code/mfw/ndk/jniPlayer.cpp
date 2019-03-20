/*******************************************************************************
	File:		jniPlayer.cpp

	Contains:	qplayer jni engine  implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-15		Bangfei			Create file

*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <jni.h>

#include "CNDKPlayer.h"
#include "CTestMng.h"
#include "CTestInst.h"

#include "ULogFunc.h"

#define 	g_szMediaPlayerName "com/qiniu/qplayer/mediaEngine/MediaPlayer"

extern "C" const char SO_FILE_VERSION[]  __attribute__ ((section (".qplayer_version"))) = "<1.1.0.73>";

CTestMng *	g_pTestMng = NULL;

// Java Bindings
static jlong native_init (JNIEnv* env, jobject obj, jobject player, jstring apkPath, jint nFlag) 
{
	JavaVM * 	jvm = 0;
	jobject 	envobj;

	env->GetJavaVM(&jvm);
	jclass clazz = env->GetObjectClass(obj);
	jclass clsPlayer = (jclass)env->NewGlobalRef(clazz);
	jobject objPlayer  = env->NewGlobalRef(player);	
	char * strPath = (char *) env->GetStringUTFChars(apkPath, NULL);

	if (nFlag == 0X80000000)	
	{
		g_pTestMng = new CTestMng ();	
		CTestInst * pInst = g_pTestMng->GetInst ();
		pInst->m_pENV = env;	
		env->GetJavaVM(&pInst->m_pJVM);	
		//pInst->m_pRndAudio = new COpenSLESRnd (NULL, NULL);
		pInst->m_pRndAudio = new CNDKAudioRnd (NULL, NULL);
		pInst->m_pRndAudio->SetNDK (jvm, env, clsPlayer, objPlayer);		
		pInst->m_pRndVideo = new CNDKVideoRnd (NULL, NULL);	
		pInst->m_pRndVideo->SetNDK (jvm, env, clsPlayer, objPlayer);
		pInst->m_pRndVidDec = new CNDKVDecRnd (NULL, NULL);	
		pInst->m_pRndVidDec->SetNDK (jvm, env, clsPlayer, objPlayer, 6);	
		env->DeleteLocalRef(clazz);
		return (long)g_pTestMng;				
	}
	else
	{
		CNDKPlayer * pPlayer = new CNDKPlayer ();
		int nRC = pPlayer->Init(jvm, env, clsPlayer, objPlayer, strPath, nFlag);
		env->ReleaseStringUTFChars(apkPath, strPath);	
		if (nRC) 
		{
			delete pPlayer;
			pPlayer = NULL;
		}
		env->DeleteLocalRef(clazz);
		return (long)pPlayer;		
	}
}

static jint native_uninit(JNIEnv* env, jobject obj,jlong nNativeContext) 
{	
	if (nNativeContext == (long)g_pTestMng)
	{
		delete g_pTestMng;
		g_pTestMng = NULL;	
		return 0;	
	}

	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	pPlayer->Uninit(env);
	delete pPlayer;
	QCLOGT ("jniPlayer", "It was Safe to exit ndk player!");
	return 0;
}

static jint native_setView (JNIEnv* env, jobject obj,jlong nNativeContext, jobject view) 
{
	// for auto test
	if (nNativeContext == (long)g_pTestMng)
	{
		QCLOGT ("qcAutotest", "SetView: %p", view);		
		CTestInst * pInst = g_pTestMng->GetInst ();
		if (pInst->m_pRndVideo != NULL)
			pInst->m_pRndVideo->SetSurface (env, view);
		if (pInst->m_pRndVidDec != NULL)
			pInst->m_pRndVidDec->SetSurface (env, view);			
		return 0;
	}

	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	pPlayer->SetView(env, view);	
	return 0;
}

static jint native_open (JNIEnv* env, jobject obj,jlong nNativeContext, jstring strUrl, jint flag) 
{		
	// for auto test
	if (nNativeContext == (long)g_pTestMng)
	{
		const char* pTestFile = env->GetStringUTFChars(strUrl, NULL);
		QCLOGT ("qcAutotest", "Open test file: %s", pTestFile);
		g_pTestMng->OpenTestFile (pTestFile);	
		env->ReleaseStringUTFChars(strUrl, pTestFile);
		return 0;
	}

	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	int nRC = 0;
	if (strUrl != NULL) 
	{
		const char* pSource = env->GetStringUTFChars(strUrl, NULL);
		QCLOGT ("jniPlayer", "Open source: %s", pSource);
		
		nRC = pPlayer->Open (pSource, flag);
		env->ReleaseStringUTFChars(strUrl, pSource);
	} 
	return nRC;
}

static jint native_play (JNIEnv* env, jobject obj,jlong nNativeContext) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	return pPlayer->Play ();
}

static jint native_pause(JNIEnv* env, jobject obj,jlong nNativeContext) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	return pPlayer->Pause();
}

static jint native_stop (JNIEnv* env, jobject obj,jlong nNativeContext) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;

	return pPlayer->Stop();
}

static jlong native_getpos(JNIEnv* env, jobject obj,jlong nNativeContext) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	long long llCurPos = 0;
	int nRC = pPlayer->GetPos(&llCurPos);
	if (nRC)
		return 0;
	return llCurPos;
}

static jint native_setpos(JNIEnv* env, jobject obj,jlong nNativeContext, jlong pos) 
{		
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	int nPos = pos;
	return pPlayer->SetPos(nPos);
}

static jlong native_getduration(JNIEnv* env, jobject obj,jlong nNativeContext) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;
	
	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;

	return pPlayer->GetDuration();
}

static jint native_getparam(JNIEnv* env, jobject obj,jlong nNativeContext, jint nId, int nParam, jobject objParam) 
{
	if (nNativeContext == (long)g_pTestMng)	
		return 0;

	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	int nRC = pPlayer->GetParam(env, nId, nParam, objParam);
	return nRC;
}

static jint native_setparam(JNIEnv* env, jobject obj,jlong nNativeContext, jint nId, jint nParam, jobject objParam)
{	
	if (nNativeContext == (long)g_pTestMng)
	{
		if (nId == 0X100)//PARAM_PID_EVENT_DONE)
		{
			CTestInst * pInst = g_pTestMng->GetInst ();
			if (pInst->m_pRndVideo != NULL)
				pInst->m_pRndVideo->SetEventDone (true);
			if (pInst->m_pRndVidDec != NULL)
				pInst->m_pRndVidDec->SetEventDone (true);				
		}	
		return 0;
	}

	CNDKPlayer* pPlayer = (CNDKPlayer*) nNativeContext;
	if (pPlayer == NULL)
		return -1;
	int nRC = pPlayer->SetParam(env, nId, nParam, objParam);
	return nRC;
}

static JNINativeMethod native_methods[] =
{
{ "nativeInit","(Ljava/lang/Object;Ljava/lang/String;I)J",(void *) native_init },
{ "nativeUninit","(J)I",(void *) native_uninit },
{ "nativeSetView","(JLjava/lang/Object;)I",(void *) native_setView },
{ "nativeOpen","(JLjava/lang/String;I)I", (void *) native_open },
{ "nativePlay", "(J)I", (void *) native_play },
{ "nativePause", "(J)I", (void *) native_pause },
{ "nativeStop", "(J)I", (void *) native_stop },
{ "nativeGetPos", "(J)J", (void *) native_getpos },
{ "nativeSetPos", "(JJ)I", (void *) native_setpos },
{ "nativeGetDuration", "(J)J", (void *) native_getduration },
{ "nativeGetParam", "(JIILjava/lang/Object;)I",(void *) native_getparam },
{ "nativeSetParam", "(JIILjava/lang/Object;)I", (void *) native_setparam },
};

jint JNI_OnLoad (JavaVM *vm, void *reserved) 
{
	JNIEnv *env = NULL;
	jint jniver = JNI_VERSION_1_4;

	if (vm->GetEnv((void**) &env, jniver) != JNI_OK) 
	{
		jniver = JNI_VERSION_1_6;
		if (vm->GetEnv((void**) &env, jniver) != JNI_OK) 
		{
			QCLOGT ("jniPlayer", "It can't get env pointer!!!");
			return 0;
		}
	}
	jclass klass = env->FindClass(g_szMediaPlayerName);
	env->RegisterNatives(klass, native_methods, sizeof(native_methods) / sizeof(native_methods[0]));
	return jniver;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{

}