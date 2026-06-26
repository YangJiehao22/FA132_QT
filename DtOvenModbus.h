#pragma once



#include <stdint.h>

#include "DtGateLimits.h"



#ifndef _WINSOCK2API_

#include <winsock2.h>

#endif



/** Modbus TCP oven: Siemens 1200 dual-box + X1M dual-chamber (single connection). */



enum OvenProfileId

{

	OVEN_PROFILE_S1200 = 0,

	OVEN_PROFILE_X1M = 1,

	OVEN_PROFILE_CUSTOM = 2,

};



enum OvenHeatMode

{

	OVEN_HEAT_BOTH = 0,

	OVEN_HEAT_U_ONLY = 1,

	OVEN_HEAT_D_ONLY = 2,

};



/** Start/stop: X1M=coil; Siemens1200=holding register pulse (write 1). */

enum OvenStartStopKind

{

	OVEN_SS_COIL = 0,

	OVEN_SS_REG_PULSE = 1,

};



/** runStateBit: >=0 bit test; -1 reg!=0 running; -2 Siemens1200 0=stop 1=run 2=fault */

struct OvenChamberMap

{

	int regSetTemp;

	int regStart;

	int regStop;

	int regPv;

	int regRunState;

	int runStateBit;

	int regFault;

};



struct GateOvenCfg

{

	bool enabled;

	OvenProfileId profile;

	OvenStartStopKind startStopKind;

	TCHAR host[64];

	int port;

	int unitId;

	int tempScale;

	double targetC;

	double readyToleranceC;

	int waitTimeoutMin;

	int pollIntervalMs;

	int connectTimeoutMs;

	int ioTimeoutMs;

	int retryCount;

	bool dualChamber;

	OvenHeatMode heatMode;

	OvenChamberMap chamberU;

	OvenChamberMap chamberD;

	bool cooldownEnabled;

	double cooldownTargetC;

	double cooldownToleranceC;

	int cooldownTimeoutMin;

};



struct GateAgingTestCfg

{

	bool enabled;

	bool heatAtStart;

};



struct GateAgingGateCfg

{

	int durationMin;

	int sampleIntervalSec;

	GateChannelLimits limits;

	double minAvdd_V;

	double maxAvdd_V;

	double minIovdd_V;

	double maxIovdd_V;

	double minDvdd_V;

	double maxDvdd_V;

};



struct GateAgingVoltageI2cCfg

{

	bool enabled;

	unsigned char i2cMode;

	unsigned short regAvdd;

	unsigned short regAvddHigh;

	unsigned short regIovdd;

	unsigned short regIovddHigh;

	unsigned short regDvdd;

	unsigned short regDvddHigh;

	double scale;

};



GateAgingTestCfg GateDefaultAgingTest();

GateOvenCfg GateDefaultOvenCfg();

GateAgingGateCfg GateDefaultAgingGate();

GateAgingVoltageI2cCfg GateDefaultAgingVoltageI2c();



void OvenApplyProfileDefaults(OvenProfileId profile, GateOvenCfg* cfg);

OvenProfileId OvenProfileFromIniString(LPCTSTR s);

LPCTSTR OvenProfileIniString(OvenProfileId id);

OvenHeatMode OvenHeatModeFromIniString(LPCTSTR s);

LPCTSTR OvenHeatModeIniString(OvenHeatMode mode);



void GateIniFillAgingTest(LPCTSTR path, const GateAgingTestCfg& fb, GateAgingTestCfg* out);

void GateIniFillOvenCfg(LPCTSTR path, const GateOvenCfg& fb, GateOvenCfg* out);

void GateIniFillAgingGate(LPCTSTR path, const GateAgingGateCfg& fb, const GateChannelLimits& defaultLimits, GateAgingGateCfg* out);

void GateIniFillAgingVoltageI2c(LPCTSTR path, const GateAgingVoltageI2cCfg& fb, GateAgingVoltageI2cCfg* out);



class COvenModbusClient

{

public:

	COvenModbusClient();

	~COvenModbusClient();



	bool Connect(const GateOvenCfg& cfg);

	void Disconnect();

	bool IsConnected() const { return m_socket != INVALID_SOCKET; }



	bool WriteSingleRegister(int addr, uint16_t value);

	bool WriteCoil(int addr, bool on);

	bool ReadHoldingRegisters(int addr, int count, uint16_t* out);

	bool ReadTemperatureC(int reg, int tempScale, double* outC);



private:

	bool Transact(const uint8_t* req, int reqLen, uint8_t* rsp, int rspCap, int* rspLen);

	bool ReadExact(uint8_t* buf, int len);

	bool WriteExact(const uint8_t* buf, int len);



	SOCKET m_socket;

	GateOvenCfg m_cfg;

	uint16_t m_transactionId;

};



bool OvenReadChamberRunState(COvenModbusClient& cli, const GateOvenCfg& cfg,

	const OvenChamberMap& map, bool* running, bool* fault);

bool OvenReadChamberPv(COvenModbusClient& cli, const GateOvenCfg& cfg,

	const OvenChamberMap& map, double* pvC);



bool OvenHeatStart(const GateOvenCfg& cfg);

bool OvenStopOnly(const GateOvenCfg& cfg);

bool OvenSetTargetTemp(const GateOvenCfg& cfg, double tempC, OvenHeatMode chambers);

/** After aging: write CooldownTargetC to setTemp regs (cooldown command for oven / mock). */
bool OvenBeginCooldown(const GateOvenCfg& cfg);

bool OvenWaitHeatReadyOnce(const GateOvenCfg& cfg, bool* ready, double* pvU, double* pvD, bool* fault);

bool OvenWaitCooldownOnce(const GateOvenCfg& cfg, bool* cooled, double* pvU, double* pvD, bool* fault);



/** Legacy alias for chamber-U advanced UI fields. */

struct OvenRegisterMap

{

	int regSetTemp;

	int coilStart;

	int coilStop;

	int regPv;

	int regRunState;

	int runStateBit;

	int regFault1;

};



void OvenChamberMapToLegacy(const OvenChamberMap& m, OvenRegisterMap* out);

void OvenLegacyToChamberMap(const OvenRegisterMap& leg, OvenChamberMap* out);


