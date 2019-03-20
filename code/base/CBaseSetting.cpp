/*******************************************************************************
	File:		CBaseSetting.cpp

	Contains:	The buff trace implement file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-06-05		Bangfei			Create file

*******************************************************************************/
#include "qcErr.h"
#include "CBaseSetting.h"

#include "USystemFunc.h"

CBaseSetting::CBaseSetting(void)
{
	g_qcs_nTimeOutConnect = 5000;
	g_qcs_nTimeOutRead = 500;

	g_qcs_nMaxPlayBuffTime = 2000;
	g_qcs_nMinPlayBuffTime = 500;

	g_qcs_nMaxSaveLiveBuffTime = 30000;
	g_qcs_nMaxSaveVodeBuffTime = 15000;
	g_qcs_nMaxSaveLcleBuffTime = 10000;

	g_qcs_szHttpHeadReferer = new char[2048];
	memset(g_qcs_szHttpHeadReferer, 0, 2048);

    g_qcs_szHttpHeadUserAgent = new char[2048];
    memset(g_qcs_szHttpHeadUserAgent, 0, 2048);

//	strcpy(g_qcs_szDNSServerName, "223.6.6.6");
	strcpy(g_qcs_szDNSServerName, "0.0.0.0");

	g_qcs_nPerferFileForamt = QC_PARSER_NONE;

	g_qcs_nPerferIOProtocol = QC_IOPROTOCOL_NONE;

	g_qcs_szPDFileCachePath = new char[2048];
	memset(g_qcs_szPDFileCachePath, 0, 2048);
	qcGetAppPath (NULL, g_qcs_szPDFileCachePath, 1024);
	strcat(g_qcs_szPDFileCachePath, "PDFileCache/");

	strcpy(g_qcs_szPDFileCacheExtName, "mp4");
    
	//g_qcs_sBackgroundColor = {0.0, 0.0, 0.0, 1.0};
	g_qcs_sBackgroundColor.fRed = 0.0;
	g_qcs_sBackgroundColor.fGreen = 0.0;
	g_qcs_sBackgroundColor.fBlue = 0.0;
	g_qcs_sBackgroundColor.fAlpha = 1.0;
   
    g_qcs_nReconnectInterval = 5000;

	g_qcs_bRndTypeIsGDI = false;

	g_qcs_bIOReadError = false;

	g_qcs_nAudioVolume = 100;
	g_qcs_nAudioDecVlm = 0;

	g_qcs_nVideoZoomLeft = 0;
	g_qcs_nVideoZoomTop = 0;
	g_qcs_nVideoZoomWidth = 0;
	g_qcs_nVideoZoomHeight = 0;

	g_qcs_nVideoRotateAngle = 0;

	g_qcs_nRTSP_UDP_TCP_Mode = 1;

	g_qcs_nPlaybackLoop = 0;

	g_qcs_nMoovBoxSkipSize = 1024 * 2;
	g_qcs_nMoovPreLoadItem = 600;

	g_qcs_nIOCacheDownSize = 1024 * 10;

	memset(g_qcs_pFileKeyText, 0, sizeof(g_qcs_pFileKeyText));
	memset(g_qcs_pCompKeyText, 0, sizeof(g_qcs_pCompKeyText));

	g_qcs_nExtAITracking = 0;
}

CBaseSetting:: ~CBaseSetting(void)
{
	delete []g_qcs_szHttpHeadReferer;
    delete []g_qcs_szHttpHeadUserAgent;
	delete []g_qcs_szPDFileCachePath;
}


