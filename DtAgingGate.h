#pragma once

#include "DtOvenModbus.h"
#include "DtLightTestReport.h"
#include <vector>

class DtCarFunction;

enum AgingChannelPhase
{
	AGING_PHASE_IDLE = 0,
	AGING_PHASE_ACTIVE,
	AGING_PHASE_SKIP,
	AGING_PHASE_PASSED,
	AGING_PHASE_FAILED,
};

struct AgingSampleSnapshot
{
	double ssrFps;
	double current_mA;
	double sensorTempC;
	double avdd_mV;
	double iovdd_mV;
	double dvdd_mV;
	bool hasTemp;
	bool hasVoltage;
	bool pass;
	CString failReason;
};

struct AgingChannelState
{
	AgingChannelPhase phase;
	bool everNg;
	AgingSampleSnapshot firstNg;
	AgingSampleSnapshot lastOk;
};

struct AgingChannelRecord
{
	int devId;
	int vcId;
	const char* resultTag;
	AgingSampleSnapshot snap;
};

void AgingInitAllChannels(DtCarFunction* fn);
void AgingMarkSkipFromLightTest(DtCarFunction* fn);
bool AgingAnyActiveChannel(const DtCarFunction* fn);
/** @param outAnyNewNg optional: set true if any channel newly entered AGING_PHASE_FAILED. */
bool AgingRunSampleRound(DtCarFunction* fn, bool* outAnyNewNg = NULL);
bool AgingIsNgCell(const DtCarFunction* fn, int devId, int vcId);
int AgingCountNgCells(const DtCarFunction* fn);
bool AgingEvaluateAllDone(const DtCarFunction* fn, bool* allPass);
void AgingAppendReportRows(DtCarFunction* fn, std::vector<LightTestChannelRecord>& rows);
void AgingLogStartSummary(const DtCarFunction* fn);
void AgingLogEndSummary(const DtCarFunction* fn, bool allPass);
