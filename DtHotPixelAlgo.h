#pragma once

#include "DtBadPixelDetect.h"
#include "ezCarDTCCM_SDK/imagekit.h"

/** Port of qtmALGO_HotPixel (Huawei 8M dark-field bright pixel / hot pixel). */
struct HotPixelHuaweiDetail
{
	unsigned int singleDefectCount;
	int clusterR;
	int clusterGr;
	int clusterGb;
	int clusterB;
	int clusterG;
	int clusterMono;
};

/** P12 (12-bit in uint16) -> UNPACK10 (0..1023), same as qtmALGO QtImageToUnpack10. */
unsigned short P12ToUnpack10Sample(unsigned short p12);
void ConvertP12ToUnpack10(const unsigned short* p12, unsigned short* u10, unsigned int count);

/**
 * Bayer P12: converts to UNPACK10 internally, then Huawei algo (threshold 200 = 10-bit domain).
 */
BadPixelDarkResult AnalyzeHotPixelHuaweiBayer16(
	const unsigned short* bayerP12,
	unsigned int width,
	unsigned int height,
	RAW_FORMAT rawFmt,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail = NULL);

/**
 * Bayer RAW8 dark-field test (R/Gr/Gb/B or R/G/B if grGbToG).
 * Thresholds from GateBadPixelDarkCfg when algoMode==1:
 *   hotDelta -> |pixel-mean| single threshold
 *   brightContrastCluster -> cluster threshold
 *   clusterMinPixels -> min pixels per cluster (brightpixnum)
 *   maxBadPixels -> max clusters per channel
 *   singleDefectPermyriad -> max singles per 100000 pixels
 */
BadPixelDarkResult AnalyzeHotPixelHuaweiBayer8(
	const unsigned char* bayer,
	unsigned int width,
	unsigned int height,
	RAW_FORMAT rawFmt,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail = NULL);

/** MONO / YUV-Y plane (full resolution). */
BadPixelDarkResult AnalyzeHotPixelHuaweiMono8(
	const unsigned char* mono,
	unsigned int width,
	unsigned int height,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail = NULL);

/** Center 10% ROI mean (Bayer uses same-color subsample). */
double HotPixelHuaweiCenterRoiMean10(const unsigned char* buf, unsigned int w, unsigned int h, bool bayerRaster, RAW_FORMAT rawFmt);

/** Center 10% ROI mean; false if too bright (not dark field, threshold 50). */
bool HotPixelHuaweiCheckDarkSceneMean(const unsigned char* buf, unsigned int w, unsigned int h, bool bayerRaster, RAW_FORMAT rawFmt);
