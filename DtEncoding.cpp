#include "stdafx.h"
#include "DtEncoding.h"
#include "DtCarFunction.h"

#include <stdarg.h>

CStringA Utf8ToAcp(const char* utf8, int byteLen)
{
	CStringA empty;
	if (utf8 == NULL || utf8[0] == 0)
		return empty;

	const int srcLen = (byteLen < 0) ? (int)strlen(utf8) : byteLen;
	if (srcLen <= 0)
		return empty;

	const int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, srcLen, NULL, 0);
	if (wlen <= 0)
		return CStringA(utf8);

	CStringW wide;
	LPWSTR wbuf = wide.GetBuffer(wlen);
	MultiByteToWideChar(CP_UTF8, 0, utf8, srcLen, wbuf, wlen);
	wide.ReleaseBuffer(wlen);

	const int alen = WideCharToMultiByte(CP_ACP, 0, wide, wide.GetLength(), NULL, 0, NULL, NULL);
	if (alen <= 0)
		return CStringA(utf8);

	CStringA acp;
	LPSTR abuf = acp.GetBuffer(alen);
	WideCharToMultiByte(CP_ACP, 0, wide, wide.GetLength(), abuf, alen, NULL, NULL);
	acp.ReleaseBuffer(alen);
	return acp;
}

CStringA AcpToUtf8(const char* acp, int byteLen)
{
	CStringA empty;
	if (acp == NULL || acp[0] == 0)
		return empty;

	const int srcLen = (byteLen < 0) ? (int)strlen(acp) : byteLen;
	if (srcLen <= 0)
		return empty;

	const int wlen = MultiByteToWideChar(CP_ACP, 0, acp, srcLen, NULL, 0);
	if (wlen <= 0)
		return CStringA(acp);

	CStringW wide;
	LPWSTR wbuf = wide.GetBuffer(wlen);
	MultiByteToWideChar(CP_ACP, 0, acp, srcLen, wbuf, wlen);
	wide.ReleaseBuffer(wlen);

	const int u8len = WideCharToMultiByte(CP_UTF8, 0, wide, wide.GetLength(), NULL, 0, NULL, NULL);
	if (u8len <= 0)
		return CStringA(acp);

	CStringA utf8;
	LPSTR u8buf = utf8.GetBuffer(u8len);
	WideCharToMultiByte(CP_UTF8, 0, wide, wide.GetLength(), u8buf, u8len, NULL, NULL);
	utf8.ReleaseBuffer(u8len);
	return utf8;
}

CStringA WideToUtf8A(LPCTSTR widePath)
{
	CStringA empty;
	if (widePath == NULL || widePath[0] == 0)
		return empty;
#ifdef _UNICODE
	const int u8len = WideCharToMultiByte(CP_UTF8, 0, widePath, -1, NULL, 0, NULL, NULL);
	if (u8len <= 1)
		return empty;
	CStringA utf8;
	LPSTR p = utf8.GetBuffer(u8len - 1);
	WideCharToMultiByte(CP_UTF8, 0, widePath, -1, p, u8len, NULL, NULL);
	utf8.ReleaseBuffer(u8len - 1);
	return utf8;
#else
	return AcpToUtf8(widePath);
#endif
}

CString Utf8ToCString(const char* utf8, int byteLen)
{
#ifdef _UNICODE
	const int srcLen = (byteLen < 0 && utf8) ? (int)strlen(utf8) : byteLen;
	if (utf8 == NULL || srcLen <= 0)
		return CString();
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, srcLen, NULL, 0);
	if (wlen <= 0)
		return CString(CStringA(utf8));
	CString out;
	LPWSTR wbuf = out.GetBuffer(wlen);
	MultiByteToWideChar(CP_UTF8, 0, utf8, srcLen, wbuf, wlen);
	out.ReleaseBuffer(wlen);
	return out;
#else
	return CString(Utf8ToAcp(utf8, byteLen));
#endif
}

static void ExpandUiNewlines(CString& s)
{
	s.Replace(_T("\\r\\n"), _T("\r\n"));
}

CString Utf8ToUiText(const char* utf8, int byteLen)
{
	CString s = Utf8ToCString(utf8, byteLen);
	ExpandUiNewlines(s);
	return s;
}

void msgUtf8(const char* utf8Fmt, ...)
{
	if (utf8Fmt == NULL)
		return;

	/* Format in UTF-8 first so DtZh::kBpOk etc. in %s stay valid; then one-shot UTF-8->GBK. */
	char utf8Body[2048];
	utf8Body[0] = 0;

	va_list ap;
	va_start(ap, utf8Fmt);
	_vsnprintf(utf8Body, sizeof(utf8Body) - 1, utf8Fmt, ap);
	va_end(ap);
	utf8Body[sizeof(utf8Body) - 1] = 0;

	CStringA acpBody = Utf8ToAcp(utf8Body);
	acpBody.Replace("\\r\\n", "\r\n");
	acpBody.Replace("\\n", "\n");
	if (!acpBody.IsEmpty())
		msg("%s", (LPCSTR)acpBody);
}
