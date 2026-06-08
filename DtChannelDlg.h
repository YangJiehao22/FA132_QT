#pragma once

#include "DtCarFunction.h"

class CDtChannelDlg;

/** Larger checkbox + row height for channel picker (VS2013 CCheckListBox is tiny by default). */
class CDtChannelCheckList : public CCheckListBox
{
public:
	CDtChannelCheckList();

	void EnsureUiFont();
	void SetUiScale(double scale);
	void SetChannelDlg(CDtChannelDlg* pDlg) { m_pChannelDlg = pDlg; }

	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual int CheckFromPoint(CPoint point, BOOL& bOnCheck) const;

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
	CFont m_font;

	double m_uiScale;
	CDtChannelDlg* m_pChannelDlg;

	int CheckPx() const;
	int RowH() const;
	int PadL() const;
	int TextGap() const;
};

/** Max list rows: 128 channels + 4 FA132 group headers. */
enum { kChannelDlgMaxRows = MAX_CC16 * MAX_DEV * MAX_VC + MAX_CC16 };

class CDtChannelDlg : public CDialogEx
{
public:
	CDtChannelDlg(DtCarFunction* pFn, CWnd* pParent = NULL);

	enum { IDD = IDD_DIALOG_CHANNEL };

	/** Group header row: 0=none, 1=checked, 2=indeterminate. */
	int GetFa132SlotCheckState(int slot);
	bool IsGroupHeaderRow(int row) const;
	int GroupSlotForRow(int row) const;
	void OnGroupHeaderClicked(int row);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnBnClickedSelectAll();
	afx_msg void OnBnClickedClearAll();
	afx_msg void OnListCheckChange();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	DtCarFunction* m_pFn;
	CDtChannelCheckList m_list;

	int m_rowDev[kChannelDlgMaxRows];
	int m_rowVc[kChannelDlgMaxRows];
	bool m_rowPickable[kChannelDlgMaxRows];
	bool m_rowIsGroupHeader[kChannelDlgMaxRows];
	int m_rowSlot[kChannelDlgMaxRows];
	int m_rowCount;

	void BuildChannelList();
	void ApplyChecksFromMemory();
	void ReadChecksToMemory();
	void UpdateSelectionStatus();
	void LayoutChannelDialog();
	void SetFa132SlotChecks(int slot, bool checked);
	void ToggleFa132SlotChecks(int slot);
};
