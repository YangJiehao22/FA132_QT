#include "stdafx.h"
#include "DtAgingGate.h"
#include "DtCarFunction.h"
#include "DtFirmwareBurn.h"
#include "DtZhUtf8.h"

extern void msgUtf8(const char* utf8Fmt, ...);

static void AgingLogFirstNg(int d, int v, const AgingSampleSnapshot& s,
	bool laneOk, bool hasAvdd, bool hasIovdd, bool hasDvdd)
{
	CStringA failA(s.failReason);
	const char* laneS = laneOk ? "OK" : "FAIL";
	msgUtf8(DtZh::kLogAgingNgEvent,
		d, v, failA.GetString(), laneS,
		s.avdd_mV, hasAvdd ? "OK" : "FAIL",
		s.iovdd_mV, hasIovdd ? "OK" : "FAIL",
		s.dvdd_mV, hasDvdd ? "OK" : "FAIL",
		s.hasTemp ? s.sensorTempC : 0.0, s.hasTemp ? 1 : 0,
		s.ssrFps, s.current_mA);
}

static bool AgingReadReg16(int devId, unsigned char slave, unsigned short reg,
	unsigned char i2cMode, unsigned short* outVal)
{
	if (outVal == NULL)
		return false;
	return ::carReadSensorReg(slave, reg, outVal, i2cMode, devId) == 1;
}

static bool AgingReadVoltageMvRegs(int devId, unsigned char slave,
	const GateAgingVoltageI2cCfg& vcfg, unsigned short regLo, unsigned short regHi, double* outMv)
{
	if (outMv == NULL || !vcfg.enabled)
		return false;
	unsigned short vLo = 0;
	if (!AgingReadReg16(devId, slave, regLo, vcfg.i2cMode, &vLo))
		return false;
	unsigned raw16 = (unsigned)(vLo & 0xFF);
	if (regHi != 0)
	{
		unsigned short vHi = 0;
		if (!AgingReadReg16(devId, slave, regHi, vcfg.i2cMode, &vHi))
			return false;
		raw16 += (unsigned)(vHi & 0xFF) << 8;
	}
	const double sc = (vcfg.scale > 1.0) ? vcfg.scale : 1000.0;
	*outMv = (double)raw16 / sc * 1000.0;
	return true;
}

static bool AgingSampleOneChannel(DtCarFunction* fn, int d, int v, AgingSampleSnapshot* snap)
{
	if (fn == NULL || snap == NULL)
		return false;

	AgingChannelState& st = fn->m_agingState[d][v];
	if (st.phase == AGING_PHASE_SKIP || st.phase == AGING_PHASE_FAILED)
		return st.phase != AGING_PHASE_FAILED;

	const GateAgingGateCfg& G = fn->m_gateAgingGate;
	const GateChannelLimits& L = G.limits;

	::carGetChannelData(&fn->m_tVcData[d][v], v, d);
	const double ssr = fn->m_tVcData[d][v].dSsrFrameRate;
	const double cur_mA = fn->m_tVcData[d][v].iCurrent / 1000000.0;

	double tempC = 0.0;
	bool hasTemp = false;
	if (fn->m_gateSensorTempI2c.enabled)
	{
		if (Sony031ReadSensorTempC(d, v, fn->m_gateFirmwareBurn, fn->m_gateTempI2cAddr[d][v],
			fn->m_gateSensorTempI2c, &tempC, false))
		{
			hasTemp = true;
		}
	}

	double avdd = 0, iovdd = 0, dvdd = 0;
	bool hasAvdd = false;
	bool hasIovdd = false;
	bool hasDvdd = false;
	bool laneOk = false;
	bool hasV = false;
	const GateAgingVoltageI2cCfg& vcfg = fn->m_gateAgingVoltageI2c;
	if (vcfg.enabled)
	{
		const GateFirmwareBurnCfg& laneCfg = fn->m_gateFirmwareBurn;
		const unsigned char slave = fn->m_gateTempI2cAddr[d][v];
		laneOk = FirmwareSelectVcLane(d, v, laneCfg);
		if (laneOk)
		{
			hasAvdd = AgingReadVoltageMvRegs(d, slave, vcfg, vcfg.regAvdd, vcfg.regAvddHigh, &avdd);
			hasIovdd = AgingReadVoltageMvRegs(d, slave, vcfg, vcfg.regIovdd, vcfg.regIovddHigh, &iovdd);
			hasDvdd = AgingReadVoltageMvRegs(d, slave, vcfg, vcfg.regDvdd, vcfg.regDvddHigh, &dvdd);
			hasV = hasAvdd && hasIovdd && hasDvdd;
		}
	}

	const bool okSsr = (ssr >= L.minSsrFps) && (ssr <= L.maxSsrFps);
	const bool okCur = (cur_mA >= L.minCurrent_mA) && (cur_mA <= L.maxCurrent_mA);
	bool okTemp = true;
	if (fn->m_gateSensorTempI2c.enabled)
		okTemp = hasTemp && (tempC >= L.minSensorTemp_C) && (tempC <= L.maxSensorTemp_C);

	bool okAvdd = true;
	bool okIovdd = true;
	bool okDvdd = true;
	if (vcfg.enabled)
	{
		if (!hasV)
			okAvdd = okIovdd = okDvdd = false;
		else
		{
			okAvdd = (avdd / 1000.0 >= G.minAvdd_V) && (avdd / 1000.0 <= G.maxAvdd_V);
			okIovdd = (iovdd / 1000.0 >= G.minIovdd_V) && (iovdd / 1000.0 <= G.maxIovdd_V);
			okDvdd = (dvdd / 1000.0 >= G.minDvdd_V) && (dvdd / 1000.0 <= G.maxDvdd_V);
		}
	}
	const bool okVolt = okAvdd && okIovdd && okDvdd;
	const bool pass = okSsr && okCur && okTemp && (!vcfg.enabled || okVolt);

	AgingSampleSnapshot s = {};
	s.ssrFps = ssr;
	s.current_mA = cur_mA;
	s.sensorTempC = tempC;
	s.avdd_mV = avdd;
	s.iovdd_mV = iovdd;
	s.dvdd_mV = dvdd;
	s.hasTemp = hasTemp;
	s.hasVoltage = hasV;
	s.pass = pass;
	if (!okSsr)
		s.failReason = _T("SsrFps");
	else if (!okCur)
		s.failReason = _T("Current");
	else if (!okTemp)
		s.failReason = _T("SensorTemp");
	else if (vcfg.enabled && !hasV)
		s.failReason = _T("VoltageRead");
	else if (vcfg.enabled && !okAvdd)
		s.failReason = _T("Avdd");
	else if (vcfg.enabled && !okIovdd)
		s.failReason = _T("Iovdd");
	else if (vcfg.enabled && !okDvdd)
		s.failReason = _T("Dvdd");

	*snap = s;

	if (!pass)
	{
		if (!st.everNg)
		{
			AgingLogFirstNg(d, v, s, laneOk, hasAvdd, hasIovdd, hasDvdd);
			st.firstNg = s;
			st.everNg = true;
			st.phase = AGING_PHASE_FAILED;
		}
		return false;
	}

	st.lastOk = s;
	if (st.phase == AGING_PHASE_ACTIVE)
		st.phase = AGING_PHASE_PASSED;
	return true;
}

struct AgingThreadParam
{
	DtCarFunction* fn;
	int devId;
	int vcId;
	bool pass;
};

static unsigned __stdcall AgingThreadProc(void* p)
{
	AgingThreadParam* tp = (AgingThreadParam*)p;
	if (tp == NULL || tp->fn == NULL)
		return 1;
	AgingSampleSnapshot snap = {};
	tp->pass = AgingSampleOneChannel(tp->fn, tp->devId, tp->vcId, &snap);
	return tp->pass ? 0 : 1;
}

static void JoinAgingWorkerThreads(std::vector<HANDLE>& handles)
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

static bool AgingChannelNeedsSample(const DtCarFunction* fn, int d, int v)
{
	if (fn == NULL)
		return false;
	const AgingChannelPhase ph = fn->m_agingState[d][v].phase;
	return ph != AGING_PHASE_SKIP && ph != AGING_PHASE_FAILED;
}

void AgingInitAllChannels(DtCarFunction* fn)
{
	if (fn == NULL)
		return;
	for (int d = 0; d < MAX_CC16 * MAX_DEV; d++)
	{
		for (int v = 0; v < MAX_VC; v++)
		{
			fn->m_agingState[d][v].phase = AGING_PHASE_IDLE;
			fn->m_agingState[d][v].everNg = false;
		}
	}
}

void AgingMarkSkipFromLightTest(DtCarFunction* fn)
{
	if (fn == NULL)
		return;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			if (fn->m_bLightGateHasResult && !fn->m_bLightGatePass[d][v])
				fn->m_agingState[d][v].phase = AGING_PHASE_SKIP;
			else
				fn->m_agingState[d][v].phase = AGING_PHASE_ACTIVE;
		}
	}
}

bool AgingAnyActiveChannel(const DtCarFunction* fn)
{
	if (fn == NULL)
		return false;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			const AgingChannelPhase ph = fn->m_agingState[d][v].phase;
			if (ph == AGING_PHASE_ACTIVE || ph == AGING_PHASE_PASSED)
				return true;
		}
	}
	return false;
}

bool AgingRunSampleRound(DtCarFunction* fn, bool* outAnyNewNg)
{
	if (fn == NULL || fn->m_iEnumDevNum <= 0)
		return false;

	if (outAnyNewNg != NULL)
		*outAnyNewNg = false;

	const GateFirmwareBurnCfg& fwCfg = fn->m_gateFirmwareBurn;
	const bool needI2c = fn->m_gateSensorTempI2c.enabled || fn->m_gateAgingVoltageI2c.enabled;
	const int chipPhases = FirmwareChipPhaseCount(fwCfg);

	std::vector<AgingThreadParam*> params;
	bool allPass = true;

	for (int chipPhase = 0; chipPhase < chipPhases; chipPhase++)
	{
		if (needI2c)
		{
			for (int d = 0; d < fn->m_iEnumDevNum; d++)
			{
				if (!fn->IsDevEnabled(d))
					continue;
				(void)FirmwareBurnSetupDevI2c(d, fwCfg);
			}
		}

		std::vector<HANDLE> threads;
		for (int d = 0; d < fn->m_iEnumDevNum; d++)
		{
			if (!fn->IsDevEnabled(d))
				continue;
			for (int v = 0; v < fn->m_iVcNum; v++)
			{
				if (!fn->IsVcEnabled(d, v))
					continue;
				if (!VcOnFirmwareChipPhase(v, chipPhase, fwCfg))
					continue;
				if (!AgingChannelNeedsSample(fn, d, v))
					continue;

				AgingThreadParam* tp = new AgingThreadParam;
				tp->fn = fn;
				tp->devId = d;
				tp->vcId = v;
				tp->pass = true;
				unsigned tid = 0;
				HANDLE h = (HANDLE)_beginthreadex(NULL, 0, AgingThreadProc, tp, 0, &tid);
				if (h == NULL)
				{
					AgingSampleSnapshot snap = {};
					tp->pass = AgingSampleOneChannel(fn, d, v, &snap);
					params.push_back(tp);
					continue;
				}
				threads.push_back(h);
				params.push_back(tp);
			}
		}

		JoinAgingWorkerThreads(threads);
	}

	for (size_t i = 0; i < params.size(); i++)
	{
		if (params[i] != NULL)
		{
			if (!params[i]->pass)
			{
				allPass = false;
				if (outAnyNewNg != NULL)
					*outAnyNewNg = true;
			}
		}
		delete params[i];
	}
	return allPass;
}

bool AgingIsNgCell(const DtCarFunction* fn, int devId, int vcId)
{
	if (fn == NULL || !fn->m_gateAgingTest.enabled)
		return false;
	if (devId < 0 || devId >= MAX_CC16 * MAX_DEV || vcId < 0 || vcId >= MAX_VC)
		return false;
	return fn->m_agingState[devId][vcId].phase == AGING_PHASE_FAILED;
}

int AgingCountNgCells(const DtCarFunction* fn)
{
	if (fn == NULL || !fn->m_gateAgingTest.enabled)
		return 0;
	int n = 0;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			if (fn->m_agingState[d][v].phase == AGING_PHASE_FAILED)
				n++;
		}
	}
	return n;
}

bool AgingEvaluateAllDone(const DtCarFunction* fn, bool* allPass)
{
	if (fn == NULL)
		return false;
	bool ok = true;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			const AgingChannelState& st = fn->m_agingState[d][v];
			if (st.phase == AGING_PHASE_SKIP)
				continue;
			if (st.phase == AGING_PHASE_FAILED)
				ok = false;
		}
	}
	if (allPass)
		*allPass = (*allPass && ok);
	return true;
}

void AgingAppendReportRows(DtCarFunction* fn, std::vector<LightTestChannelRecord>& rows)
{
	if (fn == NULL)
		return;
	for (size_t i = 0; i < rows.size(); i++)
	{
		LightTestChannelRecord& rec = rows[i];
		const int d = rec.devId;
		const int v = rec.vcId;
		if (d < 0 || d >= MAX_CC16 * MAX_DEV || v < 0 || v >= MAX_VC)
			continue;
		const AgingChannelState& st = fn->m_agingState[d][v];
		rec.agingEnabled = fn->m_gateAgingTest.enabled;
		if (!rec.agingEnabled || st.phase == AGING_PHASE_SKIP)
		{
			rec.agingResult = AGING_CSV_SKIP;
			continue;
		}
		if (st.phase == AGING_PHASE_FAILED)
		{
			rec.agingResult = AGING_CSV_NG;
			rec.agingSsrFps = st.firstNg.ssrFps;
			rec.agingCurrent_mA = st.firstNg.current_mA;
			rec.agingTempC = st.firstNg.sensorTempC;
			rec.agingHasTemp = st.firstNg.hasTemp;
			rec.agingAvdd_mV = st.firstNg.avdd_mV;
			rec.agingIovdd_mV = st.firstNg.iovdd_mV;
			rec.agingDvdd_mV = st.firstNg.dvdd_mV;
			rec.agingFailReason = st.firstNg.failReason;
			if (rec.overallPass)
			{
				rec.overallPass = false;
				rec.failStage = PROD_STAGE_AGING;
			}
		}
		else
		{
			rec.agingResult = AGING_CSV_OK;
			rec.agingSsrFps = st.lastOk.ssrFps;
			rec.agingCurrent_mA = st.lastOk.current_mA;
			rec.agingTempC = st.lastOk.sensorTempC;
			rec.agingHasTemp = st.lastOk.hasTemp;
			rec.agingAvdd_mV = st.lastOk.avdd_mV;
			rec.agingIovdd_mV = st.lastOk.iovdd_mV;
			rec.agingDvdd_mV = st.lastOk.dvdd_mV;
		}
	}
}

static int AgingCountActiveChannels(const DtCarFunction* fn)
{
	if (fn == NULL)
		return 0;
	int n = 0;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			if (fn->m_agingState[d][v].phase == AGING_PHASE_SKIP)
				continue;
			n++;
		}
	}
	return n;
}

void AgingLogStartSummary(const DtCarFunction* fn)
{
	if (fn == NULL || !fn->m_gateAgingTest.enabled)
		return;
	const GateAgingGateCfg& G = fn->m_gateAgingGate;
	msgUtf8(DtZh::kLogAgingStartDetail,
		max(1, G.durationMin),
		max(5, G.sampleIntervalSec),
		AgingCountActiveChannels(fn),
		fn->m_gateSensorTempI2c.enabled ? 1 : 0,
		fn->m_gateAgingVoltageI2c.enabled ? 1 : 0);
}

void AgingLogEndSummary(const DtCarFunction* fn, bool allPass)
{
	if (fn == NULL || !fn->m_gateAgingTest.enabled)
		return;
	int total = 0;
	int ok = 0;
	int ng = 0;
	bool hasAnyTemp = false;
	double tMin = 0.0;
	double tMax = 0.0;
	for (int d = 0; d < fn->m_iEnumDevNum; d++)
	{
		if (!fn->IsDevEnabled(d))
			continue;
		for (int v = 0; v < fn->m_iVcNum; v++)
		{
			if (!fn->IsVcEnabled(d, v))
				continue;
			const AgingChannelState& st = fn->m_agingState[d][v];
			if (st.phase == AGING_PHASE_SKIP)
				continue;
			total++;
			if (st.phase == AGING_PHASE_FAILED)
			{
				ng++;
				if (st.firstNg.hasTemp)
				{
					const double t = st.firstNg.sensorTempC;
					if (!hasAnyTemp) { tMin = tMax = t; hasAnyTemp = true; }
					else { if (t < tMin) tMin = t; if (t > tMax) tMax = t; }
				}
			}
			else
			{
				ok++;
				if (st.lastOk.hasTemp)
				{
					const double t = st.lastOk.sensorTempC;
					if (!hasAnyTemp) { tMin = tMax = t; hasAnyTemp = true; }
					else { if (t < tMin) tMin = t; if (t > tMax) tMax = t; }
				}
			}
		}
	}
	(void)allPass;
	if (hasAnyTemp)
		msgUtf8(DtZh::kLogAgingEndDetail, ok, total, ng, tMin, tMax);
	else
		msgUtf8(DtZh::kLogAgingEndNoTemp, ok, total, ng);
}
