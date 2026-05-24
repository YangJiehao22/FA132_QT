#include "stdafx.h"
#include "DtLightTestReport.h"

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

} // namespace

bool WriteLightTestReportCsv(
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
			AppendCsvField(hdr, "GateSpecIni", &first);
			AppendCsvField(hdr, "SensorIni", &first);
			AppendCsvInt(hdr, delayMs, &first);
			AppendCsvField(hdr, "Dev", &first);
			AppendCsvField(hdr, "VC", &first);
			AppendCsvField(hdr, "ChannelResult", &first);
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
			AppendCsvDouble(line, r.ssrFps, &first);
			AppendCsvDouble(line, r.ssrMin, &first);
			AppendCsvDouble(line, r.ssrMax, &first);
			AppendCsvField(line, r.okSsr ? "OK" : "NG", &first);
			AppendCsvDouble(line, r.current_mA, &first);
			AppendCsvDouble(line, r.curMin, &first);
			AppendCsvDouble(line, r.curMax, &first);
			AppendCsvField(line, r.okCur ? "OK" : "NG", &first);
			if (r.hasTemp)
				AppendCsvDouble(line, r.tempC, &first);
			else
				AppendCsvField(line, "N/A", &first);
			AppendCsvDouble(line, r.tempMin, &first);
			AppendCsvDouble(line, r.tempMax, &first);
			AppendCsvField(line, r.okTemp ? "OK" : "NG", &first);
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
