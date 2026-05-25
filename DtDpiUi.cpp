#include "stdafx.h"
#include "DtDpiUi.h"

#ifndef ODS_CHECK
#define ODS_CHECK 0x0008
#endif

void DtEnableProcessDpiAwareness()
{
	typedef BOOL (WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
	HMODULE hUser32 = ::GetModuleHandle(_T("USER32.dll"));
	if (hUser32 != NULL)
	{
		PFN_SetProcessDpiAwarenessContext pfnCtx = (PFN_SetProcessDpiAwarenessContext)
			::GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
		if (pfnCtx != NULL)
		{
			/* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (HANDLE)-4 */
			if (pfnCtx((HANDLE)(INT_PTR)-4))
				return;
		}
	}

	typedef HRESULT (WINAPI* PFN_SetProcessDpiAwareness)(int);
	HMODULE hShcore = ::LoadLibrary(_T("Shcore.dll"));
	if (hShcore != NULL)
	{
		PFN_SetProcessDpiAwareness pfn = (PFN_SetProcessDpiAwareness)
			::GetProcAddress(hShcore, "SetProcessDpiAwareness");
		if (pfn != NULL)
		{
			/* PROCESS_PER_MONITOR_DPI_AWARE = 2 */
			if (SUCCEEDED(pfn(2)))
			{
				::FreeLibrary(hShcore);
				return;
			}
		}
		::FreeLibrary(hShcore);
	}
	::SetProcessDPIAware();
}

double DtGetWindowUiScale(HWND hwnd)
{
	int dpiY = 96;
	if (hwnd != NULL && ::IsWindow(hwnd))
	{
		typedef UINT (WINAPI* PFN_GetDpiForWindow)(HWND);
		HMODULE hUser32 = ::GetModuleHandle(_T("USER32.dll"));
		if (hUser32 != NULL)
		{
			PFN_GetDpiForWindow pfn = (PFN_GetDpiForWindow)
				::GetProcAddress(hUser32, "GetDpiForWindow");
			if (pfn != NULL)
			{
				dpiY = (int)pfn(hwnd);
				if (dpiY < 72)
					dpiY = 96;
			}
			else
			{
				HDC hdc = ::GetDC(hwnd);
				if (hdc != NULL)
				{
					dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
					::ReleaseDC(hwnd, hdc);
				}
			}
		}
	}
	else
	{
		HDC hdc = ::GetDC(NULL);
		if (hdc != NULL)
		{
			dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
			::ReleaseDC(NULL, hdc);
		}
	}
	double scale = dpiY / 96.0;
	if (scale < 1.0)
		scale = 1.0;
	if (scale > 3.0)
		scale = 3.0;
	return scale;
}

bool DtCreateUiFont(CFont& font, int pointSize, bool bold, HWND hwndForDpi)
{
	if (font.GetSafeHandle() != NULL)
		font.DeleteObject();
	HWND hRef = (hwndForDpi != NULL && ::IsWindow(hwndForDpi)) ? hwndForDpi : ::GetDesktopWindow();
	const int dpiY = (int)(96.0 * DtGetWindowUiScale(hRef));
	const int height = -MulDiv(pointSize, dpiY, 72);
	return font.CreateFont(height, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Microsoft YaHei UI")) != FALSE;
}

// --- CDtScaledCheckBox ---

BEGIN_MESSAGE_MAP(CDtScaledCheckBox, CButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CDtScaledCheckBox::CDtScaledCheckBox()
	: m_scale(1.0)
	, m_pFont(NULL)
{
}

void CDtScaledCheckBox::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_scale = scale;
}

void CDtScaledCheckBox::SetCheckFont(CFont* pFont)
{
	m_pFont = pFont;
	if (pFont != NULL)
		SetFont(pFont);
}

int CDtScaledCheckBox::BoxPx() const
{
	const int sys = GetSystemMetrics(SM_CYMENUCHECK);
	const int scaled = (int)(20 * m_scale);
	return max(scaled, sys + (int)(4 * m_scale));
}

int CDtScaledCheckBox::PadL() const { return (int)(4 * m_scale); }
int CDtScaledCheckBox::TextGap() const { return (int)(8 * m_scale); }

void CDtScaledCheckBox::PreSubclassWindow()
{
	CButton::PreSubclassWindow();
	ModifyStyle(0, BS_OWNERDRAW | BS_NOTIFY);
}

static void DtScaledButtonToggleCheck(CDtScaledCheckBox* pBox, CPoint point)
{
	if (pBox == NULL || !::IsWindow(pBox->m_hWnd) || !pBox->IsWindowEnabled())
		return;

	CRect rc;
	pBox->GetClientRect(&rc);
	if (!rc.PtInRect(point))
		return;

	pBox->SetFocus();
	const int next = (pBox->GetCheck() == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
	pBox->SetCheck(next);
	pBox->Invalidate(FALSE);

	if (CWnd* pParent = pBox->GetParent())
	{
		const WPARAM wp = MAKEWPARAM((WORD)pBox->GetDlgCtrlID(), (WORD)BN_CLICKED);
		pParent->SendMessage(WM_COMMAND, wp, (LPARAM)pBox->m_hWnd);
	}
}

void CDtScaledCheckBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	DtScaledButtonToggleCheck(this, point);
	CButton::OnLButtonDown(nFlags, point);
}

void CDtScaledCheckBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	CButton::OnLButtonUp(nFlags, point);
}

void CDtScaledCheckBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == NULL)
		return;

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
		dc.SelectObject(m_pFont);
	else if (CFont* pf = GetFont())
		dc.SelectObject(pf);

	CRect rc = lpDrawItemStruct->rcItem;
	const BOOL bDisabled = (lpDrawItemStruct->itemState & ODS_DISABLED) != 0;
	const BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const BOOL bFocus = (lpDrawItemStruct->itemState & ODS_FOCUS) != 0;
	const BOOL bCheck = (GetCheck() == BST_CHECKED)
		|| ((lpDrawItemStruct->itemState & ODS_CHECK) != 0);

	COLORREF clrBg = GetSysColor(COLOR_BTNFACE);
	if (GetParent() != NULL)
	{
		CWnd* pParent = GetParent();
		if (pParent->IsKindOf(RUNTIME_CLASS(CDialog)))
			clrBg = RGB(246, 249, 252);
	}
	COLORREF clrText = GetSysColor(COLOR_WINDOWTEXT);
	if (bSelected && !bDisabled)
	{
		clrBg = GetSysColor(COLOR_HIGHLIGHT);
		clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	dc.FillSolidRect(&rc, clrBg);

	const int box = BoxPx();
	const int padL = PadL();
	const int tgap = TextGap();
	CRect rcBox(rc.left + padL, rc.top + (rc.Height() - box) / 2,
		rc.left + padL + box, rc.top + (rc.Height() - box) / 2 + box);

	UINT uState = DFCS_BUTTONCHECK;
	if (bCheck)
		uState |= DFCS_CHECKED;
	if (bDisabled)
		uState |= DFCS_INACTIVE;
	dc.DrawFrameControl(&rcBox, DFC_BUTTON, uState);

	CString text;
	GetWindowText(text);
	CRect rcText(rcBox.right + tgap, rc.top, rc.right - padL, rc.bottom);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(bDisabled ? GetSysColor(COLOR_GRAYTEXT) : clrText);
	dc.DrawText(text, &rcText, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

	if (bFocus && !bDisabled)
	{
		CRect rcFocus = rc;
		rcFocus.DeflateRect(1, 1);
		dc.DrawFocusRect(&rcFocus);
	}

	dc.Detach();
}

// --- CDtScaledRadio ---

BEGIN_MESSAGE_MAP(CDtScaledRadio, CButton)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CDtScaledRadio::CDtScaledRadio()
	: m_scale(1.0)
	, m_pFont(NULL)
{
}

void CDtScaledRadio::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_scale = scale;
}

void CDtScaledRadio::SetRadioFont(CFont* pFont)
{
	m_pFont = pFont;
	if (pFont != NULL)
		SetFont(pFont);
}

int CDtScaledRadio::CirclePx() const
{
	const int sys = GetSystemMetrics(SM_CYMENUCHECK);
	const int scaled = (int)(18 * m_scale);
	return max(scaled, sys + (int)(2 * m_scale));
}

int CDtScaledRadio::PadL() const { return (int)(4 * m_scale); }
int CDtScaledRadio::TextGap() const { return (int)(8 * m_scale); }

void CDtScaledRadio::PreSubclassWindow()
{
	CButton::PreSubclassWindow();
	ModifyStyle(0, BS_OWNERDRAW | BS_NOTIFY);
}

void CDtScaledRadio::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!IsWindowEnabled())
	{
		CButton::OnLButtonDown(nFlags, point);
		return;
	}

	CRect rc;
	GetClientRect(&rc);
	if (rc.PtInRect(point))
	{
		SetFocus();
		SetCheck(BST_CHECKED);
		Invalidate(FALSE);
		if (CWnd* pParent = GetParent())
		{
			const WPARAM wp = MAKEWPARAM((WORD)GetDlgCtrlID(), (WORD)BN_CLICKED);
			pParent->SendMessage(WM_COMMAND, wp, (LPARAM)m_hWnd);
		}
	}
	CButton::OnLButtonDown(nFlags, point);
}

void CDtScaledRadio::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == NULL)
		return;

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
		dc.SelectObject(m_pFont);
	else if (CFont* pf = GetFont())
		dc.SelectObject(pf);

	CRect rc = lpDrawItemStruct->rcItem;
	const BOOL bDisabled = (lpDrawItemStruct->itemState & ODS_DISABLED) != 0;
	const BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const BOOL bFocus = (lpDrawItemStruct->itemState & ODS_FOCUS) != 0;
	const BOOL bCheck = (GetCheck() == BST_CHECKED);

	COLORREF clrBg = GetSysColor(COLOR_BTNFACE);
	if (GetParent() != NULL)
	{
		CWnd* pParent = GetParent();
		if (pParent->IsKindOf(RUNTIME_CLASS(CDialog)))
			clrBg = RGB(246, 249, 252);
	}
	COLORREF clrText = GetSysColor(COLOR_WINDOWTEXT);
	if (bSelected && !bDisabled)
	{
		clrBg = GetSysColor(COLOR_HIGHLIGHT);
		clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	dc.FillSolidRect(&rc, clrBg);

	const int circle = CirclePx();
	const int padL = PadL();
	const int tgap = TextGap();
	CRect rcCircle(rc.left + padL, rc.top + (rc.Height() - circle) / 2,
		rc.left + padL + circle, rc.top + (rc.Height() - circle) / 2 + circle);

	UINT uState = DFCS_BUTTONRADIO;
	if (bCheck)
		uState |= DFCS_CHECKED;
	if (bDisabled)
		uState |= DFCS_INACTIVE;
	dc.DrawFrameControl(&rcCircle, DFC_BUTTON, uState);

	CString text;
	GetWindowText(text);
	CRect rcText(rcCircle.right + tgap, rc.top, rc.right - padL, rc.bottom);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(bDisabled ? GetSysColor(COLOR_GRAYTEXT) : clrText);
	dc.DrawText(text, &rcText, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

	if (bFocus && !bDisabled)
	{
		CRect rcFocus = rc;
		rcFocus.DeflateRect(1, 1);
		dc.DrawFocusRect(&rcFocus);
	}

	dc.Detach();
}
