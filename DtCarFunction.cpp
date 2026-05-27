#include "StdAfx.h"
#include <process.h>
#include "DtCarFunction.h"
#include "DtChannelDlg.h"
#include "DtLightTestReport.h"
#include "DtHotPixelAlgo.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"
#include "ezCarDTCCM_SDK/DtccmKit.h"

#include <math.h>
#include <ShlObj.h>
#include <Vfw.h>
#pragma comment (lib, "vfw32.lib")
#pragma comment (lib, "shell32.lib")

#ifdef _M_X64
#pragma comment (lib, ".\\ezCarDTCCM_SDK\\X64_Lib\\ezDtCarDTCCM64.lib")
#else
#pragma comment (lib, ".\\ezCarDTCCM_SDK\\X86_Lib\\ezDtCarDTCCM.lib")
#endif

IMPLEMENT_DYNAMIC(DtCarFunction, CWnd)

static double GateIniDbl(LPCTSTR path, LPCTSTR section, LPCTSTR key, double defVal)
{
	CString s = GetIniFileString(section, key, _T(""), path);
	if (s.IsEmpty())
		return defVal;
	return atof(CStringA(s).GetString());
}

static int GateIniInt(LPCTSTR path, LPCTSTR section, LPCTSTR key, int defVal)
{
	return GetIniFileInt(section, key, defVal, path);
}

static unsigned GateIniUint(LPCTSTR path, LPCTSTR section, LPCTSTR key, unsigned defVal)
{
	CString s = GetIniFileString(section, key, _T(""), path);
	s.Trim();
	if (s.IsEmpty())
		return defVal;
	if (s.Left(2).CompareNoCase(_T("0x")) == 0)
	{
		unsigned v = 0;
		_stscanf_s(s.Mid(2), _T("%x"), &v);
		return v;
	}
	return (unsigned)_tstoi((LPCTSTR)s);
}

static void GateWriteHex(LPCTSTR path, LPCTSTR sec, LPCTSTR key, unsigned v, int nibbles)
{
	CString s;
	if (nibbles <= 2)
		s.Format(_T("0x%02X"), v & 0xFF);
	else
		s.Format(_T("0x%04X"), v & 0xFFFF);
	WriteIniFileString(sec, key, s, path);
}

static GateBadPixelDarkCfg GateDefaultBadPixelDark()
{
	GateBadPixelDarkCfg c = {};
	c.enabled = false;
	c.algoMode = 1;
	c.maxBadPixels = 0;
	c.hotDelta = 30;
	c.hotAbsMin = 25;
	c.borderPx = 2;
	c.brightContrastCluster = 30;
	c.clusterMinPixels = 2;
	c.singleDefectPermyriad = 1;
	c.grGbToG = true;
	c.bayerPattern = -1;
	c.saveSnapshot = true;
	c.saveBmp = true;
	c.savePackedRaw = true;
	c.saveUnpack12 = true;
	c.saveUnpack10 = true;
	c.saveDir[0] = 0;
	return c;
}

static GateFirmwareBurnCfg GateDefaultFirmwareBurn()
{
	GateFirmwareBurnCfg c = {};
	c.enabled = false;
	c.fovTypeIndex = FIRMWARE_FOV_DEFAULT_INDEX;
	_tcsncpy_s(c.flashDataDir, _T("FlashData"), _TRUNCATE);
	c.fwWarmupMs = 3000;
	c.postBurnDelayMs = 1000;
	c.binPath[0] = 0;
	c.i2cRateKbps = 800;
	c.autoDetectSlave = true;
	c.useMipiVcForBurn = true;
	c.fa132DualChip = true;
	c.dualChipVcSplit = 2;
	c.powerCycleAfter = true;
	c.verifyEnabled = true;
	c.verifyBeforeGrab = false;
	c.readSensorIdEnabled = true;
	c.grabIniAfterPowerCycle[0] = 0;
	return c;
}

static void GateIniFillFirmwareBurn(LPCTSTR path, const GateFirmwareBurnCfg& fb, GateFirmwareBurnCfg* out)
{
	const TCHAR* sec = _T("firmware_burn");
	out->enabled = (GateIniInt(path, sec, _T("Enabled"), fb.enabled ? 1 : 0) != 0);
	out->fovTypeIndex = GateIniInt(path, sec, _T("FovTypeIndex"), fb.fovTypeIndex);
	if (out->fovTypeIndex < 0 || out->fovTypeIndex >= FIRMWARE_FOV_TYPE_COUNT)
		out->fovTypeIndex = FIRMWARE_FOV_DEFAULT_INDEX;
	{
		CString fovName = GetIniFileString(sec, _T("FovType"), _T(""), path);
		fovName.Trim();
		if (!fovName.IsEmpty())
			out->fovTypeIndex = FirmwareFovIndexFromName(fovName);
	}
	{
		CString dir = GetIniFileString(sec, _T("FlashDataDir"), fb.flashDataDir, path);
		dir.Trim();
		if (dir.IsEmpty())
			_tcsncpy_s(out->flashDataDir, fb.flashDataDir, _TRUNCATE);
		else
			_tcsncpy_s(out->flashDataDir, dir, _TRUNCATE);
	}
	out->fwWarmupMs = GateIniInt(path, sec, _T("FwWarmupMs"), fb.fwWarmupMs);
	if (out->fwWarmupMs < 500)
		out->fwWarmupMs = 3000;
	out->postBurnDelayMs = GateIniInt(path, sec, _T("PostBurnDelayMs"), fb.postBurnDelayMs);
	if (out->postBurnDelayMs < 0)
		out->postBurnDelayMs = 1000;
	out->i2cRateKbps = GateIniInt(path, sec, _T("I2cRateKbps"), fb.i2cRateKbps);
	if (out->i2cRateKbps < 100)
		out->i2cRateKbps = 800;
	out->autoDetectSlave = (GateIniInt(path, sec, _T("AutoDetectSlave"), fb.autoDetectSlave ? 1 : 0) != 0);
	out->useMipiVcForBurn = (GateIniInt(path, sec, _T("UseMipiVcForBurn"), fb.useMipiVcForBurn ? 1 : 0) != 0);
	out->fa132DualChip = (GateIniInt(path, sec, _T("Fa132DualChip"), fb.fa132DualChip ? 1 : 0) != 0);
	out->dualChipVcSplit = GateIniInt(path, sec, _T("DualChipVcSplit"), fb.dualChipVcSplit);
	if (out->dualChipVcSplit < 1 || out->dualChipVcSplit > MAX_VC)
		out->dualChipVcSplit = 2;
	out->powerCycleAfter = (GateIniInt(path, sec, _T("PowerCycleAfter"), fb.powerCycleAfter ? 1 : 0) != 0);
	out->verifyEnabled = (GateIniInt(path, sec, _T("VerifyEnabled"), fb.verifyEnabled ? 1 : 0) != 0);
	out->verifyBeforeGrab = (GateIniInt(path, sec, _T("VerifyBeforeGrab"), fb.verifyBeforeGrab ? 1 : 0) != 0);
	out->readSensorIdEnabled = (GateIniInt(path, sec, _T("ReadSensorIdEnabled"), fb.readSensorIdEnabled ? 1 : 0) != 0);
	{
		CString grabIni = GetIniFileString(sec, _T("GrabIniAfterPowerCycle"), _T(""), path);
		grabIni.Trim();
		if (grabIni.IsEmpty())
			out->grabIniAfterPowerCycle[0] = 0;
		else
			_tcsncpy_s(out->grabIniAfterPowerCycle, grabIni, _TRUNCATE);
	}
	out->binPath[0] = 0;
	ResolveFirmwareBinPath(*out, out->binPath);
}

static void GateIniFillBadPixelDark(LPCTSTR path, const GateBadPixelDarkCfg& fb, GateBadPixelDarkCfg* out)
{
	const TCHAR* sec = _T("bad_pixel_dark");
	out->enabled = (GateIniInt(path, sec, _T("Enabled"), fb.enabled ? 1 : 0) != 0);
	out->algoMode = GateIniInt(path, sec, _T("AlgoMode"), fb.algoMode);
	out->maxBadPixels = GateIniInt(path, sec, _T("MaxBadPixels"), fb.maxBadPixels);
	out->hotDelta = GateIniInt(path, sec, _T("HotDelta"), fb.hotDelta);
	out->hotAbsMin = GateIniInt(path, sec, _T("HotAbsMin"), fb.hotAbsMin);
	out->borderPx = GateIniInt(path, sec, _T("BorderPx"), fb.borderPx);
	out->brightContrastCluster = GateIniInt(path, sec, _T("BrightContrastCluster"), fb.brightContrastCluster);
	out->clusterMinPixels = GateIniInt(path, sec, _T("ClusterMinPixels"), fb.clusterMinPixels);
	out->singleDefectPermyriad = GateIniInt(path, sec, _T("SingleDefectPermyriad"), fb.singleDefectPermyriad);
	out->grGbToG = (GateIniInt(path, sec, _T("GrGbToG"), fb.grGbToG ? 1 : 0) != 0);
	out->bayerPattern = GateIniInt(path, sec, _T("BayerPattern"), fb.bayerPattern);
	{
		int saveSnap = GateIniInt(path, sec, _T("SaveSnapshot"), -1);
		if (saveSnap < 0)
			saveSnap = GateIniInt(path, sec, _T("SaveSnapshotOnFail"), fb.saveSnapshot ? 1 : 0);
		out->saveSnapshot = (saveSnap != 0);
	}
	out->saveBmp = (GateIniInt(path, sec, _T("SaveBmp"), fb.saveBmp ? 1 : 0) != 0);
	out->savePackedRaw = (GateIniInt(path, sec, _T("SavePackedRaw"), fb.savePackedRaw ? 1 : 0) != 0);
	out->saveUnpack12 = (GateIniInt(path, sec, _T("SaveUnpack12"), fb.saveUnpack12 ? 1 : 0) != 0);
	out->saveUnpack10 = (GateIniInt(path, sec, _T("SaveUnpack10"), fb.saveUnpack10 ? 1 : 0) != 0);
	{
		CString dir = GetIniFileString(sec, _T("SaveDir"), _T(""), path);
		dir.Trim();
		_tcsncpy_s(out->saveDir, dir.GetString(), _TRUNCATE);
	}
}

static const unsigned char kGateDefaultTempI2cAddr[MAX_VC] = { 0x34, 0x84, 0x86, 0x88 };

/** GateSpec.ini: FA132 UI only Dev0..7 x VC0..3 (not 32 slots). */
static const int kGateSpecIniDevSlots = MAX_DEV;

static GateSensorTempI2c GateDefaultSensorTempI2c()
{
	GateSensorTempI2c c = {};
	c.enabled = true;
	c.i2cAddr = kGateDefaultTempI2cAddr[0];
	c.i2cMode = 3;
	c.regLow = 0x1F40;
	c.regHigh = 0x1F41;
	c.coeffLow = 1.0;
	c.coeffHigh = 256.0;
	c.divisor = 16.0;
	c.offset = -50.0;
	return c;
}

static void GateIniFillTempI2cAddrGrid(LPCTSTR path, unsigned char addr[][MAX_VC])
{
	const TCHAR* sec = _T("sensor_temp_i2c_vc");
	for (int d = 0; d < kGateSpecIniDevSlots; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			CString key;
			key.Format(_T("D%d_V%d"), d, v);
			addr[d][v] = (unsigned char)GateIniUint(path, sec, key, kGateDefaultTempI2cAddr[v]);
		}
	}
}

static void GateInitDefaultTempI2cAddrGrid(unsigned char addr[][MAX_VC])
{
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
			addr[d][v] = kGateDefaultTempI2cAddr[v];
	}
}

static void GateIniFillSensorTempI2c(LPCTSTR path, LPCTSTR section, const GateSensorTempI2c& fb, GateSensorTempI2c* out)
{
	const TCHAR* sec = section;
	out->enabled = (GateIniInt(path, sec, _T("Enabled"), fb.enabled ? 1 : 0) != 0);
	out->i2cAddr = (unsigned char)GateIniUint(path, sec, _T("I2cAddr"), fb.i2cAddr);
	out->i2cMode = (unsigned char)GateIniUint(path, sec, _T("I2cMode"), fb.i2cMode);
	out->regLow = (unsigned short)GateIniUint(path, sec, _T("RegLow"), fb.regLow);
	out->regHigh = (unsigned short)GateIniUint(path, sec, _T("RegHigh"), fb.regHigh);
	out->coeffLow = GateIniDbl(path, sec, _T("CoeffLow"), fb.coeffLow);
	out->coeffHigh = GateIniDbl(path, sec, _T("CoeffHigh"), fb.coeffHigh);
	out->divisor = GateIniDbl(path, sec, _T("Divisor"), fb.divisor);
	out->offset = GateIniDbl(path, sec, _T("Offset"), fb.offset);
}

static void GateIniFillLimits(LPCTSTR path, LPCTSTR section, const GateChannelLimits& fb, GateChannelLimits* out)
{
	out->minSsrFps = GateIniDbl(path, section, _T("MinSsrFps"), fb.minSsrFps);
	out->maxSsrFps = GateIniDbl(path, section, _T("MaxSsrFps"), fb.maxSsrFps);
	out->minCurrent_mA = GateIniDbl(path, section, _T("MinCurrent_mA"), fb.minCurrent_mA);
	out->maxCurrent_mA = GateIniDbl(path, section, _T("MaxCurrent_mA"), fb.maxCurrent_mA);
	out->minSensorTemp_C = GateIniDbl(path, section, _T("MinSensorTemp_C"), fb.minSensorTemp_C);
	out->maxSensorTemp_C = GateIniDbl(path, section, _T("MaxSensorTemp_C"), fb.maxSensorTemp_C);
}

/** TempC = (b0*CoeffLow + b1*CoeffHigh) / Divisor + Offset; RegHigh=0 uses one byte only. */
static bool ReadSensorTempC(int devId, int vcId, const GateSensorTempI2c& cfg,
	const GateFirmwareBurnCfg& fwLane, double* outTempC)
{
	if (outTempC == NULL || !cfg.enabled)
		return false;
	if (cfg.divisor == 0.0)
		return false;

	if (!FirmwareSelectVcLane(devId, vcId, fwLane))
		return false;

	USHORT vLo = 0;
	USHORT vHi = 0;
	if (::carReadSensorReg(cfg.i2cAddr, cfg.regLow, &vLo, cfg.i2cMode, devId) != DT_ERROR_OK)
		return false;

	double raw = (double)(vLo & 0xFF) * cfg.coeffLow;
	if (cfg.regHigh != 0)
	{
		if (::carReadSensorReg(cfg.i2cAddr, cfg.regHigh, &vHi, cfg.i2cMode, devId) != DT_ERROR_OK)
			return false;
		raw += (double)(vHi & 0xFF) * cfg.coeffHigh;
	}

	*outTempC = raw / cfg.divisor + cfg.offset;
	return true;
}

namespace {

static bool IsPackedGrayFormat(IMAGE_FORMAT fmt)
{
	return fmt == FORMAT_RAW8 || fmt == FORMAT_GRAY8 || fmt == FORMAT_G8;
}

static bool IsRawBayerFormat(IMAGE_FORMAT fmt)
{
	switch (fmt)
	{
	case FORMAT_RAW10:
	case FORMAT_RAW8:
	case FORMAT_RAW16:
	case FORMAT_MIPI_RAW10:
	case FORMAT_MIPI_RAW12:
	case FORMAT_MIPI_RAW14:
	case FORMAT_MIPI_RAW20:
	case FORMAT_MIPI_RAW24:
	case FORMAT_P10:
	case FORMAT_P12:
	case FORMAT_P14:
	case FORMAT_RAW24:
		return true;
	default:
		return false;
	}
}

static bool IsYuvFormat(IMAGE_FORMAT fmt)
{
	switch (fmt)
	{
	case FORMAT_YUV:
	case FORMAT_YUV_SPI:
	case FORMAT_YUV_MTK_S:
	case FORMAT_YUV_10:
	case FORMAT_YUV_12:
		return true;
	default:
		return false;
	}
}

static bool IsYuv422Packed8(IMAGE_FORMAT fmt)
{
	return fmt == FORMAT_YUV || fmt == FORMAT_YUV_SPI || fmt == FORMAT_YUV_MTK_S;
}

static unsigned GrabSrcBytes(const DtImage_t& img)
{
	if (img.dataSize > 0)
		return img.dataSize;
	const unsigned w = img.width;
	const unsigned h = img.height;
	if (w < 1 || h < 1)
		return 0;
	switch (img.format)
	{
	case FORMAT_RAW8:
	case FORMAT_GRAY8:
	case FORMAT_G8:
		return w * h;
	case FORMAT_MIPI_RAW10:
	case FORMAT_P10:
		return (w * h * 5 + 3) / 4;
	case FORMAT_MIPI_RAW12:
	case FORMAT_P12:
		return (w * h * 3 + 1) / 2;
	case FORMAT_MIPI_RAW14:
	case FORMAT_P14:
		return (w * h * 7 + 3) / 4;
	case FORMAT_RAW10:
	case FORMAT_RAW16:
		return w * h * 2;
	case FORMAT_YUV:
	case FORMAT_YUV_SPI:
	case FORMAT_YUV_MTK_S:
		return w * h * 2;
	case FORMAT_YUV_10:
		return (w * h * 5 + 3) / 4;
	case FORMAT_YUV_12:
		return (w * h * 3 + 1) / 2;
	default:
		return w * h * 2;
	}
}

/** SDK dataSize when set; else formula size (bytes in grabImg.data). */
static unsigned GrabPayloadBytes(const DtImage_t& img)
{
	if (img.dataSize > 0)
		return img.dataSize;
	return GrabSrcBytes(img);
}

static unsigned GrabRowStrideBytes(const DtImage_t& img)
{
	const unsigned h = img.height;
	if (h < 1)
		return 0;
	const unsigned payload = GrabPayloadBytes(img);
	if (payload >= h)
		return payload / h;
	return GrabSrcBytes(img) / h;
}

static void FillSrcMetaFromGrabTab(DtImage_t& img, const GrabTab* pTab)
{
	if (pTab == NULL)
		return;
	if (IsRawBayerFormat(img.format))
	{
		if (img.rawFmt < RAW_RGGB || img.rawFmt > RAW_BGGR)
			img.rawFmt = static_cast<RAW_FORMAT>(pTab->sensor.outformat % 4);
	}
	else if (img.format == FORMAT_YUV || img.format == FORMAT_YUV_SPI
		|| img.format == FORMAT_YUV_10 || img.format == FORMAT_YUV_12
		|| img.format == FORMAT_YUV_MTK_S)
		img.yuvFmt = static_cast<YUV_FORMAT>(pTab->sensor.outformat % 4);
}

/** Tight row bytes for MIPI RAW12 (3 bytes carry 2 pixels). */
static unsigned MipiRaw12RowBytes(unsigned int w)
{
	return (w * 3 + 1) / 2;
}

/** Dothinkey/Hisi packed RAW12: b0=P0[11:4], b1=P1[11:4], b2 carries both low nibbles. */
static inline void UnpackMipiRaw12PairDt(unsigned b0, unsigned b1, unsigned b2,
	unsigned short* p0, unsigned short* p1)
{
	*p0 = (unsigned short)((b0 << 4) | (b2 & 0x0F));
	*p1 = (unsigned short)((b1 << 4) | (b2 >> 4));
}

/** CSI-2 wire RAW12: b0=P0[11:4], b1 shared, b2=P1[7:0]. */
static inline void UnpackMipiRaw12PairCsi(unsigned b0, unsigned b1, unsigned b2,
	unsigned short* p0, unsigned short* p1)
{
	*p0 = (unsigned short)((b0 << 4) | (b1 >> 4));
	*p1 = (unsigned short)(((b1 & 0x0F) << 8) | b2);
}

/** Even/odd column mean gap; natural Bayer should be much lower than wrong CSI unpack on Dt buffer. */
static double Mipi12ColumnImbalanceScore(const unsigned short* p16, unsigned int w, unsigned int h)
{
	if (p16 == NULL || w < 4 || h < 4)
		return 1e9;
	const unsigned int rows = (h < 64) ? h : 64;
	unsigned long long sumEven = 0;
	unsigned long long sumOdd = 0;
	unsigned int cntEven = 0;
	unsigned int cntOdd = 0;
	for (unsigned int y = 0; y < rows; y++)
	{
		const unsigned int row = y * w;
		for (unsigned int x = 0; x < w; x++)
		{
			const unsigned int v = p16[row + x];
			if ((x & 1) != 0)
			{
				sumOdd += v;
				cntOdd++;
			}
			else
			{
				sumEven += v;
				cntEven++;
			}
		}
	}
	if (cntEven < 1 || cntOdd < 1)
		return 1e9;
	const double meanEven = (double)sumEven / (double)cntEven;
	const double meanOdd = (double)sumOdd / (double)cntOdd;
	double d = meanEven - meanOdd;
	if (d < 0)
		d = -d;
	return d;
}

static bool UnpackMipiRaw12ToP12WithPairFn(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	std::vector<unsigned char>& out16,
	void (*unpackPair)(unsigned, unsigned, unsigned, unsigned short*, unsigned short*))
{
	const unsigned int pixels = w * h;
	if (src == NULL || pixels < 2 || w < 2 || unpackPair == NULL)
		return false;

	const unsigned rowBytes = MipiRaw12RowBytes(w);
	const unsigned tightStride = rowBytes;
	if (rowStride < tightStride)
		rowStride = tightStride;
	const unsigned needSrc = rowStride * h;
	const unsigned useBytes = (srcBytes > 0 && srcBytes < needSrc) ? srcBytes : needSrc;
	out16.resize(pixels * 2);

	unsigned int dst = 0;
	for (unsigned int y = 0; y < h; y++)
	{
		const unsigned char* row = src + y * rowStride;
		if ((y + 1) * rowStride > useBytes)
			return false;
		for (unsigned int x = 0; x + 1 < w; x += 2)
		{
			const unsigned si = (x * 3) / 2;
			if (si + 2 >= rowBytes)
				return false;
			unsigned short p0 = 0, p1 = 0;
			unpackPair(row[si], row[si + 1], row[si + 2], &p0, &p1);
			out16[dst * 2] = (unsigned char)(p0 & 0xFF);
			out16[dst * 2 + 1] = (unsigned char)((p0 >> 8) & 0xFF);
			out16[(dst + 1) * 2] = (unsigned char)(p1 & 0xFF);
			out16[(dst + 1) * 2 + 1] = (unsigned char)((p1 >> 8) & 0xFF);
			dst += 2;
		}
		if (w & 1)
		{
			const unsigned si = ((w - 1) * 3) / 2;
			if (si + 1 >= rowBytes)
				return false;
			unsigned short p0 = 0, p1 = 0;
			unpackPair(row[si], row[si + 1], 0, &p0, &p1);
			out16[dst * 2] = (unsigned char)(p0 & 0xFF);
			out16[dst * 2 + 1] = (unsigned char)((p0 >> 8) & 0xFF);
			dst++;
		}
	}
	return dst == pixels;
}

/** MIPI RAW12 -> P12 (16-bit LE), auto Dt/Hisi vs CSI-2 packing. */
static bool UnpackMipiRaw12ToP12(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	std::vector<unsigned char>& out16)
{
	std::vector<unsigned char> dtBuf;
	std::vector<unsigned char> csiBuf;
	if (!UnpackMipiRaw12ToP12WithPairFn(src, srcBytes, w, h, rowStride, dtBuf, UnpackMipiRaw12PairDt))
		return false;
	if (!UnpackMipiRaw12ToP12WithPairFn(src, srcBytes, w, h, rowStride, csiBuf, UnpackMipiRaw12PairCsi))
	{
		out16.swap(dtBuf);
		return true;
	}
	const unsigned short* pDt = reinterpret_cast<const unsigned short*>(dtBuf.data());
	const unsigned short* pCsi = reinterpret_cast<const unsigned short*>(csiBuf.data());
	const double scoreDt = Mipi12ColumnImbalanceScore(pDt, w, h);
	const double scoreCsi = Mipi12ColumnImbalanceScore(pCsi, w, h);
	if (scoreCsi + 80.0 < scoreDt)
		out16.swap(csiBuf);
	else
		out16.swap(dtBuf);
	return true;
}

/** MIPI RAW12 -> 8-bit Bayer (Dothinkey packed layout). */
static bool UnpackMipiRaw12ToGray8(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& gray)
{
	const unsigned int pixels = w * h;
	if (src == NULL || pixels < 2)
		return false;

	const unsigned needSrc = (pixels * 3 + 1) / 2;
	const unsigned useBytes = (srcBytes > 0 && srcBytes < needSrc) ? srcBytes : needSrc;
	gray.resize(pixels);

	unsigned si = 0;
	unsigned int i = 0;
	for (; i + 1 < pixels; i += 2)
	{
		if (si + 2 >= useBytes)
			return false;
		const unsigned b0 = src[si];
		const unsigned b1 = src[si + 1];
		const unsigned b2 = src[si + 2];
		unsigned short p0 = 0, p1 = 0;
		UnpackMipiRaw12PairDt(b0, b1, b2, &p0, &p1);
		gray[i] = (unsigned char)((p0 + 8) >> 4);
		gray[i + 1] = (unsigned char)((p1 + 8) >> 4);
		si += 3;
	}
	if (i < pixels)
	{
		if (si + 1 >= useBytes)
			return false;
		unsigned short p0 = 0, p1 = 0;
		UnpackMipiRaw12PairDt(src[si], src[si + 1], 0, &p0, &p1);
		gray[i] = (unsigned char)((p0 + 8) >> 4);
	}
	return true;
}

/** MIPI CSI-2 RAW10: 5 bytes carry four 10-bit pixels. */
static bool UnpackMipiRaw10ToGray8(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& gray)
{
	const unsigned int pixels = w * h;
	if (src == NULL || pixels < 4)
		return false;

	const unsigned needSrc = (pixels * 5 + 3) / 4;
	const unsigned useBytes = (srcBytes > 0 && srcBytes < needSrc) ? srcBytes : needSrc;
	gray.resize(pixels);

	unsigned si = 0;
	unsigned int i = 0;
	for (; i + 3 < pixels; i += 4)
	{
		if (si + 4 >= useBytes)
			return false;
		const unsigned b0 = src[si];
		const unsigned b1 = src[si + 1];
		const unsigned b2 = src[si + 2];
		const unsigned b3 = src[si + 3];
		const unsigned b4 = src[si + 4];
		const unsigned p0 = (b0 << 2) | (b1 >> 6);
		const unsigned p1 = ((b1 & 0x3F) << 4) | (b2 >> 4);
		const unsigned p2 = ((b2 & 0x0F) << 6) | (b3 >> 2);
		const unsigned p3 = ((b3 & 0x03) << 8) | b4;
		gray[i] = (unsigned char)(p0 >> 2);
		gray[i + 1] = (unsigned char)(p1 >> 2);
		gray[i + 2] = (unsigned char)(p2 >> 2);
		gray[i + 3] = (unsigned char)(p3 >> 2);
		si += 5;
	}
	while (i < pixels)
	{
		if (si >= useBytes)
			return false;
		gray[i++] = (unsigned char)(src[si++] >> 2);
	}
	return true;
}

static bool UnpackP12ToGray8(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& gray)
{
	const unsigned int pixels = w * h;
	const unsigned needSrc = pixels * 2;
	if (src == NULL || pixels < 1 || srcBytes < needSrc)
		return false;

	gray.resize(pixels);
	for (unsigned int i = 0; i < pixels; i++)
	{
		const unsigned v = (unsigned)src[i * 2] | ((unsigned)src[i * 2 + 1] << 8);
		gray[i] = (unsigned char)((v + 8) >> 4);
	}
	return true;
}

static RAW_FORMAT RawFmtFromGrabTab(const GrabTab* pTab)
{
	if (pTab == NULL)
		return RAW_RGGB;
	return static_cast<RAW_FORMAT>(pTab->sensor.outformat % 4);
}

/**
 * Bayer phase (matches vendor: GrabPara.sensor.outformat, not grabImg default 0).
 * GateSpec override > sensor ini outformat > SDK grab rawFmt.
 */
static RAW_FORMAT ResolveRawFmt(const DtImage_t& grabImg, const GrabTab* pTab, int bayerOverride)
{
	if (bayerOverride >= (int)RAW_RGGB && bayerOverride <= (int)RAW_BGGR)
		return static_cast<RAW_FORMAT>(bayerOverride);
	if (pTab != NULL && IsRawBayerFormat(grabImg.format))
		return static_cast<RAW_FORMAT>(pTab->sensor.outformat % 4);
	if (grabImg.rawFmt >= RAW_RGGB && grabImg.rawFmt <= RAW_BGGR)
		return grabImg.rawFmt;
	return RawFmtFromGrabTab(pTab);
}

static YUV_FORMAT YuvFmtFromGrabTab(const GrabTab* pTab)
{
	if (pTab == NULL)
		return YUV_YCBYCR;
	return static_cast<YUV_FORMAT>(pTab->sensor.outformat % 4);
}

static YUV_FORMAT ResolveYuvFmt(const DtImage_t& grabImg, const GrabTab* pTab)
{
	if (grabImg.yuvFmt >= YUV_YCBYCR && grabImg.yuvFmt <= YUV_CRYCBY)
		return grabImg.yuvFmt;
	DtImage_t meta = grabImg;
	FillSrcMetaFromGrabTab(meta, pTab);
	if (meta.yuvFmt >= YUV_YCBYCR && meta.yuvFmt <= YUV_CRYCBY)
		return meta.yuvFmt;
	return YuvFmtFromGrabTab(pTab);
}

/** YUV422 8-bit packed: extract Y plane (respects line stride from SDK buffer). */
static bool Yuv422ExtractY8(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	YUV_FORMAT yuvFmt,
	std::vector<unsigned char>& gray)
{
	const unsigned int pixels = w * h;
	const unsigned minStride = w * 2;
	if (src == NULL || pixels < 1 || rowStride < minStride || srcBytes < rowStride * h)
		return false;

	int y0 = 0;
	int y1 = 2;
	switch (yuvFmt)
	{
	case YUV_CBYCRY:
	case YUV_CRYCBY:
		y0 = 1;
		y1 = 3;
		break;
	default:
		break;
	}

	gray.resize(pixels);
	for (unsigned int row = 0; row < h; row++)
	{
		const unsigned char* line = src + row * rowStride;
		unsigned char* dst = gray.data() + row * w;
		for (unsigned int x = 0; x < w; x += 2)
		{
			const unsigned char* mac = line + x * 2;
			dst[x] = mac[y0];
			if (x + 1 < w)
				dst[x + 1] = mac[y1];
		}
	}
	return true;
}

static dtkit::YUV422 MapDtkitYuv422(YUV_FORMAT fmt)
{
	switch (fmt)
	{
	case YUV_YCRYCB: return dtkit::YUV_YCRYCB;
	case YUV_CBYCRY: return dtkit::YUV_CBYCRY;
	case YUV_CRYCBY: return dtkit::YUV_CRYCBY;
	default:         return dtkit::YUV_YCBYCR;
	}
}

/* carImageTransform on FORMAT_MIPI_RAW12 returns -29 or crashes; vendor Qt never calls it. */
static const bool kUseCarImageTransformGray = false;

static bool CopyMipiPackedTight(
	const unsigned char* src,
	unsigned srcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	std::vector<unsigned char>& tight)
{
	const unsigned rowBytes = MipiRaw12RowBytes(w);
	if (rowStride < rowBytes)
		rowStride = rowBytes;
	const unsigned need = rowStride * h;
	if (src == NULL || srcBytes < need)
		return false;
	tight.resize(rowBytes * h);
	if (rowStride == rowBytes)
		memcpy(tight.data(), src, tight.size());
	else
	{
		for (unsigned int y = 0; y < h; y++)
			memcpy(tight.data() + y * rowBytes, src + y * rowStride, rowBytes);
	}
	return true;
}

static DtImage_t MakeOwnedSnapImage(
	IMAGE_FORMAT fmt,
	RAW_FORMAT rawFmt,
	YUV_FORMAT yuvFmt,
	unsigned int w,
	unsigned int h,
	unsigned char* data,
	unsigned dataSize)
{
	DtImage_t img = {};
	img.format = fmt;
	img.rawFmt = rawFmt;
	img.yuvFmt = yuvFmt;
	img.width = w;
	img.height = h;
	img.data = data;
	img.dataSize = dataSize;
	return img;
}

typedef void (WINAPI *FnMipi12ToRawRoi)(dtkit::IMAGE_ROI src, dtkit::IMAGE_ROI dst);
typedef void (WINAPI *FnP12ToRaw8Roi)(dtkit::IMAGE_ROI src, dtkit::IMAGE_ROI dst);
typedef void (WINAPI *FnBayer2GrayRoi)(dtkit::IMAGE_ROI src, dtkit::IMAGE_ROI dst, dtkit::BAYER);
typedef void (WINAPI *FnBayer2RgbRoi)(dtkit::IMAGE_ROI src, dtkit::IMAGE_ROI dst, dtkit::BAYER);
typedef void (WINAPI *FnYuv4222RgbRoi)(dtkit::IMAGE_ROI src, dtkit::IMAGE_ROI dst, dtkit::YUV422);

static dtkit::BAYER MapDtkitBayer(RAW_FORMAT rawFmt)
{
	switch (rawFmt)
	{
	case RAW_BGGR: return dtkit::BAYER_BG;
	case RAW_GRBG: return dtkit::BAYER_GB;
	case RAW_GBRG: return dtkit::BAYER_GR;
	case RAW_RGGB:
	default:
		return dtkit::BAYER_RG;
	}
}

static void FillDtkitRoi(dtkit::IMAGE_ROI& roi, unsigned char* buf, int w, int h, dtkit::DEPTH depth, int channels = 1)
{
	roi.depth = depth;
	roi.channels = channels;
	roi.size.width = w;
	roi.size.height = h;
	roi.buffer = buf;
	roi.roi.top_left.x = 0;
	roi.roi.top_left.y = 0;
	roi.roi.size.width = w;
	roi.roi.size.height = h;
}

static HMODULE DtkitModule()
{
	static HMODULE s_mod = NULL;
	static bool s_tried = false;
	if (!s_tried)
	{
		s_tried = true;
#ifdef _M_X64
		static const char* kDllNames[] = { "DtccmKit64.dll", "DtccmKit.dll", "dtkit.dll", NULL };
#else
		static const char* kDllNames[] = { "DtccmKit.dll", "dtkit.dll", NULL };
#endif
		for (int i = 0; kDllNames[i] != NULL; i++)
		{
			s_mod = ::LoadLibraryA(kDllNames[i]);
			if (s_mod != NULL)
				break;
		}
	}
	return s_mod;
}

static bool EnsureDtkitMipiLoaded(FnMipi12ToRawRoi* outMipi12, FnP12ToRaw8Roi* outP12)
{
	static FnMipi12ToRawRoi s_mipi12 = NULL;
	static FnP12ToRaw8Roi s_p12 = NULL;
	static bool s_logged = false;

	if (outMipi12 != NULL)
		*outMipi12 = NULL;
	if (outP12 != NULL)
		*outP12 = NULL;

	HMODULE mod = DtkitModule();
	if (mod != NULL)
	{
		if (s_mipi12 == NULL)
			s_mipi12 = (FnMipi12ToRawRoi)::GetProcAddress(mod, "mipi12ToRawRoi");
		if (s_p12 == NULL)
			s_p12 = (FnP12ToRaw8Roi)::GetProcAddress(mod, "p12ToRaw8Roi");
		if (!s_logged && (s_mipi12 != NULL || s_p12 != NULL))
		{
			s_logged = true;
			msgUtf8(DtZh::kGrabDtkit, s_mipi12 ? 1 : 0, s_p12 ? 1 : 0);
		}
	}

	if (outMipi12 != NULL)
		*outMipi12 = s_mipi12;
	if (outP12 != NULL)
		*outP12 = s_p12;
	return (s_mipi12 != NULL || s_p12 != NULL);
}

static bool DtkitYuv422ToGray8(
	const unsigned char* yuv,
	unsigned rowStride,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& gray)
{
	static FnBayer2GrayRoi s_fn = NULL;
	if (s_fn == NULL)
	{
		HMODULE mod = DtkitModule();
		if (mod != NULL)
			s_fn = (FnBayer2GrayRoi)::GetProcAddress(mod, "bayer2grayRoi");
	}
	if (s_fn == NULL || yuv == NULL || rowStride < w * 2)
		return false;

	std::vector<unsigned char> srcBuf(rowStride * h);
	for (unsigned int row = 0; row < h; row++)
		memcpy(srcBuf.data() + row * rowStride, yuv + row * rowStride, rowStride);

	gray.resize(w * h);
	dtkit::IMAGE_ROI src = {};
	dtkit::IMAGE_ROI dst = {};
	FillDtkitRoi(src, srcBuf.data(), (int)w, (int)h, dtkit::DEPTH_8U, 1);
	FillDtkitRoi(dst, gray.data(), (int)w, (int)h, dtkit::DEPTH_8U, 1);
	s_fn(src, dst, dtkit::BAYER_RG);
	return true;
}

static bool DtkitYuv422ToRgb24(
	const unsigned char* yuv,
	unsigned rowStride,
	unsigned int w,
	unsigned int h,
	YUV_FORMAT yuvFmt,
	std::vector<unsigned char>& rgb)
{
	static FnYuv4222RgbRoi s_fn = NULL;
	if (s_fn == NULL)
	{
		HMODULE mod = DtkitModule();
		if (mod != NULL)
			s_fn = (FnYuv4222RgbRoi)::GetProcAddress(mod, "yuv4222rgbRoi");
	}
	if (s_fn == NULL || yuv == NULL || rowStride < w * 2)
		return false;

	std::vector<unsigned char> srcBuf(rowStride * h);
	for (unsigned int row = 0; row < h; row++)
		memcpy(srcBuf.data() + row * rowStride, yuv + row * rowStride, rowStride);

	rgb.resize(w * h * 3);
	dtkit::IMAGE_ROI src = {};
	dtkit::IMAGE_ROI dst = {};
	FillDtkitRoi(src, srcBuf.data(), (int)w, (int)h, dtkit::DEPTH_8U, 1);
	FillDtkitRoi(dst, rgb.data(), (int)w, (int)h, dtkit::DEPTH_8U, 3);
	s_fn(src, dst, MapDtkitYuv422(yuvFmt));
	return true;
}

static bool YuvToGray8Safe(
	const unsigned char* srcPtr,
	unsigned useSrcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	IMAGE_FORMAT fmt,
	YUV_FORMAT yuvFmt,
	std::vector<unsigned char>& gray,
	const char** outPathTag)
{
	if (outPathTag != NULL)
		*outPathTag = NULL;
	if (srcPtr == NULL || w < 1 || h < 1)
		return false;

	if (IsYuv422Packed8(fmt) || fmt == FORMAT_YUV_10 || fmt == FORMAT_YUV_12)
	{
		if (DtkitYuv422ToGray8(srcPtr, rowStride, w, h, gray))
		{
			if (outPathTag != NULL)
				*outPathTag = "dtkit_yuv_y";
			return true;
		}
		if (Yuv422ExtractY8(srcPtr, useSrcBytes, w, h, rowStride, yuvFmt, gray))
		{
			if (outPathTag != NULL)
				*outPathTag = IsYuv422Packed8(fmt) ? "yuv422_y8" : "yuv10_as_yuv8";
			return true;
		}
	}

	gray.clear();
	return false;
}

static bool DtkitMipi12ToP12(
	const unsigned char* mipi,
	unsigned mipiBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	std::vector<unsigned char>& p12)
{
	FnMipi12ToRawRoi fn = NULL;
	if (!EnsureDtkitMipiLoaded(&fn, NULL) || fn == NULL)
		return false;

	std::vector<unsigned char> mipiBuf;
	if (!CopyMipiPackedTight(mipi, mipiBytes, w, h, rowStride, mipiBuf))
		return false;

	p12.resize(w * h * 2);
	dtkit::IMAGE_ROI src = {};
	dtkit::IMAGE_ROI dst = {};
	/* DtccmKit: src width = image pixel width; row byte count is w*3/2 internally. */
	FillDtkitRoi(src, mipiBuf.data(), (int)w, (int)h, dtkit::DEPTH_8U);
	FillDtkitRoi(dst, p12.data(), (int)w, (int)h, dtkit::DEPTH_16U);
	fn(src, dst);
	return true;
}

static bool DtkitP12ToRaw8(
	const unsigned char* p12,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& gray)
{
	FnP12ToRaw8Roi fn = NULL;
	if (!EnsureDtkitMipiLoaded(NULL, &fn) || fn == NULL)
		return false;

	const unsigned need = w * h * 2;
	if (p12 == NULL || need < 2)
		return false;

	std::vector<unsigned char> srcBuf(p12, p12 + need);
	gray.resize(w * h);

	dtkit::IMAGE_ROI src = {};
	dtkit::IMAGE_ROI dst = {};
	FillDtkitRoi(src, srcBuf.data(), (int)w, (int)h, dtkit::DEPTH_16U);
	FillDtkitRoi(dst, gray.data(), (int)w, (int)h, dtkit::DEPTH_8U);
	fn(src, dst);
	return true;
}

/* Bayer 8-bit raster -> gray via green-channel interpolation (no carImageTransform). */
static bool BayerRasterToGreenGray8(
	const unsigned char* bayer,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt,
	std::vector<unsigned char>& gray)
{
	if (bayer == NULL || w < 2 || h < 2)
		return false;

	gray.resize(w * h);
	for (unsigned int y = 0; y < h; y++)
	{
		for (unsigned int x = 0; x < w; x++)
		{
			const bool xe = ((x & 1) != 0);
			const bool ye = ((y & 1) != 0);
			bool isGreen = false;
			switch (rawFmt)
			{
			case RAW_RGGB:
			case RAW_BGGR:
				isGreen = (xe != ye);
				break;
			case RAW_GRBG:
			case RAW_GBRG:
				isGreen = (xe == ye);
				break;
			default:
				isGreen = (xe != ye);
				break;
			}

			if (isGreen)
			{
				gray[y * w + x] = bayer[y * w + x];
			}
			else
			{
				unsigned int sum = 0;
				unsigned int cnt = 0;
				const int dx[4] = { -1, 1, 0, 0 };
				const int dy[4] = { 0, 0, -1, 1 };
				for (int k = 0; k < 4; k++)
				{
					const int nx = (int)x + dx[k];
					const int ny = (int)y + dy[k];
					if (nx < 0 || ny < 0 || (unsigned int)nx >= w || (unsigned int)ny >= h)
						continue;
					const bool nxe = ((nx & 1) != 0);
					const bool nye = ((ny & 1) != 0);
					bool nGreen = false;
					switch (rawFmt)
					{
					case RAW_RGGB:
					case RAW_BGGR:
						nGreen = (nxe != nye);
						break;
					case RAW_GRBG:
					case RAW_GBRG:
						nGreen = (nxe == nye);
						break;
					default:
						nGreen = (nxe != nye);
						break;
					}
					if (nGreen)
					{
						sum += bayer[(unsigned int)ny * w + (unsigned int)nx];
						cnt++;
					}
				}
				gray[y * w + x] = (cnt > 0) ? (unsigned char)(sum / cnt) : bayer[y * w + x];
			}
		}
	}
	return true;
}

static bool WriteGray8BmpFile(const char* path, const unsigned char* gray, unsigned int w, unsigned int h)
{
	if (path == NULL || gray == NULL || w < 1 || h < 1)
		return false;

	const unsigned int rowBytes = ((w + 3) / 4) * 4;
	const unsigned int paletteSize = 256 * 4;
	const unsigned int headerSize = 14 + 40 + paletteSize;
	const unsigned int pixelBytes = rowBytes * h;
	std::vector<unsigned char> file(headerSize + pixelBytes, 0);

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)file.data();
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(file.data() + sizeof(BITMAPFILEHEADER));
	RGBQUAD* pal = (RGBQUAD*)(file.data() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

	bfh->bfType = 0x4D42;
	bfh->bfOffBits = headerSize;
	bfh->bfSize = headerSize + pixelBytes;
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = (LONG)w;
	bih->biHeight = (LONG)h;
	bih->biPlanes = 1;
	bih->biBitCount = 8;
	bih->biCompression = BI_RGB;
	bih->biSizeImage = pixelBytes;
	for (int i = 0; i < 256; i++)
	{
		pal[i].rgbBlue = (BYTE)i;
		pal[i].rgbGreen = (BYTE)i;
		pal[i].rgbRed = (BYTE)i;
		pal[i].rgbReserved = 0;
	}

	unsigned char* dst = file.data() + headerSize;
	for (unsigned int y = 0; y < h; y++)
	{
		const unsigned char* srcRow = gray + (h - 1 - y) * w;
		memcpy(dst + y * rowBytes, srcRow, w);
	}

	try
	{
		CFile f;
		if (!f.Open(CA2T(path), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
			return false;
		f.Write(file.data(), (UINT)file.size());
		f.Close();
		return true;
	}
	catch (CFileException* e)
	{
		e->Delete();
		return false;
	}
}

static bool WriteRgb24BmpFile(const char* path, const unsigned char* rgb, unsigned int w, unsigned int h)
{
	if (path == NULL || rgb == NULL || w < 1 || h < 1)
		return false;

	const unsigned int rowBytes = ((w * 3 + 3) / 4) * 4;
	const unsigned int headerSize = 14 + 40;
	const unsigned int pixelBytes = rowBytes * h;
	std::vector<unsigned char> file(headerSize + pixelBytes, 0);

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)file.data();
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(file.data() + sizeof(BITMAPFILEHEADER));

	bfh->bfType = 0x4D42;
	bfh->bfOffBits = headerSize;
	bfh->bfSize = headerSize + pixelBytes;
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = (LONG)w;
	bih->biHeight = (LONG)h;
	bih->biPlanes = 1;
	bih->biBitCount = 24;
	bih->biCompression = BI_RGB;
	bih->biSizeImage = pixelBytes;

	unsigned char* dst = file.data() + headerSize;
	for (unsigned int y = 0; y < h; y++)
	{
		const unsigned char* srcRow = rgb + (h - 1 - y) * w * 3;
		unsigned char* dstRow = dst + y * rowBytes;
		for (unsigned int x = 0; x < w; x++)
		{
			dstRow[x * 3 + 0] = srcRow[x * 3 + 2];
			dstRow[x * 3 + 1] = srcRow[x * 3 + 1];
			dstRow[x * 3 + 2] = srcRow[x * 3 + 0];
		}
	}

	try
	{
		CFile f;
		if (!f.Open(CA2T(path), CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
			return false;
		f.Write(file.data(), (UINT)file.size());
		f.Close();
		return true;
	}
	catch (CFileException* e)
	{
		e->Delete();
		return false;
	}
}

static bool DtkitBayer8ToRgb24(
	const unsigned char* bayer8,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt,
	std::vector<unsigned char>& rgb)
{
	static FnBayer2RgbRoi s_fn = NULL;
	if (s_fn == NULL)
	{
		HMODULE mod = DtkitModule();
		if (mod != NULL)
			s_fn = (FnBayer2RgbRoi)::GetProcAddress(mod, "bayer2rgbRoi");
	}
	if (s_fn == NULL || bayer8 == NULL || w < 1 || h < 1)
		return false;

	rgb.resize(w * h * 3);
	dtkit::IMAGE_ROI src = {};
	dtkit::IMAGE_ROI dst = {};
	FillDtkitRoi(src, const_cast<unsigned char*>(bayer8), (int)w, (int)h, dtkit::DEPTH_8U, 1);
	FillDtkitRoi(dst, rgb.data(), (int)w, (int)h, dtkit::DEPTH_8U, 3);
	s_fn(src, dst, MapDtkitBayer(rawFmt));
	return true;
}

/** UNPACK10 (10-bit in uint16) -> 8-bit Bayer for preview, aligned with qtmALGO. */
static bool Unpack10ToBayer8(
	const unsigned short* u10,
	unsigned int w,
	unsigned int h,
	std::vector<unsigned char>& bayer8)
{
	const unsigned int n = w * h;
	if (u10 == NULL || n < 1)
		return false;
	bayer8.resize(n);
	for (unsigned int i = 0; i < n; i++)
		bayer8[i] = (unsigned char)(((unsigned)u10[i] + 2) >> 2);
	return true;
}

/** P12 -> UNPACK10 -> Bayer8 -> RGB24 BMP (Huawei/qtmALGO domain, not raw P12 mosaic). */
static bool SavePreviewBmpFromP12(
	const unsigned char* p12,
	unsigned p12Bytes,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt,
	const char* pathA,
	const char** outTag)
{
	if (outTag != NULL)
		*outTag = NULL;
	if (p12 == NULL || w < 1 || h < 1 || p12Bytes < w * h * 2)
		return false;

	const unsigned int n = w * h;
	const unsigned short* p16 = reinterpret_cast<const unsigned short*>(p12);
	std::vector<unsigned short> u10(n);
	ConvertP12ToUnpack10(p16, u10.data(), n);

	std::vector<unsigned char> bayer8;
	if (!Unpack10ToBayer8(u10.data(), w, h, bayer8))
		return false;

	std::vector<unsigned char> rgb;
	if (DtkitBayer8ToRgb24(bayer8.data(), w, h, rawFmt, rgb)
		&& WriteRgb24BmpFile(pathA, rgb.data(), w, h))
	{
		if (outTag != NULL)
			*outTag = "u10_bayer_rgb";
		return true;
	}

	if (outTag != NULL)
		*outTag = "u10_bayer8";
	return WriteGray8BmpFile(pathA, bayer8.data(), w, h);
}

static bool SavePreviewBmpFromBayer8(
	const unsigned char* bayer8,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt,
	const char* pathA,
	const char** outTag)
{
	if (outTag != NULL)
		*outTag = NULL;
	if (bayer8 == NULL || w < 1 || h < 1)
		return false;

	std::vector<unsigned char> rgb;
	if (DtkitBayer8ToRgb24(bayer8, w, h, rawFmt, rgb)
		&& WriteRgb24BmpFile(pathA, rgb.data(), w, h))
	{
		if (outTag != NULL)
			*outTag = "dtkit_raw8_rgb";
		return true;
	}
	if (outTag != NULL)
		*outTag = "raw8_gray";
	return WriteGray8BmpFile(pathA, bayer8, w, h);
}

static bool MipiRaw12ToP12(
	const unsigned char* srcPtr,
	unsigned useSrcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	std::vector<unsigned char>& p12,
	const char** outTag)
{
	if (outTag != NULL)
		*outTag = NULL;
	if (DtkitMipi12ToP12(srcPtr, useSrcBytes, w, h, rowStride, p12))
	{
		if (outTag != NULL)
			*outTag = "dtkit_mipi_p12";
		return true;
	}
	if (UnpackMipiRaw12ToP12(srcPtr, useSrcBytes, w, h, rowStride, p12))
	{
		if (outTag != NULL)
			*outTag = "sw_mipi_p12";
		return true;
	}
	return false;
}

/* MIPI RAW12 -> gray8 without carImageTransform: DtccmKit first, then software unpack. */
static bool MipiRaw12ToGray8Safe(
	const unsigned char* srcPtr,
	unsigned useSrcBytes,
	unsigned int w,
	unsigned int h,
	unsigned rowStride,
	RAW_FORMAT rawFmt,
	std::vector<unsigned char>& gray,
	bool& outBayerRaster,
	const char** outPathTag)
{
	outBayerRaster = false;
	if (outPathTag != NULL)
		*outPathTag = NULL;
	if (srcPtr == NULL || w < 1 || h < 1)
		return false;

	std::vector<unsigned char> p12;
	const char* p12Tag = NULL;
	if (!MipiRaw12ToP12(srcPtr, useSrcBytes, w, h, rowStride, p12, &p12Tag))
		return false;

	std::vector<unsigned char> bayer8;
	/* Prefer Bayer 8-bit raster for hot-pixel (same-color neighbors); green-gray causes false hot. */
	if (UnpackP12ToGray8(p12.data(), (unsigned)p12.size(), w, h, gray))
	{
		outBayerRaster = true;
		if (outPathTag != NULL)
			*outPathTag = "sw_p12_bayer";
		return true;
	}

	if (DtkitP12ToRaw8(p12.data(), w, h, gray))
	{
		outBayerRaster = true;
		if (outPathTag != NULL)
			*outPathTag = "dtkit_p12_bayer";
		return true;
	}

	if (DtkitP12ToRaw8(p12.data(), w, h, bayer8)
		&& BayerRasterToGreenGray8(bayer8.data(), w, h, rawFmt, gray))
	{
		outBayerRaster = false;
		if (outPathTag != NULL)
			*outPathTag = "dtkit_p12_g8";
		return true;
	}

	if (UnpackP12ToGray8(p12.data(), (unsigned)p12.size(), w, h, bayer8)
		&& BayerRasterToGreenGray8(bayer8.data(), w, h, rawFmt, gray))
	{
		outBayerRaster = false;
		if (outPathTag != NULL)
			*outPathTag = "sw_p12_green";
		return true;
	}

	if (UnpackMipiRaw12ToGray8(srcPtr, useSrcBytes, w, h, gray))
	{
		outBayerRaster = true;
		if (outPathTag != NULL)
			*outPathTag = "sw_mipi_bayer";
		return true;
	}

	gray.clear();
	return false;
}

static bool BuildGrabSrcImage(
	const unsigned char* srcPtr,
	unsigned useSrcBytes,
	unsigned int w,
	unsigned int h,
	IMAGE_FORMAT format,
	RAW_FORMAT rawFmt,
	std::vector<unsigned char>& ownedBuf,
	DtImage_t& outImg)
{
	if (srcPtr == NULL || useSrcBytes < 1 || w < 1 || h < 1)
		return false;
	ownedBuf.assign(srcPtr, srcPtr + useSrcBytes);
	outImg = {};
	outImg.format = format;
	outImg.rawFmt = rawFmt;
	outImg.width = w;
	outImg.height = h;
	outImg.dataSize = useSrcBytes;
	outImg.data = ownedBuf.data();
	return true;
}

static bool TransformGrabToGray8(
	const DtImage_t& grabImg,
	const GrabTab* pTab,
	int devId,
	std::vector<unsigned char>& gray,
	unsigned int& outW,
	unsigned int& outH,
	bool& outBayerRaster)
{
	const unsigned int w = grabImg.width;
	const unsigned int h = grabImg.height;
	const unsigned int need = w * h;
	outBayerRaster = false;
	if (grabImg.data == NULL || w < 1 || h < 1 || need < 1)
		return false;

	if (IsPackedGrayFormat(grabImg.format))
	{
		const unsigned srcBytes = GrabSrcBytes(grabImg);
		const unsigned copyBytes = srcBytes > 0 && srcBytes < need ? srcBytes : need;
		gray.assign(grabImg.data, grabImg.data + copyBytes);
		outW = w;
		outH = h;
		outBayerRaster = false;
		return gray.size() >= need;
	}

	if (IsYuvFormat(grabImg.format))
	{
		const YUV_FORMAT yuvFmt = ResolveYuvFmt(grabImg, pTab);
		const unsigned useSrcBytes = GrabPayloadBytes(grabImg);
		const unsigned rowStride = GrabRowStrideBytes(grabImg);
		if (useSrcBytes < 1 || rowStride < w * 2)
			return false;
		const char* pathTag = NULL;
		if (YuvToGray8Safe(grabImg.data, useSrcBytes, w, h, rowStride, grabImg.format, yuvFmt, gray, &pathTag))
		{
			outW = w;
			outH = h;
			outBayerRaster = false;
			return true;
		}
		return false;
	}

	const RAW_FORMAT rawFmt = ResolveRawFmt(grabImg, pTab, -1);

	const unsigned srcBytes = GrabSrcBytes(grabImg);
	if (srcBytes < 1)
		return false;

	const unsigned char* srcPtr = grabImg.data;
	const unsigned useSrcBytes = (grabImg.dataSize > 0 && grabImg.dataSize < srcBytes)
		? grabImg.dataSize : srcBytes;

	if (grabImg.format == FORMAT_MIPI_RAW12)
	{
		const unsigned rowStride = GrabRowStrideBytes(grabImg);
		const char* pathTag = NULL;
		if (MipiRaw12ToGray8Safe(srcPtr, useSrcBytes, w, h, rowStride, rawFmt, gray, outBayerRaster, &pathTag))
		{
			outW = w;
			outH = h;
			return true;
		}
		return false;
	}
	else if (grabImg.format == FORMAT_MIPI_RAW10)
	{
		if (UnpackMipiRaw10ToGray8(srcPtr, useSrcBytes, w, h, gray))
		{
			outW = w;
			outH = h;
			outBayerRaster = true;
			return true;
		}
	}
	else if (grabImg.format == FORMAT_P12)
	{
		std::vector<unsigned char> bayer8;
		if (UnpackP12ToGray8(srcPtr, useSrcBytes, w, h, gray))
		{
			outW = w;
			outH = h;
			outBayerRaster = true;
			return true;
		}
		if (DtkitP12ToRaw8(srcPtr, w, h, gray))
		{
			outW = w;
			outH = h;
			outBayerRaster = true;
			return true;
		}
		if (UnpackP12ToGray8(srcPtr, useSrcBytes, w, h, bayer8)
			&& BayerRasterToGreenGray8(bayer8.data(), w, h, rawFmt, gray))
		{
			outW = w;
			outH = h;
			outBayerRaster = false;
			return true;
		}
		return false;
	}

	if (grabImg.format == FORMAT_MIPI_RAW10)
		return false;

	if (kUseCarImageTransformGray)
	{
		std::vector<unsigned char> srcBuf(useSrcBytes);
		memcpy(srcBuf.data(), srcPtr, useSrcBytes);

		DtImage_t srcImg = grabImg;
		srcImg.data = srcBuf.data();
		srcImg.dataSize = useSrcBytes;
		FillSrcMetaFromGrabTab(srcImg, pTab);

		DtImage_t destImg = {};
		destImg.format = IsRawBayerFormat(grabImg.format) ? FORMAT_G8 : FORMAT_GRAY8;
		destImg.width = w;
		destImg.height = h;
		destImg.dataSize = need;
		gray.resize(need);
		destImg.data = gray.data();

		const int tr = ::carImageTransform(&srcImg, &destImg, devId);
		if (tr == DT_ERROR_OK)
		{
			outW = w;
			outH = h;
			outBayerRaster = false;
			return true;
		}
	}

	outW = w;
	outH = h;
	outBayerRaster = false;
	return false;
}

} // namespace

struct DtWorkThreadParam
{
	DtCarFunction* fn;
	int devId;
};

void DtCarFunction::PreloadEzCarSdkDlls()
{
	CString exePath;
	GetExePath(exePath);
	const int slash = exePath.ReverseFind(_T('\\'));
	const CString dir = (slash >= 0) ? exePath.Left(slash + 1) : exePath;
	const CStringA dirA(dir);

	::SetDllDirectoryA(dirA);

#ifdef _M_X64
	const CString kit64 = dir + _T("DtccmKit64.dll");
	const CString kit = dir + _T("DtccmKit.dll");
	if (::GetFileAttributes(kit64) != INVALID_FILE_ATTRIBUTES
		&& ::GetFileAttributes(kit) == INVALID_FILE_ATTRIBUTES)
	{
		if (::CopyFile(kit64, kit, FALSE))
			msg("Created DtccmKit.dll alias from DtccmKit64.dll\r\n");
	}
#endif

#ifdef _M_X64
	static const char* kPreload[] = {
		"ezDtCarDTCCM64.dll",
		"DtccmKit64.dll",
		"DtccmKit.dll",
		"dtccm2.dll",
		"dtccm2_legacy.dll",
		NULL
	};
#else
	static const char* kPreload[] = {
		"ezDtCarDTCCM.dll",
		"DtccmKit.dll",
		"dtccm2.dll",
		"dtccm2_legacy.dll",
		NULL
	};
#endif
	int loaded = 0;
	for (int i = 0; kPreload[i] != NULL; i++)
	{
		CStringA full = dirA + kPreload[i];
		HMODULE h = ::LoadLibraryA(full);
		if (h == NULL)
			h = ::LoadLibraryA(kPreload[i]);
		if (h != NULL)
			loaded++;
	}
	msg("SDK DLL preload: %d modules from %s\r\n", loaded, (LPCSTR)dirA);
	if (::GetModuleHandleA("DtccmKit.dll") == NULL && ::GetModuleHandleA("DtccmKit64.dll") == NULL)
	{
		CStringA kitPathA = CStringA(dirA) + "DtccmKit64.dll";
		if (::LoadLibraryA(kitPathA) == NULL)
			kitPathA = CStringA(dirA) + "DtccmKit.dll";
		::LoadLibraryA(kitPathA);
	}
	/* DtccmKit optional; carDrawImage is in ezDtCarDTCCM64.dll. Suppress noisy WARN until load failure is diagnosed.
	if (::GetModuleHandleA("DtccmKit.dll") == NULL && ::GetModuleHandleA("DtccmKit64.dll") == NULL)
		msg("WARN: DtccmKit not loaded ? carDrawImage may crash (copy SDK DLLs to exe folder)\r\n");
	*/
}

DtCarFunction::DtCarFunction()
{
	m_hwndFirmwareBurnProgress = NULL;
	m_iEnumDevNum = 0;
	m_iVcNum = 1;
	m_iBoxType = Box_UC930;
	InitializeCriticalSection(&m_csGrab);
	memset(m_hWndVideo, 0, sizeof(m_hWndVideo));
	memset(m_workPowerReady, 0, sizeof(m_workPowerReady));
	memset(m_workGrabReady, 0, sizeof(m_workGrabReady));
	memset(m_previewDisplayInit, 0, sizeof(m_previewDisplayInit));
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		for (int v = 0; v < MAX_VC; v++)
		{
			m_vidWndW[d][v] = 2;
			m_vidWndH[d][v] = 2;
		}
	ResetChannelEnable();
	memset(m_grabTabValid, 0, sizeof(m_grabTabValid));
	memset(m_grabTab, 0, sizeof(m_grabTab));
	m_specDelayMs = 2000;
	m_dwProductionRunStartTick = 0;
	m_bProductionRunActive = FALSE;
	m_bLightGateHasResult = false;
	memset(m_bLightGatePass, 0, sizeof(m_bLightGatePass));
	m_bFirmwareBurnHasResult = false;
	m_bFirmwareBurnInProgress = false;
	m_bPauseCaptureForBurn = false;
	m_bSuppressWorkDraw = false;
	memset(m_bFirmwareBurnPass, 0, sizeof(m_bFirmwareBurnPass));
	memset(m_fwBurnErrCode, 0, sizeof(m_fwBurnErrCode));
	m_bSensorIdHasResult = false;
	memset(m_bSensorIdReadOk, 0, sizeof(m_bSensorIdReadOk));
	memset(m_sensorIdHex, 0, sizeof(m_sensorIdHex));
	m_bFirmwareBurnVerifyHasResult = false;
	memset(m_bFirmwareBurnVerifyPass, 0, sizeof(m_bFirmwareBurnVerifyPass));
	m_hwndFirmwareBurnProgress = NULL;
	m_bFwBurnOverlay = false;
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		for (int v = 0; v < MAX_VC; v++)
			m_fwBurnPct[d][v] = kFwBurnPctInactive;
	m_gateBadPixelDark = GateDefaultBadPixelDark();
	m_gateFirmwareBurn = GateDefaultFirmwareBurn();
	m_gateTcpNotify = GateDefaultTcpNotify();
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		InitializeCriticalSection(&m_csBurnDev[d]);

	GetExePath(m_strDtCarIniPath);
	const int slash = m_strDtCarIniPath.ReverseFind(_T('\\'));
	if (slash >= 0)
		m_strGateSpecIniPath = m_strDtCarIniPath.Left(slash + 1) + _T("GateSpec.ini");
	else
		m_strGateSpecIniPath = _T("GateSpec.ini");

	ReadDtCarIni();
	ReadGateSpecIni();
}

DtCarFunction::~DtCarFunction()
{
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		DeleteCriticalSection(&m_csBurnDev[d]);
	SaveDtCarIni();
}

bool DtCarFunction::IsFirmwareBurnCellActive(int dev, int vc) const
{
	if (!m_bFwBurnOverlay || dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return false;
	return m_fwBurnPct[dev][vc] >= 0;
}

int DtCarFunction::GetFirmwareBurnPercent(int dev, int vc) const
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return kFwBurnPctInactive;
	return m_fwBurnPct[dev][vc];
}

void DtCarFunction::SetFirmwareBurnPercent(int dev, int vc, int pct)
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return;
	if (pct < kFwBurnPctInactive)
		pct = kFwBurnPctInactive;
	if (pct > 100)
		pct = 100;
	m_fwBurnPct[dev][vc] = pct;
}

void DtCarFunction::ClearFirmwareBurnUiState()
{
	m_bFwBurnOverlay = false;
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		for (int v = 0; v < MAX_VC; v++)
			m_fwBurnPct[d][v] = kFwBurnPctInactive;
}

void DtCarFunction::ResetFirmwareBurnUiForEnabledChannels()
{
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
		for (int v = 0; v < MAX_VC; v++)
			m_fwBurnPct[d][v] = kFwBurnPctInactive;
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;
			m_fwBurnPct[d][v] = 0;
		}
	}
}

int DtCarFunction::DrawImageOnUiThread(const DrawImage_t& di, int vcId, int devId)
{
	/* Stop/Join: never block WorkProc on UI carDrawImage (SendMessage). */
	if (m_bSuppressWorkDraw || !m_bRunning)
		return DT_ERROR_OK;

	/* Burning cell: WorkProc must not push live frames (see WorkProc). */
	if (IsFirmwareBurnCellActive(devId, vcId))
		return DT_ERROR_OK;

	DtUiDrawPack pack;
	memset(&pack, 0, sizeof(pack));
	pack.hVideoWnd = di.hVideoWnd;
	pack.nImgWndW = di.nImgWndW;
	pack.nImgWndH = di.nImgWndH;
	pack.bShowImg = di.bShowImg;
	pack.bShowText = di.bShowText;
	pack.vcId = vcId;
	pack.devId = devId;
	if (di.szShowData != NULL)
	{
		strncpy(pack.szShowData, di.szShowData, sizeof(pack.szShowData) - 1);
		pack.szShowData[sizeof(pack.szShowData) - 1] = '\0';
	}

	const HWND hUi = m_hwndFirmwareBurnProgress;
	if (hUi != NULL && ::IsWindow(hUi))
		return (int)::SendMessage(hUi, WM_DT_CAR_DRAW, 0, (LPARAM)&pack);

	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL && pMain->GetSafeHwnd() != NULL)
		return (int)::SendMessage(pMain->GetSafeHwnd(), WM_DT_CAR_DRAW, 0, (LPARAM)&pack);
	return ::carDrawImage(di, vcId, devId);
}

void DtCarFunction::ResetPreviewDisplay()
{
	memset(m_previewDisplayInit, 0, sizeof(m_previewDisplayInit));
}

bool DtCarFunction::EnsurePreviewDisplay(int dev, int vc)
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return false;
	if (m_previewDisplayInit[dev][vc])
		return true;
	const HWND h = m_hWndVideo[dev][vc];
	if (h == NULL || !::IsWindow(h))
		return false;
	if (m_vidWndW[dev][vc] < 1 || m_vidWndH[dev][vc] < 1)
		return false;
	m_previewDisplayInit[dev][vc] = true;
	return true;
}

void DtCarFunction::InitAllPreviewDisplays()
{
	for (int d = 0; d < 8; d++)
		for (int v = 0; v < 4; v++)
			EnsurePreviewDisplay(d, v);
}

void DtCarFunction::SetVideoCellLayout(int dev, int vc, HWND hwnd, unsigned short w, unsigned short h)
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV || vc < 0 || vc >= MAX_VC)
		return;
	m_hWndVideo[dev][vc] = hwnd;
	/* w/h == 0 marks hidden cell (ReSize); do not clamp to 1. */
	if (w > 0 && h > 0)
	{
		if (w > 65535) w = 65535;
		if (h > 65535) h = 65535;
	}
	m_vidWndW[dev][vc] = w;
	m_vidWndH[dev][vc] = h;
}

BEGIN_MESSAGE_MAP(DtCarFunction, CWnd)
END_MESSAGE_MAP()

// Read DtCar app settings from INI
int DtCarFunction::ReadDtCarIni() {
	m_strSensorIniPath = GetIniFileString("sensor", "INI", "", m_strDtCarIniPath);
	LoadChannelEnableIni();
	return 1;
}

bool DtCarFunction::IsDevEnabled(int dev) const
{
	if (dev < 0 || dev >= MAX_CC16 * MAX_DEV)
		return false;
	return m_iDevEnable[dev] != 0;
}

bool DtCarFunction::IsVcEnabled(int dev, int vc) const
{
	if (!IsDevEnabled(dev) || vc < 0 || vc >= MAX_VC)
		return false;
	return m_iVcEnable[dev][vc] != 0;
}

bool DtCarFunction::IsPreviewCellReady(int dev, int vc) const
{
	if (dev < 0 || dev >= m_iEnumDevNum || !IsVcEnabled(dev, vc))
		return false;
	if (vc < 0 || vc >= MAX_VC)
		return false;
	const HWND h = m_hWndVideo[dev][vc];
	if (h == NULL || !::IsWindow(h))
		return false;
	if (m_vidWndW[dev][vc] < 2 || m_vidWndH[dev][vc] < 2)
		return false;
	return true;
}

bool DtCarFunction::HasAnyChannelEnabled() const
{
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (IsVcEnabled(d, v))
				return true;
		}
	}
	return false;
}

void DtCarFunction::ResetChannelEnable()
{
	memset(m_iDevEnable, 0, sizeof(m_iDevEnable));
	memset(m_iVcEnable, 0, sizeof(m_iVcEnable));
}

void DtCarFunction::ApplyChannelEnableDefaultsAfterEnum()
{
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
	{
		m_iDevEnable[d] = 0;
		for (int v = 0; v < MAX_VC; v++)
			m_iVcEnable[d][v] = 0;
	}
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		m_iDevEnable[d] = 1;
		for (int v = 0; v < m_iVcNum; v++)
			m_iVcEnable[d][v] = 1;
	}
}

static int ChannelIniDevCount(const DtCarFunction* fn)
{
	if (fn != NULL && fn->m_iEnumDevNum > 0 && fn->m_iEnumDevNum <= MAX_DEV)
		return fn->m_iEnumDevNum;
	return MAX_DEV;
}

static int ChannelIniVcCount(const DtCarFunction* fn)
{
	if (fn != NULL && fn->m_iVcNum > 0 && fn->m_iVcNum <= MAX_VC)
		return fn->m_iVcNum;
	return MAX_VC;
}

static void ChannelEnableKey(int dev, int vc, CString& key)
{
	key.Format(_T("D%d_V%d"), dev, vc);
}

void DtCarFunction::LoadChannelEnableIni()
{
	const int devSlots = ChannelIniDevCount(this);
	const int vcSlots = ChannelIniVcCount(this);
	ResetChannelEnable();

	/* New compact format: D0_V0=1 (only enabled entries required). */
	for (int d = 0; d < devSlots; d++)
	{
		for (int v = 0; v < vcSlots; v++)
		{
			CString key;
			ChannelEnableKey(d, v, key);
			int on = GetIniFileInt(_T("ChannelEnable"), key, -1, m_strDtCarIniPath);
			if (on < 0)
			{
				const int idx = d * MAX_VC + v;
				CString legacy;
				legacy.Format(_T("VcEnable_%d"), idx);
				on = GetIniFileInt(_T("ChannelEnable"), legacy, 0, m_strDtCarIniPath);
			}
			m_iVcEnable[d][v] = (on != 0) ? 1 : 0;
		}
		BOOL anyVc = FALSE;
		for (int v = 0; v < vcSlots; v++)
		{
			if (m_iVcEnable[d][v] != 0)
			{
				anyVc = TRUE;
				break;
			}
		}
		m_iDevEnable[d] = anyVc ? 1 : 0;
	}
}

void DtCarFunction::SaveChannelEnableIni()
{
	const int devSlots = ChannelIniDevCount(this);
	const int vcSlots = ChannelIniVcCount(this);
	int enabledVc = 0;

	/* Rewrite section: only D#_V#=1 lines for enabled channels (+ header). */
	WritePrivateProfileString(_T("ChannelEnable"), NULL, NULL, m_strDtCarIniPath);
	WriteIniFileString(_T("ChannelEnable"), _T("Format"), _T("D{dev}_V{vc}=1"), m_strDtCarIniPath);
	{
		CString meta;
		meta.Format(_T("%d"), devSlots);
		WriteIniFileString(_T("ChannelEnable"), _T("DevCount"), meta, m_strDtCarIniPath);
		meta.Format(_T("%d"), vcSlots);
		WriteIniFileString(_T("ChannelEnable"), _T("VcPerDev"), meta, m_strDtCarIniPath);
	}

	for (int d = 0; d < devSlots; d++)
	{
		for (int v = 0; v < vcSlots; v++)
		{
			if (m_iVcEnable[d][v] == 0)
				continue;
			CString key;
			ChannelEnableKey(d, v, key);
			WriteIniFileString(_T("ChannelEnable"), key, _T("1"), m_strDtCarIniPath);
			enabledVc++;
		}
	}
	FlushIniFile(m_strDtCarIniPath);
	{
		CStringA pathA(m_strDtCarIniPath);
		msgUtf8(DtZh::kLogChannelSaved,
			enabledVc, devSlots, vcSlots, (LPCSTR)pathA);
	}
}

void DtCarFunction::ShowChannelSelectDialog(CWnd* pParent)
{
	if (m_iEnumDevNum <= 0)
	{
		msgUtf8(DtZh::kDlgChannelNeedEnum);
		return;
	}
	GetExePath(m_strDtCarIniPath);
	LoadChannelEnableIni();
	{
		int n = 0;
		for (int d = 0; d < m_iEnumDevNum; d++)
			for (int v = 0; v < m_iVcNum; v++)
				if (IsVcEnabled(d, v))
					n++;
		CStringA pathA(m_strDtCarIniPath);
		msgUtf8(DtZh::kLogChannelLoad, (LPCSTR)pathA, n);
	}
	if (FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(IDD_DIALOG_CHANNEL), RT_DIALOG) == NULL)
	{
		msg("Channel dialog resource missing (IDD_DIALOG_CHANNEL). Rebuild DtSample.\r\n");
		return;
	}
	CDtChannelDlg dlg(this, pParent);
	const INT_PTR ret = dlg.DoModal();
	if (ret == -1)
	{
		msg("Channel dialog failed to create. Rebuild DtSample (check DtSample.rc).\r\n");
		return;
	}
	if (ret == IDOK)
		msgUtf8(DtZh::kChannelSaved);
}

int DtCarFunction::ReadGateSpecIni()
{
	if (m_strGateSpecIniPath.IsEmpty())
		return 0;
	if (GetFileAttributes(m_strGateSpecIniPath) == INVALID_FILE_ATTRIBUTES)
	{
		const GateChannelLimits kFb = { 1.0, 200.0, 0.0, 2000.0, -40.0, 125.0 };
		m_gateDefault = kFb;
		m_gateSensorTempI2c = GateDefaultSensorTempI2c();
		GateInitDefaultTempI2cAddrGrid(m_gateTempI2cAddr);
		m_gateBadPixelDark = GateDefaultBadPixelDark();
		m_gateFirmwareBurn = GateDefaultFirmwareBurn();
		m_specDelayMs = 2000;
		SaveGateSpecIni();
	}

	m_specDelayMs = GetIniFileInt(_T("timing"), _T("DelayMs"), m_specDelayMs, m_strGateSpecIniPath);
	if (m_specDelayMs < 200)
		m_specDelayMs = 200;

	const GateChannelLimits kFb = { 1.0, 200.0, 0.0, 2000.0, -40.0, 125.0 };
	GateIniFillLimits(m_strGateSpecIniPath, _T("limits"), kFb, &m_gateDefault);

	for (int d = 0; d < kGateSpecIniDevSlots; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			CString sec;
			sec.Format(_T("D%d_V%d"), d, v);
			GateIniFillLimits(m_strGateSpecIniPath, sec, m_gateDefault, &m_gatePerChannel[d][v]);
		}
	}

	GateIniFillSensorTempI2c(m_strGateSpecIniPath, _T("sensor_temp_i2c"), GateDefaultSensorTempI2c(), &m_gateSensorTempI2c);
	GateIniFillTempI2cAddrGrid(m_strGateSpecIniPath, m_gateTempI2cAddr);
	GateIniFillBadPixelDark(m_strGateSpecIniPath, GateDefaultBadPixelDark(), &m_gateBadPixelDark);
	GateIniFillFirmwareBurn(m_strGateSpecIniPath, GateDefaultFirmwareBurn(), &m_gateFirmwareBurn);
	/* [tcp_notify] PeerHost/PeerPort -> lighting station TCP (see DtTcpNotify.cpp) */
	GateIniFillTcpNotify(m_strGateSpecIniPath, GateDefaultTcpNotify(), &m_gateTcpNotify);
	{
		CStringA pathA(m_strGateSpecIniPath);
		msgUtf8(DtZh::kLogGateSpecFw,
			m_gateFirmwareBurn.enabled ? 1 : 0,
			m_gateFirmwareBurn.readSensorIdEnabled ? 1 : 0,
			(LPCSTR)pathA);
		CStringA hostA(m_gateTcpNotify.peerHost);
		msgUtf8(DtZh::kLogGateSpecTcp,
			m_gateTcpNotify.enabled ? 1 : 0,
			hostA.GetString(), m_gateTcpNotify.peerPort);
	}
	return 1;
}

static bool GateLimEqual(const GateChannelLimits& a, const GateChannelLimits& b)
{
	const double eps = 1e-4;
	return fabs(a.minSsrFps - b.minSsrFps) < eps
		&& fabs(a.maxSsrFps - b.maxSsrFps) < eps
		&& fabs(a.minCurrent_mA - b.minCurrent_mA) < eps
		&& fabs(a.maxCurrent_mA - b.maxCurrent_mA) < eps
		&& fabs(a.minSensorTemp_C - b.minSensorTemp_C) < eps
		&& fabs(a.maxSensorTemp_C - b.maxSensorTemp_C) < eps;
}

static void GateWriteDbl(LPCTSTR path, LPCTSTR sec, LPCTSTR key, double v)
{
	CString s;
	s.Format(_T("%.3f"), v);
	WriteIniFileString(sec, key, s, path);
}

static CString GateSpecTemplatePath(LPCTSTR iniPath)
{
	TCHAR modPath[MAX_PATH] = {};
	GetModuleFileName(NULL, modPath, MAX_PATH);
	CString exeDir(modPath);
	const int slash = exeDir.ReverseFind(_T('\\'));
	if (slash >= 0)
		exeDir = exeDir.Left(slash + 1);

	CString p1 = exeDir + _T("GateSpec.template.utf8");
	if (GetFileAttributes(p1) != INVALID_FILE_ATTRIBUTES)
		return p1;

	if (iniPath != NULL && iniPath[0] != 0)
	{
		CString dir(iniPath);
		const int s2 = dir.ReverseFind(_T('\\'));
		if (s2 >= 0)
		{
			CString p2 = dir.Left(s2 + 1) + _T("GateSpec.template.utf8");
			if (GetFileAttributes(p2) != INVALID_FILE_ATTRIBUTES)
				return p2;
		}
	}
	return p1;
}

static bool ReadFileBytesA(const CString& path, CStringA& out)
{
	out.Empty();
	CFile f;
	if (!f.Open(path, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
		return false;
	const ULONGLONG sz = f.GetLength();
	if (sz == 0 || sz > 1024 * 1024)
	{
		f.Close();
		return false;
	}
	const UINT n = (UINT)sz;
	LPSTR buf = out.GetBuffer(n);
	const UINT rd = f.Read(buf, n);
	out.ReleaseBuffer(rd);
	f.Close();
	return rd > 0;
}

static void ReplaceTokenA(CStringA& text, const char* token, const CStringA& value)
{
	if (token == NULL)
		return;
	const CStringA tok(token);
	for (;;)
	{
		const int pos = text.Find(tok);
		if (pos < 0)
			break;
		text.Delete(pos, tok.GetLength());
		text.Insert(pos, value);
	}
}

static bool WriteAcpIniFile(LPCTSTR path, const CStringA& acpBody)
{
	CFile f;
	if (!f.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		return false;
	if (acpBody.GetLength() > 0)
		f.Write((LPCSTR)acpBody, (UINT)acpBody.GetLength());
	f.Close();
	return true;
}

static void LogBadPixelNgReason(int devId, int vcId, const BadPixelDarkResult& r,
	const GateBadPixelDarkCfg& cfg, bool huaweiAlgo, bool huaweiP12Domain, const HotPixelHuaweiDetail* hwDetail)
{
	if (r.pass || !r.analyzed)
		return;

	switch (r.failReason)
	{
	case BP_FAIL_DARK_SCENE:
		if (huaweiAlgo)
		{
			if (huaweiP12Domain)
				msgUtf8(DtZh::kBpFailDarkP12, r.centerRoiMean, r.frameMean);
			else
				msgUtf8(DtZh::kBpFailDark, r.centerRoiMean, r.frameMean);
		}
		else
			msgUtf8(DtZh::kBpFailDarkNeighbor, r.frameMean);
		break;
	case BP_FAIL_CLUSTER_COUNT:
		msgUtf8(DtZh::kBpFailCluster, r.badCount, cfg.maxBadPixels);
		if (hwDetail != NULL && huaweiAlgo)
		{
			msgUtf8(DtZh::kBpClusters,
				hwDetail->clusterR, hwDetail->clusterGr, hwDetail->clusterGb,
				hwDetail->clusterB, hwDetail->clusterG);
		}
		break;
	case BP_FAIL_SINGLE_COUNT:
	{
		const unsigned int singleLim = (unsigned int)((double)(r.width * r.height)
			* (double)cfg.singleDefectPermyriad / 100000.0);
		msgUtf8(DtZh::kBpFailSingle, r.singleDefectCount, singleLim);
		break;
	}
	case BP_FAIL_HOT_PIXEL_COUNT:
		msgUtf8(DtZh::kBpFailHot, r.badCount, cfg.maxBadPixels);
		break;
	default:
		msgUtf8(DtZh::kBpFailHot, r.badCount, cfg.maxBadPixels);
		break;
	}
}

int DtCarFunction::SaveGateSpecIni()
{
	LPCTSTR path = m_strGateSpecIniPath;
	if (path == NULL || path[0] == 0)
		return 0;

	DWORD attr = GetFileAttributes(path);
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
		return 0;

	int delay = m_specDelayMs;
	if (delay < 200)
		delay = 200;
	m_specDelayMs = delay;

	const GateChannelLimits& L = m_gateDefault;
	const GateSensorTempI2c& t = m_gateSensorTempI2c;
	const GateBadPixelDarkCfg& bp = m_gateBadPixelDark;
	const GateFirmwareBurnCfg& fw = m_gateFirmwareBurn;

	CStringA tplUtf8;
	const CString tplPath = GateSpecTemplatePath(path);
	if (!ReadFileBytesA(tplPath, tplUtf8))
	{
		CStringA pathA(path);
		msg("[GateSpec] template missing: %s (copy GateSpec.template.utf8 next to exe)\r\n",
			(LPCSTR)pathA);
		return 0;
	}
	if (tplUtf8.GetLength() >= 3
		&& (unsigned char)tplUtf8[0] == 0xEF
		&& (unsigned char)tplUtf8[1] == 0xBB
		&& (unsigned char)tplUtf8[2] == 0xBF)
	{
		tplUtf8 = tplUtf8.Mid(3);
	}

	CStringA v;
	v.Format("%d", delay);
	ReplaceTokenA(tplUtf8, "{{DELAY_MS}}", v);
	v.Format("%.3f", L.minSsrFps);
	ReplaceTokenA(tplUtf8, "{{MIN_SSR_FPS}}", v);
	v.Format("%.3f", L.maxSsrFps);
	ReplaceTokenA(tplUtf8, "{{MAX_SSR_FPS}}", v);
	v.Format("%.3f", L.minCurrent_mA);
	ReplaceTokenA(tplUtf8, "{{MIN_CUR_MA}}", v);
	v.Format("%.3f", L.maxCurrent_mA);
	ReplaceTokenA(tplUtf8, "{{MAX_CUR_MA}}", v);
	v.Format("%.3f", L.minSensorTemp_C);
	ReplaceTokenA(tplUtf8, "{{MIN_TEMP_C}}", v);
	v.Format("%.3f", L.maxSensorTemp_C);
	ReplaceTokenA(tplUtf8, "{{MAX_TEMP_C}}", v);
	v.Format("%d", t.enabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{TEMP_EN}}", v);
	v.Format("0x%02X", t.i2cAddr);
	ReplaceTokenA(tplUtf8, "{{TEMP_ADDR}}", v);
	v.Format("%u", t.i2cMode);
	ReplaceTokenA(tplUtf8, "{{TEMP_MODE}}", v);
	v.Format("0x%04X", t.regLow);
	ReplaceTokenA(tplUtf8, "{{TEMP_REGLO}}", v);
	v.Format("0x%04X", t.regHigh);
	ReplaceTokenA(tplUtf8, "{{TEMP_REGHI}}", v);
	v.Format("%.3f", t.coeffLow);
	ReplaceTokenA(tplUtf8, "{{TEMP_COEFFLO}}", v);
	v.Format("%.3f", t.coeffHigh);
	ReplaceTokenA(tplUtf8, "{{TEMP_COEFFHI}}", v);
	v.Format("%.3f", t.divisor);
	ReplaceTokenA(tplUtf8, "{{TEMP_DIV}}", v);
	v.Format("%.3f", t.offset);
	ReplaceTokenA(tplUtf8, "{{TEMP_OFF}}", v);
	v.Format("%d", bp.enabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_EN}}", v);
	v.Format("%d", bp.algoMode);
	ReplaceTokenA(tplUtf8, "{{BP_ALGO}}", v);
	v.Format("%d", bp.bayerPattern);
	ReplaceTokenA(tplUtf8, "{{BP_BAYER}}", v);
	v.Format("%d", bp.maxBadPixels);
	ReplaceTokenA(tplUtf8, "{{BP_MAX}}", v);
	v.Format("%d", bp.hotDelta);
	ReplaceTokenA(tplUtf8, "{{BP_HOTDELTA}}", v);
	v.Format("%d", bp.brightContrastCluster);
	ReplaceTokenA(tplUtf8, "{{BP_CL_TH}}", v);
	v.Format("%d", bp.clusterMinPixels);
	ReplaceTokenA(tplUtf8, "{{BP_CL_MIN}}", v);
	v.Format("%d", bp.singleDefectPermyriad);
	ReplaceTokenA(tplUtf8, "{{BP_PPM}}", v);
	v.Format("%d", bp.grGbToG ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_GRGB}}", v);
	v.Format("%d", bp.hotAbsMin);
	ReplaceTokenA(tplUtf8, "{{BP_ABS}}", v);
	v.Format("%d", bp.borderPx);
	ReplaceTokenA(tplUtf8, "{{BP_BORDER}}", v);
	v.Format("%d", bp.saveSnapshot ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_SAVE}}", v);
	v.Format("%d", bp.saveBmp ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_SAVE_BMP}}", v);
	v.Format("%d", bp.savePackedRaw ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_SAVE_PACKED}}", v);
	v.Format("%d", bp.saveUnpack12 ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_SAVE_U12}}", v);
	v.Format("%d", bp.saveUnpack10 ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{BP_SAVE_U10}}", v);
	{
		CStringA dirA;
#ifdef _UNICODE
		const int dirLen = (int)_tcslen(bp.saveDir);
		const int dirAcpLen = WideCharToMultiByte(CP_ACP, 0, bp.saveDir, dirLen, NULL, 0, NULL, NULL);
		if (dirAcpLen > 0)
		{
			LPSTR p = dirA.GetBuffer(dirAcpLen);
			WideCharToMultiByte(CP_ACP, 0, bp.saveDir, dirLen, p, dirAcpLen, NULL, NULL);
			dirA.ReleaseBuffer(dirAcpLen);
		}
#else
		dirA = bp.saveDir;
#endif
		ReplaceTokenA(tplUtf8, "{{BP_DIR}}", dirA);
	}
	v.Format("%d", fw.enabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_EN}}", v);
	v.Format("%d", fw.fovTypeIndex);
	ReplaceTokenA(tplUtf8, "{{FW_FOV_INDEX}}", v);
	{
		CStringA fovA(CT2A(FirmwareFovTypeName(fw.fovTypeIndex)));
		ReplaceTokenA(tplUtf8, "{{FW_FOV_TYPE}}", fovA);
	}
	{
		CStringA dirA(fw.flashDataDir);
		ReplaceTokenA(tplUtf8, "{{FW_FLASH_DIR}}", dirA);
	}
	v.Format("%d", fw.fwWarmupMs);
	ReplaceTokenA(tplUtf8, "{{FW_WARMUP_MS}}", v);
	v.Format("%d", fw.postBurnDelayMs);
	ReplaceTokenA(tplUtf8, "{{FW_POST_BURN_MS}}", v);
	v.Format("%d", fw.i2cRateKbps);
	ReplaceTokenA(tplUtf8, "{{FW_I2C_RATE}}", v);
	v.Format("%d", fw.autoDetectSlave ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_AUTO_SLAVE}}", v);
	v.Format("%d", fw.useMipiVcForBurn ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_USE_MIPI_VC}}", v);
	v.Format("%d", fw.fa132DualChip ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_FA132_DUAL_CHIP}}", v);
	v.Format("%d", fw.dualChipVcSplit);
	ReplaceTokenA(tplUtf8, "{{FW_DUAL_CHIP_VC_SPLIT}}", v);
	v.Format("%d", fw.powerCycleAfter ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_POWER_CYCLE}}", v);
	v.Format("%d", fw.verifyEnabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_VERIFY_EN}}", v);
	v.Format("%d", fw.verifyBeforeGrab ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_VERIFY_BEFORE_GRAB}}", v);
	v.Format("%d", fw.readSensorIdEnabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{FW_READ_SENSOR_ID}}", v);
	{
		CStringA grabA;
#ifdef _UNICODE
		const int grabLen = (int)_tcslen(fw.grabIniAfterPowerCycle);
		const int grabAcpLen = WideCharToMultiByte(CP_ACP, 0, fw.grabIniAfterPowerCycle, grabLen, NULL, 0, NULL, NULL);
		if (grabAcpLen > 0)
		{
			LPSTR p = grabA.GetBuffer(grabAcpLen);
			WideCharToMultiByte(CP_ACP, 0, fw.grabIniAfterPowerCycle, grabLen, p, grabAcpLen, NULL, NULL);
			grabA.ReleaseBuffer(grabAcpLen);
		}
#else
		grabA = fw.grabIniAfterPowerCycle;
#endif
		ReplaceTokenA(tplUtf8, "{{FW_GRAB_INI_AFTER_PC}}", grabA);
	}

	/* SaveGateSpecIni: [tcp_notify] tokens for GateSpec.template.utf8 */
	const GateTcpNotifyCfg& tn = m_gateTcpNotify;
	v.Format("%d", tn.enabled ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{TCP_EN}}", v);
	{
		CStringA hostA(tn.peerHost);
		ReplaceTokenA(tplUtf8, "{{TCP_HOST}}", hostA);
	}
	v.Format("%d", tn.peerPort);
	ReplaceTokenA(tplUtf8, "{{TCP_PORT}}", v);
	v.Format("%d", tn.onlyOnOverallOk ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{TCP_ONLY_OK}}", v);
	v.Format("%d", tn.connectTimeoutMs);
	ReplaceTokenA(tplUtf8, "{{TCP_CONN_MS}}", v);
	v.Format("%d", tn.sendTimeoutMs);
	ReplaceTokenA(tplUtf8, "{{TCP_SEND_MS}}", v);
	v.Format("%d", tn.retryCount);
	ReplaceTokenA(tplUtf8, "{{TCP_RETRY}}", v);
	v.Format("%d", tn.waitResponse ? 1 : 0);
	ReplaceTokenA(tplUtf8, "{{TCP_WAIT_RESP}}", v);
	v.Format("%d", tn.recvTimeoutMs);
	ReplaceTokenA(tplUtf8, "{{TCP_RECV_MS}}", v);

	const CStringA iniAcp = Utf8ToAcp(tplUtf8, tplUtf8.GetLength());
	if (!WriteAcpIniFile(path, iniAcp))
		return 0;

	for (int d = 0; d < kGateSpecIniDevSlots; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			CString key;
			key.Format(_T("D%d_V%d"), d, v);
			GateWriteHex(path, _T("sensor_temp_i2c_vc"), key, m_gateTempI2cAddr[d][v], 2);
		}
	}
	/* Drop legacy D8..D31 keys from older GateSpec.ini */
	for (int d = kGateSpecIniDevSlots; d < MAX_CC16 * MAX_DEV; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			CString key;
			key.Format(_T("D%d_V%d"), d, v);
			WritePrivateProfileString(_T("sensor_temp_i2c_vc"), key, NULL, path);
			CString sec;
			sec.Format(_T("D%d_V%d"), d, v);
			WritePrivateProfileString(sec, NULL, NULL, path);
		}
	}

	for (int d = 0; d < kGateSpecIniDevSlots; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			if (GateLimEqual(m_gatePerChannel[d][v], m_gateDefault))
			{
				CString sec;
				sec.Format(_T("D%d_V%d"), d, v);
				WritePrivateProfileString(sec, NULL, NULL, path);
			}
			else
			{
				CString sec;
				sec.Format(_T("D%d_V%d"), d, v);
				GateWriteDbl(path, sec, _T("MinSsrFps"), m_gatePerChannel[d][v].minSsrFps);
				GateWriteDbl(path, sec, _T("MaxSsrFps"), m_gatePerChannel[d][v].maxSsrFps);
				GateWriteDbl(path, sec, _T("MinCurrent_mA"), m_gatePerChannel[d][v].minCurrent_mA);
				GateWriteDbl(path, sec, _T("MaxCurrent_mA"), m_gatePerChannel[d][v].maxCurrent_mA);
				GateWriteDbl(path, sec, _T("MinSensorTemp_C"), m_gatePerChannel[d][v].minSensorTemp_C);
				GateWriteDbl(path, sec, _T("MaxSensorTemp_C"), m_gatePerChannel[d][v].maxSensorTemp_C);
			}
		}
	}
	return 1;
}

// Save DtCar app settings to INI
int DtCarFunction::SaveDtCarIni() {
	WriteIniFileString("sensor", "INI", m_strSensorIniPath, m_strDtCarIniPath);
	SaveChannelEnableIni();
	return 1;
}

void DtCarFunction::ClearLightGateResults()
{
	m_bLightGateHasResult = false;
	memset(m_bLightGatePass, 0, sizeof(m_bLightGatePass));
	m_bFirmwareBurnHasResult = false;
	m_bFirmwareBurnInProgress = false;
	memset(m_bFirmwareBurnPass, 0, sizeof(m_bFirmwareBurnPass));
	memset(m_fwBurnErrCode, 0, sizeof(m_fwBurnErrCode));
	m_bSensorIdHasResult = false;
	memset(m_bSensorIdReadOk, 0, sizeof(m_bSensorIdReadOk));
	memset(m_sensorIdHex, 0, sizeof(m_sensorIdHex));
	m_bFirmwareBurnVerifyHasResult = false;
	memset(m_bFirmwareBurnVerifyPass, 0, sizeof(m_bFirmwareBurnVerifyPass));
}

bool DtCarFunction::AnySensorIdReadFailed() const
{
	if (!m_gateFirmwareBurn.readSensorIdEnabled || !m_bSensorIdHasResult)
		return false;
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;
			if (!m_bSensorIdReadOk[d][v])
				return true;
		}
	}
	return false;
}

struct DtFirmwareBurnThreadParam
{
	DtCarFunction* fn;
	int devId;
	int vcId;
	GateFirmwareBurnCfg cfg;
	unsigned char slaveHint;
};

unsigned __stdcall DtCarFunction::FirmwareSensorIdThreadProc(void* p)
{
	DtFirmwareBurnThreadParam* tp = (DtFirmwareBurnThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	DtCarFunction* fn = tp->fn;
	const int dev = tp->devId;
	const int vc = tp->vcId;
	Sony031SensorIdResult sr = {};
	const bool ok = Sony031ReadSensorId(dev, vc, tp->cfg, tp->slaveHint, &sr);
	fn->m_bSensorIdReadOk[dev][vc] = ok;
	if (ok)
		_tcsncpy_s(fn->m_sensorIdHex[dev][vc], sr.sensorIdHex, _TRUNCATE);
	else
	{
		fn->m_sensorIdHex[dev][vc][0] = 0;
		fn->m_fwBurnErrCode[dev][vc] = sr.errorCode != 0 ? sr.errorCode : 2;
	}
	delete tp;
	return ok ? 0 : 1;
}

unsigned __stdcall DtCarFunction::FirmwareBurnThreadProc(void* p)
{
	DtFirmwareBurnThreadParam* tp = (DtFirmwareBurnThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	DtCarFunction* fn = tp->fn;
	const int dev = tp->devId;
	const int vc = tp->vcId;
	Sony031BurnResult br = {};
	const bool ok = Sony031FlashProgram(dev, vc, tp->cfg, tp->slaveHint, &br);
	fn->m_bFirmwareBurnPass[dev][vc] = ok;
	fn->m_fwBurnErrCode[dev][vc] = ok ? 0 : br.errorCode;
	if (!ok)
		msgUtf8(DtZh::kFwFail, dev, vc, br.errorCode);
	delete tp;
	return ok ? 0 : 1;
}

unsigned __stdcall DtCarFunction::FirmwareVerifyThreadProc(void* p)
{
	DtFirmwareBurnThreadParam* tp = (DtFirmwareBurnThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	DtCarFunction* fn = tp->fn;
	const int dev = tp->devId;
	const int vc = tp->vcId;
	Sony031VerifyResult vr = {};
	const bool ok = Sony031VerifyFlashCalibration(
		dev, vc, tp->cfg.fovTypeIndex, tp->cfg, tp->slaveHint, &vr);
	fn->m_bFirmwareBurnVerifyPass[dev][vc] = ok;
	delete tp;
	return ok ? 0 : 1;
}

/** Pump UI while waiting (WorkProc uses SendMessage to main HWND — avoids Stop deadlock). */
static void PumpUiWhileJoining()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/** Wait until WorkProc exits; only then CloseHandle. Uses MsgWait + UI pump. */
static bool JoinWorkThread(DtCarFunction* fn, int devId, DWORD warnAfterMs)
{
	if (fn == NULL || devId < 0 || devId >= MAX_CC16 * MAX_DEV)
		return true;
	HANDLE h = fn->m_hThread[devId];
	if (h == NULL)
		return true;

	const DWORD t0 = GetTickCount();
	bool warned = false;
	const DWORD sliceMs = 50;

	for (;;)
	{
		const DWORD w = MsgWaitForMultipleObjects(1, &h, FALSE, sliceMs, QS_ALLINPUT);
		if (w == WAIT_OBJECT_0)
		{
			const DWORD dt = GetTickCount() - t0;
			if (warned)
				msgUtf8(DtZh::kLogWorkJoin, devId, dt);
			CloseHandle(h);
			fn->m_hThread[devId] = NULL;
			return true;
		}
		if (w == WAIT_OBJECT_0 + 1)
			PumpUiWhileJoining();

		if (!warned && warnAfterMs > 0 && (GetTickCount() - t0) >= warnAfterMs)
		{
			msgUtf8(DtZh::kFwGrabPauseWarn, devId, (int)warnAfterMs);
			warned = true;
		}
	}
}

static void LogFirmwareBurnSummary(DtCarFunction* fn, bool allPass)
{
	if (fn == NULL)
		return;
	msgUtf8(allPass ? DtZh::kFwSummaryAllOk : DtZh::kFwSummaryNg);
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			const bool chOk = fn->m_bFirmwareBurnPass[d][v];
			msgUtf8(DtZh::kFwSummaryCh, d, v, chOk ? DtZh::kStrPass : DtZh::kStrFail);
		}
	}
}

bool DtCarFunction::PauseWorkThreadsForFirmwareBurn()
{
	if (!m_bRunning)
		return true;
	m_bPauseCaptureForBurn = true;
	m_bRunning = FALSE;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		::carGrabHold(i);
		if (m_workGrabReady[i])
		{
			::carUnitGrab(i);
			m_workGrabReady[i] = false;
		}
	}
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || m_hThread[i] == NULL)
			continue;
		JoinWorkThread(this, i, 3000);
	}
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		::carGrabHold(i);
	}
	return true;
}

static void MarkEnabledFirmwareBurnFailed(DtCarFunction* fn, int errCode = 1)
{
	if (fn == NULL)
		return;
	fn->m_bFirmwareBurnHasResult = true;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			fn->m_bFirmwareBurnPass[d][v] = false;
			fn->m_fwBurnErrCode[d][v] = errCode;
		}
	}
}

static void MarkDevFirmwareBurnFailed(DtCarFunction* fn, int devId, int errCode = 1)
{
	if (fn == NULL || devId < 0 || devId >= MAX_CC16 * MAX_DEV)
		return;
	fn->m_bFirmwareBurnHasResult = true;
	for (int v = 0; v < fn->m_iVcNum; v++)
	{
		if (!fn->IsVcEnabled(devId, v))
			continue;
		fn->m_bFirmwareBurnPass[devId][v] = false;
		fn->m_fwBurnErrCode[devId][v] = errCode;
	}
}

static void MarkVcFirmwareBurnFailed(DtCarFunction* fn, int devId, int vcId, int errCode = 1)
{
	if (fn == NULL || devId < 0 || devId >= MAX_CC16 * MAX_DEV
		|| vcId < 0 || vcId >= MAX_VC)
		return;
	if (!fn->IsVcEnabled(devId, vcId))
		return;
	fn->m_bFirmwareBurnHasResult = true;
	fn->m_bFirmwareBurnPass[devId][vcId] = false;
	fn->m_fwBurnErrCode[devId][vcId] = errCode;
}

static int FirmwareChipPhaseCount(const GateFirmwareBurnCfg& cfg)
{
	return cfg.fa132DualChip ? 2 : 1;
}

static bool VcOnFirmwareChipPhase(int vcId, int chipPhase, const GateFirmwareBurnCfg& cfg)
{
	if (!cfg.fa132DualChip)
		return chipPhase == 0;
	return FirmwareChipIdForVc(vcId, cfg) == chipPhase;
}

static void JoinFirmwareWorkerThreads(std::vector<HANDLE>& handles)
{
	for (size_t i = 0; i < handles.size(); i++)
	{
		if (handles[i] != NULL)
		{
			WaitForSingleObject(handles[i], INFINITE);
			CloseHandle(handles[i]);
		}
	}
	handles.clear();
}

bool DtCarFunction::RunFirmwareBurnParallel(bool afterStart)
{
	if (!m_gateFirmwareBurn.enabled)
		return true;

	TCHAR binFull[MAX_PATH] = {};
	if (!ResolveFirmwareBinPath(m_gateFirmwareBurn, binFull))
	{
		CStringA fovA(CT2A(FirmwareFovTypeName(m_gateFirmwareBurn.fovTypeIndex)));
		msgUtf8(DtZh::kFwBinReadFail, (LPCSTR)fovA);
		MarkEnabledFirmwareBurnFailed(this, 12);
		return true;
	}
	_tcsncpy_s(m_gateFirmwareBurn.binPath, binFull, _TRUNCATE);
	{
		CStringA pathA(binFull);
		msgUtf8(DtZh::kLogFwBin, (LPCSTR)pathA);
	}

	if (m_iEnumDevNum <= 0)
	{
		msgUtf8(DtZh::kFwNeedOpen);
		MarkEnabledFirmwareBurnFailed(this);
		return true;
	}
	bool captureWasRunning = (m_bRunning != FALSE);

	m_bFirmwareBurnHasResult = true;
	m_bFirmwareBurnInProgress = true;
	memset(m_bFirmwareBurnPass, 0, sizeof(m_bFirmwareBurnPass));
	memset(m_fwBurnErrCode, 0, sizeof(m_fwBurnErrCode));
	m_bSensorIdHasResult = false;
	memset(m_bSensorIdReadOk, 0, sizeof(m_bSensorIdReadOk));
	memset(m_sensorIdHex, 0, sizeof(m_sensorIdHex));

	FirmwareBurnSetProgressTarget(m_hwndFirmwareBurnProgress);

	if (afterStart)
	{
		if (!captureWasRunning)
		{
			msgUtf8(DtZh::kFwNeedStartFirst);
			FirmwareBurnSetProgressTarget(NULL);
			m_bFirmwareBurnInProgress = false;
			MarkEnabledFirmwareBurnFailed(this);
			return true;
		}
		PauseWorkThreadsForFirmwareBurn();
	}

	bool devBurnReady[MAX_CC16 * MAX_DEV] = {};
	const GateFirmwareBurnCfg& fwCfg = m_gateFirmwareBurn;
	const int chipPhases = FirmwareChipPhaseCount(fwCfg);
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		bool ready = true;
		if (!afterStart)
		{
			if (!InitWorkCapture(d))
			{
				msgUtf8(DtZh::kFwPrepDevInitFail, d);
				MarkDevFirmwareBurnFailed(this, d, 1);
				ready = false;
			}
			else if (!m_workGrabReady[d])
			{
				msgUtf8(DtZh::kFwPrepNotReady, d);
				MarkDevFirmwareBurnFailed(this, d, 1);
				ready = false;
			}
		}
		else if (!m_workGrabReady[d])
		{
			msgUtf8(DtZh::kLogFwGrabNotReady, d);
			MarkDevFirmwareBurnFailed(this, d, 1);
			ready = false;
		}
		if (!ready)
			continue;
		if (!FirmwareBurnSetupDevI2c(d, m_gateFirmwareBurn))
		{
			MarkDevFirmwareBurnFailed(this, d, 5);
			continue;
		}
		devBurnReady[d] = true;
	}

	if (m_gateFirmwareBurn.readSensorIdEnabled)
	{
		msgUtf8(DtZh::kFwSensorIdStart);
		m_bSensorIdHasResult = true;
		for (int chipPhase = 0; chipPhase < chipPhases; chipPhase++)
		{
			if (fwCfg.fa132DualChip)
				msgUtf8(DtZh::kLogFwChipPhaseSensorId, chipPhase);
			std::vector<HANDLE> idThreads;
			for (int d = 0; d < m_iEnumDevNum; d++)
			{
				if (!IsDevEnabled(d))
					continue;
				for (int v = 0; v < m_iVcNum; v++)
				{
					if (!IsVcEnabled(d, v))
						continue;
					if (!VcOnFirmwareChipPhase(v, chipPhase, fwCfg))
						continue;
					DtFirmwareBurnThreadParam* tp = new DtFirmwareBurnThreadParam;
					tp->fn = this;
					tp->devId = d;
					tp->vcId = v;
					tp->cfg = fwCfg;
					tp->slaveHint = m_gateTempI2cAddr[d][v];
					unsigned threadId = 0;
					HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &DtCarFunction::FirmwareSensorIdThreadProc, tp, 0, &threadId);
					if (h == NULL)
					{
						delete tp;
						msgUtf8(DtZh::kLogFwThreadFail, d, v);
						Sony031SensorIdResult sr = {};
						if (!Sony031ReadSensorId(d, v, fwCfg, m_gateTempI2cAddr[d][v], &sr))
						{
							m_bSensorIdReadOk[d][v] = false;
							m_fwBurnErrCode[d][v] = 2;
						}
						else
						{
							m_bSensorIdReadOk[d][v] = true;
							_tcsncpy_s(m_sensorIdHex[d][v], sr.sensorIdHex, _TRUNCATE);
						}
						continue;
					}
					idThreads.push_back(h);
				}
			}
			JoinFirmwareWorkerThreads(idThreads);
		}

		bool idAllPass = true;
		for (int d = 0; d < m_iEnumDevNum; d++)
		{
			if (!IsDevEnabled(d))
				continue;
			for (int v = 0; v < m_iVcNum; v++)
			{
				if (!IsVcEnabled(d, v))
					continue;
				if (!m_bSensorIdReadOk[d][v])
					idAllPass = false;
			}
		}
		if (!idAllPass)
			msgUtf8(DtZh::kFwSensorIdParallelFail);
	}
	else
	{
		msgUtf8(DtZh::kFwSensorIdSkipIni);
	}

	for (int chipPhase = 0; chipPhase < chipPhases; chipPhase++)
	{
		if (fwCfg.fa132DualChip)
			msgUtf8(DtZh::kLogFwChipPhaseBurn, chipPhase);
		std::vector<HANDLE> threads;
		for (int d = 0; d < m_iEnumDevNum; d++)
		{
			if (!IsDevEnabled(d) || !devBurnReady[d])
				continue;
			if (chipPhase == 0)
				msgUtf8(DtZh::kLogFwBurnDev, d, d);
			for (int v = 0; v < m_iVcNum; v++)
			{
				if (!IsVcEnabled(d, v))
					continue;
				if (!VcOnFirmwareChipPhase(v, chipPhase, fwCfg))
					continue;
				DtFirmwareBurnThreadParam* tp = new DtFirmwareBurnThreadParam;
				tp->fn = this;
				tp->devId = d;
				tp->vcId = v;
				tp->cfg = fwCfg;
				tp->slaveHint = m_gateTempI2cAddr[d][v];
				unsigned threadId = 0;
				HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &DtCarFunction::FirmwareBurnThreadProc, tp, 0, &threadId);
				if (h == NULL)
				{
					delete tp;
					msgUtf8(DtZh::kLogFwThreadFail, d, v);
					MarkVcFirmwareBurnFailed(this, d, v, 1);
					continue;
				}
				threads.push_back(h);
			}
		}
		JoinFirmwareWorkerThreads(threads);
	}

	FirmwareBurnSetProgressTarget(NULL);
	m_bFirmwareBurnInProgress = false;
	m_bPauseCaptureForBurn = false;

	bool allPass = true;
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;
			if (!m_bFirmwareBurnPass[d][v])
				allPass = false;
		}
	}

	LogFirmwareBurnSummary(this, allPass);
	/* Power-cycle after burn is handled in CDtSampleDlg (Stop / 2s / Start); do not duplicate here. */

	if (!allPass)
		msgUtf8(DtZh::kFwParallelFail);
	/* Per-VC results in m_bFirmwareBurnPass; always continue verify/light test. */
	return true;
}

static void LogFirmwareVerifySummary(DtCarFunction* fn, bool allPass)
{
	if (fn == NULL)
		return;
	msgUtf8(allPass ? DtZh::kFwVerifySummaryOk : DtZh::kFwVerifySummaryNg);
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			const bool chOk = fn->m_bFirmwareBurnVerifyPass[d][v];
			msgUtf8(DtZh::kFwVerifySummaryCh, d, v, chOk ? DtZh::kStrPass : DtZh::kStrFail);
		}
	}
}

bool DtCarFunction::PrepareForFirmwareVerify()
{
	if (m_iEnumDevNum <= 0)
	{
		msgUtf8(DtZh::kFwNeedOpen);
		return false;
	}

	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		UninitWorkCapture(i);
	}

	bool allOk = true;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || !m_grabTabValid[i])
			continue;
		const int iRet = ::carInitPower(i);
		msgUtf8(DtZh::kLogDevInitPower, i, iRet);
		if (iRet != DT_ERROR_OK)
		{
			allOk = false;
			m_workPowerReady[i] = false;
			m_workGrabReady[i] = false;
			continue;
		}
		m_workPowerReady[i] = true;
		m_workGrabReady[i] = false;
	}

	msgUtf8(DtZh::kFwVerifyPrepPower);
	return allOk;
}

bool DtCarFunction::RestoreWorkCaptureAfterVerify()
{
	m_bSuppressWorkDraw = false;
	bool allOk = true;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || !m_grabTabValid[i])
			continue;
		if (!InitWorkCapture(i))
			allOk = false;
	}
	return allOk;
}

bool DtCarFunction::RunFirmwareBurnVerifyAll()
{
	if (!m_gateFirmwareBurn.verifyEnabled)
		return true;

	if (m_iEnumDevNum <= 0)
	{
		msgUtf8(DtZh::kFwNeedOpen);
		return false;
	}

	m_bFirmwareBurnVerifyHasResult = true;
	memset(m_bFirmwareBurnVerifyPass, 0, sizeof(m_bFirmwareBurnVerifyPass));

	const GateFirmwareBurnCfg cfg = m_gateFirmwareBurn;
	{
		CStringA fovA(CT2A(FirmwareFovTypeName(cfg.fovTypeIndex)));
		msgUtf8(DtZh::kLogFwVerifyFov, (LPCSTR)fovA, cfg.fovTypeIndex);
	}

	const int chipPhases = FirmwareChipPhaseCount(cfg);
	for (int chipPhase = 0; chipPhase < chipPhases; chipPhase++)
	{
		if (cfg.fa132DualChip)
			msgUtf8(DtZh::kLogFwChipPhaseVerify, chipPhase);
		std::vector<HANDLE> threads;
		for (int d = 0; d < m_iEnumDevNum; d++)
		{
			if (!IsDevEnabled(d))
				continue;
			if (!FirmwareBurnSetupDevI2c(d, cfg))
			{
				for (int v = 0; v < m_iVcNum; v++)
				{
					if (!IsVcEnabled(d, v))
						continue;
					if (!VcOnFirmwareChipPhase(v, chipPhase, cfg))
						continue;
					m_bFirmwareBurnVerifyPass[d][v] = false;
				}
				continue;
			}
			if (chipPhase == 0)
				msgUtf8(DtZh::kLogFwVerifyDev, d, d);
			for (int v = 0; v < m_iVcNum; v++)
			{
				if (!IsVcEnabled(d, v))
					continue;
				if (!VcOnFirmwareChipPhase(v, chipPhase, cfg))
					continue;
				DtFirmwareBurnThreadParam* tp = new DtFirmwareBurnThreadParam;
				tp->fn = this;
				tp->devId = d;
				tp->vcId = v;
				tp->cfg = cfg;
				tp->slaveHint = m_gateTempI2cAddr[d][v];
				unsigned threadId = 0;
				HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &DtCarFunction::FirmwareVerifyThreadProc, tp, 0, &threadId);
				if (h == NULL)
				{
					delete tp;
					msgUtf8(DtZh::kLogFwVerifyThreadFail, d, v);
					m_bFirmwareBurnVerifyPass[d][v] = false;
					continue;
				}
				threads.push_back(h);
			}
		}
		JoinFirmwareWorkerThreads(threads);
	}

	bool allPass = true;
	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;
			if (!m_bFirmwareBurnVerifyPass[d][v])
				allPass = false;
		}
	}

	LogFirmwareVerifySummary(this, allPass);
	if (!allPass)
		msgUtf8(DtZh::kFwVerifyParallelFail);
	return true;
}

void DtCarFunction::UpdateGrayCacheFromGrab(int devId, int vcId, const DtImage_t& grabImg)
{
	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV || vcId < 0 || vcId >= MAX_VC)
		return;

	const GrabTab* pTab = m_grabTabValid[devId] ? &m_grabTab[devId] : NULL;
	std::vector<unsigned char> tmp;
	unsigned int w = 0;
	unsigned int h = 0;
	bool bayerRaster = false;
	const bool grayOk = TransformGrabToGray8(grabImg, pTab, devId, tmp, w, h, bayerRaster);

	VcGrayCache& slot = m_grayCache[devId][vcId];
	if (grayOk)
	{
		slot.pixels.swap(tmp);
		slot.width = w;
		slot.height = h;
		slot.bayerRaster = bayerRaster;
	}
	else
	{
		slot.pixels.clear();
		slot.width = grabImg.width;
		slot.height = grabImg.height;
		slot.bayerRaster = false;
	}

	const unsigned copyRaw = GrabPayloadBytes(grabImg);
	if (grabImg.data != NULL && copyRaw > 0)
	{
		DtImage_t meta = grabImg;
		FillSrcMetaFromGrabTab(meta, pTab);
		slot.rawPixels.assign(grabImg.data, grabImg.data + copyRaw);
		slot.rawFormat = grabImg.format;
		slot.rawFmt = meta.rawFmt;
		slot.yuvFmt = meta.yuvFmt;
		slot.rawDataSize = copyRaw;
		slot.rawRowStride = GrabRowStrideBytes(grabImg);
	}
	else
	{
		slot.rawPixels.clear();
		slot.rawDataSize = 0;
		slot.rawRowStride = 0;
	}

	slot.valid = (copyRaw > 0);
	if (slot.pending != 0)
		InterlockedExchange(&slot.pending, 0);
}

CString DtCarFunction::BuildProductionOutputDir(const CTime& time) const
{
	CString base;
	if (m_gateBadPixelDark.saveDir[0] != 0)
	{
		base = m_gateBadPixelDark.saveDir;
		base.Trim();
	}
	else
	{
		const int slash = m_strDtCarIniPath.ReverseFind(_T('\\'));
		if (slash >= 0)
			base = m_strDtCarIniPath.Left(slash + 1) + _T("log\\LightTest\\");
		else
			base = _T("log\\LightTest\\");
	}
	if (!base.IsEmpty() && base.Right(1) != _T("\\"))
		base += _T("\\");

	CString dir;
	dir.Format(_T("%s%04d%02d%02d\\"),
		(LPCTSTR)base,
		time.GetYear(), time.GetMonth(), time.GetDay());
	return dir;
}

void DtCarFunction::EnsureProductionSessionDir()
{
	if (!m_lightTestSessionDir.IsEmpty())
		return;
	const CTime sessionTime = CTime::GetCurrentTime();
	m_lightTestSessionDir = BuildProductionOutputDir(sessionTime);
	m_lightTestSessionTag.Format(_T("%04d%02d%02d_%02d%02d%02d"),
		sessionTime.GetYear(), sessionTime.GetMonth(), sessionTime.GetDay(),
		sessionTime.GetHour(), sessionTime.GetMinute(), sessionTime.GetSecond());
	::SHCreateDirectoryEx(NULL, m_lightTestSessionDir, NULL);
}

void DtCarFunction::WriteProductionReport(const std::vector<LightTestChannelRecord>& rows, bool allPass) const
{
	if (m_lightTestSessionDir.IsEmpty())
		return;

	CString csvPath = m_lightTestSessionDir + _T("Production_report.csv");
	if (WriteProductionReportCsv(csvPath, rows, allPass,
		m_strGateSpecIniPath, m_specDelayMs, m_strSensorIniPath))
	{
		CStringA csvA(csvPath);
		msgUtf8(DtZh::kLogProdCsvOk, csvA.GetString());
	}
	else
	{
		CStringA csvA(csvPath);
		msgUtf8(DtZh::kLogProdCsvFail, csvA.GetString());
	}
}

static void CopyChannelSensorId(const DtCarFunction* fn, int d, int v, LightTestChannelRecord& rec)
{
	if (fn == NULL)
		return;
	rec.sensorIdHex.Empty();
	if (fn->m_bSensorIdHasResult && fn->m_bSensorIdReadOk[d][v] && fn->m_sensorIdHex[d][v][0] != 0)
		rec.sensorIdHex = fn->m_sensorIdHex[d][v];
}

static void FillProductionFwFields(DtCarFunction* fn, int d, int v, LightTestChannelRecord& rec)
{
	if (fn == NULL)
		return;
	CopyChannelSensorId(fn, d, v, rec);
	rec.fwBurnEnabled = fn->m_gateFirmwareBurn.enabled;
	rec.fwBurnTested = fn->m_bFirmwareBurnHasResult;
	rec.okFwBurn = !rec.fwBurnEnabled || !rec.fwBurnTested || fn->m_bFirmwareBurnPass[d][v];
	rec.fwBurnErrCode = fn->m_fwBurnErrCode[d][v];
	rec.fwVerifyEnabled = fn->m_gateFirmwareBurn.verifyEnabled;
	rec.fwVerifyTested = fn->m_bFirmwareBurnVerifyHasResult;
	rec.okFwVerify = !rec.fwVerifyEnabled || !rec.fwVerifyTested || fn->m_bFirmwareBurnVerifyPass[d][v];
}

void DtCarFunction::BuildProductionRowsFromFirmware(int failStage,
	std::vector<LightTestChannelRecord>& outRows, bool& outAllPass) const
{
	outRows.clear();
	outAllPass = true;
	if (m_iEnumDevNum <= 0 || m_iVcNum <= 0)
		return;

	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;

			LightTestChannelRecord rec = {};
			rec.devId = d;
			rec.vcId = v;
			rec.measureSkipped = true;
			FillProductionFwFields(const_cast<DtCarFunction*>(this), d, v, rec);

			bool chPass = true;
			if (m_gateFirmwareBurn.readSensorIdEnabled && m_bSensorIdHasResult && !m_bSensorIdReadOk[d][v])
				chPass = false;
			if (rec.fwBurnEnabled && rec.fwBurnTested && !rec.okFwBurn)
				chPass = false;
			if (failStage == PROD_STAGE_VERIFY
				&& rec.fwVerifyEnabled && rec.fwVerifyTested && !rec.okFwVerify)
				chPass = false;

			rec.overallPass = chPass;
			int chStage = failStage;
			if (!chPass && m_gateFirmwareBurn.readSensorIdEnabled && m_bSensorIdHasResult
				&& !m_bSensorIdReadOk[d][v])
				chStage = PROD_STAGE_SENSOR_ID;
			rec.failStage = chPass ? PROD_STAGE_OK : chStage;
			if (!chPass)
				outAllPass = false;

			outRows.push_back(rec);
		}
	}
}

void DtCarFunction::ApplyProductionRowsToUi(const std::vector<LightTestChannelRecord>& rows)
{
	m_bLightGateHasResult = (rows.size() > 0);
	memset(m_bLightGatePass, 0, sizeof(m_bLightGatePass));
	for (size_t i = 0; i < rows.size(); i++)
	{
		const int d = rows[i].devId;
		const int v = rows[i].vcId;
		if (d >= 0 && d < MAX_CC16 * MAX_DEV && v >= 0 && v < MAX_VC)
			m_bLightGatePass[d][v] = rows[i].overallPass;
	}
}

static const char* ProductionFailStageZh(int stage)
{
	switch (stage)
	{
	case PROD_STAGE_BURN: return DtZh::kProdStageBurn;
	case PROD_STAGE_VERIFY: return DtZh::kProdStageVerify;
	case PROD_STAGE_LIGHT: return DtZh::kProdStageLight;
	case PROD_STAGE_SENSOR_ID: return DtZh::kProdStageSensorId;
	default: return DtZh::kStrOk;
	}
}

static void BuildEnabledChannelsSummaryA(const DtCarFunction* fn, CStringA& out)
{
	out.Empty();
	if (fn == NULL)
		return;
	bool any = false;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		CStringA devPart;
		devPart.Format("D%d:", d);
		bool devAny = false;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			if (devAny)
				devPart += ",";
			CStringA vcA;
			vcA.Format("V%d", v);
			devPart += vcA;
			devAny = true;
		}
		if (!devAny)
			continue;
		if (any)
			out += ";";
		out += devPart;
		any = true;
	}
	if (!any)
		out = "none";
}

void DtCarFunction::LogProductionRunStart()
{
	m_dwProductionRunStartTick = GetTickCount();
	m_bProductionRunActive = TRUE;

	const CTime now = CTime::GetCurrentTime();
	CStringA chA;
	BuildEnabledChannelsSummaryA(this, chA);

	const GateFirmwareBurnCfg& fw = m_gateFirmwareBurn;
	msgUtf8(DtZh::kProdRunBannerStart);
	msgUtf8(DtZh::kProdRunDetailStart,
		now.GetYear(), now.GetMonth(), now.GetDay(),
		now.GetHour(), now.GetMinute(), now.GetSecond(),
		(LPCSTR)chA,
		fw.enabled ? 1 : 0,
		fw.verifyEnabled ? 1 : 0,
		fw.readSensorIdEnabled ? 1 : 0,
		m_specDelayMs,
		fw.fwWarmupMs);
	if (m_strGateSpecIniPath.GetLength() > 0)
	{
		CStringA iniA(m_strGateSpecIniPath);
		msgUtf8(DtZh::kProdRunIniPath, (LPCSTR)iniA);
	}
}

void DtCarFunction::LogProductionRunEnd(int failStage, bool allPass)
{
	if (!m_bProductionRunActive)
		return;
	m_bProductionRunActive = FALSE;

	DWORD elapsedSec = 0;
	const DWORD nowTick = GetTickCount();
	if (m_dwProductionRunStartTick != 0 && nowTick >= m_dwProductionRunStartTick)
		elapsedSec = (nowTick - m_dwProductionRunStartTick) / 1000;

	int summaryStage = failStage;
	if (allPass)
		summaryStage = PROD_STAGE_OK;
	else if (summaryStage == PROD_STAGE_OK)
		summaryStage = PROD_STAGE_LIGHT;

	const char* resultS = allPass ? DtZh::kStrOk : DtZh::kStrNg;
	const char* stageS = ProductionFailStageZh(summaryStage);
	msgUtf8(DtZh::kProdRunBannerEnd, resultS, stageS, (int)elapsedSec);

	if (!m_lightTestSessionDir.IsEmpty())
	{
		CString csvPath = m_lightTestSessionDir + _T("Production_report.csv");
		CStringA csvA(csvPath);
		msgUtf8(DtZh::kProdRunCsvPath, (LPCSTR)csvA);
	}
}

bool DtCarFunction::FinalizeProductionRun(int failStage)
{
	std::vector<LightTestChannelRecord> rows;
	bool allPass = true;
	BuildProductionRowsFromFirmware(failStage, rows, allPass);
	return FinalizeProductionRun(failStage, rows, allPass);
}

bool DtCarFunction::FinalizeProductionRun(int failStage,
	const std::vector<LightTestChannelRecord>& rows, bool allPass)
{
	EnsureProductionSessionDir();
	ApplyProductionRowsToUi(rows);
	WriteProductionReport(rows, allPass);

	int summaryStage = failStage;
	if (allPass)
		summaryStage = PROD_STAGE_OK;
	else if (summaryStage == PROD_STAGE_OK)
		summaryStage = PROD_STAGE_LIGHT;

	if (allPass)
		msgUtf8(DtZh::kProdSummaryOk);
	else
		msgUtf8(DtZh::kProdSummaryNg, ProductionFailStageZh(summaryStage));

	LogProductionRunEnd(summaryStage, allPass);

	/* After CSV: async Play to PeerHost (NG still sends unless OnlyOnOverallOk=1) */
	TcpNotifyPostTestDoneAsync(m_gateTcpNotify, allPass);

	return allPass;
}

bool DtCarFunction::GrabFrameDirectOwned(int devId, int vcId,
	std::vector<unsigned char>& gray, unsigned int& outW, unsigned int& outH,
	std::vector<unsigned char>& rawCopy, IMAGE_FORMAT& imgFormat,
	RAW_FORMAT& rawFmt, YUV_FORMAT& yuvFmt, bool& bayerRaster)
{
	gray.clear();
	rawCopy.clear();
	outW = 0;
	outH = 0;
	imgFormat = FORMAT_RAW8;
	rawFmt = RAW_RGGB;
	yuvFmt = YUV_YCBYCR;
	bayerRaster = false;

	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV || vcId < 0 || vcId >= MAX_VC)
		return false;
	if (!m_bRunning || m_bSuppressWorkDraw)
	{
		msgUtf8(DtZh::kGrabDirectNotRun, devId, vcId);
		return false;
	}

	const GrabTab* pTab = m_grabTabValid[devId] ? &m_grabTab[devId] : NULL;

	DtImage_t grabImg = {};
	int grabVc = vcId;
	const int iRet = ::carGrabFrameDirect(&grabImg, &grabVc, devId);

	bool ok = false;
	EnterCriticalSection(&m_csGrab);
	if (iRet == DT_ERROR_OK && grabImg.data != NULL
		&& grabImg.width >= 1 && grabImg.height >= 1
		&& (grabVc == vcId || m_iVcNum <= 1))
	{
		const unsigned copyRaw = GrabPayloadBytes(grabImg);
		const unsigned rowStride = GrabRowStrideBytes(grabImg);

		if (copyRaw > 0)
			rawCopy.assign(grabImg.data, grabImg.data + copyRaw);

		imgFormat = grabImg.format;
		rawFmt = ResolveRawFmt(grabImg, pTab, m_gateBadPixelDark.bayerPattern);
		yuvFmt = ResolveYuvFmt(grabImg, pTab);

		msgUtf8(DtZh::kGrabOk, devId, vcId, (int)imgFormat, copyRaw, rowStride, (int)yuvFmt);
		if (IsRawBayerFormat(imgFormat) || imgFormat == FORMAT_MIPI_RAW12 || imgFormat == FORMAT_P12)
		{
			const int iniPat = (pTab != NULL) ? (pTab->sensor.outformat % 4) : -1;
			msgUtf8(DtZh::kGrabRawFmt, devId, vcId,
				(int)grabImg.rawFmt, iniPat, (int)rawFmt, m_gateBadPixelDark.bayerPattern);
		}

		ok = true;
		DtImage_t grabForGray = grabImg;
		grabForGray.rawFmt = rawFmt;
		const bool grayOk = TransformGrabToGray8(grabForGray, pTab, devId, gray, outW, outH, bayerRaster);
		if (!grayOk)
			msgUtf8(DtZh::kGrabGraySkip, devId, vcId, (int)imgFormat);

		{
			VcGrayCache& slot = m_grayCache[devId][vcId];
			if (grayOk)
				slot.pixels = gray;
			else
				slot.pixels.clear();
			slot.rawPixels = rawCopy;
			slot.width = grayOk ? outW : grabImg.width;
			slot.height = grayOk ? outH : grabImg.height;
			slot.rawFormat = imgFormat;
			slot.rawFmt = rawFmt;
			slot.yuvFmt = yuvFmt;
			slot.rawDataSize = (unsigned)rawCopy.size();
			slot.rawRowStride = rowStride;
			slot.bayerRaster = bayerRaster;
			slot.valid = !rawCopy.empty();
		}
	}

	LeaveCriticalSection(&m_csGrab);

	if (!ok)
	{
		msgUtf8(DtZh::kGrabFail, devId, vcId, iRet,
			(iRet == DT_ERROR_OK) ? (int)grabImg.format : -1,
			(iRet == DT_ERROR_OK) ? grabVc : -1);
	}
	return ok;
}

void DtCarFunction::SaveBadPixelSnapshots(int devId, int vcId, bool pass,
	const std::vector<unsigned char>& gray, unsigned int w, unsigned int h,
	CString* outBmpPath, CString* outRawUnpackedPath,
	const std::vector<unsigned char>* frameRaw, IMAGE_FORMAT frameFmt, RAW_FORMAT frameRawFmt,
	YUV_FORMAT frameYuvFmt)
{
	if (outBmpPath != NULL)
		outBmpPath->Empty();
	if (outRawUnpackedPath != NULL)
		outRawUnpackedPath->Empty();

	const GateBadPixelDarkCfg& snapCfg = m_gateBadPixelDark;
	if (!snapCfg.saveSnapshot || w < 1 || h < 1)
		return;
	const bool wantBmp = snapCfg.saveBmp;
	const bool wantPacked = snapCfg.savePackedRaw;
	const bool wantU12 = snapCfg.saveUnpack12;
	const bool wantU10 = snapCfg.saveUnpack10;
	if (!wantBmp && !wantPacked && !wantU12 && !wantU10)
		return;

	CString dir = m_lightTestSessionDir;
	if (dir.IsEmpty())
	{
		const CTime time = CTime::GetCurrentTime();
		dir = BuildProductionOutputDir(time);
	}
	::SHCreateDirectoryEx(NULL, dir, NULL);

	const LPCTSTR tag = pass ? _T("OK") : _T("NG");
	CString nameBase;
	if (!m_lightTestSessionTag.IsEmpty())
	{
		nameBase.Format(_T("Dev%d_VC%d_%s_%s"), devId, vcId, tag, (LPCTSTR)m_lightTestSessionTag);
	}
	else
	{
		const CTime time = CTime::GetCurrentTime();
		nameBase.Format(_T("Dev%d_VC%d_%s_%04d%02d%02d_%02d%02d%02d"),
			devId, vcId, tag,
			time.GetYear(), time.GetMonth(), time.GetDay(),
			time.GetHour(), time.GetMinute(), time.GetSecond());
	}

	std::vector<unsigned char> rawCopy;
	IMAGE_FORMAT imgFormat = FORMAT_RAW8;
	RAW_FORMAT rawPattern = RAW_RGGB;
	const char* rawSource = "cache";
	const GrabTab* pTab = m_grabTabValid[devId] ? &m_grabTab[devId] : NULL;

	if (frameRaw != NULL && !frameRaw->empty())
	{
		rawCopy = *frameRaw;
		imgFormat = frameFmt;
		rawPattern = frameRawFmt;
		rawSource = "carGrabFrameDirect";
	}
	else
	{
		EnterCriticalSection(&m_csGrab);
		const VcGrayCache& slot = m_grayCache[devId][vcId];
		if (slot.valid && !slot.rawPixels.empty())
		{
			rawCopy = slot.rawPixels;
			imgFormat = slot.rawFormat;
			rawPattern = slot.rawFmt;
		}
		LeaveCriticalSection(&m_csGrab);
	}

	if (rawCopy.empty())
	{
		msgUtf8(DtZh::kBpSnapNoData, devId, vcId);
		return;
	}

	{
		DtImage_t snapMeta = MakeOwnedSnapImage(imgFormat, rawPattern, frameYuvFmt, w, h,
			const_cast<unsigned char*>(rawCopy.data()), (unsigned)rawCopy.size());
		rawPattern = ResolveRawFmt(snapMeta, pTab, m_gateBadPixelDark.bayerPattern);
	}

	unsigned rowStride = (h > 0) ? (unsigned)(rawCopy.size() / h) : 0;
	{
		EnterCriticalSection(&m_csGrab);
		const VcGrayCache& slot = m_grayCache[devId][vcId];
		if (slot.rawRowStride > 0)
			rowStride = slot.rawRowStride;
		LeaveCriticalSection(&m_csGrab);
	}
	msgUtf8(DtZh::kBpSnapBegin, devId, vcId, (int)imgFormat,
		(unsigned)rawCopy.size(), rowStride, (int)rawPattern, rawSource);

	std::vector<unsigned char> p12Buf;
	std::vector<unsigned char> p12Owned;
	const char* p12Tag = NULL;
	bool haveP12 = false;
	if (imgFormat == FORMAT_MIPI_RAW12)
		haveP12 = MipiRaw12ToP12(rawCopy.data(), (unsigned)rawCopy.size(), w, h, rowStride, p12Buf, &p12Tag);
	else if (imgFormat == FORMAT_P12)
	{
		p12Buf = rawCopy;
		haveP12 = p12Buf.size() >= w * h * 2;
		p12Tag = "native_p12";
	}

	if (haveP12 && imgFormat != FORMAT_P12)
		p12Owned.swap(p12Buf);
	else if (haveP12 && imgFormat == FORMAT_P12)
		p12Owned = p12Buf;

	if (wantBmp)
	{
		CString fileBmp;
		fileBmp.Format(_T("%s%s.bmp"), (LPCTSTR)dir, (LPCTSTR)nameBase);
		const CStringA fileBmpA(fileBmp);
		bool bmpSaved = false;
		const unsigned char* p12ForBmp = NULL;
		unsigned p12BmpBytes = 0;
		if (haveP12)
		{
			if (!p12Owned.empty())
			{
				p12ForBmp = p12Owned.data();
				p12BmpBytes = (unsigned)p12Owned.size();
			}
			else if (!p12Buf.empty())
			{
				p12ForBmp = p12Buf.data();
				p12BmpBytes = (unsigned)p12Buf.size();
			}
		}
		if (p12ForBmp != NULL)
		{
			const char* bmpTag = NULL;
			if (SavePreviewBmpFromP12(p12ForBmp, p12BmpBytes, w, h, rawPattern, fileBmpA.GetString(), &bmpTag))
			{
				msgUtf8(DtZh::kBpSnapBmpP12, fileBmpA.GetString(), bmpTag != NULL ? bmpTag : "?");
				bmpSaved = true;
			}
		}
		if (!bmpSaved && imgFormat == FORMAT_RAW8 && rawCopy.size() >= w * h)
		{
			const char* bmpTag = NULL;
			if (SavePreviewBmpFromBayer8(rawCopy.data(), w, h, rawPattern, fileBmpA.GetString(), &bmpTag))
			{
				msgUtf8(DtZh::kBpSnapBmpP12, fileBmpA.GetString(), bmpTag != NULL ? bmpTag : "?");
				bmpSaved = true;
			}
		}
		if (!bmpSaved && IsYuvFormat(imgFormat) && rowStride >= w * 2)
		{
			std::vector<unsigned char> rgb;
			if (DtkitYuv422ToRgb24(rawCopy.data(), rowStride, w, h, frameYuvFmt, rgb)
				&& WriteRgb24BmpFile(fileBmpA.GetString(), rgb.data(), w, h))
			{
				msgUtf8(DtZh::kBpSnapBmpYuv, fileBmpA.GetString(), (int)frameYuvFmt);
				bmpSaved = true;
			}
		}
		if (!bmpSaved && !gray.empty() && WriteGray8BmpFile(fileBmpA.GetString(), gray.data(), w, h))
		{
			msgUtf8(DtZh::kBpSnapBmpGray, fileBmpA.GetString());
			bmpSaved = true;
		}
		if (bmpSaved)
		{
			if (outBmpPath != NULL)
				*outBmpPath = fileBmp;
		}
		else
			msgUtf8(DtZh::kBpSnapBmpFail, fileBmpA.GetString(), (unsigned)gray.size(), haveP12 ? 1 : 0);
	}

	if (wantPacked)
	{
		CString fileRawPacked;
		if (IsYuvFormat(imgFormat))
			fileRawPacked.Format(_T("%s%s_packed.yuv"), (LPCTSTR)dir, (LPCTSTR)nameBase);
		else
			fileRawPacked.Format(_T("%s%s_packed.raw"), (LPCTSTR)dir, (LPCTSTR)nameBase);
		const CStringA fileRawPackedA(fileRawPacked);
		try
		{
			CFile f;
			if (f.Open(fileRawPacked, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
			{
				f.Write(rawCopy.data(), (UINT)rawCopy.size());
				f.Close();
				msgUtf8(DtZh::kBpSnapPacked, fileRawPackedA.GetString(), (unsigned)rawCopy.size());
			}
		}
		catch (CFileException* e)
		{
			e->Delete();
		}
	}

	if ((wantU12 || wantU10) && !haveP12)
		msgUtf8(DtZh::kBpSnapP12Skip, devId, vcId, (int)imgFormat);

	if (wantU12 && haveP12)
	{
		CString fileRaw;
		fileRaw.Format(_T("%s%s_unpack12.raw"), (LPCTSTR)dir, (LPCTSTR)nameBase);
		const CStringA fileRawA(fileRaw);

		bool rawSaved = false;
		try
		{
			CFile f;
			if (f.Open(fileRaw, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
			{
				f.Write(p12Owned.data(), (UINT)p12Owned.size());
				f.Close();
				rawSaved = true;
			}
		}
		catch (CFileException* e)
		{
			e->Delete();
		}

		if (rawSaved)
		{
			msgUtf8(DtZh::kBpSnapP12Ok,
				p12Tag != NULL ? p12Tag : "?",
				fileRawA.GetString(), (unsigned)p12Owned.size(), MipiRaw12RowBytes(w));
			if (outRawUnpackedPath != NULL)
				*outRawUnpackedPath = fileRaw;
		}
		else
			msgUtf8(DtZh::kBpSnapP12Fail, fileRawA.GetString());
	}

	if (wantU10 && haveP12)
	{
		CString fileRaw10;
		fileRaw10.Format(_T("%s%s_unpack10.raw"), (LPCTSTR)dir, (LPCTSTR)nameBase);
		const CStringA fileRaw10A(fileRaw10);
		if (!p12Owned.empty() && p12Owned.size() >= w * h * 2)
		{
			const unsigned int pixels = w * h;
			const unsigned short* p16 = reinterpret_cast<const unsigned short*>(p12Owned.data());
			std::vector<unsigned short> u10(pixels);
			ConvertP12ToUnpack10(p16, u10.data(), pixels);
			try
			{
				CFile f;
				if (f.Open(fileRaw10, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
				{
					f.Write(u10.data(), (UINT)(pixels * sizeof(unsigned short)));
					f.Close();
					msgUtf8(DtZh::kBpSnapUnpack10, fileRaw10A.GetString(), (unsigned)(pixels * 2));
				}
			}
			catch (CFileException* e)
			{
				e->Delete();
			}
		}
	}

	msgUtf8(DtZh::kBpSnapDone, devId, vcId);
}

bool DtCarFunction::GrabFrameGray8(int devId, int vcId, std::vector<unsigned char>& gray, unsigned int& outW, unsigned int& outH)
{
	gray.clear();
	outW = 0;
	outH = 0;

	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV || vcId < 0 || vcId >= MAX_VC)
		return false;

	std::vector<unsigned char> rawCopy;
	IMAGE_FORMAT imgFormat = FORMAT_RAW8;
	RAW_FORMAT rawFmt = RAW_RGGB;
	YUV_FORMAT yuvFmt = YUV_YCBYCR;
	bool bayerRaster = false;
	if (GrabFrameDirectOwned(devId, vcId, gray, outW, outH, rawCopy, imgFormat, rawFmt, yuvFmt, bayerRaster))
		return true;

	if (!m_bRunning)
	{
		msgUtf8(DtZh::kGrabNotRun, devId, vcId);
		return false;
	}

	VcGrayCache& slot = m_grayCache[devId][vcId];
	InterlockedExchange(&slot.pending, 1);
	slot.valid = false;

	const DWORD deadline = GetTickCount() + 3000;
	while (GetTickCount() < deadline)
	{
		EnterCriticalSection(&m_csGrab);
		const bool ready = slot.valid && slot.pending == 0 && !slot.pixels.empty();
		if (ready)
		{
			gray = slot.pixels;
			outW = slot.width;
			outH = slot.height;
		}
		LeaveCriticalSection(&m_csGrab);
		if (ready)
		{
			InterlockedExchange(&slot.pending, 0);
			return true;
		}
		Sleep(10);
	}

	msgUtf8(DtZh::kGrabTimeout, devId, vcId);
	InterlockedExchange(&slot.pending, 0);
	return false;
}

bool DtCarFunction::RunDarkFieldBadPixelCheck(int devId, int vcId, BadPixelDarkResult* outResult,
	CString* outBmpPath, CString* outRawUnpackedPath)
{
	if (outResult != NULL)
		*outResult = BadPixelDarkResult();

	if (!m_gateBadPixelDark.enabled)
	{
		if (outResult != NULL)
		{
			outResult->analyzed = false;
			outResult->pass = true;
		}
		return true;
	}

	std::vector<unsigned char> gray;
	std::vector<unsigned char> frameRaw;
	unsigned int w = 0;
	unsigned int h = 0;
	IMAGE_FORMAT grabFmt = FORMAT_RAW8;
	RAW_FORMAT rawFmt = RAW_RGGB;
	YUV_FORMAT yuvFmt = YUV_YCBYCR;
	bool bayerRaster = false;
	if (!GrabFrameDirectOwned(devId, vcId, gray, w, h, frameRaw, grabFmt, rawFmt, yuvFmt, bayerRaster))
	{
		msgUtf8(DtZh::kBpGrabFail, devId, vcId);
		if (outResult != NULL)
		{
			outResult->analyzed = false;
			outResult->pass = false;
		}
		return false;
	}

	BadPixelDarkResult r = {};
	HotPixelHuaweiDetail hwDetail = {};
	bool huaweiP12Domain = false;
	const char* modeHint = (m_gateBadPixelDark.algoMode == 1) ? DtZh::kModeHuaweiP12 : DtZh::kModeNeighbor;
	if (m_gateBadPixelDark.algoMode == 1)
	{
		if (IsYuvFormat(grabFmt) || grabFmt == FORMAT_GRAY8 || grabFmt == FORMAT_G8)
		{
			r = AnalyzeHotPixelHuaweiMono8(gray.data(), w, h, m_gateBadPixelDark, &hwDetail);
			modeHint = DtZh::kModeHuaweiMono;
		}
		else if (grabFmt == FORMAT_MIPI_RAW12 || grabFmt == FORMAT_P12)
		{
			std::vector<unsigned char> p12;
			const char* p12Tag = NULL;
			bool haveP12 = false;
			DtImage_t grabMeta = MakeOwnedSnapImage(grabFmt, rawFmt, yuvFmt, w, h,
				frameRaw.data(), (unsigned)frameRaw.size());
			const unsigned rowStride = GrabRowStrideBytes(grabMeta);
			if (grabFmt == FORMAT_MIPI_RAW12)
				haveP12 = MipiRaw12ToP12(frameRaw.data(), (unsigned)frameRaw.size(), w, h, rowStride, p12, &p12Tag);
			else if (frameRaw.size() >= w * h * 2)
			{
				p12 = frameRaw;
				haveP12 = true;
				p12Tag = "native_p12";
			}
			if (haveP12 && p12.size() >= w * h * 2)
			{
				const unsigned short* p16 = reinterpret_cast<const unsigned short*>(p12.data());
				r = AnalyzeHotPixelHuaweiBayer16(p16, w, h, rawFmt, m_gateBadPixelDark, &hwDetail);
				huaweiP12Domain = true;
				modeHint = DtZh::kModeHuaweiP12;
				msgUtf8(DtZh::kBpHuaweiP12, devId, vcId, p12Tag ? p12Tag : "p12");
			}
			else
			{
				msgUtf8(DtZh::kBpUnpackFallback, devId, vcId);
				if (bayerRaster && gray.size() >= w * h)
					r = AnalyzeHotPixelHuaweiBayer8(gray.data(), w, h, rawFmt, m_gateBadPixelDark, &hwDetail);
				else
					r = AnalyzeDarkFieldHotPixels(gray.data(), w, h, m_gateBadPixelDark, bayerRaster);
				modeHint = DtZh::kModeNeighbor;
			}
		}
		else if (bayerRaster && gray.size() >= w * h)
		{
			r = AnalyzeHotPixelHuaweiBayer8(gray.data(), w, h, rawFmt, m_gateBadPixelDark, &hwDetail);
			modeHint = DtZh::kModeHuaweiP12;
		}
		else if (grabFmt == FORMAT_RAW8 && frameRaw.size() >= w * h)
		{
			r = AnalyzeHotPixelHuaweiBayer8(frameRaw.data(), w, h, rawFmt, m_gateBadPixelDark, &hwDetail);
			modeHint = DtZh::kModeHuaweiP12;
		}
		else
		{
			msgUtf8(DtZh::kBpAlgoFallback, devId, vcId);
			r = AnalyzeDarkFieldHotPixels(gray.data(), w, h, m_gateBadPixelDark, bayerRaster);
			modeHint = DtZh::kModeNeighbor;
		}
	}
	else
	{
		r = AnalyzeDarkFieldHotPixels(gray.data(), w, h, m_gateBadPixelDark, bayerRaster);
	}
	if (m_gateBadPixelDark.algoMode == 1)
		r.singleDefectCount = hwDetail.singleDefectCount;
	if (outResult != NULL)
		*outResult = r;

	const char* sceneHint = (r.frameMean > 128.0) ? DtZh::kSceneBright : "";
	if (m_gateBadPixelDark.algoMode != 1)
	{
		if (IsYuvFormat(grabFmt))
			modeHint = DtZh::kModeYuv;
		else if (bayerRaster)
			modeHint = DtZh::kModeNeighbor;
	}

	if (m_gateBadPixelDark.algoMode == 1 && r.analyzed)
	{
		msgUtf8(DtZh::kBpResultHw,
			devId, vcId, r.pass ? DtZh::kBpOk : DtZh::kBpNg,
			r.badCount, r.singleDefectCount, m_gateBadPixelDark.maxBadPixels,
			(unsigned)((w * h * (unsigned)m_gateBadPixelDark.singleDefectPermyriad) / 100000),
			r.centerRoiMean, r.frameMean, w, h, (int)r.failReason, modeHint, sceneHint);
		if (hwDetail.clusterR || hwDetail.clusterGr || hwDetail.clusterGb || hwDetail.clusterB || hwDetail.clusterG)
		{
			msgUtf8(DtZh::kBpClusters,
				hwDetail.clusterR, hwDetail.clusterGr, hwDetail.clusterGb,
				hwDetail.clusterB, hwDetail.clusterG);
		}
		const HotPixelHuaweiDetail* pHw = (m_gateBadPixelDark.algoMode == 1) ? &hwDetail : NULL;
		LogBadPixelNgReason(devId, vcId, r, m_gateBadPixelDark, true, huaweiP12Domain, pHw);
	}
	else
	{
		msgUtf8(DtZh::kBpResultNb,
			devId, vcId, r.pass ? DtZh::kBpOk : DtZh::kBpNg,
			r.badCount, m_gateBadPixelDark.maxBadPixels,
			r.frameMean, m_gateBadPixelDark.hotDelta, m_gateBadPixelDark.hotAbsMin, w, h,
			modeHint, sceneHint);
		LogBadPixelNgReason(devId, vcId, r, m_gateBadPixelDark, false, false, NULL);
	}

	SaveBadPixelSnapshots(devId, vcId, r.pass, gray, w, h, outBmpPath, outRawUnpackedPath,
		&frameRaw, grabFmt, rawFmt, yuvFmt);

	return r.pass;
}

struct DtLightGateThreadParam
{
	DtCarFunction* fn;
	int devId;
	int vcId;
	bool pass;
	LightTestChannelRecord rec;
	CString line;
};

static bool RunLightGateOneChannel(DtCarFunction* fn, int d, int v,
	LightTestChannelRecord& rec, CString& line)
{
	if (fn == NULL)
		return false;

	const GateChannelLimits& L = fn->m_gatePerChannel[d][v];
	GateSensorTempI2c tempCfg = fn->m_gateSensorTempI2c;
	tempCfg.i2cAddr = fn->m_gateTempI2cAddr[d][v];
	double tempC = 0.0;
	const bool hasTemp = ReadSensorTempC(d, v, tempCfg, fn->m_gateFirmwareBurn, &tempC);

	::carGetChannelData(&fn->m_tVcData[d][v], v, d);
	const double ssr = fn->m_tVcData[d][v].dSsrFrameRate;
	const double cur_mA = fn->m_tVcData[d][v].iCurrent / 1000000.0;
	const bool okSsr = (ssr >= L.minSsrFps) && (ssr <= L.maxSsrFps);
	const bool okCur = (cur_mA >= L.minCurrent_mA) && (cur_mA <= L.maxCurrent_mA);
	bool okTemp = true;
	if (tempCfg.enabled)
	{
		okTemp = hasTemp
			&& (tempC >= L.minSensorTemp_C) && (tempC <= L.maxSensorTemp_C);
	}

	bool okBadPx = true;
	BadPixelDarkResult bpRes = {};
	CString bmpPath;
	CString rawPath;
	if (fn->m_gateBadPixelDark.enabled)
		okBadPx = fn->RunDarkFieldBadPixelCheck(d, v, &bpRes, &bmpPath, &rawPath);

	const bool okFwBurn = (!fn->m_gateFirmwareBurn.enabled || !fn->m_bFirmwareBurnHasResult
		|| fn->m_bFirmwareBurnPass[d][v]);
	const bool okFwVerify = (!fn->m_gateFirmwareBurn.verifyEnabled
		|| !fn->m_bFirmwareBurnVerifyHasResult
		|| fn->m_bFirmwareBurnVerifyPass[d][v]);
	const bool okFw = okFwBurn && okFwVerify;
	const bool pass = okSsr && okCur && okTemp && okBadPx && okFw;

	fn->m_bLightGatePass[d][v] = pass;

	rec = {};
	rec.devId = d;
	rec.vcId = v;
	rec.overallPass = pass;
	rec.ssrFps = ssr;
	rec.ssrMin = L.minSsrFps;
	rec.ssrMax = L.maxSsrFps;
	rec.okSsr = okSsr;
	rec.current_mA = cur_mA;
	rec.curMin = L.minCurrent_mA;
	rec.curMax = L.maxCurrent_mA;
	rec.okCur = okCur;
	rec.hasTemp = hasTemp;
	rec.tempC = tempC;
	rec.tempMin = L.minSensorTemp_C;
	rec.tempMax = L.maxSensorTemp_C;
	rec.okTemp = okTemp;
	rec.badPixelEnabled = fn->m_gateBadPixelDark.enabled;
	rec.badAnalyzed = bpRes.analyzed;
	rec.okBadPx = okBadPx;
	rec.badCount = bpRes.badCount;
	rec.badMaxAllow = fn->m_gateBadPixelDark.maxBadPixels;
	rec.frameMean = bpRes.frameMean;
	rec.imageBmp = bmpPath;
	rec.imageRawUnpacked = rawPath;
	rec.measureSkipped = false;
	rec.failStage = pass ? PROD_STAGE_OK : PROD_STAGE_LIGHT;
	FillProductionFwFields(fn, d, v, rec);

	const TCHAR* fwTag = _T("N/A");
	if (fn->m_gateFirmwareBurn.enabled && fn->m_bFirmwareBurnHasResult)
		fwTag = okFw ? _T("OK") : _T("NG");
	if (hasTemp)
	{
		line.Format(_T("Dev%d VC%d: %s | Fw=%s | SsrFps=%.3f [%.3f,%.3f] | Cur=%.3f mA [%.3f,%.3f] | Temp=%.1f C [%.1f,%.1f] | BadPx=%s(%u)"),
			d, v, pass ? _T("OK") : _T("NG"), fwTag,
			ssr, L.minSsrFps, L.maxSsrFps,
			cur_mA, L.minCurrent_mA, L.maxCurrent_mA,
			tempC, L.minSensorTemp_C, L.maxSensorTemp_C,
			bpRes.analyzed ? (bpRes.pass ? _T("OK") : _T("NG")) : _T("N/A"),
			bpRes.badCount);
	}
	else
	{
		line.Format(_T("Dev%d VC%d: %s | Fw=%s | SsrFps=%.3f [%.3f,%.3f] | Cur=%.3f mA [%.3f,%.3f] | Temp=N/A | BadPx=%s(%u)"),
			d, v, pass ? _T("OK") : _T("NG"), fwTag,
			ssr, L.minSsrFps, L.maxSsrFps,
			cur_mA, L.minCurrent_mA, L.maxCurrent_mA,
			bpRes.analyzed ? (bpRes.pass ? _T("OK") : _T("NG")) : _T("N/A"),
			bpRes.badCount);
	}
	return pass;
}

unsigned __stdcall DtCarFunction::LightGateThreadProc(void* p)
{
	DtLightGateThreadParam* tp = (DtLightGateThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	tp->pass = RunLightGateOneChannel(tp->fn, tp->devId, tp->vcId, tp->rec, tp->line);
	return tp->pass ? 0 : 1;
}

static int LightGateParamOrder(const void* a, const void* b)
{
	const DtLightGateThreadParam* pa = *(const DtLightGateThreadParam* const*)a;
	const DtLightGateThreadParam* pb = *(const DtLightGateThreadParam* const*)b;
	if (pa->devId != pb->devId)
		return (pa->devId < pb->devId) ? -1 : 1;
	if (pa->vcId != pb->vcId)
		return (pa->vcId < pb->vcId) ? -1 : 1;
	return 0;
}

static void LogLightGateChannelLine(DtCarFunction* fn, const LightTestChannelRecord& r)
{
	if (fn == NULL)
		return;
	const char* passS = r.overallPass ? DtZh::kStrPass : DtZh::kStrFail;
	const char* fwS = DtZh::kStrNa;
	if (fn->m_gateFirmwareBurn.enabled && fn->m_bFirmwareBurnHasResult)
		fwS = fn->m_bFirmwareBurnPass[r.devId][r.vcId] ? DtZh::kStrOk : DtZh::kStrNg;
	const char* bpS = DtZh::kStrNa;
	if (r.badPixelEnabled)
		bpS = r.badAnalyzed ? (r.okBadPx ? DtZh::kStrOk : DtZh::kStrNg) : DtZh::kStrNa;

	char buf[720] = {};
	if (r.hasTemp)
	{
		_snprintf_s(buf, _TRUNCATE, DtZh::kLtChFmtTemp,
			r.devId, r.vcId, passS, fwS,
			r.ssrFps, r.ssrMin, r.ssrMax,
			r.current_mA, r.curMin, r.curMax,
			r.tempC, r.tempMin, r.tempMax,
			bpS, r.badCount);
	}
	else
	{
		_snprintf_s(buf, _TRUNCATE, DtZh::kLtChFmtNoTemp,
			r.devId, r.vcId, passS, fwS,
			r.ssrFps, r.ssrMin, r.ssrMax,
			r.current_mA, r.curMin, r.curMax,
			bpS, r.badCount);
	}
	msgUtf8(DtZh::kLogLtLine, buf);
}

bool DtCarFunction::RunLightGatePerChannelReport()
{
	ReadGateSpecIni();

	CStringA specA(m_strGateSpecIniPath);
	msgUtf8(DtZh::kLogLtIni, specA.GetString());
	msgUtf8(DtZh::kLogLtDelay,
		m_specDelayMs,
		m_gateDefault.minSsrFps, m_gateDefault.maxSsrFps,
		m_gateDefault.minCurrent_mA, m_gateDefault.maxCurrent_mA,
		m_gateDefault.minSensorTemp_C, m_gateDefault.maxSensorTemp_C);
	if (m_gateSensorTempI2c.enabled)
	{
		msgUtf8(DtZh::kLogLtTempI2c,
			m_gateSensorTempI2c.i2cMode,
			m_gateSensorTempI2c.regLow, m_gateSensorTempI2c.regHigh,
			m_gateSensorTempI2c.coeffLow, m_gateSensorTempI2c.coeffHigh,
			m_gateSensorTempI2c.divisor, m_gateSensorTempI2c.offset);
	}
	if (m_gateBadPixelDark.enabled)
	{
		msgUtf8(DtZh::kLogLtBpOn,
			m_gateBadPixelDark.algoMode,
			m_gateBadPixelDark.maxBadPixels,
			m_gateBadPixelDark.hotDelta,
			m_gateBadPixelDark.brightContrastCluster,
			m_gateBadPixelDark.clusterMinPixels,
			m_gateBadPixelDark.singleDefectPermyriad);
	}

	if (m_iEnumDevNum <= 0 || m_iVcNum <= 0)
	{
		msgUtf8(DtZh::kLogLtSkip);
		m_bLightGateHasResult = false;
		return false;
	}

	m_bLightGateHasResult = true;
	memset(m_bLightGatePass, 0, sizeof(m_bLightGatePass));

	EnsureProductionSessionDir();

	std::vector<HANDLE> threads;
	std::vector<DtLightGateThreadParam*> params;
	bool loggedDev[MAX_CC16 * MAX_DEV] = {};

	for (int d = 0; d < m_iEnumDevNum; d++)
	{
		if (!IsDevEnabled(d))
			continue;
		if (!loggedDev[d])
		{
			msgUtf8(DtZh::kLtParallelDev, d);
			loggedDev[d] = true;
		}
		for (int v = 0; v < m_iVcNum; v++)
		{
			if (!IsVcEnabled(d, v))
				continue;
			DtLightGateThreadParam* tp = new DtLightGateThreadParam;
			tp->fn = this;
			tp->devId = d;
			tp->vcId = v;
			tp->pass = false;
			unsigned threadId = 0;
			HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &DtCarFunction::LightGateThreadProc, tp, 0, &threadId);
			if (h == NULL)
			{
				msgUtf8(DtZh::kLogLtThreadFail, d, v);
				tp->pass = RunLightGateOneChannel(this, d, v, tp->rec, tp->line);
				params.push_back(tp);
				continue;
			}
			threads.push_back(h);
			params.push_back(tp);
		}
	}

	for (size_t i = 0; i < threads.size(); i++)
	{
		WaitForSingleObject(threads[i], INFINITE);
		CloseHandle(threads[i]);
	}

	if (!params.empty())
		qsort(params.data(), params.size(), sizeof(DtLightGateThreadParam*), LightGateParamOrder);

	bool allPass = true;
	std::vector<LightTestChannelRecord> reportRows;
	reportRows.reserve(params.size());
	for (size_t i = 0; i < params.size(); i++)
	{
		DtLightGateThreadParam* tp = params[i];
		if (tp == NULL)
			continue;
		if (!tp->pass)
			allPass = false;
		reportRows.push_back(tp->rec);
		LogLightGateChannelLine(this, tp->rec);
		delete tp;
	}

	FinalizeProductionRun(PROD_STAGE_LIGHT, reportRows, allPass);
	if (!allPass)
		msgUtf8(DtZh::kLtParallelFail);
	return allPass;
}

// Worker thread entry
UINT __stdcall WorkThread(LPVOID param)
{
	DtWorkThreadParam* p = (DtWorkThreadParam*)param;
	if (p != NULL && p->fn != NULL)
		p->fn->WorkProc(p->devId);
	delete p;
	return 0;
}

// Pick sensor INI via file dialog
int DtCarFunction::LoadIni() {
	CString strFileName = _T("");
	CString strFilter = "Ini File(*.ini)|*.ini";
	CFileDialog dlg(TRUE, NULL, strFileName, OFN_HIDEREADONLY, strFilter);

	if (dlg.DoModal() == IDOK)
	{
		m_strSensorIniPath = dlg.GetPathName();
		ReadGateSpecIni();
		SaveDtCarIni();
		return 1;
	}
	return 0;
}

int DtCarFunction::Enum() {
	m_iEnumDevNum = 0;
	m_iVcNum = 1;
	::carEnumerateDevice(m_cDeviceName, MAX_CC16 * MAX_DEV, &m_iEnumDevNum);
	// Temp copy of enumerated names for prefix checks
	CString strTemp[MAX_CC16 * MAX_DEV];
	const int inum = m_iEnumDevNum;
	char* keptNames[MAX_CC16 * MAX_DEV] = {};
	int keptCount = 0;
	for (int i = 0; i < inum; i++)
	{
		strTemp[i] = (m_cDeviceName[i] != NULL) ? m_cDeviceName[i] : "";
		if (strTemp[i].IsEmpty())
			continue;

		bool known = false;
		if (strncmp(strTemp[i].Mid(0, 4), "CC16", 4) == 0) {
				m_iVcNum = 4;
				m_iBoxType = Box_CC16;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "DF104", 5) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_DF104;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "DF108", 5) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_DF108;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "FA132", 5) == 0)
			{
				m_iVcNum = 4;
				m_iBoxType = Box_FA132;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 4), "FA32", 4) == 0) // FA32: same as FA132, 4 VC
			{
				m_iVcNum = 4;
				m_iBoxType = Box_FA132;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "FM101", 5) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_FM101;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 3), "GQ2", 3) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_GQ2;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 3), "GQ4", 3) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_GQ4;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "UC930", 5) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_UC930;
				known = true;
			}
			else if (strncmp(strTemp[i].Mid(0, 5), "UC980", 5) == 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_UC930;
				known = true;
			}
			else if (strTemp[i].Find(_T("UC980")) >= 0
				|| strTemp[i].Find(_T("UC930")) >= 0
				|| strTemp[i].Find(_T("MU960")) >= 0
				|| strTemp[i].Find(_T("UF920")) >= 0)
			{
				m_iVcNum = 1;
				m_iBoxType = Box_UC930;
				known = true;
			}

		if (known)
		{
			keptNames[keptCount] = m_cDeviceName[i];
			msg("%s  (Dev%d, VC=%d)\r\n", (LPCSTR)CStringA(strTemp[i]), keptCount, m_iVcNum);
			keptCount++;
		}
		else
		{
			msg("Skip unknown device name: %s\r\n", (LPCSTR)CStringA(strTemp[i]));
		}
	}
	for (int i = 0; i < keptCount; i++)
		m_cDeviceName[i] = keptNames[i];
	for (int i = keptCount; i < MAX_CC16 * MAX_DEV; i++)
		m_cDeviceName[i] = NULL;
	m_iEnumDevNum = keptCount;
	if (m_iEnumDevNum <= 0)
	{
		msg("No test box detected.\n");
		return -1;
	}
	LoadChannelEnableIni();
	if (!HasAnyChannelEnabled())
		ApplyChannelEnableDefaultsAfterEnum();
	return 1;
}


// Open test box devices
int DtCarFunction::Open() {
	if (!HasAnyChannelEnabled())
	{
		msgUtf8(DtZh::kNoChannelEnabled);
		return 0;
	}
	if (m_strSensorIniPath.IsEmpty())
	{
		msgUtf8(DtZh::kSensorIniEmpty);
		return 0;
	}
	if (::GetFileAttributes(m_strSensorIniPath) == INVALID_FILE_ATTRIBUTES)
	{
		CStringA pathA(m_strSensorIniPath);
		msgUtf8(DtZh::kSensorIniMissing, (LPCSTR)pathA);
		return 0;
	}

	int iRet = 0;
	iRet = ::carSetDeviceType(m_iEnumDevNum, (Box_Type)m_iBoxType);

	bool devOpened[MAX_CC16 * MAX_DEV] = {};
	memset(devOpened, 0, sizeof(devOpened));
	int openedCount = 0;

	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		m_grabTabValid[i] = false;
		if (!IsDevEnabled(i))
		{
			msg("DevID %d: skipped (disabled in channel selection)\n", i);
			continue;
		}
		GrabTab pGrabTab;
		msgUtf8(DtZh::kLogGrabTab, i, (int)sizeof(GrabTab));
		iRet = ::carLoadGrabPara(m_strSensorIniPath.GetBuffer(), i);
		m_strSensorIniPath.ReleaseBuffer();
		if (iRet != DT_ERROR_OK)
		{
			msgUtf8(DtZh::kSensorIniLoadFail, i);
			continue;
		}
		iRet = ::carGetGrabPara(&pGrabTab, i);
		if (iRet == DT_ERROR_OK)
		{
			m_grabTab[i] = pGrabTab;
			m_grabTabValid[i] = true;
			msgUtf8(DtZh::kLogSensorFmt, i, (unsigned)pGrabTab.sensor.outformat);
		}
		else
		{
			msgUtf8(DtZh::kSensorIniLoadFail, i);
			continue;
		}

		int iDevID = 0;
		iRet = ::carOpenDevice(m_cDeviceName[i], &iDevID, i);
		if (iRet == DT_ERROR_OK)
		{
			devOpened[i] = true;
			openedCount++;
			msgUtf8(DtZh::kLogOpenOk, i, m_cDeviceName[i]);
		}
		else
			msgUtf8(DtZh::kLogOpenFail, i, m_cDeviceName[i], iRet);
	}

	if (openedCount <= 0)
	{
		msgUtf8(DtZh::kOpenNoDevReady);
		return 0;
	}

	CommonSetting_t tCommonSetting;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!devOpened[i])
			continue;
		iRet = ::carCommonInit(tCommonSetting, i);
	}
	return 1;
}

int DtCarFunction::Close() {
	int iRet = 0;

	ResetPreviewDisplay();
	for (int i = 0; i < m_iEnumDevNum; i++)
		UninitWorkCapture(i);

	CommonSetting_t tCommonSetting;
	/* Close every enumerated Dev index (ignore channel mask). After channel
	   selection changes, a disabled Dev may still be open in the SDK. */
	for (int i = 0; i < m_iEnumDevNum; i++)
		iRet = ::carCommonUinit(tCommonSetting, i);

	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		iRet = ::carCloseDevice(i);
		if (iRet == DT_ERROR_OK)
			msgUtf8(DtZh::kLogCloseOk, i, m_cDeviceName[i] ? m_cDeviceName[i] : "");
		else if (iRet != DT_ERROR_FAILED)
			msgUtf8(DtZh::kLogCloseFail, i,
				m_cDeviceName[i] ? m_cDeviceName[i] : "", iRet);
	}
	return 1;
}

bool DtCarFunction::InitWorkCapture(int devId)
{
	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV)
		return false;
	if (m_workPowerReady[devId] && m_workGrabReady[devId])
		return true;

	int iRet = ::carInitPower(devId);
	msgUtf8(DtZh::kLogDevInitPower, devId, iRet);
	if (iRet != DT_ERROR_OK)
		return false;
	m_workPowerReady[devId] = true;

	iRet = ::carInitGrab(devId);
	msgUtf8(DtZh::kLogDevInitGrab, devId, iRet);
	if (iRet != DT_ERROR_OK)
	{
		UninitWorkCapture(devId);
		return false;
	}
	m_workGrabReady[devId] = true;
	return true;
}

void DtCarFunction::UninitWorkCapture(int devId)
{
	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV)
		return;
	if (m_workGrabReady[devId])
	{
		::carUnitGrab(devId);
		m_workGrabReady[devId] = false;
	}
	if (m_workPowerReady[devId])
	{
		::carUnitPower(devId);
		m_workPowerReady[devId] = false;
	}
}

bool DtCarFunction::ReloadGrabParaAfterPowerCycle()
{
	const TCHAR* iniPath = m_gateFirmwareBurn.grabIniAfterPowerCycle;
	if (iniPath == NULL || iniPath[0] == 0)
	{
		msgUtf8(DtZh::kFwGrabIniAfterPcSkip);
		return true;
	}
	if (::GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
	{
		CStringA pathA(iniPath);
		msgUtf8(DtZh::kFwGrabIniAfterPcMissing, (LPCSTR)pathA);
		return false;
	}

	CStringA pathLog(iniPath);
	msgUtf8(DtZh::kFwGrabIniReloadBegin, (LPCSTR)pathLog);

	int okCount = 0;
	int failCount = 0;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		if (m_workGrabReady[i])
		{
			::carUnitGrab(i);
			m_workGrabReady[i] = false;
		}
		const int iRet = ::carLoadGrabPara(iniPath, i);
		if (iRet != DT_ERROR_OK)
		{
			msgUtf8(DtZh::kFwGrabIniReloadDevFail, i, iRet);
			m_grabTabValid[i] = false;
			failCount++;
			continue;
		}
		GrabTab tab = {};
		const int gRet = ::carGetGrabPara(&tab, i);
		if (gRet != DT_ERROR_OK)
		{
			msgUtf8(DtZh::kFwGrabIniReloadDevFail, i, gRet);
			m_grabTabValid[i] = false;
			failCount++;
			continue;
		}
		m_grabTab[i] = tab;
		m_grabTabValid[i] = true;
		okCount++;
		msgUtf8(DtZh::kFwGrabIniReloadDevOk, i);
	}

	if (okCount <= 0)
		return false;
	if (failCount == 0)
		msgUtf8(DtZh::kFwGrabIniReloadOk, okCount);
	return true;
}

// Burn prep: power/grab init on UI thread, no WorkProc (no YUV before flash).
void DtCarFunction::RequestStopCapture()
{
	m_bSuppressWorkDraw = true;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		::carGrabHold(i);
		/* UnitGrab while WorkProc may be blocked in carGrabFrameDirect (~15s SDK wait).
		   GrabHold alone does not cancel an in-flight Direct grab. */
		if (m_workGrabReady[i])
		{
			::carUnitGrab(i);
			m_workGrabReady[i] = false;
			msgUtf8(DtZh::kLogWorkRequestStop, i);
		}
	}
}

struct DtFirmwarePrepThreadParam
{
	DtCarFunction* fn;
	int devId;
	volatile bool ok;
};

static unsigned __stdcall FirmwarePrepDevThreadProc(void* p)
{
	DtFirmwarePrepThreadParam* tp = (DtFirmwarePrepThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	tp->ok = tp->fn->InitWorkCapture(tp->devId);
	return tp->ok ? 0 : 1;
}

int DtCarFunction::StartFirmwarePrep()
{
	m_bRunning = FALSE;
	m_bPauseCaptureForBurn = false;
	m_bSuppressWorkDraw = false;
	for (int i = 0; i < MAX_CC16 * MAX_DEV; i++)
		m_hThread[i] = NULL;

	for (int i = 0; i < MAX_CC16 * MAX_DEV; i++)
	{
		m_workPowerReady[i] = false;
		m_workGrabReady[i] = false;
	}
	ResetPreviewDisplay();
	InitAllPreviewDisplays();

	std::vector<DtFirmwarePrepThreadParam*> params;
	std::vector<HANDLE> threads;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || !m_grabTabValid[i])
			continue;
		DtFirmwarePrepThreadParam* tp = new DtFirmwarePrepThreadParam;
		tp->fn = this;
		tp->devId = i;
		tp->ok = false;
		unsigned tid = 0;
		HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &FirmwarePrepDevThreadProc, tp, 0, &tid);
		if (h == NULL)
		{
			msgUtf8(DtZh::kFwPrepDevInitFail, i);
			tp->ok = InitWorkCapture(i);
		}
		else
		{
			threads.push_back(h);
		}
		params.push_back(tp);
	}

	for (size_t t = 0; t < threads.size(); t++)
	{
		WaitForSingleObject(threads[t], INFINITE);
		CloseHandle(threads[t]);
	}

	int started = 0;
	bool allOk = true;
	for (size_t t = 0; t < params.size(); t++)
	{
		DtFirmwarePrepThreadParam* tp = params[t];
		if (tp == NULL)
			continue;
		if (!tp->ok)
		{
			allOk = false;
			msgUtf8(DtZh::kFwPrepDevInitFail, tp->devId);
		}
		else
			started++;
		delete tp;
	}

	for (size_t t = 0; t < params.size(); t++)
	{
		DtFirmwarePrepThreadParam* tp = params[t];
		if (tp == NULL || tp->ok)
			continue;
		UninitWorkCapture(tp->devId);
	}
	if (started <= 0)
	{
		msgUtf8(DtZh::kNoChannelEnabled);
		return 0;
	}
	msgUtf8(DtZh::kFwPrepInitOnly);
	return 1;
}

// Start grab worker threads
int DtCarFunction::Start() {

	m_bRunning = true;
	m_bSuppressWorkDraw = false;
	for (int i = 0; i < MAX_CC16 * MAX_DEV; i++)
		m_hThread[i] = NULL;

	/* Capture init runs in WorkProc worker thread (same as Qt sample). */
	for (int i = 0; i < MAX_CC16 * MAX_DEV; i++)
	{
		m_workPowerReady[i] = false;
		m_workGrabReady[i] = false;
	}
	ResetPreviewDisplay();
	InitAllPreviewDisplays();

	int started = 0;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || !m_grabTabValid[i])
			continue;
		DtWorkThreadParam* p = new DtWorkThreadParam;
		p->fn = this;
		p->devId = i;
		m_hThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkThread, p, 0, NULL);
		if (m_hThread[i] == NULL)
		{
			delete p;
			msg("DevID %d: failed to start work thread\n", i);
		}
		else
		{
			started++;
		}
	}
	if (started <= 0)
	{
		m_bRunning = FALSE;
		msgUtf8(DtZh::kNoChannelEnabled);
		return 0;
	}
	return 1;
}

int DtCarFunction::Stop() {

	/* Hold grab before clearing m_bRunning so blocked carGrabFrameDirect can return. */
	RequestStopCapture();
	m_bRunning = FALSE;
	m_bPauseCaptureForBurn = false;
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i) || m_hThread[i] == NULL)
			continue;
		JoinWorkThread(this, i, 3000);
	}
	for (int i = 0; i < m_iEnumDevNum; i++)
	{
		if (!IsDevEnabled(i))
			continue;
		UninitWorkCapture(i);
	}

	return 1;
}

void DtCarFunction::WorkProc(int iDevID)
{
	msgUtf8(DtZh::kLogWorkProcEnter, m_cDeviceName[iDevID] ? m_cDeviceName[iDevID] : "?");

	if (!m_grabTabValid[iDevID])
	{
		msgUtf8(DtZh::kLogWorkProcSkip, iDevID);
		return;
	}

	int iRet;
	int iVcID = 0;

	DtImage_t grabImg;

	bool bInitStatus = InitWorkCapture(iDevID);
	if (!bInitStatus)
		msgUtf8(DtZh::kLogWorkProcInitFail, iDevID);

	DrawImage_t tDrawImage;

	while (m_bRunning && bInitStatus)
	{
		if (!m_bRunning || m_bSuppressWorkDraw)
			break;

		const int curCnt = (m_iVcNum > 0 && m_iVcNum <= MAX_VC) ? m_iVcNum : 1;
		int pmuCur[MAX_VC] = { 0 };
		::carGetPmuCurrent(pmuCur, curCnt, iDevID);
		for (int v = 0; v < curCnt; v++)
			m_tVcData[iDevID][v].iCurrent = pmuCur[v];

		if (!m_bRunning || m_bSuppressWorkDraw)
			break;

		int grabVc = 0;
		/* Do not hold m_csGrab across carGrabFrameDirect (SDK may block ~15s). */
		iRet = ::carGrabFrameDirect(&grabImg, &grabVc, iDevID);

		if (!m_bRunning || m_bSuppressWorkDraw)
			break;

		if (iRet == DT_ERROR_OK)
		{
			bool doDraw = false;
			EnterCriticalSection(&m_csGrab);
			if (m_bRunning && !m_bSuppressWorkDraw)
			{
				iVcID = grabVc;
				::carGetChannelData(&m_tVcData[iDevID][iVcID], iVcID, iDevID);

				VcGrayCache& graySlot = m_grayCache[iDevID][grabVc];
				if (graySlot.pending != 0)
					UpdateGrayCacheFromGrab(iDevID, grabVc, grabImg);

				doDraw = IsVcEnabled(iDevID, iVcID);
				if (doDraw)
				{
					HWND hVid = m_hWndVideo[iDevID][iVcID];
					if (hVid != NULL && ::IsWindow(hVid))
					{
						unsigned short iw = m_vidWndW[iDevID][iVcID];
						unsigned short ih = m_vidWndH[iDevID][iVcID];
						if (iw < 1) iw = 1;
						if (ih < 1) ih = 1;

						m_workShowText[iDevID].Format(
							"[Dev %d VC %d] %s #%d\r\n Volt:%.3fV,Current :%.3f mA\r\n frames: %llu, %.3f fps, delay:%u us \r\n Ecc err:%u, Crc err:%u\r\n Ssr fr: %.3f fps,Fr Gap(us):%u \r\n LossFr :%d\r\n ",
							iDevID, iVcID,
							m_cDeviceName[iDevID] ? m_cDeviceName[iDevID] : "",
							iDevID * m_iVcNum + iVcID + 1,
							m_tVcData[iDevID][iVcID].iVoltage / 1000.f,
							m_tVcData[iDevID][iVcID].iCurrent / 1000000.f,
							(unsigned long long)m_tVcData[iDevID][iVcID].uFrameCount,
							m_tVcData[iDevID][iVcID].dFrameRate,
							m_tVcData[iDevID][iVcID].iDelay,
							m_tVcData[iDevID][iVcID].iEccErrorCnt,
							m_tVcData[iDevID][iVcID].iCrcErrorCnt,
							m_tVcData[iDevID][iVcID].dSsrFrameRate,
							m_tVcData[iDevID][iVcID].iFrameGap,
							m_tVcData[iDevID][iVcID].iLossFrameCnt);

						memset(&tDrawImage, 0, sizeof(tDrawImage));
						tDrawImage.hVideoWnd = hVid;
						tDrawImage.nImgWndW = iw;
						tDrawImage.nImgWndH = ih;
						tDrawImage.bShowImg = true;
						tDrawImage.bShowText = true;
						tDrawImage.szShowData = (LPCSTR)m_workShowText[iDevID];
					}
					else
						doDraw = false;
				}
			}
			else
				doDraw = false;
			LeaveCriticalSection(&m_csGrab);

			if (doDraw && m_bRunning && !m_bSuppressWorkDraw
				&& !IsFirmwareBurnCellActive(iDevID, iVcID))
				DrawImageOnUiThread(tDrawImage, iVcID, iDevID);
		}
	}
	/* Uninit only on Stop()/Close() after thread join — avoids double carUnitGrab (0xC0000008). */
	msgUtf8(DtZh::kLogWorkProcExit, m_cDeviceName[iDevID]);
}

void DtCarFunction::ShowI2cDebug(int uCurSel)
{
	I2CDebug m_I2cDebug(this);
	m_I2cDebug.m_iDevID = uCurSel;
	m_I2cDebug.DoModal();
}
