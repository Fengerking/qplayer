/*******************************************************************************
 File:        qcMuxer.cpp
 
 Contains:    Muxer base interface
 
 Written by:    Jun Lin
 
 Change History (most recent first):
 2018-01-06        Jun            Create file
 
 *******************************************************************************/
#ifdef __QC_OS_WIN32__
#ifdef __QC_LIB_FFMPEG__
#include "./libWin32/stdafx.h"
#endif // __QC_LIB_FFMPEG__
#endif // __QC_OS_WIN32__

#include "qcMuxer.h"
#include "qcErr.h"

#ifdef __QC_OS_NDK__
#include <unistd.h>
#include <android/log.h>
#endif // __QC_OS_NDK__

#if defined(__QC_OS_IOS__) || defined(__QC_OS_MACOS__)
#endif

#include "qcFFLog.h"

#ifdef __QC_LIB_FFMPEG__
#include "qcFFWrap.h"
int ffMuxerOpen(void * hMuxer, const char * pURL)
{
    if(!hMuxer)
        return QC_ERR_EMPTYPOINTOR;
	QCMuxInfo * pMuxInfo = (QCMuxInfo *)hMuxer;
	QC_DEL_A(pMuxInfo->m_pFileName);
	pMuxInfo->m_pFileName = new char[strlen(pURL) + 1];
	strcpy(pMuxInfo->m_pFileName, pURL);
	return ffMux_Open(pMuxInfo);
}

int ffMuxerClose(void * hMuxer)
{
    if(!hMuxer)
        return QC_ERR_EMPTYPOINTOR;
	QCMuxInfo * pMuxInfo = (QCMuxInfo *)hMuxer;
	QC_DEL_A(pMuxInfo->m_pFileName);
	return ffMux_Close(pMuxInfo);
}

int ffMuxerInit(void * hMuxer, void * pVideoFmt, void * pAudioFmt)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	QCMuxInfo * pMuxInfo = (QCMuxInfo *)hMuxer;
	if (pVideoFmt != NULL)
		ffMux_AddTrack(pMuxInfo, pVideoFmt, QC_MEDIA_Video);
	if (pAudioFmt != NULL)
		ffMux_AddTrack(pMuxInfo, pAudioFmt, QC_MEDIA_Audio);
	return QC_ERR_NONE;
}

int ffMuxerWrite(void * hMuxer, QC_DATA_BUFF* pBuffer)
{
    if(!hMuxer)
        return QC_ERR_EMPTYPOINTOR;
	QCMuxInfo * pMuxInfo = (QCMuxInfo *)hMuxer;
	return ffMux_Write(pMuxInfo, pBuffer);
}

int ffMuxerGetParam(void * hMuxer, int nID, void * pParam)
{
    if(!hMuxer)
        return QC_ERR_EMPTYPOINTOR;
	return QC_ERR_IO_AGAIN;
}

int ffMuxerSetParam(void * hMuxer, int nID, void * pParam)
{
    if(!hMuxer)
        return QC_ERR_EMPTYPOINTOR;
	return QC_ERR_IO_AGAIN;
}
#else
#include "CBaseFFMuxer.h"
#define FFMUXERINST ((CBaseFFMuxer*)hMuxer)
int ffMuxerOpen(void * hMuxer, const char * pURL)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->Open(pURL);
}

int ffMuxerClose(void * hMuxer)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->Close();
}

int ffMuxerInit(void * hMuxer, void * pVideoFmt, void * pAudioFmt)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->Init(pVideoFmt, pAudioFmt);
}

int ffMuxerWrite(void * hMuxer, QC_DATA_BUFF* pBuffer)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->Write(pBuffer);
}

int ffMuxerGetParam(void * hMuxer, int nID, void * pParam)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->GetParam(nID, pParam);
}

int ffMuxerSetParam(void * hMuxer, int nID, void * pParam)
{
	if (!hMuxer)
		return QC_ERR_EMPTYPOINTOR;
	return FFMUXERINST->SetParam(nID, pParam);
}
#endif // __QC_LIB_FFMPEG__

int ffCreateMuxer(QC_Muxer_Func * pMuxer, QCParserFormat nFormat)
{
    if (pMuxer == NULL)
        return QC_ERR_ARG;
    pMuxer->nVer 		= 1;
    pMuxer->Open 		= ffMuxerOpen;
    pMuxer->Close 		= ffMuxerClose;
	pMuxer->Init		= ffMuxerInit;
    pMuxer->Write 		= ffMuxerWrite;
    pMuxer->GetParam 	= ffMuxerGetParam;
    pMuxer->SetParam 	= ffMuxerSetParam;
    
#ifdef __QC_LIB_FFMPEG__
	QCMuxInfo * pMuxInfo = new QCMuxInfo();
	memset(pMuxInfo, 0, sizeof(QCMuxInfo));
	pMuxInfo->m_nBuffSize = 32768;
	pMuxer->hMuxer = (void *)pMuxInfo;
#else
    CBaseFFMuxer * pNewMuxer = NULL;
    pNewMuxer = new CBaseFFMuxer (nFormat);
    if (pNewMuxer == NULL)
        return QC_ERR_FAILED;   
    pMuxer->hMuxer = (void *)pNewMuxer;
#endif // __QC_LIB_FFMPEG__
    
    qclog_init ();
    
    return QC_ERR_NONE;
}

int ffDestroyMuxer(QC_Muxer_Func * pMuxer)
{   
    if (pMuxer == NULL || pMuxer->hMuxer == NULL)
        return QC_ERR_ARG;
//	qclog_uninit();
#ifdef __QC_LIB_FFMPEG__  
	QCMuxInfo * pMuxInfo = (QCMuxInfo *)pMuxer->hMuxer;
	if (pMuxInfo != NULL && pMuxInfo->m_hCtx != NULL)
		ffMux_Close(pMuxInfo);
	delete pMuxInfo;
#else
	CBaseFFMuxer * pBaseMuxer = (CBaseFFMuxer *)pMuxer->hMuxer;
	delete pBaseMuxer;
#endif // __QC_LIB_FFMPEG__
    pMuxer->hMuxer = NULL;
    
    return QC_ERR_NONE;
}
