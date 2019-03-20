/*******************************************************************************
	File:		ULogFunc.h

	Contains:	ULogFunc define header file

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-11-29		Bangfei			Create file

*******************************************************************************/
#ifndef __ULogFunc_H__
#define __ULogFunc_H__

#include <string.h>
#include <stdio.h>

#if defined __QC_OS_NDK__ || defined __QC_OS_IOS__
#include <pthread.h>
#endif // __QC_OS_NDK__

#include "qcDef.h"

#if defined __QC_OS_NDK__
#include <android/log.h>
#ifndef LOG_TAG
#define  LOG_TAG "@@@QCLOG"
#endif // LOG_TAG
#if !defined LOGW
#define LOGW(...) ((int)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif
#if !defined LOGI
#define LOGI(...) ((int)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif
#if !defined LOGE
#define LOGE(...) ((int)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif
#endif //__QC_OS_NDK__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    
#include "UThreadFunc.h"

extern int	g_nLogOutLevel;

#define QC_LOG_LEVEL_DUMP       5
#define	QC_LOG_LEVEL_DEBUG		4
#define	QC_LOG_LEVEL_INFO		3
#define	QC_LOG_LEVEL_WARNING	2
#define	QC_LOG_LEVEL_ERR		1
#define	QC_LOG_LEVEL_NONE		0

void	qcDisplayLog (const char * pLogText);
void	qcDisplayMsg (void * hWnd, const char * pLogText);
void 	qcDumpLog(const char * pLogText);
void	qcCheckLogCache();
    
#define _QCLOG_ERROR   1
#define _QCLOG_WARNING 1
#define _QCLOG_INFO    1
#define _QCLOG_TEXT    1

#ifdef _QCLOG_ERROR
#ifdef __QC_OS_WIN32__
#define QCLOGE(fmt, ...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_ERR) \
{ \
	char		szLogText[1024]; \
	sprintf (szLogText, "@@@QCLOG Err T%08X %s L%d " fmt "\r\n", GetCurrentThreadId(), m_szObjName, __LINE__, __VA_ARGS__); \
	qcDisplayLog (szLogText); \
}
#elif defined __QC_OS_NDK__
#define QCLOGE(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_ERR) \
{ \
	LOGE ("Err  T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
	if (g_nLogOutLevel >= QC_LOG_LEVEL_DUMP) \
	{	\
    	char        szLogText[1024]; \
    	snprintf(szLogText, 1023, "Err T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
    	qcDumpLog(szLogText); \
	}	\
}
#elif defined __QC_OS_IOS__
#define QCLOGE(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_ERR) \
{ \
	char        szLogText[1024]; \
	snprintf(szLogText, 1023, "Err T%08X %s L%d " fmt "\r\n", qcThreadGetCurrentID(), m_szObjName, __LINE__, ## args); \
	qcDisplayLog (szLogText); \
}
#endif // __QC_OS_WIN32__
#else
#define QCLOGE(fmt, ...)
#endif // _QCLOG_ERROR

#ifdef _QCLOG_WARNING
#ifdef __QC_OS_WIN32__
#define QCLOGW(fmt, ...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_WARNING) \
{ \
	char		szLogText[1024]; \
	sprintf (szLogText, "@@@QCLOG Warn  T%08X %s L%d " fmt "\r\n", GetCurrentThreadId(), m_szObjName, __LINE__, __VA_ARGS__); \
	qcDisplayLog (szLogText); \
}
#elif defined __QC_OS_NDK__
#define QCLOGW(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_WARNING) \
{ \
	LOGW ("Warn T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
	if (g_nLogOutLevel >= QC_LOG_LEVEL_DUMP) \
	{    \
		char        szLogText[1024]; \
		snprintf(szLogText, 1023, "Warn T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
		qcDumpLog(szLogText); \
	}	\
}
#elif defined __QC_OS_IOS__
#define QCLOGW(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_WARNING) \
{ \
	char        szLogText[1024]; \
	snprintf(szLogText, 1023, "Warn T%08X %s L%d " fmt "\r\n", qcThreadGetCurrentID(), m_szObjName, __LINE__, ## args); \
	qcDisplayLog (szLogText); \
}
#endif // __QC_OS_WIN32__
#else
#define QCLOGW(fmt, ...)
#endif // _QCLOG_WARNING

#ifdef _QCLOG_INFO
#ifdef __QC_OS_WIN32__
#define QCLOGI(fmt, ...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	char		szLogText[1024]; \
	_snprintf(szLogText, 1023, "@@@QCLOG Info T%08X %s L%d " fmt "\r\n", GetCurrentThreadId(), m_szObjName, __LINE__, __VA_ARGS__); \
	qcDisplayLog (szLogText); \
}
#elif defined __QC_OS_NDK__
#define QCLOGI(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	LOGI ("Info T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
	if (g_nLogOutLevel >= QC_LOG_LEVEL_DUMP) \
	{    \
		char        szLogText[1024]; \
		snprintf(szLogText, 1023, "Info T%08X %s L%d " fmt "\r\n", (int)pthread_self(), m_szObjName, __LINE__, ## args); \
        qcDumpLog(szLogText); \
	}	\
}
#elif defined __QC_OS_IOS__
#define QCLOGI(fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
    char        szLogText[1024]; \
    snprintf(szLogText, 1023, "Info T%08X %s L%d " fmt "\r\n", qcThreadGetCurrentID(), m_szObjName, __LINE__, ## args); \
    qcDisplayLog (szLogText); \
}
#endif // __QC_OS_WIN32__
#else
#define QCLOGI(fmt, ...)
#endif // _QCLOG_INFO

#ifdef _QCLOG_TEXT
#ifdef __QC_OS_WIN32__
#define QCLOGT(txt, fmt, ...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	char		szLogText[1024]; \
	_snprintf (szLogText, 1023, "@@@QCLOG Info T%08X %s L%d " fmt "\r\n", GetCurrentThreadId(), txt, __LINE__, __VA_ARGS__); \
	qcDisplayLog (szLogText); \
}
#elif defined __QC_OS_NDK__
#define QCLOGT(txt, fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	LOGI ("Info T%08X %s L%d " fmt "\r\n", (int)pthread_self(), txt, __LINE__, ## args); \
	if (g_nLogOutLevel >= QC_LOG_LEVEL_DUMP) \
	{	\
		char        szLogText[1024]; \
		snprintf(szLogText, 1023, "Info T%08X %s L%d " fmt "\r\n", (int)pthread_self(), txt, __LINE__, ## args); \
		qcDumpLog(szLogText); \
	}	\
}
#elif defined __QC_OS_IOS__
#define QCLOGT(txt, fmt, args...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	char        szLogText[1024]; \
    snprintf(szLogText, 1024, "Info T%08X %s L%d " fmt "\r\n", qcThreadGetCurrentID(), txt, __LINE__, ## args); \
	qcDisplayLog (szLogText); \
}
#endif // __QC_OS_WIN32__
#else
#define QCLOGT(txt, fmt, ...)
#endif // _QCLOG_INFO

#ifdef __QC_OS_WIN32__
#define QCMSG(win, fmt, ...) \
if (g_nLogOutLevel >= QC_LOG_LEVEL_INFO) \
{ \
	char	szMsgText[1024]; \
	_snprintf (szMsgText, 1023, "QCMSG T%08X %s L%d " fmt "\r\n", GetCurrentThreadId(), m_szObjName, __LINE__, __VA_ARGS__); \
	qcDisplayMsg ((void *)win,szMsgText); \
}
#else
#define QCMSG(win, fmt, ...)
#endif // __QC_OS_WIN32__

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __ULogFunc_H__
