#include "stdafx.h"

#include "DtOvenModbus.h"

#include "DtFileOperate.h"

#include "DtEncoding.h"

#include "DtZhUtf8.h"



#include <winsock2.h>

#include <ws2tcpip.h>



#pragma comment(lib, "ws2_32.lib")



extern void msgUtf8(const char* utf8Fmt, ...);



namespace {



static int GateIniIntLocal(LPCTSTR path, LPCTSTR section, LPCTSTR key, int defVal)

{

	return GetIniFileInt(section, key, defVal, path);

}



static double GateIniDblLocal(LPCTSTR path, LPCTSTR section, LPCTSTR key, double defVal)

{

	CString s = GetIniFileString(section, key, _T(""), path);

	if (s.IsEmpty())

		return defVal;

	return atof(CStringA(s).GetString());

}



static bool EnsureWinsock()

{

	static volatile LONG s_started = 0;

	if (s_started != 0)

		return true;

	WSADATA wsa = {};

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)

		return false;

	InterlockedExchange(&s_started, 1);

	return true;

}



static SOCKET TcpConnect(const GateOvenCfg& cfg)

{

	if (cfg.host[0] == 0 || cfg.port <= 0 || !EnsureWinsock())

		return INVALID_SOCKET;



	char portA[16] = {};

	_snprintf_s(portA, _TRUNCATE, "%d", cfg.port);



	addrinfo hints = {};

	hints.ai_family = AF_UNSPEC;

	hints.ai_socktype = SOCK_STREAM;

	hints.ai_protocol = IPPROTO_TCP;



	const CStringA hostA(CT2A(cfg.host));

	addrinfo* res = NULL;

	if (getaddrinfo(hostA.GetString(), portA, &hints, &res) != 0 || res == NULL)

		return INVALID_SOCKET;



	SOCKET s = INVALID_SOCKET;

	for (addrinfo* p = res; p != NULL; p = p->ai_next)

	{

		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (s == INVALID_SOCKET)

			continue;



		u_long nb = 1;

		ioctlsocket(s, FIONBIO, &nb);

		const int cr = connect(s, p->ai_addr, (int)p->ai_addrlen);

		if (cr == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)

		{

			closesocket(s);

			s = INVALID_SOCKET;

			continue;

		}



		fd_set wfds;

		FD_ZERO(&wfds);

		FD_SET(s, &wfds);

		const int tms = (cfg.connectTimeoutMs < 500) ? 500 : cfg.connectTimeoutMs;

		timeval tv = {};

		tv.tv_sec = tms / 1000;

		tv.tv_usec = (tms % 1000) * 1000;

		if (select(0, NULL, &wfds, NULL, &tv) <= 0)

		{

			closesocket(s);

			s = INVALID_SOCKET;

			continue;

		}



		nb = 0;

		ioctlsocket(s, FIONBIO, &nb);

		break;

	}

	freeaddrinfo(res);

	return s;

}



static void SetSocketIoTimeout(SOCKET s, int ms)

{

	if (s == INVALID_SOCKET || ms <= 0)

		return;

	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ms, sizeof(ms));

	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ms, sizeof(ms));

}



static uint16_t TempToRaw(double tempC, int scale)

{

	const int sc = (scale > 0) ? scale : 10;

	return (uint16_t)(int16_t)(tempC * sc + (tempC >= 0 ? 0.5 : -0.5));

}



static bool ChamberUsesHeat(const GateOvenCfg& cfg, bool isU)

{

	if (cfg.heatMode == OVEN_HEAT_BOTH)

		return true;

	if (isU)

		return cfg.heatMode == OVEN_HEAT_U_ONLY;

	return cfg.heatMode == OVEN_HEAT_D_ONLY;

}



static bool PulseStart(COvenModbusClient& cli, const OvenChamberMap& m)

{

	return cli.WriteSingleRegister(m.regStart, 1);

}



static bool PulseStop(COvenModbusClient& cli, const OvenChamberMap& m)

{

	return cli.WriteSingleRegister(m.regStop, 1);

}



static bool CoilStart(COvenModbusClient& cli, const OvenChamberMap& m)

{

	return cli.WriteCoil(m.regStart, true);

}



static bool CoilStop(COvenModbusClient& cli, const OvenChamberMap& m)

{

	return cli.WriteCoil(m.regStop, true);

}



static void GateIniFillChamberMap(LPCTSTR path, LPCTSTR section, LPCTSTR prefix,

	const OvenChamberMap& fb, OvenChamberMap* out)

{

	CString k;

	k.Format(_T("%sSetTemp"), prefix);

	out->regSetTemp = GateIniIntLocal(path, section, k, fb.regSetTemp);

	k.Format(_T("%sStart"), prefix);

	out->regStart = GateIniIntLocal(path, section, k, fb.regStart);

	k.Format(_T("%sStop"), prefix);

	out->regStop = GateIniIntLocal(path, section, k, fb.regStop);

	k.Format(_T("%sPv"), prefix);

	out->regPv = GateIniIntLocal(path, section, k, fb.regPv);

	k.Format(_T("%sRunState"), prefix);

	out->regRunState = GateIniIntLocal(path, section, k, fb.regRunState);

	k.Format(_T("%sRunBit"), prefix);

	out->runStateBit = GateIniIntLocal(path, section, k, fb.runStateBit);

	k.Format(_T("%sFault"), prefix);

	out->regFault = GateIniIntLocal(path, section, k, fb.regFault);

}



static void GateIniFillChamberLegacy(LPCTSTR path, LPCTSTR section,

	const OvenChamberMap& fb, OvenChamberMap* out)

{

	OvenRegisterMap leg = {};

	OvenChamberMapToLegacy(fb, &leg);

	leg.regSetTemp = GateIniIntLocal(path, section, _T("RegSetTemp"), leg.regSetTemp);

	leg.coilStart = GateIniIntLocal(path, section, _T("CoilStart"), leg.coilStart);

	leg.coilStop = GateIniIntLocal(path, section, _T("CoilStop"), leg.coilStop);

	leg.regPv = GateIniIntLocal(path, section, _T("RegPv"), leg.regPv);

	leg.regRunState = GateIniIntLocal(path, section, _T("RegRunState"), leg.regRunState);

	leg.runStateBit = GateIniIntLocal(path, section, _T("RunStateBit"), leg.runStateBit);

	leg.regFault1 = GateIniIntLocal(path, section, _T("RegFault1"), leg.regFault1);

	OvenLegacyToChamberMap(leg, out);

}



} // namespace



GateAgingTestCfg GateDefaultAgingTest()

{

	GateAgingTestCfg c = {};

	c.heatAtStart = true;

	return c;

}



GateOvenCfg GateDefaultOvenCfg()

{

	GateOvenCfg c = {};

	c.enabled = true;

	c.profile = OVEN_PROFILE_S1200;

	c.port = 8000;

	_tcscpy_s(c.host, _T("192.168.1.10"));

	c.unitId = 1;

	c.tempScale = 10;

	c.targetC = 85.0;

	c.readyToleranceC = 2.0;

	c.waitTimeoutMin = 120;

	c.pollIntervalMs = 5000;

	c.connectTimeoutMs = 3000;

	c.ioTimeoutMs = 2000;

	c.retryCount = 2;

	c.dualChamber = true;

	c.heatMode = OVEN_HEAT_BOTH;

	c.cooldownEnabled = true;

	c.cooldownTargetC = 40.0;

	c.cooldownToleranceC = 2.0;

	c.cooldownTimeoutMin = 90;

	OvenApplyProfileDefaults(c.profile, &c);

	return c;

}



GateAgingGateCfg GateDefaultAgingGate()

{

	GateAgingGateCfg c = {};

	c.durationMin = 60;

	c.sampleIntervalSec = 30;

	c.limits.minSsrFps = 1.0;

	c.limits.maxSsrFps = 200.0;

	c.limits.minCurrent_mA = 0.0;

	c.limits.maxCurrent_mA = 2000.0;

	c.limits.minSensorTemp_C = -40.0;

	c.limits.maxSensorTemp_C = 125.0;

	c.minAvdd_V = 1.0;

	c.maxAvdd_V = 4.0;

	c.minIovdd_V = 1.0;

	c.maxIovdd_V = 4.0;

	c.minDvdd_V = 1.0;

	c.maxDvdd_V = 4.0;

	return c;

}



GateAgingVoltageI2cCfg GateDefaultAgingVoltageI2c()

{

	GateAgingVoltageI2cCfg c = {};

	c.enabled = true;

	c.i2cMode = 3;

	c.regAvdd = 0x1E40;

	c.regAvddHigh = 0x1E41;

	c.regIovdd = 0x1E42;

	c.regIovddHigh = 0x1E43;

	c.regDvdd = 0x1E44;

	c.regDvddHigh = 0x1E45;

	c.scale = 1000.0;

	return c;

}



void OvenApplyProfileDefaults(OvenProfileId profile, GateOvenCfg* cfg)

{

	if (cfg == NULL)

		return;

	if (profile == OVEN_PROFILE_X1M)

	{

		cfg->startStopKind = OVEN_SS_COIL;

		cfg->chamberU.regSetTemp = 8100;

		cfg->chamberU.regStart = 8000;

		cfg->chamberU.regStop = 8001;

		cfg->chamberU.regPv = 7991;

		cfg->chamberU.regRunState = 7990;

		cfg->chamberU.runStateBit = -1;

		cfg->chamberU.regFault = 0;

		cfg->chamberD.regSetTemp = 8101;

		cfg->chamberD.regStart = 8100;

		cfg->chamberD.regStop = 8101;

		cfg->chamberD.regPv = 7992;

		cfg->chamberD.regRunState = 7990;

		cfg->chamberD.runStateBit = -1;

		cfg->chamberD.regFault = 0;

	}

	else if (profile == OVEN_PROFILE_S1200)

	{

		cfg->startStopKind = OVEN_SS_REG_PULSE;

		cfg->chamberU.regSetTemp = 4;

		cfg->chamberU.regStart = 0;

		cfg->chamberU.regStop = 1;

		cfg->chamberU.regPv = 6;

		cfg->chamberU.regRunState = 2;

		cfg->chamberU.runStateBit = -2;

		cfg->chamberU.regFault = 3;

		cfg->chamberD.regSetTemp = 34;

		cfg->chamberD.regStart = 30;

		cfg->chamberD.regStop = 31;

		cfg->chamberD.regPv = 36;

		cfg->chamberD.regRunState = 32;

		cfg->chamberD.runStateBit = -2;

		cfg->chamberD.regFault = 33;

	}

	cfg->profile = profile;

}



OvenProfileId OvenProfileFromIniString(LPCTSTR s)

{

	if (s == NULL || s[0] == 0)

		return OVEN_PROFILE_S1200;

	CString t(s);

	t.Trim();

	if (t.CompareNoCase(_T("X1M")) == 0 || t.CompareNoCase(_T("X1M_V2")) == 0)

		return OVEN_PROFILE_X1M;

	if (t.CompareNoCase(_T("Custom")) == 0)

		return OVEN_PROFILE_CUSTOM;

	if (t.CompareNoCase(_T("Siemens_1200")) == 0 || t.CompareNoCase(_T("Siemens_HTH")) == 0

		|| t.CompareNoCase(_T("S1200")) == 0)

		return OVEN_PROFILE_S1200;

	return OVEN_PROFILE_S1200;

}



LPCTSTR OvenProfileIniString(OvenProfileId id)

{

	switch (id)

	{

	case OVEN_PROFILE_X1M: return _T("X1M");

	case OVEN_PROFILE_CUSTOM: return _T("Custom");

	default: return _T("Siemens_1200");

	}

}



OvenHeatMode OvenHeatModeFromIniString(LPCTSTR s)

{

	if (s == NULL || s[0] == 0)

		return OVEN_HEAT_BOTH;

	CString t(s);

	t.Trim();

	if (t.CompareNoCase(_T("UOnly")) == 0 || t.CompareNoCase(_T("U")) == 0)

		return OVEN_HEAT_U_ONLY;

	if (t.CompareNoCase(_T("DOnly")) == 0 || t.CompareNoCase(_T("D")) == 0)

		return OVEN_HEAT_D_ONLY;

	return OVEN_HEAT_BOTH;

}



LPCTSTR OvenHeatModeIniString(OvenHeatMode mode)

{

	switch (mode)

	{

	case OVEN_HEAT_U_ONLY: return _T("UOnly");

	case OVEN_HEAT_D_ONLY: return _T("DOnly");

	default: return _T("Both");

	}

}



void GateIniFillAgingTest(LPCTSTR path, const GateAgingTestCfg& fb, GateAgingTestCfg* out)

{

	out->enabled = (GateIniIntLocal(path, _T("aging_test"), _T("Enabled"), fb.enabled ? 1 : 0) != 0);

	out->heatAtStart = (GateIniIntLocal(path, _T("aging_test"), _T("HeatAtStart"), fb.heatAtStart ? 1 : 0) != 0);

}



void GateIniFillOvenCfg(LPCTSTR path, const GateOvenCfg& fb, GateOvenCfg* out)

{

	*out = fb;

	out->enabled = (GateIniIntLocal(path, _T("oven"), _T("Enabled"), fb.enabled ? 1 : 0) != 0);

	{

		CString prof = GetIniFileString(_T("oven"), _T("Profile"), OvenProfileIniString(fb.profile), path);

		out->profile = OvenProfileFromIniString(prof);

		GateOvenCfg def = fb;

		OvenApplyProfileDefaults(out->profile, &def);

		GateIniFillChamberMap(path, _T("oven"), _T("RegU"), def.chamberU, &out->chamberU);

		GateIniFillChamberMap(path, _T("oven"), _T("RegD"), def.chamberD, &out->chamberD);

		if (GetIniFileString(_T("oven"), _T("RegUSetTemp"), _T(""), path).IsEmpty()

			&& !GetIniFileString(_T("oven"), _T("RegSetTemp"), _T(""), path).IsEmpty())

			GateIniFillChamberLegacy(path, _T("oven"), def.chamberU, &out->chamberU);

		const int dPvLegacy = GateIniIntLocal(path, _T("oven_d"), _T("RegPv"), -1);

		if (GetIniFileString(_T("oven"), _T("RegDPv"), _T(""), path).IsEmpty() && dPvLegacy >= 0)

			out->chamberD.regPv = dPvLegacy;

	}

	{

		CString h = GetIniFileString(_T("oven"), _T("Host"), fb.host, path);

		h.Trim();

		_tcsncpy_s(out->host, h.GetString(), _TRUNCATE);

	}

	out->port = GateIniIntLocal(path, _T("oven"), _T("Port"), fb.port);

	out->unitId = GateIniIntLocal(path, _T("oven"), _T("UnitId"), fb.unitId);

	out->tempScale = GateIniIntLocal(path, _T("oven"), _T("TempScale"), fb.tempScale);

	if (out->tempScale < 1)

		out->tempScale = 10;

	out->targetC = GateIniDblLocal(path, _T("oven"), _T("TargetC"), fb.targetC);

	out->readyToleranceC = GateIniDblLocal(path, _T("oven"), _T("ReadyToleranceC"), fb.readyToleranceC);

	out->waitTimeoutMin = GateIniIntLocal(path, _T("oven"), _T("WaitTimeoutMin"), fb.waitTimeoutMin);

	out->pollIntervalMs = GateIniIntLocal(path, _T("oven"), _T("PollIntervalMs"), fb.pollIntervalMs);

	out->connectTimeoutMs = GateIniIntLocal(path, _T("oven"), _T("ConnectTimeoutMs"), fb.connectTimeoutMs);

	out->ioTimeoutMs = GateIniIntLocal(path, _T("oven"), _T("IoTimeoutMs"), fb.ioTimeoutMs);

	out->retryCount = GateIniIntLocal(path, _T("oven"), _T("RetryCount"), fb.retryCount);

	out->dualChamber = (GateIniIntLocal(path, _T("oven"), _T("DualChamber"), fb.dualChamber ? 1 : 0) != 0);

	if (GetIniFileString(_T("oven"), _T("DualChamber"), _T(""), path).IsEmpty())

	{

		const int dEnLegacy = GateIniIntLocal(path, _T("oven_d"), _T("Enabled"), -1);

		if (dEnLegacy >= 0)

			out->dualChamber = (dEnLegacy != 0);

	}

	{

		CString hm = GetIniFileString(_T("oven"), _T("HeatMode"), OvenHeatModeIniString(fb.heatMode), path);

		out->heatMode = OvenHeatModeFromIniString(hm);

	}

	out->cooldownEnabled = (GateIniIntLocal(path, _T("oven"), _T("CooldownEnabled"), fb.cooldownEnabled ? 1 : 0) != 0);

	out->cooldownTargetC = GateIniDblLocal(path, _T("oven"), _T("CooldownTargetC"), fb.cooldownTargetC);

	out->cooldownToleranceC = GateIniDblLocal(path, _T("oven"), _T("CooldownToleranceC"), fb.cooldownToleranceC);

	out->cooldownTimeoutMin = GateIniIntLocal(path, _T("oven"), _T("CooldownTimeoutMin"), fb.cooldownTimeoutMin);

	if (out->profile != OVEN_PROFILE_CUSTOM)

	{

		GateOvenCfg prof = *out;

		OvenApplyProfileDefaults(out->profile, &prof);

		if (GetIniFileString(_T("oven"), _T("RegUSetTemp"), _T(""), path).IsEmpty())

			out->chamberU = prof.chamberU;

		if (GetIniFileString(_T("oven"), _T("RegDSetTemp"), _T(""), path).IsEmpty())

			out->chamberD = prof.chamberD;

		out->startStopKind = prof.startStopKind;

	}

}



void GateIniFillAgingGate(LPCTSTR path, const GateAgingGateCfg& fb, const GateChannelLimits& defaultLimits, GateAgingGateCfg* out)

{

	out->durationMin = GateIniIntLocal(path, _T("aging_gate"), _T("DurationMin"), fb.durationMin);

	if (out->durationMin < 1)

		out->durationMin = 1;

	out->sampleIntervalSec = GateIniIntLocal(path, _T("aging_gate"), _T("SampleIntervalSec"), fb.sampleIntervalSec);

	if (out->sampleIntervalSec < 5)

		out->sampleIntervalSec = 5;



	GateChannelLimits lim = fb.limits;

	lim.minSsrFps = GateIniDblLocal(path, _T("aging_gate"), _T("MinSsrFps"), defaultLimits.minSsrFps);

	lim.maxSsrFps = GateIniDblLocal(path, _T("aging_gate"), _T("MaxSsrFps"), defaultLimits.maxSsrFps);

	lim.minCurrent_mA = GateIniDblLocal(path, _T("aging_gate"), _T("MinCurrent_mA"), defaultLimits.minCurrent_mA);

	lim.maxCurrent_mA = GateIniDblLocal(path, _T("aging_gate"), _T("MaxCurrent_mA"), defaultLimits.maxCurrent_mA);

	lim.minSensorTemp_C = GateIniDblLocal(path, _T("aging_gate"), _T("MinSensorTemp_C"), defaultLimits.minSensorTemp_C);

	lim.maxSensorTemp_C = GateIniDblLocal(path, _T("aging_gate"), _T("MaxSensorTemp_C"), defaultLimits.maxSensorTemp_C);

	out->limits = lim;

	out->minAvdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MinAvdd_V"), fb.minAvdd_V);

	out->maxAvdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MaxAvdd_V"), fb.maxAvdd_V);

	out->minIovdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MinIovdd_V"), fb.minIovdd_V);

	out->maxIovdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MaxIovdd_V"), fb.maxIovdd_V);

	out->minDvdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MinDvdd_V"), fb.minDvdd_V);

	out->maxDvdd_V = GateIniDblLocal(path, _T("aging_gate"), _T("MaxDvdd_V"), fb.maxDvdd_V);

}



void GateIniFillAgingVoltageI2c(LPCTSTR path, const GateAgingVoltageI2cCfg& fb, GateAgingVoltageI2cCfg* out)

{

	out->enabled = (GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("Enabled"), fb.enabled ? 1 : 0) != 0);

	out->i2cMode = (unsigned char)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("I2cMode"), fb.i2cMode);

	out->regAvdd = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegAvdd"), fb.regAvdd);

	out->regAvddHigh = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegAvddHigh"), fb.regAvddHigh);

	out->regIovdd = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegIovdd"), fb.regIovdd);

	out->regIovddHigh = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegIovddHigh"), fb.regIovddHigh);

	out->regDvdd = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegDvdd"), fb.regDvdd);

	out->regDvddHigh = (unsigned short)GateIniIntLocal(path, _T("aging_voltage_i2c"), _T("RegDvddHigh"), fb.regDvddHigh);

	out->scale = GateIniDblLocal(path, _T("aging_voltage_i2c"), _T("Scale"), fb.scale);

	if (out->scale < 1.0)

		out->scale = 1000.0;

}



void OvenChamberMapToLegacy(const OvenChamberMap& m, OvenRegisterMap* out)

{

	if (out == NULL)

		return;

	out->regSetTemp = m.regSetTemp;

	out->coilStart = m.regStart;

	out->coilStop = m.regStop;

	out->regPv = m.regPv;

	out->regRunState = m.regRunState;

	out->runStateBit = m.runStateBit;

	out->regFault1 = m.regFault;

}



void OvenLegacyToChamberMap(const OvenRegisterMap& leg, OvenChamberMap* out)

{

	if (out == NULL)

		return;

	out->regSetTemp = leg.regSetTemp;

	out->regStart = leg.coilStart;

	out->regStop = leg.coilStop;

	out->regPv = leg.regPv;

	out->regRunState = leg.regRunState;

	out->runStateBit = leg.runStateBit;

	out->regFault = leg.regFault1;

}



COvenModbusClient::COvenModbusClient()

	: m_socket(INVALID_SOCKET)

	, m_transactionId(0)

{

	memset(&m_cfg, 0, sizeof(m_cfg));

}



COvenModbusClient::~COvenModbusClient()

{

	Disconnect();

}



void COvenModbusClient::Disconnect()

{

	if (m_socket != INVALID_SOCKET)

	{

		closesocket(m_socket);

		m_socket = INVALID_SOCKET;

	}

}



bool COvenModbusClient::Connect(const GateOvenCfg& cfg)

{

	Disconnect();

	m_cfg = cfg;

	m_socket = TcpConnect(cfg);

	if (m_socket == INVALID_SOCKET)

		return false;

	SetSocketIoTimeout(m_socket, cfg.ioTimeoutMs > 0 ? cfg.ioTimeoutMs : 2000);

	return true;

}



bool COvenModbusClient::WriteExact(const uint8_t* buf, int len)

{

	if (m_socket == INVALID_SOCKET || buf == NULL || len <= 0)

		return false;

	int sent = 0;

	while (sent < len)

	{

		const int n = send(m_socket, (const char*)buf + sent, len - sent, 0);

		if (n <= 0)

			return false;

		sent += n;

	}

	return true;

}



bool COvenModbusClient::ReadExact(uint8_t* buf, int len)

{

	if (m_socket == INVALID_SOCKET || buf == NULL || len <= 0)

		return false;

	int got = 0;

	while (got < len)

	{

		const int n = recv(m_socket, (char*)buf + got, len - got, 0);

		if (n <= 0)

			return false;

		got += n;

	}

	return true;

}



bool COvenModbusClient::Transact(const uint8_t* req, int reqLen, uint8_t* rsp, int rspCap, int* rspLen)

{

	if (req == NULL || reqLen < 8 || rsp == NULL || rspCap < 9)

		return false;



	for (int attempt = 0; attempt <= m_cfg.retryCount; attempt++)

	{

		if (m_socket == INVALID_SOCKET && !Connect(m_cfg))

			continue;

		if (!WriteExact(req, reqLen))

		{

			Disconnect();

			continue;

		}



		uint8_t mbap[6] = {};

		if (!ReadExact(mbap, 6))

		{

			Disconnect();

			continue;

		}

		const int pduLen = ((int)mbap[4] << 8) | mbap[5];

		if (pduLen <= 0 || pduLen + 6 > rspCap)

		{

			Disconnect();

			continue;

		}

		memcpy(rsp, mbap, 6);

		if (!ReadExact(rsp + 6, pduLen))

		{

			Disconnect();

			continue;

		}

		if (rspLen)

			*rspLen = pduLen + 6;

		return true;

	}

	return false;

}



bool COvenModbusClient::WriteSingleRegister(int addr, uint16_t value)

{

	uint8_t req[12] = {};

	const uint16_t tid = ++m_transactionId;

	req[0] = (uint8_t)(tid >> 8);

	req[1] = (uint8_t)(tid & 0xFF);

	req[4] = 0;

	req[5] = 6;

	req[6] = (uint8_t)m_cfg.unitId;

	req[7] = 0x06;

	req[8] = (uint8_t)(addr >> 8);

	req[9] = (uint8_t)(addr & 0xFF);

	req[10] = (uint8_t)(value >> 8);

	req[11] = (uint8_t)(value & 0xFF);



	uint8_t rsp[256] = {};

	int rspLen = 0;

	if (!Transact(req, 12, rsp, (int)sizeof(rsp), &rspLen))

		return false;

	return rspLen >= 12 && rsp[7] == 0x06;

}



bool COvenModbusClient::WriteCoil(int addr, bool on)

{

	uint8_t req[12] = {};

	const uint16_t tid = ++m_transactionId;

	req[0] = (uint8_t)(tid >> 8);

	req[1] = (uint8_t)(tid & 0xFF);

	req[4] = 0;

	req[5] = 6;

	req[6] = (uint8_t)m_cfg.unitId;

	req[7] = 0x05;

	req[8] = (uint8_t)(addr >> 8);

	req[9] = (uint8_t)(addr & 0xFF);

	req[10] = on ? 0xFF : 0x00;

	req[11] = 0x00;



	uint8_t rsp[256] = {};

	int rspLen = 0;

	if (!Transact(req, 12, rsp, (int)sizeof(rsp), &rspLen))

		return false;

	return rspLen >= 12 && rsp[7] == 0x05;

}



bool COvenModbusClient::ReadHoldingRegisters(int addr, int count, uint16_t* out)

{

	if (out == NULL || count <= 0 || count > 32)

		return false;



	uint8_t req[12] = {};

	const uint16_t tid = ++m_transactionId;

	req[0] = (uint8_t)(tid >> 8);

	req[1] = (uint8_t)(tid & 0xFF);

	req[4] = 0;

	req[5] = 6;

	req[6] = (uint8_t)m_cfg.unitId;

	req[7] = 0x03;

	req[8] = (uint8_t)(addr >> 8);

	req[9] = (uint8_t)(addr & 0xFF);

	req[10] = (uint8_t)(count >> 8);

	req[11] = (uint8_t)(count & 0xFF);



	uint8_t rsp[256] = {};

	int rspLen = 0;

	if (!Transact(req, 12, rsp, (int)sizeof(rsp), &rspLen))

		return false;

	if (rspLen < 9 || rsp[7] != 0x03)

		return false;

	const int byteCount = rsp[8];

	if (byteCount != count * 2 || rspLen < 9 + byteCount)

		return false;

	for (int i = 0; i < count; i++)

		out[i] = ((uint16_t)rsp[9 + i * 2] << 8) | rsp[10 + i * 2];

	return true;

}



bool COvenModbusClient::ReadTemperatureC(int reg, int tempScale, double* outC)

{

	if (outC == NULL)

		return false;

	uint16_t v = 0;

	if (!ReadHoldingRegisters(reg, 1, &v))

		return false;

	const double sc = (tempScale > 0) ? (double)tempScale : 10.0;

	*outC = (double)v / sc;

	return true;

}



bool OvenReadChamberRunState(COvenModbusClient& cli, const GateOvenCfg& cfg,

	const OvenChamberMap& map, bool* running, bool* fault)

{

	if (running)

		*running = false;

	if (fault)

		*fault = false;



	uint16_t v = 0;

	if (!cli.ReadHoldingRegisters(map.regRunState, 1, &v))

		return false;



	if (map.runStateBit == -2)

	{

		if (fault)

			*fault = (v == 2);

		if (running)

			*running = (v == 1);

	}

	else if (map.runStateBit >= 0)

	{

		if (running)

			*running = ((v >> map.runStateBit) & 1) != 0;

	}

	else if (running)

		*running = (v != 0);



	if (fault && map.regFault > 0)

	{

		uint16_t f = 0;

		if (cli.ReadHoldingRegisters(map.regFault, 1, &f) && f != 0)

			*fault = true;

	}

	return true;

}



bool OvenReadChamberPv(COvenModbusClient& cli, const GateOvenCfg& cfg,

	const OvenChamberMap& map, double* pvC)

{

	return cli.ReadTemperatureC(map.regPv, cfg.tempScale, pvC);

}



bool OvenSetTargetTemp(const GateOvenCfg& cfg, double tempC, OvenHeatMode chambers)

{

	COvenModbusClient cli;

	if (!cli.Connect(cfg))

		return false;

	const uint16_t raw = TempToRaw(tempC, cfg.tempScale);

	bool ok = true;

	if (chambers == OVEN_HEAT_BOTH || chambers == OVEN_HEAT_U_ONLY)

		ok = cli.WriteSingleRegister(cfg.chamberU.regSetTemp, raw) && ok;

	if (chambers == OVEN_HEAT_BOTH || chambers == OVEN_HEAT_D_ONLY)

		ok = cli.WriteSingleRegister(cfg.chamberD.regSetTemp, raw) && ok;

	return ok;

}



bool OvenBeginCooldown(const GateOvenCfg& cfg)

{

	if (!cfg.enabled || !cfg.cooldownEnabled)

		return false;

	const bool ok = OvenSetTargetTemp(cfg, cfg.cooldownTargetC, cfg.heatMode);

	if (ok)

		msgUtf8(DtZh::kOvenCooldownSetTemp, cfg.cooldownTargetC);

	return ok;

}



static bool StartChamber(COvenModbusClient& cli, const GateOvenCfg& cfg, const OvenChamberMap& m)

{

	if (cfg.startStopKind == OVEN_SS_COIL)

		return CoilStart(cli, m);

	return PulseStart(cli, m);

}



static bool StopChamber(COvenModbusClient& cli, const GateOvenCfg& cfg, const OvenChamberMap& m)

{

	if (cfg.startStopKind == OVEN_SS_COIL)

		return CoilStop(cli, m);

	return PulseStop(cli, m);

}



bool OvenHeatStart(const GateOvenCfg& cfg)

{

	if (!cfg.enabled)

		return false;

	COvenModbusClient cli;

	if (!cli.Connect(cfg))

		return false;

	const uint16_t raw = TempToRaw(cfg.targetC, cfg.tempScale);

	bool ok = true;

	if (ChamberUsesHeat(cfg, true))

		ok = cli.WriteSingleRegister(cfg.chamberU.regSetTemp, raw) && ok;

	if (ChamberUsesHeat(cfg, false) && cfg.dualChamber)

		ok = cli.WriteSingleRegister(cfg.chamberD.regSetTemp, raw) && ok;

	if (!ok)

		return false;

	if (ChamberUsesHeat(cfg, true))

		ok = StartChamber(cli, cfg, cfg.chamberU) && ok;

	if (ChamberUsesHeat(cfg, false) && cfg.dualChamber)

		ok = StartChamber(cli, cfg, cfg.chamberD) && ok;

	msgUtf8(DtZh::kOvenHeatStart, cfg.host, cfg.port, cfg.targetC);

	return ok;

}



bool OvenStopOnly(const GateOvenCfg& cfg)

{

	if (!cfg.enabled)

		return true;

	COvenModbusClient cli;

	if (!cli.Connect(cfg))

		return false;

	bool ok = true;

	if (ChamberUsesHeat(cfg, true))

		ok = StopChamber(cli, cfg, cfg.chamberU) && ok;

	if (cfg.dualChamber && (cfg.heatMode == OVEN_HEAT_BOTH || cfg.heatMode == OVEN_HEAT_D_ONLY))

		ok = StopChamber(cli, cfg, cfg.chamberD) && ok;

	msgUtf8(DtZh::kOvenStop, cfg.host, cfg.port);

	return ok;

}



static bool EvalHeatReady(const GateOvenCfg& cfg, COvenModbusClient& cli,

	double* pvU, double* pvD, bool* fault, bool* ready)

{

	if (ready)

		*ready = false;

	if (fault)

		*fault = false;



	bool uRun = false, uFlt = false, dRun = false, dFlt = false;

	if (!OvenReadChamberRunState(cli, cfg, cfg.chamberU, &uRun, &uFlt))

		return false;

	if (cfg.dualChamber)

	{

		if (!OvenReadChamberRunState(cli, cfg, cfg.chamberD, &dRun, &dFlt))

			return false;

	}

	else

	{

		dRun = true;

		dFlt = false;

	}



	const bool flt = uFlt || dFlt;

	if (fault)

		*fault = flt;

	if (flt)

		return true;



	double pv1 = 0.0, pv2 = 0.0;

	if (!OvenReadChamberPv(cli, cfg, cfg.chamberU, &pv1))

		return false;

	if (pvU)

		*pvU = pv1;



	bool dOk = true;

	if (cfg.dualChamber)

	{

		if (!OvenReadChamberPv(cli, cfg, cfg.chamberD, &pv2))

			return false;

		dOk = (pv2 >= cfg.targetC - cfg.readyToleranceC);

		if (pvD)

			*pvD = pv2;

	}

	else if (pvD)

		*pvD = 0.0;



	const double minPv = cfg.targetC - cfg.readyToleranceC;

	const bool uOk = (pv1 >= minPv);

	const bool runOk = uRun && (dRun || !cfg.dualChamber);

	if (ready)

		*ready = runOk && !flt && uOk && dOk;

	return true;

}



bool OvenWaitHeatReadyOnce(const GateOvenCfg& cfg, bool* ready, double* pvU, double* pvD, bool* fault)

{

	if (!cfg.enabled)

		return false;

	COvenModbusClient cli;

	if (!cli.Connect(cfg))

		return false;

	return EvalHeatReady(cfg, cli, pvU, pvD, fault, ready);

}



bool OvenWaitCooldownOnce(const GateOvenCfg& cfg, bool* cooled, double* pvU, double* pvD, bool* fault)

{

	if (cooled)

		*cooled = false;

	if (!cfg.enabled)

		return false;

	COvenModbusClient cli;

	if (!cli.Connect(cfg))

		return false;



	bool uFlt = false, dFlt = false;

	if (!OvenReadChamberRunState(cli, cfg, cfg.chamberU, NULL, &uFlt))

		return false;

	if (cfg.dualChamber)

	{

		if (!OvenReadChamberRunState(cli, cfg, cfg.chamberD, NULL, &dFlt))

			return false;

	}

	if (fault)

		*fault = uFlt || dFlt;



	double pv1 = 0.0, pv2 = 0.0;

	if (!OvenReadChamberPv(cli, cfg, cfg.chamberU, &pv1))

		return false;

	if (pvU)

		*pvU = pv1;



	bool dOk = true;

	if (cfg.dualChamber)

	{

		if (!OvenReadChamberPv(cli, cfg, cfg.chamberD, &pv2))

			return false;

		if (pvD)

			*pvD = pv2;

		dOk = (pv2 <= cfg.cooldownTargetC + cfg.cooldownToleranceC);

	}

	else if (pvD)

		*pvD = 0.0;



	const double maxPv = cfg.cooldownTargetC + cfg.cooldownToleranceC;

	const bool uOk = (pv1 <= maxPv);

	if (cooled)

		*cooled = uOk && dOk;

	return true;

}



