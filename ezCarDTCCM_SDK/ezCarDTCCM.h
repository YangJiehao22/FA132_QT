#pragma once
#include "ezCarDtccmDef.h"
#define DTCAR_API extern "C" __declspec(dllexport)

/******************************************************************************
初始化流程
*******************************************************************************/
/** @defgroup group1 初始化相关


* @{

*/
/// @brief 枚举设备，获得设备名及设备个数
///
/// @param DeviceName:枚举的设备名
/// @param iDeviceNumMax:指定枚举设备的最大个数
/// @param pDeviceNum:枚举的设备个数
///
/// @retval DT_ERROR_OK:枚举操作成功
/// @retval DT_ERROR_FAILED:枚举操作失败
/// @retval DT_ERROR_INTERNAL_ERROR:内部错误
DTCAR_API int carEnumerateDevice(char *DeviceName[], int iDeviceNumMax, int *pDeviceNum);

/// @brief 加载初始化参数，如PIN定义、sensor参数、并口参数、MIPI参数、GPIO参数、OS参数
///
/// @param filename：初始化参数文件名
/// @param pGrabTab：返回初始化参数，内部指针参数不要去修改。
/// @param iDevID：设备ID
DTCAR_API int carLoadGrabPara(const char *filename, int iDevID);

/// @brief 加载初始化参数，如PIN定义、sensor参数、并口参数、MIPI参数、GPIO参数、OS参数
///
/// @param filename：初始化参数文件名
/// @param pGrabTab：返回初始化参数，内部指针参数不要去修改。
/// @param iDevID：设备ID
DTCAR_API int carGetGrabPara(GrabTab *pGrabTab, int iDevID);

/// @brief 设置设备类型
///
/// @param DeviceNum：需要指定的设备数量
/// @param boxType：设备类型 boxType值参考ezCarDtccmDef.h文件Box_Type定义
DTCAR_API int carSetDeviceType(int DeviceNum,int boxType);

/// @brief 打开设备
///
/// @param pszDeviceName：打开设备的名称
/// @param pDevID：返回打开设备的ID号，这个ID要传给其它接口用
/// @param iDevID：传入的设备ID号
///
/// @retval DT_ERROR_OK：打开设备成功
/// @retval DT_ERROR_FAILED：打开设备失败
/// @retval DT_ERROR_INTERNAL_ERROR：内部错误
/// @retval DT_ERROR_PARAMETER_INVALID：参数无效
DTCAR_API int carOpenDevice(const char *pszDeviceName, int *pDevID, int iDevID);

/// @brief 关闭设备
///
/// @param iDevID：设备ID
///
/// @retval DT_ERROR_OK：关闭设备成功
/// @retval DT_ERROR_FAILED：关闭设备失败
DTCAR_API int carCloseDevice(int iDevID);

/// @brief 公用功能初始化

/// @param commonSetting：功能使能结构体，参考ezCarDtccmDef.h文件CommonSetting_t结构体定义
/// @param iDevID：设备ID
///
/// @retval DT_ERROR_OK：成功
/// @retval DT_ERROR_FAILED：失败
DTCAR_API int carCommonInit(CommonSetting_t commonSetting, int iDevID);

/// @brief 公用功能反初始化

/// @param commonSetting：功能使能结构体，参考ezCarDtccmDef.h文件CommonSetting_t结构体定义
/// @param iDevID：设备ID
///
/// @retval DT_ERROR_OK：成功
/// @retval DT_ERROR_FAILED：失败
DTCAR_API int carCommonUinit(CommonSetting_t commonSetting, int iDevID);

/// @brief 初始化电源 
///
/// @param iDevID：设备ID
DTCAR_API int carInitPower(int iDevID);

/// @brief 关闭电源
///
/// @param iDevID：设备ID
DTCAR_API int carUnitPower(int iDevID);

/// @brief 初始化测试盒采集
///
/// @param iDevID：设备ID
DTCAR_API int carInitGrab(int iDevID);

/// @brief 关闭测试盒采集
///
/// @param iDevID：设备ID
DTCAR_API int carUnitGrab(int iDevID);
/** @} */ // end of group1

/******************************************************************************
常用接口
*******************************************************************************/
/** @defgroup group2 常用接口


* @{

*/
/// @brief 帧采集，不需要为DtImage_t申请内存，不要释放DtImage_t，测试盒用完会释放的
///
/// @param grabImg：帧数据相关结构体
/// @param iVcID：获取虚拟通道ID
/// @param iDevID：设备ID
DTCAR_API int carGrabFrameDirect(DtImage_t *grabImg, int *iVcID,int iDevID = DEFAULT_DEV_ID);

/// @brief 绘图
///
/// @param DrawImage_t：绘图相关参数结构体
/// @param iVcID：虚拟通道ID
/// @param iDevID：设备ID
DTCAR_API int carDrawImage(DrawImage_t tDrawImage, int iVcID, int iDevID = DEFAULT_DEV_ID);

/// @brief 获取数据，数据内容更新
///
/// @param VcData_t：对应到虚拟通道的各项数据结构体
/// @param iVcID：虚拟通道ID
/// @param iDevID：设备ID
DTCAR_API int carGetChannelData(VcData_t *tVcData, int iVcID, int iDevID = DEFAULT_DEV_ID);

/// @brief 获取数据指针
///
/// @param VcData_t：对应到虚拟通道的各项数据结构体指针
/// @param iVcID：虚拟通道ID
/// @param iDevID：设备ID
DTCAR_API int carGetVcData(VcData_t **tVcData, int iVcID, int iDevID = DEFAULT_DEV_ID);

/// @brief 获取电流，执行获取电流后VcData会更新电流数据
/// @param iCurrent：获取电流值
/// @param iCount：需要获取的电流VC通道数量(CC:4路VC , DF:2路VC, FA:4路VC)
/// @param iDevID：设备ID
DTCAR_API int carGetPmuCurrent(int *iCurrent, int iCount, int iDevID = DEFAULT_DEV_ID);

/// @brief FA132 96718 双芯片：切换当前 I2C 目标芯片（0=第一颗，1=第二颗）
DTCAR_API int carSetChipID(int ChipID, int iDevID = DEFAULT_DEV_ID);

/// @brief 读SESNOR寄存器,I2C通讯模式byI2cMode的设置值见I2CMODE定义
///
/// @param uAddr：从器件地址
/// @param uReg：寄存器地址
/// @param pValue：读到的寄存器的值
/// @param byMode：I2C模式，I2CMODE
///
/// @retval DT_ERROR_OK：读SENSOR寄存器操作成功
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_INVALID：byMode参数无效
/// @retval DT_ERROR_TIME_OUT：通讯超时
/// @retval DT_ERROR_INTERNAL_ERROR：内部错误
///
/// @see I2CMODE
DTCAR_API int carReadSensorReg(unsigned char uAddr, unsigned short uReg, unsigned short *pValue, unsigned char byMode, int iDevID = DEFAULT_DEV_ID);

/// @brief 写SENSOR寄存器,I2C通讯模式byI2cMode的设置值见I2CMODE定义
/// 
/// @param uAddr：从器件地址
/// @param uReg：寄存器地址
/// @param uValue：写入寄存器的值
/// @param byMode：I2C模式，I2CMODE
///
/// @retval DT_ERROR_OK：写SENSOR寄存器操作成功
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_INVALID：byMode参数无效
/// @retval DT_ERROR_TIME_OUT：通讯超时
/// @retval DT_ERROR_INTERNAL_ERROR：内部错误
///
/// @see I2CMODE
DTCAR_API int carWriteSensorReg(unsigned char uAddr, unsigned short uReg, unsigned short uValue, unsigned char byMode, int iDevID = DEFAULT_DEV_ID);

/// @brief 写SENSOR寄存器，支持向一个寄存器写入一个数据块
///
/// @param uDevAddr：从器件地址
/// @param uRegAddr：寄存器地址
/// @param uRegAddrSize：寄存器地址的字节数
/// @param pData：写入寄存器的数据块
/// @param uSize：写入寄存器的数据块的字节数
///
/// @retval DT_ERROR_OK：完成写SENSOR寄存器块操作成功
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_INVALID：uSize参数无效
/// @retval DT_ERROR_TIME_OUT：通讯超时
/// @retval DT_ERROR_INTERNAL_ERROR：内部错误
DTCAR_API int carWriteSensorI2c(unsigned char uDevAddr, unsigned short uRegAddr, unsigned char uRegAddrSize, unsigned char *pData, unsigned short uSize, int iDevID = DEFAULT_DEV_ID);

/// @brief 读SENSOR寄存器，支持向一个寄存器读出一个数据块
///
/// @param uDevAddr：从器件地址
/// @param uRegAddr：寄存器地址
/// @param uRegAddrSize：寄存器地址的字节数
/// @param pData：读到寄存器的值
/// @param uSize：读出寄存器的数据块的字节数
/// @param bNoStop：是否发出I2C的STOP命令，一般情况下默认为FALSE，bNoStop=TRUE表示写阶段不会有I2C的停止命令，直接进入读阶段，bNoStop=FALSE有I2C的停止命令，再进入读阶段
///
/// @retval DT_ERROR_OK：完成读SENSOR寄存器块操作成功
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_INVALID：uSize参数无效
/// @retval DT_ERROR_TIME_OUT：通讯超时
/// @retval DT_ERROR_INTERNAL_ERROR：内部错误
DTCAR_API int carReadSensorI2c(unsigned char uDevAddr, unsigned short uRegAddr, unsigned char uRegAddrSize, unsigned char *pData, unsigned short uSize, bool bNoStop = false, int iDevID = DEFAULT_DEV_ID);

/// @brief 设置电源电压
///
/// @param Power：电源类型，参见枚举类型“SENSOR_POWER”
/// @param Voltage：设置的电源电压值，单位mV
/// @param iCount：电源路数
///
/// @retval DT_ERROR_OK：设置电源电压成功
/// @retval DT_ERROR_FAILED：设置电源电压失败
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_OUT_OF_BOUND：参数超出了范围
DTCAR_API int carPmuSetVoltage(int Power[], int Voltage[], int iCount, int iDevID);

/// @brief 获取电源电压，如果能获取检测到的，尽量使用检测到的数据，否则返回电压设置值
///
/// @param Power：电源类型，参见枚举类型“SENSOR_POWER”
/// @param Voltage：获取的电源电压值，单位mV
/// @param iCount：电源路数
///
/// @retval DT_ERROR_OK：设置电源电压成功
/// @retval DT_ERROR_FAILED：设置电源电压失败
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_OUT_OF_BOUND：参数超出了范围
///
/// @see SENSOR_POWER
DTCAR_API int carPmuGetVoltage(int Power[], int Voltage[], int iCount, int iDevID);

/// @brief 获取电源电流。
///
/// @param Power：电源类型，参见枚举类型“SENSOR_POWER”
/// @param Current：电流值,单位nA
/// @param iCount：电源路数
///
/// @retval DT_ERROR_OK：获取电源电流成功
/// @retval DT_ERROR_FAILED：获取电源电流失败
/// @retval DT_ERROR_COMM_ERROR：通讯错误
/// @retval DT_ERROR_PARAMETER_OUT_OF_BOUND：参数超出了范围
///
/// @see SENSOR_POWER
DTCAR_API int carPmuGetCurrent(int Power[], int Current[], int iCount, int iDevID);

/** @} */ // end of group2


/******************************************************************************
其他功能
*******************************************************************************/
/** @defgroup group3 其他功能


* @{

*/
/// @brief 度信内部信息对话框，便于工程师调试用
///
/// @param hwnd：窗口句柄，可以给NULL
///
/// @retval DT_ERROR_OK：操作成功
DTCAR_API int carShowInternalStatusDialog(HWND hwnd, int iDevID);
DTCAR_API int carShowInternalDebugDialog(HWND hwnd, int iDevID);

/// @bfief 采集暂停，设备帧ID继续增加，如果继续抓帧，会返回-1000错误码
DTCAR_API int carGrabHold(int iDevID);

/// @brief 采集重启
///
/// @param uDelayTime：设置数据流延时输出，单位us
/// @param uFrameNum:设置重启采集后采集帧数，达到设置的采集帧数，将自动GrabHold；如果设为0，将连续输出帧
/// 
/// @retval DT_ERROR_OK：设置成功
/// @retval DT_ERROR_FAILED：设置失败
/// @retval DT_ERROR_COMM_ERROR：通讯错误
DTCAR_API int carGrabRestart(double lfDelayTime, unsigned int uFrameNum, int iDevID);

/// @brief 读取传入指针的地址并打印在LOG中
/// 
/// @param p：指针
/// @param iDevID：设备ID(可以不设置)
DTCAR_API int carAddressTest(void * p, int iDevID = DEFAULT_DEV_ID);

DTCAR_API int carDllTest(const char * filename, int iDevID);
/// @brief 设置MASK值
/// 
/// @param MASK值：bit0-bit3，分别为VC0-VC3
/// @param iDevID：设备ID
DTCAR_API int carSetMask(int iMask = 0, int iDevID = DEFAULT_DEV_ID);

/// @brief 设置TakePixel值
/// 
/// @param iVcID：VC通道ID
/// @param bEnd：是否使能
/// @param iDevID：设备ID
DTCAR_API int carTakePixel(int iVcID, bool bEnd,int iDevID = DEFAULT_DEV_ID);

/// @brief  扩展控制接口
/// @param dwCtrl:	控制码，可能由派生类定义
/// @param dwParam:	控制码相关参数
/// @param pIn:		读取的数据
/// @param pInSize:	读取数据的大小，字节数，调用者指定要求读取的字节数，如果成功，返回实际读取的字节数
/// @param pOut:	写入的数据
/// @param pOutSize:写入数据的大小，字节数，调用者指定要求写入的字节数，如果成功，返回实际写入的字节数
DTCAR_API int carExCtrl(unsigned long dwCtrl, unsigned long  dwParam, void* pIn, int *pInSize, void* pOut, int *pOutSize, int iDevID);


/// @brief 保存图片  
///
/// @param fileName：文件名
/// @param srcImg：源图像数据
/// @param imgType：保存图片类型, 参考枚举类型SAVE_IMG_TYPE
///
/// @retval DT_ERROR_OK：获取电流成功
/// @retval DT_ERROR_FORMAT_INVALID：格式错误，格式不匹配
/// @retval DT_ERROR_NO_MEMORY：内存错误
/// @retval DT_ERROR_FAILED：其它错误
///
/// @note 
///
/// @code
/// //example示例，保存图片
/// iRet = ezGrabFrame(&pGrabBuf, &sFrameInfo, m_nDevID);
/// if (iRet == DT_ERROR_OK)
/// {
/// 	CTime time = CTime::GetCurrentTime();
/// 	CString sfilename;
/// 	sfilename.Format(_T("%02d%02d%02d%02d%02d"), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond());
/// 	CString sTemp, szOutformat;
/// 
/// 	DtImage_t srcImg;
/// 	srcImg.format = static_cast<IMAGE_FORMAT>(sFrameInfo.byImgFormat);
/// 	srcImg.dataSize = sFrameInfo.uDataSize;
/// 	srcImg.data = pGrabBuf;
/// 	srcImg.width = sFrameInfo.uWidth;
/// 	srcImg.height = sFrameInfo.uHeight;
/// 	srcImg.rawFmt = static_cast<RAW_FORMAT>(m_GrabPara.sensor.outformat);
/// 
/// 	switch (m_GrabPara.sensor.outformat)
/// 	{
/// 	case OUTFORMAT_RGGB:
/// 	szOutformat = _T("RGGB");
/// 	break;
/// 
/// 	case OUTFORMAT_GRBG:
/// 	szOutformat = _T("GRBG");
/// 	break;
/// 
/// 	case OUTFORMAT_GBRG:
/// 	szOutformat = _T("GBRG");
/// 	break;
/// 
/// 	case OUTFORMAT_BGGR:
/// 	szOutformat = _T("BGGR");
/// 	break;
/// 	}
/// 
/// 	sTemp.Format(_T("_%d_%d_%s"), sFrameInfo.uWidth, sFrameInfo.uHeight, szOutformat);
/// 	sfilename = sfilename + sTemp;
/// 
/// 	LPCTSTR lpfilter = _T("Raw8(*.raw)|*.raw|Raw_2Bytes(*.raw)|*.raw|BMP(24)(*.bmp)|*.bmp|JPEG(*.jpg)|*.jpg|PNG(*.png)|*.png||");
/// 	CFileDialog dlg(FALSE, NULL, sfilename, OFN_FILEMUSTEXIST, lpfilter);
/// 
/// 	if (IDOK != dlg.DoModal())
/// 	{
/// 		return;
/// 	}
/// 
/// 	CString PathName = dlg.GetPathName();
/// 	int index = dlg.m_ofn.nFilterIndex;
/// 
/// 	switch (index)
/// 	{
/// 	case 1: //保存Raw8格式图片
/// 	{
/// 		PathName = PathName + _T(".") + _T("raw");
/// 		iRet = ezSaveImage(PathName, srcImg, SAVE_IMG_TYPE::RAW8, m_nDevID);
/// 		break;
/// 	}
/// 
/// 	case 2://保存Raw10 P10格式图片
/// 	{
/// 		PathName = PathName + _T(".") + _T("raw");
/// 		iRet = ezSaveImage(PathName, srcImg, SAVE_IMG_TYPE::RAW, m_nDevID);
/// 		break;
/// 	}
/// 
/// 	case 3://保存BMP格式图片
/// 	{
/// 		PathName = PathName + _T(".") + _T("bmp");
/// 		iRet = ezSaveImage(PathName, srcImg, SAVE_IMG_TYPE::BMP, m_nDevID);
/// 		break;
/// 	}
/// 	case 4://保存jpg格式图片
/// 	{
/// 		PathName = PathName + _T(".") + _T("jpg");
/// 		iRet = ezSaveImage(PathName, srcImg, SAVE_IMG_TYPE::BMP, m_nDevID);
/// 		break;
/// 	}
/// 	case 5://保存png格式图片
/// 	{
/// 		PathName = PathName + _T(".") + _T("png");
/// 		iRet = ezSaveImage(PathName, srcImg, SAVE_IMG_TYPE::BMP, m_nDevID);
/// 		break;
/// 	}
/// }
/// @endcode
DTCAR_API int carSaveImage(const char *fileName, DtImage_t srcImg, int imgType, int iDevID);

///	@brief 	原始图像数据格式转换
/// @param	srcImg:		原始图像数据结构体描述，DtImage_t结构体说明参见"ImageKit.h"控制码相关参数
/// @param	destImg:	目标图像数据结构体描述，DtImage_t结构体说明参见"ImageKit.h"读取的数据
/// @param	iDevID：设备ID
DTCAR_API int carImageTransform(DtImage_t *srcImg, DtImage_t *destImg, int iDevID);

///	@brief 	开启视频录制
/// @param	RecordVideo_t:		录制参数结构体
/// @param	iDevID：设备ID
/// @param	iVcID：虚拟通道ID
DTCAR_API int carStartVideo(RecordVideo_t RecordVideo,int iDevID, int iVcID);

///	@brief 	关闭视频录制
/// @param	iDevID：设备ID
/// @param	iVcID：虚拟通道ID
DTCAR_API int carStopVideo(int iDevID, int iVcID);

/** @} */ // end of group3


/******************************************************************************
特殊功能
*******************************************************************************/
/** @defgroup group4 特殊功能


* @{

*/
/// @brief 使能图像分割
///
/// @param bEnable：开启/关闭
/// @param iDevID：设备ID
DTCAR_API int carSetImgBlock(bool bEnable, int iDevID);

/// @brief 绘图图像分割块
///
/// @param DrawImage_t：绘图相关参数结构体
/// @param iVcID：虚拟通道ID
/// @param iDevID：设备ID
/// @param iBlock：图像块ID
DTCAR_API int carDrawImageBlock(DrawImage_t tDrawImage, int iVcID, int iDevID, int iBlock);

/// @brief Block帧采集，不需要为DtImage_t申请内存，不要释放DtImage_t，测试盒用完会释放的,
///
/// @param grabImg：帧数据相关结构体
/// @param iVcID：获取虚拟通道ID
/// @param iDevID：设备ID
DTCAR_API int carGrabFrameDirectBlock(DtImage_t *grabImg, int *iVcID, int iDevID);

DTCAR_API int carSetSaveHeadImage(bool bEnable, const char *fileName, int imgType, int iDevID);
DTCAR_API int carSetSaveEndImage(bool bEnable, const char *fileName, int imgType, int iDevID);
DTCAR_API int carSetSaveErrImage(bool bEnable, const char *fileName, int imgType, int iDevID);

/** @} */ // end of group4