#include "stdafx.h"
#include "DtHotPixelAlgo.h"

#include <stdlib.h>
#include <string.h>

namespace {

struct SeedPt
{
	int width;
	int height;
};

static void MarkBrightVsMean8(
	const unsigned char* src,
	unsigned int w,
	unsigned int h,
	int threshold,
	unsigned char* mask,
	unsigned int& singleCount)
{
	unsigned long long sum = 0;
	const unsigned int n = w * h;
	for (unsigned int i = 0; i < n; i++)
		sum += src[i];
	const double mean = (double)sum / (double)n;
	singleCount = 0;
	for (unsigned int j = 0; j < h; j++)
	{
		for (unsigned int i = 0; i < w; i++)
		{
			const unsigned int idx = j * w + i;
			const double d = (double)src[idx] - mean;
			const double ad = d < 0 ? -d : d;
			if (ad >= (double)threshold)
			{
				mask[idx] = 1;
				singleCount++;
			}
			else
			{
				mask[idx] = 0;
			}
		}
	}
}

static bool IsIsolatedBright(const unsigned char* mask, unsigned int w, unsigned int h, unsigned int x, unsigned int y)
{
	if (x < 1 || y < 1 || x + 1 >= w || y + 1 >= h)
		return true;
	const unsigned int i = y * w + x;
	if (!mask[i])
		return false;
	if (mask[i - w - 1] || mask[i - w] || mask[i - w + 1]
		|| mask[i - 1] || mask[i + 1]
		|| mask[i + w - 1] || mask[i + w] || mask[i + w + 1])
		return false;
	return true;
}

static void FloodCluster8(
	unsigned char* mask,
	unsigned int w,
	unsigned int h,
	unsigned int sx,
	unsigned int sy,
	unsigned int& clusterPixels)
{
	SeedPt* stack = (SeedPt*)malloc(sizeof(SeedPt) * w * h);
	if (stack == NULL)
		return;

	int top = 0;
	stack[top].width = (int)sx;
	stack[top].height = (int)sy;
	top++;
	mask[sy * w + sx] = 0;
	clusterPixels = 1;

	while (top > 0)
	{
		top--;
		const int cx = stack[top].width;
		const int cy = stack[top].height;

		static const int kDx[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		static const int kDy[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		for (int k = 0; k < 8; k++)
		{
			const int nx = cx + kDx[k];
			const int ny = cy + kDy[k];
			if (nx < 0 || ny < 0 || (unsigned int)nx >= w || (unsigned int)ny >= h)
				continue;
			const unsigned int ni = (unsigned int)ny * w + (unsigned int)nx;
			if (mask[ni] == 0)
				continue;
			mask[ni] = 0;
			clusterPixels++;
			stack[top].width = nx;
			stack[top].height = ny;
			top++;
		}
	}
	free(stack);
}

static int CountBrightClusters(unsigned char* mask, unsigned int w, unsigned int h, int minClusterPixels)
{
	int clusterCount = 0;
	for (unsigned int y = 3; y + 3 < h; y++)
	{
		for (unsigned int x = 3; x + 3 < w; x++)
		{
			const unsigned int i = y * w + x;
			if (!mask[i])
				continue;
			if (IsIsolatedBright(mask, w, h, x, y))
				continue;

			unsigned int pixels = 0;
			FloodCluster8(mask, w, h, x, y, pixels);
			if ((int)pixels > minClusterPixels)
				clusterCount++;
		}
	}
	return clusterCount;
}

static int RunChannel8(
	const unsigned char* ch,
	unsigned int cw,
	unsigned int chh,
	int singleTh,
	int clusterTh,
	int minClusterPx,
	unsigned int& outSingles)
{
	std::vector<unsigned char> maskSingle(cw * chh);
	unsigned int singles = 0;
	MarkBrightVsMean8(ch, cw, chh, singleTh, maskSingle.data(), singles);
	outSingles += singles;

	std::vector<unsigned char> maskCluster(cw * chh);
	unsigned int dummy = 0;
	MarkBrightVsMean8(ch, cw, chh, clusterTh, maskCluster.data(), dummy);
	return CountBrightClusters(maskCluster.data(), cw, chh, minClusterPx);
}

static bool SplitBayerGrGbToG(
	const unsigned char* src,
	unsigned char* r,
	unsigned char* g,
	unsigned char* b,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt)
{
	if ((w & 1) || (h & 1))
		return false;

	unsigned int idx = 0;
	for (unsigned int row = 0; row < h; row += 2)
	{
		const unsigned int line = row * w;
		for (unsigned int col = 0; col < w; col += 2)
		{
			const unsigned int p = line + col;
			switch (rawFmt)
			{
			case RAW_RGGB:
				r[idx] = src[p];
				g[(idx << 1) + 1] = src[p + 1];
				g[(idx << 1)] = src[p + w];
				b[idx] = src[p + w + 1];
				break;
			case RAW_GRBG:
				g[(idx << 1)] = src[p];
				r[idx] = src[p + 1];
				b[idx] = src[p + w];
				g[(idx << 1) + 1] = src[p + w + 1];
				break;
			case RAW_GBRG:
				g[(idx << 1)] = src[p];
				b[idx] = src[p + 1];
				r[idx] = src[p + w];
				g[(idx << 1) + 1] = src[p + w + 1];
				break;
			case RAW_BGGR:
				b[idx] = src[p];
				g[(idx << 1) + 1] = src[p + 1];
				g[(idx << 1)] = src[p + w];
				r[idx] = src[p + w + 1];
				break;
			default:
				return false;
			}
			idx++;
		}
	}
	return true;
}

static bool SplitBayerFour(
	const unsigned char* src,
	unsigned char* r,
	unsigned char* gr,
	unsigned char* gb,
	unsigned char* b,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt)
{
	if ((w & 1) || (h & 1))
		return false;

	unsigned int idx = 0;
	for (unsigned int row = 0; row < h; row += 2)
	{
		const unsigned int line = row * w;
		for (unsigned int col = 0; col < w; col += 2)
		{
			const unsigned int p = line + col;
			switch (rawFmt)
			{
			case RAW_RGGB:
				r[idx] = src[p];
				gr[idx] = src[p + 1];
				gb[idx] = src[p + w];
				b[idx] = src[p + w + 1];
				break;
			case RAW_GRBG:
				gr[idx] = src[p];
				r[idx] = src[p + 1];
				b[idx] = src[p + w];
				gb[idx] = src[p + w + 1];
				break;
			case RAW_GBRG:
				gb[idx] = src[p];
				b[idx] = src[p + 1];
				r[idx] = src[p + w];
				gr[idx] = src[p + w + 1];
				break;
			case RAW_BGGR:
				b[idx] = src[p];
				gb[idx] = src[p + 1];
				gr[idx] = src[p + w];
				r[idx] = src[p + w + 1];
				break;
			default:
				return false;
			}
			idx++;
		}
	}
	return true;
}

static double BayerRoiMean10(const unsigned char* bayer, unsigned int w, unsigned int h, RAW_FORMAT rawFmt)
{
	const unsigned int x0 = (unsigned int)((1.0 - 0.1) / 2.0 * w);
	const unsigned int y0 = (unsigned int)((1.0 - 0.1) / 2.0 * h);
	const unsigned int x1 = x0 + (unsigned int)(0.1 * w);
	const unsigned int y1 = y0 + (unsigned int)(0.1 * h);
	unsigned long long sum = 0;
	unsigned int cnt = 0;

	int offA = 0;
	int offB = 1;
	switch (rawFmt)
	{
	case RAW_RGGB:
	case RAW_BGGR:
		if ((x0 & 1) == (y0 & 1)) { offA = 1; offB = 0; }
		break;
	case RAW_GRBG:
	case RAW_GBRG:
		if ((x0 & 1) != (y0 & 1)) { offA = 1; offB = 0; }
		break;
	default:
		break;
	}

	for (unsigned int y = y0; y + 1 < y1; y += 2)
	{
		for (unsigned int x = x0; x + 1 < x1; x += 2)
		{
			const unsigned int p = y * w + x;
			sum += bayer[p + offA];
			sum += bayer[p + w + offB];
			cnt += 2;
		}
	}
	return cnt > 0 ? (double)sum / (double)cnt : 255.0;
}

static double BayerRoiMean16(const unsigned short* bayer, unsigned int w, unsigned int h, RAW_FORMAT rawFmt)
{
	const unsigned int x0 = (unsigned int)((1.0 - 0.1) / 2.0 * w);
	const unsigned int y0 = (unsigned int)((1.0 - 0.1) / 2.0 * h);
	const unsigned int x1 = x0 + (unsigned int)(0.1 * w);
	const unsigned int y1 = y0 + (unsigned int)(0.1 * h);
	unsigned long long sum = 0;
	unsigned int cnt = 0;

	int offA = 0;
	int offB = 1;
	switch (rawFmt)
	{
	case RAW_RGGB:
	case RAW_BGGR:
		if ((x0 & 1) == (y0 & 1)) { offA = 1; offB = 0; }
		break;
	case RAW_GRBG:
	case RAW_GBRG:
		if ((x0 & 1) != (y0 & 1)) { offA = 1; offB = 0; }
		break;
	default:
		break;
	}

	for (unsigned int y = y0; y + 1 < y1; y += 2)
	{
		for (unsigned int x = x0; x + 1 < x1; x += 2)
		{
			const unsigned int p = y * w + x;
			sum += bayer[p + offA];
			sum += bayer[p + w + offB];
			cnt += 2;
		}
	}
	return cnt > 0 ? (double)sum / (double)cnt : 65535.0;
}

static void MarkBrightVsMean16(
	const unsigned short* src,
	unsigned int w,
	unsigned int h,
	int threshold,
	unsigned char* mask,
	unsigned int& singleCount)
{
	unsigned long long sum = 0;
	const unsigned int n = w * h;
	for (unsigned int i = 0; i < n; i++)
		sum += src[i];
	const double mean = (double)sum / (double)n;
	singleCount = 0;
	for (unsigned int j = 0; j < h; j++)
	{
		for (unsigned int i = 0; i < w; i++)
		{
			const unsigned int idx = j * w + i;
			const double d = (double)src[idx] - mean;
			const double ad = d < 0 ? -d : d;
			if (ad >= (double)threshold)
			{
				mask[idx] = 1;
				singleCount++;
			}
			else
			{
				mask[idx] = 0;
			}
		}
	}
}

static int RunChannel16(
	const unsigned short* ch,
	unsigned int cw,
	unsigned int chh,
	int singleTh,
	int clusterTh,
	int minClusterPx,
	unsigned int& outSingles)
{
	std::vector<unsigned char> maskSingle(cw * chh);
	unsigned int singles = 0;
	MarkBrightVsMean16(ch, cw, chh, singleTh, maskSingle.data(), singles);
	outSingles += singles;

	std::vector<unsigned char> maskCluster(cw * chh);
	unsigned int dummy = 0;
	MarkBrightVsMean16(ch, cw, chh, clusterTh, maskCluster.data(), dummy);
	return CountBrightClusters(maskCluster.data(), cw, chh, minClusterPx);
}

static bool SplitBayerGrGbToG16(
	const unsigned short* src,
	unsigned short* r,
	unsigned short* g,
	unsigned short* b,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt)
{
	if ((w & 1) || (h & 1))
		return false;

	unsigned int idx = 0;
	for (unsigned int row = 0; row < h; row += 2)
	{
		const unsigned int line = row * w;
		for (unsigned int col = 0; col < w; col += 2)
		{
			const unsigned int p = line + col;
			switch (rawFmt)
			{
			case RAW_RGGB:
				r[idx] = src[p];
				g[(idx << 1) + 1] = src[p + 1];
				g[(idx << 1)] = src[p + w];
				b[idx] = src[p + w + 1];
				break;
			case RAW_GRBG:
				g[(idx << 1)] = src[p];
				r[idx] = src[p + 1];
				b[idx] = src[p + w];
				g[(idx << 1) + 1] = src[p + w + 1];
				break;
			case RAW_GBRG:
				g[(idx << 1)] = src[p];
				b[idx] = src[p + 1];
				r[idx] = src[p + w];
				g[(idx << 1) + 1] = src[p + w + 1];
				break;
			case RAW_BGGR:
				b[idx] = src[p];
				g[(idx << 1) + 1] = src[p + 1];
				g[(idx << 1)] = src[p + w];
				r[idx] = src[p + w + 1];
				break;
			default:
				return false;
			}
			idx++;
		}
	}
	return true;
}

static bool SplitBayerFour16(
	const unsigned short* src,
	unsigned short* r,
	unsigned short* gr,
	unsigned short* gb,
	unsigned short* b,
	unsigned int w,
	unsigned int h,
	RAW_FORMAT rawFmt)
{
	if ((w & 1) || (h & 1))
		return false;

	unsigned int idx = 0;
	for (unsigned int row = 0; row < h; row += 2)
	{
		const unsigned int line = row * w;
		for (unsigned int col = 0; col < w; col += 2)
		{
			const unsigned int p = line + col;
			switch (rawFmt)
			{
			case RAW_RGGB:
				r[idx] = src[p];
				gr[idx] = src[p + 1];
				gb[idx] = src[p + w];
				b[idx] = src[p + w + 1];
				break;
			case RAW_GRBG:
				gr[idx] = src[p];
				r[idx] = src[p + 1];
				b[idx] = src[p + w];
				gb[idx] = src[p + w + 1];
				break;
			case RAW_GBRG:
				gb[idx] = src[p];
				b[idx] = src[p + 1];
				r[idx] = src[p + w];
				gr[idx] = src[p + w + 1];
				break;
			case RAW_BGGR:
				b[idx] = src[p];
				gb[idx] = src[p + 1];
				gr[idx] = src[p + w];
				r[idx] = src[p + w + 1];
				break;
			default:
				return false;
			}
			idx++;
		}
	}
	return true;
}

static double FrameMeanUnpack10AsGray8(const unsigned short* u10, unsigned int n)
{
	if (u10 == NULL || n < 1)
		return 255.0;
	unsigned long long sum = 0;
	for (unsigned int i = 0; i < n; i++)
		sum += ((unsigned)u10[i] + 2) >> 2;
	return (double)sum / (double)n;
}

} // namespace

unsigned short P12ToUnpack10Sample(unsigned short p12)
{
	unsigned v = p12;
	if (v > 4095)
		v = 4095;
	return (unsigned short)((v + 2) >> 2);
}

void ConvertP12ToUnpack10(const unsigned short* p12, unsigned short* u10, unsigned int count)
{
	if (p12 == NULL || u10 == NULL)
		return;
	for (unsigned int i = 0; i < count; i++)
		u10[i] = P12ToUnpack10Sample(p12[i]);
}

double HotPixelHuaweiCenterRoiMean10(const unsigned char* buf, unsigned int w, unsigned int h, bool bayerRaster, RAW_FORMAT rawFmt)
{
	if (buf == NULL || w < 8 || h < 8)
		return 255.0;

	if (bayerRaster)
		return BayerRoiMean10(buf, w, h, rawFmt);

	const unsigned int x0 = (unsigned int)((1.0 - 0.1) / 2.0 * w);
	const unsigned int y0 = (unsigned int)((1.0 - 0.1) / 2.0 * h);
	const unsigned int x1 = x0 + (unsigned int)(0.1 * w);
	const unsigned int y1 = y0 + (unsigned int)(0.1 * h);
	unsigned long long sum = 0;
	unsigned int cnt = 0;
	for (unsigned int y = y0; y < y1; y++)
	{
		for (unsigned int x = x0; x < x1; x++)
		{
			sum += buf[y * w + x];
			cnt++;
		}
	}
	return cnt > 0 ? (double)sum / (double)cnt : 255.0;
}

bool HotPixelHuaweiCheckDarkSceneMean(const unsigned char* buf, unsigned int w, unsigned int h, bool bayerRaster, RAW_FORMAT rawFmt)
{
	return HotPixelHuaweiCenterRoiMean10(buf, w, h, bayerRaster, rawFmt) <= 50.0;
}

BadPixelDarkResult AnalyzeHotPixelHuaweiMono8(
	const unsigned char* mono,
	unsigned int width,
	unsigned int height,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail)
{
	BadPixelDarkResult r = {};
	r.analyzed = false;
	r.pass = true;
	if (detail != NULL)
		*detail = HotPixelHuaweiDetail();

	if (mono == NULL || width < 8 || height < 8)
		return r;

	unsigned long long sum = 0;
	const unsigned int n = width * height;
	for (unsigned int i = 0; i < n; i++)
		sum += mono[i];
	r.frameMean = (double)sum / (double)n;
	r.width = width;
	r.height = height;
	r.analyzed = true;
	r.centerRoiMean = HotPixelHuaweiCenterRoiMean10(mono, width, height, false, RAW_RGGB);

	if (!HotPixelHuaweiCheckDarkSceneMean(mono, width, height, false, RAW_RGGB))
	{
		r.pass = false;
		r.failReason = BP_FAIL_DARK_SCENE;
		return r;
	}

	const int singleTh = cfg.hotDelta > 0 ? cfg.hotDelta : 25;
	const int clusterTh = cfg.brightContrastCluster > 0 ? cfg.brightContrastCluster : singleTh;
	const int minClusterPx = cfg.clusterMinPixels > 0 ? cfg.clusterMinPixels : 2;
	const int maxCluster = cfg.maxBadPixels < 0 ? 0 : cfg.maxBadPixels;
	const unsigned int singleLimit = (unsigned int)((double)n * (double)cfg.singleDefectPermyriad / 100000.0);

	unsigned int singles = 0;
	const int clusters = RunChannel8(mono, width, height, singleTh, clusterTh, minClusterPx, singles);

	if (detail != NULL)
	{
		detail->singleDefectCount = singles;
		detail->clusterMono = clusters;
	}

	r.badCount = (unsigned int)clusters;
	r.singleDefectCount = singles;
	r.pass = (clusters <= maxCluster) && (singles <= singleLimit);
	if (!r.pass)
	{
		if (clusters > maxCluster)
			r.failReason = BP_FAIL_CLUSTER_COUNT;
		else if (singles > singleLimit)
			r.failReason = BP_FAIL_SINGLE_COUNT;
	}
	return r;
}

BadPixelDarkResult AnalyzeHotPixelHuaweiBayer8(
	const unsigned char* bayer,
	unsigned int width,
	unsigned int height,
	RAW_FORMAT rawFmt,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail)
{
	BadPixelDarkResult r = {};
	r.analyzed = false;
	r.pass = true;
	if (detail != NULL)
		*detail = HotPixelHuaweiDetail();

	if (bayer == NULL || width < 8 || height < 8 || (width & 1) || (height & 1))
		return r;

	unsigned long long sum = 0;
	const unsigned int n = width * height;
	for (unsigned int i = 0; i < n; i++)
		sum += bayer[i];
	r.frameMean = (double)sum / (double)n;
	r.width = width;
	r.height = height;
	r.analyzed = true;
	r.centerRoiMean = HotPixelHuaweiCenterRoiMean10(bayer, width, height, true, rawFmt);

	if (!HotPixelHuaweiCheckDarkSceneMean(bayer, width, height, true, rawFmt))
	{
		r.pass = false;
		r.failReason = BP_FAIL_DARK_SCENE;
		return r;
	}

	const unsigned int cw = width / 2;
	const unsigned int ch = height / 2;
	const unsigned int chN = cw * ch;
	const int singleTh = cfg.hotDelta > 0 ? cfg.hotDelta : 25;
	const int clusterTh = cfg.brightContrastCluster > 0 ? cfg.brightContrastCluster : singleTh;
	const int minClusterPx = cfg.clusterMinPixels > 0 ? cfg.clusterMinPixels : 2;
	const int maxCluster = cfg.maxBadPixels < 0 ? 0 : cfg.maxBadPixels;
	const unsigned int singleLimit = (unsigned int)((double)n * (double)cfg.singleDefectPermyriad / 100000.0);

	unsigned int singles = 0;
	int cR = 0, cGr = 0, cGb = 0, cB = 0, cG = 0;

	if (cfg.grGbToG)
	{
		std::vector<unsigned char> rCh(chN), gCh(chN * 2), bCh(chN);
		if (!SplitBayerGrGbToG(bayer, rCh.data(), gCh.data(), bCh.data(), width, height, rawFmt))
			return r;

		cR = RunChannel8(rCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cB = RunChannel8(bCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cG = RunChannel8(gCh.data(), cw, height, singleTh, clusterTh, minClusterPx, singles);
	}
	else
	{
		std::vector<unsigned char> rCh(chN), grCh(chN), gbCh(chN), bCh(chN);
		if (!SplitBayerFour(bayer, rCh.data(), grCh.data(), gbCh.data(), bCh.data(), width, height, rawFmt))
			return r;

		cR = RunChannel8(rCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cGr = RunChannel8(grCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cGb = RunChannel8(gbCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cB = RunChannel8(bCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
	}

	if (detail != NULL)
	{
		detail->singleDefectCount = singles;
		detail->clusterR = cR;
		detail->clusterGr = cGr;
		detail->clusterGb = cGb;
		detail->clusterB = cB;
		detail->clusterG = cG;
	}

	r.badCount = (unsigned int)(cR + cGr + cGb + cB + cG);
	bool passCh = true;
	if (cfg.grGbToG)
		passCh = (cR <= maxCluster) && (cB <= maxCluster) && (cG <= maxCluster);
	else
		passCh = (cR <= maxCluster) && (cGr <= maxCluster) && (cGb <= maxCluster) && (cB <= maxCluster);

	r.singleDefectCount = singles;
	r.pass = passCh && (singles <= singleLimit);
	if (!r.pass)
	{
		if (!passCh)
			r.failReason = BP_FAIL_CLUSTER_COUNT;
		else if (singles > singleLimit)
			r.failReason = BP_FAIL_SINGLE_COUNT;
	}
	return r;
}

BadPixelDarkResult AnalyzeHotPixelHuaweiBayer16(
	const unsigned short* bayer,
	unsigned int width,
	unsigned int height,
	RAW_FORMAT rawFmt,
	const GateBadPixelDarkCfg& cfg,
	HotPixelHuaweiDetail* detail)
{
	BadPixelDarkResult r = {};
	r.analyzed = false;
	r.pass = true;
	if (detail != NULL)
		*detail = HotPixelHuaweiDetail();

	if (bayer == NULL || width < 8 || height < 8 || (width & 1) || (height & 1))
		return r;

	const unsigned int n = width * height;
	std::vector<unsigned short> u10(n);
	ConvertP12ToUnpack10(bayer, u10.data(), n);

	r.frameMean = FrameMeanUnpack10AsGray8(u10.data(), n);
	r.width = width;
	r.height = height;
	r.analyzed = true;
	r.centerRoiMean = BayerRoiMean16(u10.data(), width, height, rawFmt);

	/* qtmALGO: RAW12 -> QtImageToUnpack10, then center ROI mean <= 200 (10-bit domain). */
	if (r.centerRoiMean > 200.0)
	{
		r.pass = false;
		r.failReason = BP_FAIL_DARK_SCENE;
		return r;
	}

	const unsigned int cw = width / 2;
	const unsigned int ch = height / 2;
	const unsigned int chN = cw * ch;
	const int singleTh = cfg.hotDelta > 0 ? cfg.hotDelta : 25;
	const int clusterTh = cfg.brightContrastCluster > 0 ? cfg.brightContrastCluster : singleTh;
	const int minClusterPx = cfg.clusterMinPixels > 0 ? cfg.clusterMinPixels : 2;
	const int maxCluster = cfg.maxBadPixels < 0 ? 0 : cfg.maxBadPixels;
	const unsigned int singleLimit = (unsigned int)((double)n * (double)cfg.singleDefectPermyriad / 100000.0);

	unsigned int singles = 0;
	int cR = 0, cGr = 0, cGb = 0, cB = 0, cG = 0;

	if (cfg.grGbToG)
	{
		std::vector<unsigned short> rCh(chN), gCh(chN * 2), bCh(chN);
		if (!SplitBayerGrGbToG16(u10.data(), rCh.data(), gCh.data(), bCh.data(), width, height, rawFmt))
			return r;

		cR = RunChannel16(rCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cB = RunChannel16(bCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cG = RunChannel16(gCh.data(), cw, height, singleTh, clusterTh, minClusterPx, singles);
	}
	else
	{
		std::vector<unsigned short> rCh(chN), grCh(chN), gbCh(chN), bCh(chN);
		if (!SplitBayerFour16(u10.data(), rCh.data(), grCh.data(), gbCh.data(), bCh.data(), width, height, rawFmt))
			return r;

		cR = RunChannel16(rCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cGr = RunChannel16(grCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cGb = RunChannel16(gbCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
		cB = RunChannel16(bCh.data(), cw, ch, singleTh, clusterTh, minClusterPx, singles);
	}

	if (detail != NULL)
	{
		detail->singleDefectCount = singles;
		detail->clusterR = cR;
		detail->clusterGr = cGr;
		detail->clusterGb = cGb;
		detail->clusterB = cB;
		detail->clusterG = cG;
	}

	r.badCount = (unsigned int)(cR + cGr + cGb + cB + cG);
	bool passCh = true;
	if (cfg.grGbToG)
		passCh = (cR <= maxCluster) && (cB <= maxCluster) && (cG <= maxCluster);
	else
		passCh = (cR <= maxCluster) && (cGr <= maxCluster) && (cGb <= maxCluster) && (cB <= maxCluster);

	r.singleDefectCount = singles;
	r.pass = passCh && (singles <= singleLimit);
	if (!r.pass)
	{
		if (!passCh)
			r.failReason = BP_FAIL_CLUSTER_COUNT;
		else if (singles > singleLimit)
			r.failReason = BP_FAIL_SINGLE_COUNT;
	}
	return r;
}
