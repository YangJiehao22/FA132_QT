#include "StdAfx.h"
#include "DtFileOperate.h"

int GetExePath(CString &strPath) {
	/* Build exe-path INI file name (same base name as .exe) */
	char szTmp[MAX_PATH + 1];
	HANDLE hModule;
	hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		if (GetModuleFileName((HMODULE)hModule, szTmp, sizeof(szTmp) - 4) > 0)
		{
			char *p = strrchr(szTmp, '.');
			if (p != NULL)
			{
				p[1] = 'i';
				p[2] = 'n';
				p[3] = 'i';
				p[4] = 0;
			}
		}
	}

	strPath = szTmp;
	return 1;
}

int GetIniFileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName) {
	return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, lpFileName);
}

CString GetIniFileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR nDefault, LPCTSTR lpFileName) {
	TCHAR szTmp[4096];
	GetPrivateProfileString(lpAppName, lpKeyName, nDefault, szTmp, (int)(sizeof(szTmp) / sizeof(szTmp[0])), lpFileName);
	return szTmp;
}

void FlushIniFile(LPCTSTR lpFileName)
{
	if (lpFileName != NULL && lpFileName[0] != 0)
		WritePrivateProfileString(NULL, NULL, NULL, lpFileName);
}

int WriteIniFileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nInt, LPCTSTR lpFileName) {
	CString str;
	str.Format(_T("%d"), nInt);
	return WritePrivateProfileString(lpAppName, lpKeyName, str, lpFileName) ? 1 : 0;
}

int WriteIniFileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName) {
	int iRet = WritePrivateProfileString(lpAppName, lpKeyName, lpString, lpFileName);
	return iRet;
}
