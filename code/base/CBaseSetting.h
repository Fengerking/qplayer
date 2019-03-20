/*******************************************************************************
	File:		CBaseSetting.h

	Contains:	The buffer trace header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-05		Bangfei			Create file

*******************************************************************************/
#ifndef __CBaseSetting_H__
#define __CBaseSetting_H__

#include "qcType.h"
#include "qcData.h"

class CBaseSetting
{
public:
	CBaseSetting(void);
	virtual ~CBaseSetting(void);

public:
	// socket connect and read time out
	int		g_qcs_nTimeOutConnect;
	int		g_qcs_nTimeOutRead;

	// buffer mng play max and min butter time
	int		g_qcs_nMaxPlayBuffTime;
	int		g_qcs_nMinPlayBuffTime;

	// buffer mng save live and file max butter time
	int		g_qcs_nMaxSaveLiveBuffTime;
	int		g_qcs_nMaxSaveVodeBuffTime;
	int		g_qcs_nMaxSaveLcleBuffTime;

	char *	g_qcs_szHttpHeadReferer;
    char *  g_qcs_szHttpHeadUserAgent;

	char	g_qcs_szDNSServerName[64];

	int		g_qcs_nPerferFileForamt;

	int		g_qcs_nPerferIOProtocol;

	char *	g_qcs_szPDFileCachePath;
	char 	g_qcs_szPDFileCacheExtName[32];

    QC_COLOR	g_qcs_sBackgroundColor;

    int		g_qcs_nReconnectInterval;

	bool	g_qcs_bRndTypeIsGDI;

	bool	g_qcs_bIOReadError;

	int		g_qcs_nAudioVolume;
	int		g_qcs_nAudioDecVlm;

	int		g_qcs_nVideoZoomLeft;
	int		g_qcs_nVideoZoomTop;
	int		g_qcs_nVideoZoomWidth;
	int		g_qcs_nVideoZoomHeight;

	int		g_qcs_nVideoRotateAngle;

	int		g_qcs_nRTSP_UDP_TCP_Mode;

	int		g_qcs_nPlaybackLoop;

	int		g_qcs_nMoovBoxSkipSize;
	int		g_qcs_nMoovPreLoadItem;

	int		g_qcs_nIOCacheDownSize;

	char	g_qcs_pFileKeyText[128];
	char	g_qcs_pCompKeyText[128];

	int		g_qcs_nExtAITracking;
};





#endif // __CBaseSetting_H__
