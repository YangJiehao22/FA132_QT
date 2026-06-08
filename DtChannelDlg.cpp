#include "stdafx.h"
#include "DtChannelDlg.h"
#include "DtFileOperate.h"
#include "DtZhUtf8.h"
#include "DtEncoding.h"
#include "DtDpiUi.h"
#include "afxdialogex.h"

namespace {

static void MoveDlgItem(CWnd* dlg, UINT id, const CRect& r)
{
	if (dlg == NULL)
		return;
	CWnd* p = dlg->GetDlgItem(id);
	if (p != NULL && ::IsWindow(p->m_hWnd))
		p->SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER);
}

static int MeasureStaticTextHeight(CWnd* pwnd, int widthPx)
{
	if (pwnd == NULL || widthPx < 32)
		return 48;
	CString text;
	pwnd->GetWindowText(text);
	if (text.IsEmpty())
		return 48;
	CClientDC dc(pwnd);
	CFont* pf = pwnd->GetFont();
	if (pf != NULL)
		dc.SelectObject(pf);
	CRect rc(0, 0, widthPx, 0);
	dc.DrawText(text, &rc, DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX | DT_EXPANDTABS);
	return max(1, rc.Height());
}

} // namespace

CDtChannelCheckList::CDtChannelCheckList()
	: m_uiScale(1.0)
	, m_pChannelDlg(NULL)
{
}

void CDtChannelCheckList::SetUiScale(double scale)
{
	if (scale < 1.0)
		scale = 1.0;
	m_uiScale = scale;
}

int CDtChannelCheckList::CheckPx() const { return (int)(22 * m_uiScale); }
int CDtChannelCheckList::RowH() const { return (int)(36 * m_uiScale); }
int CDtChannelCheckList::PadL() const { return (int)(10 * m_uiScale); }
int CDtChannelCheckList::TextGap() const { return (int)(10 * m_uiScale); }

void CDtChannelCheckList::EnsureUiFont()
{
	DtCreateUiFont(m_font, 9, false);
	SetFont(&m_font);
}

void CDtChannelCheckList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct != NULL)
		lpMeasureItemStruct->itemHeight = RowH();
}

namespace {

static void DrawGroupCheckState(CDC& dc, const CRect& rcCheck, int state, BOOL bDisabled)
{
	UINT uState = DFCS_BUTTONCHECK;
	if (state == 1)
		uState |= DFCS_CHECKED;
	if (bDisabled)
		uState |= DFCS_INACTIVE;
	dc.DrawFrameControl((LPRECT)&rcCheck, DFC_BUTTON, uState);
	if (state == 2 && !bDisabled)
	{
		CRect rcMark = rcCheck;
		rcMark.DeflateRect(max(4, rcCheck.Width() / 4), max(4, rcCheck.Height() / 4));
		dc.FillSolidRect(&rcMark, RGB(55, 65, 81));
	}
}

} // namespace

void CDtChannelCheckList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == NULL)
		return;

	const int nItem = (int)lpDrawItemStruct->itemID;
	if (nItem < 0)
		return;

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	if (m_font.GetSafeHandle() != NULL)
		dc.SelectObject(&m_font);

	CRect rc = lpDrawItemStruct->rcItem;
	const BOOL bDisabled = (lpDrawItemStruct->itemState & ODS_DISABLED) != 0;
	const BOOL bSelected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	const bool isGroupHeader = (m_pChannelDlg != NULL && m_pChannelDlg->IsGroupHeaderRow(nItem));

	COLORREF clrBg = GetSysColor(COLOR_WINDOW);
	COLORREF clrText = GetSysColor(COLOR_WINDOWTEXT);
	if (isGroupHeader)
		clrBg = bDisabled ? RGB(248, 250, 252) : RGB(236, 242, 255);
	if (bSelected && !bDisabled)
	{
		clrBg = isGroupHeader ? RGB(219, 234, 254) : GetSysColor(COLOR_HIGHLIGHT);
		clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	dc.FillSolidRect(&rc, clrBg);

	const int cpx = CheckPx();
	const int padL = PadL();
	const int tgap = TextGap();
	CRect rcCheck(rc.left + padL, rc.top + (rc.Height() - cpx) / 2,
		rc.left + padL + cpx, rc.top + (rc.Height() - cpx) / 2 + cpx);

	if (isGroupHeader)
	{
		const int slot = m_pChannelDlg->GroupSlotForRow(nItem);
		const int grpState = m_pChannelDlg->GetFa132SlotCheckState(slot);
		DrawGroupCheckState(dc, rcCheck, grpState, bDisabled);
	}
	else
	{
		const BOOL bCheck = (GetCheck(nItem) != 0);
		UINT uState = DFCS_BUTTONCHECK;
		if (bCheck)
			uState |= DFCS_CHECKED;
		if (bDisabled)
			uState |= DFCS_INACTIVE;
		dc.DrawFrameControl(&rcCheck, DFC_BUTTON, uState);
	}

	CString text;
	GetText(nItem, text);
	CRect rcText(rcCheck.right + tgap, rc.top, rc.right - 4, rc.bottom);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(bDisabled ? RGB(148, 163, 184) : clrText);
	UINT textFmt = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;
	if (isGroupHeader)
		textFmt |= DT_NOPREFIX;
	dc.DrawText(text, &rcText, textFmt);

	dc.Detach();
}

int CDtChannelCheckList::CheckFromPoint(CPoint point, BOOL& bOnCheck) const
{
	bOnCheck = FALSE;
	const int nCount = GetCount();
	if (nCount <= 0)
		return -1;

	for (int i = 0; i < nCount; i++)
	{
		CRect rc;
		GetItemRect(i, &rc);
		if (!rc.PtInRect(point))
			continue;

		const int cpx = CheckPx();
		const int padL = PadL();
		CRect rcCheck(rc.left + padL, rc.top + (rc.Height() - cpx) / 2,
			rc.left + padL + cpx, rc.top + (rc.Height() - cpx) / 2 + cpx);
		rcCheck.InflateRect(4, 4);
		if (rcCheck.PtInRect(point))
			bOnCheck = TRUE;
		else
			bOnCheck = FALSE;
		return i;
	}
	return -1;
}

BEGIN_MESSAGE_MAP(CDtChannelCheckList, CCheckListBox)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CDtChannelCheckList::OnLButtonDown(UINT nFlags, CPoint point)
{
	BOOL onCheck = FALSE;
	const int idx = CheckFromPoint(point, onCheck);
	if (idx >= 0 && IsEnabled(idx))
	{
		if (m_pChannelDlg != NULL && m_pChannelDlg->IsGroupHeaderRow(idx))
		{
			m_pChannelDlg->OnGroupHeaderClicked(idx);
			SetCaretIndex(idx);
			Invalidate();
			return;
		}
		SetCheck(idx, GetCheck(idx) ? 0 : 1);
		SetCaretIndex(idx);
		Invalidate();
		if (CWnd* pParent = GetParent())
		{
			pParent->SendMessage(
				WM_COMMAND,
				MAKEWPARAM((WORD)GetDlgCtrlID(), CLBN_CHKCHANGE),
				(LPARAM)m_hWnd);
		}
		return;
	}
	CCheckListBox::OnLButtonDown(nFlags, point);
}

CDtChannelDlg::CDtChannelDlg(DtCarFunction* pFn, CWnd* pParent)
	: CDialogEx(IDD_DIALOG_CHANNEL, pParent)
	, m_pFn(pFn)
	, m_rowCount(0)
{
	memset(m_rowDev, 0, sizeof(m_rowDev));
	memset(m_rowVc, 0, sizeof(m_rowVc));
	memset(m_rowPickable, 0, sizeof(m_rowPickable));
	memset(m_rowIsGroupHeader, 0, sizeof(m_rowIsGroupHeader));
	memset(m_rowSlot, -1, sizeof(m_rowSlot));
}

void CDtChannelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CHANNEL, m_list);
}

BEGIN_MESSAGE_MAP(CDtChannelDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CH_ALL, &CDtChannelDlg::OnBnClickedSelectAll)
	ON_BN_CLICKED(IDC_BTN_CH_NONE, &CDtChannelDlg::OnBnClickedClearAll)
	ON_CLBN_CHKCHANGE(IDC_TREE_CHANNEL, &CDtChannelDlg::OnListCheckChange)
	ON_MESSAGE(WM_DPICHANGED, &CDtChannelDlg::OnDpiChanged)
END_MESSAGE_MAP()

LRESULT CDtChannelDlg::OnDpiChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LayoutChannelDialog();
	m_list.Invalidate(FALSE);
	return 0;
}

BOOL CDtChannelDlg::OnInitDialog()
{
	if (!CDialogEx::OnInitDialog())
		return FALSE;

	if (m_list.GetSafeHwnd() == NULL)
		m_list.SubclassDlgItem(IDC_TREE_CHANNEL, this);

	SetWindowText(ZH_UTF8(kDlgChannelTitle));
	if (CWnd* p = GetDlgItem(IDOK))
		p->SetWindowText(ZH_UTF8(kSpecOk));
	if (CWnd* p = GetDlgItem(IDCANCEL))
		p->SetWindowText(ZH_UTF8(kSpecCancel));
	if (CWnd* p = GetDlgItem(IDC_BTN_CH_ALL))
		p->SetWindowText(ZH_UTF8(kDlgChannelAll));
	if (CWnd* p = GetDlgItem(IDC_BTN_CH_NONE))
		p->SetWindowText(ZH_UTF8(kDlgChannelNone));
	if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
	{
		CString hint = ZH_UTF8(kDlgChannelHint);
		hint += _T("\r\n");
		hint += ZH_UTF8(kDlgChannelColHint);
		hint += _T("\r\n");
		hint += ZH_UTF8(kDlgChannelFa132HdrHint);
		p->SetWindowText(hint);
	}
	if (CWnd* p = GetDlgItem(IDC_STATIC_CH_GROUP))
		p->SetWindowText(ZH_UTF8(kDlgChannelGroup));

	if (m_list.GetSafeHwnd() != NULL)
	{
		const double scale = DtGetWindowUiScale(m_hWnd);
		m_list.SetUiScale(scale);
		m_list.SetChannelDlg(this);
		m_list.EnsureUiFont();
	}

	if (m_pFn == NULL || m_pFn->m_iEnumDevNum <= 0)
	{
		if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
			p->SetWindowText(ZH_UTF8(kDlgChannelNeedEnum));
		LayoutChannelDialog();
		UpdateSelectionStatus();
		return TRUE;
	}

	m_pFn->RefreshFa132SlotsAfterEnum();
	GetExePath(m_pFn->m_strDtCarIniPath);
	m_pFn->LoadChannelEnableIni();

	if (m_list.GetSafeHwnd() == NULL)
	{
		if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
			p->SetWindowText(_T("List init failed (IDC_TREE_CHANNEL). Rebuild project."));
		return TRUE;
	}

	BuildChannelList();
	ApplyChecksFromMemory();
	LayoutChannelDialog();
	UpdateSelectionStatus();
	return TRUE;
}

void CDtChannelDlg::LayoutChannelDialog()
{
	const double s = DtGetWindowUiScale(m_hWnd);
	m_list.SetUiScale(s);

	CRect wr;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0);
	const int margin = (int)(14 * s);
	const int btnH = (int)(30 * s);
	const int btnW = (int)(92 * s);
	const int btnGap = (int)(10 * s);
	const int clientW = (int)(480 * s);
	const int clientH = min((int)(wr.Height() * 88 / 100), (int)(580 * s));

	DWORD style = GetStyle();
	DWORD exStyle = GetExStyle();
	CRect want(0, 0, clientW, clientH);
	AdjustWindowRectEx(&want, style, FALSE, exStyle);
	int winW = want.Width();
	int winH = want.Height();
	int posX = wr.left + (wr.Width() - winW) / 2;
	int posY = wr.top + (wr.Height() - winH) / 2;
	SetWindowPos(NULL, posX, posY, winW, winH, SWP_NOZORDER);

	if (CFont* pf = m_list.GetFont())
	{
		for (UINT id : { IDC_STATIC_CH_HINT, IDC_STATIC_CH_GROUP, IDC_STATIC_CH_STATUS,
			IDC_BTN_CH_ALL, IDC_BTN_CH_NONE, IDOK, IDCANCEL })
		{
			if (CWnd* p = GetDlgItem(id))
				p->SetFont(pf);
		}
	}

	const int hintWidth = max(32, clientW - margin * 2);
	int hintH = (int)(52 * s);
	if (CWnd* pHint = GetDlgItem(IDC_STATIC_CH_HINT))
		hintH = max(hintH, MeasureStaticTextHeight(pHint, hintWidth) + (int)(12 * s));
	const int statusH = (int)(22 * s);
	const int btnRowY = clientH - margin - btnH;
	const int statusY = btnRowY - statusH - (int)(6 * s);
	const int listTop = margin + hintH + (int)(10 * s);
	const int listBottom = statusY - (int)(8 * s);
	const int grpTop = listTop - (int)(14 * s);

	MoveDlgItem(this, IDC_STATIC_CH_HINT, CRect(margin, margin, clientW - margin, margin + hintH));
	MoveDlgItem(this, IDC_STATIC_CH_GROUP, CRect(margin, grpTop, clientW - margin, listBottom + (int)(10 * s)));
	MoveDlgItem(this, IDC_TREE_CHANNEL, CRect(margin + (int)(8 * s), listTop, clientW - margin - (int)(8 * s), listBottom));
	MoveDlgItem(this, IDC_STATIC_CH_STATUS, CRect(margin + (int)(8 * s), statusY, clientW - margin - (int)(220 * s), statusY + statusH));
	MoveDlgItem(this, IDC_BTN_CH_ALL, CRect(clientW - margin - btnW * 2 - btnGap, btnRowY, clientW - margin - btnW - btnGap, btnRowY + btnH));
	MoveDlgItem(this, IDC_BTN_CH_NONE, CRect(clientW - margin - btnW, btnRowY, clientW - margin, btnRowY + btnH));
	MoveDlgItem(this, IDOK, CRect(margin, btnRowY, margin + btnW, btnRowY + btnH));
	MoveDlgItem(this, IDCANCEL, CRect(margin + btnW + btnGap, btnRowY, margin + btnW * 2 + btnGap, btnRowY + btnH));
}

void CDtChannelDlg::BuildChannelList()
{
	m_list.ResetContent();
	m_rowCount = 0;

	const int vcNum = (m_pFn->m_iVcNum > 0) ? m_pFn->m_iVcNum : 1;
	for (int slot = 0; slot < MAX_CC16; slot++)
	{
		CString hdr;
		hdr.Format(Utf8ToCString(DtZh::kDlgChannelFa132Hdr),
			slot + 1, slot * MAX_DEV, slot * MAX_DEV + MAX_DEV - 1);
		const int hdrIdx = m_list.AddString(hdr);
		if (hdrIdx < 0 || hdrIdx >= kChannelDlgMaxRows)
			continue;
		const bool slotOnline = m_pFn->IsFa132SlotOnline(slot);
		m_rowDev[hdrIdx] = -1;
		m_rowVc[hdrIdx] = -1;
		m_rowPickable[hdrIdx] = false;
		m_rowIsGroupHeader[hdrIdx] = true;
		m_rowSlot[hdrIdx] = slot;
		m_list.Enable(hdrIdx, slotOnline ? TRUE : FALSE);
		m_rowCount = hdrIdx + 1;

		for (int d = slot * MAX_DEV; d < (slot + 1) * MAX_DEV; d++)
		{
			CString serial;
			if (m_pFn->IsDevEnumPresent(d))
			{
				const char* name = m_pFn->m_cDeviceName[d];
				if (name != NULL && name[0] != 0)
					serial = name;
				else
					serial.Format(_T("Dev%d"), d);
			}
			else
				serial.Format(_T("Dev%d"), d);
			if (serial.GetLength() > 32)
				serial = serial.Left(29) + _T("...");

			const bool devPickable = slotOnline && m_pFn->IsDevEnumPresent(d);
			for (int v = 0; v < vcNum; v++)
			{
				CString line;
				const int ch = d * vcNum + v + 1;
				if (devPickable && v == 0)
					line.Format(_T("D%d  V%d  CH%03d  %s"), d, v, ch, (LPCTSTR)serial);
				else if (devPickable)
					line.Format(_T("D%d  V%d  CH%03d"), d, v, ch);
				else
					line.Format(_T("D%d  V%d  CH%03d  [offline]"), d, v, ch);

				const int idx = m_list.AddString(line);
				if (idx < 0 || idx >= kChannelDlgMaxRows)
					continue;
				m_rowDev[idx] = d;
				m_rowVc[idx] = v;
				m_rowPickable[idx] = devPickable;
				m_rowIsGroupHeader[idx] = false;
				m_rowSlot[idx] = slot;
				m_list.Enable(idx, devPickable ? TRUE : FALSE);
				m_rowCount = idx + 1;
			}
		}
	}
}

void CDtChannelDlg::ApplyChecksFromMemory()
{
	for (int i = 0; i < m_rowCount; i++)
	{
		if (!m_rowPickable[i])
			continue;
		const int d = m_rowDev[i];
		const int v = m_rowVc[i];
		const int on = (m_pFn->m_iVcEnable[d][v] != 0) ? 1 : 0;
		m_list.SetCheck(i, on);
	}
	m_list.Invalidate(FALSE);
}

void CDtChannelDlg::ReadChecksToMemory()
{
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
	{
		m_pFn->m_iDevEnable[d] = 0;
		for (int v = 0; v < MAX_VC; v++)
			m_pFn->m_iVcEnable[d][v] = 0;
	}

	for (int i = 0; i < m_rowCount; i++)
	{
		if (!m_rowPickable[i])
			continue;
		const int d = m_rowDev[i];
		const int v = m_rowVc[i];
		if (m_list.GetCheck(i) != 0)
		{
			m_pFn->m_iVcEnable[d][v] = 1;
			m_pFn->m_iDevEnable[d] = 1;
		}
	}
}

void CDtChannelDlg::UpdateSelectionStatus()
{
	int checked = 0;
	int pickable = 0;
	if (m_list.GetSafeHwnd() != NULL)
	{
		for (int i = 0; i < m_rowCount; i++)
		{
			if (!m_rowPickable[i])
				continue;
			pickable++;
			if (m_list.GetCheck(i) != 0)
				checked++;
		}
	}
	CString text;
	text.Format(ZH_UTF8(kDlgChannelStatus), checked, pickable);
	if (CWnd* p = GetDlgItem(IDC_STATIC_CH_STATUS))
		p->SetWindowText(text);
}

void CDtChannelDlg::OnListCheckChange()
{
	UpdateSelectionStatus();
	/* Refresh group header tri-state after VC row toggles. */
	m_list.Invalidate(FALSE);
}

bool CDtChannelDlg::IsGroupHeaderRow(int row) const
{
	return row >= 0 && row < m_rowCount && m_rowIsGroupHeader[row];
}

int CDtChannelDlg::GroupSlotForRow(int row) const
{
	if (!IsGroupHeaderRow(row))
		return -1;
	return m_rowSlot[row];
}

int CDtChannelDlg::GetFa132SlotCheckState(int slot)
{
	if (m_pFn == NULL || slot < 0 || slot >= MAX_CC16 || !m_pFn->IsFa132SlotOnline(slot))
		return 0;

	int pickable = 0;
	int checked = 0;
	const int devLo = slot * MAX_DEV;
	const int devHi = devLo + MAX_DEV;
	for (int i = 0; i < m_rowCount; i++)
	{
		if (!m_rowPickable[i])
			continue;
		const int d = m_rowDev[i];
		if (d < devLo || d >= devHi)
			continue;
		pickable++;
		if (m_list.GetCheck(i) != 0)
			checked++;
	}
	if (pickable <= 0)
		return 0;
	if (checked <= 0)
		return 0;
	if (checked >= pickable)
		return 1;
	return 2;
}

void CDtChannelDlg::SetFa132SlotChecks(int slot, bool checked)
{
	if (m_pFn == NULL || slot < 0 || slot >= MAX_CC16 || !m_pFn->IsFa132SlotOnline(slot))
		return;

	const int devLo = slot * MAX_DEV;
	const int devHi = devLo + MAX_DEV;
	for (int i = 0; i < m_rowCount; i++)
	{
		if (!m_rowPickable[i])
			continue;
		const int d = m_rowDev[i];
		if (d < devLo || d >= devHi)
			continue;
		m_list.SetCheck(i, checked ? 1 : 0);
	}
	m_list.Invalidate(FALSE);
	UpdateSelectionStatus();
}

void CDtChannelDlg::ToggleFa132SlotChecks(int slot)
{
	const int state = GetFa132SlotCheckState(slot);
	SetFa132SlotChecks(slot, state != 1);
}

void CDtChannelDlg::OnGroupHeaderClicked(int row)
{
	if (!IsGroupHeaderRow(row))
		return;
	const int slot = m_rowSlot[row];
	if (slot < 0 || m_pFn == NULL || !m_pFn->IsFa132SlotOnline(slot))
		return;
	ToggleFa132SlotChecks(slot);
}

void CDtChannelDlg::OnBnClickedSelectAll()
{
	for (int i = 0; i < m_rowCount; i++)
	{
		if (m_rowPickable[i])
			m_list.SetCheck(i, 1);
	}
	m_list.Invalidate(FALSE);
	UpdateSelectionStatus();
}

void CDtChannelDlg::OnBnClickedClearAll()
{
	for (int i = 0; i < m_rowCount; i++)
	{
		if (m_rowPickable[i])
			m_list.SetCheck(i, 0);
	}
	m_list.Invalidate(FALSE);
	UpdateSelectionStatus();
}

void CDtChannelDlg::OnOK()
{
	if (m_pFn != NULL && m_pFn->m_iEnumDevNum > 0 && m_rowCount > 0)
	{
		ReadChecksToMemory();
		if (!m_pFn->HasAnyChannelEnabled())
		{
			AfxMessageBox(ZH_UTF8(kDlgChannelPickOne), MB_ICONWARNING);
			return;
		}
		m_pFn->SaveChannelEnableIni();
	}
	CDialogEx::OnOK();
}
