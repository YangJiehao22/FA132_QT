#pragma once

#include "DtCarFunction.h"

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
	afx_msg void OnTvnClickTree(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
	DtCarFunction* m_pFn;
	CTreeCtrl m_tree;
	HTREEITEM m_hDevItem[MAX_CC16 * MAX_DEV];

	void SyncDevCheckFromVcs(int dev);
	void SetVcChecks(int dev, BOOL checked);
};
