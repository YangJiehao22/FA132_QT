#include "stdafx.h"
#include "DtChannelDlg.h"
#include "DtZhUtf8.h"
#include "DtEncoding.h"
#include "afxdialogex.h"

CDtChannelDlg::CDtChannelDlg(DtCarFunction* pFn, CWnd* pParent)
	: CDialogEx(IDD_DIALOG_CHANNEL, pParent)
	, m_pFn(pFn)
{
	memset(m_hDevItem, 0, sizeof(m_hDevItem));
}

void CDtChannelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CHANNEL, m_tree);
}

BEGIN_MESSAGE_MAP(CDtChannelDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CH_ALL, &CDtChannelDlg::OnBnClickedSelectAll)
	ON_BN_CLICKED(IDC_BTN_CH_NONE, &CDtChannelDlg::OnBnClickedClearAll)
	ON_NOTIFY(NM_CLICK, IDC_TREE_CHANNEL, &CDtChannelDlg::OnTvnClickTree)
END_MESSAGE_MAP()

BOOL CDtChannelDlg::OnInitDialog()
{
	if (!CDialogEx::OnInitDialog())
		return FALSE;

	if (m_tree.GetSafeHwnd() == NULL)
		m_tree.SubclassDlgItem(IDC_TREE_CHANNEL, this);

	SetWindowText(ZH_UTF8(kDlgChannelTitle));
	if (CWnd* p = GetDlgItem(IDOK))
		p->SetWindowText(ZH_UTF8(kSpecOk));
	if (CWnd* p = GetDlgItem(IDCANCEL))
		p->SetWindowText(ZH_UTF8(kSpecCancel));
	if (CWnd* p = GetDlgItem(IDC_BTN_CH_ALL))
		p->SetWindowText(ZH_UTF8(kDlgChannelAll));
	if (CWnd* p = GetDlgItem(IDC_BTN_CH_NONE))
		p->SetWindowText(ZH_UTF8(kDlgChannelNone));
	if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
		p->SetWindowText(ZH_UTF8(kDlgChannelHint));

	if (m_pFn == NULL || m_pFn->m_iEnumDevNum <= 0)
	{
		if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
			p->SetWindowText(ZH_UTF8(kDlgChannelNeedEnum));
		return TRUE;
	}

	if (m_tree.GetSafeHwnd() == NULL)
	{
		if (CWnd* p = GetDlgItem(IDC_STATIC_CH_HINT))
			p->SetWindowText(_T("Tree control init failed (IDC_TREE_CHANNEL). Rebuild project."));
		return TRUE;
	}

	m_tree.ModifyStyle(0, TVS_CHECKBOXES | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);

	int vcNum = m_pFn->m_iVcNum;
	if (vcNum < 1)
		vcNum = 1;

	for (int d = 0; d < m_pFn->m_iEnumDevNum; d++)
	{
		CString devLabel;
		devLabel.Format(_T("Dev%d"), d);
		const char* name = m_pFn->m_cDeviceName[d];
		if (name != NULL && name[0] != 0)
			devLabel += CString(_T("  ")) + CString(name);

		m_hDevItem[d] = m_tree.InsertItem(devLabel, TVI_ROOT, TVI_LAST);
		if (m_hDevItem[d] == NULL)
			continue;
		m_tree.SetCheck(m_hDevItem[d], m_pFn->IsDevEnabled(d));

		for (int v = 0; v < vcNum; v++)
		{
			CString vcLabel;
			vcLabel.Format(_T("VC%d  (CH%02d)"), v, d * vcNum + v + 1);
			HTREEITEM hVc = m_tree.InsertItem(vcLabel, m_hDevItem[d], TVI_LAST);
			m_tree.SetItemData(hVc, (DWORD_PTR)((d << 8) | v));
			m_tree.SetCheck(hVc, m_pFn->IsVcEnabled(d, v));
		}
		m_tree.Expand(m_hDevItem[d], TVE_EXPAND);
	}

	return TRUE;
}

void CDtChannelDlg::SetVcChecks(int dev, BOOL checked)
{
	if (m_hDevItem[dev] == NULL)
		return;
	HTREEITEM hChild = m_tree.GetChildItem(m_hDevItem[dev]);
	while (hChild != NULL)
	{
		m_tree.SetCheck(hChild, checked);
		hChild = m_tree.GetNextSiblingItem(hChild);
	}
}

void CDtChannelDlg::SyncDevCheckFromVcs(int dev)
{
	if (m_hDevItem[dev] == NULL)
		return;
	BOOL any = FALSE;
	HTREEITEM hChild = m_tree.GetChildItem(m_hDevItem[dev]);
	while (hChild != NULL)
	{
		if (m_tree.GetCheck(hChild))
			any = TRUE;
		hChild = m_tree.GetNextSiblingItem(hChild);
	}
	m_tree.SetCheck(m_hDevItem[dev], any);
}

void CDtChannelDlg::OnTvnClickTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	if (m_pFn == NULL)
		return;

	CPoint pt;
	GetCursorPos(&pt);
	m_tree.ScreenToClient(&pt);

	UINT flags = 0;
	HTREEITEM hItem = m_tree.HitTest(pt, &flags);
	if (hItem == NULL)
		return;

	m_tree.SelectItem(hItem);
	const BOOL checked = m_tree.GetCheck(hItem);

	for (int d = 0; d < m_pFn->m_iEnumDevNum; d++)
	{
		if (hItem == m_hDevItem[d])
		{
			SetVcChecks(d, checked);
			return;
		}
	}

	HTREEITEM hParent = m_tree.GetParentItem(hItem);
	for (int d = 0; d < m_pFn->m_iEnumDevNum; d++)
	{
		if (hParent == m_hDevItem[d])
		{
			SyncDevCheckFromVcs(d);
			return;
		}
	}
}

void CDtChannelDlg::OnBnClickedSelectAll()
{
	for (int d = 0; d < (m_pFn != NULL ? m_pFn->m_iEnumDevNum : 0); d++)
	{
		if (m_hDevItem[d] != NULL)
			m_tree.SetCheck(m_hDevItem[d], TRUE);
		SetVcChecks(d, TRUE);
	}
}

void CDtChannelDlg::OnBnClickedClearAll()
{
	for (int d = 0; d < (m_pFn != NULL ? m_pFn->m_iEnumDevNum : 0); d++)
	{
		if (m_hDevItem[d] != NULL)
			m_tree.SetCheck(m_hDevItem[d], FALSE);
		SetVcChecks(d, FALSE);
	}
}

void CDtChannelDlg::OnOK()
{
	if (m_pFn != NULL && m_pFn->m_iEnumDevNum > 0)
	{
		for (int d = 0; d < m_pFn->m_iEnumDevNum; d++)
		{
			const BOOL devOn = (m_hDevItem[d] != NULL) ? m_tree.GetCheck(m_hDevItem[d]) : FALSE;
			m_pFn->m_iDevEnable[d] = devOn ? 1 : 0;

			HTREEITEM hChild = (m_hDevItem[d] != NULL) ? m_tree.GetChildItem(m_hDevItem[d]) : NULL;
			for (int v = 0; v < m_pFn->m_iVcNum; v++)
			{
				BOOL vcOn = FALSE;
				if (hChild != NULL)
				{
					vcOn = m_tree.GetCheck(hChild);
					hChild = m_tree.GetNextSiblingItem(hChild);
				}
				m_pFn->m_iVcEnable[d][v] = (devOn && vcOn) ? 1 : 0;
			}
		}
		if (!m_pFn->HasAnyChannelEnabled())
		{
			AfxMessageBox(ZH_UTF8(kDlgChannelPickOne), MB_ICONWARNING);
			return;
		}
		m_pFn->SaveChannelEnableIni();
	}
	CDialogEx::OnOK();
}
