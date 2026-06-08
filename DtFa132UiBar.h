#pragma once

#include "afxwin.h"
#include "ezCarDTCCM_SDK/ezCarDtccmDef.h"
#include "DtCarFunction.h"

/** Owner-draw FA132 tab strip with online/offline + per-box test result. */
class CDtFa132TabCtrl : public CTabCtrl
{
public:
	CDtFa132TabCtrl();

	void SetUiScale(double scale);
	void SetTabFont(CFont* pFont);
	void SetSlotOnline(int slot, bool online);
	void SetSlotTestResult(int slot, DtCarFunction::Fa132SlotTestResult result, int ngCount);
	void ApplyTabSizing(int totalWidthPx);
	int PreferredTabRowHeight() const;
	/** Window height for tabs-only strip (no page display area). */
	int MeasureStripHeight(int totalWidthPx);

	void DrawTabItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

private:
	double m_uiScale;
	CFont* m_pFont;
	bool m_slotOnline[MAX_CC16];
	DtCarFunction::Fa132SlotTestResult m_slotTestResult[MAX_CC16];
	int m_slotNgCount[MAX_CC16];
};

/** Painted status row: online count badge + four FA132 slot chips. */
class CDtFa132OverviewBar : public CStatic
{
public:
	CDtFa132OverviewBar();

	void SetUiScale(double scale);
	void SetBarFont(CFont* pFont);
	void SetOnlineCount(int count);
	void SetActiveTab(int tab);
	void SetSlotOnline(int slot, bool online);
	void SetSlotTestResult(int slot, DtCarFunction::Fa132SlotTestResult result, int ngCount);
	void SetRunSummary(bool hasResult, int ngSlotCount, int okSlotCount);
	int PreferredBarHeight() const;

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	double m_uiScale;
	CFont* m_pFont;
	int m_onlineCount;
	int m_activeTab;
	bool m_slotOnline[MAX_CC16];
	DtCarFunction::Fa132SlotTestResult m_slotTestResult[MAX_CC16];
	int m_slotNgCount[MAX_CC16];
	bool m_hasRunResult;
	int m_ngSlotCount;
	int m_okSlotCount;
};
