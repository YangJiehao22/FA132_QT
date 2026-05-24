#pragma once
#include "afxwin.h"
#include "resource.h"

#include "ezCarDTCCM_SDK/ezCarDTCCM.h"
// I2CDebug 对话框

class I2CDebug : public CDialogEx
{
	DECLARE_DYNAMIC(I2CDebug)

public:
	I2CDebug(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~I2CDebug();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IICDEBUG_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	CComboBox m_I2cModeCtrl;

	unsigned char m_SlaveID;

	int m_iDevID;

	int m_I2cMode;
	int m_RegLenth;// 寄存器地址的字节长度
	int m_ValueLenth;// 寄存器数值的字节长度

	CString m_Address;
	CString m_Register;
	CString m_Value;
	afx_msg void OnBnClickedWriteButton();
	afx_msg void OnBnClickedReadButton();


	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeI2cModeCombo();
	afx_msg void OnBnClickedSearchButton();
};
