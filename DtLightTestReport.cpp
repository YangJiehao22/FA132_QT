#include "stdafx.h"
#include "DtLightTestReport.h"

const char* ProductionFailStageTag(int stage)
{
	switch (stage)
	{
	case PROD_STAGE_BURN: return "BURN";
	case PROD_STAGE_VERIFY: return "VERIFY";
	case PROD_STAGE_LIGHT: return "LIGHT";
	case PROD_STAGE_OK:
	default: return "OK";
	}
}

namespace {

static void AppendCsvField(CStringA& line, const char* value, bool* first)
{
	if (!*first)
		line += ',';
	*first = false;

	if (value == NULL)
		return;

	line += '"';
	for (const char* p = value; *p; ++p)
	{
		if (*p == '"')
			line += "\"\"";
		else
			line += *p;
	}
	line += '"';
}

static void AppendCsvDouble(CStringA& line, double v, bool* first)
{
	CStringA s;
	s.Format("%.3f", v);
	AppendCsvField(line, s, first);
}

static void AppendCsvInt(CStringA& line, int v, bool* first)
{
	CStringA s;
	s.Format("%d", v);
	AppendCsvField(line, s, first);
}

static void AppendCsvUInt(CStringA& line, unsigned int v, bool* first)
{
	CStringA s;
	s.Format("%u", v);
	AppendCsvField(line, s, first);
}

static const char* TriResult(bool measured, bool ok)
{
	if (!measured)
		return "SKIP";
	return ok ? "OK" : "NG";
}

static const char* FwResultTag(bool enabled, bool tested, bool ok)
{
	if (!enabled)
		return "N/A";
	if (!tested)
		return "SKIP";
	return ok ? "OK" : "NG";
}

} // namespace

bool WriteProductionReportCsv(
	LPCTSTR csvPath,
	const std::vector<LightTestChannelRecord>& rows,
	bool allPass,
	LPCTSTR gateSpecPath,
	int delayMs,
	LPCTSTR sensorIniPath)
{
	if (csvPath == NULL || csvPath[0] == 0)
		return false;

	try
	{
		const bool fileExists = (GetFileAttributes(csvPath) != INVALID_FILE_ATTRIBUTES);
		CFile f;
		if (!fileExists)
		{
			if (!f.Open(csvPath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
				return false;
			const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
			f.Write(bom, 3);
		}
		else
		{
			if (!f.Open(csvPath, CFile::modeWrite | CFile::modeNoTruncate | CFile::typeBinary))
				return false;
			f.SeekToEnd();
		}

		auto writeLine = [&](const CStringA& line) {
			f.Write(line.GetString(), (UINT)line.GetLength());
			f.Write("\r\n", 2);
		};

		if (!fileExists)
		{
			CStringA hdr;
			bool first = true;
			AppendCsvField(hdr, "Time", &first);
			AppendCsvField(hdr, "OverallResult", &first);
			AppendCsvField(hdr, "FailStage", &first);
			AppendCsvField(hdr, "GateSpecIni", &first);
			AppendCsvField(hdr, "SensorIni", &first);
			AppendCsvInt(hdr, delayMs, &first);
			AppendCsvField(hdr, "Dev", &first);
			AppendCsvField(hdr, "VC", &first);
			AppendCsvField(hdr, "ChannelResult", &first);
			AppendCsvField(hdr, "FwBurnResult", &first);
			AppendCsvField(hdr, "FwVerifyResult", &first);
			AppendCsvField(hdr, "FwBurnErrCode", &first);
			AppendCsvField(hdr, "SsrFps", &first);
			AppendCsvField(hdr, "SsrMin", &first);
			AppendCsvField(hdr, "SsrMax", &first);
			AppendCsvField(hdr, "SsrResult", &first);
			AppendCsvField(hdr, "Current_mA", &first);
			AppendCsvField(hdr, "CurMin", &first);
			AppendCsvField(hdr, "CurMax", &first);
			AppendCsvField(hdr, "CurResult", &first);
			AppendCsvField(hdr, "Temp_C", &first);
			AppendCsvField(hdr, "TempMin", &first);
			AppendCsvField(hdr, "TempMax", &first);
			AppendCsvField(hdr, "TempResult", &first);
			AppendCsvField(hdr, "BadPixelEnabled", &first);
			AppendCsvField(hdr, "BadCount", &first);
			AppendCsvField(hdr, "BadMaxAllow", &first);
			AppendCsvField(hdr, "FrameMean", &first);
			AppendCsvField(hdr, "BadPixelResult", &first);
			AppendCsvField(hdr, "ImageBmp", &first);
			AppendCsvField(hdr, "ImageRawUnpacked", &first);
			writeLine(hdr);
		}

		const CTime now = CTime::GetCurrentTime();
		CStringA timeA;
		timeA.Format("%04d-%02d-%02d %02d:%02d:%02d",
			now.GetYear(), now.GetMonth(), now.GetDay(),
			now.GetHour(), now.GetMinute(), now.GetSecond());
		const CStringA overallA(allPass ? "OK" : "NG");
		const CStringA gateA(gateSpecPath != NULL ? gateSpecPath : "");
		const CStringA sensorA(sensorIniPath != NULL ? sensorIniPath : "");

		int runFailStage = PROD_STAGE_OK;
		for (size_t i = 0; i < rows.size(); i++)
		{
			if (!rows[i].overallPass && rows[i].failStage != PROD_STAGE_OK)
			{
				runFailStage = rows[i].failStage;
				break;
			}
		}
		const CStringA failStageA(ProductionFailStageTag(runFailStage));

		for (size_t i = 0; i < rows.size(); i++)
		{
			const LightTestChannelRecord& r = rows[i];
			CStringA line;
			bool first = true;
			AppendCsvField(line, timeA, &first);
			if (i == 0)
				AppendCsvField(line, overallA, &first);
			else
				AppendCsvField(line, "", &first);
			if (i == 0)
				AppendCsvField(line, failStageA, &first);
			else
				AppendCsvField(line, "", &first);
			if (i == 0)
			{
				AppendCsvField(line, gateA, &first);
				AppendCsvField(line, sensorA, &first);
				AppendCsvInt(line, delayMs, &first);
			}
			else
			{
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
			}
			AppendCsvInt(line, r.devId, &first);
			AppendCsvInt(line, r.vcId, &first);
			AppendCsvField(line, r.overallPass ? "OK" : "NG", &first);
			AppendCsvField(line, FwResultTag(r.fwBurnEnabled, r.fwBurnTested, r.okFwBurn), &first);
			AppendCsvField(line, FwResultTag(r.fwVerifyEnabled, r.fwVerifyTested, r.okFwVerify), &first);
			if (r.fwBurnEnabled && r.fwBurnTested && !r.okFwBurn && r.fwBurnErrCode != 0)
				AppendCsvInt(line, r.fwBurnErrCode, &first);
			else
				AppendCsvField(line, "", &first);

			if (r.measureSkipped)
			{
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "SKIP", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "SKIP", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "SKIP", &first);
				AppendCsvField(line, r.badPixelEnabled ? "1" : "0", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "", &first);
				AppendCsvField(line, "SKIP", &first);
			}
			else
			{
				AppendCsvDouble(line, r.ssrFps, &first);
				AppendCsvDouble(line, r.ssrMin, &first);
				AppendCsvDouble(line, r.ssrMax, &first);
				AppendCsvField(line, TriResult(true, r.okSsr), &first);
				AppendCsvDouble(line, r.current_mA, &first);
				AppendCsvDouble(line, r.curMin, &first);
				AppendCsvDouble(line, r.curMax, &first);
				AppendCsvField(line, TriResult(true, r.okCur), &first);
				if (r.hasTemp)
					AppendCsvDouble(line, r.tempC, &first);
				else
					AppendCsvField(line, "N/A", &first);
				AppendCsvDouble(line, r.tempMin, &first);
				AppendCsvDouble(line, r.tempMax, &first);
				AppendCsvField(line, TriResult(true, r.okTemp), &first);
				AppendCsvField(line, r.badPixelEnabled ? "1" : "0", &first);
				AppendCsvUInt(line, r.badCount, &first);
				AppendCsvInt(line, r.badMaxAllow, &first);
				AppendCsvDouble(line, r.frameMean, &first);
				if (!r.badPixelEnabled)
					AppendCsvField(line, "N/A", &first);
				else if (!r.badAnalyzed)
					AppendCsvField(line, "NG", &first);
				else
					AppendCsvField(line, r.okBadPx ? "OK" : "NG", &first);
			}
			AppendCsvField(line, CStringA(r.imageBmp), &first);
			AppendCsvField(line, CStringA(r.imageRawUnpacked), &first);
			writeLine(line);
		}

		f.Close();
		return true;
	}
	catch (CFileException* e)
	{
		e->Delete();
		return false;
	}
}
