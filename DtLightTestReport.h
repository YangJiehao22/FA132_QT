#pragma once

#include <vector>

/** Overall production outcome stage (CSV FailStage / logging). */
enum ProductionFailStage
{
	PROD_STAGE_OK = 0,
	PROD_STAGE_BURN = 1,
	PROD_STAGE_VERIFY = 2,
	PROD_STAGE_LIGHT = 3,
	PROD_STAGE_SENSOR_ID = 4,
	PROD_STAGE_AGING = 5,
};

enum AgingCsvResult
{
	AGING_CSV_NA = 0,
	AGING_CSV_SKIP,
	AGING_CSV_OK,
	AGING_CSV_NG,
};

/** ASCII tag for CSV column FailStage. */
const char* ProductionFailStageTag(int stage);

struct LightTestChannelRecord
{
	int devId;
	int vcId;
	bool overallPass;

	/** Dominant fail stage for this run (OK / BURN / VERIFY / LIGHT). */
	int failStage;

	/** When true, Ssr/Cur/Temp/BadPx columns are written as SKIP (early abort). */
	bool measureSkipped;

	bool fwBurnEnabled;
	bool fwBurnTested;
	bool okFwBurn;
	int fwBurnErrCode;

	bool fwVerifyEnabled;
	bool fwVerifyTested;
	bool okFwVerify;

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

	/** Lowercase hex, 20 chars when read OK (Ruibo 0x7E80 x10). */
	CString sensorIdHex;

	/** [aging_test] Enabled=1 fields (NA when aging off). */
	bool agingEnabled;
	int agingResult;
	double agingSsrFps;
	double agingCurrent_mA;
	bool agingHasTemp;
	double agingTempC;
	double agingAvdd_mV;
	double agingIovdd_mV;
	double agingDvdd_mV;
	CString agingFailReason;
};

/** Append rows to daily Production_report.csv (create with header+BOM if missing). */
bool WriteProductionReportCsv(
	LPCTSTR csvPath,
	const std::vector<LightTestChannelRecord>& rows,
	bool allPass,
	LPCTSTR gateSpecPath,
	int delayMs,
	LPCTSTR sensorIniPath);
