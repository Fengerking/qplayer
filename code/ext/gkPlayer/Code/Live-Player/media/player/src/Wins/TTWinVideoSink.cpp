/**
* File : TTWinVideoSink.cpp  
* Created on : 2014-9-1
* Author : yongping.lin
* Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
* Description : TTWinVideoSink实现文件
*/

#include "GKOsalConfig.h"
// INCLUDES
#include "Wins/TTWinVideoSink.h"
#include "TTSleep.h"
#include "TTSysTime.h"
#include "TTLog.h"

#include <windows.h>
#include <ddraw.h>
#include <mmsystem.h>
#include <stdio.h>

#pragma   comment(lib,"winmm.lib")

CTTWinVideoSink::CTTWinVideoSink(CTTSrcDemux* aSrcMux, TTCBaseAudioSink* aAudioSink, GKDecoderType aDecoderType)
: TTCBaseVideoSink(aSrcMux, aAudioSink, aDecoderType)
, mDD(NULL)
, mDDSPrimary(NULL)
, mDDSOffScr(NULL)		
, mSurfaceLost(0)
, mLeftOffset(0)
, mTopOffset(0)
, mDrawWidth(0)
, mDrawHeight(0)
{
	memset(&mOffScrSurfDesc, 0, sizeof(mOffScrSurfDesc));
	checkCPUFeature();
}

CTTWinVideoSink::~CTTWinVideoSink()
{
	closeVideoView ();
}

TTInt CTTWinVideoSink::stop()
{
	GKCAutoLock Lock(&mCritical);
	TTCBaseVideoSink::stop();

	//FlushWnd();

	closeVideoView ();

	return TTKErrNone;
}

TTInt CTTWinVideoSink::render()
{
	TTInt nErr = TTKErrNone;

	if (mDD == NULL || mDDSOffScr == NULL)
		newVideoView();

	HRESULT hr = DD_OK;

	nErr = checkSurfaceLost();
	if(TTKErrNone != nErr)
		return nErr;

	if(NULL == mDDSPrimary || NULL == mDDSOffScr)
		return TTKErrBadHandle;

	if(mSinkBuffer.Buffer[0] == NULL || mSinkBuffer.Buffer[1] == NULL || mSinkBuffer.Buffer[2] == NULL)
		return TTKErrArgument;

	RECT rctDest, rcSource;

	rcSource.left = 0;
    rcSource.top = 0;
	rcSource.right = mOffScrSurfDesc.dwWidth;
    rcSource.bottom = mOffScrSurfDesc.dwHeight;

    ::GetClientRect((HWND)mView, &rctDest);

	rctDest.left += mLeftOffset;
	rctDest.top += mTopOffset;
	rctDest.right = rctDest.left + mDrawWidth;
	rctDest.bottom = rctDest.top + mDrawHeight;

    ::ClientToScreen((HWND)mView, (LPPOINT)&rctDest.left);
    ::ClientToScreen((HWND)mView, (LPPOINT)&rctDest.right);

	if(!mSurfaceLost) {
		while(true) {
			hr = mDDSOffScr->Lock(&rcSource, &mOffScrSurfDesc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
			if(DDERR_WASSTILLDRAWING == hr) {
				LOGI("lock off screen return DDERR_WASSTILLDRAWING");
				TTSleep(2);
			}	else {
				break;
			}
		}

		if(DD_OK != hr) {
			LOGI("failed to lock off screen 0x%08X", hr);

			if(DDERR_SURFACELOST == hr || DDERR_UNSUPPORTED == hr){
				LOGI("here surface lost!");
				mSurfaceLost = true;
			}
			return 0;
		}

		LPBYTE lpSurf = (LPBYTE)mOffScrSurfDesc.lpSurface;
		TTInt i;
		if(lpSurf) {
			TTPBYTE pSrc = mSinkBuffer.Buffer[0];
			for (i = 0; i < mVideoFormat.Height; i++) {
				memcpy (lpSurf, pSrc, mVideoFormat.Width);

				pSrc += mSinkBuffer.Stride[0];
				lpSurf += mOffScrSurfDesc.lPitch;
			}

			pSrc = mSinkBuffer.Buffer[2];
			for (i = 0; i < mVideoFormat.Height / 2; i++) {
				memcpy(lpSurf, pSrc, mVideoFormat.Width / 2);

				pSrc += mSinkBuffer.Stride[2];
				lpSurf += mOffScrSurfDesc.lPitch / 2;
			}

			pSrc = mSinkBuffer.Buffer[1];
			for (i = 0; i < mVideoFormat.Height / 2; i++) {
				memcpy(lpSurf, pSrc, mVideoFormat.Width / 2);

				pSrc += mSinkBuffer.Stride[1];
				lpSurf += mOffScrSurfDesc.lPitch / 2;
			}
		}

		hr = mDDSOffScr->Unlock(&rcSource);
		if(DD_OK != hr)	{
			LOGI("failed to unlock off screen 0x%08X", hr);
		}
	}

	// Blt off screen surface to primary surface
	hr = mDDSPrimary->Blt(&rctDest, mDDSOffScr, &rcSource, DDBLT_WAIT, NULL);

	if(DD_OK != hr) {
		if(DDERR_SURFACELOST == hr || DDERR_UNSUPPORTED == hr)	{
			mSurfaceLost = true;
		}
		mSurfaceLost = false;

		return TTKErrCancel;
	} else	{ 
		mSurfaceLost = false;
	}

	return nErr;
}


TTInt CTTWinVideoSink::newVideoView()
{
	closeVideoView ();

	if(mVideoFormat.Width == 0 || mVideoFormat.Height == 0)	{
		return TTKErrNotReady;
	}

	HRESULT hr = DirectDrawCreateEx(NULL, (VOID**)&mDD, IID_IDirectDraw7, NULL);
	if(DD_OK != hr)	{
		LOGI("failed to DirectDrawCreateEx 0x%08X", hr);
		return TTKErrNotReady;
	}

	hr = mDD->SetCooperativeLevel((HWND)mView, DDSCL_NORMAL | DDSCL_ALLOWREBOOT | DDSCL_MULTITHREADED);
	if(DD_OK != hr)	{
		LOGI("failed to SetCooperativeLevel 0x%08X", hr);
		return TTKErrNotReady;
	}

	DDSURFACEDESC2 sPrimarySurfDesc;
	memset(&sPrimarySurfDesc, 0, sizeof(sPrimarySurfDesc));
	sPrimarySurfDesc.dwSize = sizeof(sPrimarySurfDesc);
	sPrimarySurfDesc.dwFlags = DDSD_CAPS;
	sPrimarySurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hr = mDD->CreateSurface(&sPrimarySurfDesc, &mDDSPrimary, NULL);
	if(DD_OK != hr)	{
		LOGI("failed to CreateSurface for primary surface 0x%08X", hr);
		return TTKErrUnknown;
	}

	LPDIRECTDRAWCLIPPER pcClipper;   // clipper
	hr = mDD->CreateClipper(0, &pcClipper, NULL);
	if(DD_OK != hr)	{
		LOGI("failed to CreateClipper 0x%08X", hr);
		return TTKErrUnknown;
	}

	hr = pcClipper->SetHWnd(0, (HWND)mView);
	if(DD_OK != hr)	{
		LOGI("failed to SetHWnd 0x%08X", hr);
		pcClipper->Release();
		return TTKErrUnknown;
	}

	hr = mDDSPrimary->SetClipper(pcClipper);
	if(DD_OK != hr)	{
		LOGI("failed to SetClipper 0x%08X", hr);
		pcClipper->Release();
		return TTKErrUnknown;
	}

	// done with clipper
	pcClipper->Release();

	// create YUV off screen surface
	memset(&mOffScrSurfDesc, 0, sizeof(mOffScrSurfDesc));
	mOffScrSurfDesc.dwSize = sizeof(mOffScrSurfDesc);
	mOffScrSurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	mOffScrSurfDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	mOffScrSurfDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	mOffScrSurfDesc.ddpfPixelFormat.dwFlags  = DDPF_FOURCC | DDPF_YUV ;
	mOffScrSurfDesc.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y','V', '1', '2');
	mOffScrSurfDesc.ddpfPixelFormat.dwYUVBitCount = 8;
	mOffScrSurfDesc.dwWidth = mVideoFormat.Width;
	mOffScrSurfDesc.dwHeight = mVideoFormat.Height;
	hr = mDD->CreateSurface(&mOffScrSurfDesc, &mDDSOffScr, NULL);
	if (DD_OK != hr) {
		LOGI("failed to CreateSurface for offscreen surface 0x%08X  dwWidth x dwHeight = %d x %d", hr, mVideoFormat.Width, mVideoFormat.Height);
		return TTKErrUnknown;
	}

	//FlushWnd();
	updateRect();

	mHWDec = 0;

	return TTKErrNone;
}

TTInt CTTWinVideoSink::closeVideoView()
{
	if(mDD != NULL)	{
		// release off screen surface
		if(mDDSOffScr)	{
			mDDSOffScr->Release();
			mDDSOffScr = NULL;
		}

		// release primary surface
		if(mDDSPrimary)	{
			mDDSPrimary->Release();
			mDDSPrimary = NULL;
		}

		mDD->Release();
		mDD = NULL;
	}

	return TTKErrNone;
}

TTInt CTTWinVideoSink::checkSurfaceLost()
{
	HRESULT hr = S_OK;
	if(mSurfaceLost && mDDSPrimary)	{
		// try restore primary surface
		hr = mDDSPrimary->Restore();
		if(DD_OK != hr)	{
			if(DDERR_WRONGMODE == hr) {
				newVideoView();
				mSurfaceLost = false;
			}
		}
	}

	if (mSurfaceLost && mDDSOffScr)	{
		// try restore off screen surface
		hr = mDDSOffScr->Restore();
		if(DD_OK != hr)	{
			if(DDERR_WRONGMODE == hr) {
				newVideoView();
				mSurfaceLost = false;
			}
		}
	}

	return TTKErrNone;
}

void CTTWinVideoSink::updateRect()
{
	RECT rc;
	::GetClientRect((HWND)mView, &rc);

	TTInt nWidth = rc.right - rc.left;
	TTInt nHeight = rc.bottom - rc.top;

	mDrawWidth = nWidth;
	mDrawHeight = nHeight;

	mLeftOffset = 0;
	mTopOffset = 0;

	if(mVideoFormat.Width*nHeight > mVideoFormat.Height*nWidth) {
		mDrawHeight = mVideoFormat.Height*nWidth/mVideoFormat.Width;
		mDrawHeight = mDrawHeight & ~1;
		mTopOffset = (nHeight - mDrawHeight)/2;
		mTopOffset = mTopOffset & ~1;
	}else if(mVideoFormat.Width*nHeight < mVideoFormat.Height*nWidth){
		mDrawWidth = mVideoFormat.Width*nHeight/mVideoFormat.Height;
		mDrawWidth = mDrawWidth & ~1;
		mLeftOffset = (nWidth - mDrawWidth)/2;
		mLeftOffset = mLeftOffset & ~1;
	}
}

void CTTWinVideoSink::FlushWnd()
{
	HDC hWinDC = ::GetDC((HWND)mView);
	HBRUSH hBlackBrush = ::CreateSolidBrush(RGB(0, 0, 0));

	RECT rc;
	::GetClientRect((HWND)mView, &rc);

	FillRect((HDC)hWinDC, &rc, hBlackBrush);

	::ReleaseDC((HWND)mView, hWinDC);
}