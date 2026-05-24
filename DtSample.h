
// DtSample.h
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include stdafx.h before this file to build the PCH"
#endif

#include "resource.h"

class CDtSampleApp : public CWinApp
{
public:
	CDtSampleApp();

public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CDtSampleApp theApp;
