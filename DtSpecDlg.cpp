#include "stdafx.h"
#include "DtSample.h"
#include "DtSpecDlg.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"
#include "DtDpiUi.h"
#include "afxdialogex.h"

#include <math.h>
#include <ShlObj.h>
#pragma comment(lib, "shell32.lib")

namespace {

static double ParseDouble(const CString& s)
{
	return atof(CStringA(s).GetString());
}

static void FormatDouble(CString& out, double v)
{
	out.Format(_T("%.3f"), v);
}

static unsigned int ParseHexUint(const CString& s)
{
	CString t = s;
	t.Trim();
	if (t.IsEmpty())
		return 0;
	if (t.Left(2).CompareNoCase(_T("0x")) == 0)
		t = t.Mid(2);
	unsigned int v = 0;
	_stscanf_s(t, _T("%x"), &v);
	return v;
}

static void FormatHex(CString& out, unsigned int v, int widthDigits)
{
	if (widthDigits <= 2)
		out.Format(_T("0x%02X"), v & 0xFF);
	else
		out.Format(_T("0x%04X"), v & 0xFFFF);
}

static CSize TextExtentForWnd(CWnd* pWnd, CDC& dc, LPCTSTR text)
{
	CFont* pUse = pWnd->GetFont();
	CFont* pOld = pUse ? dc.SelectObject(pUse) : NULL;
	const CSize sz = dc.GetTextExtent(text, (int)_tcslen(text));
	if (pOld)
		dc.SelectObject(pOld);
	return sz;
}

static CString BrowseFolder(HWND hOwner, const CString& initial)
{
	CString result;
	BROWSEINFO bi = {};
	bi.hwndOwner = hOwner;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	if (!initial.IsEmpty())
		bi.lpszTitle = initial;
	LPITEMIDLIST pidl = ::SHBrowseForFolder(&bi);
	if (pidl == NULL)
		return result;

	TCHAR path[MAX_PATH] = {};
	if (::SHGetPathFromIDList(pidl, path))
		result = path;
	::CoTaskMemFree(pidl);
	return result;
}

static void MoveWnd(CWnd* p, const CRect& r)
{
	if (p != NULL && ::IsWindow(p->m_hWnd))
		p->SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER);
}

static int LabelWidth(CWnd* pDlg, CWnd& lbl, double scale)
{
	CString t;
	lbl.GetWindowText(t);
	if (t.IsEmpty())
		return (int)(48 * scale);
	CClientDC dc(pDlg);
	return TextExtentForWnd(&lbl, dc, t).cx + (int)(10 * scale);
}

struct SpecLayoutMetrics
{
	int rowH;
	int chkRowH;
	int rowStep;
	int editH;
	int gap;
	int grpHdr;
	int grpPadB;
	int grpGap;
};

static SpecLayoutMetrics GetSpecLayoutMetrics(CWnd* pDlg, CFont& font, double scale)
{
	SpecLayoutMetrics m = {};
	CClientDC dc(pDlg);
	CFont* pOld = dc.SelectObject(&font);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);
	if (pOld)
		dc.SelectObject(pOld);

	const int sysChk = GetSystemMetrics(SM_CYMENUCHECK);
	m.rowH = tm.tmHeight + tm.tmExternalLeading + (int)(12 * scale);
	if (m.rowH < (int)(30 * scale))
		m.rowH = (int)(30 * scale);
	m.chkRowH = max(m.rowH, sysChk + (int)(14 * scale));
	if (m.chkRowH < (int)(38 * scale))
		m.chkRowH = (int)(38 * scale);
	m.gap = (int)(12 * scale);
	m.rowStep = m.chkRowH + m.gap;
	m.editH = max(m.rowH - 2, (int)(26 * scale));
	m.grpHdr = (int)(28 * scale);
	m.grpPadB = (int)(14 * scale);
	m.grpGap = (int)(16 * scale);
	return m;
}

static void ParkHiddenWnd(CWnd* p)
{
	MoveWnd(p, CRect(-4000, -4000, -3900, -3900));
}

static int SpecButtonHeight(CWnd* pDlg, CFont& font, double scale)
{
	if (pDlg == NULL)
		return (int)(34 * scale);
	CClientDC dc(pDlg);
	CFont* pOld = dc.SelectObject(&font);
	TEXTMETRIC tm = {};
	dc.GetTextMetrics(&tm);
	if (pOld)
		dc.SelectObject(pOld);
	const int h = tm.tmHeight + tm.tmExternalLeading + (int)(14 * scale);
	return max((int)(46 * scale), h);
}

static int MeasureStaticHeight(CWnd* pWnd, int width, const CString& text)
{
	if (pWnd == NULL || !::IsWindow(pWnd->m_hWnd) || text.IsEmpty())
		return 0;
	CRect rc(0, 0, max(width, 1), 0);
	CClientDC dc(pWnd);
	CFont* pOld = dc.SelectObject(pWnd->GetFont());
	dc.DrawText(text, &rc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
	if (pOld)
		dc.SelectObject(pOld);
	return rc.Height() + 6;
}

} // namespace

CDtSpecDlg::CDtSpecDlg(DtCarFunction* pFn, CWnd* pParent)
	: CDialogEx(CDtSpecDlg::IDD, pParent)
	, m_pFn(pFn)
	, m_specActivePage(0)
	, m_specFrameReady(false)
	, m_specLayoutScale(0.0)
	, m_specMargin(0)
	, m_specTabH(0)
	, m_specBtnBandH(0)
	, m_specBtnH(0)
	, m_specBtnW(0)
	, m_specScrollPos(0)
	, m_specContentH(0)
	, m_specVisibleContentH(0)
{
}

void CDtSpecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_SPEC, m_tab);
	DDX_Control(pDX, IDC_STATIC_SPEC_TITLE, m_stTitle);
	DDX_Control(pDX, IDC_STATIC_SPEC_PATH, m_stPath);
	DDX_Control(pDX, IDC_GRP_SPEC_TIMING, m_grpTiming);
	DDX_Control(pDX, IDC_LBL_SPEC_DELAY, m_lblDelay);
	DDX_Control(pDX, IDC_EDIT_SPEC_DELAY, m_edDelay);
	DDX_Control(pDX, IDC_GRP_SPEC_LIMITS, m_grpLimits);
	DDX_Control(pDX, IDC_LBL_DEF1, m_lblDef1);
	DDX_Control(pDX, IDC_LBL_DEF2, m_lblDef2);
	DDX_Control(pDX, IDC_LBL_DEF3, m_lblDef3);
	DDX_Control(pDX, IDC_LBL_DEF4, m_lblDef4);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MINSSR, m_edDefMinSsr);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MAXSSR, m_edDefMaxSsr);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MINCUR, m_edDefMinCur);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MAXCUR, m_edDefMaxCur);
	DDX_Control(pDX, IDC_LBL_DEF5, m_lblDef5);
	DDX_Control(pDX, IDC_LBL_DEF6, m_lblDef6);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MINTEMP, m_edDefMinTemp);
	DDX_Control(pDX, IDC_EDIT_SPEC_DEF_MAXTEMP, m_edDefMaxTemp);
	DDX_Control(pDX, IDC_GRP_SPEC_TEMP_I2C, m_grpTempI2c);
	DDX_Control(pDX, IDC_CHK_SPEC_TEMP_EN, m_chkTempEn);
	DDX_Control(pDX, IDC_LBL_TEMP_ADDR, m_lblTempAddr);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_ADDR, m_edTempAddr);
	DDX_Control(pDX, IDC_LBL_TEMP_MODE, m_lblTempMode);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_MODE, m_edTempMode);
	DDX_Control(pDX, IDC_LBL_TEMP_REGLO, m_lblTempRegLo);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_REGLO, m_edTempRegLo);
	DDX_Control(pDX, IDC_LBL_TEMP_REGHI, m_lblTempRegHi);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_REGHI, m_edTempRegHi);
	DDX_Control(pDX, IDC_LBL_TEMP_COEFFLO, m_lblTempCoeffLo);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_COEFFLO, m_edTempCoeffLo);
	DDX_Control(pDX, IDC_LBL_TEMP_COEFFHI, m_lblTempCoeffHi);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_COEFFHI, m_edTempCoeffHi);
	DDX_Control(pDX, IDC_LBL_TEMP_DIV, m_lblTempDiv);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_DIV, m_edTempDiv);
	DDX_Control(pDX, IDC_LBL_TEMP_OFFSET, m_lblTempOffset);
	DDX_Control(pDX, IDC_EDIT_SPEC_TEMP_OFFSET, m_edTempOffset);
	DDX_Control(pDX, IDC_STATIC_SPEC_FORMULA, m_stFormula);
	DDX_Control(pDX, IDC_STATIC_SPEC_HINT, m_stHint);
	DDX_Control(pDX, IDC_GRP_SPEC_BADPIXEL, m_grpBadPixel);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_EN, m_chkBpEn);
	DDX_Control(pDX, IDC_LBL_BP_ALGO, m_lblBpAlgo);
	DDX_Control(pDX, IDC_RAD_BP_ALGO_NEIGHBOR, m_radBpNeighbor);
	DDX_Control(pDX, IDC_RAD_BP_ALGO_HUAWEI, m_radBpHuawei);
	DDX_Control(pDX, IDC_GRP_BP_HUAWEI, m_grpBpHuawei);
	DDX_Control(pDX, IDC_GRP_BP_NEIGHBOR, m_grpBpNeighbor);
	DDX_Control(pDX, IDC_LBL_BP_CLUSTER_TH, m_lblBpClusterTh);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_CLUSTER_TH, m_edBpClusterTh);
	DDX_Control(pDX, IDC_LBL_BP_CLUSTER_MIN, m_lblBpClusterMin);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_CLUSTER_MIN, m_edBpClusterMin);
	DDX_Control(pDX, IDC_LBL_BP_SINGLE_PPM, m_lblBpSinglePpm);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_SINGLE_PPM, m_edBpSinglePpm);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_GRGBTOG, m_chkBpGrGbToG);
	DDX_Control(pDX, IDC_LBL_BP_MAX, m_lblBpMax);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_MAX, m_edBpMax);
	DDX_Control(pDX, IDC_LBL_BP_HOTDELTA, m_lblBpHotDelta);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_HOTDELTA, m_edBpHotDelta);
	DDX_Control(pDX, IDC_LBL_BP_HOTABS, m_lblBpHotAbs);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_HOTABS, m_edBpHotAbs);
	DDX_Control(pDX, IDC_LBL_BP_BORDER, m_lblBpBorder);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_BORDER, m_edBpBorder);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_SAVE, m_chkBpSave);
	DDX_Control(pDX, IDC_GRP_BP_SNAP_FILES, m_grpBpSnapFiles);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_SAVE_BMP, m_chkBpSaveBmp);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_SAVE_PACKED, m_chkBpSavePacked);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_SAVE_U12, m_chkBpSaveU12);
	DDX_Control(pDX, IDC_CHK_SPEC_BP_SAVE_U10, m_chkBpSaveU10);
	DDX_Control(pDX, IDC_LBL_BP_DIR, m_lblBpDir);
	DDX_Control(pDX, IDC_EDIT_SPEC_BP_DIR, m_edBpDir);
	DDX_Control(pDX, IDC_BTN_SPEC_BP_BROWSE, m_btnBpBrowse);
	DDX_Control(pDX, IDC_STATIC_SPEC_BP_HINT, m_stBpHint);
	DDX_Control(pDX, IDC_GRP_RESERVED2, m_grpFirmware);
	DDX_Control(pDX, IDC_CHK_SPEC_FW_EN, m_chkFwEn);
	DDX_Control(pDX, IDC_LBL_SPEC_FW_FOV, m_lblFwFov);
	DDX_Control(pDX, IDC_COMBO_SPEC_FW_FOV, m_cmbFwFov);
	DDX_Control(pDX, IDC_LBL_SPEC_FW_WARMUP, m_lblFwWarmup);
	DDX_Control(pDX, IDC_EDIT_SPEC_FW_WARMUP, m_edFwWarmup);
	DDX_Control(pDX, IDC_LBL_SPEC_FW_PATH, m_lblFwPath);
	DDX_Control(pDX, IDC_STATIC_SPEC_FW_PATH, m_stFwPath);
	DDX_Control(pDX, IDC_LBL_SPEC_FW_GRAB_INI, m_lblFwGrabIni);
	DDX_Control(pDX, IDC_EDIT_SPEC_FW_GRAB_INI, m_edFwGrabIni);
	DDX_Control(pDX, IDC_BTN_SPEC_FW_GRAB_BROWSE, m_btnFwGrabBrowse);
	DDX_Control(pDX, IDC_STATIC_SPEC_FW_HINT, m_stFwHint);
}

BEGIN_MESSAGE_MAP(CDtSpecDlg, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SPEC, &CDtSpecDlg::OnTcnSelchangeTabSpec)
	ON_BN_CLICKED(IDC_BTN_SPEC_BP_BROWSE, &CDtSpecDlg::OnBnClickedBtnBpBrowse)
	ON_BN_CLICKED(IDC_CHK_SPEC_BP_SAVE, &CDtSpecDlg::OnBnClickedBpSave)
	ON_BN_CLICKED(IDC_RAD_BP_ALGO_NEIGHBOR, &CDtSpecDlg::OnBnClickedRadBpAlgo)
	ON_BN_CLICKED(IDC_RAD_BP_ALGO_HUAWEI, &CDtSpecDlg::OnBnClickedRadBpAlgo)
	ON_CONTROL(CBN_SELCHANGE, IDC_COMBO_SPEC_FW_FOV, &CDtSpecDlg::OnCbnSelchangeFwFov)
	ON_BN_CLICKED(IDC_BTN_SPEC_FW_GRAB_BROWSE, &CDtSpecDlg::OnBnClickedBtnFwGrabBrowse)
	ON_BN_CLICKED(IDC_CHK_SPEC_FW_EN, &CDtSpecDlg::OnBnClickedChkFwEn)
	ON_MESSAGE(WM_DPICHANGED, &CDtSpecDlg::OnDpiChanged)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

BOOL CDtSpecDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect r;
	GetClientRect(&r);
	pDC->FillSolidRect(&r, RGB(246, 249, 252));
	return TRUE;
}

void CDtSpecDlg::UpdateFormulaText()
{
	CString f;
	f.Format(_T("Temp(C) = (b0*%.3g + b1*%.3g) / %.3g + %.3g  |  b0=RegLow, b1=RegHigh(0=skip)"),
		m_tempI2c.coeffLow, m_tempI2c.coeffHigh, m_tempI2c.divisor, m_tempI2c.offset);
	m_stFormula.SetWindowText(f);
}

double CDtSpecDlg::GetSpecUiScale() const
{
	return DtGetWindowUiScale(m_hWnd);
}

void CDtSpecDlg::HideGatePageControls()
{
	CWnd* wnds[] = {
		&m_stTitle, &m_stPath, &m_grpTiming, &m_lblDelay, &m_edDelay,
		&m_grpLimits, &m_lblDef1, &m_lblDef2, &m_lblDef3, &m_lblDef4,
		&m_lblDef5, &m_lblDef6, &m_edDefMinSsr, &m_edDefMaxSsr,
		&m_edDefMinCur, &m_edDefMaxCur, &m_edDefMinTemp, &m_edDefMaxTemp,
		&m_grpTempI2c, &m_chkTempEn, &m_lblTempAddr, &m_lblTempMode,
		&m_lblTempRegLo, &m_lblTempRegHi, &m_lblTempCoeffLo, &m_lblTempCoeffHi,
		&m_lblTempDiv, &m_lblTempOffset, &m_edTempAddr, &m_edTempMode,
		&m_edTempRegLo, &m_edTempRegHi, &m_edTempCoeffLo, &m_edTempCoeffHi,
		&m_edTempDiv, &m_edTempOffset, &m_stFormula, &m_stHint,
	};
	for (int i = 0; i < (int)(sizeof(wnds) / sizeof(wnds[0])); i++)
	{
		if (wnds[i] != NULL && ::IsWindow(wnds[i]->m_hWnd))
		{
			wnds[i]->ShowWindow(SW_HIDE);
			wnds[i]->EnableWindow(FALSE);
			ParkHiddenWnd(wnds[i]);
		}
	}
}

void CDtSpecDlg::HideBadPixelPageControls()
{
	CWnd* wnds[] = {
		&m_grpBadPixel, &m_chkBpEn, &m_lblBpAlgo, &m_radBpNeighbor, &m_radBpHuawei,
		&m_grpBpHuawei, &m_grpBpNeighbor, &m_lblBpClusterTh, &m_edBpClusterTh,
		&m_lblBpClusterMin, &m_edBpClusterMin, &m_lblBpSinglePpm, &m_edBpSinglePpm,
		&m_chkBpGrGbToG, &m_lblBpMax, &m_lblBpHotDelta, &m_edBpMax, &m_edBpHotDelta,
		&m_lblBpHotAbs, &m_edBpHotAbs, &m_lblBpBorder, &m_edBpBorder,
		&m_chkBpSave, &m_grpBpSnapFiles, &m_chkBpSaveBmp, &m_chkBpSavePacked,
		&m_chkBpSaveU12, &m_chkBpSaveU10, &m_lblBpDir, &m_edBpDir, &m_btnBpBrowse,
		&m_stBpHint,
	};
	for (int i = 0; i < (int)(sizeof(wnds) / sizeof(wnds[0])); i++)
	{
		if (wnds[i] != NULL && ::IsWindow(wnds[i]->m_hWnd))
		{
			wnds[i]->ShowWindow(SW_HIDE);
			wnds[i]->EnableWindow(FALSE);
			ParkHiddenWnd(wnds[i]);
		}
	}
}

void CDtSpecDlg::HideFirmwarePageControls()
{
	CWnd* wnds[] = {
		&m_grpFirmware, &m_chkFwEn, &m_lblFwFov, &m_lblFwWarmup, &m_edFwWarmup,
		&m_lblFwPath, &m_stFwPath, &m_lblFwGrabIni, &m_edFwGrabIni, &m_btnFwGrabBrowse,
		&m_stFwHint,
	};
	for (int i = 0; i < (int)(sizeof(wnds) / sizeof(wnds[0])); i++)
	{
		if (wnds[i] != NULL && ::IsWindow(wnds[i]->m_hWnd))
		{
			wnds[i]->ShowWindow(SW_HIDE);
			wnds[i]->EnableWindow(FALSE);
			ParkHiddenWnd(wnds[i]);
		}
	}
	if (::IsWindow(m_cmbFwFov.m_hWnd))
	{
		m_cmbFwFov.ShowWindow(SW_HIDE);
		m_cmbFwFov.EnableWindow(FALSE);
		ParkHiddenWnd(&m_cmbFwFov);
	}
}

void CDtSpecDlg::HideAllSpecPageControls()
{
	HideGatePageControls();
	HideBadPixelPageControls();
	HideFirmwarePageControls();
}

BOOL CDtSpecDlg::PreTranslateMessage(MSG* pMsg)
{
	return CDialogEx::PreTranslateMessage(pMsg);
}

int CDtSpecDlg::LayoutGatePage(const CRect& viewport, double scale, bool bShow)
{
	const bool allowShow = bShow && (m_specActivePage == 0);

	const SpecLayoutMetrics m = GetSpecLayoutMetrics(this, m_fontBody, scale);
	const int pad = (int)(14 * scale);
	const int x = viewport.left + pad;
	const int w = max(viewport.Width() - pad * 2, (int)(400 * scale));
	const int innerW = w - pad * 2;
	const int colW = (innerW - m.gap) / 2;
	const int maxEditW = (int)(140 * scale);
	const int y0 = viewport.top;

	auto show = [allowShow](CWnd& w) {
		if (!allowShow)
			return;
		w.EnableWindow(TRUE);
		w.ShowWindow(SW_SHOW);
	};

	auto place2col = [&](int cy, CStatic& lbl1, CEdit& ed1, CStatic& lbl2, CEdit& ed2) {
		auto oneCol = [&](int col, CStatic& lbl, CEdit& ed) {
			const int cx = x + pad + col * (colW + m.gap);
			int lw = LabelWidth(this, lbl, scale);
			const int maxLbl = colW - maxEditW - m.gap;
			if (lw > maxLbl)
				lw = maxLbl;
			const int ew = min(maxEditW, colW - lw - m.gap);
			MoveWnd(&lbl, CRect(cx, cy, cx + lw, cy + m.rowH));
			MoveWnd(&ed, CRect(cx + lw + m.gap, cy, cx + lw + m.gap + ew, cy + m.editH));
			show(lbl);
			show(ed);
		};
		oneCol(0, lbl1, ed1);
		oneCol(1, lbl2, ed2);
	};

	int y = y0;

	MoveWnd(&m_stTitle, CRect(x, y, x + w, y + m.rowH));
	show(m_stTitle);
	y += m.rowStep;
	MoveWnd(&m_stPath, CRect(x, y, x + w, y + m.rowH));
	show(m_stPath);
	y += m.rowStep + m.grpGap / 2;

	{
		const int gTop = y;
		const int gH = m.grpHdr + m.grpPadB + m.rowH;
		MoveWnd(&m_grpTiming, CRect(x, gTop, x + w, gTop + gH));
		show(m_grpTiming);
		const int cy = gTop + m.grpHdr;
		const int lw = LabelWidth(this, m_lblDelay, scale);
		const int ew = min(maxEditW, w - pad * 2 - lw - m.gap);
		MoveWnd(&m_lblDelay, CRect(x + pad, cy, x + pad + lw, cy + m.rowH));
		MoveWnd(&m_edDelay, CRect(x + pad + lw + m.gap, cy, x + pad + lw + m.gap + ew, cy + m.editH));
		show(m_lblDelay);
		show(m_edDelay);
		y = gTop + gH + m.grpGap;
	}

	{
		const int gTop = y;
		const int gH = m.grpHdr + m.grpPadB + m.rowH + m.rowStep * 2;
		MoveWnd(&m_grpLimits, CRect(x, gTop, x + w, gTop + gH));
		show(m_grpLimits);
		int cy = gTop + m.grpHdr;
		place2col(cy, m_lblDef1, m_edDefMinSsr, m_lblDef2, m_edDefMaxSsr);
		cy += m.rowStep;
		place2col(cy, m_lblDef3, m_edDefMinCur, m_lblDef4, m_edDefMaxCur);
		cy += m.rowStep;
		place2col(cy, m_lblDef5, m_edDefMinTemp, m_lblDef6, m_edDefMaxTemp);
		y = gTop + gH + m.grpGap;
	}

	{
		const int gTop = y;
		const int tempRows = 5;
		const int gH = m.grpHdr + m.grpPadB + m.chkRowH + m.rowStep * (tempRows - 1);
		MoveWnd(&m_grpTempI2c, CRect(x, gTop, x + w, gTop + gH));
		show(m_grpTempI2c);
		int cy = gTop + m.grpHdr;
		MoveWnd(&m_chkTempEn, CRect(x + pad, cy, x + w - pad, cy + m.chkRowH));
		show(m_chkTempEn);
		cy += m.rowStep;
		place2col(cy, m_lblTempAddr, m_edTempAddr, m_lblTempMode, m_edTempMode);
		cy += m.rowStep;
		place2col(cy, m_lblTempRegLo, m_edTempRegLo, m_lblTempRegHi, m_edTempRegHi);
		cy += m.rowStep;
		place2col(cy, m_lblTempCoeffLo, m_edTempCoeffLo, m_lblTempCoeffHi, m_edTempCoeffHi);
		cy += m.rowStep;
		place2col(cy, m_lblTempDiv, m_edTempDiv, m_lblTempOffset, m_edTempOffset);
		y = max(gTop + gH, cy + m.rowH) + m.grpGap;
	}

	const int formulaH = max(m.rowH + (int)(4 * scale), (int)(32 * scale));
	int formBottom = y + formulaH;
	if (viewport.bottom > y && formBottom > viewport.bottom)
		formBottom = viewport.bottom;
	if (formBottom > y)
	{
		MoveWnd(&m_stFormula, CRect(x, y, x + w, formBottom));
		show(m_stFormula);
		y = formBottom + m.gap;
	}

	CString hintText;
	m_stHint.GetWindowText(hintText);
	int hintH = max((int)(32 * scale), MeasureStaticHeight(&m_stHint, w, hintText));
	const int maxBottom = viewport.bottom;
	if (maxBottom > y && y + hintH > maxBottom)
		hintH = max(maxBottom - y, m.rowH);
	if (maxBottom > y && hintH > 0)
	{
		MoveWnd(&m_stHint, CRect(x, y, x + w, y + hintH));
		show(m_stHint);
		y += hintH + pad;
		if (y > maxBottom)
			y = maxBottom;
	}

	return y - y0;
}

int CDtSpecDlg::LayoutBadPixelPage(const CRect& viewport, double scale, bool bShow)
{
	const bool allowShow = bShow && (m_specActivePage == 1);

	if (allowShow)
		UpdateBadPixelLabels();

	const SpecLayoutMetrics m = GetSpecLayoutMetrics(this, m_fontBody, scale);
	const int pad = (int)(14 * scale);
	const int editW = (int)(88 * scale);
	const int x = viewport.left + pad;
	const int w = max(viewport.Width() - pad * 2, (int)(400 * scale));
	const int innerW = w - pad * 2;
	const int colW = (innerW - m.gap) / 2;
	const int y0 = viewport.top;
	int y = y0 + pad;

	const BOOL huawei = (m_radBpHuawei.GetCheck() == BST_CHECKED);

	auto show = [allowShow](CWnd& w) {
		if (!allowShow)
			return;
		w.EnableWindow(TRUE);
		w.ShowWindow(SW_SHOW);
	};

	m_grpBadPixel.ShowWindow(SW_HIDE);
	ParkHiddenWnd(&m_grpBadPixel);

	auto place2col = [&](int cy, CStatic& lbl1, CEdit& ed1, CStatic& lbl2, CEdit& ed2) {
		auto oneCol = [&](int col, CStatic& lbl, CEdit& ed) {
			const int cx = x + pad + col * (colW + m.gap);
			int lw = LabelWidth(this, lbl, scale);
			const int maxLbl = colW - editW - m.gap;
			if (lw > maxLbl)
				lw = maxLbl;
			MoveWnd(&lbl, CRect(cx, cy, cx + lw, cy + m.rowH));
			MoveWnd(&ed, CRect(cx + lw + m.gap, cy, cx + colW, cy + m.editH));
			show(lbl);
			show(ed);
		};
		oneCol(0, lbl1, ed1);
		oneCol(1, lbl2, ed2);
	};

	auto pairInGroup = [&](int gTop, int row, int col, CStatic& lbl, CEdit& ed) {
		const int cx = x + pad + col * (colW + m.gap);
		const int yy = gTop + m.grpHdr + row * m.rowStep;
		int lw = LabelWidth(this, lbl, scale);
		const int maxLbl = colW - editW - m.gap;
		if (lw > maxLbl)
			lw = maxLbl;
		MoveWnd(&lbl, CRect(cx, yy, cx + lw, yy + m.rowH));
		MoveWnd(&ed, CRect(cx + lw + m.gap, yy, cx + colW, yy + m.editH));
		show(lbl);
		show(ed);
	};

	MoveWnd(&m_chkBpEn, CRect(x, y, x + w, y + m.chkRowH));
	show(m_chkBpEn);
	y += m.rowStep;

	const int lblAlgoW = LabelWidth(this, m_lblBpAlgo, scale);
	const int radAreaW = w - lblAlgoW - m.gap;
	const int radW = (radAreaW - m.gap) / 2;
	MoveWnd(&m_lblBpAlgo, CRect(x, y, x + lblAlgoW, y + m.rowH));
	MoveWnd(&m_radBpNeighbor, CRect(x + lblAlgoW + m.gap, y, x + lblAlgoW + m.gap + radW, y + m.chkRowH));
	MoveWnd(&m_radBpHuawei, CRect(x + lblAlgoW + m.gap + radW + m.gap, y, x + w, y + m.chkRowH));
	show(m_lblBpAlgo);
	show(m_radBpNeighbor);
	show(m_radBpHuawei);
	y += m.rowStep + m.gap;

	if (huawei)
	{
		m_grpBpNeighbor.ShowWindow(SW_HIDE);
		ParkHiddenWnd(&m_grpBpNeighbor);

		const int gTop = y;
		const int gH = m.grpHdr + m.grpPadB + m.rowH + m.rowStep;
		MoveWnd(&m_grpBpHuawei, CRect(x, gTop, x + w, gTop + gH));
		show(m_grpBpHuawei);
		pairInGroup(gTop, 0, 0, m_lblBpClusterTh, m_edBpClusterTh);
		pairInGroup(gTop, 0, 1, m_lblBpClusterMin, m_edBpClusterMin);
		pairInGroup(gTop, 1, 0, m_lblBpSinglePpm, m_edBpSinglePpm);
		{
			const int yy = gTop + m.grpHdr + m.rowStep;
			const int cx = x + pad + colW + m.gap;
			MoveWnd(&m_chkBpGrGbToG, CRect(cx, yy, cx + colW, yy + m.chkRowH));
			show(m_chkBpGrGbToG);
		}

		m_lblBpHotAbs.ShowWindow(SW_HIDE);
		m_edBpHotAbs.ShowWindow(SW_HIDE);
		m_lblBpBorder.ShowWindow(SW_HIDE);
		m_edBpBorder.ShowWindow(SW_HIDE);
		ParkHiddenWnd(&m_lblBpHotAbs);
		ParkHiddenWnd(&m_edBpHotAbs);
		ParkHiddenWnd(&m_lblBpBorder);
		ParkHiddenWnd(&m_edBpBorder);

		y = gTop + gH + m.grpGap;
	}
	else
	{
		m_grpBpHuawei.ShowWindow(SW_HIDE);
		ParkHiddenWnd(&m_grpBpHuawei);

		m_lblBpClusterTh.ShowWindow(SW_HIDE);
		m_edBpClusterTh.ShowWindow(SW_HIDE);
		m_lblBpClusterMin.ShowWindow(SW_HIDE);
		m_edBpClusterMin.ShowWindow(SW_HIDE);
		m_lblBpSinglePpm.ShowWindow(SW_HIDE);
		m_edBpSinglePpm.ShowWindow(SW_HIDE);
		m_chkBpGrGbToG.ShowWindow(SW_HIDE);
		ParkHiddenWnd(&m_lblBpClusterTh);
		ParkHiddenWnd(&m_edBpClusterTh);
		ParkHiddenWnd(&m_lblBpClusterMin);
		ParkHiddenWnd(&m_edBpClusterMin);
		ParkHiddenWnd(&m_lblBpSinglePpm);
		ParkHiddenWnd(&m_edBpSinglePpm);
		ParkHiddenWnd(&m_chkBpGrGbToG);

		const int gTop = y;
		const int gH = m.grpHdr + m.grpPadB + m.rowH;
		MoveWnd(&m_grpBpNeighbor, CRect(x, gTop, x + w, gTop + gH));
		show(m_grpBpNeighbor);
		const int nbLblW = max(LabelWidth(this, m_lblBpHotAbs, scale), LabelWidth(this, m_lblBpBorder, scale));
		const int nbYy = gTop + m.grpHdr;
		MoveWnd(&m_lblBpHotAbs, CRect(x + pad, nbYy, x + pad + nbLblW, nbYy + m.rowH));
		MoveWnd(&m_edBpHotAbs, CRect(x + pad + nbLblW + m.gap, nbYy, x + pad + colW, nbYy + m.editH));
		MoveWnd(&m_lblBpBorder, CRect(x + pad + colW + m.gap, nbYy, x + pad + colW + m.gap + nbLblW, nbYy + m.rowH));
		MoveWnd(&m_edBpBorder, CRect(x + pad + colW + m.gap + nbLblW + m.gap, nbYy, x + w - pad, nbYy + m.editH));
		show(m_lblBpHotAbs);
		show(m_edBpHotAbs);
		show(m_lblBpBorder);
		show(m_edBpBorder);
		y = gTop + gH + m.grpGap;
	}

	place2col(y, m_lblBpMax, m_edBpMax, m_lblBpHotDelta, m_edBpHotDelta);
	y += m.rowStep + m.gap;

	MoveWnd(&m_chkBpSave, CRect(x, y, x + w, y + m.chkRowH));
	show(m_chkBpSave);
	y += m.rowStep;

	const int snapTop = y;
	const int snapH = m.grpHdr + m.grpPadB + m.chkRowH + m.rowStep * 3;
	MoveWnd(&m_grpBpSnapFiles, CRect(x, snapTop, x + w, snapTop + snapH));
	show(m_grpBpSnapFiles);
	int sy = snapTop + m.grpHdr;
	CButton* snapChk[4] = { &m_chkBpSaveBmp, &m_chkBpSavePacked, &m_chkBpSaveU12, &m_chkBpSaveU10 };
	for (int i = 0; i < 4; i++)
	{
		MoveWnd(snapChk[i], CRect(x + pad, sy, x + w - pad, sy + m.chkRowH));
		show(*snapChk[i]);
		sy += m.rowStep;
	}
	y = snapTop + snapH + m.grpGap;

	const int btnW = (int)(80 * scale);
	const int dirLblW = LabelWidth(this, m_lblBpDir, scale);
	MoveWnd(&m_lblBpDir, CRect(x, y, x + dirLblW, y + m.rowH));
	MoveWnd(&m_btnBpBrowse, CRect(x + w - btnW, y, x + w, y + m.rowH));
	MoveWnd(&m_edBpDir, CRect(x + dirLblW + m.gap, y, x + w - btnW - m.gap, y + m.editH));
	show(m_lblBpDir);
	show(m_btnBpBrowse);
	show(m_edBpDir);
	y += m.rowStep + m.gap;

	CString hintText;
	m_stBpHint.GetWindowText(hintText);
	const int hintH = max((int)(56 * scale), MeasureStaticHeight(&m_stBpHint, w, hintText));
	MoveWnd(&m_stBpHint, CRect(x, y, x + w, y + hintH));
	show(m_stBpHint);
	y += hintH + pad;

	return y - y0;
}

int CDtSpecDlg::LayoutFirmwarePage(const CRect& viewport, double scale, bool bShow)
{
	const bool allowShow = bShow && (m_specActivePage == 2);

	const SpecLayoutMetrics m = GetSpecLayoutMetrics(this, m_fontBody, scale);
	const int pad = (int)(16 * scale);
	const int x = viewport.left + pad;
	const int w = max(viewport.Width() - pad * 2, (int)(400 * scale));
	const int y0 = viewport.top;
	int y = y0 + pad;

	auto show = [allowShow](CWnd& w) {
		if (!allowShow)
			return;
		w.EnableWindow(TRUE);
		w.ShowWindow(SW_SHOW);
	};

	const int chkH = m.chkRowH;
	const int fovLblW = LabelWidth(this, m_lblFwFov, scale);
	const int warmLblW = LabelWidth(this, m_lblFwWarmup, scale);
	const int pathLblW = LabelWidth(this, m_lblFwPath, scale);
	const int grabLblW = LabelWidth(this, m_lblFwGrabIni, scale);
	const int browseW = max((int)(64 * scale), 56);
	const int comboH = m.editH + max((int)(120 * scale), 100);
	CString hintText;
	m_stFwHint.GetWindowText(hintText);
	const int hintH = max((int)(48 * scale), MeasureStaticHeight(&m_stFwHint, w - pad * 2, hintText));
	const int gH = m.grpHdr + m.grpPadB + chkH + m.rowStep + comboH + m.gap
		+ m.rowStep + m.rowH + m.rowStep + m.rowH + m.rowStep + m.rowH + m.gap + hintH;

	MoveWnd(&m_grpFirmware, CRect(x, y, x + w, y + gH));
	int gy = y + m.grpHdr;
	const int ix = x + pad;
	const int iw = w - pad * 2;

	MoveWnd(&m_chkFwEn, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkFwEn);
	gy += m.rowStep;

	MoveWnd(&m_lblFwFov, CRect(ix, gy, ix + fovLblW, gy + m.rowH));
	show(m_lblFwFov);
	MoveWnd(&m_cmbFwFov, CRect(ix + fovLblW + m.gap, gy, ix + iw, gy + comboH));
	m_cmbFwFov.SetItemHeight(-1, m.rowH);
	show(m_cmbFwFov);
	gy += comboH + m.gap;

	MoveWnd(&m_lblFwWarmup, CRect(ix, gy, ix + warmLblW, gy + m.rowH));
	MoveWnd(&m_edFwWarmup, CRect(ix + warmLblW + m.gap, gy, ix + warmLblW + m.gap + (int)(80 * scale), gy + m.editH));
	show(m_lblFwWarmup);
	show(m_edFwWarmup);
	gy += m.rowStep;

	MoveWnd(&m_lblFwPath, CRect(ix, gy, ix + pathLblW, gy + m.rowH));
	MoveWnd(&m_stFwPath, CRect(ix + pathLblW + m.gap, gy, ix + iw, gy + m.rowH));
	show(m_lblFwPath);
	show(m_stFwPath);
	gy += m.rowStep;

	MoveWnd(&m_lblFwGrabIni, CRect(ix, gy, ix + grabLblW, gy + m.rowH));
	MoveWnd(&m_edFwGrabIni, CRect(ix + grabLblW + m.gap, gy, ix + iw - browseW - m.gap, gy + m.editH));
	MoveWnd(&m_btnFwGrabBrowse, CRect(ix + iw - browseW, gy, ix + iw, gy + m.editH));
	show(m_lblFwGrabIni);
	show(m_edFwGrabIni);
	show(m_btnFwGrabBrowse);
	gy += m.rowStep + m.gap;

	MoveWnd(&m_stFwHint, CRect(ix, gy, ix + iw, gy + hintH));
	show(m_stFwHint);
	if (allowShow)
	{
		m_grpFirmware.EnableWindow(TRUE);
		m_grpFirmware.ShowWindow(SW_SHOW);
	}
	y += gH + pad;

	return y - y0;
}

void CDtSpecDlg::ResizeSpecClient(int clientW, int clientH)
{
	CRect wr;
	GetWindowRect(&wr);
	CRect rc(0, 0, clientW, clientH);
	CalcWindowRect(&rc, CWnd::adjustOutside);
	SetWindowPos(NULL, wr.left, wr.top, rc.Width(), rc.Height(), SWP_NOZORDER);
}

void CDtSpecDlg::ClampSpecDialogToWorkArea(const CRect& workArea)
{
	CRect wr;
	GetWindowRect(&wr);
	int x = wr.left;
	int y = wr.top;
	if (wr.right > workArea.right)
		x = workArea.right - wr.Width();
	if (wr.left < workArea.left)
		x = workArea.left;
	if (wr.bottom > workArea.bottom)
		y = workArea.bottom - wr.Height();
	if (wr.top < workArea.top)
		y = workArea.top;
	if (x != wr.left || y != wr.top)
		SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

int CDtSpecDlg::SpecChromeHeightPx(double scale) const
{
	const int margin = (int)(16 * scale);
	const int tabH = (int)(32 * scale);
	CDtSpecDlg* pDlg = const_cast<CDtSpecDlg*>(this);
	const int btnH = SpecButtonHeight(pDlg, pDlg->m_fontBody, scale);
	const int btnBand = btnH + (int)(28 * scale);
	return margin + tabH + margin + btnBand + (int)(12 * scale);
}

void CDtSpecDlg::ApplySpecScroll(int newPos)
{
	if (!(GetStyle() & WS_VSCROLL))
		return;

	const int maxPos = max(0, m_specContentH - m_specVisibleContentH);
	if (newPos < 0)
		newPos = 0;
	if (newPos > maxPos)
		newPos = maxPos;
	if (newPos == m_specScrollPos)
		return;

	m_specScrollPos = newPos;
	SetScrollPos(SB_VERT, m_specScrollPos, TRUE);
	RelayoutSpecPage();
}

void CDtSpecDlg::ClipActiveSpecPageControls(int visTop, int visBottom)
{
	if (!(GetStyle() & WS_VSCROLL))
		return;

	auto clipList = [&](CWnd* wnds[], int count) {
		for (int i = 0; i < count; i++)
		{
			CWnd* p = wnds[i];
			if (p == NULL || !::IsWindow(p->m_hWnd) || !p->IsWindowVisible())
				continue;
			CRect rc;
			p->GetWindowRect(&rc);
			ScreenToClient(&rc);
			if (rc.bottom <= visTop + 1 || rc.top >= visBottom - 1)
			{
				p->ShowWindow(SW_HIDE);
				p->EnableWindow(FALSE);
			}
		}
	};

	if (m_specActivePage == 0)
	{
		CWnd* wnds[] = {
			&m_stTitle, &m_stPath, &m_grpTiming, &m_lblDelay, &m_edDelay,
			&m_grpLimits, &m_lblDef1, &m_lblDef2, &m_lblDef3, &m_lblDef4,
			&m_lblDef5, &m_lblDef6, &m_edDefMinSsr, &m_edDefMaxSsr,
			&m_edDefMinCur, &m_edDefMaxCur, &m_edDefMinTemp, &m_edDefMaxTemp,
			&m_grpTempI2c, &m_chkTempEn, &m_lblTempAddr, &m_lblTempMode,
			&m_lblTempRegLo, &m_lblTempRegHi, &m_lblTempCoeffLo, &m_lblTempCoeffHi,
			&m_lblTempDiv, &m_lblTempOffset, &m_edTempAddr, &m_edTempMode,
			&m_edTempRegLo, &m_edTempRegHi, &m_edTempCoeffLo, &m_edTempCoeffHi,
			&m_edTempDiv, &m_edTempOffset, &m_stFormula, &m_stHint,
		};
		clipList(wnds, (int)(sizeof(wnds) / sizeof(wnds[0])));
	}
	else if (m_specActivePage == 1)
	{
		CWnd* wnds[] = {
			&m_grpBadPixel, &m_chkBpEn, &m_lblBpAlgo, &m_radBpNeighbor, &m_radBpHuawei,
			&m_grpBpHuawei, &m_grpBpNeighbor, &m_lblBpClusterTh, &m_edBpClusterTh,
			&m_lblBpClusterMin, &m_edBpClusterMin, &m_lblBpSinglePpm, &m_edBpSinglePpm,
			&m_chkBpGrGbToG, &m_lblBpMax, &m_lblBpHotDelta, &m_edBpMax, &m_edBpHotDelta,
			&m_lblBpHotAbs, &m_edBpHotAbs, &m_lblBpBorder, &m_edBpBorder,
			&m_chkBpSave, &m_grpBpSnapFiles, &m_chkBpSaveBmp, &m_chkBpSavePacked,
			&m_chkBpSaveU12, &m_chkBpSaveU10, &m_lblBpDir, &m_edBpDir, &m_btnBpBrowse,
			&m_stBpHint,
		};
		clipList(wnds, (int)(sizeof(wnds) / sizeof(wnds[0])));
	}
	else
	{
		CWnd* wnds[] = {
			&m_grpFirmware, &m_chkFwEn, &m_lblFwFov, &m_lblFwWarmup, &m_edFwWarmup,
			&m_lblFwPath, &m_stFwPath, &m_lblFwGrabIni, &m_edFwGrabIni, &m_btnFwGrabBrowse,
			&m_stFwHint, &m_cmbFwFov,
		};
		clipList(wnds, (int)(sizeof(wnds) / sizeof(wnds[0])));
	}
}

void CDtSpecDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (!(GetStyle() & WS_VSCROLL))
	{
		CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si, TRUE);

	int pos = m_specScrollPos;
	const int line = max((int)(24 * m_specLayoutScale), 8);
	switch (nSBCode)
	{
	case SB_LINEUP:
		pos -= line;
		break;
	case SB_LINEDOWN:
		pos += line;
		break;
	case SB_PAGEUP:
		pos -= (int)si.nPage;
		break;
	case SB_PAGEDOWN:
		pos += (int)si.nPage;
		break;
	case SB_TOP:
		pos = 0;
		break;
	case SB_BOTTOM:
		pos = max(0, (int)(si.nMax - (int)si.nPage));
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		pos = (int)nPos;
		break;
	default:
		CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	ApplySpecScroll(pos);
	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CDtSpecDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (GetStyle() & WS_VSCROLL)
	{
		const int step = max((int)(48 * m_specLayoutScale), 16);
		ApplySpecScroll(m_specScrollPos - (zDelta / WHEEL_DELTA) * step);
		return TRUE;
	}
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

int CDtSpecDlg::MaxClientHeightForWorkArea(int clientW, const CRect& workArea, double scale) const
{
	const int maxOuterH = workArea.Height() - (int)(16 * scale);
	if (maxOuterH <= 0)
		return (int)(480 * scale);

	CDtSpecDlg* pDlg = const_cast<CDtSpecDlg*>(this);
	auto outerHForClient = [&](int clientH) -> int {
		CRect rc(0, 0, clientW, clientH);
		pDlg->CalcWindowRect(&rc, CWnd::adjustOutside);
		return rc.Height();
	};

	int lo = (int)(360 * scale);
	int hi = (int)(2000 * scale);
	while (lo < hi)
	{
		const int mid = (lo + hi + 1) / 2;
		if (outerHForClient(mid) <= maxOuterH)
			lo = mid;
		else
			hi = mid - 1;
	}
	return lo;
}

void CDtSpecDlg::PlaceSpecTabAndButtons(const CRect& cr)
{
	const int btnGap = (int)(12 * m_specLayoutScale);
	const int btnMargin = max((int)(10 * m_specLayoutScale), m_specMargin / 2);
	int btnTop = cr.bottom - m_specBtnH - btnMargin;
	const int minBtnTop = m_specMargin + m_specTabH + m_specMargin;
	if (btnTop < minBtnTop)
		btnTop = minBtnTop;

	const int okLeft = cr.right - m_specMargin - m_specBtnW - btnGap - m_specBtnW;
	const int cancelLeft = cr.right - m_specMargin - m_specBtnW;
	const CRect okRc(okLeft, btnTop, okLeft + m_specBtnW, btnTop + m_specBtnH);
	const CRect cancelRc(cancelLeft, btnTop, cancelLeft + m_specBtnW, btnTop + m_specBtnH);

	if (CWnd* pTab = GetDlgItem(IDC_TAB_SPEC))
		pTab->SetWindowPos(NULL, m_specMargin, m_specMargin, cr.Width() - m_specMargin * 2, m_specTabH, SWP_NOZORDER);

	if (CWnd* pOk = GetDlgItem(IDOK))
	{
		pOk->SetWindowText(ZH_UTF8(kSpecOk));
		pOk->EnableWindow(TRUE);
		MoveWnd(pOk, okRc);
		pOk->ShowWindow(SW_SHOW);
	}
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
	{
		pCancel->SetWindowText(ZH_UTF8(kSpecCancel));
		pCancel->EnableWindow(TRUE);
		MoveWnd(pCancel, cancelRc);
		pCancel->ShowWindow(SW_SHOW);
	}

	RaiseSpecDialogButtons();
}

void CDtSpecDlg::RaiseSpecDialogButtons()
{
	const UINT kBtnIds[] = { IDOK, IDCANCEL };
	for (int i = 0; i < 2; i++)
	{
		if (CWnd* pBtn = GetDlgItem(kBtnIds[i]))
		{
			if (::IsWindow(pBtn->m_hWnd))
				pBtn->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
}

int CDtSpecDlg::MeasureMaxPageHeight(const CRect& viewport, double scale)
{
	const int savedPage = m_specActivePage;
	int maxH = 0;
	for (int p = 0; p < 3; p++)
	{
		m_specActivePage = p;
		int h = 0;
		if (p == 0)
		{
			UpdateFormulaText();
			h = LayoutGatePage(viewport, scale, false);
		}
		else if (p == 1)
			h = LayoutBadPixelPage(viewport, scale, false);
		else
			h = LayoutFirmwarePage(viewport, scale, false);
		if (h > maxH)
			maxH = h;
	}
	m_specActivePage = savedPage;
	return maxH;
}

void CDtSpecDlg::InitSpecDialogFrame(bool force)
{
	const double dpiScale = GetSpecUiScale();
	if (!force && m_specFrameReady && fabs(dpiScale - m_specLayoutScale) < 0.01)
		return;

	CRect wr;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0);

	double fitScale = dpiScale;
	int contentH = 0;
	int wantW = 0;

	for (double s = dpiScale; s >= 0.82; s -= 0.025)
	{
		const int margin = (int)(16 * s);
		wantW = min((int)(720 * s), wr.Width() - (int)(32 * s));
		const int contentWidth = max(wantW - margin * 2, (int)(480 * s));
		const CRect measureArea(0, 0, contentWidth, 20000);

		HideAllSpecPageControls();
		contentH = MeasureMaxPageHeight(measureArea, s);
		HideAllSpecPageControls();

		const int chromeH = SpecChromeHeightPx(s);
		const int maxClientH = MaxClientHeightForWorkArea(wantW, wr, s);
		fitScale = s;
		if (contentH + chromeH <= maxClientH)
			break;
	}

	m_specLayoutScale = fitScale;
	m_specMargin = (int)(16 * fitScale);
	m_specTabH = (int)(32 * fitScale);
	m_specBtnH = SpecButtonHeight(this, m_fontBody, fitScale);
	m_specBtnW = (int)(100 * fitScale);
	m_specBtnBandH = m_specBtnH + (int)(28 * fitScale);

	wantW = min((int)(720 * fitScale), wr.Width() - (int)(32 * fitScale));
	const int contentWidth = max(wantW - m_specMargin * 2, (int)(480 * fitScale));
	const CRect measureArea(0, 0, contentWidth, 20000);

	HideAllSpecPageControls();
	contentH = MeasureMaxPageHeight(measureArea, fitScale);
	HideAllSpecPageControls();
	m_specContentH = contentH;

	const int chromeH = SpecChromeHeightPx(fitScale);
	int wantClientH = contentH + chromeH;
	const int minClientH = m_specBtnBandH + m_specTabH + m_specMargin * 3 + (int)(120 * fitScale);
	if (wantClientH < minClientH)
		wantClientH = minClientH;

	const int maxClientH = MaxClientHeightForWorkArea(wantW, wr, fitScale);
	if (wantClientH > maxClientH)
		wantClientH = maxClientH;

	ResizeSpecClient(wantW, wantClientH);

	CRect wrWin;
	GetWindowRect(&wrWin);
	const int posX = wr.left + (wr.Width() - wrWin.Width()) / 2;
	const int posY = wr.top + (wr.Height() - wrWin.Height()) / 2;
	SetWindowPos(NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	ClampSpecDialogToWorkArea(wr);

	CRect cr;
	GetClientRect(&cr);
	PlaceSpecTabAndButtons(cr);

	const int contentChrome = m_specMargin + m_specTabH + m_specMargin + m_specBtnBandH;
	m_specVisibleContentH = max(0, cr.bottom - contentChrome);
	if (m_specVisibleContentH < (int)(100 * fitScale))
		m_specVisibleContentH = (int)(100 * fitScale);

	const BOOL needScroll = (contentH > m_specVisibleContentH + (int)(4 * fitScale));
	if (needScroll)
	{
		ModifyStyle(0, WS_VSCROLL);
		const int maxPos = max(0, contentH - m_specVisibleContentH);
		if (m_specScrollPos > maxPos)
			m_specScrollPos = maxPos;

		SCROLLINFO si = {};
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin = 0;
		si.nMax = contentH;
		si.nPage = (UINT)m_specVisibleContentH;
		si.nPos = m_specScrollPos;
		SetScrollInfo(SB_VERT, &si, TRUE);
		ShowScrollBar(SB_VERT, TRUE);
	}
	else
	{
		m_specScrollPos = 0;
		ModifyStyle(WS_VSCROLL, 0);
		ShowScrollBar(SB_VERT, FALSE);
	}

	m_specFrameReady = true;
}

void CDtSpecDlg::RelayoutSpecPage()
{
	if (!m_specFrameReady)
		InitSpecDialogFrame(false);

	const double scale = m_specLayoutScale;
	const int page = m_specActivePage;

	CRect cr;
	GetClientRect(&cr);

	const int contentTop = m_specMargin + m_specTabH + m_specMargin;
	const int contentBottom = cr.bottom - m_specBtnBandH;
	const int visH = max(contentBottom - contentTop, (int)(80 * scale));
	const int layoutH = max(m_specContentH, visH);
	const CRect layoutVp(
		m_specMargin,
		contentTop - m_specScrollPos,
		cr.right - m_specMargin,
		contentTop - m_specScrollPos + layoutH);

	HideAllSpecPageControls();
	if (page == 0)
	{
		UpdateFormulaText();
		LayoutGatePage(layoutVp, scale, true);
	}
	else if (page == 1)
		LayoutBadPixelPage(layoutVp, scale, true);
	else
		LayoutFirmwarePage(layoutVp, scale, true);

	if (page != 0)
		HideGatePageControls();
	if (page != 1)
		HideBadPixelPageControls();
	if (page != 2)
		HideFirmwarePageControls();

	ClipActiveSpecPageControls(contentTop, contentBottom);
	AdjustSpecPageZOrder();
}

static void SpecPushWndToBack(CWnd* pWnd)
{
	if (pWnd != NULL && ::IsWindow(pWnd->m_hWnd))
		pWnd->SetWindowPos(&CWnd::wndBottom, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void CDtSpecDlg::AdjustSpecPageZOrder()
{
	CButton* groups[] = {
		&m_grpTiming, &m_grpLimits, &m_grpTempI2c, &m_grpBadPixel,
		&m_grpBpHuawei, &m_grpBpNeighbor, &m_grpBpSnapFiles, &m_grpFirmware,
	};
	for (int i = 0; i < (int)(sizeof(groups) / sizeof(groups[0])); i++)
		SpecPushWndToBack(groups[i]);

	RaiseSpecDialogButtons();
}

static void EnsureStandardButtonStyle(CWnd* pWnd, bool radio)
{
	if (pWnd == NULL || !::IsWindow(pWnd->m_hWnd))
		return;
	pWnd->ModifyStyle(BS_OWNERDRAW, 0);
	if (radio)
		pWnd->ModifyStyle(0, WS_TABSTOP);
	else
		pWnd->ModifyStyle(0, BS_AUTOCHECKBOX | WS_TABSTOP);
}

void CDtSpecDlg::EnsureStandardCheckAndRadioButtons()
{
	const UINT chkIds[] = {
		IDC_CHK_SPEC_TEMP_EN, IDC_CHK_SPEC_BP_EN, IDC_CHK_SPEC_BP_GRGBTOG,
		IDC_CHK_SPEC_BP_SAVE, IDC_CHK_SPEC_BP_SAVE_BMP, IDC_CHK_SPEC_BP_SAVE_PACKED,
		IDC_CHK_SPEC_BP_SAVE_U12, IDC_CHK_SPEC_BP_SAVE_U10, IDC_CHK_SPEC_FW_EN,
	};
	for (int i = 0; i < (int)(sizeof(chkIds) / sizeof(chkIds[0])); i++)
		EnsureStandardButtonStyle(GetDlgItem(chkIds[i]), false);

	const UINT radIds[] = { IDC_RAD_BP_ALGO_NEIGHBOR, IDC_RAD_BP_ALGO_HUAWEI };
	for (int i = 0; i < (int)(sizeof(radIds) / sizeof(radIds[0])); i++)
		EnsureStandardButtonStyle(GetDlgItem(radIds[i]), true);
}

void CDtSpecDlg::ApplyDialogFonts()
{
	DtCreateUiFont(m_fontTitle, 10, true, m_hWnd);
	DtCreateUiFont(m_fontBody, 9, false, m_hWnd);
	DtCreateUiFont(m_fontSmall, 8, false, m_hWnd);

	m_tab.SetFont(&m_fontBody);
	m_stTitle.SetFont(&m_fontTitle);
	m_stPath.SetFont(&m_fontBody);
	m_grpTiming.SetFont(&m_fontBody);
	m_grpLimits.SetFont(&m_fontBody);
	m_grpTempI2c.SetFont(&m_fontBody);
	m_grpBadPixel.SetFont(&m_fontBody);
	m_grpFirmware.SetFont(&m_fontTitle);
	m_lblDelay.SetFont(&m_fontBody);
	m_lblDef1.SetFont(&m_fontBody);
	m_lblDef2.SetFont(&m_fontBody);
	m_lblDef3.SetFont(&m_fontBody);
	m_lblDef4.SetFont(&m_fontBody);
	m_lblDef5.SetFont(&m_fontBody);
	m_lblDef6.SetFont(&m_fontBody);
	m_edDelay.SetFont(&m_fontBody);
	m_edDefMinSsr.SetFont(&m_fontBody);
	m_edDefMaxSsr.SetFont(&m_fontBody);
	m_edDefMinCur.SetFont(&m_fontBody);
	m_edDefMaxCur.SetFont(&m_fontBody);
	m_edDefMinTemp.SetFont(&m_fontBody);
	m_edDefMaxTemp.SetFont(&m_fontBody);
	m_chkTempEn.SetFont(&m_fontBody);
	m_lblTempAddr.SetFont(&m_fontBody);
	m_lblTempMode.SetFont(&m_fontBody);
	m_lblTempRegLo.SetFont(&m_fontBody);
	m_lblTempRegHi.SetFont(&m_fontBody);
	m_lblTempCoeffLo.SetFont(&m_fontBody);
	m_lblTempCoeffHi.SetFont(&m_fontBody);
	m_lblTempDiv.SetFont(&m_fontBody);
	m_lblTempOffset.SetFont(&m_fontBody);
	m_edTempAddr.SetFont(&m_fontBody);
	m_edTempMode.SetFont(&m_fontBody);
	m_edTempRegLo.SetFont(&m_fontBody);
	m_edTempRegHi.SetFont(&m_fontBody);
	m_edTempCoeffLo.SetFont(&m_fontBody);
	m_edTempCoeffHi.SetFont(&m_fontBody);
	m_edTempDiv.SetFont(&m_fontBody);
	m_edTempOffset.SetFont(&m_fontBody);
	m_stFormula.SetFont(&m_fontSmall);
	m_stHint.SetFont(&m_fontSmall);
	m_chkBpEn.SetFont(&m_fontBody);
	m_lblBpAlgo.SetFont(&m_fontBody);
	m_radBpNeighbor.SetFont(&m_fontBody);
	m_radBpHuawei.SetFont(&m_fontBody);
	m_grpBpHuawei.SetFont(&m_fontBody);
	m_grpBpNeighbor.SetFont(&m_fontBody);
	m_lblBpClusterTh.SetFont(&m_fontBody);
	m_lblBpClusterMin.SetFont(&m_fontBody);
	m_lblBpSinglePpm.SetFont(&m_fontBody);
	m_edBpClusterTh.SetFont(&m_fontBody);
	m_edBpClusterMin.SetFont(&m_fontBody);
	m_edBpSinglePpm.SetFont(&m_fontBody);
	m_chkBpGrGbToG.SetFont(&m_fontBody);
	m_lblBpMax.SetFont(&m_fontBody);
	m_lblBpHotDelta.SetFont(&m_fontBody);
	m_lblBpHotAbs.SetFont(&m_fontBody);
	m_lblBpBorder.SetFont(&m_fontBody);
	m_edBpMax.SetFont(&m_fontBody);
	m_edBpHotDelta.SetFont(&m_fontBody);
	m_edBpHotAbs.SetFont(&m_fontBody);
	m_edBpBorder.SetFont(&m_fontBody);
	m_chkBpSave.SetFont(&m_fontBody);
	m_grpBpSnapFiles.SetFont(&m_fontBody);
	m_chkBpSaveBmp.SetFont(&m_fontBody);
	m_chkBpSavePacked.SetFont(&m_fontBody);
	m_chkBpSaveU12.SetFont(&m_fontBody);
	m_chkBpSaveU10.SetFont(&m_fontBody);
	m_lblBpDir.SetFont(&m_fontBody);
	m_edBpDir.SetFont(&m_fontBody);
	m_btnBpBrowse.SetFont(&m_fontBody);
	m_stBpHint.SetFont(&m_fontSmall);
	m_chkFwEn.SetFont(&m_fontBody);
	m_lblFwFov.SetFont(&m_fontBody);
	m_cmbFwFov.SetFont(&m_fontBody);
	m_lblFwWarmup.SetFont(&m_fontBody);
	m_edFwWarmup.SetFont(&m_fontBody);
	m_lblFwPath.SetFont(&m_fontBody);
	m_stFwPath.SetFont(&m_fontSmall);
	m_lblFwGrabIni.SetFont(&m_fontBody);
	m_edFwGrabIni.SetFont(&m_fontBody);
	m_btnFwGrabBrowse.SetFont(&m_fontBody);
	m_stFwHint.SetFont(&m_fontSmall);
}

void CDtSpecDlg::ShowSpecPage(int page)
{
	if (page < 0)
		page = 0;
	if (page > 2)
		page = 2;
	m_specActivePage = page;
	if (m_pFn != NULL)
		m_firmware = m_pFn->m_gateFirmwareBurn;

	m_specScrollPos = 0;
	if (GetStyle() & WS_VSCROLL)
		SetScrollPos(SB_VERT, 0, TRUE);

	RelayoutSpecPage();

	if (page == 2 && m_pFn != NULL)
	{
		m_chkFwEn.SetCheck(m_firmware.enabled ? BST_CHECKED : BST_UNCHECKED);
		m_chkFwEn.EnableWindow(TRUE);
	}
}

LRESULT CDtSpecDlg::OnDpiChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_fontBody.GetSafeHandle())
	{
		m_fontTitle.DeleteObject();
		m_fontBody.DeleteObject();
		m_fontSmall.DeleteObject();
	}
	m_specFrameReady = false;
	m_specScrollPos = 0;
	ApplyDialogFonts();
	EnsureStandardCheckAndRadioButtons();
	InitSpecDialogFrame(true);
	RelayoutSpecPage();
	return 0;
}

void CDtSpecDlg::OnTcnSelchangeTabSpec(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	const int sel = m_tab.GetCurSel();
	if (sel >= 0)
	{
		ShowSpecPage(sel);
		if (sel == 0)
			UpdateFormulaText();
		else if (sel == 1)
			UpdateBadPixelSnapTypeUi();
		else if (sel == 2)
			UpdateFirmwarePathLabel();
	}
	*pResult = 0;
}

static bool SpecDlgCheckRequiredControl(CWnd* pDlg, UINT id, LPCTSTR name, CString& errOut)
{
	if (pDlg == NULL || pDlg->GetDlgItem(id) == NULL)
	{
		errOut.Format(
			_T("Spec dialog missing control:\r\n  %s (ID=%u)\r\n")
			_T("If you renamed IDs, sync resource.h + DtSpecSettings.rc + DoDataExchange in DtSpecDlg.cpp, then Rebuild."),
			name, id);
		return false;
	}
	return true;
}

BOOL CDtSpecDlg::OnInitDialog()
{
	if (!CDialogEx::OnInitDialog())
		return FALSE;

	CString err;
	static const struct { UINT id; LPCTSTR name; } kRequired[] = {
		{ IDC_TAB_SPEC, _T("IDC_TAB_SPEC") },
		{ IDC_GRP_RESERVED2, _T("IDC_GRP_RESERVED2 (firmware group)") },
		{ IDC_COMBO_SPEC_FW_FOV, _T("IDC_COMBO_SPEC_FW_FOV") },
		{ IDC_CHK_SPEC_FW_EN, _T("IDC_CHK_SPEC_FW_EN") },
	};
	for (int i = 0; i < (int)(sizeof(kRequired) / sizeof(kRequired[0])); i++)
	{
		if (!SpecDlgCheckRequiredControl(this, kRequired[i].id, kRequired[i].name, err))
		{
			AfxMessageBox(err, MB_ICONERROR);
			EndDialog(IDCANCEL);
			return TRUE;
		}
	}

	if (!::IsWindow(m_cmbFwFov.m_hWnd))
	{
		err.Format(
			_T("DDX_Control failed for firmware combo (IDC_COMBO_SPEC_FW_FOV=%u).\r\n")
			_T("Member m_cmbFwFov has no HWND — check DoDataExchange ID matches DtSpecSettings.rc."),
			(UINT)IDC_COMBO_SPEC_FW_FOV);
		AfxMessageBox(err, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return TRUE;
	}

	if (m_pFn == NULL)
	{
		EndDialog(IDCANCEL);
		return TRUE;
	}

	ModifyStyle(WS_VSCROLL, 0);
	ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	SetWindowText(ZH_UTF8(kSpecTitle));

	ApplyDialogFonts();
	EnsureStandardCheckAndRadioButtons();
	m_stHint.ModifyStyle(SS_SIMPLE, 0);
	m_stBpHint.ModifyStyle(SS_SIMPLE, 0);
	m_stFwHint.ModifyStyle(SS_SIMPLE, 0);

	if (CWnd* pOk = GetDlgItem(IDOK))
		pOk->SetFont(&m_fontBody);
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
		pCancel->SetFont(&m_fontBody);

	m_grpTiming.SetWindowText(ZH_UTF8(kSpecGrpTiming));
	m_grpLimits.SetWindowText(ZH_UTF8(kSpecGrpLimits));
	m_grpTempI2c.SetWindowText(ZH_UTF8(kSpecGrpTemp));
	m_grpBadPixel.SetWindowText(ZH_UTF8(kSpecGrpBp));
	m_grpFirmware.SetWindowText(ZH_UTF8(kSpecGrpFirmware));
	m_stHint.SetWindowText(ZH_UTF8(kSpecHint));
	m_lblDef1.SetWindowText(ZH_UTF8(kSpecDefMinFps));
	m_lblDef2.SetWindowText(ZH_UTF8(kSpecDefMaxFps));
	m_lblDef3.SetWindowText(ZH_UTF8(kSpecDefMinCur));
	m_lblDef4.SetWindowText(ZH_UTF8(kSpecDefMaxCur));
	m_lblDef5.SetWindowText(ZH_UTF8(kSpecDefMinTemp));
	m_lblDef6.SetWindowText(ZH_UTF8(kSpecDefMaxTemp));
	m_chkTempEn.SetWindowText(ZH_UTF8(kSpecTempEn));
	m_lblTempAddr.SetWindowText(ZH_UTF8(kSpecTempAddr));
	m_lblTempMode.SetWindowText(_T("Mode"));
	m_lblTempRegLo.SetWindowText(_T("RegLow"));
	m_lblTempRegHi.SetWindowText(_T("RegHigh"));
	m_lblTempCoeffLo.SetWindowText(_T("CoeffLow"));
	m_lblTempCoeffHi.SetWindowText(_T("CoeffHigh"));
	m_lblTempDiv.SetWindowText(_T("Divisor"));
	m_lblTempOffset.SetWindowText(_T("Offset"));

	m_stTitle.SetWindowText(ZH_UTF8(kSpecCfgFile));
	m_tab.DeleteAllItems();
	m_tab.InsertItem(0, _T("GateSpec.ini"));
	m_tab.InsertItem(1, ZH_UTF8(kSpecTabBp));
	m_tab.InsertItem(2, ZH_UTF8(kSpecTabFirmware));

	m_chkBpEn.SetWindowText(ZH_UTF8(kSpecBpEn));
	m_lblBpAlgo.SetWindowText(ZH_UTF8(kSpecBpAlgo));
	m_radBpNeighbor.SetWindowText(ZH_UTF8(kSpecBpNeighbor));
	m_radBpHuawei.SetWindowText(ZH_UTF8(kSpecBpHuawei));
	m_grpBpHuawei.SetWindowText(ZH_UTF8(kSpecBpGrpHw));
	m_grpBpNeighbor.SetWindowText(ZH_UTF8(kSpecBpGrpNb));
	m_lblBpClusterTh.SetWindowText(ZH_UTF8(kSpecBpClTh));
	m_lblBpClusterMin.SetWindowText(ZH_UTF8(kSpecBpClMin));
	m_lblBpSinglePpm.SetWindowText(ZH_UTF8(kSpecBpPpm));
	m_chkBpGrGbToG.SetWindowText(ZH_UTF8(kSpecBpGrGb));
	m_lblBpMax.SetWindowText(ZH_UTF8(kSpecBpMaxCl));
	m_lblBpHotDelta.SetWindowText(ZH_UTF8(kSpecBpHotDeltaHw));
	m_lblBpHotAbs.SetWindowText(ZH_UTF8(kSpecBpHotAbs));
	m_lblBpBorder.SetWindowText(ZH_UTF8(kSpecBpBorder));
	m_chkBpSave.SetWindowText(ZH_UTF8(kSpecBpSave));
	m_grpBpSnapFiles.SetWindowText(ZH_UTF8(kSpecBpSnapGrp));
	m_chkBpSaveBmp.SetWindowText(ZH_UTF8(kSpecBpSaveBmp));
	m_chkBpSavePacked.SetWindowText(ZH_UTF8(kSpecBpSavePacked));
	m_chkBpSaveU12.SetWindowText(ZH_UTF8(kSpecBpSaveU12));
	m_chkBpSaveU10.SetWindowText(ZH_UTF8(kSpecBpSaveU10));
	m_lblBpDir.SetWindowText(ZH_UTF8(kSpecBpDir));
	m_btnBpBrowse.SetWindowText(ZH_UTF8(kSpecBpBrowse));
	m_stBpHint.SetWindowText(_T(""));

	m_chkFwEn.SetWindowText(ZH_UTF8(kSpecFwEn));
	m_lblFwFov.SetWindowText(ZH_UTF8(kSpecFwFov));
	m_lblFwWarmup.SetWindowText(ZH_UTF8(kSpecFwWarmup));
	m_lblFwPath.SetWindowText(ZH_UTF8(kSpecFwPath));
	m_lblFwGrabIni.SetWindowText(ZH_UTF8(kSpecFwGrabIniAfterPc));
	m_btnFwGrabBrowse.SetWindowText(ZH_UTF8(kSpecBpBrowse));
	m_stFwHint.SetWindowText(ZH_UI(kSpecFwHint));

	FillFirmwareFovCombo();

	m_grpBadPixel.ShowWindow(SW_HIDE);
	m_grpFirmware.ShowWindow(SW_HIDE);
	m_chkFwEn.ShowWindow(SW_HIDE);
	m_lblFwFov.ShowWindow(SW_HIDE);
	m_cmbFwFov.ShowWindow(SW_HIDE);
	m_lblFwWarmup.ShowWindow(SW_HIDE);
	m_edFwWarmup.ShowWindow(SW_HIDE);
	m_lblFwPath.ShowWindow(SW_HIDE);
	m_stFwPath.ShowWindow(SW_HIDE);
	m_lblFwGrabIni.ShowWindow(SW_HIDE);
	m_edFwGrabIni.ShowWindow(SW_HIDE);
	m_btnFwGrabBrowse.ShowWindow(SW_HIDE);
	m_stFwHint.ShowWindow(SW_HIDE);

	m_pFn->ReadGateSpecIni();
	m_def = m_pFn->m_gateDefault;
	m_tempI2c = m_pFn->m_gateSensorTempI2c;
	m_badPixel = m_pFn->m_gateBadPixelDark;
	m_firmware = m_pFn->m_gateFirmwareBurn;

	CString pathLine;
	pathLine.Format(_T("%s"), (LPCTSTR)m_pFn->m_strGateSpecIniPath);
	m_stPath.SetWindowText(pathLine);

	CString s;
	s.Format(_T("%d"), m_pFn->m_specDelayMs);
	m_edDelay.SetWindowText(s);

	FormatDouble(s, m_def.minSsrFps);
	m_edDefMinSsr.SetWindowText(s);
	FormatDouble(s, m_def.maxSsrFps);
	m_edDefMaxSsr.SetWindowText(s);
	FormatDouble(s, m_def.minCurrent_mA);
	m_edDefMinCur.SetWindowText(s);
	FormatDouble(s, m_def.maxCurrent_mA);
	m_edDefMaxCur.SetWindowText(s);
	FormatDouble(s, m_def.minSensorTemp_C);
	m_edDefMinTemp.SetWindowText(s);
	FormatDouble(s, m_def.maxSensorTemp_C);
	m_edDefMaxTemp.SetWindowText(s);

	m_chkTempEn.SetCheck(m_tempI2c.enabled ? BST_CHECKED : BST_UNCHECKED);
	FormatHex(s, m_tempI2c.i2cAddr, 2);
	m_edTempAddr.SetWindowText(s);
	s.Format(_T("%u"), m_tempI2c.i2cMode);
	m_edTempMode.SetWindowText(s);
	FormatHex(s, m_tempI2c.regLow, 4);
	m_edTempRegLo.SetWindowText(s);
	FormatHex(s, m_tempI2c.regHigh, 4);
	m_edTempRegHi.SetWindowText(s);
	FormatDouble(s, m_tempI2c.coeffLow);
	m_edTempCoeffLo.SetWindowText(s);
	FormatDouble(s, m_tempI2c.coeffHigh);
	m_edTempCoeffHi.SetWindowText(s);
	FormatDouble(s, m_tempI2c.divisor);
	m_edTempDiv.SetWindowText(s);
	FormatDouble(s, m_tempI2c.offset);
	m_edTempOffset.SetWindowText(s);

	m_chkBpEn.SetCheck(m_badPixel.enabled ? BST_CHECKED : BST_UNCHECKED);
	if (m_badPixel.algoMode == 1)
	{
		m_radBpHuawei.SetCheck(BST_CHECKED);
		m_radBpNeighbor.SetCheck(BST_UNCHECKED);
	}
	else
	{
		m_radBpNeighbor.SetCheck(BST_CHECKED);
		m_radBpHuawei.SetCheck(BST_UNCHECKED);
	}
	s.Format(_T("%d"), m_badPixel.maxBadPixels);
	m_edBpMax.SetWindowText(s);
	s.Format(_T("%d"), m_badPixel.hotDelta);
	m_edBpHotDelta.SetWindowText(s);
	s.Format(_T("%d"), m_badPixel.brightContrastCluster);
	m_edBpClusterTh.SetWindowText(s);
	s.Format(_T("%d"), m_badPixel.clusterMinPixels);
	m_edBpClusterMin.SetWindowText(s);
	s.Format(_T("%d"), m_badPixel.singleDefectPermyriad);
	m_edBpSinglePpm.SetWindowText(s);
	m_chkBpGrGbToG.SetCheck(m_badPixel.grGbToG ? BST_CHECKED : BST_UNCHECKED);
	s.Format(_T("%d"), m_badPixel.hotAbsMin);
	m_edBpHotAbs.SetWindowText(s);
	s.Format(_T("%d"), m_badPixel.borderPx);
	m_edBpBorder.SetWindowText(s);
	m_chkBpSave.SetCheck(m_badPixel.saveSnapshot ? BST_CHECKED : BST_UNCHECKED);
	m_chkBpSaveBmp.SetCheck(m_badPixel.saveBmp ? BST_CHECKED : BST_UNCHECKED);
	m_chkBpSavePacked.SetCheck(m_badPixel.savePackedRaw ? BST_CHECKED : BST_UNCHECKED);
	m_chkBpSaveU12.SetCheck(m_badPixel.saveUnpack12 ? BST_CHECKED : BST_UNCHECKED);
	m_chkBpSaveU10.SetCheck(m_badPixel.saveUnpack10 ? BST_CHECKED : BST_UNCHECKED);
	m_edBpDir.SetWindowText(m_badPixel.saveDir);

	m_chkFwEn.SetCheck(m_firmware.enabled ? BST_CHECKED : BST_UNCHECKED);
	if (::IsWindow(m_cmbFwFov.m_hWnd) && m_cmbFwFov.GetCount() == FIRMWARE_FOV_TYPE_COUNT)
	{
		int fovSel = m_firmware.fovTypeIndex;
		if (fovSel < 0 || fovSel >= FIRMWARE_FOV_TYPE_COUNT)
			fovSel = FIRMWARE_FOV_DEFAULT_INDEX;
		m_cmbFwFov.SetCurSel(fovSel);
	}
	s.Format(_T("%d"), m_firmware.fwWarmupMs);
	m_edFwWarmup.SetWindowText(s);
	m_edFwGrabIni.SetWindowText(m_firmware.grabIniAfterPowerCycle);
	UpdateFirmwarePathLabel();

	UpdateFormulaText();
	m_tab.SetCurSel(0);
	m_specFrameReady = false;
	ShowSpecPage(0);
	UpdateBadPixelAlgoUi();
	UpdateBadPixelSnapTypeUi();

	return TRUE;
}

void CDtSpecDlg::OnOK()
{
	CString t;
	m_edDelay.GetWindowText(t);
	const int delay = _tstoi((LPCTSTR)t);
	if (delay < 200)
	{
		AfxMessageBox(_T("DelayMs must be >= 200."), MB_ICONWARNING);
		return;
	}

	m_edDefMinSsr.GetWindowText(t);
	m_def.minSsrFps = ParseDouble(t);
	m_edDefMaxSsr.GetWindowText(t);
	m_def.maxSsrFps = ParseDouble(t);
	m_edDefMinCur.GetWindowText(t);
	m_def.minCurrent_mA = ParseDouble(t);
	m_edDefMaxCur.GetWindowText(t);
	m_def.maxCurrent_mA = ParseDouble(t);
	m_edDefMinTemp.GetWindowText(t);
	m_def.minSensorTemp_C = ParseDouble(t);
	m_edDefMaxTemp.GetWindowText(t);
	m_def.maxSensorTemp_C = ParseDouble(t);

	m_tempI2c.enabled = (m_chkTempEn.GetCheck() == BST_CHECKED);
	m_edTempAddr.GetWindowText(t);
	m_tempI2c.i2cAddr = (unsigned char)ParseHexUint(t);
	m_edTempMode.GetWindowText(t);
	m_tempI2c.i2cMode = (unsigned char)_tstoi((LPCTSTR)t);
	m_edTempRegLo.GetWindowText(t);
	m_tempI2c.regLow = (unsigned short)ParseHexUint(t);
	m_edTempRegHi.GetWindowText(t);
	m_tempI2c.regHigh = (unsigned short)ParseHexUint(t);
	m_edTempCoeffLo.GetWindowText(t);
	m_tempI2c.coeffLow = ParseDouble(t);
	m_edTempCoeffHi.GetWindowText(t);
	m_tempI2c.coeffHigh = ParseDouble(t);
	m_edTempDiv.GetWindowText(t);
	m_tempI2c.divisor = ParseDouble(t);
	m_edTempOffset.GetWindowText(t);
	m_tempI2c.offset = ParseDouble(t);

	if (!(m_def.minSsrFps <= m_def.maxSsrFps
		&& m_def.minCurrent_mA <= m_def.maxCurrent_mA
		&& m_def.minSensorTemp_C <= m_def.maxSensorTemp_C))
	{
		AfxMessageBox(_T("Limits invalid: min must be <= max."), MB_ICONWARNING);
		return;
	}
	if (m_tempI2c.enabled && fabs(m_tempI2c.divisor) < 1e-12)
	{
		AfxMessageBox(_T("Divisor must not be 0 when I2C temp read is enabled."), MB_ICONWARNING);
		return;
	}

	m_badPixel.enabled = (m_chkBpEn.GetCheck() == BST_CHECKED);
	m_badPixel.algoMode = (m_radBpHuawei.GetCheck() == BST_CHECKED) ? 1 : 0;
	m_edBpMax.GetWindowText(t);
	m_badPixel.maxBadPixels = _tstoi((LPCTSTR)t);
	m_edBpHotDelta.GetWindowText(t);
	m_badPixel.hotDelta = _tstoi((LPCTSTR)t);
	m_edBpClusterTh.GetWindowText(t);
	m_badPixel.brightContrastCluster = _tstoi((LPCTSTR)t);
	m_edBpClusterMin.GetWindowText(t);
	m_badPixel.clusterMinPixels = _tstoi((LPCTSTR)t);
	m_edBpSinglePpm.GetWindowText(t);
	m_badPixel.singleDefectPermyriad = _tstoi((LPCTSTR)t);
	m_badPixel.grGbToG = (m_chkBpGrGbToG.GetCheck() == BST_CHECKED);
	m_edBpHotAbs.GetWindowText(t);
	m_badPixel.hotAbsMin = _tstoi((LPCTSTR)t);
	m_edBpBorder.GetWindowText(t);
	m_badPixel.borderPx = _tstoi((LPCTSTR)t);
	m_badPixel.saveSnapshot = (m_chkBpSave.GetCheck() == BST_CHECKED);
	m_badPixel.saveBmp = (m_chkBpSaveBmp.GetCheck() == BST_CHECKED);
	m_badPixel.savePackedRaw = (m_chkBpSavePacked.GetCheck() == BST_CHECKED);
	m_badPixel.saveUnpack12 = (m_chkBpSaveU12.GetCheck() == BST_CHECKED);
	m_badPixel.saveUnpack10 = (m_chkBpSaveU10.GetCheck() == BST_CHECKED);
	if (m_badPixel.saveSnapshot
		&& !m_badPixel.saveBmp && !m_badPixel.savePackedRaw
		&& !m_badPixel.saveUnpack12 && !m_badPixel.saveUnpack10)
	{
		AfxMessageBox(ZH_UTF8(kSpecBpSnapTypeWarn), MB_ICONWARNING);
		return;
	}
	m_edBpDir.GetWindowText(t);
	t.Trim();
	_tcsncpy_s(m_badPixel.saveDir, (LPCTSTR)t, _TRUNCATE);
	if (m_badPixel.maxBadPixels < 0)
	{
		AfxMessageBox(_T("MaxBadPixels / cluster limit must be >= 0."), MB_ICONWARNING);
		return;
	}
	if (m_badPixel.hotDelta < 1)
	{
		AfxMessageBox(_T("HotDelta must be >= 1."), MB_ICONWARNING);
		return;
	}
	if (m_badPixel.algoMode == 1)
	{
		if (m_badPixel.brightContrastCluster < 1)
		{
			AfxMessageBox(_T("Cluster threshold must be >= 1."), MB_ICONWARNING);
			return;
		}
		if (m_badPixel.clusterMinPixels < 2)
		{
			AfxMessageBox(_T("ClusterMinPixels must be >= 2."), MB_ICONWARNING);
			return;
		}
		if (m_badPixel.singleDefectPermyriad < 0)
		{
			AfxMessageBox(_T("SingleDefectPermyriad must be >= 0."), MB_ICONWARNING);
			return;
		}
	}
	else if (m_badPixel.hotAbsMin < 1 || m_badPixel.borderPx < 1)
	{
		AfxMessageBox(_T("HotAbsMin / BorderPx must be >= 1 for neighbor mode."), MB_ICONWARNING);
		return;
	}

	m_pFn->m_specDelayMs = delay;
	m_pFn->m_gateDefault = m_def;
	m_pFn->m_gateSensorTempI2c = m_tempI2c;
	m_pFn->m_gateBadPixelDark = m_badPixel;
	m_firmware.enabled = (m_chkFwEn.GetCheck() == BST_CHECKED);
	m_firmware.fovTypeIndex = m_cmbFwFov.GetCurSel();
	if (m_firmware.fovTypeIndex < 0)
		m_firmware.fovTypeIndex = FIRMWARE_FOV_DEFAULT_INDEX;
	m_edFwWarmup.GetWindowText(t);
	m_firmware.fwWarmupMs = _tstoi((LPCTSTR)t);
	if (m_firmware.fwWarmupMs < 500)
		m_firmware.fwWarmupMs = 3000;
	m_edFwGrabIni.GetWindowText(t);
	t.Trim();
	if (t.IsEmpty())
		m_firmware.grabIniAfterPowerCycle[0] = 0;
	else
		_tcsncpy_s(m_firmware.grabIniAfterPowerCycle, (LPCTSTR)t, _TRUNCATE);
	if (m_firmware.enabled)
	{
		TCHAR bin[MAX_PATH] = {};
		if (!ResolveFirmwareBinPath(m_firmware, bin))
		{
			CString warn;
			warn.Format(_T("Firmware not found:\r\nFlashData\\031%s.bin"),
				FirmwareFovTypeName(m_firmware.fovTypeIndex));
			AfxMessageBox(warn, MB_ICONWARNING);
			return;
		}
	}
	m_pFn->m_gateFirmwareBurn = m_firmware;
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
			m_pFn->m_gatePerChannel[d][v] = m_def;
	}
	if (!m_pFn->SaveGateSpecIni())
	{
		AfxMessageBox(_T("Failed to save GateSpec.ini (check path and write permission)."), MB_ICONERROR);
		return;
	}

	CStringA pathA(m_pFn->m_strGateSpecIniPath);
	msgUtf8(DtZh::kLogGateSpecSaved, pathA.GetString());

	CDialogEx::OnOK();
}

void CDtSpecDlg::OnBnClickedBtnBpBrowse()
{
	CString cur;
	m_edBpDir.GetWindowText(cur);
	cur.Trim();
	const CString picked = BrowseFolder(m_hWnd, ZH_UTF8(kBrowseBpDir));
	if (!picked.IsEmpty())
		m_edBpDir.SetWindowText(picked);
}

void CDtSpecDlg::OnBnClickedBtnFwGrabBrowse()
{
	CString cur;
	m_edFwGrabIni.GetWindowText(cur);
	cur.Trim();
	CFileDialog dlg(TRUE, _T("ini"), cur.IsEmpty() ? NULL : (LPCTSTR)cur,
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("Ini File (*.ini)|*.ini||"), this);
	dlg.m_ofn.lpstrTitle = ZH_UTF8(kBrowseFwGrabIni);
	if (dlg.DoModal() == IDOK)
		m_edFwGrabIni.SetWindowText(dlg.GetPathName());
}

void CDtSpecDlg::FillFirmwareFovCombo()
{
	if (!::IsWindow(m_cmbFwFov.m_hWnd))
		return;
	int sel = m_cmbFwFov.GetCurSel();
	if (sel < 0 && m_pFn != NULL)
		sel = m_pFn->m_gateFirmwareBurn.fovTypeIndex;
	m_cmbFwFov.ResetContent();
	for (int i = 0; i < FIRMWARE_FOV_TYPE_COUNT; i++)
		m_cmbFwFov.AddString(FirmwareFovTypeName(i));
	if (sel < 0 || sel >= FIRMWARE_FOV_TYPE_COUNT)
		sel = FIRMWARE_FOV_DEFAULT_INDEX;
	m_cmbFwFov.SetCurSel(sel);
}

void CDtSpecDlg::OnBnClickedChkFwEn()
{
	if (m_specActivePage != 2)
		return;
	m_firmware.enabled = (m_chkFwEn.GetCheck() == BST_CHECKED);
}

void CDtSpecDlg::OnCbnSelchangeFwFov()
{
	UpdateFirmwarePathLabel();
}

void CDtSpecDlg::UpdateFirmwarePathLabel()
{
	if (!::IsWindow(m_cmbFwFov.m_hWnd))
		return;
	GateFirmwareBurnCfg tmp = m_firmware;
	tmp.fovTypeIndex = m_cmbFwFov.GetCurSel();
	if (tmp.fovTypeIndex < 0)
		tmp.fovTypeIndex = FIRMWARE_FOV_DEFAULT_INDEX;
	TCHAR bin[MAX_PATH] = {};
	if (ResolveFirmwareBinPath(tmp, bin))
		m_stFwPath.SetWindowText(bin);
	else
		m_stFwPath.SetWindowText(_T("(file not found)"));
}

void CDtSpecDlg::UpdateBadPixelSnapTypeUi()
{
	const BOOL saveOn = (m_chkBpSave.GetCheck() == BST_CHECKED);
	auto en = [&](CWnd& w, BOOL on) { w.EnableWindow(on); };
	en(m_grpBpSnapFiles, saveOn);
	en(m_chkBpSaveBmp, saveOn);
	en(m_chkBpSavePacked, saveOn);
	en(m_chkBpSaveU12, saveOn);
	en(m_chkBpSaveU10, saveOn);
}

void CDtSpecDlg::OnBnClickedBpSave()
{
	UpdateBadPixelSnapTypeUi();
}

void CDtSpecDlg::UpdateBadPixelLabels()
{
	const BOOL huawei = (::IsWindow(m_radBpHuawei.m_hWnd) && m_radBpHuawei.GetCheck() == BST_CHECKED);
	if (huawei)
	{
		m_lblBpMax.SetWindowText(ZH_UTF8(kSpecBpMaxCl));
		m_lblBpHotDelta.SetWindowText(ZH_UTF8(kSpecBpHotDeltaHw));
		m_stBpHint.SetWindowText(ZH_UI(kSpecBpHintHw));
	}
	else
	{
		m_lblBpMax.SetWindowText(ZH_UTF8(kSpecBpMaxHot));
		m_lblBpHotDelta.SetWindowText(ZH_UTF8(kSpecBpHotDeltaNb));
		m_stBpHint.SetWindowText(ZH_UI(kSpecBpHintNb));
	}
}

void CDtSpecDlg::UpdateBadPixelAlgoUi()
{
	if (m_tab.GetSafeHwnd() == NULL || m_tab.GetCurSel() != 1)
		return;

	UpdateBadPixelLabels();
	RelayoutSpecPage();
	UpdateBadPixelSnapTypeUi();
}

void CDtSpecDlg::OnBnClickedRadBpAlgo()
{
	UpdateBadPixelAlgoUi();
}

