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
/** Firmware prep worker finished (WPARAM=1 OK / 0 fail, LPARAM=prep generation). */
#define WM_FW_PREP_DONE (WM_USER + 5)

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
#include "DtTcpNotify.h"

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

/** Sensor temp via same lane/slave as SensorID (GateSpec [sensor_temp_i2c] formula). */
bool Sony031ReadSensorTempC(
	int devId,
	int vcId,
	const GateFirmwareBurnCfg& laneCfg,
	unsigned char slaveHint,
	const GateSensorTempI2c& tempCfg,
	double* outTempC);

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
	/** After ReloadGrabParaAfterPowerCycle: skip one main-INI reload in InitWorkCapture. */
	bool    m_skipMainGrabReloadOnce[MAX_CC16 * MAX_DEV];
	bool    m_previewDisplayInit[MAX_CC16 * MAX_DEV][MAX_VC];
	CStringA m_workShowText[MAX_CC16 * MAX_DEV];	/* per-Dev overlay text for carDrawImage */

	/* GateSpec.ini next to exe: timing + per-channel optional overrides */
	CString m_strGateSpecIniPath;
	int     m_specDelayMs;
	/** [timing] PostI2cDelayMs: after SensorID/verify I2C, before light-test fps sample (Enabled=0 only). */
	int     m_specPostI2cDelayMs;
	GateChannelLimits m_gateDefault;
	GateChannelLimits m_gatePerChannel[MAX_CC16 * MAX_DEV][MAX_VC];
	/** [sensor_temp_i2c] formula; [sensor_temp_i2c_vc] D#_V# = per-lane sensor I2C slave (temp + burn). */
	GateSensorTempI2c m_gateSensorTempI2c;
	unsigned char m_gateTempI2cAddr[MAX_CC16 * MAX_DEV][MAX_VC];
	GateBadPixelDarkCfg m_gateBadPixelDark;
	GateFirmwareBurnCfg m_gateFirmwareBurn;
	/** GateSpec.ini [tcp_notify]: TCP client to lighting station (Play after production). */
	GateTcpNotifyCfg m_gateTcpNotify;

	int LoadIni();
	int ReadDtCarIni();
	int SaveDtCarIni();
	/** Show channel enable dialog (call Enum first). */
	void ShowChannelSelectDialog(CWnd* pParent);
	/** Ms to wait after I2C (SensorID/verify) before light-test sample; 0 if burn or I2C steps off. */
	int LightTestSettleMsAfterI2c() const;
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
	void UninitWorkCapture(int devId, bool unitPower = true);
	/** carGrabRestart on enabled Devs after I2C (SensorID/verify) before light test. */
	void RestartGrabForLightTest();
	/** After PowerCycleAfter: carLoadGrabPara(grabIniAfterPowerCycle) per enabled Dev (before Start). */
	bool ReloadGrabParaAfterPowerCycle();
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
	/** Unified UI + Production_report.csv for burn/verify abort or light-test completion. */
	bool FinalizeProductionRun(int failStage);
	bool FinalizeProductionRun(int failStage, const std::vector<LightTestChannelRecord>& rows, bool allPass);
	/** Prominent log banner at Start / end of one production run (same-day log file). */
	void LogProductionRunStart();
	void LogProductionRunEnd(int failStage, bool allPass);
	/** Parallel Sony031 flash on enabled Dev/VC (same Dev: all VC threads in parallel).
	    afterStart=false (UI): prep init only, no pause/join; power-cycle stays in DtSampleDlg.
	    afterStart=true (legacy): capture threads running, PauseWorkThreads before burn. */
	bool RunFirmwareBurnParallel(bool afterStart = false);
	/** Parallel SensorID read (0x7E80×10) on enabled Dev/VC; independent of Enabled burn flag. */
	bool RunSensorIdReadParallel();
	/** Before 14-reg verify: drop GrabTab (UnitGrab), InitPower only (FA132 cal read). */
	bool PrepareForFirmwareVerify();
	/** After verify while capture was running: re-InitGrab and resume preview. */
	bool RestoreWorkCaptureAfterVerify();
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
	int m_fwBurnErrCode[MAX_CC16 * MAX_DEV][MAX_VC];
	bool m_bSensorIdHasResult;
	bool m_bSensorIdReadOk[MAX_CC16 * MAX_DEV][MAX_VC];
	TCHAR m_sensorIdHex[MAX_CC16 * MAX_DEV][MAX_VC][21];
	bool m_bSensorTempHasResult;
	bool m_hasSensorTemp[MAX_CC16 * MAX_DEV][MAX_VC];
	double m_sensorTempC[MAX_CC16 * MAX_DEV][MAX_VC];
	bool AnySensorIdReadFailed() const;
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
	CString BuildProductionOutputDir(const CTime& time) const;
	void EnsureProductionSessionDir();
	void WriteProductionReport(const std::vector<LightTestChannelRecord>& rows, bool allPass) const;
	void BuildProductionRowsFromFirmware(int failStage, std::vector<LightTestChannelRecord>& outRows, bool& outAllPass) const;
	void ApplyProductionRowsToUi(const std::vector<LightTestChannelRecord>& rows);
	static unsigned __stdcall FirmwareBurnThreadProc(void* pParam);
	static unsigned __stdcall FirmwareSensorIdThreadProc(void* pParam);
	static unsigned __stdcall FirmwareVerifyThreadProc(void* pParam);
	static unsigned __stdcall LightGateThreadProc(void* pParam);

	CString m_lightTestSessionDir;
	CString m_lightTestSessionTag;
	DWORD m_dwProductionRunStartTick;
	BOOL m_bProductionRunActive;

	DECLARE_MESSAGE_MAP()
};

