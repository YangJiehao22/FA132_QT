// I2CDebug.cpp : 实现文件
//

#include "stdafx.h"
#include "I2CDebug.h"
#include "afxdialogex.h"


// I2CDebug 对话框

IMPLEMENT_DYNAMIC(I2CDebug, CDialogEx)

I2CDebug::I2CDebug(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_IICDEBUG_DIALOG, pParent)
	, m_Address(_T(""))
	, m_Register(_T(""))
	, m_Value(_T(""))
	, m_I2cMode(0)
	, m_RegLenth(4)
	, m_ValueLenth(4)
	, m_SlaveID(0)
	, m_iDevID(0)
{

}

I2CDebug::~I2CDebug()
{
}

void I2CDebug::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_I2C_MODE_COMBO, m_I2cModeCtrl);
	DDX_Text(pDX, IDC_ADDRESS_EDIT, m_Address);
	DDV_MaxChars(pDX, m_Address, 2);
	DDX_Text(pDX, IDC_REGISTER_EDIT, m_Register);
	DDV_MaxChars(pDX, m_Register, m_RegLenth);
	DDX_Text(pDX, IDC_VALUE_EDIT, m_Value);
	DDV_MaxChars(pDX, m_Value, m_ValueLenth);
}


BEGIN_MESSAGE_MAP(I2CDebug, CDialogEx)
	ON_BN_CLICKED(IDC_WRITE_BUTTON, &I2CDebug::OnBnClickedWriteButton)
	ON_BN_CLICKED(IDC_READ_BUTTON, &I2CDebug::OnBnClickedReadButton)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_I2C_MODE_COMBO, &I2CDebug::OnCbnSelchangeI2cModeCombo)
	ON_BN_CLICKED(IDC_SEARCH_BUTTON, &I2CDebug::OnBnClickedSearchButton)
END_MESSAGE_MAP()


BOOL I2CDebug::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_I2cModeCtrl.AddString(_T("0: Reg[8]  Val[8]"));
	m_I2cModeCtrl.AddString(_T("1: Reg[8]  Val[16]"));
	m_I2cModeCtrl.AddString(_T("2: Reg[16] Val[8]"));
	m_I2cModeCtrl.AddString(_T("3: Reg[16] Val[16]"));

	//初始化IIC mode
	switch (m_I2cMode)
	{
	case I2CMODE_NORMAL:
		m_RegLenth = 2;
		m_ValueLenth = 2;
		m_I2cModeCtrl.SetCurSel(0);
		break;
	case I2CMODE_MICRON:
		m_RegLenth = 2;
		m_ValueLenth = 4;
		m_I2cModeCtrl.SetCurSel(1);
		break;
	case I2CMODE_STMICRO:
		m_RegLenth = 4;
		m_ValueLenth = 2;
		m_I2cModeCtrl.SetCurSel(2);
		break;
	case I2CMODE_MICRON2:
		m_RegLenth = 4;
		m_ValueLenth = 4;
		m_I2cModeCtrl.SetCurSel(3);
		break;
	default:
		break;
	}

	//初始化IIC SlaveID
	m_Address.Format(_T("%X"), m_SlaveID);

	UpdateData(false); 
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void I2CDebug::OnBnClickedSearchButton()
{
	// TODO: 在此添加控件通知处理程序代码
	int i;
	BYTE bySlaveId[8] = { 0 };
	int count = 0;
	USHORT tmp;
	CString sSlaveInfo, sTemp;
	sSlaveInfo.Empty();
	for (i = 0x02; i < 0xff; i += 2)
	{
		if (count >= 8)
		{
			break;
		}
		if (::carReadSensorReg(i, 0, &tmp, 0, m_iDevID))
		{
			bySlaveId[count] = i;
			count++;
		}
	}

	if (!count)
	{
		AfxMessageBox(_T("Don't find I2C slave device in current reset/pwdn pin state!"));
	}
	else
	{
		for (i = 0; i < count; i++)
		{
			sTemp.Format(_T("0x%02x; "), bySlaveId[i]);
			sSlaveInfo += sTemp;
		}
		AfxMessageBox(sSlaveInfo);
	}
}


void I2CDebug::OnCbnSelchangeI2cModeCombo()
{
	// TODO: 在此添加控件通知处理程序代码
	switch (m_I2cModeCtrl.GetCurSel())
	{
	case 0:
		m_I2cMode = I2CMODE_NORMAL;
		m_RegLenth = 2;
		m_ValueLenth = 2;
		break;
	case 1:
		m_I2cMode = I2CMODE_MICRON;
		m_RegLenth = 2;
		m_ValueLenth = 4;
		break;
	case 2:
		m_I2cMode = I2CMODE_STMICRO;
		m_RegLenth = 4;
		m_ValueLenth = 2;
		break;
	case 3:
		m_I2cMode = I2CMODE_MICRON2;
		m_RegLenth = 4;
		m_ValueLenth = 4;
		break;
	default:
		break;
	}

	UpdateData(false);
}

// I2CDebug 消息处理程序
void I2CDebug::OnBnClickedWriteButton()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	USHORT uAddr;
	USHORT uValue;
	USHORT uReg;

	//去除字符串首尾空格
	m_Address.Trim();
	m_Register.Trim();
	m_Value.Trim();
	//将字符串转换为整形
	sscanf_s(m_Address, _T("%hX"), &uAddr);
	sscanf_s(m_Register, _T("%hX"), &uReg);
	sscanf_s(m_Value, _T("%hX"), &uValue);

	int iRet = 0;
	iRet = ::carWriteSensorReg(uAddr, uReg, uValue, m_I2cMode,m_iDevID);
	if (iRet != DT_ERROR_OK)
	{
		CString sTemp;
		sTemp.Format(_T("Write I2C fail,error code:%d"), iRet);
		AfxMessageBox(sTemp);
	}
}


void I2CDebug::OnBnClickedReadButton()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	USHORT uAddr;
	USHORT uReg;
	USHORT uValue;
	//去除字符串首尾空格
	m_Address.Trim();
	m_Register.Trim();
	//将字符串转换为整形
	sscanf_s(m_Address, _T("%hX"), &uAddr);
	sscanf_s(m_Register, _T("%hX"), &uReg);
	int iRet = DT_ERROR_OK;
	iRet = ::carReadSensorReg(uAddr, uReg, &uValue, m_I2cMode, m_iDevID);
	if (iRet != DT_ERROR_OK)
	{
		CString sTmp;
		sTmp.Format(_T("Read I2C fail,error code:%d"), iRet);
		AfxMessageBox(sTmp);
	}
	m_Value.Format(_T("%X"), uValue);
	UpdateData(false);
}

void I2CDebug::OnDestroy()
{
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
}