#pragma once

#include "DtZhUtf8.h"

/** UTF-8 (ASCII .cpp) -> system ANSI (GBK on Chinese Windows) for MultiByte MFC / msg(). */
CStringA Utf8ToAcp(const char* utf8, int byteLen = -1);
/** System ANSI (GBK) -> UTF-8 for daily log files (editors expect UTF-8). */
CStringA AcpToUtf8(const char* acp, int byteLen = -1);
CString Utf8ToCString(const char* utf8, int byteLen = -1);
/** UTF-8 -> UI string; expands literal \\r\\n in DtZh literals to line breaks. */
CString Utf8ToUiText(const char* utf8, int byteLen = -1);

/** Log helper: utf8Fmt is UTF-8; converted to GBK before Edit control / daily log. */
void msgUtf8(const char* utf8Fmt, ...);

#define ZH_UTF8(x) Utf8ToCString(DtZh::x)
#define ZH_UI(x) Utf8ToUiText(DtZh::x)
