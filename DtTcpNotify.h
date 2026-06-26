#pragma once

// TCP client: notify lighting station after production test (GateSpec.ini [tcp_notify])
// Protocol: Order=Play, JSON UTF-8, line ends with \n

struct GateTcpNotifyCfg
{
	bool enabled;
	/** Requires enabled: Close test box after full run, before Play. */
	bool closeBoxAfterTest;
	TCHAR peerHost[256];
	int peerPort;
	bool onlyOnOverallOk;
	int connectTimeoutMs;
	int sendTimeoutMs;
	int retryCount;
	bool waitResponse;
	int recvTimeoutMs;
};

GateTcpNotifyCfg GateDefaultTcpNotify();

void GateIniFillTcpNotify(LPCTSTR path, const GateTcpNotifyCfg& defaults, GateTcpNotifyCfg* out);

void BuildTcpPlayOrderPayload(CStringA& outUtf8);

void TcpNotifyPostTestDoneAsync(const GateTcpNotifyCfg& cfg, bool allPass);
