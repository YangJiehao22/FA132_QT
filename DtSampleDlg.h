
// DtSampleDlg.h
//

#pragma once
#include "afxwin.h"
#include "afxbutton.h"
#include "DtCarFunction.h"

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

	int ReSize();
	void ChangeModuleSize(int nID, int nCamCnt, CRect rect, UINT VideoWinID);
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

	UINT m_uWndCtrlID[MAX_CC16 * MAX_DEV][MAX_VC];

	/** Async firmware burn (UI thread must not call RunFirmwareBurnParallel). */
	HANDLE m_hFwBurnThread;
	DWORD m_fwBurnGeneration;
	DWORD m_fwBurnHandledGen;
	BOOL m_bFwPowerCyclePending;
	BOOL m_bPreviewFrozen;

	void WaitForFwBurnThread(DWORD timeoutMs);
	void BeginFirmwareBurnAsync();
	void ContinueAfterFirmwareBurn(bool burnOk);
	bool RunFirmwareBurnVerifyOrStop();
	void StopCaptureForFirmwarePowerCycle();
	void StopCaptureAndShowResults(bool forFwPowerCycle);
	afx_msg LRESULT OnFwBurnDone(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFwBurnProgress(WPARAM wParam, LPARAM lParam);

	static unsigned __stdcall FirmwareBurnWorkerProc(void* param);

	void ClearFwBurnCellOverlay(bool invalidateCells = true);
	bool AnyFirmwareBurnChannelFailed() const;
	void PaintPreviewCellsBurnResult();
	void ResetFwBurnCellOverlay();
	void InvalidateEnabledPreviewCells();
	void PaintPreviewCellsTestResult();
	void PaintPreviewCellsIdle();
	void PaintPreviewCellState(int dev, int vc, LPCTSTR tip, COLORREF bg, COLORREF fg);
	void PaintPreviewCellFirmware(int dev, int vc);
};
