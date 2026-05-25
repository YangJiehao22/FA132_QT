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
	afx_msg void OnBnClickedChkFwEn();

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
	bool m_specFrameReady;
	double m_specLayoutScale;
	int m_specMargin;
	int m_specTabH;
	int m_specBtnBandH;
	int m_specBtnH;
	int m_specBtnW;

	void ApplyDialogFonts();
	void EnsureStandardCheckAndRadioButtons();
	void ResizeSpecClient(int clientW, int clientH);
	void ClampSpecDialogToWorkArea(const CRect& workArea);
	int MaxClientHeightForWorkArea(int clientW, const CRect& workArea, double scale) const;
	void PlaceSpecTabAndButtons(const CRect& cr);
	void ShowSpecPage(int page);
	void UpdateFormulaText();
	/** Once per open/DPI: size dialog, tab strip, OK/Cancel. */
	void InitSpecDialogFrame(bool force);
	/** Tab switch: layout active page only (no resize). */
	void RelayoutSpecPage();
	void AdjustSpecPageZOrder();
	void RaiseSpecDialogButtons();
	void HideGatePageControls();
	void HideBadPixelPageControls();
	void HideFirmwarePageControls();
	void HideAllSpecPageControls();
	double GetSpecUiScale() const;
	int MeasureMaxPageHeight(const CRect& viewport, double scale);
	int LayoutGatePage(const CRect& viewport, double scale, bool bShow);
	int LayoutBadPixelPage(const CRect& viewport, double scale, bool bShow);
	int LayoutFirmwarePage(const CRect& viewport, double scale, bool bShow);
	void FillFirmwareFovCombo();
	void UpdateFirmwarePathLabel();
	afx_msg void OnCbnSelchangeFwFov();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	void UpdateBadPixelLabels();
	void UpdateBadPixelAlgoUi();
	void UpdateBadPixelSnapTypeUi();
};

