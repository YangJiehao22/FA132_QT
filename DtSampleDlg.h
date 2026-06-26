
// DtSampleDlg.h
//

#pragma once
#include "afxwin.h"
#include "afxbutton.h"
#include "DtCarFunction.h"
#include "DtFa132UiBar.h"
#include "DtMesBar.h"

// CDtSampleDlg dialog
class CDtSampleDlg : public CDialogEx
{
public:
	CDtSampleDlg(CWnd* pParent = NULL);
	~CDtSampleDlg();

	UINT64 GetSysTime();

	enum { IDD = IDD_DTSAMPLE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnTcnSelchangeTabFa132(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();

	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonSpec();
	afx_msg void OnBnClickedButtonLoad();
	afx_msg void OnBnClickedButtonEnum();
	afx_msg void OnBnClickedButtonChannel();
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonStart();

	afx_msg LRESULT OnMsg(WPARAM wP, LPARAM lP);
	afx_msg LRESULT OnDtCarDraw(WPARAM wP, LPARAM lP);

public:
	void PopupMenu(int iIndex, int uWndCtrIDIndex);

	int ReSize(bool schedulePreviewRepaint = true);
	void ChangeModuleSize(int nID, int nCamCnt, CRect rect, UINT VideoWinID, int globalDevBase);
	void InitFa132TabUi();
	void RefreshFa132Ui();
	void UpdateFa132OverviewOnly();
	void SyncFa132StripVisualState();
	void FocusFirstNgFa132Tab();
	void SchedulePreviewGridRepaint();
	void RedrawPreviewGrid();
	void OnFa132TabChanged();
	int ActiveFa132TabBaseDev() const;
	int GlobalDevForLayout(int localDev) const;
	bool IsGlobalDevOnActiveTab(int globalDev) const;
	CWnd* GetPreviewWndForGlobalDev(int globalDev, int vc) const;
	void ClearAllPreviewVideoBindings();
	void ClearAllPreviewCellSurfaces(COLORREF bg);
	void PaintPreviewCellVideoIdle(int globalDev, int vc);
	void PaintPreviewCellDisconnected(int localDev, int vc);
	int LayoutToolbar(const CRect& rcClient);
	void InitStyledToolbarButtons();
	void UpdatePrimaryButtonLooks();
	void InitChromePalette();

public:
	DtCarFunction m_dtFunction;

	LARGE_INTEGER m_liFreq;

	UINT m_uCurSel;
	UINT m_uWndCtrIDIndex;

	CEdit m_editMsg;
	CFont m_fontLog;
	CFont m_fontBtn;

	CMFCButton m_btnLoad;
	CMFCButton m_btnEdit;
	CMFCButton m_btnSpec;
	CMFCButton m_btnEnum;
	CMFCButton m_btnChannel;
	CMFCButton m_btnOpen;
	CMFCButton m_btnStart;

	CBrush m_brDlg;
	CBrush m_brLog;
	CBrush m_brIni;
	CBrush m_brVideo;
	int m_cyToolbarBottom;

	BOOL m_bOpen;
	BOOL m_bStart;

	UINT m_uWndCtrlID[MAX_DEV][MAX_VC];

	CDtMesBar m_mesBar;
	CDtFa132TabCtrl m_tabFa132;
	CDtFa132OverviewBar m_fa132Overview;
	int m_iActiveFa132Tab;
	BOOL m_bPreviewGridRepaintPosted;
	CFont m_fontTab;
	CFont m_fontOverview;
	CFont m_fontMesEdit;

	/** Async firmware prep / burn (UI thread must not block on InitGrab or burn). */
	HANDLE m_hFwPrepThread;
	HANDLE m_hFwBurnThread;
	DWORD m_fwPrepGeneration;
	DWORD m_fwBurnGeneration;
	DWORD m_fwBurnHandledGen;
	BOOL m_bFwPowerCyclePending;
	BOOL m_bPreviewFrozen;
	/** Keep Burn OK/NG painted while capture stopped for post-burn power cycle. */
	BOOL m_bPreviewBurnStickyHold;
	/** After post-burn power-on: streaming wait / no-signal until final results. */
	BOOL m_bPreviewPostPowerStream;
	BOOL m_bPreviewStreamSettleDone;
	/** STREAM_GATE phase-2: wait after I2C before sampling fps for light test. */
	BOOL m_bLightTestAfterI2cSettle;

	/** Aging flow after light test (aging_test.Enabled=1). */
	BOOL m_bAgingFlowActive;
	BOOL m_bCooldownActive;
	BOOL m_bOvenHeatInvolved;
	BOOL m_bAgingDataFinalized;
	int m_agingElapsedSec;
	DWORD m_agingWaitStartTick;
	DWORD m_agingMonitorStartTick;
	DWORD m_agingCooldownStartTick;
	bool m_bAgingFinalAllPass;
	BOOL m_bCooldownLogStarted;
	DWORD m_lastCooldownLogTick;
	int m_lastAgingProgressLogMin;

	void BeginOvenHeatIfNeeded();
	void BeginAgingAfterLightTest();
	void FinalizeAgingProductionData(bool ovenFault);
	void BeginOvenCooldownAfterAging(bool ovenFault);
	void CompleteAgingRunUi();
	void FinishAgingProductionRun(bool ovenFault);
	void OnOvenWaitTimer();
	void OnAgingSampleTimer();
	void OnOvenCooldownTimer();
	void UpdateAgingProgressTitle();
	bool ShouldShowAgingMonitor() const;
	static unsigned __stdcall OvenHeatWorkerProc(void* param);

	void WaitForFwPrepThread(DWORD timeoutMs);
	void WaitForFwBurnThread(DWORD timeoutMs);
	bool BeginFirmwarePrepAsync();
	void BeginFirmwareBurnAsync();
	void ContinueAfterFirmwareBurn();
	/** Enabled=0: read SensorID after Start + DelayMs (stream up), not in Prep. */
	void RunSensorIdAfterStreamIfNeeded();
	/** After Start or post-burn: optional power-cycle, 14-reg verify, then light test. */
	void ScheduleFirmwareVerifyThenLightTest();
	bool RunFirmwareBurnVerifyOrStop();
	void StopCaptureForFirmwarePowerCycle();
	void StopCaptureAndShowResults(bool forFwPowerCycle);
	void ReleaseTestBoxAndNotifyPeer(bool allPass);
	afx_msg LRESULT OnFwPrepDone(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFwBurnDone(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFwBurnProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPreviewGridRepaint(WPARAM wParam, LPARAM lParam);

	static unsigned __stdcall FirmwarePrepWorkerProc(void* param);
	static unsigned __stdcall FirmwareBurnWorkerProc(void* param);

	void ClearFwBurnCellOverlay(bool invalidateCells = true);
	bool AnyFirmwareBurnChannelFailed() const;
	void PaintPreviewCellsBurnResult();
	void PaintPreviewCellsBurnSticky();
	void ResetFwBurnCellOverlay();
	void InvalidateEnabledPreviewCells();
	void PaintPreviewCellsTestResult();
	void PaintPreviewCellsIdle();
	void PaintPreviewCellState(int dev, int vc, LPCTSTR tip, COLORREF bg, COLORREF fg, bool dashedBorder = false);
	void PaintPreviewCellOff(int dev, int vc);
	void PaintPreviewCellWait(int dev, int vc);
	void PaintPreviewCellBurnOk(int dev, int vc);
	void PaintPreviewCellBurnNg(int dev, int vc);
	void PaintPreviewCellsFirmwareBurn();
	void PaintPreviewCellFirmware(int dev, int vc);
	void PaintPreviewCellStreamingWait(int dev, int vc);
	void PaintPreviewCellNoSignal(int dev, int vc);
	void PaintPreviewCellStreamNg(int dev, int vc);
	void PaintPreviewCellsAgingMonitor();
	void PaintPreviewCellAgingNg(int dev, int vc, LPCTSTR failReason);
	void PaintPreviewCellsStreamState();
	bool ShouldShowPreviewStreamState() const;
	bool ShouldPaintPreviewStreamNg(int dev) const;
	void StartPreviewStreamRefreshTimer();
	void StopPreviewStreamRefreshTimer();
	void PaintPreviewCellsPostPowerStream();
	void PaintPreviewCellsPostPowerStreamState();
	bool IsPreviewCellStreamingLive(int dev, int vc) const;
	bool IsPreviewChannelOn(int dev, int vc) const;
};
