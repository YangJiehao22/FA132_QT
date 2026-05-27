#include "stdafx.h"
#include "DtSample.h"
#include "DtSampleDlg.h"
#include "DtSpecDlg.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"
#include "afxdialogex.h"

#include <sys/stat.h>

#ifndef TIMER_ID_STREAM_GATE
#define TIMER_ID_STREAM_GATE 1
#endif
#ifndef TIMER_ID_FW_BURN
#define TIMER_ID_FW_BURN 2
#endif
#ifndef TIMER_ID_FW_POWER_SETTLE
#define TIMER_ID_FW_POWER_SETTLE 3
#endif
#ifndef TIMER_ID_FW_POWER_OFF
#define TIMER_ID_FW_POWER_OFF 4
#endif

struct DtFwWorkerParam
{
	HWND hwnd;
	DtCarFunction* fn;
	DWORD generation;
};

namespace {

static void ApplyMfcToolbarLook(CMFCButton& b, COLORREF face, COLORREF text, COLORREF textHot)
{
	b.m_nFlatStyle = CMFCButton::BUTTONSTYLE_SEMIFLAT;
	b.m_bTransparent = FALSE;
	b.m_bDrawFocus = FALSE;
	b.m_bDontUseWinXPTheme = TRUE;
	b.SetFaceColor(face);
	b.SetTextColor(text);
	b.SetTextHotColor(textHot);
}

static void SetTriVertex(TRIVERTEX& v, int x, int y, COLORREF c)
{
	v.x = x;
	v.y = y;
	v.Red = (COLOR16)(GetRValue(c) << 8);
	v.Green = (COLOR16)(GetGValue(c) << 8);
	v.Blue = (COLOR16)(GetBValue(c) << 8);
	v.Alpha = 0;
}

} // namespace

static CRITICAL_SECTION s_dailyLogCs;
struct DailyLogCsInit {
	DailyLogCsInit() { InitializeCriticalSection(&s_dailyLogCs); }
	~DailyLogCsInit() { DeleteCriticalSection(&s_dailyLogCs); }
} s_dailyLogCsGuard;

static CStringA AppGetExeDirectoryA()
{
	char sz[MAX_PATH];
	if (::GetModuleFileNameA(NULL, sz, MAX_PATH) == 0)
		return CStringA(".\\");
	char* p = strrchr(sz, '\\');
	if (p)
		p[1] = '\0';
	else
		sz[0] = '\0';
	return CStringA(sz);
}

/** Append one line to exeDir\log\YYYY-MM-DD.log with local timestamp (thread-safe). */
static void AppendDailyTimestampedLog(const char* pszBody)
{
	if (pszBody == NULL)
		return;
	size_t len = strlen(pszBody);
	while (len > 0 && (pszBody[len - 1] == '\n' || pszBody[len - 1] == '\r'))
		len--;

	EnterCriticalSection(&s_dailyLogCs);

	SYSTEMTIME st = {};
	::GetLocalTime(&st);
	CStringA dir = AppGetExeDirectoryA() + "log\\";
	::CreateDirectoryA(dir.GetString(), NULL);

	CStringA path;
	path.Format("%s%04d-%02d-%02d.log", dir.GetString(),
		(int)st.wYear, (int)st.wMonth, (int)st.wDay);

	FILE* fp = NULL;
	if (fopen_s(&fp, path.GetString(), "ab") != 0 || fp == NULL)
	{
		LeaveCriticalSection(&s_dailyLogCs);
		return;
	}

	struct _stat64 stFile = {};
	const bool newFile = (_stat64(path.GetString(), &stFile) != 0 || stFile.st_size == 0);
	if (newFile)
		fwrite("\xEF\xBB\xBF", 1, 3, fp);

	CStringA ts;
	ts.Format("[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
		(int)st.wYear, (int)st.wMonth, (int)st.wDay,
		(int)st.wHour, (int)st.wMinute, (int)st.wSecond, (int)st.wMilliseconds);

	const CStringA tsUtf8 = AcpToUtf8(ts);
	fwrite(tsUtf8.GetString(), 1, (size_t)tsUtf8.GetLength(), fp);
	if (len > 0)
	{
		const CStringA bodyUtf8 = AcpToUtf8(pszBody, (int)len);
		fwrite(bodyUtf8.GetString(), 1, (size_t)bodyUtf8.GetLength(), fp);
	}
	fwrite("\r\n", 1, 2, fp);
	fclose(fp);

	LeaveCriticalSection(&s_dailyLogCs);
}

/* Append text to log edit control; mirror to daily log file (printf-style). */
void msg(LPCSTR lpszFmt, ...)
{
    static CDtSampleDlg* pDlg = NULL;

    char *szTmp = (char*)malloc(512);
    if (szTmp != NULL)
    {
        va_list argList;
        va_start(argList, lpszFmt);
        _vsnprintf(szTmp, 512, lpszFmt, argList); 
        va_end(argList);
		szTmp[511] = '\0';
		AppendDailyTimestampedLog(szTmp);
        if (pDlg == NULL)
        {
            pDlg = (CDtSampleDlg*)AfxGetMainWnd();
        }

        if (pDlg != NULL)
        {
            if (!PostMessage(pDlg->GetSafeHwnd(), WM_MSG, (WPARAM)szTmp, 0))
            {
                free(szTmp);
            }
        }
        else
        {
            free(szTmp);
        }
    }
}

// About dialog (Help menu)
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDtSampleDlg dialog


CDtSampleDlg::CDtSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDtSampleDlg::IDD, pParent)
    , m_uCurSel(0)
	, m_bOpen(FALSE)
	, m_bStart(FALSE)
	, m_cyToolbarBottom(0)
	, m_hFwPrepThread(NULL)
	, m_hFwBurnThread(NULL)
	, m_fwPrepGeneration(0)
	, m_fwBurnGeneration(0)
	, m_fwBurnHandledGen((DWORD)-1)
	, m_bFwPowerCyclePending(FALSE)
	, m_bPreviewFrozen(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    QueryPerformanceFrequency(&m_liFreq);
}

CDtSampleDlg::~CDtSampleDlg()
{
	++m_fwBurnGeneration;
	WaitForFwBurnThread(INFINITE);
}

void CDtSampleDlg::InitChromePalette()
{
	if (m_brDlg.GetSafeHandle() != NULL)
		return;
	m_brDlg.CreateSolidBrush(RGB(246, 249, 252));
	m_brLog.CreateSolidBrush(RGB(30, 41, 59));
	m_brIni.CreateSolidBrush(RGB(252, 253, 255));
	m_brVideo.CreateSolidBrush(RGB(17, 24, 39));
}

BOOL CDtSampleDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect rc;
	GetClientRect(&rc);
	/* Full solid erase first. (ExcludeClipRect on all children + full-client GradientFill left huge black regions on maximize.) */
	const COLORREF kFill = RGB(246, 249, 252);
	pDC->FillSolidRect(&rc, kFill);

	const int bandH = min(rc.Height(), max(72, m_cyToolbarBottom + 12));
	TRIVERTEX tv[2];
	SetTriVertex(tv[0], 0, 0, RGB(224, 234, 248));
	SetTriVertex(tv[1], 0, max(0, bandH - 1), kFill);
	GRADIENT_RECT gr = { 0, 1 };
	const int save = pDC->SaveDC();
	CRect band(0, 0, rc.Width(), bandH);
	pDC->IntersectClipRect(&band);
	::GradientFill(pDC->GetSafeHdc(), tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
	pDC->RestoreDC(save);

	CRect topStripe(0, 0, rc.Width(), 4);
	pDC->FillSolidRect(&topStripe, RGB(37, 99, 235));
	if (m_cyToolbarBottom > 4)
	{
		CRect sep(0, m_cyToolbarBottom - 1, rc.Width(), m_cyToolbarBottom);
		pDC->FillSolidRect(&sep, RGB(186, 214, 252));
	}
	return TRUE;
}

HBRUSH CDtSampleDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd == NULL)
		return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	const UINT id = pWnd->GetDlgCtrlID();
	if (id == IDC_EDIT_MSG2)
	{
		pDC->SetBkColor(RGB(30, 41, 59));
		pDC->SetTextColor(RGB(226, 232, 240));
		return (HBRUSH)m_brLog.GetSafeHandle();
	}
	if (id == IDC_EDIT_INI)
	{
		pDC->SetBkColor(RGB(252, 253, 255));
		pDC->SetTextColor(RGB(15, 23, 42));
		return (HBRUSH)m_brIni.GetSafeHandle();
	}
	if (id >= 2000 && id < 2000 + 32)
	{
		pDC->SetBkColor(RGB(17, 24, 39));
		pDC->SetTextColor(RGB(148, 163, 184));
		return (HBRUSH)m_brVideo.GetSafeHandle();
	}
	if (nCtlColor == CTLCOLOR_DLG)
	{
		pDC->SetBkColor(RGB(246, 249, 252));
		return (HBRUSH)m_brDlg.GetSafeHandle();
	}
	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

/* High-resolution timestamp in microseconds */
UINT64 CDtSampleDlg::GetSysTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (li.QuadPart * 1000000)/m_liFreq.QuadPart;
}

void CDtSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MSG2, m_editMsg);
	DDX_Text(pDX, IDC_EDIT_INI, m_dtFunction.m_strSensorIniPath);
}

BEGIN_MESSAGE_MAP(CDtSampleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_WM_CLOSE()
	ON_MESSAGE(WM_MSG, OnMsg)
	ON_MESSAGE(WM_DT_CAR_DRAW, OnDtCarDraw)
	ON_MESSAGE(WM_FW_PREP_DONE, OnFwPrepDone)
	ON_MESSAGE(WM_FW_BURN_DONE, OnFwBurnDone)
	ON_MESSAGE(WM_FW_BURN_PROGRESS, OnFwBurnProgress)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CDtSampleDlg::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_ENUM, &CDtSampleDlg::OnBnClickedButtonEnum)
	ON_BN_CLICKED(IDC_BUTTON_CHANNEL, &CDtSampleDlg::OnBnClickedButtonChannel)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CDtSampleDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_START, &CDtSampleDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, &CDtSampleDlg::OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_SPEC, &CDtSampleDlg::OnBnClickedButtonSpec)
END_MESSAGE_MAP()


BOOL CDtSampleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_dtFunction.PreloadEzCarSdkDlls();

	/* Parent erase (gradient) must not paint over children; fixes toolbar/video after maximize. */
	ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	InitChromePalette();
	SetWindowText(_T("QT_FA132_Software"));

	// About box menu in system menu
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Dialog icon (large and small)
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	InitStyledToolbarButtons();

	if (CWnd* pIni = GetDlgItem(IDC_EDIT_INI))
		pIni->ModifyStyleEx(0, WS_EX_STATICEDGE);
	if (m_editMsg.GetSafeHwnd())
		m_editMsg.ModifyStyleEx(0, WS_EX_STATICEDGE);

	int WndCtrlID = 2000;
	for (int i = 0;i < 8;i++)
	{
		for (int j = 0;j < 4;j++)
		{
			m_uWndCtrlID[i][j] = WndCtrlID;
			m_dtFunction.m_hWndVideo[i][j] = GetDlgItem(WndCtrlID)->GetSafeHwnd();
			WndCtrlID++;
		}
	}

	msgUtf8(DtZh::kLogAppBanner1);
	msgUtf8(DtZh::kLogAppBanner2);
    
    /* Auto-enumerate devices on startup */
    OnBnClickedButtonEnum();

	if (m_fontLog.GetSafeHandle() == NULL)
		m_fontLog.CreatePointFont(100, _T("Consolas"));
	if (m_editMsg.GetSafeHwnd())
		m_editMsg.SetFont(&m_fontLog);

	m_dtFunction.SetFirmwareBurnProgressWnd(m_hWnd);

	ReSize();
	m_dtFunction.InitAllPreviewDisplays();

	return TRUE;
}

void CDtSampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CDtSampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // paint icon when dialog is minimized

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		const int cxIcon = GetSystemMetrics(SM_CXICON);
		const int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		const int x = (rect.Width() - cxIcon + 1) / 2;
		const int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CDtSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CDtSampleDlg::OnDtCarDraw(WPARAM wP, LPARAM lP)
{
	(void)wP;
	DtUiDrawPack* p = (DtUiDrawPack*)lP;
	if (p == NULL || p->hVideoWnd == NULL || !::IsWindow(p->hVideoWnd))
		return DT_ERROR_FAILED;

	if (m_bPreviewFrozen || m_dtFunction.IsFirmwareBurnCellActive(p->devId, p->vcId))
		return DT_ERROR_OK;

	DrawImage_t di;
	memset(&di, 0, sizeof(di));
	di.hVideoWnd = p->hVideoWnd;
	di.nImgWndW = p->nImgWndW;
	di.nImgWndH = p->nImgWndH;
	di.bShowImg = p->bShowImg;
	di.bShowText = p->bShowText;
	di.szShowData = p->szShowData;
	return ::carDrawImage(di, p->vcId, p->devId);
}

LRESULT CDtSampleDlg::OnMsg(WPARAM wP, LPARAM lP)
{
    static int nLineCount = 0;

    char* lpszMsg = (LPSTR)wP;
    if (m_editMsg.GetSafeHwnd() != NULL)
    {
        if (nLineCount >= 10000)
        {
            nLineCount = 0;
            m_editMsg.Clear();
        }

        int len = m_editMsg.GetWindowTextLength();
        if (len >= 0)
        {
            if (len > 10000)
            {
                m_editMsg.Clear();
                m_editMsg.SetWindowText(lpszMsg);
            }
            else
            {
                m_editMsg.SetSel(len, len);
                m_editMsg.ReplaceSel(lpszMsg);
            }
        }
    }
    if (wP != NULL)
    {
        free(lpszMsg);
    }
    return 0;
}

void CDtSampleDlg::OnBnClickedButtonLoad()
{
	m_dtFunction.LoadIni();
    UpdateData(FALSE);
}

void CDtSampleDlg::OnBnClickedButtonEnum()
{
	m_dtFunction.Enum();
	ReSize();
}

void CDtSampleDlg::OnBnClickedButtonChannel()
{
	if (m_bStart)
		OnBnClickedButtonStart();
	if (m_bOpen)
	{
		m_dtFunction.Close();
		m_bOpen = FALSE;
		if (m_btnOpen.GetSafeHwnd())
			m_btnOpen.SetWindowText(_T("Open"));
		else if (GetDlgItem(IDC_BUTTON_OPEN))
			GetDlgItem(IDC_BUTTON_OPEN)->SetWindowText(_T("Open"));
		UpdatePrimaryButtonLooks();
		msgUtf8(DtZh::kDlgChannelAutoClose);
	}
	m_dtFunction.ShowChannelSelectDialog(this);
	ReSize();
	m_dtFunction.InitAllPreviewDisplays();
	m_bPreviewFrozen = FALSE;
	PaintPreviewCellsIdle();
}

void CDtSampleDlg::OnBnClickedButtonOpen()
{
	if (!m_bOpen)
	{
		if (!m_dtFunction.Open())
			return;
		m_bOpen = TRUE;
		m_bPreviewFrozen = FALSE;
		ReSize();
		m_dtFunction.InitAllPreviewDisplays();
		PaintPreviewCellsIdle();
	}
	else {
		if (m_bStart)
		{
			OnBnClickedButtonStart();
		}
		m_dtFunction.Close();
		m_bOpen = FALSE;
	}
	
	if (m_btnOpen.GetSafeHwnd())
		m_btnOpen.SetWindowText(m_bOpen ? _T("Close") : _T("Open"));
	else
		GetDlgItem(IDC_BUTTON_OPEN)->SetWindowText(m_bOpen ? "Close" : "Open");
	UpdatePrimaryButtonLooks();
}

void CDtSampleDlg::OnBnClickedButtonEdit()
{
    /* Open sensor INI with the default editor */
    ShellExecute(NULL, "open", m_dtFunction.m_strSensorIniPath, NULL, NULL, SW_SHOWNORMAL); 
}

void CDtSampleDlg::OnBnClickedButtonSpec()
{
	CDtSpecDlg dlg(&m_dtFunction, this);
	dlg.DoModal();
}

void CDtSampleDlg::OnBnClickedButtonStart()
{
	UpdateData(TRUE);
	if (!m_bStart)
	{
		m_bPreviewFrozen = FALSE;
		SetWindowText(_T("QT_FA132_Software"));
		m_dtFunction.ClearLightGateResults();
		m_dtFunction.ReadDtCarIni();
		m_dtFunction.ReadGateSpecIni();
		ReSize();
		const bool fwBurn = m_dtFunction.m_gateFirmwareBurn.enabled;
		const bool fwVerify = m_dtFunction.m_gateFirmwareBurn.verifyEnabled;
		KillTimer(TIMER_ID_FW_BURN);
		KillTimer(TIMER_ID_FW_POWER_SETTLE);
		KillTimer(TIMER_ID_FW_POWER_OFF);
		KillTimer(TIMER_ID_STREAM_GATE);
		if (fwBurn)
		{
			++m_fwPrepGeneration;
			if (!BeginFirmwarePrepAsync())
				return;
			m_bStart = TRUE;
			SetTimer(0, 1000, NULL);
		}
		else if (fwVerify)
		{
			if (!m_dtFunction.Start())
				return;
			m_bStart = TRUE;
			SetTimer(0, 1000, NULL);
			ScheduleFirmwareVerifyThenLightTest();
		}
		else if (!m_dtFunction.Start())
		{
			return;
		}
		else
		{
			m_bStart = TRUE;
			SetTimer(0, 1000, NULL);
			SetTimer(TIMER_ID_STREAM_GATE, m_dtFunction.m_specDelayMs, NULL);
		}
	}
	else {
		StopCaptureAndShowResults(m_bFwPowerCyclePending != FALSE);
	}
update_start_btn:
	if (m_btnStart.GetSafeHwnd())
		m_btnStart.SetWindowText(m_bStart ? _T("Stop") : _T("Start"));
	else
		GetDlgItem(IDC_BUTTON_START)->SetWindowText(m_bStart ? "Stop" : "Start");
	UpdatePrimaryButtonLooks();
}

void CDtSampleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (nType == SIZE_MINIMIZED)
		return;
	if (!m_editMsg.GetSafeHwnd())
		return;
	/* Always use live client rect (maximize / DPI paths can disagree with WM_SIZE cx,cy). */
	CRect rc;
	GetClientRect(&rc);
	if (rc.Width() <= 0 || rc.Height() <= 0)
		return;
	ReSize();
}

void CDtSampleDlg::OnClose()
{
	++m_fwPrepGeneration;
	++m_fwBurnGeneration;
	WaitForFwPrepThread(30000);
	WaitForFwBurnThread(30000);
    if (m_bOpen)
    {
        OnBnClickedButtonOpen();
    }

    CDialogEx::OnClose();
}

void CDtSampleDlg::PopupMenu(int iIndex, int uWndCtrIDIndex)
{
   
    CPoint pt;
    GetCursorPos(&pt);

    CMenu menu;

    menu.LoadMenu(IDR_MENU1);
    CMenu *pop = menu.GetSubMenu(0);

    m_uCurSel = (UINT)iIndex;
	m_uWndCtrIDIndex = (UINT)uWndCtrIDIndex;
	// MessageBox("Right-click command", "PopupMenu", MB_OK);
	//msg("Popup Menu %d\r\n",m_uCurSel);
    pop->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL);	
}

BOOL CDtSampleDlg::PreTranslateMessage(MSG* pMsg)
{
    if ( pMsg->message == WM_RBUTTONDOWN )
    {
        /* 32 previews: 8 Dev x 4 VC, map to m_hWndVideo[i][j] */
		 for (int i = 0; i < 8; i++) {
			 for (int j = 0; j < 4; j++)
			 {
				 if (pMsg->hwnd == GetDlgItem(m_uWndCtrlID[i][j])->GetSafeHwnd())
				 {
					 PopupMenu(i, j);
				 }
			 }
		 }
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}

BOOL CDtSampleDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{

    if ( HIWORD(wParam) == 0 )
    {
		
        /* Right-click context menu on preview */
        switch (LOWORD(wParam))
        {
        case ID_DEVICE_INTERNALSTATUS:
			::carShowInternalStatusDialog(m_hWnd, m_uCurSel);

            break;

        case ID_DEVICE_INTERNALDEBUG:			
            ::carShowInternalDebugDialog(m_hWnd, m_uCurSel);
            break;

        case ID_DEVICE_HOLD:
			::carGrabHold(m_uCurSel);
            break;

        case ID_DEVICE_RESTART:
			::carGrabRestart(0, 0, m_uCurSel);
            break;

		case ID_DEVICE_SAVEPIC:
			break;
		case ID_DEVICE_I2CDEBUG:
			m_dtFunction.ShowI2cDebug(m_uCurSel);
        }
    }


    return CDialogEx::OnCommand(wParam, lParam);
}

unsigned __stdcall CDtSampleDlg::FirmwarePrepWorkerProc(void* param)
{
	DtFwWorkerParam* ctx = (DtFwWorkerParam*)param;
	if (ctx == NULL)
		return 1;
	const HWND hwnd = ctx->hwnd;
	const DWORD gen = ctx->generation;
	DtCarFunction* fn = ctx->fn;
	delete ctx;
	bool ok = false;
	if (fn != NULL && ::IsWindow(hwnd))
		ok = (fn->StartFirmwarePrep() != 0);
	if (::IsWindow(hwnd))
		::PostMessage(hwnd, WM_FW_PREP_DONE, ok ? 1u : 0u, (LPARAM)gen);
	return ok ? 0u : 1u;
}

unsigned __stdcall CDtSampleDlg::FirmwareBurnWorkerProc(void* param)
{
	DtFwWorkerParam* ctx = (DtFwWorkerParam*)param;
	if (ctx == NULL)
		return 1;
	const HWND hwnd = ctx->hwnd;
	const DWORD gen = ctx->generation;
	DtCarFunction* fn = ctx->fn;
	delete ctx;
	bool ok = false;
	if (fn != NULL && ::IsWindow(hwnd))
		ok = fn->RunFirmwareBurnParallel(false);
	if (::IsWindow(hwnd))
		::PostMessage(hwnd, WM_FW_BURN_DONE, ok ? 1u : 0u, (LPARAM)gen);
	return ok ? 0u : 1u;
}

void CDtSampleDlg::WaitForFwPrepThread(DWORD timeoutMs)
{
	if (m_hFwPrepThread == NULL)
		return;
	const DWORD w = WaitForSingleObject(m_hFwPrepThread, timeoutMs);
	if (w == WAIT_OBJECT_0)
	{
		CloseHandle(m_hFwPrepThread);
		m_hFwPrepThread = NULL;
	}
}

void CDtSampleDlg::WaitForFwBurnThread(DWORD timeoutMs)
{
	if (m_hFwBurnThread == NULL)
		return;
	const DWORD w = WaitForSingleObject(m_hFwBurnThread, timeoutMs);
	if (w == WAIT_OBJECT_0)
	{
		CloseHandle(m_hFwBurnThread);
		m_hFwBurnThread = NULL;
	}
}

bool CDtSampleDlg::BeginFirmwarePrepAsync()
{
	WaitForFwPrepThread(0);
	if (m_hFwPrepThread != NULL)
		return false;
	WaitForFwBurnThread(0);
	DtFwWorkerParam* ctx = new DtFwWorkerParam;
	ctx->hwnd = m_hWnd;
	ctx->fn = &m_dtFunction;
	ctx->generation = m_fwPrepGeneration;
	unsigned tid = 0;
	m_hFwPrepThread = (HANDLE)_beginthreadex(NULL, 0, &FirmwarePrepWorkerProc, ctx, 0, &tid);
	if (m_hFwPrepThread == NULL)
	{
		delete ctx;
		msgUtf8(DtZh::kFwPrepAsyncFail);
		return false;
	}
	msgUtf8(DtZh::kFwPrepAsync);
	return true;
}

void CDtSampleDlg::BeginFirmwareBurnAsync()
{
	WaitForFwBurnThread(0);
	if (m_hFwBurnThread != NULL)
		return;
	m_dtFunction.ReadGateSpecIni();
	m_dtFunction.SetFirmwareBurnOverlayActive(true);
	ResetFwBurnCellOverlay();
	++m_fwBurnGeneration;
	m_fwBurnHandledGen = (DWORD)-1;
	DtFwWorkerParam* ctx = new DtFwWorkerParam;
	ctx->hwnd = m_hWnd;
	ctx->fn = &m_dtFunction;
	ctx->generation = m_fwBurnGeneration;
	unsigned tid = 0;
	m_hFwBurnThread = (HANDLE)_beginthreadex(NULL, 0, &FirmwareBurnWorkerProc, ctx, 0, &tid);
	if (m_hFwBurnThread == NULL)
	{
		delete ctx;
		msgUtf8(DtZh::kFwParallelFail);
		OnBnClickedButtonStart();
		return;
	}
	msgUtf8(DtZh::kFwBurnBg);
}

void CDtSampleDlg::StopCaptureForFirmwarePowerCycle()
{
	WaitForFwPrepThread(0);
	WaitForFwBurnThread(0);
	ClearFwBurnCellOverlay();
	if (m_bStart)
	{
		m_dtFunction.Stop();
		m_bStart = FALSE;
		KillTimer(0);
		KillTimer(TIMER_ID_FW_BURN);
		KillTimer(TIMER_ID_FW_POWER_SETTLE);
	}
	InvalidateEnabledPreviewCells();
}

void CDtSampleDlg::StopCaptureAndShowResults(bool forFwPowerCycle)
{
	++m_fwPrepGeneration;
	KillTimer(0);
	KillTimer(TIMER_ID_FW_BURN);
	KillTimer(TIMER_ID_FW_POWER_SETTLE);

	/* Hold grab before painting results so WorkProc can exit join quickly. */
	if (m_bStart)
		m_dtFunction.RequestStopCapture();

	if (!forFwPowerCycle)
	{
		++m_fwBurnGeneration;
		m_bFwPowerCyclePending = FALSE;
		if (!m_dtFunction.m_bLightGateHasResult && !AnyFirmwareBurnChannelFailed())
			m_dtFunction.ClearFirmwareBurnUiState();
		KillTimer(TIMER_ID_FW_POWER_OFF);
		KillTimer(TIMER_ID_STREAM_GATE);
		m_bPreviewFrozen = TRUE;
		if (m_dtFunction.m_bLightGateHasResult)
			PaintPreviewCellsTestResult();
		else if (AnyFirmwareBurnChannelFailed())
			PaintPreviewCellsBurnResult();
		else
			PaintPreviewCellsTestResult();
	}
	else
	{
		ClearFwBurnCellOverlay(false);
		KillTimer(TIMER_ID_FW_POWER_OFF);
		KillTimer(TIMER_ID_STREAM_GATE);
	}

	WaitForFwPrepThread(0);
	WaitForFwBurnThread(0);
	if (m_bStart)
	{
		m_dtFunction.Stop();
		m_bStart = FALSE;
	}
	if (!forFwPowerCycle)
	{
		if (m_btnStart.GetSafeHwnd())
			m_btnStart.SetWindowText(_T("Start"));
		else if (GetDlgItem(IDC_BUTTON_START))
			GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Start"));
		UpdatePrimaryButtonLooks();
	}
}

void CDtSampleDlg::ContinueAfterFirmwareBurn(bool burnOk)
{
	if (!burnOk)
	{
		const int stage = m_dtFunction.AnySensorIdReadFailed()
			? PROD_STAGE_SENSOR_ID : PROD_STAGE_BURN;
		m_dtFunction.FinalizeProductionRun(stage);
		SetWindowText(ZH_UTF8(kMainTitleTestNg));
		if (m_bStart)
			StopCaptureAndShowResults(FALSE);
		return;
	}
	ScheduleFirmwareVerifyThenLightTest();
}

void CDtSampleDlg::ScheduleFirmwareVerifyThenLightTest()
{
	const GateFirmwareBurnCfg& fw = m_dtFunction.m_gateFirmwareBurn;
	if (!fw.verifyEnabled)
	{
		int delay = fw.enabled ? fw.postBurnDelayMs : m_dtFunction.m_specDelayMs;
		if (fw.enabled && delay < 200)
			delay = 1000;
		if (!fw.enabled)
		{
			if (!m_dtFunction.Start())
				return;
			m_bStart = TRUE;
			SetTimer(0, 1000, NULL);
		}
		SetTimer(TIMER_ID_STREAM_GATE, delay, NULL);
		return;
	}
	/* PowerCycleAfter only after firmware burn; verify-only stays on one Start (no Stop). */
	if (fw.powerCycleAfter && fw.enabled)
	{
		msgUtf8(DtZh::kFwPowerCyclePlan);
		m_bFwPowerCyclePending = TRUE;
		StopCaptureForFirmwarePowerCycle();
		SetTimer(TIMER_ID_FW_POWER_OFF, 2000, NULL);
		return;
	}
	int settleMs = m_dtFunction.m_specDelayMs;
	if (settleMs < 200)
		settleMs = 200;
	if (fw.enabled)
		msgUtf8(DtZh::kFwPowerSettleWait, settleMs);
	else
		msgUtf8(DtZh::kFwVerifySettleNoPc, settleMs);
	SetTimer(TIMER_ID_FW_POWER_SETTLE, settleMs, NULL);
}

bool CDtSampleDlg::RunFirmwareBurnVerifyOrStop()
{
	const GateFirmwareBurnCfg& fw = m_dtFunction.m_gateFirmwareBurn;
	if (!fw.verifyEnabled)
		return true;
	m_dtFunction.ReadGateSpecIni();

	const bool usePrep = fw.verifyBeforeGrab;
	const bool captureWasUp = (m_dtFunction.m_bRunning != FALSE);
	if (usePrep)
	{
		if (captureWasUp)
			m_dtFunction.RequestStopCapture();
		if (!m_dtFunction.PrepareForFirmwareVerify())
		{
			if (captureWasUp)
				m_dtFunction.RestoreWorkCaptureAfterVerify();
			msgUtf8(DtZh::kFwVerifyPrepFail);
			m_dtFunction.FinalizeProductionRun(PROD_STAGE_VERIFY);
			SetWindowText(ZH_UTF8(kMainTitleTestNg));
			if (m_bStart)
				StopCaptureAndShowResults(FALSE);
			return false;
		}
	}

	msgUtf8(DtZh::kFwVerifyStart);
	const bool ok = m_dtFunction.RunFirmwareBurnVerifyAll();

	if (usePrep && captureWasUp)
		m_dtFunction.RestoreWorkCaptureAfterVerify();

	if (!ok)
	{
		msgUtf8(DtZh::kFwVerifyFail);
		m_dtFunction.FinalizeProductionRun(PROD_STAGE_VERIFY);
		SetWindowText(ZH_UTF8(kMainTitleTestNg));
		if (m_bStart)
			StopCaptureAndShowResults(FALSE);
		return false;
	}
	msgUtf8(DtZh::kFwVerifyOk);
	return true;
}

LRESULT CDtSampleDlg::OnFwPrepDone(WPARAM wParam, LPARAM lParam)
{
	WaitForFwPrepThread(0);
	const DWORD gen = (DWORD)lParam;
	if (gen != m_fwPrepGeneration)
		return 0;
	if (!m_bStart)
		return 0;

	const bool prepOk = (wParam != 0);
	if (!prepOk)
	{
		msgUtf8(DtZh::kFwPrepAsyncFail);
		++m_fwPrepGeneration;
		m_bStart = FALSE;
		KillTimer(0);
		KillTimer(TIMER_ID_FW_BURN);
		if (m_btnStart.GetSafeHwnd())
			m_btnStart.SetWindowText(_T("Start"));
		else if (GetDlgItem(IDC_BUTTON_START))
			GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Start"));
		UpdatePrimaryButtonLooks();
		return 0;
	}

	int warmup = m_dtFunction.m_gateFirmwareBurn.fwWarmupMs;
	if (warmup < 500)
		warmup = 500;
	msgUtf8(DtZh::kFwWaitWarmup, warmup);
	SetTimer(TIMER_ID_FW_BURN, warmup, NULL);
	return 0;
}

LRESULT CDtSampleDlg::OnFwBurnDone(WPARAM wParam, LPARAM lParam)
{
	WaitForFwBurnThread(0);
	const DWORD gen = (DWORD)lParam;
	if (gen != m_fwBurnGeneration)
		return 0;
	if (gen == m_fwBurnHandledGen)
		return 0;
	m_fwBurnHandledGen = gen;

	const bool burnOk = (wParam != 0);
	if (!burnOk)
	{
		ContinueAfterFirmwareBurn(false);
		return 0;
	}

	if (!m_bStart && !m_bFwPowerCyclePending)
		return 0;

	ClearFwBurnCellOverlay(false);
	ContinueAfterFirmwareBurn(true);
	return 0;
}

LRESULT CDtSampleDlg::OnFwBurnProgress(WPARAM wParam, LPARAM lParam)
{
	const int pct = (int)wParam;
	const int vc = (int)LOWORD(lParam);
	const int dev = (int)HIWORD(lParam);
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return 0;
	m_dtFunction.SetFirmwareBurnPercent(dev, vc, pct);
	PaintPreviewCellFirmware(dev, vc);
	return 0;
}

void CDtSampleDlg::ClearFwBurnCellOverlay(bool invalidateCells)
{
	m_dtFunction.ClearFirmwareBurnUiState();
	if (invalidateCells)
		InvalidateEnabledPreviewCells();
}

bool CDtSampleDlg::AnyFirmwareBurnChannelFailed() const
{
	if (!m_dtFunction.m_bFirmwareBurnHasResult)
		return false;
	for (int d = 0; d < m_dtFunction.m_iEnumDevNum; d++)
	{
		if (!m_dtFunction.IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_dtFunction.m_iVcNum; v++)
		{
			if (!m_dtFunction.IsVcEnabled(d, v))
				continue;
			if (!m_dtFunction.m_bFirmwareBurnPass[d][v])
				return true;
		}
	}
	return false;
}

void CDtSampleDlg::PaintPreviewCellsBurnResult()
{
	const COLORREF idleBg = RGB(55, 65, 81);
	const COLORREF idleFg = RGB(203, 213, 225);
	for (int d = 0; d < 8; d++)
	{
		for (int v = 0; v < 4; v++)
		{
			const bool channelOn = (d < m_dtFunction.m_iEnumDevNum && v < m_dtFunction.m_iVcNum
				&& m_dtFunction.IsDevEnabled(d) && m_dtFunction.IsVcEnabled(d, v));
			if (!channelOn)
			{
				CString tip;
				tip.Format(_T("D%d V%d\nIdle"), d, v);
				PaintPreviewCellState(d, v, tip, idleBg, idleFg);
				continue;
			}
			if (m_dtFunction.m_bFirmwareBurnPass[d][v])
			{
				CString tip;
				tip.Format(_T("D%d V%d\nIdle"), d, v);
				PaintPreviewCellState(d, v, tip, idleBg, idleFg);
				continue;
			}
			m_dtFunction.SetFirmwareBurnPercent(d, v, 100);
			PaintPreviewCellFirmware(d, v);
		}
	}
	m_dtFunction.ClearFirmwareBurnUiState();
}

void CDtSampleDlg::InvalidateEnabledPreviewCells()
{
	for (int d = 0; d < m_dtFunction.m_iEnumDevNum; d++)
	{
		if (!m_dtFunction.IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_dtFunction.m_iVcNum; v++)
		{
			if (!m_dtFunction.IsVcEnabled(d, v))
				continue;
			CWnd* pWnd = GetDlgItem(m_uWndCtrlID[d][v]);
			if (pWnd != NULL && pWnd->GetSafeHwnd() != NULL)
				pWnd->Invalidate(TRUE);
		}
	}
}

void CDtSampleDlg::PaintPreviewCellState(int dev, int vc, LPCTSTR tip, COLORREF bg, COLORREF fg)
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return;
	CWnd* pWnd = GetDlgItem(m_uWndCtrlID[dev][vc]);
	if (pWnd == NULL || pWnd->GetSafeHwnd() == NULL)
		return;

	CClientDC dc(pWnd);
	CRect rFull;
	pWnd->GetClientRect(&rFull);
	if (rFull.IsRectEmpty())
		return;

	dc.FillSolidRect(&rFull, bg);

	CFont font;
	const int pt = (rFull.Width() < 80) ? 9 : 11;
	font.CreateFont(
		-MulDiv(pt, dc.GetDeviceCaps(LOGPIXELSY), 72), 0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
	CFont* pOld = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(fg);

	CRect rBlock(rFull);
	rBlock.DeflateRect(4, 4);
	CRect rCalc(rBlock);
	dc.DrawText(tip, &rCalc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
	const int textW = rCalc.Width();
	const int textH = rCalc.Height();
	const int x0 = rBlock.left + (rBlock.Width() - textW) / 2;
	const int y0 = rBlock.top + (rBlock.Height() - textH) / 2;
	CRect rDraw(x0, y0, x0 + textW, y0 + textH);
	dc.DrawText(tip, &rDraw, DT_CENTER | DT_TOP | DT_WORDBREAK);
	dc.SelectObject(pOld);
	pWnd->ShowWindow(SW_SHOW);
	pWnd->UpdateWindow();
}

void CDtSampleDlg::PaintPreviewCellsIdle()
{
	const COLORREF bg = RGB(55, 65, 81);
	const COLORREF fg = RGB(203, 213, 225);
	for (int d = 0; d < 8; d++)
	{
		for (int v = 0; v < 4; v++)
		{
			CString tip;
			tip.Format(_T("D%d V%d\nIdle"), d, v);
			PaintPreviewCellState(d, v, tip, bg, fg);
		}
	}
}

void CDtSampleDlg::PaintPreviewCellsTestResult()
{
	const COLORREF idleBg = RGB(55, 65, 81);
	const COLORREF idleFg = RGB(203, 213, 225);
	for (int d = 0; d < 8; d++)
	{
		for (int v = 0; v < 4; v++)
		{
			const bool channelOn = (d < m_dtFunction.m_iEnumDevNum && v < m_dtFunction.m_iVcNum
				&& m_dtFunction.IsDevEnabled(d) && m_dtFunction.IsVcEnabled(d, v));

			COLORREF bg = idleBg;
			COLORREF fg = idleFg;
			CString tip;
			if (!channelOn)
				tip.Format(_T("D%d V%d\nIdle"), d, v);
			else if (m_dtFunction.m_bLightGateHasResult)
			{
				if (m_dtFunction.m_bLightGatePass[d][v])
				{
					bg = RGB(22, 163, 74);
					fg = RGB(255, 255, 255);
					tip.Format(_T("D%d V%d\nOK"), d, v);
				}
				else
				{
					bg = RGB(220, 38, 38);
					fg = RGB(255, 255, 255);
					tip.Format(_T("D%d V%d\nNG"), d, v);
				}
			}
			else
				tip.Format(_T("D%d V%d\nIdle"), d, v);

			PaintPreviewCellState(d, v, tip, bg, fg);
		}
	}
}

void CDtSampleDlg::ResetFwBurnCellOverlay()
{
	m_dtFunction.ResetFirmwareBurnUiForEnabledChannels();
	for (int d = 0; d < m_dtFunction.m_iEnumDevNum; d++)
	{
		if (!m_dtFunction.IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_dtFunction.m_iVcNum; v++)
		{
			if (!m_dtFunction.IsVcEnabled(d, v))
				continue;
			PaintPreviewCellFirmware(d, v);
		}
	}
}

void CDtSampleDlg::PaintPreviewCellFirmware(int dev, int vc)
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return;
	const int pct = m_dtFunction.GetFirmwareBurnPercent(dev, vc);
	if (pct < 0)
		return;

	CWnd* pWnd = GetDlgItem(m_uWndCtrlID[dev][vc]);
	if (pWnd == NULL || pWnd->GetSafeHwnd() == NULL)
		return;

	CClientDC dc(pWnd);
	CRect rFull;
	pWnd->GetClientRect(&rFull);
	if (rFull.IsRectEmpty())
		return;

	/* Do not show NG while burn threads still running (progress may hit 100% first). */
	const bool burnFail = (pct >= 100 && m_dtFunction.m_bFirmwareBurnHasResult
		&& !m_dtFunction.m_bFirmwareBurnInProgress
		&& !m_dtFunction.m_bFirmwareBurnPass[dev][vc]);

	COLORREF bg = RGB(30, 41, 59);
	COLORREF fg = RGB(226, 232, 240);
	COLORREF barFill = RGB(34, 211, 238);
	COLORREF barTrack = RGB(51, 65, 85);
	if (burnFail)
	{
		bg = RGB(127, 29, 29);
		fg = RGB(255, 255, 255);
		barFill = RGB(248, 113, 113);
		barTrack = RGB(69, 10, 10);
	}

	/* Full fill clears carDrawImage / burn text residue. */
	dc.FillSolidRect(&rFull, bg);
	dc.FillSolidRect(&rFull, bg);

	const int barH = max(4, rFull.Height() / 10);
	CRect rcBar(rFull.left, rFull.bottom - barH, rFull.right, rFull.bottom);
	dc.FillSolidRect(&rcBar, barTrack);
	if (pct > 0)
	{
		CRect rcFill(rcBar);
		rcFill.right = rcBar.left + MulDiv(rcBar.Width(), min(pct, 100), 100);
		if (rcFill.right > rcFill.left)
			dc.FillSolidRect(&rcFill, barFill);
	}

	CFont font;
	const int pt = (rFull.Width() < 80) ? 8 : 10;
	font.CreateFont(
		-MulDiv(pt, dc.GetDeviceCaps(LOGPIXELSY), 72), 0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
	CFont* pOld = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(fg);

	CString tip;
	if (burnFail)
		tip.Format(_T("D%d V%d\nBurn NG"), dev, vc);
	else
		tip.Format(_T("D%d V%d\nBurn %d%%"), dev, vc, min(pct, 100));

	CRect rText(rFull.left, rFull.top, rFull.right, rcBar.top - 1);
	rText.DeflateRect(2, 2);
	dc.DrawText(tip, &rText, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_EDITCONTROL);
	dc.SelectObject(pOld);
}


void CDtSampleDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{		
#if 1  //2020/12/15
		for (int i=0; i<4; i++)  //
		{
			//m_dtFunction.m_devCtrlObj[i].devCtrl.GetPmuCurrent(m_dtFunction.m_devCtrlObj[i].iCurrent);
		}
#endif	
	}
	else if (nIDEvent == TIMER_ID_FW_BURN)
	{
		KillTimer(TIMER_ID_FW_BURN);
		if (!m_bStart)
			return;
		BeginFirmwareBurnAsync();
	}
	else if (nIDEvent == TIMER_ID_FW_POWER_OFF)
	{
		KillTimer(TIMER_ID_FW_POWER_OFF);
		if (!m_bFwPowerCyclePending)
			return;
		m_bFwPowerCyclePending = FALSE;
		msgUtf8(DtZh::kFwPowerCycleRestart);
		m_dtFunction.ReadGateSpecIni();
		if (!m_dtFunction.Start())
		{
			msgUtf8(DtZh::kLogPwCycleStartFail);
			return;
		}
		m_bStart = TRUE;
		ClearFwBurnCellOverlay();
		SetTimer(0, 1000, NULL);
		int settleMs = m_dtFunction.m_specDelayMs;
		if (settleMs < 200)
			settleMs = 200;
		msgUtf8(DtZh::kFwPowerSettleWait, settleMs);
		if (m_btnStart.GetSafeHwnd())
			m_btnStart.SetWindowText(_T("Stop"));
		else if (GetDlgItem(IDC_BUTTON_START))
			GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Stop"));
		UpdatePrimaryButtonLooks();
		SetTimer(TIMER_ID_FW_POWER_SETTLE, settleMs, NULL);
	}
	else if (nIDEvent == TIMER_ID_FW_POWER_SETTLE)
	{
		KillTimer(TIMER_ID_FW_POWER_SETTLE);
		if (!m_bStart && !m_dtFunction.m_gateFirmwareBurn.verifyEnabled)
			return;
		if (!RunFirmwareBurnVerifyOrStop())
			return;
		if (m_dtFunction.m_bRunning == FALSE)
		{
			if (!m_dtFunction.Start())
			{
				msgUtf8(DtZh::kLogPwCycleStartFail);
				return;
			}
			m_bStart = TRUE;
			ClearFwBurnCellOverlay();
			SetTimer(0, 1000, NULL);
			if (m_btnStart.GetSafeHwnd())
				m_btnStart.SetWindowText(_T("Stop"));
			else if (GetDlgItem(IDC_BUTTON_START))
				GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Stop"));
			UpdatePrimaryButtonLooks();
		}
		/* DelayMs already waited before verify; go to light test immediately. */
		SetTimer(TIMER_ID_STREAM_GATE, 0, NULL);
	}
	else if (nIDEvent == TIMER_ID_STREAM_GATE)
	{
		KillTimer(TIMER_ID_STREAM_GATE);
		if (!m_bStart)
			return;
		const bool ok = m_dtFunction.RunLightGatePerChannelReport();
		if (ok)
		{
			msgUtf8(DtZh::kLogLtPass);
			SetWindowText(ZH_UTF8(kMainTitleTestOk));
		}
		else
		{
			msgUtf8(DtZh::kLogLtNg);
			SetWindowText(ZH_UTF8(kMainTitleTestNg));
		}
		/* FinalizeProductionRun (CSV + preview NG/OK) already ran inside RunLightGatePerChannelReport. */
		StopCaptureAndShowResults(FALSE);
		if (m_btnStart.GetSafeHwnd())
			m_btnStart.SetWindowText(_T("Start"));
		else if (GetDlgItem(IDC_BUTTON_START))
			GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Start"));
		UpdatePrimaryButtonLooks();
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CDtSampleDlg::InitStyledToolbarButtons()
{
	if (GetDlgItem(IDC_BUTTON_LOAD) == NULL)
		return;

	if (m_fontBtn.GetSafeHandle() == NULL)
	{
		CClientDC dc(this);
		const int h = -MulDiv(16, dc.GetDeviceCaps(LOGPIXELSY), 72);
		m_fontBtn.CreateFont(h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("MS Shell Dlg"));
	}

	VERIFY(m_btnLoad.SubclassDlgItem(IDC_BUTTON_LOAD, this));
	VERIFY(m_btnEdit.SubclassDlgItem(IDC_BUTTON_EDIT, this));
	if (GetDlgItem(IDC_BUTTON_SPEC) != NULL)
		VERIFY(m_btnSpec.SubclassDlgItem(IDC_BUTTON_SPEC, this));
	VERIFY(m_btnEnum.SubclassDlgItem(IDC_BUTTON_ENUM, this));
	if (GetDlgItem(IDC_BUTTON_CHANNEL) != NULL)
		VERIFY(m_btnChannel.SubclassDlgItem(IDC_BUTTON_CHANNEL, this));
	VERIFY(m_btnOpen.SubclassDlgItem(IDC_BUTTON_OPEN, this));
	VERIFY(m_btnStart.SubclassDlgItem(IDC_BUTTON_START, this));

	m_btnLoad.SetFont(&m_fontBtn);
	m_btnEdit.SetFont(&m_fontBtn);
	if (m_btnSpec.GetSafeHwnd() != NULL)
		m_btnSpec.SetFont(&m_fontBtn);
	m_btnEnum.SetFont(&m_fontBtn);
	if (m_btnChannel.GetSafeHwnd() != NULL)
	{
		m_btnChannel.SetFont(&m_fontBtn);
		m_btnChannel.SetWindowText(ZH_UTF8(kMainBtnChannel));
		ApplyMfcToolbarLook(m_btnChannel, RGB(255, 255, 255), RGB(51, 65, 85), RGB(15, 23, 42));
	}
	m_btnOpen.SetFont(&m_fontBtn);
	m_btnStart.SetFont(&m_fontBtn);

	ApplyMfcToolbarLook(m_btnLoad, RGB(255, 255, 255), RGB(51, 65, 85), RGB(15, 23, 42));
	ApplyMfcToolbarLook(m_btnEdit, RGB(255, 255, 255), RGB(51, 65, 85), RGB(15, 23, 42));
	if (m_btnSpec.GetSafeHwnd() != NULL)
	{
		ApplyMfcToolbarLook(m_btnSpec, RGB(255, 255, 255), RGB(51, 65, 85), RGB(15, 23, 42));
		m_btnSpec.SetWindowText(ZH_UTF8(kMainBtnSpec));
	}
	ApplyMfcToolbarLook(m_btnEnum, RGB(241, 245, 249), RGB(30, 58, 138), RGB(30, 64, 175));

	UpdatePrimaryButtonLooks();
}

void CDtSampleDlg::UpdatePrimaryButtonLooks()
{
	if (!m_btnOpen.GetSafeHwnd() || !m_btnStart.GetSafeHwnd())
		return;

	if (m_bOpen)
	{
		ApplyMfcToolbarLook(m_btnOpen, RGB(234, 88, 12), RGB(255, 255, 255), RGB(255, 237, 213));
	}
	else
	{
		ApplyMfcToolbarLook(m_btnOpen, RGB(37, 99, 235), RGB(255, 255, 255), RGB(219, 234, 254));
	}
	m_btnOpen.Invalidate();

	if (m_bStart)
	{
		ApplyMfcToolbarLook(m_btnStart, RGB(220, 38, 38), RGB(255, 255, 255), RGB(254, 226, 226));
	}
	else
	{
		ApplyMfcToolbarLook(m_btnStart, RGB(22, 163, 74), RGB(255, 255, 255), RGB(220, 252, 231));
	}
	m_btnStart.Invalidate();
}

int CDtSampleDlg::LayoutToolbar(const CRect& rcClient)
{
	const int margin = 10;
	const int gap = 10;
	const int btnH = 56;
	int btnW = 138;
	CMFCButton* const toolbarBtns[7] = {
		&m_btnLoad, &m_btnEdit, &m_btnSpec, &m_btnEnum, &m_btnChannel, &m_btnOpen, &m_btnStart
	};
	static const UINT toolbarIds[7] = {
		IDC_BUTTON_LOAD, IDC_BUTTON_EDIT, IDC_BUTTON_SPEC, IDC_BUTTON_ENUM,
		IDC_BUTTON_CHANNEL, IDC_BUTTON_OPEN, IDC_BUTTON_START
	};
	const int minIniW = 120;
	const int y = margin;
	/* Wider than Load/Edit for four Chinese chars at toolbar font size. */
	const int kSpecBtnMinW = 132;

	while (btnW > 72)
	{
		const int specW = max(btnW, kSpecBtnMinW);
		const int totalBtn = 6 * btnW + specW + 6 * gap;
		const int rowNeed = margin * 2 + totalBtn + gap + minIniW;
		if (rcClient.Width() >= rowNeed)
			break;
		btnW -= 4;
	}
	if (btnW < 72)
		btnW = 72;

	const int specBtnW = max(btnW, kSpecBtnMinW);
	const int totalBtn = 6 * btnW + specBtnW + 6 * gap;
	const int rawIni = rcClient.Width() - margin * 2 - totalBtn - gap;
	/* INI stretches to all space left of buttons (no capped 400px gap on wide/maximized windows). */
	const int iniW = max(0, rawIni);

	int x = margin;
	if (CWnd* pIni = GetDlgItem(IDC_EDIT_INI))
		pIni->MoveWindow(x, y, iniW, btnH, TRUE);
	x += iniW + gap;
	for (int k = 0; k < 7; k++)
	{
		const int w = (toolbarIds[k] == IDC_BUTTON_SPEC) ? specBtnW : btnW;
		CMFCButton* pBtn = toolbarBtns[k];
		if (pBtn != NULL && pBtn->GetSafeHwnd() != NULL)
			pBtn->MoveWindow(x, y, w, btnH, TRUE);
		else if (CWnd* p = GetDlgItem(toolbarIds[k]))
			p->MoveWindow(x, y, w, btnH, TRUE);
		x += w + gap;
	}

	return y + btnH + 18;
}

int CDtSampleDlg::ReSize() {
	CRect rcClient;
	GetClientRect(&rcClient);
	const int margin = 10;
	const int topBar = LayoutToolbar(rcClient);
	m_cyToolbarBottom = topBar;
	const int logGap = 10;
	const int cw = max(1, rcClient.Width());
	int logW = MulDiv(cw, 30, 100);
	if (logW < 220)
		logW = 220;
	if (logW > 440)
		logW = 440;
	const int minVideo = 160;
	if (cw - logW - logGap - margin * 2 < minVideo)
		logW = max(120, cw - minVideo - logGap - margin * 2);
	/* Never let log width exceed horizontal budget (avoids overlap after maximize). */
	{
		const int maxLog = cw - logGap - margin * 2 - 40;
		if (logW > maxLog)
			logW = max(40, maxLog);
	}

	if (GetDlgItem(IDC_EDIT_MSG2) && GetDlgItem(IDC_EDIT_MSG2)->GetSafeHwnd())
	{
		const int logTop = topBar;
		int logH = rcClient.Height() - logTop - margin;
		if (logH < 60)
			logH = 60;
		GetDlgItem(IDC_EDIT_MSG2)->MoveWindow(
			rcClient.right - logW - logGap, logTop, logW, logH, TRUE);
	}

	CRect rcVideo(margin, topBar,
		rcClient.right - logW - logGap - margin,
		rcClient.bottom - margin);
	if (rcVideo.Width() < 32 || rcVideo.Height() < 32)
		rcVideo.SetRect(margin, topBar, margin + max(32, rcClient.Width() - logW - logGap - 2 * margin), rcClient.bottom - margin);

	/* Always show 8 Dev x 4 VC = 32 preview cells (channel mask only affects Open/Start/draw). */
	const int nCamCnt = 32;
	for (int dev = 0; dev < 8; dev++)
	{
		for (int vc = 0; vc < 4; vc++)
		{
			UINT vid = m_uWndCtrlID[dev][vc];
			CWnd* pV = GetDlgItem(vid);
			if (pV == NULL)
				continue;
			const int layoutIdx = dev * 4 + vc;
			ChangeModuleSize(layoutIdx, nCamCnt, rcVideo, vid);
			pV->ShowWindow(SW_SHOW);
		}
	}
	return 1;
}

void CDtSampleDlg::ChangeModuleSize(int nID, int nCamCnt, CRect rect, UINT VideoWinID)
{
	if (rect.Width() < 1 || rect.Height() < 1)
		return;

	int  Temp_x;
	int Temp_y;
	int  nWndCnt_X;
	int nWndCnt_Y;

	switch (nCamCnt)
	{
	case 1:
		Temp_x = 0;
		Temp_y = 0;
		nWndCnt_X = 1;
		nWndCnt_Y = 1;
		break;
	case 2:
		Temp_x = nID;
		Temp_y = 0;
		nWndCnt_X = 2;
		nWndCnt_Y = 1;
		break;
	case 3:
	case 4:
		Temp_x = nID % 2;
		Temp_y = nID / 2;
		nWndCnt_X = 2;
		nWndCnt_Y = 2;
		break;
	case 5:
	case 6:
		Temp_x = nID % 3;
		Temp_y = nID / 3;
		nWndCnt_X = 3;
		nWndCnt_Y = 2;
		break;
	case 7:
	case 8:
		Temp_x = nID % 4;
		Temp_y = nID / 4;
		nWndCnt_X = 4;
		nWndCnt_Y = 2;
		break;
	case 9:
		Temp_x = nID % 3;
		Temp_y = nID / 3;
		nWndCnt_X = 3;
		nWndCnt_Y = 3;
		break;
	case 10:
	case 11:
	case 12:
		Temp_x = nID % 4;
		Temp_y = nID / 4;
		nWndCnt_X = 4;
		nWndCnt_Y = 3;
		break;
	case 13:
	case 14:
	case 15:
	case 16:
		Temp_x = nID % 4;
		Temp_y = nID / 4;
		nWndCnt_X = 4;
		nWndCnt_Y = 4;
		break;
	case 32:
		/* 32 channels: 8 Dev x 4 VC; layout index maps dev=idx/4, vc=idx%4 */
		Temp_x = nID % 8;
		Temp_y = nID / 8;
		nWndCnt_X = 8;
		nWndCnt_Y = 4;
		break;
	default:
		nWndCnt_X = 1;
		nWndCnt_Y = 1;
		Temp_x = 0;
		Temp_y = 0;
		break;
	}
	{
		int cellW = rect.Width() / nWndCnt_X;
		int cellH = rect.Height() / nWndCnt_Y;
		const int gap = (nCamCnt == 32) ? 5 : 3;
		int cw = cellW - gap * 2;
		int ch = cellH - gap * 2;
		if (cw < 1) cw = 1;
		if (ch < 1) ch = 1;
		CWnd* pV = GetDlgItem(VideoWinID);
		if (pV != NULL && pV->GetSafeHwnd() != NULL)
		{
			pV->MoveWindow(
				rect.left + cellW * Temp_x + gap,
				rect.top + cellH * Temp_y + gap,
				cw, ch, TRUE);
			const int idx = (int)VideoWinID - 2000;
			if (idx >= 0 && idx < 32)
			{
				const int dev = idx / 4;
				const int vc = idx % 4;
				m_dtFunction.SetVideoCellLayout(dev, vc, pV->GetSafeHwnd(),
					(unsigned short)cw, (unsigned short)ch);
			}
		}
	}
}

