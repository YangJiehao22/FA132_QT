#include "stdafx.h"
#include "DtTcpNotify.h"
#include "DtFileOperate.h"
#include "DtEncoding.h"
#include "DtZhUtf8.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// TCP client: after all VC tests, notify lighting station (GateSpec.ini [tcp_notify]).
// Payload: {"Order":"Play","SNList":[]}\n  (UTF-8, newline terminated)

namespace {

static int GateIniIntLocal(LPCTSTR path, LPCTSTR section, LPCTSTR key, int defVal)
{
	return GetIniFileInt(section, key, defVal, path);
}

static bool EnsureWinsock()
{
	static volatile LONG s_started = 0;
	if (s_started != 0)
		return true;
	WSADATA wsa = {};
	const int r = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (r != 0)
		return false;
	InterlockedExchange(&s_started, 1);
	return true;
}

static SOCKET TcpConnectSocket(const GateTcpNotifyCfg& cfg)
{
	if (cfg.peerHost[0] == 0 || cfg.peerPort <= 0 || !EnsureWinsock())
		return INVALID_SOCKET;

	char portA[16] = {};
	_snprintf_s(portA, _TRUNCATE, "%d", cfg.peerPort);

	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	const CStringA hostA(CT2A(cfg.peerHost));
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

static bool TcpSendRecvPlay(const GateTcpNotifyCfg& cfg,
	const char* payload, int payloadLen, CStringA& outResponse)
{
	outResponse.Empty();
	if (payload == NULL || payloadLen <= 0)
		return false;

	SOCKET s = TcpConnectSocket(cfg);
	if (s == INVALID_SOCKET)
		return false;

	const int sndT = (cfg.sendTimeoutMs < 500) ? 500 : cfg.sendTimeoutMs;
	const int rcvT = (cfg.recvTimeoutMs < 500) ? 500 : cfg.recvTimeoutMs;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sndT, sizeof(sndT));
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvT, sizeof(rcvT));

	int sent = 0;
	while (sent < payloadLen)
	{
		const int n = send(s, payload + sent, payloadLen - sent, 0);
		if (n <= 0)
		{
			closesocket(s);
			return false;
		}
		sent += n;
	}

	if (!cfg.waitResponse)
	{
		closesocket(s);
		return true;
	}

	char buf[4096] = {};
	int total = 0;
	for (;;)
	{
		const int n = recv(s, buf + total, (int)sizeof(buf) - 1 - total, 0);
		if (n > 0)
		{
			total += n;
			buf[total] = 0;
			if (strchr(buf, '\n') != NULL)
				break;
			if (total >= (int)sizeof(buf) - 1)
				break;
			continue;
		}
		if (n == 0)
			break;
		const int err = WSAGetLastError();
		if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK)
			break;
		break;
	}
	closesocket(s);

	if (total > 0)
	{
		outResponse = buf;
		outResponse.Trim();
	}
	return true;
}

struct TcpNotifyWorkerParam
{
	GateTcpNotifyCfg cfg;
	CStringA payload;
};

unsigned __stdcall TcpNotifyWorkerProc(void* p)
{
	TcpNotifyWorkerParam* tp = (TcpNotifyWorkerParam*)p;
	if (tp == NULL)
		return 1;

	const int tries = (tp->cfg.retryCount < 1) ? 1 : (tp->cfg.retryCount + 1);
	bool ok = false;
	CStringA response;
	for (int i = 0; i < tries; i++)
	{
		if (TcpSendRecvPlay(tp->cfg, tp->payload.GetString(), tp->payload.GetLength(), response))
		{
			ok = true;
			break;
		}
		if (i + 1 < tries)
			Sleep(200);
	}

	const CStringA hostA(CT2A(tp->cfg.peerHost));
	if (ok)
	{
		if (!response.IsEmpty())
			msgUtf8(DtZh::kTcpNotifyResp, hostA.GetString(), tp->cfg.peerPort, response.GetString());
		else
			msgUtf8(DtZh::kTcpNotifyOk, hostA.GetString(), tp->cfg.peerPort, tp->payload.GetString());
	}
	else
		msgUtf8(DtZh::kTcpNotifyFail, hostA.GetString(), tp->cfg.peerPort);

	delete tp;
	return ok ? 0 : 1;
}

} // namespace

GateTcpNotifyCfg GateDefaultTcpNotify()
{
	GateTcpNotifyCfg c = {};
	c.enabled = false;
	c.closeBoxAfterTest = false;
	_tcsncpy_s(c.peerHost, _T("127.0.0.1"), _TRUNCATE);
	c.peerPort = 9000;
	c.onlyOnOverallOk = false;
	c.connectTimeoutMs = 3000;
	c.sendTimeoutMs = 2000;
	c.retryCount = 2;
	c.waitResponse = true;
	c.recvTimeoutMs = 3000;
	return c;
}

void GateIniFillTcpNotify(LPCTSTR path, const GateTcpNotifyCfg& fb, GateTcpNotifyCfg* out)
{
	if (out == NULL)
		return;
	const TCHAR* sec = _T("tcp_notify");
	*out = fb;
	out->enabled = (GateIniIntLocal(path, sec, _T("Enabled"), fb.enabled ? 1 : 0) != 0);
	out->closeBoxAfterTest = (GateIniIntLocal(path, sec, _T("CloseBoxAfterTest"), fb.closeBoxAfterTest ? 1 : 0) != 0);
	{
		CString host = GetIniFileString(sec, _T("PeerHost"), fb.peerHost, path);
		host.Trim();
		if (!host.IsEmpty())
			_tcsncpy_s(out->peerHost, host, _TRUNCATE);
	}
	out->peerPort = GateIniIntLocal(path, sec, _T("PeerPort"), fb.peerPort);
	if (out->peerPort <= 0 || out->peerPort > 65535)
		out->peerPort = 9000;
	out->onlyOnOverallOk = (GateIniIntLocal(path, sec, _T("OnlyOnOverallOk"), fb.onlyOnOverallOk ? 1 : 0) != 0);
	out->connectTimeoutMs = GateIniIntLocal(path, sec, _T("ConnectTimeoutMs"), fb.connectTimeoutMs);
	if (out->connectTimeoutMs < 500)
		out->connectTimeoutMs = 3000;
	out->sendTimeoutMs = GateIniIntLocal(path, sec, _T("SendTimeoutMs"), fb.sendTimeoutMs);
	if (out->sendTimeoutMs < 500)
		out->sendTimeoutMs = 2000;
	out->retryCount = GateIniIntLocal(path, sec, _T("RetryCount"), fb.retryCount);
	if (out->retryCount < 0)
		out->retryCount = 0;
	if (out->retryCount > 10)
		out->retryCount = 10;
	out->waitResponse = (GateIniIntLocal(path, sec, _T("WaitResponse"), fb.waitResponse ? 1 : 0) != 0);
	out->recvTimeoutMs = GateIniIntLocal(path, sec, _T("RecvTimeoutMs"), fb.recvTimeoutMs);
	if (out->recvTimeoutMs < 500)
		out->recvTimeoutMs = 3000;
}

void BuildTcpPlayOrderPayload(CStringA& outUtf8)
{
	outUtf8 = "{\"Order\":\"Play\",\"SNList\":[]}\n";
}

void TcpNotifyPostTestDoneAsync(const GateTcpNotifyCfg& cfg, bool allPass)
{
	if (!cfg.enabled)
		return;
	if (cfg.onlyOnOverallOk && !allPass)
	{
		msgUtf8(DtZh::kTcpNotifySkipNg);
		return;
	}
	if (cfg.peerHost[0] == 0 || cfg.peerPort <= 0)
	{
		msgUtf8(DtZh::kTcpNotifySkipCfg);
		return;
	}

	CStringA payload;
	BuildTcpPlayOrderPayload(payload);

	TcpNotifyWorkerParam* tp = new TcpNotifyWorkerParam;
	tp->cfg = cfg;
	tp->payload = payload;

	const CStringA hostA(CT2A(cfg.peerHost));
	msgUtf8(DtZh::kTcpNotifySendPlay, hostA.GetString(), cfg.peerPort, payload.GetString());

	unsigned tid = 0;
	HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &TcpNotifyWorkerProc, tp, 0, &tid);
	if (h == NULL)
	{
		delete tp;
		msgUtf8(DtZh::kTcpNotifyThreadFail);
		return;
	}
	CloseHandle(h);
}
