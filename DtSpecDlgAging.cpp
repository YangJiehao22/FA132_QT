#include "stdafx.h"
#include "DtSpecDlg.h"
#include "DtOvenModbus.h"

namespace {

static void MoveWnd(CWnd* p, const CRect& r)
{
	if (p != NULL && ::IsWindow(p->m_hWnd))
		p->SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER);
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

static int LabelWidth(CWnd* pDlg, CWnd& lbl, double scale)
{
	CString t;
	lbl.GetWindowText(t);
	if (t.IsEmpty())
		return (int)(72 * scale);
	CClientDC dc(pDlg);
	return TextExtentForWnd(&lbl, dc, t).cx + (int)(12 * scale);
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
	return rc.Height() + 8;
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

static void ReadChamberLegacyFromEdits(CEdit& edSet, CEdit& edStart, CEdit& edStop,
	CEdit& edPv, CEdit& edRun, CEdit& edRunBit, CEdit& edFault, OvenChamberMap* out)
{
	if (out == NULL)
		return;
	CString t;
	edSet.GetWindowText(t);
	out->regSetTemp = _tstoi(t);
	edStart.GetWindowText(t);
	out->regStart = _tstoi(t);
	edStop.GetWindowText(t);
	out->regStop = _tstoi(t);
	edPv.GetWindowText(t);
	out->regPv = _tstoi(t);
	edRun.GetWindowText(t);
	out->regRunState = _tstoi(t);
	edRunBit.GetWindowText(t);
	out->runStateBit = _tstoi(t);
	edFault.GetWindowText(t);
	out->regFault = _tstoi(t);
}

static void WriteChamberLegacyToEdits(const OvenChamberMap& m,
	CEdit& edSet, CEdit& edStart, CEdit& edStop,
	CEdit& edPv, CEdit& edRun, CEdit& edRunBit, CEdit& edFault)
{
	CString s;
	s.Format(_T("%d"), m.regSetTemp);
	edSet.SetWindowText(s);
	s.Format(_T("%d"), m.regStart);
	edStart.SetWindowText(s);
	s.Format(_T("%d"), m.regStop);
	edStop.SetWindowText(s);
	s.Format(_T("%d"), m.regPv);
	edPv.SetWindowText(s);
	s.Format(_T("%d"), m.regRunState);
	edRun.SetWindowText(s);
	s.Format(_T("%d"), m.runStateBit);
	edRunBit.SetWindowText(s);
	s.Format(_T("%d"), m.regFault);
	edFault.SetWindowText(s);
}

} // namespace

void CDtSpecDlg::HideAgingPageControls()
{
	CWnd* ctrls[] = {
		&m_grpAging, &m_chkAgingEn, &m_chkAgingHeatStart,
		&m_grpOven, &m_lblOvenProfile, &m_cmbOvenProfile,
		&m_lblOvenHeatMode, &m_cmbOvenHeatMode,
		&m_lblOvenHost, &m_edOvenHost, &m_lblOvenPort, &m_edOvenPort,
		&m_lblOvenUnit, &m_edOvenUnit, &m_lblOvenTarget, &m_edOvenTarget,
		&m_lblOvenTol, &m_edOvenTol, &m_lblOvenWait, &m_edOvenWait,
		&m_lblOvenPoll, &m_edOvenPoll, &m_chkOvenDual, &m_lblOvenDPv, &m_edOvenDPv,
		&m_chkOvenCooldown, &m_lblOvenCooldownTarget, &m_edOvenCooldownTarget,
		&m_lblOvenCooldownTimeout, &m_edOvenCooldownTimeout,
		&m_lblOvenCooldownTol, &m_edOvenCooldownTol,
		&m_btnOvenTest, &m_btnOvenReadPv, &m_btnOvenSetTemp, &m_btnOvenStart, &m_btnOvenStop,
		&m_grpAgingGate, &m_lblAgingDur, &m_edAgingDur, &m_lblAgingSample, &m_edAgingSample,
		&m_lblAgingMinSsr, &m_edAgingMinSsr, &m_lblAgingMaxSsr, &m_edAgingMaxSsr,
		&m_lblAgingMinCur, &m_edAgingMinCur, &m_lblAgingMaxCur, &m_edAgingMaxCur,
		&m_lblAgingMinTemp, &m_edAgingMinTemp, &m_lblAgingMaxTemp, &m_edAgingMaxTemp,
		&m_lblAgingMinAvdd, &m_edAgingMinAvdd, &m_lblAgingMaxAvdd, &m_edAgingMaxAvdd,
		&m_lblAgingMinIovdd, &m_edAgingMinIovdd, &m_lblAgingMaxIovdd, &m_edAgingMaxIovdd,
		&m_lblAgingMinDvdd, &m_edAgingMinDvdd, &m_lblAgingMaxDvdd, &m_edAgingMaxDvdd,
		&m_grpAgingVolt, &m_chkAgingVoltEn, &m_lblAgingVoltMode, &m_edAgingVoltMode,
		&m_lblAgingVoltScale, &m_edAgingVoltScale, &m_lblAgingRegAvdd, &m_edAgingRegAvdd,
		&m_lblAgingRegAvddH, &m_edAgingRegAvddH, &m_lblAgingRegIovdd, &m_edAgingRegIovdd,
		&m_lblAgingRegIovddH, &m_edAgingRegIovddH, &m_lblAgingRegDvdd, &m_edAgingRegDvdd,
		&m_lblAgingRegDvddH, &m_edAgingRegDvddH,
		&m_grpOvenAdv, &m_lblOvenRegSet, &m_edOvenRegSet,
		&m_lblOvenCoilStart, &m_edOvenCoilStart, &m_lblOvenCoilStop, &m_edOvenCoilStop,
		&m_lblOvenRegPv, &m_edOvenRegPv, &m_lblOvenRegRun, &m_edOvenRegRun,
		&m_lblOvenRunBit, &m_edOvenRunBit, &m_lblOvenRegFault, &m_edOvenRegFault,
		&m_stAgingHint,
	};
	for (int i = 0; i < (int)(sizeof(ctrls) / sizeof(ctrls[0])); i++)
	{
		if (ctrls[i]->GetSafeHwnd())
			ctrls[i]->ShowWindow(SW_HIDE);
	}
}

void CDtSpecDlg::FillOvenProfileCombo()
{
	if (!m_cmbOvenProfile.GetSafeHwnd())
		return;
	m_cmbOvenProfile.ResetContent();
	m_cmbOvenProfile.AddString(_T("Siemens_1200"));
	m_cmbOvenProfile.AddString(_T("X1M"));
	m_cmbOvenProfile.AddString(_T("Custom"));
	const int rowH = max((int)(30 * m_specLayoutScale), 24);
	m_cmbOvenProfile.SetItemHeight(-1, rowH);
	for (int i = 0; i < m_cmbOvenProfile.GetCount(); i++)
		m_cmbOvenProfile.SetItemHeight(i, rowH);
}

void CDtSpecDlg::FillOvenHeatModeCombo()
{
	if (!m_cmbOvenHeatMode.GetSafeHwnd())
		return;
	m_cmbOvenHeatMode.ResetContent();
	m_cmbOvenHeatMode.AddString(_T("Both"));
	m_cmbOvenHeatMode.AddString(_T("UOnly"));
	m_cmbOvenHeatMode.AddString(_T("DOnly"));
	const int rowH = max((int)(30 * m_specLayoutScale), 24);
	m_cmbOvenHeatMode.SetItemHeight(-1, rowH);
	for (int i = 0; i < m_cmbOvenHeatMode.GetCount(); i++)
		m_cmbOvenHeatMode.SetItemHeight(i, rowH);
}

void CDtSpecDlg::ApplyOvenProfileToUi(OvenProfileId profile)
{
	GateOvenCfg tmp = m_oven;
	tmp.profile = profile;
	if (profile != OVEN_PROFILE_CUSTOM)
		OvenApplyProfileDefaults(profile, &tmp);
	m_oven.profile = profile;
	m_oven.startStopKind = tmp.startStopKind;
	m_oven.chamberU = tmp.chamberU;
	if (profile != OVEN_PROFILE_CUSTOM)
		m_oven.chamberD = tmp.chamberD;

	WriteChamberLegacyToEdits(m_oven.chamberU,
		m_edOvenRegSet, m_edOvenCoilStart, m_edOvenCoilStop,
		m_edOvenRegPv, m_edOvenRegRun, m_edOvenRunBit, m_edOvenRegFault);
	CString s;
	s.Format(_T("%d"), m_oven.chamberD.regPv);
	m_edOvenDPv.SetWindowText(s);

	UpdateOvenRegEditEnableState();
}

void CDtSpecDlg::UpdateOvenRegEditEnableState()
{
	if (!m_cmbOvenProfile.GetSafeHwnd())
		return;
	int sel = m_cmbOvenProfile.GetCurSel();
	if (sel < 0 || sel > OVEN_PROFILE_CUSTOM)
		sel = OVEN_PROFILE_S1200;
	const bool custom = (sel == OVEN_PROFILE_CUSTOM);
	CEdit* regEdits[] = {
		&m_edOvenRegSet, &m_edOvenCoilStart, &m_edOvenCoilStop,
		&m_edOvenRegPv, &m_edOvenRegRun, &m_edOvenRunBit, &m_edOvenRegFault,
		&m_edOvenDPv,
	};
	for (size_t i = 0; i < sizeof(regEdits) / sizeof(regEdits[0]); i++)
	{
		if (regEdits[i]->GetSafeHwnd())
		{
			regEdits[i]->SetReadOnly(!custom);
			regEdits[i]->EnableWindow(custom);
		}
	}
}

void CDtSpecDlg::ReadOvenFromUi(GateOvenCfg* out)
{
	if (out == NULL)
		return;
	CString t;
	m_edOvenHost.GetWindowText(t);
	_tcsncpy_s(out->host, t.GetString(), _TRUNCATE);
	m_edOvenPort.GetWindowText(t);
	out->port = _tstoi(t);
	m_edOvenUnit.GetWindowText(t);
	out->unitId = _tstoi(t);
	m_edOvenTarget.GetWindowText(t);
	out->targetC = _tstof(t);
	m_edOvenTol.GetWindowText(t);
	out->readyToleranceC = _tstof(t);
	m_edOvenWait.GetWindowText(t);
	out->waitTimeoutMin = _tstoi(t);
	m_edOvenPoll.GetWindowText(t);
	out->pollIntervalMs = _tstoi(t);
	out->dualChamber = (m_chkOvenDual.GetCheck() == BST_CHECKED);
	out->cooldownEnabled = (m_chkOvenCooldown.GetCheck() == BST_CHECKED);
	m_edOvenCooldownTarget.GetWindowText(t);
	out->cooldownTargetC = _tstof(t);
	m_edOvenCooldownTimeout.GetWindowText(t);
	out->cooldownTimeoutMin = _tstoi(t);
	m_edOvenCooldownTol.GetWindowText(t);
	out->cooldownToleranceC = _tstof(t);
	const int hmSel = m_cmbOvenHeatMode.GetCurSel();
	if (hmSel == 1) out->heatMode = OVEN_HEAT_U_ONLY;
	else if (hmSel == 2) out->heatMode = OVEN_HEAT_D_ONLY;
	else out->heatMode = OVEN_HEAT_BOTH;
	ReadChamberLegacyFromEdits(m_edOvenRegSet, m_edOvenCoilStart, m_edOvenCoilStop,
		m_edOvenRegPv, m_edOvenRegRun, m_edOvenRunBit, m_edOvenRegFault, &out->chamberU);
	m_edOvenDPv.GetWindowText(t);
	out->chamberD.regPv = _tstoi(t);
}

void CDtSpecDlg::OnCbnSelchangeOvenProfile()
{
	const int sel = m_cmbOvenProfile.GetCurSel();
	OvenProfileId p = OVEN_PROFILE_S1200;
	if (sel == 1) p = OVEN_PROFILE_X1M;
	else if (sel == 2) p = OVEN_PROFILE_CUSTOM;
	ApplyOvenProfileToUi(p);
}

GateOvenCfg CDtSpecDlg::BuildOvenCfgFromUi() const
{
	GateOvenCfg cfg = m_oven;
	const_cast<CDtSpecDlg*>(this)->ReadOvenFromUi(&cfg);
	cfg.enabled = true;
	cfg.profile = (OvenProfileId)m_cmbOvenProfile.GetCurSel();
	if (cfg.profile < 0 || cfg.profile > OVEN_PROFILE_CUSTOM)
		cfg.profile = OVEN_PROFILE_S1200;
	if (cfg.profile != OVEN_PROFILE_CUSTOM)
		OvenApplyProfileDefaults(cfg.profile, &cfg);
	return cfg;
}

void CDtSpecDlg::OnBnClickedOvenTest()
{
	const GateOvenCfg cfg = BuildOvenCfgFromUi();
	COvenModbusClient cli;
	if (!cli.Connect(cfg))
	{
		AfxMessageBox(_T("Modbus TCP connect failed."), MB_ICONWARNING);
		return;
	}
	double pvU = 0.0;
	double pvD = 0.0;
	CString msg;
	if (cli.ReadTemperatureC(cfg.chamberU.regPv, cfg.tempScale, &pvU))
	{
		if (cfg.dualChamber && cli.ReadTemperatureC(cfg.chamberD.regPv, cfg.tempScale, &pvD))
			msg.Format(_T("Connect OK. U=%.1f C D=%.1f C"), pvU, pvD);
		else
			msg.Format(_T("Connect OK. U PV=%.1f C"), pvU);
		AfxMessageBox(msg, MB_ICONINFORMATION);
	}
	else
		AfxMessageBox(_T("Connect OK but read PV failed."), MB_ICONWARNING);
}

void CDtSpecDlg::OnBnClickedOvenReadPv()
{
	OnBnClickedOvenTest();
}

void CDtSpecDlg::OnBnClickedOvenSetTemp()
{
	const GateOvenCfg cfg = BuildOvenCfgFromUi();
	if (OvenSetTargetTemp(cfg, cfg.targetC, cfg.heatMode))
		AfxMessageBox(_T("SetTemp OK."), MB_ICONINFORMATION);
	else
		AfxMessageBox(_T("SetTemp failed."), MB_ICONWARNING);
}

void CDtSpecDlg::OnBnClickedOvenStart()
{
	const GateOvenCfg cfg = BuildOvenCfgFromUi();
	if (OvenHeatStart(cfg))
		AfxMessageBox(_T("Start sent."), MB_ICONINFORMATION);
	else
		AfxMessageBox(_T("Start failed."), MB_ICONWARNING);
}

void CDtSpecDlg::OnBnClickedOvenStop()
{
	const GateOvenCfg cfg = BuildOvenCfgFromUi();
	if (OvenStopOnly(cfg))
		AfxMessageBox(_T("Stop sent."), MB_ICONINFORMATION);
	else
		AfxMessageBox(_T("Stop failed."), MB_ICONWARNING);
}

int CDtSpecDlg::LayoutAgingPage(const CRect& viewport, double scale, bool bShow)
{
	const bool allowShow = bShow && (m_specActivePage == 3);
	const SpecLayoutMetrics m = GetSpecLayoutMetrics(this, m_fontBody, scale);
	const int pad = (int)(16 * scale);
	const int x = viewport.left + pad;
	const int w = max(viewport.Width() - pad * 2, (int)(480 * scale));
	const int y0 = viewport.top;
	int y = y0 + pad;

	auto show = [allowShow](CWnd& wnd) {
		if (!allowShow) return;
		wnd.EnableWindow(TRUE);
		wnd.ShowWindow(SW_SHOW);
	};

	const int chkH = m.chkRowH;
	const int rowH = m.rowH;
	const int editH = m.editH;
	const int ix = x + pad;
	const int iw = w - pad * 2;
	const int edShort = max((int)(56 * scale), 52);
	const int edMed = max((int)(72 * scale), 64);
	const int edHost = max((int)(140 * scale), 120);
	const int colGap = m.gap;

	CString hintText;
	m_stAgingHint.GetWindowText(hintText);
	const int hintH = max((int)(36 * scale), MeasureStaticHeight(&m_stAgingHint, iw, hintText));

	const int profLblW = LabelWidth(this, m_lblOvenProfile, scale);
	const int heatLblW = LabelWidth(this, m_lblOvenHeatMode, scale);
	const int hostLblW = LabelWidth(this, m_lblOvenHost, scale);
	const int portLblW = LabelWidth(this, m_lblOvenPort, scale);
	const int unitLblW = LabelWidth(this, m_lblOvenUnit, scale);
	const int targetLblW = LabelWidth(this, m_lblOvenTarget, scale);
	const int tolLblW = LabelWidth(this, m_lblOvenTol, scale);
	const int waitLblW = LabelWidth(this, m_lblOvenWait, scale);
	const int pollLblW = LabelWidth(this, m_lblOvenPoll, scale);
	const int dPvLblW = LabelWidth(this, m_lblOvenDPv, scale);
	const int cdTargetLblW = LabelWidth(this, m_lblOvenCooldownTarget, scale);
	const int cdTimeoutLblW = LabelWidth(this, m_lblOvenCooldownTimeout, scale);
	const int cdTolLblW = LabelWidth(this, m_lblOvenCooldownTol, scale);
	const int durLblW = LabelWidth(this, m_lblAgingDur, scale);
	const int sampleLblW = LabelWidth(this, m_lblAgingSample, scale);
	const int minSsrLblW = LabelWidth(this, m_lblAgingMinSsr, scale);
	const int maxSsrLblW = LabelWidth(this, m_lblAgingMaxSsr, scale);
	const int minCurLblW = LabelWidth(this, m_lblAgingMinCur, scale);
	const int maxCurLblW = LabelWidth(this, m_lblAgingMaxCur, scale);
	const int minTempLblW = LabelWidth(this, m_lblAgingMinTemp, scale);
	const int maxTempLblW = LabelWidth(this, m_lblAgingMaxTemp, scale);
	const int minAvddLblW = LabelWidth(this, m_lblAgingMinAvdd, scale);
	const int maxAvddLblW = LabelWidth(this, m_lblAgingMaxAvdd, scale);
	const int minIovddLblW = LabelWidth(this, m_lblAgingMinIovdd, scale);
	const int maxIovddLblW = LabelWidth(this, m_lblAgingMaxIovdd, scale);
	const int minDvddLblW = LabelWidth(this, m_lblAgingMinDvdd, scale);
	const int maxDvddLblW = LabelWidth(this, m_lblAgingMaxDvdd, scale);
	const int voltModeLblW = LabelWidth(this, m_lblAgingVoltMode, scale);
	const int voltScaleLblW = LabelWidth(this, m_lblAgingVoltScale, scale);
	const int regAvddLblW = LabelWidth(this, m_lblAgingRegAvdd, scale);
	const int regAvddHLblW = LabelWidth(this, m_lblAgingRegAvddH, scale);
	const int regIovddLblW = LabelWidth(this, m_lblAgingRegIovdd, scale);
	const int regIovddHLblW = LabelWidth(this, m_lblAgingRegIovddH, scale);
	const int regDvddLblW = LabelWidth(this, m_lblAgingRegDvdd, scale);
	const int regDvddHLblW = LabelWidth(this, m_lblAgingRegDvddH, scale);

	const int btnH = editH;
	const int btnTestW = max((int)(72 * scale), LabelWidth(this, m_btnOvenTest, scale) + (int)(16 * scale));
	const int btnReadW = max((int)(56 * scale), LabelWidth(this, m_btnOvenReadPv, scale) + (int)(16 * scale));
	const int btnSetW = max((int)(52 * scale), LabelWidth(this, m_btnOvenSetTemp, scale) + (int)(16 * scale));
	const int btnStartW = max((int)(52 * scale), LabelWidth(this, m_btnOvenStart, scale) + (int)(16 * scale));
	const int btnStopW = max((int)(52 * scale), LabelWidth(this, m_btnOvenStop, scale) + (int)(16 * scale));
	const int profileComboH = editH + max((int)(120 * scale), 100);
	const int heatComboH = profileComboH;

	const int g1Top = y;
	int gy = g1Top + m.grpHdr + (int)(4 * scale);
	MoveWnd(&m_chkAgingEn, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkAgingEn);
	gy += chkH + m.gap;
	MoveWnd(&m_chkAgingHeatStart, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkAgingHeatStart);
	const int g1H = gy + chkH + m.grpPadB - g1Top;
	MoveWnd(&m_grpAging, CRect(x, g1Top, x + w, g1Top + g1H));
	show(m_grpAging);
	y = g1Top + g1H + m.grpGap;

	const int g2Top = y;
	gy = g2Top + m.grpHdr + (int)(4 * scale);

	MoveWnd(&m_lblOvenProfile, CRect(ix, gy, ix + profLblW, gy + rowH));
	MoveWnd(&m_cmbOvenProfile, CRect(ix + profLblW + colGap, gy, ix + iw, gy + profileComboH));
	m_cmbOvenProfile.SetItemHeight(-1, rowH);
	for (int ci = 0; ci < m_cmbOvenProfile.GetCount(); ci++)
		m_cmbOvenProfile.SetItemHeight(ci, rowH);
	gy += m.rowStep;

	MoveWnd(&m_lblOvenHeatMode, CRect(ix, gy, ix + heatLblW, gy + rowH));
	MoveWnd(&m_cmbOvenHeatMode, CRect(ix + heatLblW + colGap, gy, ix + iw, gy + heatComboH));
	m_cmbOvenHeatMode.SetItemHeight(-1, rowH);
	for (int ci = 0; ci < m_cmbOvenHeatMode.GetCount(); ci++)
		m_cmbOvenHeatMode.SetItemHeight(ci, rowH);
	gy += m.rowStep;

	MoveWnd(&m_lblOvenHost, CRect(ix, gy, ix + hostLblW, gy + rowH));
	MoveWnd(&m_edOvenHost, CRect(ix + hostLblW + colGap, gy, ix + hostLblW + colGap + edHost, gy + editH));
	MoveWnd(&m_lblOvenPort, CRect(ix + hostLblW + colGap + edHost + colGap, gy, ix + hostLblW + colGap + edHost + colGap + portLblW, gy + rowH));
	MoveWnd(&m_edOvenPort, CRect(ix + hostLblW + colGap + edHost + colGap + portLblW + colGap, gy, ix + hostLblW + colGap + edHost + colGap + portLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblOvenUnit, CRect(ix, gy, ix + unitLblW, gy + rowH));
	MoveWnd(&m_edOvenUnit, CRect(ix + unitLblW + colGap, gy, ix + unitLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblOvenTarget, CRect(ix + unitLblW + colGap + edShort + colGap, gy, ix + unitLblW + colGap + edShort + colGap + targetLblW, gy + rowH));
	MoveWnd(&m_edOvenTarget, CRect(ix + unitLblW + colGap + edShort + colGap + targetLblW + colGap, gy, ix + unitLblW + colGap + edShort + colGap + targetLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblOvenTol, CRect(ix, gy, ix + tolLblW, gy + rowH));
	MoveWnd(&m_edOvenTol, CRect(ix + tolLblW + colGap, gy, ix + tolLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblOvenWait, CRect(ix + tolLblW + colGap + edMed + colGap, gy, ix + tolLblW + colGap + edMed + colGap + waitLblW, gy + rowH));
	MoveWnd(&m_edOvenWait, CRect(ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap, gy, ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblOvenPoll, CRect(ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap + edShort + colGap, gy, ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap + edShort + colGap + pollLblW, gy + rowH));
	MoveWnd(&m_edOvenPoll, CRect(ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap + edShort + colGap + pollLblW + colGap, gy, ix + tolLblW + colGap + edMed + colGap + waitLblW + colGap + edShort + colGap + pollLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_chkOvenDual, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkOvenDual);
	gy += m.rowStep;

	MoveWnd(&m_lblOvenDPv, CRect(ix, gy, ix + dPvLblW, gy + rowH));
	MoveWnd(&m_edOvenDPv, CRect(ix + dPvLblW + colGap, gy, ix + dPvLblW + colGap + edShort, gy + editH));
	show(m_lblOvenDPv);
	show(m_edOvenDPv);
	gy += m.rowStep;

	MoveWnd(&m_chkOvenCooldown, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkOvenCooldown);
	gy += m.rowStep;

	MoveWnd(&m_lblOvenCooldownTarget, CRect(ix, gy, ix + cdTargetLblW, gy + rowH));
	MoveWnd(&m_edOvenCooldownTarget, CRect(ix + cdTargetLblW + colGap, gy, ix + cdTargetLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblOvenCooldownTimeout, CRect(ix + cdTargetLblW + colGap + edMed + colGap, gy, ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW, gy + rowH));
	MoveWnd(&m_edOvenCooldownTimeout, CRect(ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap, gy, ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblOvenCooldownTol, CRect(ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap + edShort + colGap, gy, ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap + edShort + colGap + cdTolLblW, gy + rowH));
	MoveWnd(&m_edOvenCooldownTol, CRect(ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap + edShort + colGap + cdTolLblW + colGap, gy, ix + cdTargetLblW + colGap + edMed + colGap + cdTimeoutLblW + colGap + edShort + colGap + cdTolLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	const int btnRowW = btnTestW + btnReadW + btnSetW + btnStartW + btnStopW + colGap * 4;
	int bx = ix;
	if (btnRowW < iw)
		bx = ix + (iw - btnRowW) / 2;
	MoveWnd(&m_btnOvenTest, CRect(bx, gy, bx + btnTestW, gy + btnH));
	bx += btnTestW + colGap;
	MoveWnd(&m_btnOvenReadPv, CRect(bx, gy, bx + btnReadW, gy + btnH));
	bx += btnReadW + colGap;
	MoveWnd(&m_btnOvenSetTemp, CRect(bx, gy, bx + btnSetW, gy + btnH));
	bx += btnSetW + colGap;
	MoveWnd(&m_btnOvenStart, CRect(bx, gy, bx + btnStartW, gy + btnH));
	bx += btnStartW + colGap;
	MoveWnd(&m_btnOvenStop, CRect(bx, gy, bx + btnStopW, gy + btnH));

	if (allowShow)
	{
		const CWnd* ovenCtrls[] = {
			&m_lblOvenProfile, &m_cmbOvenProfile, &m_lblOvenHeatMode, &m_cmbOvenHeatMode,
			&m_lblOvenHost, &m_edOvenHost, &m_lblOvenPort, &m_edOvenPort,
			&m_lblOvenUnit, &m_edOvenUnit, &m_lblOvenTarget, &m_edOvenTarget,
			&m_lblOvenTol, &m_edOvenTol, &m_lblOvenWait, &m_edOvenWait,
			&m_lblOvenPoll, &m_edOvenPoll, &m_chkOvenDual, &m_lblOvenDPv, &m_edOvenDPv,
			&m_chkOvenCooldown, &m_lblOvenCooldownTarget, &m_edOvenCooldownTarget,
			&m_lblOvenCooldownTimeout, &m_edOvenCooldownTimeout,
			&m_lblOvenCooldownTol, &m_edOvenCooldownTol,
			&m_btnOvenTest, &m_btnOvenReadPv, &m_btnOvenSetTemp, &m_btnOvenStart, &m_btnOvenStop,
		};
		for (size_t i = 0; i < sizeof(ovenCtrls) / sizeof(ovenCtrls[0]); i++)
		{
			CWnd* p = const_cast<CWnd*>(ovenCtrls[i]);
			p->EnableWindow(TRUE);
			p->ShowWindow(SW_SHOW);
		}
	}
	const int g2H = gy + btnH + m.grpPadB - g2Top;
	MoveWnd(&m_grpOven, CRect(x, g2Top, x + w, g2Top + g2H));
	if (allowShow)
	{
		m_grpOven.EnableWindow(TRUE);
		m_grpOven.ShowWindow(SW_SHOW);
	}
	y = g2Top + g2H + m.grpGap;

	const int g3Top = y;
	gy = g3Top + m.grpHdr + (int)(4 * scale);

	MoveWnd(&m_lblAgingDur, CRect(ix, gy, ix + durLblW, gy + rowH));
	MoveWnd(&m_edAgingDur, CRect(ix + durLblW + colGap, gy, ix + durLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingSample, CRect(ix + durLblW + colGap + edShort + colGap, gy, ix + durLblW + colGap + edShort + colGap + sampleLblW, gy + rowH));
	MoveWnd(&m_edAgingSample, CRect(ix + durLblW + colGap + edShort + colGap + sampleLblW + colGap, gy, ix + durLblW + colGap + edShort + colGap + sampleLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinSsr, CRect(ix, gy, ix + minSsrLblW, gy + rowH));
	MoveWnd(&m_edAgingMinSsr, CRect(ix + minSsrLblW + colGap, gy, ix + minSsrLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxSsr, CRect(ix + minSsrLblW + colGap + edMed + colGap, gy, ix + minSsrLblW + colGap + edMed + colGap + maxSsrLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxSsr, CRect(ix + minSsrLblW + colGap + edMed + colGap + maxSsrLblW + colGap, gy, ix + minSsrLblW + colGap + edMed + colGap + maxSsrLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinCur, CRect(ix, gy, ix + minCurLblW, gy + rowH));
	MoveWnd(&m_edAgingMinCur, CRect(ix + minCurLblW + colGap, gy, ix + minCurLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxCur, CRect(ix + minCurLblW + colGap + edMed + colGap, gy, ix + minCurLblW + colGap + edMed + colGap + maxCurLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxCur, CRect(ix + minCurLblW + colGap + edMed + colGap + maxCurLblW + colGap, gy, ix + minCurLblW + colGap + edMed + colGap + maxCurLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinTemp, CRect(ix, gy, ix + minTempLblW, gy + rowH));
	MoveWnd(&m_edAgingMinTemp, CRect(ix + minTempLblW + colGap, gy, ix + minTempLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxTemp, CRect(ix + minTempLblW + colGap + edMed + colGap, gy, ix + minTempLblW + colGap + edMed + colGap + maxTempLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxTemp, CRect(ix + minTempLblW + colGap + edMed + colGap + maxTempLblW + colGap, gy, ix + minTempLblW + colGap + edMed + colGap + maxTempLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinAvdd, CRect(ix, gy, ix + minAvddLblW, gy + rowH));
	MoveWnd(&m_edAgingMinAvdd, CRect(ix + minAvddLblW + colGap, gy, ix + minAvddLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxAvdd, CRect(ix + minAvddLblW + colGap + edMed + colGap, gy, ix + minAvddLblW + colGap + edMed + colGap + maxAvddLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxAvdd, CRect(ix + minAvddLblW + colGap + edMed + colGap + maxAvddLblW + colGap, gy, ix + minAvddLblW + colGap + edMed + colGap + maxAvddLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinIovdd, CRect(ix, gy, ix + minIovddLblW, gy + rowH));
	MoveWnd(&m_edAgingMinIovdd, CRect(ix + minIovddLblW + colGap, gy, ix + minIovddLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxIovdd, CRect(ix + minIovddLblW + colGap + edMed + colGap, gy, ix + minIovddLblW + colGap + edMed + colGap + maxIovddLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxIovdd, CRect(ix + minIovddLblW + colGap + edMed + colGap + maxIovddLblW + colGap, gy, ix + minIovddLblW + colGap + edMed + colGap + maxIovddLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingMinDvdd, CRect(ix, gy, ix + minDvddLblW, gy + rowH));
	MoveWnd(&m_edAgingMinDvdd, CRect(ix + minDvddLblW + colGap, gy, ix + minDvddLblW + colGap + edMed, gy + editH));
	MoveWnd(&m_lblAgingMaxDvdd, CRect(ix + minDvddLblW + colGap + edMed + colGap, gy, ix + minDvddLblW + colGap + edMed + colGap + maxDvddLblW, gy + rowH));
	MoveWnd(&m_edAgingMaxDvdd, CRect(ix + minDvddLblW + colGap + edMed + colGap + maxDvddLblW + colGap, gy, ix + minDvddLblW + colGap + edMed + colGap + maxDvddLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	if (allowShow)
	{
		const CWnd* g[] = {
			&m_lblAgingDur, &m_edAgingDur, &m_lblAgingSample, &m_edAgingSample,
			&m_lblAgingMinSsr, &m_edAgingMinSsr, &m_lblAgingMaxSsr, &m_edAgingMaxSsr,
			&m_lblAgingMinCur, &m_edAgingMinCur, &m_lblAgingMaxCur, &m_edAgingMaxCur,
			&m_lblAgingMinTemp, &m_edAgingMinTemp, &m_lblAgingMaxTemp, &m_edAgingMaxTemp,
			&m_lblAgingMinAvdd, &m_edAgingMinAvdd, &m_lblAgingMaxAvdd, &m_edAgingMaxAvdd,
			&m_lblAgingMinIovdd, &m_edAgingMinIovdd, &m_lblAgingMaxIovdd, &m_edAgingMaxIovdd,
			&m_lblAgingMinDvdd, &m_edAgingMinDvdd, &m_lblAgingMaxDvdd, &m_edAgingMaxDvdd,
		};
		for (size_t i = 0; i < sizeof(g) / sizeof(g[0]); i++)
		{
			CWnd* p = const_cast<CWnd*>(g[i]);
			p->EnableWindow(TRUE);
			p->ShowWindow(SW_SHOW);
		}
	}
	const int g3H = gy + m.grpPadB - g3Top;
	MoveWnd(&m_grpAgingGate, CRect(x, g3Top, x + w, g3Top + g3H));
	if (allowShow)
	{
		m_grpAgingGate.EnableWindow(TRUE);
		m_grpAgingGate.ShowWindow(SW_SHOW);
	}
	y = g3Top + g3H + m.grpGap;

	const int g4Top = y;
	gy = g4Top + m.grpHdr + (int)(4 * scale);
	MoveWnd(&m_chkAgingVoltEn, CRect(ix, gy, ix + iw, gy + chkH));
	show(m_chkAgingVoltEn);
	gy += m.rowStep;

	MoveWnd(&m_lblAgingVoltMode, CRect(ix, gy, ix + voltModeLblW, gy + rowH));
	MoveWnd(&m_edAgingVoltMode, CRect(ix + voltModeLblW + colGap, gy, ix + voltModeLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingVoltScale, CRect(ix + voltModeLblW + colGap + edShort + colGap, gy, ix + voltModeLblW + colGap + edShort + colGap + voltScaleLblW, gy + rowH));
	MoveWnd(&m_edAgingVoltScale, CRect(ix + voltModeLblW + colGap + edShort + colGap + voltScaleLblW + colGap, gy, ix + voltModeLblW + colGap + edShort + colGap + voltScaleLblW + colGap + edMed, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingRegAvdd, CRect(ix, gy, ix + regAvddLblW, gy + rowH));
	MoveWnd(&m_edAgingRegAvdd, CRect(ix + regAvddLblW + colGap, gy, ix + regAvddLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingRegIovdd, CRect(ix + regAvddLblW + colGap + edShort + colGap, gy, ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW, gy + rowH));
	MoveWnd(&m_edAgingRegIovdd, CRect(ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap, gy, ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingRegDvdd, CRect(ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap + edShort + colGap, gy, ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap + edShort + colGap + regDvddLblW, gy + rowH));
	MoveWnd(&m_edAgingRegDvdd, CRect(ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap + edShort + colGap + regDvddLblW + colGap, gy, ix + regAvddLblW + colGap + edShort + colGap + regIovddLblW + colGap + edShort + colGap + regDvddLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	MoveWnd(&m_lblAgingRegAvddH, CRect(ix, gy, ix + regAvddHLblW, gy + rowH));
	MoveWnd(&m_edAgingRegAvddH, CRect(ix + regAvddHLblW + colGap, gy, ix + regAvddHLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingRegIovddH, CRect(ix + regAvddHLblW + colGap + edShort + colGap, gy, ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW, gy + rowH));
	MoveWnd(&m_edAgingRegIovddH, CRect(ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap, gy, ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap + edShort, gy + editH));
	MoveWnd(&m_lblAgingRegDvddH, CRect(ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap + edShort + colGap, gy, ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap + edShort + colGap + regDvddHLblW, gy + rowH));
	MoveWnd(&m_edAgingRegDvddH, CRect(ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap + edShort + colGap + regDvddHLblW + colGap, gy, ix + regAvddHLblW + colGap + edShort + colGap + regIovddHLblW + colGap + edShort + colGap + regDvddHLblW + colGap + edShort, gy + editH));
	gy += m.rowStep;

	if (allowShow)
	{
		const CWnd* v[] = {
			&m_chkAgingVoltEn, &m_lblAgingVoltMode, &m_edAgingVoltMode,
			&m_lblAgingVoltScale, &m_edAgingVoltScale,
			&m_lblAgingRegAvdd, &m_edAgingRegAvdd, &m_lblAgingRegAvddH, &m_edAgingRegAvddH,
			&m_lblAgingRegIovdd, &m_edAgingRegIovdd, &m_lblAgingRegIovddH, &m_edAgingRegIovddH,
			&m_lblAgingRegDvdd, &m_edAgingRegDvdd, &m_lblAgingRegDvddH, &m_edAgingRegDvddH,
		};
		for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); i++)
		{
			CWnd* p = const_cast<CWnd*>(v[i]);
			p->EnableWindow(TRUE);
			p->ShowWindow(SW_SHOW);
		}
	}
	const int g4H = gy + m.grpPadB - g4Top;
	MoveWnd(&m_grpAgingVolt, CRect(x, g4Top, x + w, g4Top + g4H));
	if (allowShow)
	{
		m_grpAgingVolt.EnableWindow(TRUE);
		m_grpAgingVolt.ShowWindow(SW_SHOW);
	}
	y = g4Top + g4H + m.grpGap;

	const int g5Top = y;
	int advY = g5Top + m.grpHdr + (int)(4 * scale);
	const int advEdW = max((int)(52 * scale), 48);
	auto placeAdvField = [&](int rowY, int& ax, CStatic& lbl, CEdit& ed) {
		const int lw = LabelWidth(this, lbl, scale);
		MoveWnd(&lbl, CRect(ax, rowY, ax + lw, rowY + rowH));
		MoveWnd(&ed, CRect(ax + lw + colGap, rowY, ax + lw + colGap + advEdW, rowY + editH));
		ax += lw + colGap + advEdW + colGap;
	};
	int ax = ix;
	placeAdvField(advY, ax, m_lblOvenRegSet, m_edOvenRegSet);
	placeAdvField(advY, ax, m_lblOvenCoilStart, m_edOvenCoilStart);
	placeAdvField(advY, ax, m_lblOvenCoilStop, m_edOvenCoilStop);
	advY += m.rowStep;
	ax = ix;
	placeAdvField(advY, ax, m_lblOvenRegPv, m_edOvenRegPv);
	placeAdvField(advY, ax, m_lblOvenRegRun, m_edOvenRegRun);
	advY += m.rowStep;
	ax = ix;
	placeAdvField(advY, ax, m_lblOvenRunBit, m_edOvenRunBit);
	placeAdvField(advY, ax, m_lblOvenRegFault, m_edOvenRegFault);

	if (allowShow)
	{
		const CWnd* a[] = {
			&m_lblOvenRegSet, &m_edOvenRegSet, &m_lblOvenCoilStart, &m_edOvenCoilStart,
			&m_lblOvenCoilStop, &m_edOvenCoilStop, &m_lblOvenRegPv, &m_edOvenRegPv,
			&m_lblOvenRegRun, &m_edOvenRegRun, &m_lblOvenRunBit, &m_edOvenRunBit,
			&m_lblOvenRegFault, &m_edOvenRegFault,
		};
		for (size_t i = 0; i < sizeof(a) / sizeof(a[0]); i++)
		{
			CWnd* p = const_cast<CWnd*>(a[i]);
			p->EnableWindow(TRUE);
			p->ShowWindow(SW_SHOW);
		}
	}
	const int g5H = max(advY + rowH + m.grpPadB - g5Top, m.grpHdr + m.grpPadB + m.rowStep);
	MoveWnd(&m_grpOvenAdv, CRect(x, g5Top, x + w, g5Top + g5H));
	if (allowShow)
	{
		m_grpOvenAdv.EnableWindow(TRUE);
		m_grpOvenAdv.ShowWindow(SW_SHOW);
	}
	y = g5Top + g5H + m.grpGap;

	MoveWnd(&m_stAgingHint, CRect(ix, y, ix + iw, y + hintH));
	show(m_stAgingHint);
	y += hintH + pad;

	if (allowShow)
		UpdateOvenRegEditEnableState();

	return y - y0;
}
