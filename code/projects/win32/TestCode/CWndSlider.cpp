/*******************************************************************************
	File:		CWndSlider.cpp

	Contains:	Window slide pos implement code

	Written by:	Bangfei Jin

	Change History (most recent first):
	2016-12-29		Bangfei			Create file

*******************************************************************************/
#include "windows.h"
#include "tchar.h"

#include "CWndSlider.h"
#include "resource.h"

#include "ULogFunc.h"

#pragma warning (disable : 4996)

CWndSlider::CWndSlider(HINSTANCE hInst)
	: CWndBase (hInst)
	, m_hPenBound (NULL)
	, m_hBrushBG (NULL)
	, m_nThumbPos (0)
	, m_nMinPos (0)
	, m_nMaxPos (100)
	, m_nCurPos (0)
	, m_hMemDC (NULL)
	, m_hBitmap(NULL)
{
}

CWndSlider::~CWndSlider(void)
{
	if (m_hPenBound != NULL)
		DeleteObject (m_hPenBound);
	m_hPenBound = NULL;
	if (m_hBrushBG != NULL)
		DeleteObject (m_hBrushBG);
	m_hBrushBG = NULL;
	DeleteDC(m_hMemDC);
	DeleteObject(m_hBitmap);
}

bool CWndSlider::CreateWnd (HWND hParent, RECT rcView, COLORREF clrBG)
{
	if (!CWndBase::CreateWnd (hParent, rcView, clrBG))
		return false;

	m_hPenBound = ::CreatePen (PS_SOLID, 2, RGB (80, 80, 80));
	m_hBrushBG = ::CreateSolidBrush (RGB (120, 120, 120));

	SetRect (&m_rcThumb, 0, 0, 32, rcView.bottom);
	m_hBrushTmb = ::CreateSolidBrush (RGB (200, 200, 200));

	m_hBitmap = CreateBitmap(rcView.right - rcView.left, rcView.bottom - rcView.top, 1, 32, NULL);
	HDC hWinDC = GetWindowDC(hParent);
	m_hMemDC = CreateCompatibleDC(hWinDC);
	ReleaseDC(hParent, hWinDC);
	SelectObject(m_hMemDC, m_hBitmap);

	return true;
}

bool CWndSlider::SetRange (int nMin, int nMax)
{
	if (nMax < nMin || nMin < 0)
		return false;

	m_nMinPos = nMin;
	m_nMaxPos = nMax;

	return true;
}

int CWndSlider::GetPos (void)
{
	return (int)m_nCurPos;
}

int CWndSlider::SetPos (int nPos)
{
	if (nPos < m_nMinPos || nPos > m_nMaxPos || m_nMaxPos == m_nMinPos)
		return -1;

	if (m_nMaxPos <= 0)
		return -1;

	m_nCurPos = nPos;

	RECT rcWnd;
	GetClientRect (m_hWnd, &rcWnd);

	UpdateThumb (false);

	m_nThumbPos = m_nCurPos * (rcWnd.right - m_rcThumb.right) / (m_nMaxPos - m_nMinPos);

	//UpdateThumb (true);
	InvalidateRect(m_hWnd, NULL, TRUE);

	return (int)m_nCurPos;
}

void CWndSlider::UpdateThumb (bool bNewPos)
{
	RECT rcThumb;
	SetRect (&rcThumb, (int)m_nThumbPos, 0, (int)m_nThumbPos + m_rcThumb.right, m_rcThumb.bottom);
	if (rcThumb.left >= 2)
		rcThumb.left -= 2;
	if (rcThumb.right <= m_rcWnd.right - 2)
		rcThumb.right += 2;

	if (bNewPos)
		InvalidateRect (m_hWnd, &rcThumb, FALSE);
	else
		InvalidateRect (m_hWnd, &rcThumb, TRUE);
}

LRESULT CWndSlider::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rcWnd;
	if (hwnd != NULL)
		GetClientRect (hwnd, &rcWnd);

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		SetCapture (hwnd);
		if (m_nMaxPos <= 0)
			return S_OK;

		UpdateThumb (false);

		m_nThumbPos = LOWORD(lParam) - m_rcThumb.right / 2;
		if (m_nThumbPos < 0)
			m_nThumbPos = 0;
		else if (m_nThumbPos > rcWnd.right - m_rcThumb.right)
			m_nThumbPos = rcWnd.right - m_rcThumb.right;
		m_nCurPos = m_nThumbPos * (m_nMaxPos - m_nMinPos) / (rcWnd.right - m_rcThumb.right);

		UpdateThumb (true);
 
		PostMessage (m_hParent, WM_YYSLD_NEWPOS, (int)m_nCurPos, 0);

		return S_OK;

	case WM_LBUTTONUP:
		ReleaseCapture ();
		return S_OK;

	case WM_MOUSEMOVE:
		if (wParam != MK_LBUTTON)
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		if (m_nMaxPos <= 0)
			return DefWindowProc(hwnd, uMsg, wParam, lParam);

		UpdateThumb (false);

		m_nThumbPos = LOWORD(lParam) - m_rcThumb.right / 2;
		if (m_nThumbPos < 0)
			m_nThumbPos = 0;
		else if (m_nThumbPos > rcWnd.right - m_rcThumb.right)
			m_nThumbPos = rcWnd.right - m_rcThumb.right;
		m_nCurPos = m_nThumbPos * (m_nMaxPos - m_nMinPos) / (rcWnd.right - m_rcThumb.right);

		UpdateThumb (true);

		PostMessage (m_hParent, WM_YYSLD_NEWPOS, (int)m_nCurPos, 0);

		return S_OK;

	case WM_PAINT:
	{
		SelectObject (m_hMemDC, m_hPenBound);
		SelectObject (m_hMemDC, m_hBrushBG);
		FillRect (m_hMemDC, &rcWnd, m_hBrushBG);

		SetTextColor(m_hMemDC, RGB(0, 0, 0));
		SetBkMode(m_hMemDC, TRANSPARENT);

		rcWnd.top += 4;
		char	szTimePos[32];
		int		nPos = (int)(m_nCurPos / 1000);
		sprintf(szTimePos, "%02d:%02d:%02d", nPos / 3600, (nPos % 3600 / 60), (nPos % 60));
		DrawText(m_hMemDC, szTimePos, 8, &rcWnd, DT_LEFT);
		rcWnd.left = rcWnd.right / 3;
		DrawText(m_hMemDC, szTimePos, 8, &rcWnd, DT_LEFT);

		nPos = (int)((m_nMaxPos - m_nMinPos) / 1000);
		sprintf(szTimePos, "%02d:%02d:%02d", nPos / 3600, (nPos % 3600 / 60), (nPos % 60));
		rcWnd.left = rcWnd.right * 2 / 3;
		DrawText(m_hMemDC, szTimePos, 8, &rcWnd, DT_LEFT);
		DrawText(m_hMemDC, szTimePos, 8, &rcWnd, DT_RIGHT);

		SelectObject (m_hMemDC, m_hBrushTmb);
		Rectangle (m_hMemDC, (int)m_nThumbPos, m_rcThumb.top, (int)m_nThumbPos + m_rcThumb.right, m_rcThumb.bottom);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		BitBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, m_hMemDC, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);

		return S_OK;// DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	case WM_ERASEBKGND:
		return S_OK;

	default:
		break;
	}

	return	CWndBase::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

