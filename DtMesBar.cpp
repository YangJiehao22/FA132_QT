#include "stdafx.h"
#include "DtMesBar.h"
#include "DtDpiUi.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"
#include "Resource.h"

namespace MesBarColors
{
	const COLORREF kBarBg = RGB(241, 245, 249);
	const COLORREF kBarBorder = RGB(203, 213, 225);
	const COLORREF kMesBadgeBg = RGB(219, 234, 254);
	const COLORREF kMesBadgeFg = RGB(29, 78, 216);
	const COLORREF kBoundBadgeBg = RGB(254, 249, 195);
	const COLORREF kBoundBadgeFg = RGB(161, 98, 7);
	const COLORREF kBoundDoneBg = RGB(220, 252, 231);
	const COLORREF kBoundDoneFg = RGB(22, 101, 52);
	const COLORREF kLabelFg = RGB(51, 65, 85);
	const COLORREF kSlotActiveBg = RGB(239, 246, 255);
	const COLORREF kSlotActiveBorder = RGB(37, 99, 235);
	const COLORREF kDotOnline = RGB(34, 197, 94);
	const COLORREF kDotOffline = RGB(203, 213, 225);
	const COLORREF kEditBg = RGB(255, 255, 255);
	const COLORREF kEditDisabledBg = RGB(248, 250, 252);
	const COLORREF kEditLockedBg = RGB(187, 247, 208);
	const COLORREF kEditLockedBorder = RGB(34, 197, 94);
	const COLORREF kEditLockedFg = RGB(21, 128, 61);
	const COLORREF kWoLabelLockedFg = RGB(22, 101, 52);
	const COLORREF kWoLockBtnIdleBg = RGB(226, 232, 240);
	const COLORREF kWoLockBtnIdleFg = RGB(100, 116, 139);
	const COLORREF kWoLockBtnReadyBg = RGB(254, 243, 199);
	const COLORREF kWoLockBtnReadyFg = RGB(180, 83, 9);
	const COLORREF kWoLockBtnLockedBg = RGB(22, 163, 74);
	const COLORREF kWoLockBtnLockedFg = RGB(255, 255, 255);
	const COLORREF kEditFg = RGB(15, 23, 42);
	const COLORREF kEditDisabledFg = RGB(148, 163, 184);
	const COLORREF kOfflinePhBg = RGB(248, 250, 252);
	const COLORREF kOfflinePhBorder = RGB(226, 232, 240);
	const COLORREF kOfflinePhFg = RGB(148, 163, 184);
}

static COLORREF DarkenColor(COLORREF c, int delta)
{
	return RGB(max(0, (int)GetRValue(c) - delta), max(0, (int)GetGValue(c) - delta), max(0, (int)GetBValue(c) - delta));
}

static void GetWoLockButtonColors(bool locked, bool hasText, bool pressed,
	COLORREF& bg, COLORREF& fg, COLORREF& border)
{
	if (locked)
	{
		bg = MesBarColors::kWoLockBtnLockedBg;
		fg = MesBarColors::kWoLockBtnLockedFg;
		border = RGB(21, 128, 61);
	}
	else if (hasText)
	{
		bg = MesBarColors::kWoLockBtnReadyBg;
		fg = MesBarColors::kWoLockBtnReadyFg;
		border = RGB(245, 158, 11);
	}
	else
	{
		bg = MesBarColors::kWoLockBtnIdleBg;
		fg = MesBarColors::kWoLockBtnIdleFg;
		border = RGB(203, 213, 225);
	}
	if (pressed)
		bg = DarkenColor(bg, 24);
}

static void DrawRoundRect(CDC& dc, const CRect& rc, COLORREF fill, COLORREF border, int radius)
{
	CPen pen(PS_SOLID, 1, border);
	CBrush br(fill);
	CPen* pOldPen = dc.SelectObject(&pen);
	CBrush* pOldBr = dc.SelectObject(&br);
	dc.RoundRect(rc, CPoint(radius, radius));
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBr);
}

static void DrawStatusDot(CDC& dc, int cx, int cy, int radius, COLORREF fill, COLORREF border)
{
	CBrush br(fill);
	CPen pen(PS_SOLID, 1, border);
	CBrush* pOldBr = dc.SelectObject(&br);
	CPen* pOldPen = dc.SelectObject(&pen);
	dc.Ellipse(cx - radius, cy - radius, cx + radius, cy + radius);
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBr);
}

static void SetEditCueBanner(CEdit& edit, LPCTSTR text)
{
	if (edit.GetSafeHwnd() == NULL)
		return;
	edit.SendMessage(0x1501, TRUE, (LPARAM)(text != NULL ? text : _T("")));
}

static void DrawSingleLineLabel(
	CDC& dc, CFont* pFont, const CRect& rcBlock, LPCTSTR text,
	COLORREF fg, UINT extraFmt = 0)
{
	if (rcBlock.IsRectEmpty() || text == NULL)
		return;
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(fg);
	if (pFont != NULL && pFont->GetSafeHandle() != NULL)
		dc.SelectObject(pFont);
	dc.DrawText(text, (LPRECT)&rcBlock, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | extraFmt);
}

CDtMesBar::CDtMesBar()
	: m_uiScale(1.0)
	, m_pFont(NULL)
	, m_pEditFont(NULL)
	, m_activeTab(0)
	, m_bWorkOrderLocked(false)
{
	for (int i = 0; i < MAX_CC16; i++)
		m_slotOnline[i] = false;
	m_brEdit.CreateSolidBrush(MesBarColors::kEditBg);
	m_brEditDisabled.CreateSolidBrush(MesBarColors::kEditDisabledBg);
	m_brEditLocked.CreateSolidBrush(MesBarColors::kEditLockedBg);
}

BOOL CDtMesBar::Create(CWnd* pParent, UINT nId)
{
	if (pParent == NULL)
		return FALSE;

	const int h = PreferredHeight();
	CRect rc(0, 0, 100, h);
	if (!CWnd::Create(NULL, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rc, pParent, nId))
		return FALSE;

	const DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
	const DWORD staticStyle = WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_NOPREFIX;

	m_lblWorkOrder.Create(ZH_UTF8(kMesWorkOrderLabel), staticStyle, CRect(0, 0, 10, 10), this, IDC_MES_LBL_WORK_ORDER);
	m_editWorkOrder.Create(editStyle, CRect(0, 0, 10, 10), this, IDC_MES_WORK_ORDER);
	m_staticWoLocked.Create(_T(""), WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS,
		CRect(0, 0, 10, 10), this, IDC_MES_WO_LOCKED);
	m_staticWoLocked.ShowWindow(SW_HIDE);
	m_btnWoLock.Create(_T(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
		CRect(0, 0, 10, 10), this, IDC_MES_WO_LOCK);
	m_lblBound.Create(_T(""), staticStyle | SS_CENTER, CRect(0, 0, 10, 10), this, IDC_MES_BOUND_LABEL);
	m_lblBound.ShowWindow(SW_HIDE);

	for (int s = 0; s < MAX_CC16; s++)
		m_editBoard[s].Create(editStyle, CRect(0, 0, 10, 10), this, IDC_MES_BOARD0 + s);

	m_editWorkOrder.SetLimitText(128);
	for (int s = 0; s < MAX_CC16; s++)
		m_editBoard[s].SetLimitText(128);

	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
	{
		m_lblWorkOrder.SetFont(m_pFont);
		m_lblBound.SetFont(m_pFont);
		m_btnWoLock.SetFont(m_pFont);
	}
	if (m_pEditFont != NULL && m_pEditFont->GetSafeHandle() != NULL)
	{
		m_editWorkOrder.SetFont(m_pEditFont);
		m_staticWoLocked.SetFont(m_pEditFont);
		for (int s = 0; s < MAX_CC16; s++)
			m_editBoard[s].SetFont(m_pEditFont);
	}

	SetEditCueBanner(m_editWorkOrder, ZH_UTF8(kMesWorkOrderPh));
	for (int s = 0; s < MAX_CC16; s++)
		ApplySlotEditState(s);

	ApplyWoLockButtonLook();
	RefreshBoundSummaryText();
	LayoutChildren();
	return TRUE;
}

void CDtMesBar::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_uiScale = scale;
}

void CDtMesBar::SetBarFont(CFont* pFont)
{
	m_pFont = pFont;
	if (m_lblWorkOrder.GetSafeHwnd() != NULL)
		m_lblWorkOrder.SetFont(pFont);
	if (m_lblBound.GetSafeHwnd() != NULL)
		m_lblBound.SetFont(pFont);
	if (m_btnWoLock.GetSafeHwnd() != NULL)
		m_btnWoLock.SetFont(pFont);
}

void CDtMesBar::SetEditFont(CFont* pFont)
{
	m_pEditFont = pFont;
	if (m_editWorkOrder.GetSafeHwnd() != NULL)
		m_editWorkOrder.SetFont(pFont);
	if (m_staticWoLocked.GetSafeHwnd() != NULL)
		m_staticWoLocked.SetFont(pFont);
	for (int s = 0; s < MAX_CC16; s++)
	{
		if (m_editBoard[s].GetSafeHwnd() != NULL)
			m_editBoard[s].SetFont(pFont);
	}
}

int CDtMesBar::PreferredHeight() const
{
	const double scale = (m_uiScale < 1.0) ? 1.0 : m_uiScale;
	const int editH = max(22, (int)(24 * scale));
	const int rowGap = max(4, (int)(5 * scale));
	const int slotLblH = max(16, (int)(18 * scale));
	const int vPad = max(8, (int)(10 * scale)) * 2;
	const int contentH = editH + rowGap + slotLblH + max(2, (int)(3 * scale)) + editH;
	return max(88, vPad + contentH);
}

int CDtMesBar::MeasureBoundBadgeWidth(const CString& boundText) const
{
	int boundBadgeW = max(72, (int)(84 * m_uiScale));
	if (GetSafeHwnd() == NULL || boundText.IsEmpty())
		return boundBadgeW;

	CClientDC dc(const_cast<CDtMesBar*>(this));
	CFont* pOld = NULL;
	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
		pOld = dc.SelectObject(m_pFont);
	CSize sz = {};
	::GetTextExtentPoint32(dc.GetSafeHdc(), boundText, boundText.GetLength(), &sz);
	if (pOld != NULL)
		dc.SelectObject(pOld);
	const int padH = max(12, (int)(14 * m_uiScale));
	return max(boundBadgeW, (int)sz.cx + padH * 2);
}

void CDtMesBar::RefreshBoundSummaryText()
{
	const int online = CountOnlineSlots();
	const int bound = CountBoundOnlineSlots();
	m_strBoundSummary.Format(ZH_UTF8(kMesBoundFmt), bound, online > 0 ? online : MAX_CC16);
}

void CDtMesBar::CalcLayout(MesBarLayout& out) const
{
	CRect rcClient;
	GetClientRect(&rcClient);
	out = MesBarLayout();

	const int pad = max(6, (int)(8 * m_uiScale));
	const int gap = max(4, (int)(6 * m_uiScale));
	const int rowGap = max(4, (int)(5 * m_uiScale));
	const int editH = max(22, (int)(24 * m_uiScale));
	const int badgeH = editH;
	const int mesBadgeW = max(44, (int)(48 * m_uiScale));
	const int lblWoW = max(44, (int)(52 * m_uiScale));
	const int lockBtnW = max(52, (int)(56 * m_uiScale));
	CString boundText = m_strBoundSummary;
	if (boundText.IsEmpty())
	{
		const int online = CountOnlineSlots();
		const int bound = CountBoundOnlineSlots();
		boundText.Format(ZH_UTF8(kMesBoundFmt), bound, online > 0 ? online : MAX_CC16);
	}
	const int boundBadgeW = MeasureBoundBadgeWidth(boundText);

	out.rcInner = rcClient;
	out.rcInner.DeflateRect(pad, max(4, (int)(5 * m_uiScale)));

	const int row1Top = out.rcInner.top;
	const int row1Bottom = row1Top + badgeH;
	out.row2Top = row1Bottom + rowGap;
	out.slotLblH = max(16, (int)(18 * m_uiScale));
	out.colGap = max(4, (int)(6 * m_uiScale));
	out.colW = max(40, (out.rcInner.Width() - out.colGap * (MAX_CC16 - 1)) / MAX_CC16);

	const int woEditW = max(100, out.rcInner.Width() - mesBadgeW - lblWoW - lockBtnW - boundBadgeW - gap * 5);
	int x = out.rcInner.left + mesBadgeW + gap + lblWoW + gap;
	out.rcWoEdit.SetRect(x, row1Top, x + woEditW, row1Bottom);
	out.rcBound.SetRect(out.rcInner.right - boundBadgeW, row1Top, out.rcInner.right, row1Bottom);

	const int slotEditTop = out.row2Top + out.slotLblH + max(2, (int)(3 * m_uiScale));
	x = out.rcInner.left;
	for (int s = 0; s < MAX_CC16; s++)
	{
		CRect rcCol(x, out.row2Top, x + out.colW, out.rcInner.bottom);
		int editBottom = slotEditTop + editH;
		if (editBottom > rcCol.bottom)
			editBottom = rcCol.bottom;
		out.rcBoardEdit[s].SetRect(rcCol.left, slotEditTop, rcCol.right, editBottom);
		x += out.colW + out.colGap;
	}
}

void CDtMesBar::LayoutChildren()
{
	if (GetSafeHwnd() == NULL)
		return;

	MesBarLayout layout;
	CalcLayout(layout);
	if (layout.rcInner.IsRectEmpty())
		return;

	const int gap = max(4, (int)(6 * m_uiScale));
	const int row1Top = layout.rcInner.top;
	const int row1Bottom = row1Top + max(22, (int)(24 * m_uiScale));
	const int mesBadgeW = max(44, (int)(48 * m_uiScale));
	const int lblWoW = max(44, (int)(52 * m_uiScale));

	int x = layout.rcInner.left;
	CRect rcMesBadge(x, row1Top, x + mesBadgeW, row1Bottom);
	x = rcMesBadge.right + gap;
	CRect rcLblWo(x, row1Top, x + lblWoW, row1Bottom);
	CRect rcWoEdit = layout.rcWoEdit;
	CRect rcLock(rcWoEdit.right + gap, row1Top, layout.rcBound.left - gap, row1Bottom);
	CRect rcBound = layout.rcBound;

	if (m_lblWorkOrder.GetSafeHwnd())
		m_lblWorkOrder.MoveWindow(rcLblWo, FALSE);
	if (m_editWorkOrder.GetSafeHwnd())
		m_editWorkOrder.MoveWindow(rcWoEdit, FALSE);
	if (m_staticWoLocked.GetSafeHwnd())
		m_staticWoLocked.MoveWindow(rcWoEdit, FALSE);
	if (m_btnWoLock.GetSafeHwnd())
		m_btnWoLock.MoveWindow(rcLock, FALSE);
	(void)rcBound;

	for (int s = 0; s < MAX_CC16; s++)
	{
		if (m_editBoard[s].GetSafeHwnd() && layout.rcBoardEdit[s].Height() >= 18)
			m_editBoard[s].MoveWindow(layout.rcBoardEdit[s], TRUE);
	}

	Invalidate(FALSE);
}

void CDtMesBar::SetSlotOnline(int slot, bool online)
{
	if (slot < 0 || slot >= MAX_CC16)
		return;
	m_slotOnline[slot] = online;
	ApplySlotEditState(slot);
	UpdateBoundSummary();
}

void CDtMesBar::SetActiveTab(int tab)
{
	if (tab < 0)
		tab = 0;
	if (tab >= MAX_CC16)
		tab = MAX_CC16 - 1;
	m_activeTab = tab;
	Invalidate(FALSE);
}

void CDtMesBar::SyncSlotStates()
{
	for (int s = 0; s < MAX_CC16; s++)
		ApplySlotEditState(s);
	UpdateBoundSummary();
	Invalidate(FALSE);
}

CString CDtMesBar::TrimScanText(const CString& text)
{
	CString t = text;
	t.Trim();
	t.Remove(_T('\r'));
	t.Remove(_T('\n'));
	return t;
}

void CDtMesBar::ApplySlotEditState(int slot)
{
	if (slot < 0 || slot >= MAX_CC16)
		return;
	if (m_editBoard[slot].GetSafeHwnd() == NULL)
		return;

	const bool online = m_slotOnline[slot];
	if (online)
	{
		m_editBoard[slot].ShowWindow(SW_SHOW);
		m_editBoard[slot].EnableWindow(TRUE);
		SetEditCueBanner(m_editBoard[slot], ZH_UTF8(kMesBoardScanPh));
	}
	else
	{
		/* Hide disabled edit: cue banner "未连接" causes vertical-line artifacts at high DPI. */
		m_editBoard[slot].ShowWindow(SW_HIDE);
		m_editBoard[slot].SetWindowText(_T(""));
		SetEditCueBanner(m_editBoard[slot], _T(""));
	}
}

void CDtMesBar::SetWorkOrderLocked(bool locked)
{
	CString woText = GetWorkOrderText();

	m_bWorkOrderLocked = locked;
	if (locked)
	{
		if (m_editWorkOrder.GetSafeHwnd() != NULL)
		{
			m_editWorkOrder.GetWindowText(woText);
			woText = TrimScanText(woText);
			m_editWorkOrder.ShowWindow(SW_HIDE);
		}
		if (m_staticWoLocked.GetSafeHwnd() != NULL)
		{
			m_staticWoLocked.SetWindowText(woText);
			m_staticWoLocked.ShowWindow(SW_SHOW);
			m_staticWoLocked.Invalidate(TRUE);
		}
	}
	else
	{
		if (m_staticWoLocked.GetSafeHwnd() != NULL)
		{
			m_staticWoLocked.GetWindowText(woText);
			woText = TrimScanText(woText);
			m_staticWoLocked.ShowWindow(SW_HIDE);
		}
		if (m_editWorkOrder.GetSafeHwnd() != NULL)
		{
			m_editWorkOrder.SetWindowText(woText);
			SetEditCueBanner(m_editWorkOrder, ZH_UTF8(kMesWorkOrderPh));
			m_editWorkOrder.ShowWindow(SW_SHOW);
			m_editWorkOrder.Invalidate(TRUE);
		}
	}

	if (m_lblWorkOrder.GetSafeHwnd() != NULL)
		m_lblWorkOrder.Invalidate(TRUE);
	ApplyWoLockButtonLook();
	Invalidate(FALSE);
}

void CDtMesBar::ApplyWoLockButtonLook()
{
	if (m_btnWoLock.GetSafeHwnd() != NULL)
		m_btnWoLock.Invalidate(TRUE);
}

void CDtMesBar::OnWorkOrderChanged()
{
	if (!m_bWorkOrderLocked)
		ApplyWoLockButtonLook();
}

bool CDtMesBar::IsWorkOrderLocked() const
{
	return m_bWorkOrderLocked;
}

void CDtMesBar::OnWoLockClicked()
{
	if (m_bWorkOrderLocked)
	{
		SetWorkOrderLocked(false);
		if (m_editWorkOrder.GetSafeHwnd() != NULL)
			m_editWorkOrder.SetFocus();
		return;
	}

	if (GetWorkOrderText().IsEmpty())
	{
		if (m_editWorkOrder.GetSafeHwnd() != NULL)
			m_editWorkOrder.SetFocus();
		return;
	}

	SetWorkOrderLocked(true);
}

int CDtMesBar::CountOnlineSlots() const
{
	int n = 0;
	for (int s = 0; s < MAX_CC16; s++)
	{
		if (m_slotOnline[s])
			n++;
	}
	return n;
}

int CDtMesBar::CountBoundOnlineSlots() const
{
	int n = 0;
	for (int s = 0; s < MAX_CC16; s++)
	{
		if (m_slotOnline[s] && IsBoardCodeFilled(s))
			n++;
	}
	return n;
}

void CDtMesBar::UpdateBoundSummary()
{
	RefreshBoundSummaryText();
	if (GetSafeHwnd() != NULL)
	{
		LayoutChildren();
		Invalidate(FALSE);
	}
}

CString CDtMesBar::GetWorkOrderText() const
{
	CString text;
	if (m_bWorkOrderLocked && m_staticWoLocked.GetSafeHwnd() != NULL)
		m_staticWoLocked.GetWindowText(text);
	else if (m_editWorkOrder.GetSafeHwnd() != NULL)
		m_editWorkOrder.GetWindowText(text);
	return TrimScanText(text);
}

CString CDtMesBar::GetBoardCode(int slot) const
{
	CString text;
	if (slot >= 0 && slot < MAX_CC16 && m_editBoard[slot].GetSafeHwnd() != NULL)
		m_editBoard[slot].GetWindowText(text);
	return TrimScanText(text);
}

bool CDtMesBar::IsBoardCodeFilled(int slot) const
{
	return !GetBoardCode(slot).IsEmpty();
}

void CDtMesBar::FocusNextEmptyOnlineSlot(int afterSlot)
{
	for (int pass = 0; pass < 2; pass++)
	{
		const int start = (pass == 0) ? afterSlot + 1 : 0;
		const int end = (pass == 0) ? MAX_CC16 : afterSlot + 1;
		for (int s = start; s < end; s++)
		{
			if (!m_slotOnline[s])
				continue;
			if (IsBoardCodeFilled(s))
				continue;
			if (m_editBoard[s].GetSafeHwnd() != NULL)
			{
				m_editBoard[s].SetFocus();
				m_editBoard[s].SetSel(0, -1);
			}
			return;
		}
	}
}

BOOL CDtMesBar::PreTranslateScanMessage(MSG* pMsg)
{
	if (pMsg == NULL || pMsg->message != WM_KEYDOWN || pMsg->wParam != VK_RETURN)
		return FALSE;

	HWND hFocus = ::GetFocus();
	if (m_editWorkOrder.GetSafeHwnd() != NULL && hFocus == m_editWorkOrder.GetSafeHwnd())
	{
		if (m_bWorkOrderLocked)
			return TRUE;

		CString text;
		m_editWorkOrder.GetWindowText(text);
		text = TrimScanText(text);
		m_editWorkOrder.SetWindowText(text);
		if (!text.IsEmpty())
		{
			SetWorkOrderLocked(true);
			FocusNextEmptyOnlineSlot(-1);
		}
		return TRUE;
	}

	for (int s = 0; s < MAX_CC16; s++)
	{
		if (m_editBoard[s].GetSafeHwnd() == NULL || hFocus != m_editBoard[s].GetSafeHwnd())
			continue;

		CString text;
		m_editBoard[s].GetWindowText(text);
		text = TrimScanText(text);
		m_editBoard[s].SetWindowText(text);
		UpdateBoundSummary();
		FocusNextEmptyOnlineSlot(s);
		return TRUE;
	}
	return FALSE;
}

void CDtMesBar::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_MES_WO_LOCK || lpDrawItemStruct == NULL)
	{
		CWnd::OnDrawItem(nIDCtl, lpDrawItemStruct);
		return;
	}

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	CRect rc = lpDrawItemStruct->rcItem;

	const bool pressed = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const bool hasText = !GetWorkOrderText().IsEmpty();
	COLORREF bg = 0, fg = 0, border = 0;
	GetWoLockButtonColors(m_bWorkOrderLocked, hasText, pressed, bg, fg, border);

	DrawRoundRect(dc, rc, bg, border, max(3, (int)(4 * m_uiScale)));
	const CString btnText = m_bWorkOrderLocked ? ZH_UTF8(kMesWoUnlock) : ZH_UTF8(kMesWoLock);
	DrawSingleLineLabel(dc, m_pFont, rc, btnText, fg, DT_CENTER);

	if (lpDrawItemStruct->itemState & ODS_FOCUS)
	{
		CRect rcFocus = rc;
		rcFocus.DeflateRect(2, 2);
		dc.DrawFocusRect(&rcFocus);
	}

	dc.Detach();
}

void CDtMesBar::OnPaint()
{
	CPaintDC dc(this);
	CRect rcClient;
	GetClientRect(&rcClient);
	if (rcClient.IsRectEmpty())
		return;

	const int radius = max(6, (int)(8 * m_uiScale));
	DrawRoundRect(dc, rcClient, MesBarColors::kBarBg, MesBarColors::kBarBorder, radius);

	MesBarLayout layout;
	CalcLayout(layout);

	const int editH = max(22, (int)(24 * m_uiScale));
	const int badgeH = editH;
	const int mesBadgeW = max(44, (int)(48 * m_uiScale));
	const int row1Top = layout.rcInner.top;
	const int row1Bottom = row1Top + badgeH;

	int x = layout.rcInner.left;
	CRect rcMesBadge(x, row1Top, x + mesBadgeW, row1Bottom);
	DrawRoundRect(dc, rcMesBadge, MesBarColors::kMesBadgeBg, RGB(147, 197, 253), max(4, (int)(6 * m_uiScale)));
	CRect rcMesText = rcMesBadge;
	rcMesText.DeflateRect(max(4, (int)(6 * m_uiScale)), 0);
	DrawSingleLineLabel(dc, m_pFont, rcMesText, ZH_UTF8(kMesBarTitle), MesBarColors::kMesBadgeFg, DT_CENTER);

	const int online = CountOnlineSlots();
	const int bound = CountBoundOnlineSlots();
	const bool allBound = online > 0 && bound >= online;
	CRect rcBound = layout.rcBound;
	DrawRoundRect(dc, rcBound,
		allBound ? MesBarColors::kBoundDoneBg : MesBarColors::kBoundBadgeBg,
		allBound ? RGB(134, 239, 172) : RGB(253, 224, 71),
		max(4, (int)(6 * m_uiScale)));
	CString boundText = m_strBoundSummary;
	if (boundText.IsEmpty())
		boundText.Format(ZH_UTF8(kMesBoundFmt), bound, online > 0 ? online : MAX_CC16);
	CRect rcBoundText = rcBound;
	rcBoundText.DeflateRect(max(6, (int)(8 * m_uiScale)), 0);
	DrawSingleLineLabel(dc, m_pFont, rcBoundText, boundText,
		allBound ? MesBarColors::kBoundDoneFg : MesBarColors::kBoundBadgeFg, DT_CENTER);

	const int dotR = max(3, (int)(4 * m_uiScale));
	x = layout.rcInner.left;
	for (int s = 0; s < MAX_CC16; s++)
	{
		CRect rcCol(x, layout.row2Top, x + layout.colW, layout.rcInner.bottom);
		if (s == m_activeTab)
		{
			CRect rcActive = rcCol;
			rcActive.bottom = layout.row2Top + layout.slotLblH + editH + max(6, (int)(8 * m_uiScale));
			DrawRoundRect(dc, rcActive, MesBarColors::kSlotActiveBg, MesBarColors::kSlotActiveBorder, max(4, (int)(6 * m_uiScale)));
		}

		CString slotName;
		slotName.Format(ZH_UTF8(kFa132TabFmt), s + 1);
		const int dotCx = rcCol.left + dotR + max(2, (int)(3 * m_uiScale));
		const int dotCy = layout.row2Top + layout.slotLblH / 2;
		DrawStatusDot(dc, dotCx, dotCy, dotR,
			m_slotOnline[s] ? MesBarColors::kDotOnline : MesBarColors::kDotOffline,
			RGB(226, 232, 240));

		CRect rcSlotLbl(rcCol.left + dotR * 2 + max(4, (int)(6 * m_uiScale)), layout.row2Top,
			rcCol.right, layout.row2Top + layout.slotLblH);
		DrawSingleLineLabel(dc, m_pFont, rcSlotLbl, slotName, MesBarColors::kLabelFg);

		if (!m_slotOnline[s] && layout.rcBoardEdit[s].Height() >= 12)
		{
			CRect rcPh = layout.rcBoardEdit[s];
			DrawRoundRect(dc, rcPh, MesBarColors::kOfflinePhBg, MesBarColors::kOfflinePhBorder, max(3, (int)(4 * m_uiScale)));
			DrawSingleLineLabel(dc, m_pFont, rcPh, ZH_UTF8(kMesOfflinePh), MesBarColors::kOfflinePhFg, DT_CENTER);
		}

		x += layout.colW + layout.colGap;
	}
}

HBRUSH CDtMesBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd == NULL)
		return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);

	const UINT id = pWnd->GetDlgCtrlID();
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		if (id == IDC_MES_WO_LOCKED)
		{
			pDC->SetBkMode(OPAQUE);
			pDC->SetBkColor(MesBarColors::kEditLockedBg);
			pDC->SetTextColor(MesBarColors::kEditLockedFg);
			return (HBRUSH)m_brEditLocked.GetSafeHandle();
		}

		pDC->SetBkMode(TRANSPARENT);
		if (id == IDC_MES_LBL_WORK_ORDER && m_bWorkOrderLocked)
		{
			pDC->SetTextColor(MesBarColors::kWoLabelLockedFg);
		}
		else
		{
			pDC->SetTextColor(MesBarColors::kLabelFg);
		}
		return (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
	}

	if (nCtlColor == CTLCOLOR_EDIT)
	{
		const bool enabled = pWnd->IsWindowEnabled() ? true : false;
		pDC->SetBkColor(enabled ? MesBarColors::kEditBg : MesBarColors::kEditDisabledBg);
		pDC->SetTextColor(enabled ? MesBarColors::kEditFg : MesBarColors::kEditDisabledFg);
		return (HBRUSH)(enabled ? m_brEdit.GetSafeHandle() : m_brEditDisabled.GetSafeHandle());
	}

	return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CDtMesBar::OnBoardCodeChanged(UINT /*nID*/)
{
	UpdateBoundSummary();
}

BEGIN_MESSAGE_MAP(CDtMesBar, CWnd)
	ON_WM_PAINT()
	ON_WM_DRAWITEM()
	ON_WM_CTLCOLOR()
	ON_CONTROL_RANGE(EN_CHANGE, IDC_MES_BOARD0, IDC_MES_BOARD3, OnBoardCodeChanged)
	ON_EN_CHANGE(IDC_MES_WORK_ORDER, OnWorkOrderChanged)
	ON_BN_CLICKED(IDC_MES_WO_LOCK, OnWoLockClicked)
END_MESSAGE_MAP()
