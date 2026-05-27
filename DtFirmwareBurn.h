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
	/** Ms after Start (prep, no grab) before burn; I2C/power settle only. */
	int fwWarmupMs;
	/** Ms after burn (and optional power cycle) before LightTest. */
	int postBurnDelayMs;
	int i2cRateKbps;
	bool autoDetectSlave;
	/** FA132 multi-VC: true = SetMipiImageVC(vc) before flash/SensorID on that lane. */
	bool useMipiVcForBurn;
	/** FA132 dual-chip: carSetChipID before per-VC I2C (vc<split->0, vc>=split->1). */
	bool fa132DualChip;
	/** VC index >= this value uses ChipID 1 (default 2: VC2/VC3 on second chip). */
	int dualChipVcSplit;
	bool powerCycleAfter;
	/** After burn + power-cycle: read key regs (Ruibo ReadFlashCalibrationResult). */
	bool verifyEnabled;
	/** 1=verify after UnitGrab+InitPower only (FA132 cal experiment); 0=read during capture (UC930 default). */
	bool verifyBeforeGrab;
	/** Before burn: read 10 regs @0x7E80 (Ruibo ReadSensorID). */
	bool readSensorIdEnabled;
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

/** Ruibo ReadSensorID: 10 x 16-bit @0x7E80, hex string (20 chars). errorCode 2 = read fail. */
struct Sony031SensorIdResult
{
	bool success;
	int errorCode;
	TCHAR sensorIdHex[21];
	unsigned char slaveId;
};

const TCHAR* FirmwareFovTypeName(int index);
int FirmwareFovIndexFromName(LPCTSTR name);
/** Build {exe}\\{flashDataDir}\\031{Fov}.bin ; returns false if file missing. */
bool ResolveFirmwareBinPath(const GateFirmwareBurnCfg& cfg, TCHAR outPath[MAX_PATH]);

/** Set I2C rate once per Dev before parallel VC burn threads (not per thread). */
bool FirmwareBurnSetupDevI2c(int devId, const GateFirmwareBurnCfg& cfg);

/** FA132 dual-chip: 0 = first 96718 (VC < split), 1 = second. */
int FirmwareChipIdForVc(int vcId, const GateFirmwareBurnCfg& cfg);

/** FA132: carSetChipID + optional SetMipiImageVC for the given VC lane. */
bool FirmwareSelectVcLane(int devId, int vcId, const GateFirmwareBurnCfg& cfg);

/** UI thread receives WM_FW_BURN_PROGRESS (see DtCarFunction.h). */
void FirmwareBurnSetProgressTarget(HWND hwnd);
void FirmwareBurnReportProgress(int devId, int vcId, int percent);

/** Read module SensorID before flash (Ruibo ReadSensorID). */
bool Sony031ReadSensorId(
	int devId,
	int vcId,
	const GateFirmwareBurnCfg& cfg,
	unsigned char slaveHint,
	Sony031SensorIdResult* outResult);

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
