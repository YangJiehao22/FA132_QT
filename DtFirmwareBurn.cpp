#include "stdafx.h"
#include "DtFirmwareBurn.h"
#include "DtCarFunction.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"
#include "ezCarDTCCM_SDK/ezCarDTCCM.h"
#include "ezCarDTCCM_SDK/imagekit.h"

#include <vector>

static HWND g_fwProgressHwnd = NULL;

namespace {

typedef int (WINAPI* PFN_SetMipiImageVC)(UINT uVc, BOOL bEnable, BYTE byChannel, int iDevID);
typedef int (WINAPI* PFN_SetSensorI2cRateEx)(UINT uKHz, int iDevID);

static HMODULE g_hDtccm2 = NULL;
static PFN_SetMipiImageVC g_pfnSetMipiImageVC = NULL;
static PFN_SetSensorI2cRateEx g_pfnSetSensorI2cRateEx = NULL;

static bool EnsureDtccm2Procs()
{
	if (g_pfnSetMipiImageVC != NULL && g_pfnSetSensorI2cRateEx != NULL)
		return true;
	if (g_hDtccm2 == NULL)
	{
		TCHAR modPath[MAX_PATH] = {};
		GetModuleFileName(NULL, modPath, MAX_PATH);
		CString dir(modPath);
		const int slash = dir.ReverseFind(_T('\\'));
		if (slash >= 0)
			dir = dir.Left(slash + 1);
		const CString dllPath = dir + _T("dtccm2.dll");
		g_hDtccm2 = ::LoadLibrary(dllPath);
		if (g_hDtccm2 == NULL)
			g_hDtccm2 = ::LoadLibrary(_T("dtccm2.dll"));
	}
	if (g_hDtccm2 == NULL)
		return false;
	if (g_pfnSetMipiImageVC == NULL)
		g_pfnSetMipiImageVC = (PFN_SetMipiImageVC)GetProcAddress(g_hDtccm2, "SetMipiImageVC");
	if (g_pfnSetSensorI2cRateEx == NULL)
		g_pfnSetSensorI2cRateEx = (PFN_SetSensorI2cRateEx)GetProcAddress(g_hDtccm2, "SetSensorI2cRateEx");
	return (g_pfnSetMipiImageVC != NULL && g_pfnSetSensorI2cRateEx != NULL);
}


const int kSony031FlashSize = 0x80000;
const unsigned char kI2cMode = 3;

static int WriteReg(int devId, unsigned char slave, unsigned short reg, unsigned short val)
{
	return ::carWriteSensorReg(slave, reg, val, kI2cMode, devId);
}

static int ReadReg16(int devId, unsigned char slave, unsigned short reg, unsigned short* outVal)
{
	if (outVal == NULL)
		return 0;
	return ::carReadSensorReg(slave, reg, outVal, kI2cMode, devId);
}

static int WriteI2cBlock(int devId, unsigned char slave, const unsigned char* data, unsigned short size)
{
	if (data == NULL || size == 0)
		return 0;
	return ::carWriteSensorI2c(slave, 0x0000, 2,
		const_cast<unsigned char*>(data), size, devId);
}

static bool ReadBinFile(LPCTSTR path, std::vector<unsigned char>& out)
{
	out.clear();
	if (path == NULL || path[0] == 0)
		return false;
	CFile f;
	if (!f.Open(path, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
		return false;
	const ULONGLONG len = f.GetLength();
	if (len < (ULONGLONG)kSony031FlashSize)
	{
		f.Close();
		return false;
	}
	out.resize(kSony031FlashSize);
	const UINT want = (UINT)kSony031FlashSize;
	const UINT rd = f.Read(out.data(), want);
	if (rd != want)
	{
		f.Close();
		out.clear();
		return false;
	}
	f.Close();
	return true;
}

static bool ProbeFlashSlave(int devId, unsigned char slave, unsigned char* outSlave)
{
	if (outSlave == NULL || slave == 0)
		return false;
	unsigned short v = 0;
	if (ReadReg16(devId, slave, 0x7E80, &v) == 1)
	{
		*outSlave = slave;
		return true;
	}
	return false;
}

static bool DetectFlashSlave(int devId, unsigned char* outSlave)
{
	if (outSlave == NULL)
		return false;
	return ProbeFlashSlave(devId, 0x36, outSlave)
		|| ProbeFlashSlave(devId, 0x34, outSlave);
}

/** Per-VC addr from GateSpec (D#_V#). Global 0x36/0x34 scan only when hint is 0 (Ruibo). */
static bool ResolveFlashSlave(int devId, int vcId, unsigned char slaveHint,
	bool autoDetect, unsigned char* outSlave)
{
	if (outSlave == NULL)
		return false;
	(void)vcId;
	if (slaveHint != 0)
		return ProbeFlashSlave(devId, slaveHint, outSlave);
	if (autoDetect)
		return DetectFlashSlave(devId, outSlave);
	return false;
}

static bool SelectMipiVc(int devId, int vcId)
{
	if (!EnsureDtccm2Procs())
		return false;
	const int iRet = g_pfnSetMipiImageVC((UINT)vcId, TRUE, CHANNEL_A, devId);
	return (iRet == DT_ERROR_OK);
}

} // namespace

void FirmwareBurnSetProgressTarget(HWND hwnd)
{
	g_fwProgressHwnd = hwnd;
}

void FirmwareBurnReportProgress(int devId, int vcId, int percent)
{
	const HWND h = g_fwProgressHwnd;
	if (h == NULL || !::IsWindow(h))
		return;
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;
	if (devId < 0 || devId > 255 || vcId < 0 || vcId > 255)
		return;
	::PostMessage(h, WM_FW_BURN_PROGRESS, (WPARAM)percent, (LPARAM)MAKELPARAM(vcId, devId));
}

static const TCHAR* kFirmwareFovTypes[FIRMWARE_FOV_TYPE_COUNT] = {
	_T("RABOB065200"),
	_T("RABOM826200"),
	_T("RABOM826100"),
	_T("RABOM82660Flip"),
	_T("RABOM82660"),
};

const TCHAR* FirmwareFovTypeName(int index)
{
	if (index < 0 || index >= FIRMWARE_FOV_TYPE_COUNT)
		index = FIRMWARE_FOV_DEFAULT_INDEX;
	return kFirmwareFovTypes[index];
}

int FirmwareFovIndexFromName(LPCTSTR name)
{
	if (name == NULL || name[0] == 0)
		return FIRMWARE_FOV_DEFAULT_INDEX;
	for (int i = 0; i < FIRMWARE_FOV_TYPE_COUNT; i++)
	{
		if (_tcsicmp(name, kFirmwareFovTypes[i]) == 0)
			return i;
	}
	return FIRMWARE_FOV_DEFAULT_INDEX;
}

bool ResolveFirmwareBinPath(const GateFirmwareBurnCfg& cfg, TCHAR outPath[MAX_PATH])
{
	if (outPath == NULL)
		return false;
	outPath[0] = 0;

	int idx = cfg.fovTypeIndex;
	if (idx < 0 || idx >= FIRMWARE_FOV_TYPE_COUNT)
		idx = FIRMWARE_FOV_DEFAULT_INDEX;

	TCHAR exeDir[MAX_PATH] = {};
	GetModuleFileName(NULL, exeDir, MAX_PATH);
	CString dir(exeDir);
	const int slash = dir.ReverseFind(_T('\\'));
	if (slash >= 0)
		dir = dir.Left(slash + 1);

	CString sub = cfg.flashDataDir;
	sub.Trim();
	if (sub.IsEmpty())
		sub = _T("FlashData");

	CString full;
	full.Format(_T("%s%s\\031%s.bin"), (LPCTSTR)dir, (LPCTSTR)sub, FirmwareFovTypeName(idx));
	_tcsncpy_s(outPath, MAX_PATH, full, _TRUNCATE);
	return (::GetFileAttributes(full) != INVALID_FILE_ATTRIBUTES);
}

bool FirmwareBurnSetupDevI2c(int devId, const GateFirmwareBurnCfg& cfg)
{
	if (!EnsureDtccm2Procs())
		return false;
	const int rateKHz = (cfg.i2cRateKbps >= 1) ? cfg.i2cRateKbps : 1;
	const int iRet = g_pfnSetSensorI2cRateEx((UINT)rateKHz, devId);
	if (iRet != DT_ERROR_OK)
	{
		msgUtf8(DtZh::kFwI2cRateFail, devId, iRet);
		return false;
	}
	return true;
}

bool Sony031FlashProgram(
	int devId,
	int vcId,
	const GateFirmwareBurnCfg& cfg,
	unsigned char slaveHint,
	Sony031BurnResult* outResult)
{
	Sony031BurnResult local = {};
	if (outResult == NULL)
		outResult = &local;
	outResult->success = false;
	outResult->errorCode = 0;
	outResult->slaveId = 0;

	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV || vcId < 0 || vcId >= MAX_VC)
	{
		outResult->errorCode = 10;
		return false;
	}
	if (cfg.binPath[0] == 0)
	{
		msgUtf8(DtZh::kFwBinMissing);
		outResult->errorCode = 11;
		return false;
	}

	std::vector<unsigned char> bin;
	if (!ReadBinFile(cfg.binPath, bin))
	{
		CStringA pathA(cfg.binPath);
		msgUtf8(DtZh::kFwBinReadFail, pathA.GetString());
		outResult->errorCode = 12;
		return false;
	}

	if (cfg.useMipiVcForBurn)
	{
		if (!SelectMipiVc(devId, vcId))
		{
			msgUtf8(DtZh::kFwSetVcFail, devId, vcId);
			outResult->errorCode = 13;
			return false;
		}
	}

	unsigned char slave = 0;
	if (!ResolveFlashSlave(devId, vcId, slaveHint, cfg.autoDetectSlave, &slave))
	{
		msgUtf8(DtZh::kFwSlaveDetectFail, devId, vcId);
		outResult->errorCode = 3;
		return false;
	}
	outResult->slaveId = slave;

	msgUtf8(DtZh::kFwParallelVc, devId, vcId, (unsigned)slave);
	FirmwareBurnReportProgress(devId, vcId, 0);
	/* unprotect */
	if (WriteReg(devId, slave, 0x8A12, 0x08) != 1
		|| WriteReg(devId, slave, 0x8AC1, 0x00) != 1)
	{
		outResult->errorCode = 4;
		return false;
	}
	/* Enter bulk program mode (Ruibo: 3 ms between 0xf4 and 0xf7, then 3 ms before sectors). */
	if (WriteReg(devId, slave, 0xffff, 0xf4) != 1)
	{
		outResult->errorCode = 4;
		return false;
	}
	Sleep(3);
	if (WriteReg(devId, slave, 0xffff, 0xf7) != 1)
	{
		outResult->errorCode = 4;
		msgUtf8(DtZh::kFwFail, devId, vcId, outResult->errorCode);
		return false;
	}
	Sleep(3);

	unsigned int sectorAddr = 0;
	unsigned char pData[256] = {};
	for (int j = 0; j < 0x80; j++)
	{
		if (WriteReg(devId, slave, 0x8000, 0x03) != 1
			|| WriteReg(devId, slave, 0x8001, (unsigned short)((sectorAddr >> 24) & 0xff)) != 1
			|| WriteReg(devId, slave, 0x8002, (unsigned short)((sectorAddr >> 16) & 0xff)) != 1
			|| WriteReg(devId, slave, 0x8003, (unsigned short)((sectorAddr >> 8) & 0xff)) != 1
			|| WriteReg(devId, slave, 0x8004, (unsigned short)(sectorAddr & 0xff)) != 1
			|| WriteReg(devId, slave, 0x8005, 0x5a) != 1)
		{
			outResult->errorCode = 4;
			return false;
		}
		Sleep(52);

		for (int k = 0; k < 16; k++)
		{
			for (int i = 0; i < 256; i++)
				pData[i] = bin[(size_t)(4096 * j + i + k * 256)];

			if (WriteI2cBlock(devId, slave, pData, 256) != 1)
			{
				outResult->errorCode = 4;
				return false;
			}
			Sleep(3);

			if (WriteReg(devId, slave, 0x8000, 0x02) != 1
				|| WriteReg(devId, slave, 0x8001, (unsigned short)((sectorAddr >> 24) & 0xff)) != 1
				|| WriteReg(devId, slave, 0x8002, (unsigned short)((sectorAddr >> 16) & 0xff)) != 1
				|| WriteReg(devId, slave, 0x8003, (unsigned short)(((sectorAddr + 0x100 * (unsigned)k) >> 8) & 0xff)) != 1
				|| WriteReg(devId, slave, 0x8004, (unsigned short)(sectorAddr & 0xff)) != 1
				|| WriteReg(devId, slave, 0x8005, 0x5a) != 1)
			{
				outResult->errorCode = 4;
				return false;
			}
			Sleep(4);
		}
		sectorAddr += 4096;
		FirmwareBurnReportProgress(devId, vcId, (j + 1) * 100 / 128);
	}

	if (WriteReg(devId, slave, 0xffff, 0xf5) != 1)
	{
		outResult->errorCode = 4;
		return false;
	}

	FirmwareBurnReportProgress(devId, vcId, 100);
	outResult->success = true;
	outResult->errorCode = 0;
	msgUtf8(DtZh::kFwOk, devId, vcId);
	return true;
}
