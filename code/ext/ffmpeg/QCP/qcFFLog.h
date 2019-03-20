/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#ifndef __qcFFLog_H__
#define __qcFFLog_H__

#ifdef __QC_OS_NDK__
#include <unistd.h>
#include <android/log.h>
#endif // __QC_OS_NDK__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __QC_OS_WIN32__
#include "windows.h"
#endif // __QC_OS_WIN32__

extern int		g_nQcCodecLogLevel;

#define  FFLOG_TAG "@@@QCCODEC"

#ifdef __QC_OS_WIN32__
#define FFLOGI(TAG, ...)	((void)sprintf(g_szFFLogText, __VA_ARGS__))
#define FFLOGW(TAG, ...)	((void)sprintf(g_szFFLogText, __VA_ARGS__))
#define FFLOGE(TAG, ...)	((void)sprintf(g_szFFLogText, __VA_ARGS__))
#elif defined __QC_OS_NDK__
#define FFLOGE(TAG, ...) 	((void)__android_log_vprint(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
#define FFLOGW(TAG, ...) 	((void)__android_log_vprint(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define FFLOGI(TAG, ...) 	((void)__android_log_vprint(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#elif defined __QC_OS_IOS__
#define FFLOGI(TAG, ...)	((void)vprintf(__VA_ARGS__))
#define FFLOGW(TAG, ...)	((void)vprintf(__VA_ARGS__))
#define FFLOGE(TAG, ...)	((void)vprintf(__VA_ARGS__))
#endif // __QC_OS_WIN32__

extern char	g_szFFLogText[1024];
extern char	g_szFFOutText[1024];

void	qclog_init (void);
void	qclog_uninit(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // __qcFFLog_H__
