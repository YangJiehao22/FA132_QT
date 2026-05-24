#pragma once

#include <vector>

/** Black-field hot-pixel gate (GateSpec.ini [bad_pixel_dark]). */
struct GateBadPixelDarkCfg
{
	bool enabled;
	/** 0=8-neighbor simple; 1=qtmALGO_HotPixel (Huawei dark-field). */
	int algoMode;
	int maxBadPixels;
	int hotDelta;
	int hotAbsMin;
	int borderPx;
	/** Cluster threshold |pixel-mean| (brightConctractld); 0=use hotDelta. */
	int brightContrastCluster;
	/** Min pixels in one cluster (brightpixnum). */
	int clusterMinPixels;
	/** Max single defects per 100000 pixels (SingleDefectPercent). */
	int singleDefectPermyriad;
	/** Merge Gr+Gb to G channel (GrGbtoG). */
	bool grGbToG;
	/**
	 * Bayer phase for RAW/MIPI: -1=auto (SDK grab rawFmt, else sensor ini outformat%%4).
	 * 0=RGGB 1=GRBG 2=GBRG 3=BGGR (matches DtPara.ini outformat for RAW).
	 */
	int bayerPattern;
	/** Master switch: enable snapshot save on PASS/FAIL. */
	bool saveSnapshot;
	/** Save preview BMP (*.bmp). */
	bool saveBmp;
	/** Save packed MIPI/YUV (*_packed.raw / *_packed.yuv). */
	bool savePackedRaw;
	/** Save UNPACK12 / P12 (*_unpack12.raw). */
	bool saveUnpack12;
	/** Save UNPACK10 for Huawei algo (*_unpack10.raw). */
	bool saveUnpack10;
	/** Output folder; empty = exe\\log\\badpixel\\ */
	TCHAR saveDir[MAX_PATH];
};

/** Why bad-pixel test failed (for log). */
enum BadPixelFailReason
{
	BP_FAIL_NONE = 0,
	BP_FAIL_DARK_SCENE,       /**< Not dark field (ROI/frame too bright). */
	BP_FAIL_CLUSTER_COUNT,    /**< Huawei: cluster count over limit. */
	BP_FAIL_SINGLE_COUNT,     /**< Huawei: single defect count over limit. */
	BP_FAIL_HOT_PIXEL_COUNT,  /**< Neighbor mode: hot pixel count over limit. */
};

struct BadPixelDarkResult
{
	bool analyzed;
	bool pass;
	unsigned int badCount;
	unsigned int singleDefectCount;
	unsigned int width;
	unsigned int height;
	double frameMean;
	/** Center 10% ROI mean (Huawei dark check); 0 if not computed. */
	double centerRoiMean;
	BadPixelFailReason failReason;
};

/**
 * Hot-pixel test for black field.
 * @param bayerRaster true: Bayer RAW8 raster (same-color neighbors at distance 2).
 *                    false: planar gray / YUV-Y (8-neighbors at distance 1).
 */
BadPixelDarkResult AnalyzeDarkFieldHotPixels(
	const unsigned char* gray,
	unsigned int width,
	unsigned int height,
	const GateBadPixelDarkCfg& cfg,
	bool bayerRaster = false);
