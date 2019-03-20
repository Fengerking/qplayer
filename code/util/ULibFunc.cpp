/*******************************************************************************
	File:		ULibFunc.cpp

	Contains:	The utility for library implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-11		Bangfei			Create file

*******************************************************************************/
#include "stdlib.h"

#include "ULibFunc.h"
#include "USystemFunc.h"
#if defined __QC_OS_NDK__ || defined __QC_OS_IOS__
#include "dlfcn.h"
#endif // __QC_OS_NDK__


#if defined __QC_OS_IOS__
#include "ULibMng.h"
#endif

#include "ULogFunc.h"

#ifdef __QC_OS_WIN32__
#pragma warning (disable : 4996)
#endif // __QC_OS_WIN32__

void * qcLibLoad (const char * pLibName, int nFlag)
{
	void * hDll = NULL;
	char szLib[256];
	
#ifdef __QC_OS_WIN32__
    _tcscpy (szLib, pLibName);
	_tcscat (szLib, _T(".dll"));
	hDll = LoadLibrary (szLib);
#elif defined __QC_OS_NDK__
	strcpy (szLib, "lib");
	strcat (szLib, pLibName);
	strcat (szLib, ".so");
	hDll = dlopen (szLib, RTLD_NOW);
#elif defined __QC_OS_IOS__
    strcpy (szLib, "lib");
    strcat (szLib, pLibName);
    strcat (szLib, ".a");
    hDll = (void*)1;
#endif // __QC_OS_WIN32__
	if (hDll == NULL)
	{
#ifdef __QC_OS_WIN32__
		_tcscpy (szLib, g_szWorkPath);
		_tcscat (szLib, pLibName);
		_tcscat (szLib, _T(".dll"));
		hDll = LoadLibrary (szLib);
#elif defined __QC_OS_NDK__
	//	QCLOGT ("ULIBFunc", "Load %s failed! %s. Err: %s", pLibName, szLib, dlerror ());

		strcpy (szLib, g_szWorkPath);
		strcat (szLib, "lib");
		strcat (szLib, pLibName);
		strcat (szLib, ".so");
		hDll = dlopen (szLib, RTLD_NOW);
		if (hDll == NULL && nFlag == 1)
		{
			QCLOGT ("ULIBFunc", "Load %s failed! %s. Err: %s", pLibName, szLib, dlerror ());
			strcpy (szLib, "/system/lib/lib");
			strcat (szLib, pLibName);
			strcat (szLib, ".so");
			hDll = dlopen (szLib, RTLD_NOW);			
		}
#endif // _Os_WIN32
	}
	if (hDll == NULL)
	{
#ifdef __QC_OS_NDK__
		QCLOGT ("ULIBFunc", "Load %s failed! %s. Err: %s", pLibName, szLib, dlerror ());
#else
		QCLOGT ("ULIBFunc", "Load %s failed! %s", pLibName, szLib);
#endif // __QC_OS_NDK__
	}
	else
		QCLOGT ("ULIBFunc", "Load %s  %s. hLib =  %p", pLibName, szLib, hDll);	
	return hDll;
}

void * qcLibGetAddr (void * hLib, const char * pFuncName, int nFlag)
{
	void * hFunc = NULL;

#ifdef __QC_OS_WIN32__
	hFunc = GetProcAddress ((HMODULE)hLib, pFuncName);
#elif defined __QC_OS_NDK__
	hFunc = dlsym (hLib, pFuncName);
#elif defined __QC_OS_IOS__
    hFunc = qcStaticLibGetAddr(hLib, pFuncName, nFlag);
#endif // __QC_OS_WIN32__
	if (hFunc == NULL)
#ifdef __QC_OS_NDK__	
		QCLOGT ("ULibFunc",  "GetAddr %s Failed! Hlib = %p error is %s", pFuncName, hLib, dlerror ());
#else
		QCLOGT ("ULibFunc",  "GetAddr %s Failed! Hlib = %p ", pFuncName, hLib);
#endif // __QC_OS_NDK__
	return hFunc;
}

int qcLibFree (void * hLib, int nFlag)
{
#ifdef __QC_OS_WIN32__
	FreeLibrary ((HMODULE)hLib);
#elif defined __QC_OS_NDK__
	dlclose (hLib);
#endif // __QC_OS_WIN32__
	QCLOGT ("ULIBFunc", "Free lib =  %p", hLib);		
	return 0;
}
