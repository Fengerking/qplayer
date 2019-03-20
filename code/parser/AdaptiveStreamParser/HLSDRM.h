/*******************************************************************************
File:		HLSDrm.h

Contains:	HLS  DRM Common header file.

Written by:	Qichao Shen

Change History (most recent first):
2017-01-03		Qichao			Create file

*******************************************************************************/
#ifndef __HLS_DRM_H__
#define __HLS_DRM_H__

/**
 * Structure of DRM process parameter
 */

typedef 
enum E_HLS_DRM_TYPE
{
	E_HLS_DRM_NONE = 0x00000000,
	E_HLS_DRM_AES128 = 0x00000001,
	E_HLS_DRM_QINIU_DRM = 0x00000002,
	E_HLS_DRM_MAX = 0X7FFFFFFF,
};

struct S_DRM_HSL_PROCESS_INFO
{
	char	strCurURL[4096];
	char	strKeyString[1024];	//#EXT-X-KEY string
	unsigned int	ulSequenceNum;
	void*	pReserved;
};


#endif // __HLS_DRM_H__

