#pragma once

#include <vector>

#include "afxwin.h"
#include "Resource.h"
#include "DtFileOperate.h"
#include "I2CDebug.h"

#include "ezCarDTCCM_SDK/ezCarDTCCM.h"

#define WM_MSG	WM_USER+1

extern void msg(LPCSTR lpszFmt, ...);

#define WM_DT_CAR_DRAW (WM_USER + 2)
/** Firmware burn worker finished (WPARAM=1 OK / 0 fail, LPARAM=generation). */
#define WM_FW_BURN_DONE (WM_USER + 3)
/** Per-channel burn progress (WPARAM=percent 0..100, LPARAM=MAKELPARAM(vc, dev)). */
#define WM_FW_BURN_PROGRESS (WM_USER + 4)

/** Passed to main dialog for UI-thread carDrawImage (MFC preview HWND). */
struct DtUiDrawPack
{
	HWND hVideoWnd;
	unsigned short nImgWndW;
	unsigned short nImgWndH;
	bool bShowImg;
	bool bShowText;
	char szShowData[1024];
	int vcId;
	int devId;
};

/** Limits for light test (GateSpec.ini): sensor fps, current mA, sensor temp */
struct GateChannelLimits
{
	double minSsrFps;
	double maxSsrFps;
	double minCurrent_mA;
	double maxCurrent_mA;
	double minSensorTemp_C;
	double maxSensorTemp_C;
};

#include "DtBadPixelDetect.h"
#include "DtLightTestReport.h"
#include "DtFirmwareBurn.h"

/** I2C read + generic formula for sensor temperature (GateSpec.ini [sensor_temp_i2c]). */
struct GateSensorTempI2c
{
	bool enabled;
	unsigned char i2cAddr;
	unsigned char i2cMode;
	unsigned short regLow;
	unsigned short regHigh;
	double coeffLow;
	double coeffHigh;
	double divisor;
	double offset;
};

class DtCarFunction : public CWnd
{
	DECLARE_DYNAMIC(DtCarFunction)
public:
	DtCarFunction();
	~DtCarFunction();

	char *m_cDeviceName[MAX_CC16*MAX_DEV];		// device name from Enum()

	int m_iEnumDevNum;					// number of enumerated devices
	int m_iBoxType;
	int m_iVcNum;

	/** 1=use in Open/Start/Test; 0=skip (Qt-style DevEnable) */
	int m_iDevEnable[MAX_CC16 * MAX_DEV];
	/** 1=use in preview/test; 0=skip (Qt-style VcEnable) */
	int m_iVcEnable[MAX_CC16 * MAX_DEV][MAX_VC];

	int iWorkID;

	bool IsDevEnabled(int dev) const;
	bool IsVcEnabled(int dev, int vc) const;
	/** Enabled channel with valid visible preview size (for carDrawImage). */
	bool IsPreviewCellReady(int dev, int vc) const;
	/** True if at least one enumerated Dev has an enabled VC. */
	bool HasAnyChannelEnabled() const;
	void ResetChannelEnable();
	void ApplyChannelEnableDefaultsAfterEnum();
	void LoadChannelEnableIni();
	void SaveChannelEnableIni();

	VcData_t m_tVcData[MAX_CC16*MAX_DEV][MAX_VC];

	HANDLE	m_hThread[MAX_CC16*MAX_DEV];        // per-Dev worker thread handle
	HWND	m_hWndVideo[MAX_CC16*MAX_DEV][MAX_VC];      // preview static control HWND
	unsigned short m_vidWndW[MAX_CC16 * MAX_DEV][MAX_VC]; /* layout size from UI thread */
	unsigned short m_vidWndH[MAX_CC16 * MAX_DEV][MAX_VC];
	/** Called from ReSize on UI thread (HWND + client size for carDrawImage). */
	void SetVideoCellLayout(int dev, int vc, HWND hwnd, unsigned short w, unsigned short h);
	/** Mark preview HWND/layout valid after ReSize (UI thread). */
	bool EnsurePreviewDisplay(int dev, int vc);
	void ResetPreviewDisplay();
	void InitAllPreviewDisplays();

	/* DtCar.ini path next to exe */
	CString m_strDtCarIniPath;
	/* sensor grab parameter INI path */
	CString m_strSensorIniPath;

	BOOL    m_bRunning;				/* WorkProc loop flag */
	bool    m_bSuppressWorkDraw;	/* Stop requested: no SendMessage draw, GrabHold ASAP */
	bool    m_bPauseCaptureForBurn;	/* pause grab for firmware burn; keep power/grab init */
	bool    m_workPowerReady[MAX_CC16 * MAX_DEV];
	bool    m_workGrabReady[MAX_CC16 * MAX_DEV];
	bool    m_previewDisplayInit[MAX_CC16 * MAX_DEV][MAX_VC];
	CStringA m_workShowText[MAX_CC16 * MAX_DEV];	/* per-Dev overlay text for carDrawImage */

	/* GateSpec.ini next to exe: timing + per-channel optional overrides */
	CString m_strGateSpecIniPath;
	int     m_specDelayMs;
	GateChannelLimits m_gateDefault;
	GateChannelLimits m_gatePerChannel[MAX_CC16 * MAX_DEV][MAX_VC];
	/** Formula from [sensor_temp_i2c]; per Dev/VC I2C addr from [sensor_temp_i2c_vc] D#_V#. */
	GateSensorTempI2c m_gateSensorTempI2c;
	unsigned char m_gateTempI2cAddr[MAX_CC16 * MAX_DEV][MAX_VC];
	GateBadPixelDarkCfg m_gateBadPixelDark;
	GateFirmwareBurnCfg m_gateFirmwareBurn;

	int LoadIni();
	int ReadDtCarIni();
	int SaveDtCarIni();
	/** Show channel enable dialog (call Enum first). */
	void ShowChannelSelectDialog(CWnd* pParent);
	/** Reload GateSpec.ini (timing + limits) from disk */
	int ReadGateSpecIni();
	/** Write current timing + limits + per-channel overrides to GateSpec.ini */
	/** @return 1 on success, 0 on failure */
	int SaveGateSpecIni();

	void ShowI2cDebug(int uCurSel);

	/** Load SDK DLLs from exe dir; create DtccmKit.dll alias on x64. Call before Enum/Open. */
	void PreloadEzCarSdkDlls();

	int Enum();
	int Open();
	int Close();
	int Start();
	/** Firmware prod: init power/grab only, no WorkProc / no carGrabFrameDirect. */
	int StartFirmwarePrep();
	/** GrabHold + suppress preview (call before Stop join; safe to call twice). */
	void RequestStopCapture();
	int Stop();

	/** UI-thread capture init (carInitPower + carInitGrab). */
	bool InitWorkCapture(int devId);
	void UninitWorkCapture(int devId);
	/** carDrawImage on UI thread (required for MFC CStatic preview). */
	int DrawImageOnUiThread(const DrawImage_t& di, int vcId, int devId);

	static const int kFwBurnPctInactive = -1;
	void SetFirmwareBurnOverlayActive(bool active) { m_bFwBurnOverlay = active; }
	bool IsFirmwareBurnOverlayActive() const { return m_bFwBurnOverlay; }
	bool IsFirmwareBurnInProgress() const { return m_bFirmwareBurnInProgress; }
	bool IsFirmwareBurnCellActive(int dev, int vc) const;
	int GetFirmwareBurnPercent(int dev, int vc) const;
	void SetFirmwareBurnPercent(int dev, int vc, int pct);
	void ClearFirmwareBurnUiState();
	void ResetFirmwareBurnUiForEnabledChannels();

	/** Parallel light test per enabled Dev/VC (SsrFps/Cur/Temp/BadPx/Fw). */
	bool RunLightGatePerChannelReport();
	/** Parallel Sony031 flash on enabled Dev/VC (same Dev: all VC threads in parallel).
	    afterStart=false (UI): prep init only, no pause/join; power-cycle stays in DtSampleDlg.
	    afterStart=true (legacy): capture threads running, PauseWorkThreads before burn. */
	bool RunFirmwareBurnParallel(bool afterStart = false);
	/** Parallel post-burn register verify on enabled Dev/VC (same pattern as burn). */
	bool RunFirmwareBurnVerifyAll();
	void SetFirmwareBurnProgressWnd(HWND hwnd) { m_hwndFirmwareBurnProgress = hwnd; }
	/** Clear saved gate UI state (call when user starts a new capture session) */
	void ClearLightGateResults();

	/* Last light-test result for Stop screen (per Dev/VC cell) */
	bool m_bLightGateHasResult;
	bool m_bLightGatePass[MAX_CC16 * MAX_DEV][MAX_VC];
	bool m_bFirmwareBurnHasResult;
	bool m_bFirmwareBurnInProgress;
	bool m_bFirmwareBurnPass[MAX_CC16 * MAX_DEV][MAX_VC];
	bool m_bFirmwareBurnVerifyHasResult;
	bool m_bFirmwareBurnVerifyPass[MAX_CC16 * MAX_DEV][MAX_VC];
	bool PauseWorkThreadsForFirmwareBurn();

	/** Grab one frame and convert to 8-bit gray (thread-safe with WorkProc). */
	bool GrabFrameGray8(int devId, int vcId, std::vector<unsigned char>& gray, unsigned int& outW, unsigned int& outH);
	bool RunDarkFieldBadPixelCheck(int devId, int vcId, BadPixelDarkResult* outResult,
		CString* outBmpPath = NULL, CString* outRawUnpackedPath = NULL);

	void WorkProc(int iDevID);

protected:
	struct VcGrayCache
	{
		std::vector<unsigned char> pixels;
		std::vector<unsigned char> rawPixels;
		unsigned int width;
		unsigned int height;
		IMAGE_FORMAT rawFormat;
		RAW_FORMAT rawFmt;
		YUV_FORMAT yuvFmt;
		unsigned int rawDataSize;
		unsigned int rawRowStride;
		bool bayerRaster;
		bool valid;
		volatile LONG pending;
		VcGrayCache() : width(0), height(0), rawFormat(FORMAT_RAW8), rawFmt(RAW_RGGB),
			yuvFmt(YUV_YCBYCR), rawDataSize(0), rawRowStride(0), bayerRaster(false), valid(false), pending(0) {}
	};

	CRITICAL_SECTION m_csGrab;
	CRITICAL_SECTION m_csBurnDev[MAX_CC16 * MAX_DEV];
	HWND m_hwndFirmwareBurnProgress;
	bool m_bFwBurnOverlay;
	int m_fwBurnPct[MAX_CC16 * MAX_DEV][MAX_VC];
	bool m_grabTabValid[MAX_CC16 * MAX_DEV];
	GrabTab m_grabTab[MAX_CC16 * MAX_DEV];
	VcGrayCache m_grayCache[MAX_CC16 * MAX_DEV][MAX_VC];

	void UpdateGrayCacheFromGrab(int devId, int vcId, const DtImage_t& grabImg);
	/** carGrabFrameDirect + copy raw (SDK reuses buffer on next grab). */
	bool GrabFrameDirectOwned(int devId, int vcId,
		std::vector<unsigned char>& gray, unsigned int& outW, unsigned int& outH,
		std::vector<unsigned char>& rawCopy, IMAGE_FORMAT& imgFormat,
		RAW_FORMAT& rawFmt, YUV_FORMAT& yuvFmt, bool& bayerRaster);
	void SaveBadPixelSnapshots(int devId, int vcId, bool pass,
		const std::vector<unsigned char>& gray, unsigned int w, unsigned int h,
		CString* outBmpPath, CString* outRawUnpackedPath,
		const std::vector<unsigned char>* frameRaw = NULL, IMAGE_FORMAT frameFmt = FORMAT_RAW8,
		RAW_FORMAT frameRawFmt = RAW_RGGB, YUV_FORMAT frameYuvFmt = YUV_YCBYCR);
	CString BuildLightTestOutputDir(const CTime& time) const;
	void WriteLightTestReport(const std::vector<LightTestChannelRecord>& rows, bool allPass) const;
	static unsigned __stdcall FirmwareBurnThreadProc(void* pParam);
	static unsigned __stdcall FirmwareVerifyThreadProc(void* pParam);
	static unsigned __stdcall LightGateThreadProc(void* pParam);

	CString m_lightTestSessionDir;
	CString m_lightTestSessionTag;

	DECLARE_MESSAGE_MAP()
};

