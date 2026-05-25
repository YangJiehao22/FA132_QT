#pragma once

#include "DtCarFunction.h"

/** Larger checkbox + row height for channel picker (VS2013 CCheckListBox is tiny by default). */
class CDtChannelCheckList : public CCheckListBox
{
public:
	CDtChannelCheckList();

	void EnsureUiFont();
	void SetUiScale(double scale);

	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual int CheckFromPoint(CPoint point, BOOL& bOnCheck) const;

protected:
	CFont m_font;

	double m_uiScale;

	int CheckPx() const;
	int RowH() const;
	int PadL() const;
	int TextGap() const;
};

class CDtChannelDlg : public CDialogEx
{
public:
	CDtChannelDlg(DtCarFunction* pFn, CWnd* pParent = NULL);

	enum { IDD = IDD_DIALOG_CHANNEL };

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

	int m_rowDev[MAX_CC16 * MAX_DEV * MAX_VC];
	int m_rowVc[MAX_CC16 * MAX_DEV * MAX_VC];
	int m_rowCount;

	void BuildChannelList();
	void ApplyChecksFromMemory();
	void ReadChecksToMemory();
	void UpdateSelectionStatus();
	void LayoutChannelDialog();
};
