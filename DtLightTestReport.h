#pragma once

#include <vector>

struct LightTestChannelRecord
{
	int devId;
	int vcId;
	bool overallPass;

	double ssrFps;
	double ssrMin;
	double ssrMax;
	bool okSsr;

	double current_mA;
	double curMin;
	double curMax;
	bool okCur;

	bool hasTemp;
	double tempC;
	double tempMin;
	double tempMax;
	bool okTemp;

	bool badPixelEnabled;
	bool badAnalyzed;
	bool okBadPx;
	unsigned int badCount;
	int badMaxAllow;
	double frameMean;

	CString imageBmp;
	CString imageRawUnpacked;
};

/** Append rows to daily CSV (create with header+BOM if missing). Returns true on success. */
bool WriteLightTestReportCsv(
	LPCTSTR csvPath,
	const std::vector<LightTestChannelRecord>& rows,
	bool allPass,
	LPCTSTR gateSpecPath,
	int delayMs,
	LPCTSTR sensorIniPath);
