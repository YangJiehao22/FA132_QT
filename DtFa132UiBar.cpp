#include "stdafx.h"
#include "DtFa132UiBar.h"
#include "DtDpiUi.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"

namespace Fa132UiColors
{
	const COLORREF kTabOnlineSelBg = RGB(37, 99, 235);
	const COLORREF kTabOnlineSelFg = RGB(255, 255, 255);
	const COLORREF kTabOnlineBg = RGB(236, 253, 245);
	const COLORREF kTabOnlineFg = RGB(21, 128, 61);
	const COLORREF kTabOfflineSelBg = RGB(100, 116, 139);
	const COLORREF kTabOfflineSelFg = RGB(255, 255, 255);
	const COLORREF kTabOfflineBg = RGB(248, 250, 252);
	const COLORREF kTabOfflineFg = RGB(100, 116, 139);
	const COLORREF kDotOnline = RGB(34, 197, 94);
	const COLORREF kDotOffline = RGB(203, 213, 225);
	const COLORREF kBarSelected = RGB(37, 99, 235);

	const COLORREF kOverviewBg = RGB(241, 245, 249);
	const COLORREF kOverviewBorder = RGB(203, 213, 225);
	const COLORREF kBadgeOnlineBg = RGB(220, 252, 231);
	const COLORREF kBadgeOnlineFg = RGB(22, 101, 52);
	const COLORREF kBadgeOfflineBg = RGB(241, 245, 249);
	const COLORREF kBadgeOfflineFg = RGB(100, 116, 139);
	const COLORREF kChipOnlineBg = RGB(255, 255, 255);
	const COLORREF kChipOnlineBorder = RGB(134, 239, 172);
	const COLORREF kChipOfflineBg = RGB(248, 250, 252);
	const COLORREF kChipOfflineBorder = RGB(226, 232, 240);
	const COLORREF kChipActiveRing = RGB(37, 99, 235);
	const COLORREF kTabNgBg = RGB(254, 226, 226);
	const COLORREF kTabNgFg = RGB(185, 28, 28);
	const COLORREF kTabNgSelBg = RGB(220, 38, 38);
	const COLORREF kTabNgSelFg = RGB(255, 255, 255);
	const COLORREF kTabOkResultBg = RGB(220, 252, 231);
	const COLORREF kTabOkResultFg = RGB(21, 128, 61);
	const COLORREF kTabOkResultSelBg = RGB(22, 163, 74);
	const COLORREF kDotNg = RGB(239, 68, 68);
	const COLORREF kBadgeNgBg = RGB(254, 226, 226);
	const COLORREF kBadgeNgFg = RGB(185, 28, 28);
	const COLORREF kBadgeOkRunBg = RGB(220, 252, 231);
	const COLORREF kBadgeOkRunFg = RGB(22, 101, 52);
	const COLORREF kChipNgBg = RGB(254, 242, 242);
	const COLORREF kChipNgBorder = RGB(252, 165, 165);
	const COLORREF kChipOkRunBg = RGB(240, 253, 244);
	const COLORREF kChipOkRunBorder = RGB(134, 239, 172);
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

static int MeasureFontLineHeight(CDC& dc, CFont* pFont)
{
	if (pFont != NULL && pFont->GetSafeHandle() != NULL)
		dc.SelectObject(pFont);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);
	return tm.tmHeight + tm.tmExternalLeading;
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

static void DrawTwoLineLabel(
	CDC& dc, CFont* pFontTitle, CFont* pFontSub,
	const CRect& rcBlock, LPCTSTR line1, LPCTSTR line2,
	COLORREF fg1, COLORREF fg2, bool center = false)
{
	if (rcBlock.IsRectEmpty())
		return;

	const UINT fmt = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | (center ? DT_CENTER : DT_LEFT);

	const int h1 = MeasureFontLineHeight(dc, pFontTitle);
	const int h2 = MeasureFontLineHeight(dc, pFontSub);
	const int gap = max(1, rcBlock.Height() / 16);
	const int totalH = h1 + gap + h2;
	int y = rcBlock.top + max(0, (rcBlock.Height() - totalH) / 2);

	dc.SetBkMode(TRANSPARENT);

	CRect rc1(rcBlock.left, y, rcBlock.right, y + h1);
	dc.SetTextColor(fg1);
	if (pFontTitle != NULL && pFontTitle->GetSafeHandle() != NULL)
		dc.SelectObject(pFontTitle);
	dc.DrawText(line1, &rc1, fmt);

	y += h1 + gap;
	CRect rc2(rcBlock.left, y, rcBlock.right, y + h2);
	dc.SetTextColor(fg2);
	if (pFontSub != NULL && pFontSub->GetSafeHandle() != NULL)
		dc.SelectObject(pFontSub);
	dc.DrawText(line2, &rc2, fmt);
}

// --- CDtFa132TabCtrl ---

CDtFa132TabCtrl::CDtFa132TabCtrl()
	: m_uiScale(1.0)
	, m_pFont(NULL)
{
	for (int i = 0; i < MAX_CC16; i++)
	{
		m_slotOnline[i] = false;
		m_slotTestResult[i] = DtCarFunction::Fa132SlotResultNone;
		m_slotNgCount[i] = 0;
	}
}

void CDtFa132TabCtrl::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_uiScale = scale;
}

void CDtFa132TabCtrl::SetTabFont(CFont* pFont)
{
	m_pFont = pFont;
}

void CDtFa132TabCtrl::SetSlotOnline(int slot, bool online)
{
	if (slot >= 0 && slot < MAX_CC16)
		m_slotOnline[slot] = online;
}

void CDtFa132TabCtrl::SetSlotTestResult(int slot, DtCarFunction::Fa132SlotTestResult result, int ngCount)
{
	if (slot < 0 || slot >= MAX_CC16)
		return;
	m_slotTestResult[slot] = result;
	m_slotNgCount[slot] = (ngCount < 0) ? 0 : ngCount;
}

void CDtFa132TabCtrl::ApplyTabSizing(int totalWidthPx)
{
	if (GetSafeHwnd() == NULL || totalWidthPx < 32)
		return;
	const int gap = max(4, (int)(6 * m_uiScale));
	const int tabW = max(96, (totalWidthPx - gap * (MAX_CC16 + 1)) / MAX_CC16);
	const int tabH = PreferredTabRowHeight();
	SetItemSize(CSize(tabW, tabH));
}

int CDtFa132TabCtrl::PreferredTabRowHeight() const
{
	return max(36, (int)(38 * m_uiScale));
}

int CDtFa132TabCtrl::MeasureStripHeight(int totalWidthPx)
{
	if (GetSafeHwnd() == NULL)
		return PreferredTabRowHeight();

	ApplyTabSizing(totalWidthPx);
	if (GetItemCount() <= 0)
		return PreferredTabRowHeight();

	CRect rcItem;
	GetItemRect(0, &rcItem);
	int maxBottom = rcItem.bottom;
	for (int i = 1; i < GetItemCount(); i++)
	{
		CRect rc;
		GetItemRect(i, &rc);
		if (rc.bottom > maxBottom)
			maxBottom = rc.bottom;
	}

	const int pad = max(2, (int)(4 * m_uiScale));
	CRect rcWnd(0, 0, max(totalWidthPx, rcItem.right + pad), maxBottom + pad);
	AdjustRect(TRUE, &rcWnd);
	return max(PreferredTabRowHeight(), rcWnd.Height());
}

void CDtFa132TabCtrl::DrawTabItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == NULL)
		return;

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	const int slot = (int)lpDrawItemStruct->itemID;
	const bool online = (slot >= 0 && slot < MAX_CC16) ? m_slotOnline[slot] : false;
	const bool selected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const DtCarFunction::Fa132SlotTestResult testResult =
		(slot >= 0 && slot < MAX_CC16) ? m_slotTestResult[slot] : DtCarFunction::Fa132SlotResultNone;
	const int ngCount = (slot >= 0 && slot < MAX_CC16) ? m_slotNgCount[slot] : 0;

	COLORREF bg, fg, dotFill, dotBorder;
	if (!online)
	{
		if (selected)
		{
			bg = Fa132UiColors::kTabOfflineSelBg;
			fg = Fa132UiColors::kTabOfflineSelFg;
		}
		else
		{
			bg = Fa132UiColors::kTabOfflineBg;
			fg = Fa132UiColors::kTabOfflineFg;
		}
		dotFill = Fa132UiColors::kDotOffline;
		dotBorder = RGB(148, 163, 184);
	}
	else if (testResult == DtCarFunction::Fa132SlotResultNg)
	{
		if (selected)
		{
			bg = Fa132UiColors::kTabNgSelBg;
			fg = Fa132UiColors::kTabNgSelFg;
		}
		else
		{
			bg = Fa132UiColors::kTabNgBg;
			fg = Fa132UiColors::kTabNgFg;
		}
		dotFill = Fa132UiColors::kDotNg;
		dotBorder = RGB(185, 28, 28);
	}
	else if (testResult == DtCarFunction::Fa132SlotResultOk)
	{
		if (selected)
		{
			bg = Fa132UiColors::kTabOkResultSelBg;
			fg = Fa132UiColors::kTabOnlineSelFg;
		}
		else
		{
			bg = Fa132UiColors::kTabOkResultBg;
			fg = Fa132UiColors::kTabOkResultFg;
		}
		dotFill = Fa132UiColors::kDotOnline;
		dotBorder = RGB(22, 163, 74);
	}
	else
	{
		if (selected)
		{
			bg = Fa132UiColors::kTabOnlineSelBg;
			fg = Fa132UiColors::kTabOnlineSelFg;
		}
		else
		{
			bg = Fa132UiColors::kTabOnlineBg;
			fg = Fa132UiColors::kTabOnlineFg;
		}
		dotFill = Fa132UiColors::kDotOnline;
		dotBorder = RGB(22, 163, 74);
	}

	CRect rc = lpDrawItemStruct->rcItem;
	rc.DeflateRect(2, 2);
	dc.FillSolidRect(&rc, bg);

	const int barH = selected ? max(3, (int)(4 * m_uiScale)) : 0;
	if (selected)
	{
		CRect bar = rc;
		bar.top = bar.bottom - barH;
		dc.FillSolidRect(&bar, Fa132UiColors::kBarSelected);
	}

	const int dotR = max(4, (int)(5 * m_uiScale));
	CRect rcText = rc;
	rcText.bottom -= barH;
	const int dotCy = rcText.top + rcText.Height() / 2;

	CString title;
	title.Format(_T("FA132-%d"), slot + 1);
	if (online && testResult == DtCarFunction::Fa132SlotResultNg)
	{
		if (ngCount > 0)
			title.AppendFormat(ZH_UTF8(kFa132TabNgInlineFmt), ngCount);
		else
			title += ZH_UTF8(kFa132TabNgInline);
	}
	else if (online && testResult == DtCarFunction::Fa132SlotResultOk)
		title += ZH_UTF8(kFa132TabOkInline);

	CFont* pOld = NULL;
	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
		pOld = dc.SelectObject(m_pFont);

	SIZE szTitle = {};
	::GetTextExtentPoint32(dc.GetSafeHdc(), title, title.GetLength(), &szTitle);
	const int gap = max(6, (int)(8 * m_uiScale));
	const int blockW = dotR * 2 + gap + szTitle.cx;
	const int blockLeft = rcText.left + max(0, (rcText.Width() - blockW) / 2);
	const int dotCx = blockLeft + dotR;
	DrawStatusDot(dc, dotCx, dotCy, dotR, dotFill, dotBorder);

	CRect rcLine = rcText;
	rcLine.left = dotCx + dotR + gap;
	rcLine.right = blockLeft + blockW;
	DrawSingleLineLabel(dc, m_pFont, rcLine, title, fg, DT_LEFT);

	if (pOld != NULL)
		dc.SelectObject(pOld);
	dc.Detach();
}

// --- CDtFa132OverviewBar ---

BEGIN_MESSAGE_MAP(CDtFa132OverviewBar, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CDtFa132OverviewBar::CDtFa132OverviewBar()
	: m_uiScale(1.0)
	, m_pFont(NULL)
	, m_onlineCount(0)
	, m_activeTab(0)
	, m_hasRunResult(false)
	, m_ngSlotCount(0)
	, m_okSlotCount(0)
{
	for (int i = 0; i < MAX_CC16; i++)
	{
		m_slotOnline[i] = false;
		m_slotTestResult[i] = DtCarFunction::Fa132SlotResultNone;
		m_slotNgCount[i] = 0;
	}
}

void CDtFa132OverviewBar::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_uiScale = scale;
}

void CDtFa132OverviewBar::SetBarFont(CFont* pFont)
{
	m_pFont = pFont;
}

void CDtFa132OverviewBar::SetOnlineCount(int count)
{
	if (count < 0)
		count = 0;
	if (count > MAX_CC16)
		count = MAX_CC16;
	m_onlineCount = count;
}

void CDtFa132OverviewBar::SetActiveTab(int tab)
{
	if (tab < 0)
		tab = 0;
	if (tab >= MAX_CC16)
		tab = MAX_CC16 - 1;
	m_activeTab = tab;
}

void CDtFa132OverviewBar::SetSlotOnline(int slot, bool online)
{
	if (slot >= 0 && slot < MAX_CC16)
		m_slotOnline[slot] = online;
}

void CDtFa132OverviewBar::SetSlotTestResult(int slot, DtCarFunction::Fa132SlotTestResult result, int ngCount)
{
	if (slot < 0 || slot >= MAX_CC16)
		return;
	m_slotTestResult[slot] = result;
	m_slotNgCount[slot] = (ngCount < 0) ? 0 : ngCount;
}

void CDtFa132OverviewBar::SetRunSummary(bool hasResult, int ngSlotCount, int okSlotCount)
{
	m_hasRunResult = hasResult;
	m_ngSlotCount = (ngSlotCount < 0) ? 0 : ngSlotCount;
	m_okSlotCount = (okSlotCount < 0) ? 0 : okSlotCount;
}

int CDtFa132OverviewBar::PreferredBarHeight() const
{
	return max(36, (int)(38 * m_uiScale));
}

void CDtFa132OverviewBar::OnPaint()
{
	CPaintDC dc(this);
	CRect rcClient;
	GetClientRect(&rcClient);
	if (rcClient.IsRectEmpty())
		return;

	const int radius = max(6, (int)(8 * m_uiScale));
	DrawRoundRect(dc, rcClient, Fa132UiColors::kOverviewBg, Fa132UiColors::kOverviewBorder, radius);

	const int pad = max(6, (int)(8 * m_uiScale));
	const int gap = max(6, (int)(8 * m_uiScale));
	CRect rcInner = rcClient;
	rcInner.DeflateRect(pad, max(4, (int)(5 * m_uiScale)));

	CFont* pOldFont = NULL;
	if (m_pFont != NULL && m_pFont->GetSafeHandle() != NULL)
		pOldFont = dc.SelectObject(m_pFont);
	dc.SetBkMode(TRANSPARENT);

	CString badgeText;
	COLORREF badgeBg = Fa132UiColors::kBadgeOfflineBg;
	COLORREF badgeBorder = Fa132UiColors::kOverviewBorder;
	COLORREF badgeFg = Fa132UiColors::kBadgeOfflineFg;
	if (m_hasRunResult)
	{
		if (m_ngSlotCount > 0)
		{
			badgeText.Format(ZH_UTF8(kFa132RunSummaryNgFmt), m_ngSlotCount, MAX_CC16);
			badgeBg = Fa132UiColors::kBadgeNgBg;
			badgeBorder = Fa132UiColors::kChipNgBorder;
			badgeFg = Fa132UiColors::kBadgeNgFg;
		}
		else
		{
			badgeText = ZH_UTF8(kFa132RunSummaryAllOk);
			badgeBg = Fa132UiColors::kBadgeOkRunBg;
			badgeBorder = Fa132UiColors::kChipOkRunBorder;
			badgeFg = Fa132UiColors::kBadgeOkRunFg;
		}
	}
	else
	{
		badgeText.Format(_T("%s %d/%d"), (LPCTSTR)ZH_UTF8(kFa132OnlineBadge), m_onlineCount, MAX_CC16);
		const bool anyOnline = m_onlineCount > 0;
		if (anyOnline)
		{
			badgeBg = Fa132UiColors::kBadgeOnlineBg;
			badgeBorder = RGB(134, 239, 172);
			badgeFg = Fa132UiColors::kBadgeOnlineFg;
		}
	}
	SIZE szBadge = {};
	::GetTextExtentPoint32(dc.GetSafeHdc(), badgeText, badgeText.GetLength(), &szBadge);
	const int badgePadH = max(12, (int)(14 * m_uiScale));
	const int badgeW = max(max(88, (int)(96 * m_uiScale)), (int)szBadge.cx + badgePadH * 2);

	CRect rcBadge(rcInner.left, rcInner.top, rcInner.left + badgeW, rcInner.bottom);
	DrawRoundRect(dc, rcBadge, badgeBg, badgeBorder, max(4, (int)(6 * m_uiScale)));

	CRect rcBadgeText = rcBadge;
	rcBadgeText.DeflateRect(badgePadH, max(2, (int)(3 * m_uiScale)));
	const int saveClip = dc.SaveDC();
	dc.IntersectClipRect(&rcBadgeText);
	DrawSingleLineLabel(dc, m_pFont, rcBadgeText, badgeText, badgeFg, DT_CENTER);
	dc.RestoreDC(saveClip);

	CRect rcChips = rcInner;
	rcChips.left = rcBadge.right + gap;
	const int chipW = max(80, (rcChips.Width() - gap * (MAX_CC16 - 1)) / MAX_CC16);
	for (int s = 0; s < MAX_CC16; s++)
	{
		CRect rcChip(
			rcChips.left + s * (chipW + gap), rcChips.top,
			rcChips.left + s * (chipW + gap) + chipW, rcChips.bottom);
		if (rcChip.right > rcChips.right)
			rcChip.right = rcChips.right;

		const bool online = m_slotOnline[s];
		const bool active = (s == m_activeTab);
		const DtCarFunction::Fa132SlotTestResult testResult = m_slotTestResult[s];
		const int ngCount = m_slotNgCount[s];
		COLORREF chipBg = online ? Fa132UiColors::kChipOnlineBg : Fa132UiColors::kChipOfflineBg;
		COLORREF chipBorder = online ? Fa132UiColors::kChipOnlineBorder : Fa132UiColors::kChipOfflineBorder;
		if (online && testResult == DtCarFunction::Fa132SlotResultNg)
		{
			chipBg = Fa132UiColors::kChipNgBg;
			chipBorder = Fa132UiColors::kChipNgBorder;
		}
		else if (online && testResult == DtCarFunction::Fa132SlotResultOk)
		{
			chipBg = Fa132UiColors::kChipOkRunBg;
			chipBorder = Fa132UiColors::kChipOkRunBorder;
		}
		DrawRoundRect(dc, rcChip, chipBg, chipBorder, max(4, (int)(5 * m_uiScale)));

		if (active)
		{
			CPen pen(PS_SOLID, 2, Fa132UiColors::kChipActiveRing);
			CPen* pOldPen = dc.SelectObject(&pen);
			CBrush* pNull = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
			CBrush* pOldBr = dc.SelectObject(pNull);
			CRect rcRing = rcChip;
			rcRing.DeflateRect(1, 1);
			dc.RoundRect(&rcRing, CPoint(max(4, (int)(5 * m_uiScale)), max(4, (int)(5 * m_uiScale))));
			dc.SelectObject(pOldBr);
			dc.SelectObject(pOldPen);
		}

		const int dotR = max(3, (int)(4 * m_uiScale));
		const int dotCx = rcChip.left + max(8, (int)(10 * m_uiScale));
		const int dotCy = rcChip.top + rcChip.Height() / 2;
		COLORREF dotFill = Fa132UiColors::kDotOffline;
		COLORREF dotBorder = RGB(148, 163, 184);
		if (online)
		{
			if (testResult == DtCarFunction::Fa132SlotResultNg)
			{
				dotFill = Fa132UiColors::kDotNg;
				dotBorder = RGB(185, 28, 28);
			}
			else
			{
				dotFill = Fa132UiColors::kDotOnline;
				dotBorder = RGB(22, 163, 74);
			}
		}
		DrawStatusDot(dc, dotCx, dotCy, dotR, dotFill, dotBorder);

		CString chipText;
		chipText.Format(_T("FA132-%d"), s + 1);
		if (!online)
			chipText.AppendFormat(_T("  %s"), (LPCTSTR)ZH_UTF8(kFa132StatusOffline));
		else if (testResult == DtCarFunction::Fa132SlotResultNg)
		{
			if (ngCount > 0)
				chipText.AppendFormat(ZH_UTF8(kFa132ChipNgInlineFmt), ngCount);
			else
				chipText += ZH_UTF8(kFa132ChipNgInline);
		}
		else if (testResult == DtCarFunction::Fa132SlotResultOk)
			chipText += ZH_UTF8(kFa132ChipOkInline);
		else
		{
			const int devLo = s * MAX_DEV;
			const int devHi = devLo + MAX_DEV - 1;
			chipText.AppendFormat(_T("  Dev%d-%d"), devLo, devHi);
		}

		COLORREF chipFg = RGB(148, 163, 184);
		if (online)
		{
			if (testResult == DtCarFunction::Fa132SlotResultNg)
				chipFg = Fa132UiColors::kTabNgFg;
			else if (testResult == DtCarFunction::Fa132SlotResultOk)
				chipFg = Fa132UiColors::kTabOkResultFg;
			else
				chipFg = active ? RGB(15, 23, 42) : RGB(22, 101, 52);
		}

		CRect rcText = rcChip;
		rcText.left = dotCx + dotR + max(5, (int)(6 * m_uiScale));
		rcText.DeflateRect(0, 1);
		DrawSingleLineLabel(dc, m_pFont, rcText, chipText, chipFg, DT_LEFT);
	}

	if (pOldFont != NULL)
		dc.SelectObject(pOldFont);
}
