#pragma once

#include "afxwin.h"
#include "ezCarDTCCM_SDK/ezCarDtccmDef.h"

/** MES input strip: one work order + four FA132 board-edge scan fields (UI only, no upload). */
class CDtMesBar : public CWnd
{
public:
	CDtMesBar();

	BOOL Create(CWnd* pParent, UINT nId);
	void SetUiScale(double scale);
	void SetBarFont(CFont* pFont);
	void SetEditFont(CFont* pFont);
	int PreferredHeight() const;
	void LayoutChildren();
	void SetSlotOnline(int slot, bool online);
	void SetActiveTab(int tab);
	void SyncSlotStates();
	void UpdateBoundSummary();

	CString GetWorkOrderText() const;
	CString GetBoardCode(int slot) const;
	bool IsBoardCodeFilled(int slot) const;
	bool IsWorkOrderLocked() const;

	/** Handle Enter in scan fields; returns TRUE if consumed. */
	BOOL PreTranslateScanMessage(MSG* pMsg);

protected:
	afx_msg void OnPaint();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBoardCodeChanged(UINT nID);
	afx_msg void OnWorkOrderChanged();
	afx_msg void OnWoLockClicked();
	DECLARE_MESSAGE_MAP()

private:
	struct MesBarLayout
	{
		CRect rcInner;
		CRect rcWoEdit;
		CRect rcBound;
		CRect rcBoardEdit[MAX_CC16];
		int row2Top;
		int slotLblH;
		int colW;
		int colGap;
	};

	void CalcLayout(MesBarLayout& out) const;
	void RefreshBoundSummaryText();
	int MeasureBoundBadgeWidth(const CString& boundText) const;
	void ApplySlotEditState(int slot);
	void SetWorkOrderLocked(bool locked);
	void ApplyWoLockButtonLook();
	void FocusNextEmptyOnlineSlot(int afterSlot);
	int CountOnlineSlots() const;
	int CountBoundOnlineSlots() const;
	static CString TrimScanText(const CString& text);

	double m_uiScale;
	CFont* m_pFont;
	CFont* m_pEditFont;
	bool m_slotOnline[MAX_CC16];
	int m_activeTab;
	bool m_bWorkOrderLocked;
	CString m_strBoundSummary;

	CEdit m_editWorkOrder;
	CStatic m_staticWoLocked;
	CEdit m_editBoard[MAX_CC16];
	CStatic m_lblWorkOrder;
	CStatic m_lblBound;
	CButton m_btnWoLock;

	CBrush m_brEdit;
	CBrush m_brEditDisabled;
	CBrush m_brEditLocked;
};
