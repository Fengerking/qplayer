#ifndef __RTSPClient_API_H__
#define __RTSPClient_API_H__

#include "common_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct
	{
		// The Rtsp handle Instance.
		void *			hRtspIns;		
		int(*RTSP_GetErrCode)  (CM_RTSP_Handle hHandle);
		int(*RTSP_SetCallback) (CM_RTSP_Handle hHandle, RTSPSourceCallBack _callback);
		int(*RTSP_OpenStream) (CM_RTSP_Handle hHandle, int _channelid, char *_url, int _connType, unsigned int _mediaType, void *userPtr, int _verbosity);
		int(*RTSP_CloseStream) (CM_RTSP_Handle hHandle);
	} CM_Rtsp_Ins;


	CM_DLLEXPORT_C int CM_API qcCreateRtspIns(CM_Rtsp_Ins *fRtspIns, void * hInst);
	typedef int (CM_API QCCREATERTSPINS) (CM_Rtsp_Ins  *fRtspIns, void * hInst);
	CM_DLLEXPORT_C int CM_API qcDestroyRtspIns(CM_Rtsp_Ins  *fRtspIns);
	typedef int (CM_API QCDESTROYRTSPINS) (CM_Rtsp_Ins  *fRtspIns);

#ifdef __cplusplus
}
#endif

#endif