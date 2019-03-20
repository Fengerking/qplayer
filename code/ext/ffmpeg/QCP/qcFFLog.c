/*******************************************************************************
	File:		qcCodec.cpp

	Contains:	Create the Codec with codec id

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-02-24		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"

#include "qcFFLog.h"
#include "libavcodec/avcodec.h"

int		g_nQcCodecLogLevel = 4;
char	g_szFFLogText[1024];
char	g_szFFOutText[1024];

void qclog_print(void *ptr, int level, const char *fmt, va_list vl)
{
	strcpy(g_szFFLogText, "");
	switch (level)
	{
	 case AV_LOG_DEBUG:
//		FFLOGI(FFLOG_TAG, fmt, vl);
		return;
	 case AV_LOG_VERBOSE:
//		FFLOGI(FFLOG_TAG, fmt, vl);
		return;
	 case AV_LOG_INFO:
		 if (g_nQcCodecLogLevel >= 3)
			FFLOGI(FFLOG_TAG, fmt, vl);
		break;		 
	 case AV_LOG_WARNING:
		 if (g_nQcCodecLogLevel >= 2)
			 FFLOGW(FFLOG_TAG, fmt, vl);
		break;	 
	 case AV_LOG_ERROR:
		 if (g_nQcCodecLogLevel >= 1)
			 FFLOGE(FFLOG_TAG, fmt, vl);
		break;
	}
	if (strlen (g_szFFLogText) <= 0)
		return;

#ifdef __QC_OS_WIN32__
	sprintf(g_szFFOutText, "@@@QCODEC LEV %d %s", level, g_szFFLogText);
	OutputDebugString(g_szFFOutText);
#elif defined __QC_OS_IOS__
    snprintf(g_szFFOutText, 1024, "@@@QCODEC LEV %d %s\n", level, g_szFFLogText);
    printf(g_szFFOutText);
#endif // WIN32
}

void qclog_init (void)
{
	av_log_set_callback(qclog_print);
	av_log_set_level(AV_LOG_INFO);
}

void qclog_uninit(void)
{
	av_log_set_callback(NULL);
}