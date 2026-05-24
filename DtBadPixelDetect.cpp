#include "stdafx.h"
#include "DtBadPixelDetect.h"

#include <math.h>

namespace {

static double CenterRoiMean10(const unsigned char* gray, unsigned int width, unsigned int height)
{
	if (gray == NULL || width < 8 || height < 8)
		return 255.0;
	const unsigned int x0 = (unsigned int)((1.0 - 0.1) / 2.0 * width);
	const unsigned int y0 = (unsigned int)((1.0 - 0.1) / 2.0 * height);
	const unsigned int x1 = x0 + (unsigned int)(0.1 * width);
	const unsigned int y1 = y0 + (unsigned int)(0.1 * height);
	unsigned long long sum = 0;
	unsigned int cnt = 0;
	for (unsigned int y = y0; y < y1; y++)
	{
		for (unsigned int x = x0; x < x1; x++)
		{
			sum += gray[y * width + x];
			cnt++;
		}
	}
	return cnt > 0 ? (double)sum / (double)cnt : 255.0;
}

static const int kBayerNeighborDx[] = { -2, 0, 2, -2, 2, -2, 0, 2 };
static const int kBayerNeighborDy[] = { -2, -2, -2, 0, 0, 2, 2, 2 };

static const int kGrayNeighborDx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
static const int kGrayNeighborDy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };

} // namespace

BadPixelDarkResult AnalyzeDarkFieldHotPixels(
	const unsigned char* gray,
	unsigned int width,
	unsigned int height,
	const GateBadPixelDarkCfg& cfg,
	bool bayerRaster)
{
	BadPixelDarkResult r = {};
	r.analyzed = false;
	r.pass = true;

	if (gray == NULL || width < 3 || height < 3)
		return r;

	const int border = cfg.borderPx < 1 ? 1 : cfg.borderPx;
	const int needBorder = bayerRaster ? 2 : border;
	if ((unsigned int)(needBorder * 2 + 1) >= width || (unsigned int)(needBorder * 2 + 1) >= height)
		return r;

	r.width = width;
	r.height = height;
	r.analyzed = true;

	unsigned long long sumAll = 0;
	const unsigned int countAll = width * height;
	for (unsigned int i = 0; i < countAll; i++)
		sumAll += gray[i];
	r.frameMean = (double)sumAll / (double)countAll;

	r.centerRoiMean = CenterRoiMean10(gray, width, height);

	if (r.frameMean > 128.0)
	{
		r.pass = false;
		r.failReason = BP_FAIL_DARK_SCENE;
		return r;
	}

	const int hotDelta = cfg.hotDelta > 0 ? cfg.hotDelta : 1;
	const int hotAbsMin = cfg.hotAbsMin > 0 ? cfg.hotAbsMin : 1;
	const int maxAllow = cfg.maxBadPixels < 0 ? 0 : cfg.maxBadPixels;

	const int* ndx = bayerRaster ? kBayerNeighborDx : kGrayNeighborDx;
	const int* ndy = bayerRaster ? kBayerNeighborDy : kGrayNeighborDy;
	const unsigned int y0 = (unsigned int)needBorder;
	const unsigned int y1 = height - (unsigned int)needBorder;
	const unsigned int x0 = (unsigned int)needBorder;
	const unsigned int x1 = width - (unsigned int)needBorder;

	for (unsigned int y = y0; y < y1; y++)
	{
		for (unsigned int x = x0; x < x1; x++)
		{
			const unsigned int c = gray[y * width + x];
			if ((int)c < hotAbsMin)
				continue;

			unsigned int nSum = 0;
			unsigned int nCnt = 0;
			for (int ni = 0; ni < 8; ni++)
			{
				const int nx = (int)x + ndx[ni];
				const int ny = (int)y + ndy[ni];
				if (nx < 0 || ny < 0 || (unsigned int)nx >= width || (unsigned int)ny >= height)
					continue;
				nSum += gray[(unsigned int)ny * width + (unsigned int)nx];
				nCnt++;
			}
			if (nCnt == 0)
				continue;

			const unsigned int nMean = nSum / nCnt;
			if (c > nMean + (unsigned int)hotDelta)
				r.badCount++;
		}
	}

	r.pass = (r.badCount <= (unsigned int)maxAllow);
	if (!r.pass)
		r.failReason = BP_FAIL_HOT_PIXEL_COUNT;
	return r;
}
