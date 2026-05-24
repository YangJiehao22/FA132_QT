#pragma once

#include "DtCarFunction.h"

/** GateSpec.ini + reserved tab (DtSpecSettings.rc). All channels share global [limits]. */
class CDtSpecDlg : public CDialogEx
{
public:
	explicit CDtSpecDlg(DtCarFunction* pFn, CWnd* pParent = NULL);

	enum { IDD = IDD_DIALOG_SPEC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTcnSelchangeTabSpec(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnBpBrowse();
	afx_msg void OnBnClickedBpSave();
	afx_msg void OnBnClickedRadBpAlgo();

	DECLARE_MESSAGE_MAP()

	DtCarFunction* m_pFn;

	CFont m_fontTitle;
	CFont m_fontBody;
	CFont m_fontSmall;

	CTabCtrl m_tab;
	CStatic m_stTitle;
	CStatic m_stPath;
	CStatic m_stHint;
	CStatic m_stFormula;
	CButton m_grpTiming;
	CButton m_grpLimits;
	CButton m_grpTempI2c;
	CButton m_grpBadPixel;
	CButton m_chkBpEn;
	CStatic m_lblBpAlgo;
	CButton m_radBpNeighbor;
	CButton m_radBpHuawei;
	CButton m_grpBpHuawei;
	CButton m_grpBpNeighbor;
	CStatic m_lblBpClusterTh;
	CEdit m_edBpClusterTh;
	CStatic m_lblBpClusterMin;
	CEdit m_edBpClusterMin;
	CStatic m_lblBpSinglePpm;
	CEdit m_edBpSinglePpm;
	CButton m_chkBpGrGbToG;
	CStatic m_lblBpMax;
	CStatic m_lblBpHotDelta;
	CStatic m_lblBpHotAbs;
	CStatic m_lblBpBorder;
	CEdit m_edBpMax;
	CEdit m_edBpHotDelta;
	CEdit m_edBpHotAbs;
	CEdit m_edBpBorder;
	CButton m_chkBpSave;
	CButton m_grpBpSnapFiles;
	CButton m_chkBpSaveBmp;
	CButton m_chkBpSavePacked;
	CButton m_chkBpSaveU12;
	CButton m_chkBpSaveU10;
	CStatic m_lblBpDir;
	CEdit m_edBpDir;
	CButton m_btnBpBrowse;
	CStatic m_stBpHint;
	CButton m_grpFirmware;
	CButton m_chkFwEn;
	CStatic m_lblFwFov;
	CComboBox m_cmbFwFov;
	CStatic m_lblFwWarmup;
	CEdit m_edFwWarmup;
	CStatic m_lblFwPath;
	CStatic m_stFwPath;
	CStatic m_stFwHint;
	CStatic m_lblDelay;
	CEdit m_edDelay;
	CStatic m_lblDef1;
	CStatic m_lblDef2;
	CStatic m_lblDef3;
	CStatic m_lblDef4;
	CStatic m_lblDef5;
	CStatic m_lblDef6;
	CEdit m_edDefMinSsr;
	CEdit m_edDefMaxSsr;
	CEdit m_edDefMinCur;
	CEdit m_edDefMaxCur;
	CEdit m_edDefMinTemp;
	CEdit m_edDefMaxTemp;
	CButton m_chkTempEn;
	CStatic m_lblTempAddr;
	CStatic m_lblTempMode;
	CStatic m_lblTempRegLo;
	CStatic m_lblTempRegHi;
	CStatic m_lblTempCoeffLo;
	CStatic m_lblTempCoeffHi;
	CStatic m_lblTempDiv;
	CStatic m_lblTempOffset;
	CEdit m_edTempAddr;
	CEdit m_edTempMode;
	CEdit m_edTempRegLo;
	CEdit m_edTempRegHi;
	CEdit m_edTempCoeffLo;
	CEdit m_edTempCoeffHi;
	CEdit m_edTempDiv;
	CEdit m_edTempOffset;

	GateChannelLimits m_def;
	GateSensorTempI2c m_tempI2c;
	GateBadPixelDarkCfg m_badPixel;
	GateFirmwareBurnCfg m_firmware;

	int m_specActivePage;

	void ApplyDialogFonts();
	void ShowSpecPage(int page);
	void UpdateFormulaText();
	/** Resize dialog for DPI/screen and lay out the active tab. */
	void LayoutSpecDialog();
	void HideGatePageControls();
	void HideBadPixelPageControls();
	void HideFirmwarePageControls();
	void HideAllSpecPageControls();
	double GetSpecUiScale() const;
	int LayoutGatePage(const CRect& viewport, double scale);
	int LayoutBadPixelPage(const CRect& viewport, double scale);
	int LayoutFirmwarePage(const CRect& viewport, double scale);
	void FillFirmwareFovCombo();
	void UpdateFirmwarePathLabel();
	afx_msg void OnCbnSelchangeFwFov();
	void UpdateBadPixelAlgoUi();
	void UpdateBadPixelSnapTypeUi();
};

