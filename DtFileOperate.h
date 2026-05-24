#pragma once

int GetExePath(CString &strPath);
int GetIniFileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName);
CString GetIniFileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR nDefault, LPCTSTR lpFileName);
int WriteIniFileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nInt, LPCTSTR lpFileName);
int WriteIniFileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName);
/** Flush Win32 profile cache so the next read sees what was just written. */
void FlushIniFile(LPCTSTR lpFileName);

