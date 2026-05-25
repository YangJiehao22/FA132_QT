#pragma once

/** Enable Per-Monitor V2 DPI awareness. Call once at startup. */
void DtEnableProcessDpiAwareness();

/** UI scale vs 96 DPI (1.0 = 100%, 1.25 = 125%). Prefer hwnd's monitor when available. */
double DtGetWindowUiScale(HWND hwnd);

/** Create dialog font (negative height = points). hwndForDpi optional for monitor DPI. */
bool DtCreateUiFont(CFont& font, int pointSize, bool bold = false, HWND hwndForDpi = NULL);

/** Owner-draw checkbox: scales box with DPI (fixes tiny system checkbox on 125%/150%). */
class CDtScaledCheckBox : public CButton
{
public:
	CDtScaledCheckBox();

	void SetUiScale(double scale);
	void SetCheckFont(CFont* pFont);

protected:
	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	double m_scale;
	CFont* m_pFont;
	int BoxPx() const;
	int PadL() const;
	int TextGap() const;
};

/** Owner-draw radio button with DPI-scaled circle. */
class CDtScaledRadio : public CButton
{
public:
	CDtScaledRadio();

	void SetUiScale(double scale);
	void SetRadioFont(CFont* pFont);

protected:
	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	double m_scale;
	CFont* m_pFont;
	int CirclePx() const;
	int PadL() const;
	int TextGap() const;
};
