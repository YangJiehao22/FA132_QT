#pragma once

#include "ezCarDTCCM_SDK/ezCarDtccmDef.h"

/** Production FOV types (Ruibo RABO* only; bin: FlashData\\031{Name}.bin). */
#define FIRMWARE_FOV_TYPE_COUNT 5
#define FIRMWARE_FOV_DEFAULT_INDEX 0

/** GateSpec.ini [firmware_burn] */
struct GateFirmwareBurnCfg
{
	bool enabled;
	/** Index 0..4 into FirmwareFovTypeName() */
	int fovTypeIndex;
	/** Relative to exe, default FlashData -> exe\\FlashData\\031{Fov}.bin */
	TCHAR flashDataDir[MAX_PATH];
	/** Ms after Start() before burn (chip must be running). */
	int fwWarmupMs;
	/** Ms after burn (and optional power cycle) before LightTest. */
	int postBurnDelayMs;
	int i2cRateKbps;
	bool autoDetectSlave;
	/** FA132: burn by per-VC I2C addr (Dothinkey); false = call SetMipiImageVC before burn. */
	bool useMipiVcForBurn;
	bool powerCycleAfter;
	/** After burn + power-cycle: read key regs (Ruibo ReadFlashCalibrationResult). */
	bool verifyEnabled;
	/** Resolved full path (runtime). */
	TCHAR binPath[MAX_PATH];
};

struct Sony031VerifyResult
{
	bool success;
	int failIndex;
	unsigned short expected;
	unsigned short actual;
	unsigned short values[14];
};

struct Sony031BurnResult
{
	bool success;
	int errorCode;
	unsigned char slaveId;
};

const TCHAR* FirmwareFovTypeName(int index);
int FirmwareFovIndexFromName(LPCTSTR name);
/** Build {exe}\\{flashDataDir}\\031{Fov}.bin ; returns false if file missing. */
bool ResolveFirmwareBinPath(const GateFirmwareBurnCfg& cfg, TCHAR outPath[MAX_PATH]);

/** Set I2C rate once per Dev before parallel VC burn threads (not per thread). */
bool FirmwareBurnSetupDevI2c(int devId, const GateFirmwareBurnCfg& cfg);

/** UI thread receives WM_FW_BURN_PROGRESS (see DtCarFunction.h). */
void FirmwareBurnSetProgressTarget(HWND hwnd);
void FirmwareBurnReportProgress(int devId, int vcId, int percent);

/** Sony ISX031 512KB flash program (Ruibo Sony031WriteFlash core). */
bool Sony031FlashProgram(
	int devId,
	int vcId,
	const GateFirmwareBurnCfg& cfg,
	unsigned char slaveHint,
	Sony031BurnResult* outResult);

/** Post-burn register check (Ruibo ReadFlashCalibrationResult), per FovTypeIndex. */
bool Sony031VerifyFlashCalibration(
	int devId,
	int vcId,
	int fovTypeIndex,
	const GateFirmwareBurnCfg& cfg,
	unsigned char slaveHint,
	Sony031VerifyResult* outResult);
