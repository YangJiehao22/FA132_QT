#ifndef __IMAGEKIT_H__
#define __IMAGEKIT_H__

/**************************************************************************************** 
*
* imagekit系列各种硬件版本定义
*
****************************************************************************************/

#define VER_HS100   0x1000
#define VER_HS200   0x1020

#define VER_HS120   0x1020 
#define VER_HS128   0x1200
#define VER_HS230   0x1230

#define VER_HS280   0x1280

#define VER_HS130   0x1300
#define VER_HS300   0x1330
#define VER_HS320   0x1332

#define VER_HV810   0x1380
#define VER_HV820   0x1382
#define VER_HV910   0x1390
#define VER_HV920   0x1392

/**************************************************************************************** 
*
* SENSOR的初始化与控制相关
*
****************************************************************************************/

typedef struct _SensorTab
{
    /// @brief SENSOR宽度
    USHORT width;                       ///<SENSOR宽度
    /// @brief SENSOR高度
    USHORT height;                      ///<SENSOR高度
    /// @brief SENSOR数据类型
    BYTE type;                          ///<SENSOR数据类型
    /// @brief SENSOR的RESET和PWDN引脚设置
    BYTE pin;                           ///<SENSOR的RESET和PWDN引脚设置
    /// @brief SENSOR的器件地址
    BYTE SlaveID;                       ///<SENSOR的器件地址
    /// @brief SENSOR的I2C模式
    BYTE mode;                          ///<SENSOR的I2C模式
    /// @brief SENSOR标志寄存器1.
    USHORT FlagReg;                     ///<SENSOR标志寄存器1.
    /// @brief SENSOR标志寄存器1的值
    USHORT FlagData;                    ///<SENSOR标志寄存器1的值
    /// @brief SENSOR标志寄存器1的掩码值
    USHORT FlagMask;                    ///<SENSOR标志寄存器1的掩码值
    /// @brief SENSOR标志寄存器2.
    USHORT FlagReg1;                    ///<SENSOR标志寄存器2.
    /// @brief SENSOR标志寄存器2的值
    USHORT FlagData1;                   ///<SENSOR标志寄存器2的值
    /// @brief SENSOR标志寄存器2的掩码值
    USHORT FlagMask1;                   ///<SENSOR标志寄存器2的掩码值
    /// @brief SENSOR的名称
    char name[64];                      ///<SENSOR的名称

    /// @brief 初始化SENSOR数据表
    USHORT* ParaList;                   ///<初始化SENSOR数据表
    /// @brief 初始化SENSOR数据表大小，单位字节
    USHORT  ParaListSize;               ///<初始化SENSOR数据表大小，单位字节

    /// @brief SENSOR进入Sleep模式的参数表
    USHORT* SleepParaList;              ///<SENSOR进入Sleep模式的参数表
    /// @brief SENSOR进入Sleep模式的参数表大小，单位字节
    USHORT  SleepParaListSize;          ///<SENSOR进入Sleep模式的参数表大小，单位字节

    /// @brief SENSOR输出数据格式，YUV//0:YCbYCr;	//1:YCrYCb;	//2:CbYCrY;	//3:CrYCbY.
    BYTE outformat;                     ///<SENSOR输出数据格式，YUV//0:YCbYCr;	//1:YCrYCb;	//2:CbYCrY;	//3:CrYCbY.
    /// @brief SENSOR的输入时钟MCLK，0:12M; 1:24M; 2:48M.
    int mclk;                           ///<SENSOR的输入时钟MCLK，0:12M; 1:24M; 2:48M.
    /// @brief SENSOR的AVDD电压值
    BYTE avdd;                          ///<SENSOR的AVDD电压值
    /// @brief SENSOR的DOVDD电压值
    BYTE dovdd;                         ///<SENSOR的DOVDD电压值
    /// @brief SENSOR的DVDD电压值		
    BYTE dvdd;                          ///<SENSOR的DVDD电压值

    /// @brief SENSOR的数据接口类型
    BYTE port;                          ///<SENSOR的数据接口类型
    USHORT Ext0;
    USHORT Ext1;
    USHORT Ext2; 

    /// @brief AF初始化参数表
    USHORT* AF_InitParaList;            ///<AF初始化参数表
    /// @brief AF初始化参数表大小，单位字节
    USHORT  AF_InitParaListSize;        ///<AF初始化参数表大小，单位字节

    /// @brief AF_AUTO参数表
    USHORT* AF_AutoParaList;            ///<AF_AUTO参数表
    /// @brief AF_AUTO参数表大小，单位字节
    USHORT  AF_AutoParaListSize;        ///<AF_AUTO参数表大小，单位字节

    /// @brief AF_FAR参数表
    USHORT* AF_FarParaList;             ///<AF_FAR参数表
    /// @brief AF_FAR参数表大小，单位字节
    USHORT  AF_FarParaListSize;         ///<AF_FAR参数表大小，单位字节

    /// @brief AF_NEAR参数表
    USHORT* AF_NearParaList;            ///<AF_NEAR参数表
    /// @brief AF_NEAR参数表大小，单位字节
    USHORT  AF_NearParaListSize;        ///<AF_NEAR参数表大小，单位字节

    /// @brief 曝光参数表
    USHORT* Exposure_ParaList;          ///<曝光参数表
    /// @brief 曝光参数表大小，单位字节
    USHORT  Exposure_ParaListSize;      ///<曝光参数表大小，单位字节

    /// @brief 增益参数表
    USHORT* Gain_ParaList;              ///<增益参数表
    /// @brief 增益参数表大小，单位字节
    USHORT	Gain_ParaListSize;          ///<增益参数表大小，单位字节 

	_SensorTab()
	{
		width=0;
		height=0;
		type=0;
		pin=0;
		SlaveID=0;
		mode=0;
		FlagReg=0;
		FlagData=0;
		FlagMask=0;
		FlagReg1=0;
		FlagData1=0;
		FlagMask1=0;
		memset(name,0,sizeof(name));

		ParaList=NULL;
		ParaListSize=0;
		SleepParaList=NULL;
		SleepParaListSize=0;

		outformat=0;
		mclk=0;               //0:12M; 1:24M; 2:48M.
		avdd=0;               // 
		dovdd=0;              //
		dvdd=0;

		port=0; 	
		Ext0=0;
		Ext1=0;
		Ext2=0; 

		AF_InitParaList=NULL;        //AF_InitParaList
		AF_InitParaListSize=0;

		AF_AutoParaList=NULL;
		AF_AutoParaListSize=0;

		AF_FarParaList=NULL;
		AF_FarParaListSize=0;

		AF_NearParaList=NULL;
		AF_NearParaListSize=0;

		Exposure_ParaList=NULL;      //曝光
		Exposure_ParaListSize=0;

		Gain_ParaList=NULL;          //增益
		Gain_ParaListSize=0;
	}
}SensorTab, *pSensorTab;

//紧随SensorTab, 增加SensorTab2定义
///////////////////////////////////////////////////////////
typedef struct _SensorTab2
{
	/// @brief SENSOR宽度
	UINT width;          ///<SENSOR宽度
	/// @brief SENSOR高度
	UINT height;         ///<SENSOR高度

	UINT Quick_w;		///<Quick View 宽度	
	UINT Quick_h;		///<Quick View 高度

	/// @brief SENSOR数据类型
	UINT type;             ///<SENSOR数据类型
	/// @brief SENSOR的RESET和PWDN引脚设置
	UINT pin;              ///<SENSOR的RESET和PWDN引脚设置
	/// @brief SENSOR的器件地址
	UINT SlaveID;          ///<SENSOR的器件地址
	/// @brief SENSOR的I2C模式
	UINT mode;						 ///<SENSOR的I2C模式
	/// @brief SENSOR标志寄存器1.
	UINT FlagReg;				 ///<SENSOR标志寄存器1.
	/// @brief SENSOR标志寄存器1的值
	UINT FlagData;			 ///<SENSOR标志寄存器1的值
	/// @brief SENSOR标志寄存器1的掩码值
	UINT FlagMask;			 ///<SENSOR标志寄存器1的掩码值
	/// @brief SENSOR标志寄存器2.
	UINT FlagReg1;			 ///<SENSOR标志寄存器2.
	/// @brief SENSOR标志寄存器2的值
	UINT FlagData1;			 ///<SENSOR标志寄存器2的值
	/// @brief SENSOR标志寄存器2的掩码值
	UINT FlagMask1;			 ///<SENSOR标志寄存器2的掩码值
	/// @brief SENSOR的名称
	char name[64];				///<SENSOR的名称

	/// @brief 初始化SENSOR数据表
	UINT* ParaList;			///<初始化SENSOR数据表
	/// @brief 初始化SENSOR数据表大小，单位字节
	UINT  ParaListSize; ///<初始化SENSOR数据表大小，单位字节

	/// @brief SENSOR进入Sleep模式的参数表
	UINT* SleepParaList;	///<SENSOR进入Sleep模式的参数表
	/// @brief SENSOR进入Sleep模式的参数表大小，单位字节
	UINT  SleepParaListSize;///<SENSOR进入Sleep模式的参数表大小，单位字节

	/// @brief SENSOR进入Quick View模式的参数表
	UINT* QuickParaList;	///<SENSOR进入Quick View模式的参数表
	/// @brief SENSOR进入Quick View模式的参数表大小，单位字节
	UINT  QuickParaListSize;///<SENSOR进入Quick View模式的参数表大小，单位字节

	/// @brief SENSOR输出数据格式，YUV//0:YCbYCr;	//1:YCrYCb;	//2:CbYCrY;	//3:CrYCbY.
	UINT outformat;         ///<SENSOR输出数据格式，YUV//0:YCbYCr;	//1:YCrYCb;	//2:CbYCrY;	//3:CrYCbY.
	/// @brief SENSOR的输入时钟MCLK，0:12M; 1:24M; 2:48M.
	UINT mclk;               ///<SENSOR的输入时钟MCLK，0:12M; 1:24M; 2:48M.
	/// @brief SENSOR的AVDD电压值
	UINT avdd;              ///<SENSOR的AVDD电压值
	/// @brief SENSOR的DOVDD电压值
	UINT dovdd;             ///<SENSOR的DOVDD电压值
	/// @brief SENSOR的DVDD电压值		
	UINT dvdd;							///<SENSOR的DVDD电压值

	/// @brief SENSOR的数据接口类型
	UINT port; 							///<SENSOR的数据接口类型
	UINT Ext0;
	UINT Ext1;
	UINT Ext2; 

	/// @brief AF初始化参数表
	UINT* AF_InitParaList;        ///<AF初始化参数表
	/// @brief AF初始化参数表大小，单位字节
	UINT  AF_InitParaListSize;		///<AF初始化参数表大小，单位字节

	/// @brief AF_AUTO参数表
	UINT* AF_AutoParaList;				///<AF_AUTO参数表
	/// @brief AF_AUTO参数表大小，单位字节
	UINT  AF_AutoParaListSize;		///<AF_AUTO参数表大小，单位字节

	/// @brief AF_FAR参数表
	UINT* AF_FarParaList;					///<AF_FAR参数表
	/// @brief AF_FAR参数表大小，单位字节
	UINT  AF_FarParaListSize;			///<AF_FAR参数表大小，单位字节

	/// @brief AF_NEAR参数表
	UINT* AF_NearParaList;				///<AF_NEAR参数表
	/// @brief AF_NEAR参数表大小，单位字节
	UINT  AF_NearParaListSize;		///<AF_NEAR参数表大小，单位字节

	/// @brief 曝光参数表
	UINT* Exposure_ParaList;      ///<曝光参数表
	/// @brief 曝光参数表大小，单位字节
	UINT  Exposure_ParaListSize;	///<曝光参数表大小，单位字节

	/// @brief 增益参数表
	UINT* Gain_ParaList;          ///<增益参数表
	/// @brief 增益参数表大小，单位字节
	UINT	Gain_ParaListSize;			///<增益参数表大小，单位字节 

	_SensorTab2()
	{
		width=0;
		height=0;
		Quick_w = 0; //20141031
		Quick_h = 0; //20141031
		type=0;
		pin=0;
		SlaveID=0;
		mode=0;
		FlagReg=0;
		FlagData=0;
		FlagMask=0;
		FlagReg1=0;
		FlagData1=0;
		FlagMask1=0;
		memset(name,0,sizeof(name));

		ParaList=NULL;
		ParaListSize=0;
		SleepParaList=NULL;
		SleepParaListSize=0;

		QuickParaList = NULL; //20141031
		QuickParaListSize = 0; //20141031

		outformat = 0;
		mclk= 0;               //0:12M; 1:24M; 2:48M.
		avdd= 0;               // 
		dovdd = 0;              //
		dvdd = 0;

		port=0; 	
		Ext0=0;
		Ext1=0;
		Ext2=0; 

		AF_InitParaList=NULL;        //AF_InitParaList
		AF_InitParaListSize=0;

		AF_AutoParaList=NULL;
		AF_AutoParaListSize=0;

		AF_FarParaList=NULL;
		AF_FarParaListSize=0;

		AF_NearParaList=NULL;
		AF_NearParaListSize=0;

		Exposure_ParaList=NULL;      //曝光
		Exposure_ParaListSize=0;

		Gain_ParaList=NULL;          //增益
		Gain_ParaListSize=0;
	}
}SensorTab2, *pSensorTab2;

/** @defgroup group1 设备信息相关
@{

*/

typedef struct DtDevEnumInfo_s
{
    // 设备名称
    char    DeviceName[256];         //<设备名称，包含了机型名 + 用户定义名 + 序列号 + 设备ID(拨码开关)
    // 版本号
    char    Version[1024];
    // 备用
    int     Rsv[64];
    // kittype
    // 功能属性
}DtDevEnumInfo_t;


/** @} */ // end of group2

/** @defgroup group2 ISP相关
@{

*/

/** @name SENSOR输出图像类型定义(SensorTab::type的取值定义)
@{

*/
/* SENSOR输出图像类型定义(SensorTab::type的取值定义)， 建议使用对应的IMAGE_FORMAT枚举类型 */
#define D_RAW10				0x00      
#define D_RAW8				0x01
#define D_MIPI_RAW8			0x01
#define D_YUV				0x02 
#define D_RAW16				0x03
#define D_MIPI_RAW16		0x03
#define D_RGB565			0x04
#define D_YUV_SPI			0x05
#define D_MIPI_RAW10		0x06    ///< 5 bytes = 4 pixel...
#define D_MIPI_RAW12		0x07    ///< 3 bytes = 2 pixel...
#define D_RAW12				0x07
#define D_YUV_MTK_S			0x08    //MTK output...
#define D_YUV_10			0x09
#define D_YUV_12			0x0a

#define D_MIPI_RAW14		0x0b    ///< 7 bytes = 4 pixel...
#define D_MIPI_RAW20		0x0c	///< 5 bytes = 2 pixel...
#define D_MIPI_RAW24		0x0d	///< 3 bytes = 1 pixel...

#define D_BGR24             0x20    ///< 排列顺序为B，G，R，各8bit
#define D_BGR32             0x21    ///< 排列顺序为B，G，R，0各8bit

#define D_RGB888            D_BGR24

#define D_P10               0x24    ///< 一个像素占两个字节，LSB，0～1023，一般用于MIPI_RAW10转换
#define D_P12               0x25    ///< 一个像素占两个字节，LSB，0～4095，一般用于MIPI_RAW12转换
#define D_P14               0x26    ///< 一个像素占两个字节，LSB，0～16383，一般用于MIPI_RAW14转换

#define D_G8                0x28
#define D_G10               0x29
#define D_GRAY8				0x2a

#define D_RGB888_DVP12BIT   0x30    ///<rgb888 并口12bit

#define D_RAW24				0x40	///<3字节，一般是将MIPI raw24原始字节流，转成23:16,15:8,7:0
//#define D_RGB24             0x0b
//#define D_HISPI_SP			0x09    //aptina hispi packet sp.
/** @} */


/** @name RAW转RGB算法定义
@{

*/
/* RAW转RGB算法定义 */
#define RAW2RGB_NORMAL			0
#define RAW2RGB_SMOOTH			1
#define RAW2RGB_SHARP			2
#define RAW2RGB_SHARP_ENHANCE   3   //增强sharp算法
#define RAW2RGB_SHARP_ENHANCE2  4	//增强sharp算法，对应dtPixel的清晰插值
/** @} */

/** @name YUV图像4种输出格式定义
@{

*/
/* RAW、YUV图像4种输出格式定义(SensorTab::outformat的取值定义) */
/// YUV图像4种输出格式定义。
enum OUTFORMAT_YUV
{
	OUTFORMAT_YCbYCr = 0,///<YCbYCr输出格式
	OUTFORMAT_YCrYCb,///<YCrYCb输出格式
	OUTFORMAT_CbYCrY,///<CbYCrY输出格式
	OUTFORMAT_CrYCbY,///<CrYCbY输出格式
};
/** @} */

/** @name RAW图像4种输出格式定义
@{

*/
/// RAW图像4种输出格式定义。
enum OUTFORMAT_RGB
{
	OUTFORMAT_RGGB = 0,///<RGGB输出格式
	OUTFORMAT_GRBG,///<GRBG输出格式
	OUTFORMAT_GBRG,///<GBRG输出格式
	OUTFORMAT_BGGR,///<BGGR输出格式
};
/** @} */

/* 本系统支持的RAW格式、YUV格式定义 */
/** @name 支持的RAW格式定义
@{

*/
/// 支持的RAW格式。
enum RAW_FORMAT
{
	RAW_RGGB = 0,		///<RAW格式按RGGB排列
	RAW_GRBG,			///<RAW格式按GRBG排列
	RAW_GBRG,			///<RAW格式按GBRG排列
	RAW_BGGR,			///<RAW格式按BGGR排列
};
/** @} */

/** @name 支持的YUV格式定义
@{

*/
/// 支持的YUV格式。
enum YUV_FORMAT
{
	YUV_YCBYCR = 0,     ///<YUV格式按YCBYCR排列
	YUV_YCRYCB,         ///<YUV格式按YCRYCB排列
	YUV_CBYCRY,         ///<YUV格式按CBYCRY排列
	YUV_CRYCBY,         ///<YUV格式按CRYCBY排列
};
/** @} */

/** @name 图像处理模式选择
@{

*/
/// 图像处理模式选择
enum ISP_MODE
{
	NORMAL = 0,	///普通的处理模式，抓帧得到的是raw数据
	S2DFAST,	///S2DFAST模式，抓帧得到的是RGB数据
	S2DFAST_GPU,///S2DFAST_GPU模式，GPU进行图像处理，抓帧得到的是RGB数据
};

/** @} */

/** @name 图像格式定义
@{

*/
enum IMAGE_FORMAT
{
    FORMAT_RAW10 = 0x00,                
    FORMAT_RAW8 = 0x01,
    FORMAT_YUV = 0x02, 
    FORMAT_RAW16 = 0x03,
    FORMAT_RGB565 = 0x04,
    FORMAT_YUV_SPI	= 0x05,
    FORMAT_MIPI_RAW10 = 0x06,           ///< 5 bytes = 4 pixel...
    FORMAT_MIPI_RAW12 = 0x07,           ///< 3 bytes = 2 pixel...
    FORMAT_YUV_MTK_S = 0x08,            ///< MTK output...
    FORMAT_YUV_10 = 0x09,
    FORMAT_YUV_12 = 0x0a,
    FORMAT_MIPI_RAW14 = 0x0b,           ///< 7 bytes = 4 pixel...
	FORMAT_MIPI_RAW20 =	0x0c,			///< 5 bytes = 2 pixel...
	FORMAT_MIPI_RAW24 =	0x0d,			///< 3 bytes = 1 pixel...
    FORMAT_SAMSUNG_DVS  = 0x010,
	FORMAT_BGR24 = 0x20,                ///< 排列顺序为B，G，R，各8bit
    FORMAT_BGR32 = 0x21,                ///< 排列顺序为B，G，R，0各8bit
    FORMAT_P10 = 0x24,                  ///< 一个像素占两个字节，LSB，0～1023，一般用于MIPI_RAW10转换
    FORMAT_P12 = 0x25,                  ///< 一个像素占两个字节，LSB，0～4095，一般用于MIPI_RAW12转换
    FORMAT_P14 = 0x26,                  ///< 一个像素占两个字节，LSB，0～16383，一般用于MIPI_RAW14转换
    FORMAT_G8 = 0x28,                   ///< 只取G值，8bit
    FORMAT_G10 = 0x29,                  ///< 只取G值，16bit
	FORMAT_GRAY8 = 0x2a,

	FORMAT_RAW24 = 0x40,
};
/** @} */

/** @brief ROI结构体描述
@{

*/
typedef struct DtRoi_s
{
    UINT            x;		    ///< ROI起始点X坐标值
    UINT            y;	        ///< ROI起始点Y坐标值
    UINT            w;		    ///< ROI宽度
    UINT            h;		    ///< ROI高度
}DtRoi_t;

/** @} */

/** @brief 图像数据结构体描述
@{

*/
typedef struct DtImage_s 
{
    IMAGE_FORMAT    format;      ///< 图像格式
    RAW_FORMAT      rawFmt;      ///< RAW格式细节
    YUV_FORMAT      yuvFmt;      ///< YUV格式细节
    UINT            width;       ///< 图像尺寸
    UINT            height;	     ///< 图像尺寸
    BYTE            *data;       ///< 图像数据
    unsigned int    dataSize;    ///< buffer空间大小
    UINT            resv[8];     ///< 保留32字节
}DtImage_t;
/** @} */

/** @} */ // end of group2


/** @defgroup group3 SENSOR相关


* @{

*/

/** @name Iintsenosr/InitSensor SENSOR寄存器参数表中附带的控制字定义
@{

*/
/* SENSOR寄存器参数表中附带的控制字定义 */

/*
示例：
0xffff,0x10;延时16ms
0xfffe,0x02;i2c模式切换为模式2
...
*/
/* ffe1-ffff转义字符 */
#define DTDELAY				0xffff                  //delay
#define DTMODE				0xfffe                  //i2c模式切换，I2CMODE
#define DTOR				0xfffd                  //读到值，与待写入的值相或
#define DTAND				0xfffc                  //读到的值，与待写入的值相与
#define DTPOLLT				0xfffb                  //读寄存器的次数
#define DTPOLL1				0xfffa                  //执行，次数由DTPOLLT设置的，判断val与读回值是否一致，一致则立即退出，不需要执行DTPOLLT设置的次数
#define DTPOLL0				0xfff9                  //执行，次数由DTPOLLT设置的，判断val与读回值是否相反，一致则立即退出，不需要执行DTPOLLT设置的次数
#define DTI2CADDR			0xfff8                  //设置器件地址
#define DTI2CREG			0xfff7                  //poll设置的寄存器
#define DTAFTYPE			0xfff6  //20121223 added... modify the AF Device type
#define DTAFADDR			0xfff5  //20121223 added... modify the AF Device Address..	
#define DTSPIMTKCTRL		0xfff4                  //mtk spi模式接收设置

//ulm928 LVDS SENSOR转义字符
#define DTLANECNT			0xfff3					//lane路数
#define DTXVSSIZE			0xfff2					//xvs size
#define DTXHSSIZE			0xfff1					//xhs size
#define DTVSTART			0xfff0					//roi垂直方向开始坐标
#define DTVEND				0xffef					//roi垂直方向结束坐标
#define DTHSTART			0xffee					//roi水平方向开始坐标
#define DTHEND				0xffed					//roi水平方向结束坐标
#define DTLANEVALID			0xffec					//lvds lane有效使能位
#define DTLINESEL           0xffeb					//线扫sensor行选择，0是raw0开始，1是raw1开始，2是raw2开始

#define DTPAGESTART			0xffea					//page模式 起始地址
#define DTPAGESIZE			0xffdf					//page模式写入字节长度

/*
这2个宏是跟DTDELAY配合使用的：
0xffff,0xfef1
*/
#define DTMACRO_ON			0xfef0					//转义字符开启
#define DTMACRO_OFF			0xfef1					//转义字符关闭
#define DTPAGE				0xfef2					//页操作模式
#define DTEND				0xfeff	


// XHS
// XVS
// V_START
// V_END
// H_START
// H_END
/** @} */

/** @name SENSOR电源电压选择定义（dtccm使用的定义）
@{

*/
/* SENSOR电源电压选择定义 */
#define AVDD_28				0x00
#define AVDD_25				0x01
#define AVDD_18				0x02
#define AVDD_DEFAULT		0x03

#define DOVDD_28			0x00
#define DOVDD_25			0x01
#define DOVDD_18			0x02
#define DOVDD_DEFAULT		0x03

#define DVDD_18				0x00
#define DVDD_15				0x01
#define DVDD_12				0x02
#define DVDD_DEFAULT		0x03

#define AFVCC_33			0x00
#define AFVCC_28			0x01
#define AFVCC_18			0x02
#define AFVCC_DEFAULT		0x03
/** @} */

/** @name SENSOR输入时钟选择定义
@{

*/
/* SENSOR输入时钟选择定义 */
//you can use these enum type ,or use MHZ or hundred KHZ directly 
enum MCLKOUT
{
	MCLK_6M = 0,
	MCLK_8M,
	MCLK_10M,
	MCLK_11M4,
	MCLK_12M,
	MCLK_12M5,
	MCLK_13M5,
	MCLK_15M,
	MCLK_18M,
	MCLK_24M,
	MCLK_25M,
	MCLK_27M,
	MCLK_30M,
	MCLK_32M,
	MCLK_36M,
	MCLK_40M,
	MCLK_45M,
	MCLK_48M,
	MCLK_50M,
	MCLK_60M,
	MCLK_DEFAULT,
};
/** @} */

/** @name 多SENSOR模组通道定义(目前只在DTLC2中存在CHANNEL_B)
@{

*/
/* 多SENSOR模组通道定义(目前只在DTLC2/UH920中存在CHANNEL_B) **/
#define CHANNEL_A                   0x01 /// 只使用A通道
#define CHANNEL_B                   0x02 // 只是用B通道
#define CHANNEL_AB                  0x03 // AB通道同时使用
/** @} */ 

/** @name SensorEnable函数中，使能SENSOR时，RESET/PWDN管脚的电平状态定义
@{

*/
/* SensorEnable函数中，使能SENSOR时，RESET/PWDN管脚的电平状态定义 */
#define RESET_H                     0x02
#define RESET_L                     0x00
#define PWDN_H                      0x01
#define PWDN_L                      0x00
#define PWDN2_H                     0x04
#define PWDN2_L                     0x00
/** @} */

/** @name 支持的SENSOR数据接口定义
@{

*/
/// 定义支持的SENSOR数据接口类型。
typedef enum
{
    SENSOR_PORT_MIPI = 0x00,               ///<MIPI接口
    SENSOR_PORT_PARA = 0x01,               ///<并行同步接口
    SENSOR_PORT_MTK_SERIAL = 0x02,         ///<MTK公司的串行接口
    SENSOR_PORT_SPI = 0x03,                ///<SPI接口
    SENSOR_PORT_SIM = 0x04,                ///<模拟图像，用于测试
    SENSOR_PORT_HISPI = 0x05,              ///<Aptina的HISPI接口,支持packet sp格式
    SENSOR_PORT_ZX_SERIAL = 0x06,          ///<展讯的串行接口
    SENSOR_PORT_SAMSUNG_DVS = 0x07,        ///<三星DVS
    SENSOR_PORT_HISILICON_LVDS = 0x08,     ///<海思LVDS
    SENSOR_PORT_SMARTSENS_DDR_4BIT = 0x09, ///<思特微DDR 4bit
    SENSOR_PORT_BT656 = 0x0a,              ///<bt656格式
    SENSOR_PORT_SMARTSENS_SPI = 0x0b,      ///<思特微SPI 1bit sdr/ddr

    SENSOR_PORT_SLVS_EC = 0x40,            ///<sony slvs-ec

    SENSOR_PORT_DP = 0x50,                 ///<dp接收

    SENSOR_PORT_SONY_LVDS = 0x81,          ///<索尼 LVDS
    SENSOR_PORT_SMARTSENS_LVDS = 0x82,     ///<思特威 LVDS
    SENSOR_PORT_SMARTSENS_LINE_SCAN_LVDS = 0x83,    ///< 思特微线扫
    SENSOR_PORT_PANASONIC_LVDS = 0x85,     ///<松下 LVDS
    SENSOR_PORT_SUPERPIX_LVDS = 0x86,      ///<思比科 LVDS
    SENSOR_PORT_ORBBEC_LVDS = 0x87,         ///<奥比中光
	SENSOR_PORT_LIMICRO_LVDS = 0x88,		///<砺芯微
}SENSOR_PORT;
/** @} */

/** @name 早期版本使用的宏定义
@{

*/
/* 早期版本使用的宏定义 */
#define PORT_MIPI			0   ///<MIPI output
#define PORT_PARALLEL		1   ///<Parallel output
#define PORT_MTK			2   ///<MTK output
#define PORT_SPI			3   ///<SPI output
#define PORT_TEST			4   ///<TEST ouput. FPGA output the image...
#define PORT_HISPI			5   ///<aptina HISPI packet sp...
#define PORT_ZX2_4			6   ///<zhanxun 2bit/4bit packet sp...
#define PORT_MAX			7   ///<maxium... can't support >=PORT_MAX
/** @} */

#define PORT_SONY_LVDS		0x81 
#define PORT_PANASONIC		0x85  
/** @name 定义柔性接口中的各种管脚功能
@{

*/
/* 定义柔性接口中的各种管脚功能 */
typedef enum
{
    PIN_D0 = 0,         ///< 并口数据输入bit0 
    PIN_D1,             ///< 并口数据输入bit1
    PIN_D2,             ///< 并口数据输入bit2
    PIN_D3,             ///< 并口数据输入bit3
    PIN_D4,             ///< 并口数据输入bit4
    PIN_D5,             ///< 并口数据输入bit5
    PIN_D6,             ///< 并口数据输入bit6
    PIN_D7,             ///< 并口数据输入bit7
    PIN_D8,             ///< 并口数据输入bit8
    PIN_D9,             ///< 并口数据输入bit9
    PIN_PCLK,           ///< 并口输入同步时钟
    PIN_HSYNC,          ///< 并口输入行同步
    PIN_VSYNC,          ///< 并口输出场同步
    PIN_MCLK,           ///< 测试盒输出时钟给sensor mclk管脚
    PIN_RESET,          ///< 测试盒输出给sensor reset管脚
    PIN_PWDN,           ///< 测试盒输出给sensor Pwdn管脚
    PIN_PWDN2,          ///< 测试盒输出给sensor Pwdn管脚
    PIN_GPIO1,          ///< 测试盒输出GPIO1
    PIN_SDA,            ///< 测试盒输出I2C总线 SDA管脚
    PIN_SCL,            ///< 测试盒输出I2C总线 SCL管脚
    PIN_NC,             ///< no-connect 输入
    PIN_GPIO2,          ///< 测试盒输出GPIO2
    PIN_GPIO3,          ///< 测试盒输出GPIO3
    PIN_GPIO4,          ///< 测试盒输出GPIO4
    PIN_NC1,            ///< no-connect 输入
    PIN_NC2,            ///< no-connect 输入
    PIN_D10,            ///< 并口数据输入bit10
    PIN_D11,            ///< 并口数据输入bit11
    PIN_SPI_SCK,        ///< 测试盒输出SPI接口 SCK管脚
    PIN_SPI_CS,         ///< 测试盒输出SPI接口 CS管脚
    PIN_SPI_SDI,        ///< 测试盒输出SPI接口 SDI管脚
    PIN_SPI_SDO,        ///< 测试盒输出SPI接口 SDO管脚
    PIN_SPI_SDA,        ///< 测试盒SPI接口 SDA管脚 三线SPI的时候，双向管脚
    PIN_CLK_ADJ_200K,   ///< 输出0-200Khz的可调节时钟频率 =PIN_PWM2_OUT
    PIN_CLK_ADJ_18M,    ///< 输出0-18Mhz的可调节时钟频率 =PIN_PWM1_OUT
    PIN_GPIO5,          ///< 测试盒输出GPIO5
    PIN_GPIO6,          ///< 测试盒输出GPIO6
    PIN_GPIO7,          ///< 测试盒输出GPIO7
    PIN_GPIO8,          ///< 测试盒输出GPIO8
    PIN_FRAME_UPDATE,   ///< 输出帧信号，接收到帧，翻转管脚电平状态
    PIN_D12,            ///< 并口数据输入bit12
    PIN_D13,            ///< 并口数据输入bit13
    PIN_D14,            ///< 并口数据输入bit14
    PIN_D15,            ///< 并口数据输入bit15
    PIN_FRAME_REFRESH,  ///< 输出帧信号，接收到帧，翻转管脚电平状态
    PIN_PWM3_OUT,       ///< PW3输出，可调时钟和占空比
    PIN_PWM4_OUT,       ///< PWM4输出，可调时钟和占空比
    PIN_PWM5_OUT,       ///< PWM5输出，可调时钟和占空比
	PIN_EXT_SCL,        ///< 扩展I2C SCL
	PIN_EXT_SDA,        ///< 扩展I2C SDA
	PIN_FSYNC,          ///< 帧同步信号
	PIN_INT_RES,        ///< 中断信号
	PIN_TX,				///< 串口TX
	PIN_RX,				///< 串口RX
	PIN_PWM1_IN,		///< PWM1输入检测
	PIN_PWM2_IN,		///< PWM2输入检测
    PIN_NUM_MAX         ///< 最大Pin定义个数
}PIN_FUNC;             
/** @} */

#define PIN_PWM1_OUT    PIN_CLK_ADJ_18M
#define PIN_PWM2_OUT    PIN_CLK_ADJ_200K


/** @name 定义柔性接口管脚名称(编号)
@{

*/
/* 定义柔性接口管脚名称(编号) */
typedef enum
{
    PIN_IO1 = 0,
    PIN_IO2,
    PIN_IO3,
    PIN_IO4,
    PIN_IO5,
    PIN_IO6,
    PIN_IO7,
    PIN_IO8,
    PIN_IO9,
    PIN_IO10,
    PIN_IO11,
    PIN_IO12,
    PIN_IO13,
    PIN_IO14,
    PIN_IO15,
    PIN_IO16,
    PIN_IO17,
    PIN_IO18,
    PIN_IO19,
    PIN_IO20,
    PIN_IO21,
    PIN_IO22,
    PIN_IO23,
    PIN_IO24,
    PIN_IO25,
    PIN_IO26,
}SOFT_PIN;
/** @} */

/* Mipi 包信息 */
typedef struct MipiPgInfo_s
{
    UINT vc;                 ///< mipi vc通道
    UINT id;                 ///< MIPI 包id
    UINT uWidth;             ///< 指定宽度，像素
    UINT uHeight;            ///< 指定高度，包个数
    UINT uSize;              ///< 这个包的数据量大小，底层根据这个size信息申请buffer
    UINT uType;              ///< 原始包输出的数据格式，如：MIPI_RAW10，MIPI_RAW12等
    UINT uTargetType;        ///< 设置目标格式,如：D_P10,D_RGB24等
    UINT rsv[15];            ///< 备用
}MipiPgInfo_t;

/* 单VC信息 */
typedef struct SingleVcInfo_s 
{
	UINT uPktNum;					///<VC中传输的MIPI包总数，包含短包和长包
	UINT uDatIDInfo[8];				///<VC通道中出现的DataID，包含短包及长包DataID，最多记录8个DataID
	UINT uDatIDPayloadPktNum[8];	///<VC中出现DataID的Payload包长信息记录，最多支持8个
	UINT uDatIDOnceFramePktNum[8];	///<统计到的一帧数据 VC中出现的对应DataID包个数
	UINT rsv[16];					///< 备用
}SingleVcInfo_t;

/* 多VC信息 */
typedef struct MultVcInfo_s
{
	SingleVcInfo_t Vc[32];	///<32个缓存

	UINT uInfo[32];			///<CSI-2数据流中使用的VC通道信息(VC通道号)
	BOOL bVld[32];			///<缓存通道是否有使用
	UINT uMode;				///<多VC接收模式，1=多VC模式，0=单vc接收方式
	UINT uFs;				///<用于判定帧开始的短包所在VC通道
	UINT uFe;				///<用于判定帧结束的短包所在VC通道
	UINT rsv[16];			///< 备用
}MultVcInfo_t;

/* MIPI ctrl 扩展结构体 */
typedef struct MipiCtrlEx_s
{
    BYTE            byPhyType;          ///< mipi phy set (d-phy_1.5G/d-phy_2.5G,c-phy_2.0G
    BYTE            byLaneCnt;          ///< lane个数设置，有1/2/4lane
	BYTE			resv1[2];			///< 预留字节对齐
    DWORD           dwCtrl;             ///< MIPI ctrl,使用位定义，见MIPI_CTRL_XXXX定义
    UINT            uVc;                ///< 设置接收的图像通道号，0/1/2/3
    BOOL            bVCFilterEn;        ///< 使能过滤其他的虚拟通道
    UINT            uPackID;            ///< 使能输出的ID号
    BOOL            bPackIDEn;          ///< 使能当前设置的ID号输出 */
    BYTE            byLp00MinTime;      ///< LP00的最小时间，单位是：1个时钟周期  
    BYTE            byMipiMode;         ///< 1是多VC模式（该模式下，可指定哪个VC是帧开始或帧结束），0是普通模式（检测到哪个VC短包先出，就作为帧开始和帧结束）
    BYTE            bySetFsForVc;       ///< 指定VC为FS(0,1,2,3)
    BYTE            bySetFeForVc;       ///< 指定VC为FE(0,1,2,3)
    USHORT          uLp01MaxTime;       ///< LP01最大时间，超过这个时间，会卡控不出图,单位是mipi时钟的1/4
	BYTE			resv2[2];			///< 预留字节对齐
    UINT            uSettleTime;		///< uq200上支持，一般不需要设置，根据芯片参数配置
    USHORT          uOpRate;            ///< cphy单位Msps，dphy单位Mbps
    BYTE            resv3[1];
	BYTE			byClkPhaseChk;	    ///< 进入HS状态后延时检测，避免干扰数据导致误判，以SerDes并行时钟周期为单位。
	BYTE			byHs00MinTime;		///< HS00最小值，GP4有效
	BYTE			byCphyPhase;		///< Cphy相位配置，有0和1两个相位可调，GP4有效
    /* 保留，填充0 */
    BYTE            resv[46];

}MipiCtrlEx_t;

/** @name MIPI控制器特性的位定义
@{

*/
/* MIPI控制器特性的位定义 */
#define MIPI_CTRL_LP_EN						(1<<0)      ///< 允许进入LP状态
#define MIPI_CTRL_AUTO_START				(1<<1)      ///< 出现差分信号后自动启动，用于OS测试
#define MIPI_CTRL_NON_CONT					(1<<2)      ///< 使用非连续时钟
#define MIPI_CTRL_FULL_CAP					(1<<3)      ///< 完整数据包获取，包括包头和CRC16校验，将导致每行图像数据增加6字节
#define MIPI_CTRL_CLK_LP_CHK				(1<<4)      ///< 对CLK Lane的LP状态进行检测，强制要求MIPI TX端的Clk Lane必须进入一次LP状态
#define MIPI_CTRL_CLK_LP01_CHK				(1<<6)      ///< 对CLK Lane的LP-10状态进行检查，如果没有这个状态，将会一直等待
#define MIPI_CTRL_DAT_LP01_CHK				(1<<7)      ///< 对DATA Lane的LP-10状态进行检查，如果没有这个状态，将会一直等待
#define MIPI_CTRL_SHORT_PACKET_EN			(1<<10)     ///< 短包输出使能
#define MIPI_CTRL_DESCRAMBLING				(1<<11)     ///< mipi解扰使能，注意：必须是mipi要输出加扰才能使能这个位
#define MIPI_CTRL_LRTE_EN					(1<<15)     ///< 开启LRTE模式
#define MIPI_CTRL_LANE_CNT_DET_MANUAL		(1<<16)     ///< lane/trio个数手动设置，不使能是自动检测lane/trio个数
#define MIPI_CTRL_CLK_PHASE_CHK				(1<<17)     ///< 针对思特威需要检查D-PHY时钟相位的需求
#define MIPI_CTRL_LP00_MIN_TIME_DET_MANUAL	(1<<18)     ///< LP00的最小时间使能手动设置值生效
#define MIPI_CTRL_CPHY_WEAK_MODE			(1<<19)     ///< C-PHY弱信号
#define MIPI_CTRL_PHASE_LOOK				(1<<20)     ///< 相位锁定
/** @} */


/** @} */

/** @name MIPI Phy type选择
@{

*/
/* MIPI Phy type选择 */
#define	MIPI_DPHY_1_5G              0       ///< 1.5G以下不带deskew校准的DPHY选择
#define MIPI_DPHY_2_5G              1       ///< 1.5G以上带deskew校准的DPHY选择
#define MIPI_CPHY                   2       ///< CPHY选择
/** @} */

/** @name 同步并行接口特性的位定义
@{

*/
/* 同步并行接口特性的位定义 */
#define PARA_PCLK_RVS               (1<<3)	///< PCLK取反
#define PARA_VSYNC_RVS              (1<<4)	///< VSYNC取反
#define PARA_HSYNC_RVS              (1<<5)	///< HSYNC取反
#define PARA_AUTO_POL               (1<<6)	///< VSYNC,HSYNC极性自动识别
/** @} */

/** @name 同步并行接口,3bit用于选择位宽
@{

*/
/* 3bit用于选择位宽 */
#define PARA_BW_8BIT                0		
#define PARA_BW_10BIT               1
#define PARA_BW_12BIT               2
#define PARA_BW_16BIT               3
/** @} */

/** @name HiSPI接口特性的位定义
@{

*/
/* HiSPI接口特性的位定义 */
// 2bit用于选择位宽
#define	HISPI_WW_10BIT              0
#define	HISPI_WW_12BIT              1
#define	HISPI_WW_14BIT              2
#define	HISPI_WW_16BIT              3
/** @} */

/** @name 模拟图像模块特性的位定义
@{

*/
/* 模拟图像模块特性的位定义 */
// 2bit用于选择模拟图像的样式
#define	SIM_STYLE1                  0       ///< 输出固定颜色
#define	SIM_STYLE2                  1       ///< 水平渐变
#define	SIM_STYLE3                  2       ///< 垂直渐变
#define	SIM_STYLE4                  3       ///< 每帧渐变
/** @} */

/** @name slvsec模式选择
@{

*/
typedef enum 
{
    SLVSEC_4LANE_MODE = 0,          ///< SLVS-EC 4lane
    SLVSEC_8LANE_MODE = 1,          ///< SLVS-EC 8lane
    SLVSEC_4PLUS4LANE_MODE = 2,     ///< SLVS-EC 4+4lane
    SLVSEC_6PLUS6LANE_MODE = 3,     ///< SLVS-EC 6+6lane
}SlvsEcModeSel;
/** @} */

typedef enum 
{
	SDR_1B,				
	SDR_2B,
	DDR_1B,
	DDR_2B,
	SDR_4B,
	DDR_4B,
}SPIImageType;

/** @name slvsec接收的数据格式选择
@{

*/
// 4+4或者6+6模式生效
typedef enum 
{
    RAW10 = 0,
    RAW12 = 1
}SlvsEcFormat;
/** @} */

/** @brief SLVS-EC接口配置
@{
*/
/* 用于配置SLVS-EC接口 */
typedef struct SlvsECCtrl_s
{
    UINT            uLaneNum;        ///< lane个数
    UINT            uXvsSize;        ///< 设备输出的场同步信号个数
    UINT            uXhsSize;        ///< 设备输出的行同步信号个数
    SlvsEcModeSel   Mode;            ///< 见SlvsEcModeSel
    SlvsEcFormat    Format;          ///< slvs数据格式，有raw10/raw12
    UINT            Rsv[14];         ///< 备用
}SlvsECCtrl_t;
/** @} */

/** @brief Pwm输出
@{
*/
/* 用于配置柔性IO PWM */
typedef struct PwmOutConfig_s
{
    BOOL        bEn;                   ///< 使能输出
    UINT        uDiv;                  ///< 预分频系数，设置范围100-65535
    float       fDutyCycle;            ///< 高电平占空比，如设置75，则表示高电平75%，低电平25%
    float       fPeriod;               ///< 周期，单位us
	UINT		uIo;				   ///< GPIO号索引，如果是0，则表示用SetSoftPin柔性IO配置的;不为0，则为GPIO编号，支持Pin中存储的编号值从1开始，1表示GPIO1,2表示GPIO2...
	UINT        Rsv[14];               ///< 备用

}PwmOutConfig_t;
/** @} */

/** @brief Pwm输入
@{
*/
/* 用于配置柔性IO PWM */
typedef struct PwmInConfig_s
{
	BOOL        bEn;                   ///< 使能输入
	UINT		uPwmIndex;			   ///< Pwm索引，从0开始计数,0:pwm1,1:pwm2,2:pwm3,3:pwm4
	UINT		uPwmStatisticCnt;	   ///< 统计计数值，用来计算频率的平均值
	UINT        Rsv[13];               ///< 备用
}PwmInConfig_t;

/** @} */

/** @} */

/** @brief Pwm结果
@{
*/
/* 用于配置柔性IO PWM */
typedef struct PwmInMeasureResult_s
{
	float       fDutyCycle;            ///< 高电平占空比
	float       fPeriod;               ///< 周期，单位us
	double		fFrequency;			   ///< 频率，单位hz
	UINT		uPwmIndex;			   ///< Pwm索引，从0开始计数,0:pwm1,1:pwm2,2:pwm3,3:pwm4
	UINT        Rsv[13];               ///< 备用
}PwmInMeasureResult_t;

/** @} */

/** @name dp接收速率选择
@{

*/
typedef enum
{
    DP_REV_1_62G = 0,               ///< 1.62Gbps
    DP_REV_2_7G = 1,                ///< 2.7Gbps
    DP_REV_5_4G = 2,                ///< 5.4Gbps
    DP_REV_6G = 3,                  ///< 6.0Gbps
    DP_REV_6_6G = 4,                ///< 6.6Gbps
    DP_REV_8_1G = 5,                ///< 8.1Gbps
    DP_REV_9G = 6,                  ///< 9Gbps
    DP_REV_9_6G = 7,                ///< 9.6Gbps
    DP_REV_12G = 8,                 ///< 12Gbps
    DP_REV_8_7G = 9                 ///< 8.7G
}DpRecRateSel;
/** @} */

/** @name DP控制器特性的位定义
@{

*/
/* DP控制器特性的位定义 */
#define DP_CTRL_ALP_EN                  (1 << 0)      ///< ALP接收模式设置
#define DP_CTRL_DESCRAMBLING_BYPASS     (1 << 1)      ///< 关闭DP解扰使能
#define DP_CTRL_CRC_CHECK               (1 << 2)      ///< DP接收crc检查使能
#define DP_CTRL_ORIG_STREAM             (1 << 5)      ///< 输出原始数据流，默认是mipi字节流
#define DP_CTRL_LANE_CNT_DET_MANUAL     (1 << 16)     ///< lane个数手动设置，不使能是自动检测lane个数
/** @} */

/** @name Pin 连接和极性选择
*/
#define DP_PIN_ML_K     0
#define DP_PIN_ML_M     1
#define DP_PIN_ML_W     2

/** @} */

/** @name 
*/
#define DP_ALP_TYPE_NORMAL           0           // 正常ALP模式
#define DP_ALP_TYPE_8_7G_SPECIAL1    1           // 8.7G特殊的ALP模式，C1跟H1
/** @} */

/** @brief dp接收控制
@{

*/
typedef struct DpRecCtrl_s
{
    DpRecRateSel          uRateSel;           ///< 速率设置
    DWORD                 dwCtrl;             ///< dp接收ctrl设置，位定义方式，参考DP_CTRL_XXX
    BYTE                  byLaneCnt;          ///< lane个数设置
    BYTE                  byRxPol;            ///< 极性配置
    BYTE                  Rsv1[2];            ///< 预留
    DWORD                 dwLaneSet;          ///< lane设置,b[3:0]lane0，b[7:4]lane1,b[11:8]lane2,b[15:12]lane3
    BYTE                  byFormat;           ///< 图像格式
    BYTE                  PinType;            ///< 快速指定pin定义，底层集成了几组pin定义，免去手动设置的繁琐,默认选择DP_PIN_ML_W
    BYTE                  byAlpType;          ///< 在DP_CTRL_ALP_EN打开情况下才有用，配置值参见DP_ALP_TYPE_XXX
	BYTE                  Rsv2[1];            ///< 预留
	float				  fRate;			  ///< 速率
    BYTE                  Rsv[48];            ///< 预留
}DpRecCtrl_t;

/** @} */

/**************************************************************************************** 
*
* I2C总线相关
*
****************************************************************************************/
/**@brief 多寄存器读写
@{

*/

typedef struct MultRegRwInfo_s
{
    UCHAR   uRw;        ///< 1读，0写
    UCHAR   Rsv1[3];    ///< 备用
    UINT    uRegAddr;   ///< 寄存器地址
    UINT    uRegData;   ///< 寄存器写入/读回的数据
    UINT    uRegDelay;  ///< 写/读寄存器之间的延时时间，单位us
	double  lfTimeStamp;///< 读寄存器数据，附带8字节的时间戳；需要固件支持SpecialI2c，否则时间戳功能无效
    UINT    Rsv2[2];    ///< 备用
}MultRegRwInfo_t;

typedef struct MultAddrRegRwInfo_s
{
	UCHAR*   pRw;		///< 1读，0写
	UCHAR*   pMode;		///< 模式
    UCHAR*   pDevAddr;  ///< 器件地址
    UINT*    pRegAddr;  ///< 寄存器地址
    UINT*    pRegData;  ///< 寄存器写入/读回的数据
    USHORT*  pRegDelay; ///< 写/读寄存器之间的延时时间，单位us
	USHORT   uRegNum;	///< I2C总包数量
	USHORT   Rsv1;		///< 备用
    UINT     Rsv2[4];   ///< 备用
}MultAddrRegRwInfo_t;

/** @} */

enum TriggerMode
{
    TRIGGER_RISING_EDGE = 0,        ///<上升沿触发
    TRIGGER_FALLING_EDGE = 1,       ///<下降沿触发
};

/** @brief triggerio配置
@{

*/
typedef struct TriggerIoCfg_s
{
    BOOL            bEnable;        ///< 使能输出
    TriggerMode     Mode;           ///< 模式设置
    UINT            uDelay;         ///< 单位us，read启动后延时uDelay时间输出
    UINT            uHold;          ///< Trigger输出后，保持多长时间高/低电平，单位us
    UINT            Rsv[4];
}TriggerIoCfg_t;

/** @} */

/** @brief Fsync/Vsync测量配置
@{

*/
typedef struct _FVsyncMeasure
{
	UINT32 uFpVp;		///< Fsync上升沿与相邻的Vsync上升沿的时间差(单位ns)
	UINT32 uFpVn;		///< Fsync上升沿与相邻的Vsync下降沿的时间差(单位ns)
	UINT32 uFnVp;		///< Fsync下降沿与相邻的Vsync上升沿的时间差(单位ns)
	UINT32 uFnVn;		///< Fsync下降沿与相邻的Vsync下降沿的时间差(单位ns)
	UINT32 uRsv[8];		///< 预留
}FVsyncMeasure;
/** @} */

/**@brief special i2c
@{
*/
typedef struct I2cRwEnhance_s
{
    BYTE    uRw;        ///<1读,0写
    BYTE    bySlave;    ///8位器件地址
    BYTE    Rsv1[2];    ///<预留
    UINT    uRegAddr;   ///<寄存器地址
    UCHAR   uRegSize;   ///<寄存器地址字节数大小,0,1,2,3,4
    USHORT  uDataSize;  ///<写入或者读回数据size大小
    BYTE    Rsv2;       ///<预留
    UINT    uDelay;     ///<本次I2C操作的delay时间，单位us
    BYTE    *pData;     ///<写入或读出的数据
}I2cRwEnhance_t;
/** @} */

/* @bfief i2c timer read
@{
*/
typedef struct RdRegTimerCfg_s
{
	BYTE    byBusSel;           ///<总线选择
    BYTE    byDevAddr;          ///<器件地址 I2C器件地址1
	BYTE    byDevAddr2;         ///<器件地址 I2C器件地址2		新增2023/12/15
    BYTE    Rsv1[6-1];          ///<预留
    BYTE    *pRegAddr;          ///<寄存器地址
    USHORT  uRegAddrSize;       ///<寄存器地址字节数
    USHORT  uDataSize;          ///<配置读出寄存器的数据块字节数
    int     iCycleTimeUs;       ///<配置定时器时间, 单位us
    int     iNum;               ///<决定底层缓存读回I2C数据的buffer大小
    int     Rsv2[9];            ///<预留
}RdRegTimerCfg_t;
/** @} */

/* @name 返回寄存器数据，ID，时间戳等信息
@{
*/
typedef struct RtnRdRegInfo_s
{
	USHORT  uDevAddr;           ///<器件地址					新增2023/12/15
    USHORT  uID;                ///<I2C数据包ID，从1开始计数
    BYTE    Rsv[10-2];          ///<预留
    double  lfTimeStamp;        ///<返回当前包的时间戳
    int     iDataSize;          ///<用户指定pData大小，数据量大小应该大于等于RdI2cTimer_t结构体当中uDataSize
    BYTE    *pData;             ///<返回数据，
}RtnRdRegInfo_t;
/** @} */

/* @name special I2C信息定义
@{
*/
typedef struct SpecialI2cConfigInfo_s
{
	BYTE	byI2cBus;			///<I2C总线选择：0表示SensorI2C总线；1表示扩展I2C总线
	BYTE    Rsv[1];				///<预留（字节对齐）

	USHORT	uI2cFormat;			///<I2C返回数据格式（1支持，0忽略）：bit0表示ID（0~65535）；bit1表示时间戳（单位us，8字节）；

	BYTE    Rsv2[7];           ///<预留
}SpecialI2cConfigInfo_t;
/** @} */

/* @name special I2C命令包定义
@{
*/
typedef struct SpI2cPacket_s
{
    UINT    uPacketIdx;            ///<包ID计数
    UINT    uRepeateCnt;           ///<当前包重复次数
    int     Rsv1[4];               ///<预留
    UINT    uAfterDelay_us;        ///<i2c命令结束完，延时时间设置us
    BYTE    bySlave;               ///<器件地址
    BYTE    Rsv2[3];               ///<预留
    int     sizeW;                 ///<写入数据量，pDataW大小
    int     sizeR;                 ///<读回数据量
    BYTE    *pDataW;               ///<写入的数据包buf
}SpI2cPacket_t;

/*speciali2c包发送的格式*/
typedef struct Speciali2cPkt_s
{
    int iPackid;					///包ID
    int iRePeatecnt;				///当前包ID，重复次数
    BOOL bTriggerOut;				///是否triggerout
    DWORD dwAfterDelay_us;			///当前周期结束后延时
    BYTE bySlave;					///当前包的从机地址
    BYTE byRsv;
    BYTE bySizeW;					///pDataW大小
    BYTE bySizeR;					///读回数据大小
    BYTE *pDataW;					///当前包的发送的数据
}Speciali2cPkt_t;

/*special i2c start*/
typedef struct SpecialI2cStart_s
{
    int iStartPktid;
    int iEndPktid;
    int iTimeOut;
    int iSize;
    BYTE *pData;
}SpecialI2cStart_t;

/** @} */

/*
* 常见的寄存器读写模式
* I2C mode definiton
* when read or write by I2c ,should use this definiton...
* Normal Mode:8 bit address,8 bit data,
* Samsung Mode:8 bit address,8 bit data,but has a stop between slave ID and addr...
* Micron:8 bit address,16bit data...
* Stmicro:16bit addr ,8bit data,such as eeprom and stmicro sensor...
*/
/** @name I2C模式定义
@{

*/
///I2C模式定义。
enum I2CMODE
{
    I2CMODE_NORMAL=0,   ///< 8 bit addr,8 bit value 
    I2CMODE_SAMSUNG,    ///< 8 bit addr,8 bit value,Stopen
    I2CMODE_MICRON,     ///< 8 bit addr,16 bit value
    I2CMODE_STMICRO,    ///< 16 bit addr,8 bit value, (eeprom also)
    I2CMODE_MICRON2,    ///< 16 bit addr,16 bit value
    I2CMODE_A2_D4,      ///< a2_d4：16 bit addr，32 bit value
    I2CMODE_A4_D2,      ///< a4_d2：32 bit addr，16 bit value
    I2CMODE_A4_D4,      ///< a4_d4：32 bit addr，32 bit value
};

///sensor i2c上拉电阻选择
enum I2CPULLUPRESISTOR
{
    PULLUP_RESISTOR_1_5K=0,     ///< 1.5K pull up resistor
    PULLUP_RESISTOR_4_7K=1,     ///< 4.7K pull up resistor
    PULLUP_RESISTOR_0_56K = 2,  ///< CMU958/DMU956/DMU927这些机型无560欧姆上拉电阻，如设置560欧姆上拉电阻，测试盒实际会使用1.14K上拉电阻
    PULLUP_RESISTOR_CLOSED=3,   ///< 关闭上拉电阻
    PULLUP_RESISTOR_1_14K = 4,
    PULLUP_RESISTOR_0_375K = 5,
    PULLUP_RESISTOR_0_407K = 6,
    PULLUP_RESISTOR_0_5K = 7,
};
/** @} */

/** @name i2c/i3c模式定义
@{
*/
typedef enum 
{
    I2C_BUS = 0,                //sensori2c
    I3C_SDR_BUS = 1,            //sensori3c srd模式

	I2C_BUS_EXT = 10,           //扩展I2C总线

	SPI_BUS_FULL_DUPLEX = 20,	//SPI（全双工）
	SPI_BUS_HALF_DUPLEX = 21,	//SPI（半双工）
	SPI_BUS_DJ = 22,			//SPI（大疆）

}SensorBusType;

/**@} */

/**@brief I3C配置命令定义
@{

*/

typedef enum 
{
    I3C_ASSIGN_DYNAMIC_ADDRESS_FROM_STATIC = 0x87,		///< SETDASA动态地址分配
    I3C_SET_MWL = 0x89,
    I3C_SET_MRL = 0x8A,
    I3C_GET_MWL = 0x8B,
    I3C_GET_MRL = 0x8C,
    I3C_HIS_WRITE = 0x96,       // his写命令
    I3C_HIS_READ = 0x97,        // his读命令
}I3CCmdDef;
/** @} */


/**@brief I3C配置
@{

*/

typedef struct I3CConfig_s
{
    BYTE        byStaticAddr;   ///< 静态地址，一般为从I2C的地址
    I3CCmdDef   uCmd;           ///< I3C命令码，如动态分配地址命令，读写最大长度设置等命令，该命令一般是1个字节，这里用4个字节应该足够了
    UINT        uCmdSize;       ///< 动态分配地址命令和读写长度设置命令都为1字节
    UINT        uSize;          ///< 配置数据size的大小
    BYTE        *pData;         ///< 配置数据
}I3CConfig_t;
/** @} */

/**@brief his I3C读
@{

*/

typedef struct I3CCmdForHis_s
{
    BYTE        byStaticAddr;   ///< 静态地址，一般为从I2C的地址
    I3CCmdDef   uCmd;           ///< I3C命令码，如动态分配地址命令，读写最大长度设置等命令，该命令一般是1个字节，这里用4个字节应该足够了
    UINT        uCmdSize;       ///< 动态分配地址命令和读写长度设置命令都为1字节
    UINT        uAddr;          ///< 发送地址
    UINT        uAddrSize;      ///< 地址size大小
    BYTE        Rsv[4];         ///< 预留
    UINT        uSize;          ///< 配置数据size的大小
    BYTE        *pData;         ///< 配置数据
}I3CCmdForHis_t;
/** @} */


/** @name SPI模式定义
@{

*/
///SPI模式定义
enum SPIMODE
{
    SPIMODE_SONY_A1_D1=0x81,    ///< 8 bit addr,8 bit value		bit0-bit7
    SPIMODE_SONY_A1_D2,         ///< 8 bit addr,16 bit value
    SPIMODE_SONY_A2_D1,         ///< 16 bit addr,8 bit value
    SPIMODE_SONY_A2_D2,         ///< 16 bit addr,16 bit value

    SPIMODE_40KFPS_A1_D1=0x88,

    /* panasonic lsb */
    SPIMODE_PANASONIC_A1_D1=0x91,       ///< 8 bit addr,8 bit value 
    SPIMODE_PANASONIC_A1_D2,            ///< 8 bit addr,16 bit value
    SPIMODE_PANASONIC_A2_D1,            ///< 16 bit addr,8 bit value
    SPIMODE_PANASONIC_A2_D2,            ///< 16 bit addr,16 bit value

    /* smartsens msb */
    SPIMODE_SMARTSENS_A2_D1=0xcb,       ///< 16 bit addr,8 bit value  bit15-bit0

    /* gpixel spi msb*/
    SPIMODE_GPIXEL_A2_D1=0xd0,           ///<one control bit,9 bit addr,8 bit value
};
/** @} */



#define	MASTER_CTRL_DATA_SHIFT  (1<<0)      ///< 数据移位模式  0: MSB 先出, 1: LSB 先出
#define	MASTER_CTRL_CPOL        (1<<1)      ///< 时钟极性（Clock polarity） - SPI空闲时，时钟信号的电平状态[ 1: 空闲时高电平 ，0: 空闲时低电平 ]

/* 时钟相位（Clock phase） - SPI在SCLK第几个边沿采样
CPHA=0，表示第一个边沿：
CPOL=0，idle时候的是低电平，第一个边沿就是从低变到高，所以是上升沿；
对于CPOL=1，idle时候的是高电平，第一个边沿就是从高变到低，所以是下降沿；
CPHA=1，表示第二个边沿：
CPOL=0，idle时候的是低电平，第二个边沿就是从高变到低，所以是下降沿；
CPOL=1，idle时候的是高电平，第一个边沿就是从低变到高，所以是上升沿；
*/
#define	MASTER_CTRL_CPHA        (1<<2)
#define MASTER_CTRL_DELAY       (1<<3)      ///< 0:寄存器间隔无需延时；1：寄存器写完后需延时，延时值由data中给出

#define MASTER_CTRL_THREE_WIRE	(1<<4)      ///< 0:是四线模式，1是三线模式
#define MASTER_CTRL_RW_ENDGE_DIFF    (1<<5) ///< 读写边沿是否一致，1是相反，0是一致
#define MASTER_CTRL_CS_HIGH     (1<<6)      ///< 片选高有效，1是高有效，0是低有效
#define MASTER_CTRL_GPIXEL     (1<<7)      ///< 长光sensor

/** @name SPI配置结构体
@{

*/
/* SPI配置结构体 */
typedef struct MasterSpiConfig_s
{
	double  fMhz;               ///< 配置SPI的时钟
	BYTE    byWordLen;          ///< Word length in bits. 0： 8bit ；1：16bit（暂时无效废弃），默认为0
	BYTE    byCtrl;             ///< 支持的位控制码：MASTER_CTRL_DATA_SHIFT/MASTER_CTRL_CPOL/ MASTER_CTRL_CPHA/MASTER_CTRL_DELAY
	USHORT  uInterval;			///< 间隙时间配置,单位40ns 参数范围0~65535，0表示不加入间隙时间，1表示40ns,2表示80ns....
	BYTE    Rsv[62];            ///< 保留 */
}MasterSpiConfig_t;
/** @} */

/** @name 返回设备的通讯接口类型
@{

*/
typedef enum 
{
    USB2_0 = 0,
    USB3_0 = 1,
    USB3_1 = 2,

    FIBRE = 20      /// 光纤产品
}DevLinkType;
/** @} */

/** @brief 设备链接描述结构体
@{
*/
/* 返回设备的链接信息 */
typedef struct DevLinkStatus_s
{
    BOOL            bLinkOk;        //< 指示当前link是否正常(已经连接设备)
    double          lfLinkRate;     //< Link 速度，单位Mbps	
    DevLinkType     LinkType;       //< Link 类型
    ULONG           uTransLineNum;  //< 当前设备传输线个数，如USB或者光纤线个数
    ULONG           uPortMask;      //< 当前设备光口MASK信息，BIT0为1，表示光口0使用，BIT1为1，表示光口1使用
    ULONG           Rsv[30];
}DevLinkStatus_t;
/** @} */

/** @brief 设备信息描述结构体
@{
*/
/* 返回设备的信息 */
typedef struct DevInfo_s
{
    /* 原始帧，盒子采集到的总帧数,这些帧是sensor原始输出的 */
    UINT            uOrgFrameCnt;

    /* 电脑端驱动底层GrabFrame接口获取的正确帧计数，这些帧经过进一步检查，例如宽高是否匹配，确保了正确性；
一般情况下：uOrgFrameCnt >= uFrameOkCnt + uFrameProblemCnt; */
    UINT            uFrameOkCnt;

    /* 采集的问题帧计数，这些帧可能是宽高不匹配或者有crc，ecc等错误的帧 */
    UINT            uFrameProblemCnt;

    /* 被提交/输出的有效帧计数,由GrabFrame接口输出给上层调用者的帧计数 */
    UINT            uFrameOutCnt;

    /* sensor帧率 */
    float           fSsrFps;

    /* dp接收pn9模式下，crc计算change个数，无误码正常接收应该是0*/
    UINT            uCrcChangeCnt;

	/*DDR FIFO空间满错误计数*/
	UINT			uFifoFullErrCnt;

	/*校验和错误计数*/
	UINT			uCheckSumErrCnt;

	/*HDR错误计数*/
	UINT			uHdrErrCnt;

	/*ID错误计数*/
	UINT			uIdErrCnt;

	/* 备用 */
	UINT            Rsv[58];
}DevInfo_t;
/** @} */


/*example：
注意：uVcBitMap是把vc通道号映射成bit方式，如下转成用户想要的结果：
for (i = 0; i<4; i++)
{
    if (uVcBitMap & (1 << i))
    {
        str.Format("virtual channel %d used\r\n", i);
    }
}
注意：uDataIdBitMap是把dataid通道号映射成bit方式，如下转成用户想要的结果
for (i = 0; i<64; i++)
{
    if (uDataIdBitMap & (1 << i))
    {
        str.Format("DataID:%02X found\r\n", i);
        strResult += str;
    }
}
*/

/** @brief 获取MIPI状态信息
@{
*/
typedef struct MipiStatusInfo_s
{
    /* ecc corrected cnt */
    UINT            uEccCorrectedCnt;

    /* packets per frame */
    UINT            uPacketsPerFrame;

    /* lane lock state,bit0-bit3依次表示lane1-lane4的状态,为1表示锁定了 */
    UINT            uLaneLockState;

    /* vc bitmap */
    UINT            uVcBitMap;

    /* data id bitmap */
    UINT64          uDataIdBitMap;

    /* 备用 */
    UINT            Rsv[125];
    
}MipiStatusInfo_t;
/** @} */

/** @brief 抓帧策略设置
@{
*/
/* 用户可设置对数据量不匹配或crc等错误帧过滤 */
typedef struct FrameFilter_s
{
    /* 对有crc错误的帧过滤 */
    bool            bCrcErrorFilter;

    /* 对size不匹配的帧过滤 */
    bool            bSizeErrorFilter;

	/* 对Ecc错误忽略，底层FPGA控制*/
	BYTE            uEccErrorIgnore;

	BYTE            Rsv2[3];
    /* 备用 */
    UINT            Rsv[63];

}FrameFilter_t;
/** @} */

/** @} */ // end of group3

/**************************************************************************************** 
*
* 图像数据采集相关 
*
****************************************************************************************/


/** @defgroup group4 图像数据采集相关
@{

*/

/** @name FrameBuffer模式配置
@{

*/
///< FrameBuffer模式配置

#define	BUF_MODE_NORMAL     0           ///< 一般模式，缓存效果相当于FIFO；当缓存量超过；\n
                                        // 缓存上限设置时，新的帧将不会被写入到缓存；

#define BUF_MODE_SKIP       1           ///< 跳帧模式，缓存中的帧将不会出现“排队”现象；

#define	BUF_MODE_NEWEST     2           ///< NEWEST模式，目前只对PCI-E接口的机型有效；\n
                                        // GrabFrame将获取最新缓存到的帧；对于其他机型\n
                                        // 将等效于SKIP模式
/** @} */

/** @brief 拼帧设置
@{
*/
typedef struct sFrameMerge
{
	BOOL     bEnable;				///< 使能拼帧，为1拼帧才生效
	USHORT   uFrameNum;				///< 设置合并的帧数,1-65535
	USHORT	 resv[5];				///< 预留
}FrameMerge;
/** @} */

/** @brief Buffer信息描述结构体
@{
*/
/* 用于配置FrameBuffer */
typedef struct _FrameBufferConfig
{
    ULONG       uMode;              ///< frame buffer模式选择,见BUF_MODE_XXXX
    ULONG       uBufferSize;        ///< 设备中的帧缓存大小(字节)，设备固定，用户设置无效
    ULONG       uUpLimit;           ///< 设备缓存上限设置(字节)，缓存量超过这个上限时，新的帧将被丢弃
    ULONG       uBufferFrames;      ///< 驱动的帧缓存数,只对BUF_MODE_NORMAL模式有效
    BOOL        bLite;              ///< 是否使用紧凑的内存申请方式，ISP使用的内存将不预先申请
    BOOL        bDropDelayFrame;    ///< 是否丢弃存储在软件buff停留了很久的帧，如调用GrabFrame抓帧不及时，停留在软件队列当中的帧时间很久，可使能丢弃
    ULONG       uFrameLatencyTime;  ///< bDropDelayFrame为true时候有效，停留在软件队列当中的时间设置，大于这个时间的，丢弃,单位us
    UINT        uSetMultFramesAvg;  ///< 设置多帧平均帧数，计算多帧像素平均值后输出平均值的帧，大于1且目标格式是像素才生效，如果是0和1表示不取平均
    ULONG       resv[11];           ///< 保留，填充0
}FrameBufferConfig;
/** @} */

/** @brief 帧相关信息，与帧数据对应，对其进行描述
@{
*/
/* 帧相关信息，与帧数据对应，对其进行描述 */
typedef struct sFrameInfo
{
    BYTE	byChannel;      ///< 图像通道标识，只有UH920/DTLC2支持
    USHORT	uWidth;         ///< 图像的宽度，单位字节
    USHORT	uHeight;        ///< 图像的高度，单位字节
    UINT	uDataSize;      ///< 数据量大小，单位字节
    UINT64	uiTimeStamp;    ///< 帧开始时间戳值，单位us
}FrameInfo;
/** @} */
/** @brief 扩展的帧信息结构体
@{
*/
// 扩展的帧信息结构体
typedef struct sFrameInfoEx
{
    BYTE	byChannel;          ///< 图像通道标识，只有UH920/DTLC2支持
    BYTE    byMipiVcChannel;    ///< MIPI虚拟通道号，只对MIPI接口有效
    BYTE    resvl[2];           ///< 保留2字节，填充0
    BYTE    byImgFormat;        ///< 图像格式，D_RAW8、D_RAW10...
    USHORT	uWidth;             ///< 图像的宽度，单位字节
    USHORT	uHeight;            ///< 图像的高度，单位字节
    UINT	uDataSize;          ///< 数据量大小，单位字节
    UINT	uFrameTag;          ///< 帧标识
    double  fFSTimeStamp;       ///< 帧开始的时间戳，单位us
    double  fFETimeStamp;       ///< 帧结束的时间戳，单位us
    UINT	uEccErrorCnt;       ///< 每帧的ECC错误计数，只对MIPI接口有效
    UINT	uCrcErrorCnt;       ///< 每帧的CRC错误计数，只对MIPI接口有效
    UINT	uFrameID;           ///< 帧计数
    USHORT  uXpos;              ///< vivo需求找圆心中心点用到 x轴坐标
    USHORT  uYpos;              ///< y轴坐标
	void    *pFlCrcBuff;		///< 帧行CrcBuff缓存指针
	UINT	uFlCrcBuffBitNum;	///< 帧行CrcBuff缓存Bit个数（1行由1个Bit位表示，第0字节的0Bit表示帧的第一行，依此类推，单字节Bit位对齐）
    UINT	resv[59-2];         ///< 保留，填充0
}FrameInfoEx;
/** @} */
/** @name FrameInfoEx::uFrameTag位定义解释 
@{

*/
/* FrameInfoEx::uFrameTag */
/* 采集已经开始 */
#define FRM_INFO_TAG_STARTED        1

/* 帧已经采集完成 */
#define FRM_INFO_TAG_GRAB_OK        (1<<1)

/* 帧已经处理完成 */
#define FRM_INFO_TAG_PROC_OK        (1<<2)

/* 帧已经坏掉或无效，可能采集和处理了一半，后续无需再处理，也不用提交 */
#define FRM_INFO_TAG_BAD            (1<<4)

/* 可能出现了一些错误，但是数据量完整 */
#define FRM_INFO_TAG_ERR            (1<<5)

/* Restart之后的第一帧 */
#define FRM_INFO_TAG_FIRST          (1<<6)

/* TestWindow使能标志 */
#define FRM_INFO_TAG_TW             (1<<7)
/** @} */

/** @brief 设置帧CRC校验结果缓存数据结构
@{
*/
/* 帧CRC缓存，对其进行描述 */
typedef struct sFrameCrcBufInfo
{
    BYTE	byCrcBufEn;			 ///< 帧行CrcBuf缓存使能.（1:开启功能,0:关闭功能）
	BYTE    resv[7];			 ///< 保留，填充0
    USHORT	uCrcBufSize;         ///< 帧行CRC缓存数据大小申请（字接）. 建议：数据大小 = 帧行数 / 8 + 1，每行由1bit表示
	USHORT  resv2[7];			 ///< 保留，填充0

}FrameCrcBufInfo;
/** @} */

/** @name 预览窗口定义
@{

*/
/* 预览窗口定义 */
#define PREVIEW_ROI_B0    0x00
#define PREVIEW_ROI_B1    0x01
#define PREVIEW_ROI_B2    0x02
#define PREVIEW_ROI_B3    0x03
#define PREVIEW_ROI_B4    0x04
#define PREVIEW_ROI_GRID  0x05
#define PREVIEW_QUICK	  0x06 
#define PREVIEW_FULL      0x07
#define PREVIEW_NOTHING   0x08
/** @} */

typedef enum
{
	WORK_NORMAL = 0x00,
	WORK_STANDBY_CURRENT_TEST = 0x40,		/// 进入待机电流测试模式，会关闭I2C的上拉电阻
	WORK_OS_TEST = 0x60
}WorkMode;
/** @} */

/** @} */ // end of group4

/******************************ae功能***************************/

/* 根据sensor type可预先知道跟曝光相关的寄存器信息 */
typedef enum
{
	SENSOR_TYPE_BWI = 0
}SENSORTYPE;

typedef enum 
{
	AE_D50 = 0,
	AE_SFR = 1,
	AE_Collimator = 2 
}AE_MODE;

/** @brief ae功能参数设置
@{

*/
typedef struct AeParam_s
{
	AE_MODE			modeOfAe;				
	SENSORTYPE		sensorType;					///< 不同的sensorType，曝光相关寄存器可能不一样
	USHORT			uTargetLumMin;				///< 目标亮度值最小值
	USHORT			uTargetLumMax;				///< 目标亮度值最大值
	USHORT			uAdjCntMax;					///< 允许最大的调节次数
	USHORT			uFps;						///< sensor帧率
	double			lfTargetBrightness_100Rank;	///< 0 < value <100 
	DtRoi_t			Roi;						///< 统计的区域设置
	UINT			Rsv[4];

	AeParam_s()
	{
		modeOfAe = AE_D50;
		sensorType = SENSOR_TYPE_BWI;
		uTargetLumMax = 0;
		uTargetLumMin = 0;
		uAdjCntMax = 3;
		uFps = 10;
		lfTargetBrightness_100Rank = 70;
		Roi.w = 0;
		Roi.h = 0;
		Roi.x = 0;
		Roi.y = 0;
		Rsv[0] = 0;
		Rsv[1] = 0;
		Rsv[2] = 0;
		Rsv[3] = 0;
	}
}AeParam_t;

/** @} */

typedef struct BlackPixelDetCfg_s
{
    /*像素阈值，低于这个像素值，判定会黑色像素*/
    int     iBlackTh;

    /*使能硬件检测，开启后找圆心的中心点坐标*/
    BOOL    bEn;

    /*检测黑点或检测白点,为true检测黑点*/
    BOOL    bDetBp;

    /*输出坐标放大倍数设置,0是2倍，1是4倍，2是8倍，3是16倍，4是32倍，5是64倍，6是128倍，7是256倍*/
    int     iMgf;

    /*预留*/
    int     Rsv[6];
}BlackPixelDetCfg_t;



typedef struct _DelayGrab
{
	int ver;				///< 结构体版本号
	UINT delayMode;			///<  0: FPGA 跳; 1: FPGA时间戳;2: 上位机跳 3: 自动跳
	bool stopAfterUpload;	///< 上传1帧之后就停止上传
	UINT skipFrameNum;
	ULONG timestamp;

}DelayGrab;

typedef struct _GrabFrameCfg
{
	int ver;				///< 结构体版本号
	void* imageBuf;			///< 存取图像数据的内存指针
	UINT bufsize;			///< imageBuf大小， 单位: Byte
	UINT imageformat;		///< 图像数据式，如果和SENSOR格式不一致，会自动转化为imageFormat格式; 参照enum IMAGE_FORMAT
	UINT timeout;			///< 等待时间
	UINT mode;				///< 0: 普通模式，1: 延迟抓模式
	DelayGrab delayCfg;		///<
	bool recompact_vc_data; ///< 0 Chinese 设置是否重新打包虚通道数据 English Set whether to repackage virtual channel data

}GrabFrameCfg;

typedef struct _GrabFrameData
{
	int ver;							///< 结构体版本号
	ULONG frameIndexPC;					///< PC接收端索引
	ULONG frameIndexFPGA;				///< FPGA端索引
	bool errorFrame;					///< 是否为错误帧
	UINT recsize;						///< 收到的数据大小，单位: Byte
	ULONG frameStartTimeStamp;			///< 帧开始时间戳，单位:us
	ULONG frameEndTimeStamp;			///< 帧结束时间戳，单位: us
	UINT data_channel_count;			///< ~Chinese 数据通道数量 ~English Number of data channels
	UINT data_channel_vcs[10];			///< Chinese 每数据通道对应的虚拟通道编号 English The virtual channel number corresponding to each data channel
	UINT data_types[10];				///<~Chinese 每个数据通道对应的数据类型 ~English The data type corresponding to each data channel
	UINT data_channel_buffer_size[10];	///< Chinese 每数据通道对应的数据量,单位: Byte English The amount of data corresponding to each data channel, unit: Byte

}GrabFrameData;

/**************************************************************************************** 
*
* 电源管理相关
*
****************************************************************************************/
/** @defgroup group5 电源管理单元相关

@{

*/

/* 帧相关信息，与帧数据对应，对其进行描述 */
/** @name SENSOR需要的电源类型定义
@{

*/
/* 定义SENSOR需要的电源类型 */
///定义SENSOR需要的电源类型。
typedef enum
{
    /* A通道，或只有一个通道时 */
    POWER_AVDD = 0,         ///<AVDD
    POWER_DOVDD = 1,        ///<DOVDD
    POWER_DVDD = 2,         ///<DVDD
    POWER_AFVCC = 3,        ///<AFVCC
    POWER_VPP = 4,          ///<VPP

    /* B通道,(B通道电源定义，只有UH920使用) */
    POWER_AVDD_B = 5,       ///<B通道AVDD
    POWER_DOVDD_B = 6,      ///<B通道DOVDD
    POWER_DVDD_B = 7,       ///<B通道DVDD
    POWER_AFVCC_B = 8,      ///<B通道AFVCC
    POWER_VPP_B = 9,        ///<B通道VPP

    /* 新增加的电源通道定义 */
    POWER_OISVDD = 10,      ///<Oisvdd电源通道，CTG970/KM27才有
    POWER_AVDD2 = 11,       ///<AVDD2电源通道
    POWER_AUX1 = 12,        ///<AUX1电源通道，支持大电流
    POWER_AUX2 = 13,        ///<备用
    POWER_VPP2 = 14,        ///<VPP2电源通道，VPP升级通道

    /* CC16机型定义的电源 */
    POWER_SENSOR0 = 40,     ///<CC16/CC16Pro sensor0通道电源
    POWER_SENSOR1 = 41,     ///<CC16/CC16Pro sensor1通道电源
    POWER_SENSOR2 = 42,     ///<CC16/CC16Pro sensor2通道电源
    POWER_SENSOR3 = 43,     ///<CC16/CC16Pro sensor3通道电源
    POWER_VDDIO = 70        ///<CC16/CC16Pro 内部bank电源通道
}SENSOR_POWER;

#define POWER_AUX   POWER_AUX1

#define POWER_POC1   POWER_AVDD2
#define POWER_POC2   POWER_AFVCC_B

/*电源通道重映射*/
#define POWER_CH1   POWER_AVDD2 
#define POWER_CH2   POWER_AVDD 
#define POWER_CH3   POWER_AFVCC 
#define POWER_CH4   POWER_DVDD 
#define POWER_CH5   POWER_DOVDD
#define POWER_CH6   POWER_VPP
#define POWER_CH7   POWER_AUX1
#define POWER_CH8   POWER_AUX2

/** @} */

/** @name SENSOR系统电源类型定义
@{

*/
/* 定义系统电源类型 */
///定义系统电源类型。
typedef enum
{
    POWER_SYS_12V = 0,  ///<12V系统电源
    POWER_SYS_5V,       ///<5V系统电源
    POWER_SYS_3_3V      ///<3.3V系统电源
}SYS_POWER;
/** @} */

/** @name SENSOR电源模式定义
@{

*/
/* 定义电源模式 */
///定义电源模式。
typedef enum
{
    POWER_MODE_WORK = 0,///<SENSOR电源为工作模式
    POWER_MODE_STANDBY, ///<SENSOR电源为待机模式
    POWER_MODE_PWDN,    ///<SENSOR电源为掉电模式
    POWER_MODE_PWDN2    ///<SENSOR电源为掉电模式
}POWER_MODE;
/** @} */

/** @name 电流测试量程定义
@{

*/
/* 定义电流测试量程 */
///定义电流测试量程。
typedef enum
{
    CURRENT_RANGE_MA = 0,   ///<电流测试量程为mA
    CURRENT_RANGE_UA,       ///<电流测试量程为uA
    CURRENT_RANGE_NA,       ///<电流测试量程为nA
    CURRENT_RANGE_NA2       ///<电流测试量程为nA
}CURRENT_RANGE;
/** @} */

/** @name 电流测试模式
@{

*/
/* 定义电流测试模式 */
///定义电流测试模式。
typedef enum
{
    MEASUREMENT_MODE_NORMAL = 0,                        ///<普通模式，返回电流平均值，该模式下，lfTriggerPoint设置的值失效
    MEASUREMENT_MODE_TRIGGER_POINT = 1,                 ///<电流采集使用触发点模式，TOF大电流采集建议使用
    MEASUREMENT_MODE_MAX = 2,                           ///<取最大电流值
    MEASUREMENT_MODE_INSTANT = 3,                       ///<瞬间电流模式
    MEASUREMENT_MODE_SINGLE_AVG = 4,                    ///<单通道采集，每个通道计算电流平均值
}CURRENT_MEASUREMENT_MODE;
/** @} */

/** @name 电流采集设置
@{
*/
typedef struct PmuCurrentMeasurement_s
{
    SENSOR_POWER             PowerType;             ///<电源类型
    CURRENT_MEASUREMENT_MODE MeasureMode;           ///<参见CURRENT_MEASUREMENT_MODE定义
    UINT                     uStatisticsNum;        ///<设置统计个数，MEASUREMENT_MODE_TRIGGER_POINT电流采集使用触发点模式时候有效
    double                   lfTriggerPoint;        ///<电流采集触发值	，单位，mA。精度，1mA。只有MU950(TOF)机型支持触发采样点设置，只支持PosEN=TRUE
    BOOL                     bPosEn;                ///<标识触发值正向或反向有效，为True，则大于TriggerPoint触发采集电流，为False，则小于TriggerPoint触发采集电流
	UINT                     uGain;					///<电源增益配置:bit[0~2]表示增益(0~7 = 1~128倍增益)，bit[4]表示固定增益(0)和自动增益(1)
    UINT                     Rsv[31];               ///<备用
}PmuCurrentMeasurement_t;
/** @} */

/** @name 返回电流值
@{
*/
typedef struct PmuRtnCurrent_s
{
    SENSOR_POWER            PowerType;              ///<指定电源通道
    UINT                    uDelay;                 ///<写完所有寄存器之后延时获取电流，单位us
    UINT                    Rsv[7];                 ///<备用
    UINT                    uNum;                   ///<指定要获取的电流个数，跟buffer大小对应
    UINT                    *pRetNum;               ///<返回实际电流个数
    double                  *pCurrent;              ///<返回的电流值
}PmuRtnCurrent_t;
/** @} */

/** @name pattern io配置
@{
*/
typedef struct PatternIoCfg_s
{
    /*写完寄存器之后延时时间，单位us*/
    UINT                 uDelay;

    /*io检测间隔时间，单位us*/
    UINT                 uInterval;

    /*检测次数*/
    UINT                 uTime;

    /*预期io电平设置,目前支持bit0-bit4(po1-po5)，对应bit为1则期望值是高电平，底层就会以识别高电平为正确，低电平为错误*/
    UINT                 uExpectLevel;

    /*实际返回结果，目前支持bit0-bit4(po1-po5)，对应bit为1则结果ok，如果不正确对应bit返回0*/
    UINT                 uResult;

    /*预留*/
    int                  Rsv[4];
}PatternIoCfg_t;
/** @} */

/** @name 滤波电容设置
@{

*/
/* 定义滤波电容模式 */
typedef enum 
{
    FILTER_CAP_AUTO_SET_MODE = 0,                   ///<自动配置模式
    FILTER_CAP_MANUAL_SET_MODE = 1,                 ///<该模式用户可手动设置开启/关闭滤波电容
}FILTER_CAP_MODE;
/** @} */

/** @name 滤波电容配置
@{
*/
typedef struct PmuFilterCap_s
{
    SENSOR_POWER             PowerType;             ///<电源类型
    FILTER_CAP_MODE          Mode;                  ///<模式设置
    BOOL                     bEn;                   ///<手动设置模式才有用，true打开，false关闭
    UINT                     Rsv[16];               ///<备用
}PmuFilterCap_t;
/**@} */

/** @name 格科治具测试模式
@{
*/
/* 定义模式 */
typedef enum 
{
	TOOL_MODE_OFF_LINE = 0,						///<线下模式
	TOOL_MODE_ON_LINE  = 1,						///<线上模式
}TOOL_MODE;
/** @} */

/** @name 线上模式对应AUX类型,线下模式配置无效
@{
*/
/* 定义AUX类型*/
typedef enum 
{
	AUX_TYPE_OUT		= 0,					///<做DPS输出使用
	AUX_TYPE_IN			= 1,					///<做DPS输入使用
	AUX_TYPE_DVDD_FB	= 2,					///<做DVDD反馈线使用
}AUX_TYPE;
/** @} */

/** @name 格科治具测试配置数据结构
@{
*/
typedef struct PmuTestToolCfg_s
{
	TOOL_MODE		ToolMode;					///<治具模式
	AUX_TYPE		AuxType;					///<AUX配置类型
	UINT			Rsv[7];						///<备用
}PmuTestToolCfg_t;
/**@} */
/** @name 电源报警计数获取
@{
*/
typedef struct PmuAlarmCnt_s
{
    USHORT  uHwIoOcpCnt;            ///< 硬件IO触发电流报警，区分sensor port，不区分电源通道，所有电源通道共用
    USHORT  Rsv1[15];
    USHORT  uHwIoOvpCnt;            ///< 硬件IO触发的过压报警，区分sensor port，不区分电源通道，所有电源通道共用
    USHORT  uHwIoLdoLimtCnt;
    USHORT  Rsv2[14];
    USHORT  RangeOcpCnt[8];         ///< 超量程报警，区分sensor port，区分电源通道，依次是AFVCC,.DVDD,DOVDD,VPP,AVDD,AVDD2,AUX,RSV(备用通道)
    USHORT  Rsv3[8];
    USHORT  HwChOcpCnt[8];          ///< 硬件过流报警，区分sensor port，区分电源通道，依次是AFVCC,.DVDD,DOVDD,VPP,AVDD,AVDD2,AUX,RSV(备用通道)
    USHORT  Rsv4[8];
    USHORT  SwChOcpCnt[8];          ///< 软件过流报警，区分sensor port，区分电源通道，依次是AFVCC,.DVDD,DOVDD,VPP,AVDD,AVDD2,AUX,RSV(备用通道)
    USHORT  Rsv5[8];
}PmuAlarmCnt_t;
/**@} */

/** @name ADC选择，CP20Pro/F22S/F22LC有多个ADC
@{
*/
typedef enum
{
    DT_ADC0 = 0,        // 每个sensor通道支持6路ADC，外壳丝印是ADC_1,ADC_2...，vref电压采集
    DT_ADC1 = 1,        // 每个sensor通道支持8路ADC，对应mclk/pwdn/rst/po1-po5
    DT_ADC2 = 2,        // 每个sensor通道支持10路ADC，对应mipi lp管脚

    DT_HSADC0 = 10,			// 每个sensor通道支持2路高速ADC,对应scl/sda

	DT_PMU_ADC1 = 21		// 每个sensor通道支持PMU电压采集

}ADC_SEL;
/** @} */

/** @name ADC0采集通道设置,CP20/CP20Pro第1个普通ADC，对应硬件外壳丝印ADC1-ADC6
@{
*/
typedef enum
{
    ADC_1 = 0,
    ADC_2 = 1,
    ADC_3 = 2,
    ADC_4 = 3,
    ADC_5 = 4,
    ADC_6 = 5
}ADC_CHANNEL;

/** @} */

/** @name ADC1通道设置,F22S/CP20Pro第2个普通ADC，对应硬件外壳丝印管脚po1-po5/pwdn/reset/mclk//scl/sda
GQ4S上ADC1_9和ADC1_10对应的SCL/SDA电平
@{
*/
typedef enum
{
    ADC1_1 = 0, ///<po1
    ADC1_2 = 1, ///<po2
    ADC1_3 = 2, ///<po3
    ADC1_4 = 3, ///<po4
    ADC1_5 = 4, ///<po5
    ADC1_6 = 5, ///<rst
    ADC1_7 = 6, ///<pwdn
    ADC1_8 = 7, ///<mclk
    ADC1_9 = 8, ///<scl
    ADC1_10 = 9 ///<sda
}ADC1_CHANNEL;

/** @} */

/** @name ADC3通道设置,F22S/CP20Pro第2个普通ADC，内部AD，测量mipi lp管脚电压
@{
*/
typedef enum
{
    ADC2_1 = 0, ///<d1p(d2a)
    ADC2_2 = 1, ///<d1n(d2b)
    ADC2_3 = 2, ///<d3p(d2c)
    ADC2_4 = 3, ///<d3n(nc)
    ADC2_5 = 4, ///<clkp(d1b)
    ADC2_6 = 5, ///<clkn(d1c)
    ADC2_7 = 6, ///<d2p(d0a)
    ADC2_8 = 7, ///<d2n(d0b)
    ADC2_9 = 8, ///<d0p(d0c)
    ADC2_10 = 9 ///<d0n(d1a)
}ADC2_CHANNEL;

/** @} */


/** @name HSADC通道设置，F22S/CP20Pro的高速AD，scl，sda
@{
*/
typedef enum
{
    HSADC0_1 = 0,
    HSADC0_2 = 1,
}HSADC0_CHANNEL;
/** @} */

/** @name HSADC模式设置
@{
*/
typedef enum 
{
    ADC_TRIGGER = 0,      // 命令触发采集转换
    ADC_NORMAL = 1        // 普通模式，AD打开后一直采集
}ADC_CP_MODE;
/** @} */

/** @name 配置待获取的ADC通道
@{
*/
typedef struct DtAdcWaveDateCfg_s
{
    ADC_SEL    AdcSel;              // 选择ADC0/ADC1/ADC2/HSADC
    UINT       uChannelNum;         // 选择的ADC的通道号
    UINT       uBufSize;            // 指定要读取的通道数据量大小
    int        Rsv[32];             // 备用
}DtAdcWaveDateCfg_t;

/** @} */

/** @name adc采集准备
*/
typedef struct DtAdcPrepare_s
{
    ADC_SEL    AdcSel;              // 选择ADC0/ADC1/ADC2/HSADC
    UINT       uChannelNum;         // 选择的ADC的通道号
    int        Rsv[64];             // 预留
}DtAdcPrepare_t;
/** @} */

/** @name 返回待获取的ADC通道结果
@{
*/
typedef struct DtAdcRetWaveDateInfo_s
{
    int        iMax;              ///< 最大值
    int        iMin;              ///< 最小值
    int        uRetSize;          ///< 返回的电压数据个数
    int        Rsv[32];
    int        *pBuf;             ///< 返回的数据
}DtAdcRetWaveDateInfo_t;

/** @} */

/** @name 返回待获取的ADC通道结果
@{
*/
typedef struct DtRwSensorI2cParam_s
{
    UCHAR   uDevAddr;              ///< 设备地址
    UINT    uRegAddr;              ///< 寄存器地址
    UCHAR   uRegAddrSize;          ///< 寄存器地址大小
    BYTE    *pData;                ///< 写入/读出数据
    USHORT  uSize;                 ///< 数据量大小
    BOOL    bNoStop;               ///< 读过程是否发送stop
    BOOL    bRw;                   ///< bRw为true表示读，bRw为false表示写
    BYTE    Rsv[32];               ///< 备用
}DtRwSensorI2cParam_t;
/** @} */

/** @name ADC配置结构体
@{
*/
typedef struct DtAdcCfg_s
{   
    // 如果选择通道0,1，则dwAdcChNumSel = 1<<1|1<<0;
    // 如果对应ADC的通道全选择打开，则dwAdcChNumSel = 0xffffffff
	// ADC0:bit0-bit5:vref1-vref6
	// ADC1:bit0-bit7:po1/po2/po3/po4/po5/rst/pwdn/mclk/scl/sda
	// ADC2:bit0-bit9:d1p(d2a)/d1n(d2b)/d3p(d2c)/d3n(nc)/clkp(d1b)/clkn(d1c)/d2p(d0a)/d2n(d0b)/d0p(d0c)/d0n(d1a)
    DWORD                 dwAdcChNumSel;              ///<选择指定ADC的通道号，高速adc需要指定通道打开/关闭，其他普通ADC（LP检测，GPIO通道）指定暂不生效，所有通道是一起打开
	UINT                  uCtrl;						  ///<bit位表示0表示正常采集，1表示采集并校准。					  
	int                   Rsv1[2];
    double                lfSpeedKhz;                 ///<ADC采样速度配置，1Mhz = 1000Khz
    BOOL                  bEn;                        ///<使能采集
    ADC_CP_MODE           Mode;                       ///<模式设置，高速
    int                   Rsv2[64];                   ///<备用
}DtAdcCfg_t;
/**@} */

/** @name 状态设置，如果要测试LP电压高电平，
要设置寄存器让模组处于LP状态
@{
*/
typedef struct ezPreWriteReg_s
{
    UCHAR                 uDevAddr;
    USHORT                uRegAddr;
    UCHAR                 uRegAddrSize;
    BYTE                  *pData;
    USHORT                uSize;
    BYTE                  Rsv[32];
}ezPreWriteReg_t;

/**@} */

/**@} */

/** @} */ // end of group5

/**************************************************************************************** 
*
* 按键功能定义(dtTest.exe使用的定义)
*
****************************************************************************************/
/** @defgroup group6 初始化控制相关
@{

*/
/** @name 按键功能定义
@{

*/ 
#define KEY_ROI_B0			0x100
#define KEY_ROI_B1			0x80
#define KEY_ROI_B2			0x40
#define KEY_ROI_B3			0x10
#define KEY_ROI_B4			0x20
#define KEY_ROI_GRID		0x04
#define KEY_FULL			0x08
#define KEY_PLAY			0x01
#define KEY_CAM				0x02
#define KEY_NOTHING			0x00 
/** @} */
/** @} */ // end of group6

/**************************************************************************************** 
*
* 本系统支持的AF器件型号定义
*
****************************************************************************************/
/** @defgroup group7 AF相关
@{

*/
/** @name 支持的AF器件型号定义
@{

*/
#define AF_DRV_AD5820		0
#define AF_DRV_DW9710		0
#define AF_DRV_DW9714		0

#define AF_DRV_AD5823		1
#define AF_DRV_FP5512		2
#define AF_DRV_DW9718		3
#define AF_DRV_BU64241		4
#define AF_DRV_LV8498		5
#define AF_DRV_BU64291		6
#define AF_DRV_AD1457		7

#define AF_DRV_DW9761		8
#define AF_DRV_AD5816		8

#define AF_DRV_AK7345		9
#define AF_DRV_DW9800		10

#define AF_DRV_ZC533		11
#define AF_DRV_BU64295		12
#define AF_DRV_DW9719		13
//#define AF_DRV_SC9714		14
#define AF_DRV_FP5518		14

#define AF_DRV_AK7374		15

#define AF_DRV_LC898219		16

#define AF_DRV_MAX			30

/** @} */
/** @} */ // end of group7

/**************************************************************************************** 
*
* OS/LC相关
*
****************************************************************************************/
/** @defgroup group8 LC/OS相关
@{

*/
/** @name OS/LC测试配置定义
@{

*/
/* OS/LC测试配置定义，在LC_OS_CommandConfig函数中使用 */

#define OS_CFG_TEST_ENA                     (1<<7) ///<OS测试使能

#define LC_CFG_TEST_ENA                     (1<<6) ///<LC测试使能

#define OS_CFG_CHANNEL_A                    (1<<5) ///<使能测试A通道OS测试(DTLC2机型支持，其他机型不生效)

#define OS_CFG_CHANNEL_B                    (1<<4) ///<使能测试B通道OS测试(DTLC2机型支持，其他机型不生效)

#define OS_CFG_HIGH                         (1<<3) ///<使能正端OS测试

#define OS_CFG_LOW                          (1<<2) ///<使能负端OS测试

#define OS_CFG_DOUBLE                       (1<<8) ///<G系列光纤产品（G42/G22）配置支持A,B或者C，D一起进行OS测试模式，双摄模组共地的时候使用，
                                                   ///<并且注意:该模式下需要做同步，保证2个模组同时进入OS测试或退出OS测试

#define OS_CFG_QUICK                        (1<<15) ///<暂不支持

/*
LC测试位定义
*/
#define LC_CFG_MIPI_PIN_TEST_FIRST          (1<<9) ///<mipi管脚先测试

#define LC_CFG_HIGH                         (1<<1) ///<使能正端LC测试

#define LC_CFG_LOW                          (1<<0) ///<使能负端LC测试
/** @} */

/** @name OS/LC测试结果定义，OS_Read函数返回的结果
@{

*/
/* OS/LC测试结果定义，OS_Read函数返回的结果 */
#define OS_TEST_RESULT_PASS                 0       ///< 通过测试

#define OS_TEST_RESULT_NG                   1       ///< 未通过测试

#define OS_TEST_RESULT_PIN_CONNECTED        0x80    ///< pin 之间存在短路关系，如cp20的AVDD与AVDD_FB连通的

#define OS_TEST_RESULT_FAILED               0xfe    ///< 测试失败

#define OS_TEST_RESULT_INVALID              0xff    ///< 测试无效
/** @} */

/*测试pin状态*/
typedef enum 
{
    PIN_OPEN = 0,
    PIN_GND = 1
}OsTestPinState_e;

/** @name OS测试配置参数，主要配置电容
@{

*/
typedef struct OsTetCfgEx_s
{
    /*预留*/
    int                 Rsv1;
    
    /*电容配置,延时*/
    UINT                uCap;               // 单位nf，如0.1uf则uCap=100

    /*管脚测试电压配置,测试电压，单位uV*/
    int                 iVoltage;

    /*管脚电流配置，单位uA*/
    int                 iCurrent;

    /*PIN 状态设置*/
    OsTestPinState_e    PinState;

    /*预留*/
    int                 Rsv[5];
}OsTetCfgEx_t;
/** @} */

/** @name DtOsTest函数使用的结构体
@{
*/
typedef struct  DtOsTestCommand_s
{
	/* 启动测试位使能 */
	DWORD dwSensorEn;			//模组共地情况下有效，模组未共地，直接设置为0
								//bit0-bit3对应同一个盒子的每个通道,如共地情况下，4通道盒子一起测试，dwSensorEn = 0x0000000f
								//如模组共地情况下，点测1个通道，则对应bit置为1，如：只测试0通道dwSensorEn = 0x00000001
	/*由OS_CFG_XXX配置，同LC_OS_CommandConfig Command*/
	DWORD dwCommand;

	/*OSTestVol：测试电压，单位mV*/
	int iOSTestVol;

	/*PowerCurrent:电源电流，单位uA*/
	int iPowerCurrent;

	/* GpioCurren:GPIO电流，单位uA*/
	int iGpioCurrent;

	/* io 使能测试，同LC_OS_CommandConfig的IoMask*/
	DWORD dwPinEnL;			// 低32位

	/* io 使能测试，同LC_OS_CommandConfig的IoMask*/
	DWORD dwPinEnH;			// 高32位
	
	/* 预留 */
	int Rsv[8];
}DtOsTestCommand_t;

/** @} */

/* lc测试量程设置 */
enum LcTestCurrentRange_e
{
    LC_TEST_MA = 0,
    LC_TEST_UA = 1,
    LC_TEST_NA = 2
};

//    D2p  =0,//Mipi口
//    D2n,
//    D0p,
//    D0n,
//    Clkp,
//    Clkn,
//    D1p,
//    D1n,
//    D3p,
//    D3n,
//    /*GPIO*/
//    Pwdn=10,//Gpio口
//    Rst,
//    Scl,
//    Sda,
//    Mclk,
//    Po5,
//    Po4,
//    Po3,
//    Po2,
//    Po1,

/* LC配置结构体 */
typedef struct LcConfig_s
{
    DWORD                   dwLcCtrl;           /// < LC测试控制，位定义，bit[0]负端LC测试，bit[1]正端LC测试，bit[6]LC测试使能
    LcTestCurrentRange_e    LcRange;            /// < LC测试量程选择，mA/uA/nA,CP81是MIPI的量程
    int                     iMipiVoltage;       /// < mipi测试源电压配置，单位uV
    int                     iIoVoltage;         /// < 普通IO测试源电压配置，单位uV
    DWORD                   dwPinMask1;         /// < bit0-bit31有效，bit[x]=1：表示对应管脚使能测试,目前F22LC支持20个pin,兼容os测试pinmask顺序
                                                /// < bit0-bit8是电源和地，暂时F22LC不支持，bit9-bit18是mipi管脚有效，bit[19]=po5
    DWORD                   dwPinMask2;         /// < bit4-bit12有效，bit[4]=MCLK,bit[5]=PWDN,bit[6]=RESET;bit[7]=SCL；bit[8]=SDA;bit[9]=PO2；
                                                /// < bit[10]=PO1;bit[11]=PO3;bit[12]=PO4.
                                                
    int                     iTestMode;          /// < 1是测试模式，2是校准模式。用户只需要用测试模式即可
    int                     iSampleNum;         /// < 采样次数
    int                     iSampleGain;        /// < 增益配置
    LcTestCurrentRange_e    GpioRange;          /// < Gpio可以单独配置量程
    int                     Rsv[31];            /// < 预留
}LcConfig_t;

enum ResisTestRange
{
    RESIS_TEST_1_3R = 0,        //1.3欧以下,100mA
    RESIS_TEST_13R = 1,         //13欧      10mA
    RESIS_TEST_AUTO = 0xff,     //自动档位
};

#define RESIS_TEST_CTRL_EN              1<<0            ///< 电阻测试使能

#define RESIS_TEST_CTRL_CALIBRATION     1<<1            ///< 校准模式使能

#define RESIS_TEST_CTRL_THREE_WIRE      1<<2            ///< 三线电阻测试模式

#define RESIS_TEST_CTRL_FOUR_WIRE       1<<3            ///< 四线电阻测试模式

#define RESIS_TEST_CTRL_PULSE			1<<8            ///< 脉冲测量模式

/*电阻测试*/
typedef struct ResisTestConfig_s
{
    DWORD                   dwCtrl;             ///< 电阻测试控制
    ResisTestRange          Range;              ///< 默认处于自动档位，一般不需要设置
    int                     iCurrent;           ///< 测试电流,单位uA
    int                     iVoltage;           ///< 测试电压设置，单位uV
    int                     iSampleNum;         ///< 采样次数
    int                     iSampleGain;        ///< 增益配置
    DWORD                   dwPinMask;          ///< 使能pin测试，bit0是RES1，bit1是RES2
    int                     iDelay;             ///< 校准的时候有用，delay时间设置，单位ms
	int                     iPulseTime;         ///< 脉冲测量时间，单位ms
    int                     Rsv[6];
}ResisTestConfig_t;

/** @} */ // end of group8


/************************************************************************
*
*外部扩展IO定义
*
/************************************************************************/
/** @defgroup group9 扩展IO
@{

*/
/** @name 外部扩展IO管脚定义
@{

*/
typedef enum
{
	GPIO0=0,	///<GPIO0
	GPIO1,		///<GPIO1
	GPIO2,		///<GPIO2
	GPIO3,		///<GPIO3
	GPIO4,		///<GPIO4
	GPIO5,		///<GPIO5
	GPIO6,		///<GPIO6
	GPIO7,		///<GPIO7
	GPIO8,		///<GPIO8
	GPIO9,		///<GPIO9
	GPIO10,		///<GPIO10
	GPIO11,		///<GPIO11
}EXT_GPIO;
/** @} */

/** @name 外部扩展IO模式定义
@{

*/
typedef enum
{
    GPIO_INPUT = 0,	///<输入模式
    GPIO_OUTPUT,	///<恒电平输出模式
    GPIO_OUTPUT_PP, ///<高低电平交互输出

}EXT_GPIO_MODE;
/** @} */

/** @name 外部扩展I2C总线定义
@{

*/
typedef enum
{
    EXT_I2C_BUS1 = 0,	///<扩展I2C总线1
    EXT_I2C_BUS2,		///<扩展I2C总线2
    EXT_I2C_BUS_NUM,	///<扩展I2C总线数量

}EXT_I2C_BUS_TYPE;
/** @} */

/** @} */ // end of group9

/* 调试报告信息，功能模块 */
enum DtDbgRePortPart_e
{
    /* 默认或未作功能分类 */
    PART_DEFAULT = 0x0,

    /* 控制相关 */
    PART_CONTROL = 0x10,

    /* 数据流或采集相关 */
    PART_STREAM = 0x20,

    /* mipi */
    PART_MIPI = 0x30,

    /* GPIO功能相关 */
    PART_GPIO = 0x40,

    /* 图像效果相关 */
    PART_IMAGE = 0x50,

    /* power相关 */
    PART_POWER = 0x60,

    /* open/short test相关 */
    PART_OS_TEST = 0x70,

    /* 主控制器调试信息 */
    PART_MAIN_CONTROL = 0x80
};

/* 调试报告信息，级别 */
enum DtDbgRePoportLevel_e
{
    /* 信息或提示 */
    LEVEL_INFO = 0x10,

    /* 问题或故障 */
    LEVEL_PROBLEM = 0x40
};

/* 调试报告 */
typedef struct DtDbgReport_s
{
    /* 调试报告的功能分类 */
    DtDbgRePortPart_e       Part;

    /* 调试报告的级别 */
    DtDbgRePoportLevel_e    Level;

    /* 是否强制记录到文件，不管是否开启log文件记录 */
    bool                    bForce;

    /* 保留64字节 */
    UINT                    resv[16];

    /* 调试报告的相关文本内容，将插入到log文件当中 */
    char                    text[128];
}DtDbgReport_t;

// 定义的支持测试电阻的管脚
enum ResisCalculateForIoSet
{
    PO1 = 0,            //外壳PO1
    PO2 = 1,            //外壳PO2
    PO3 = 2,            //外壳PO3
    PO4 = 3,             //外壳PO4

    RESIS_PARASITIC_1 = 10  //三线模式下的寄生电阻
};
#define RESIS_1     PO1         // 
#define RESIS_2     PO2         //

//老的测试方式，基本没人用了
#define     RESIS_TEST_COMMON_GROUND        1<<0

/* 触发 光源触发与芯片触发*/
typedef struct TrigPwm_s
{
	float fPwmCyc;    //周期
	float fPwmPul;    //占空比
	float fTrigHigh;  //触发高电平时间
	float fTrigDealy; //触发延时

	/*
	* trig与pwm关联配置 
	* bit0【1=pmw信号使能，0=pmw不信号使能】; 
	* bit1【1=trig与pwm关联，trig输出后延时输出pwm，0=trig与pwm独立关系】; 
	* bit2【1=使能输出一次trig信号】;
	*/
	int iTpCtrl;	   //trig与pwm关联配置 

	int iPwmSyncCtrl;  //PWM同步触发配置(仅车载DF108使用) 

}TrigPwm_t;


/************************************************************************
*
* 串口定义
*
/************************************************************************/
/** @defgroup group10 扩展IO
@{

*/

// 定义波特率
enum 
{
    BAUD_RATE_9600 = 0,
    BAUD_RATE_19200,
    BAUD_RATE_38400,
	BAUD_RATE_43000,
    BAUD_RATE_57600,
	BAUD_RATE_76800,
	BAUD_RATE_115200,
	BAUD_RATE_128000,
	BAUD_RATE_230400,
	BAUD_RATE_256000,
	BAUD_RATE_460800,
	BAUD_RATE_921600,
	BAUD_RATE_1000000,
	BAUD_RATE_2000000,
	BAUD_RATE_3000000,
};

// 定义校验位类型
enum 
{
    NO_PARITY   = 0, // 无校验位
    ODD_PARITY  = 1, // 奇校验
    EVEN_PARITY = 2, // 偶校验
};

// 定义停止位长度
enum 
{
    ONE_STOP_BIT   = 0, // 1个停止位
	ONE5_STOP_BITS = 1, // 1.5个停止位
    TWO_STOP_BITS  = 2, // 2个停止位
};

// 定义数据位长度
enum 
{
    DATA_BITS_5 = 5, // 5个数据位
    DATA_BITS_6 = 6, // 6个数据位
    DATA_BITS_7 = 7, // 7个数据位
    DATA_BITS_8 = 8, // 8个数据位
};

// 串口配置数据结构
typedef struct UartConfig_s
{
    UINT		BaudRate;		// 波特率 (e.g., 9600, 115200)

    BYTE		DataBits;		// 数据位长度

    BYTE		Parity;			// 校验位类型

    BYTE		StopBits;		// 停止位长度

	BYTE		OpenClose;		// 串口开启和关闭操作 1表示串口打开，0表示串口关闭

	UINT		Rsv2[6];		// 预留

}UartConfig_t;

/**************************************************************************************** 
*
* 一些SDK接口函数中使用到的宏定义(dtccm使用的定义)
*
****************************************************************************************/
//PMU range....
#define PMU1_1						0x11
#define PMU1_2						0x10
#define PMU1_3						0x12
#define PMU2_1						0x21
#define PMU2_2						0x20
#define PMU2_3						0x22
#define PMU3_1						0x31
#define PMU3_2						0x30
#define PMU3_3						0x32
#define PMU4_1						0x41
#define PMU4_2						0x40
#define PMU4_3						0X42
#define PMU5_1						0x51
#define PMU5_2						0x50
#define PMU5_3						0X52

#define I2C_400K					1
#define I2C_100K					0

#define I_MAX_100mA					1
#define I_MAX_300mA					0

#define PMU_ON						0
#define PMU_OFF						1

#define POWER_ON					1
#define POWER_OFF					0

#define CLK_ON						1
#define CLK_OFF						0

#define IO_PULLUP					1
#define IO_NOPULL					0

#define MULTICAM_NORMAL				0x00
#define MULTICAM_PWDN_NOT			0x01
#define MULTICAM_RESET_PWDN_OVERLAP	0x02
#define MULTICAM_SPECIAL			0x03


/*
easy 电源接口相关定义
*/

typedef enum
{
    EZ_POWER_CH_ALL = 0,
    EZ_POWER_CH_DOVDD = 0,
    EZ_POWER_CH_DVDD = 1,
    EZ_POWER_CH_AVDD = 2,
    EZ_POWER_CH_VPP = 3,
    EZ_POWER_CH_AFVCC = 4,
    EZ_POWER_CH_OISVDD = 5,
    EZ_POWER_CH_AVDD2 = 6,
    EZ_POWER_CH_AUX1 = 7,
    EZ_POWER_CH_VPP2 = 8,
}esPowerCh;

#define     POWER_CONFIG_ONOFF                  1 << 0      // 开关设置
#define     POWER_CONFIG_VOLTAGE                1 << 1      // 电压值设置
#define     POWER_CONFIG_CURRENTLIMIT           1 << 2      // 限流值设置
#define     POWER_CONFIG_SLOPE                  1 << 3      // 斜率设置
#define     POWER_CONFIG_SPEED                  1 << 4      // 采样速度设置
#define     POWER_CONFIG_RANGE                  1 << 5      // 量程设置

typedef struct _ezPowerSequence
{
	UINT			uSequence;			// 电压时序,电压上电或下电优先级设置，如0为优先级最高，1次之。如果优先级一致，则同时上电或下电
	double			lfDelay;			// 延时设置,ms
	UINT			rsv[16];
}ezPowerSequence;

// 电源通道，每个电源通道包含开关、限流、电压、上升斜率等设置
typedef struct _ezPowerChannel
{
	BOOL            bOnOff;             // 开关
	double          lfVolt;             // 电压,mV
	double          lfCurrentLimit;     // 限流值,uA
	double          lfSlope;			// 斜率,mV/ms
	CURRENT_RANGE	range;				// 电流量程,nA,uA,mA
	UINT			uSpeed;				// 电流采样速度,ms
	ezPowerSequence sequence;			// 时序控制
	UINT			uConfigCode;		// 功能配置码,通过位定义的方式指示哪些参数需要配置到设备
	UINT			uStateCode;			// 状态码
    BOOL            bCapEn;
	UINT			rsv[32];
}ezPowerChannel;

typedef struct _ezPowerConfig
{
	ezPowerChannel  dovdd;
	ezPowerChannel  dvdd;
	ezPowerChannel  avdd;
	ezPowerChannel  vpp;
	ezPowerChannel  afvcc;
	ezPowerChannel  oisvdd;
	ezPowerChannel	avdd2;
	ezPowerChannel  aux1;
	ezPowerChannel  aux2;
	ezPowerChannel	vpp2;
	ezPowerChannel  rsv[16];
}ezPowerConfig;

/** @} */

#define     SENSOR_I2C_RD_NO_STOP       1 << 0          // I2C读阶段不发送stop指令

/** @name SensorI2cRw结构体
@{
*/
typedef struct ezSensorI2cRw_s
{
    UINT        uCtrl;          ///< 控制码,SENSOR_I2C_xxx
    BYTE        bySlaveAddr;    ///< 从器件地址
    BYTE        *pWrData;       ///< 写入数据块
    UINT        uWrSize;        ///< 写入数据块的字节数
    BYTE        *pRdData;       ///< 读回数据块
    UINT        uRdSize;        ///< 读出数据块字节数
    UINT        rsv[16];        ///< 保留
}ezSensorI2cRw_t;
/** @} */

// 定义功能控制码

typedef enum
{
    EZ_POWER_ON = 0x10,         // 电源上电控制:上电时序，电压设置，斜率设置，量程等一次配置成功
    EZ_POWER_UP = 0x11,         // 电源下电
    EZ_POWER_GET = 0x12,        // 获取电源各参数状态
}ezCtrl;

/* @name 返回待测IO的高电压和低电压值
@{
*/
typedef struct ezIoVoltageResult_s
{
    BOOL            bEn;            ///< 使能测试
    int             iHighVolt;      ///< 返回高电压值，单位mV
    int             iLowVolt;       ///< 返回低电压值，单位mV
    int             Rsv[16];
}ezIoVoltageResult_t;
/**@} */

//pwdn/reset/mclk/po1-po5/scl/sda
typedef struct ezIoVoltageChannel_s
{
    ezIoVoltageResult_t     Po1Result;
    ezIoVoltageResult_t     Po2Result;
    ezIoVoltageResult_t     Po3Result;
    ezIoVoltageResult_t     Po4Result;
    ezIoVoltageResult_t     Po5Result;
    ezIoVoltageResult_t     PwdnResult;
    ezIoVoltageResult_t     ResetResult;
    ezIoVoltageResult_t     MclkResult;
    ezIoVoltageResult_t     SclResult;
    ezIoVoltageResult_t     SdaResult;
    ezIoVoltageResult_t     Rsv[16];
}ezIoVoltageChannel_t;

/******/
/* 指纹模组配置 */
typedef struct FpmConfig_s
{
    BYTE    byMode;
    UINT    uIrqCnt;            ///< 设置几行为一个中断行,如宽度为320，uIrqCnt=4，则一次读取320*4个数据更新中断号
    BYTE    rsv[64];            ///< 保留
}FpmConfig_t;

/* @name 电源通道，快速配置通道开关
@{
*/

typedef struct ezPowerSequence_s
{
	SENSOR_POWER    Power;
	UINT			Volt;
	UINT            bOnOff;             // 开关
	UINT			uSequence;			// 电压时序,电压上电或下电优先级设置，如0为优先级最高，1次之。如果优先级一致，则同时上电或下电
	double			lfDelay;			// 延时设置,ms
	UINT			rsv[4];
}ezPowerSequence_t;

typedef struct ezPmuOnOffConfig_s
{
	ezPowerSequence_t power1;
	ezPowerSequence_t power2;
	ezPowerSequence_t power3;
	ezPowerSequence_t power4;
	ezPowerSequence_t power5;
	ezPowerSequence_t power6;
	ezPowerSequence_t power7;
	ezPowerSequence_t power8;
	int				iCount;				// 配置通道数
	ezPowerSequence_t rsv[4];
}ezPowerOnOffConfig_t;




/* @name pn9配置参数
@{
*/


typedef struct pn9ParamConfig_s
{
	IMAGE_FORMAT    format;      ///< 图像格式
    UINT            width;       ///< 图像尺寸
    UINT            height;	     ///< 图像尺寸
    unsigned long   pInDataSize; //文件大小
    BYTE            *pInData;     //写文件数据
	UINT		    rsv[4];
}pn9ParamConfig_t;

/*****************************************************************
sensortab v2版本结构体
*****************************************************************/

/*sensor特性*/
typedef struct SensorProp_s
{
	/// @brief SENSOR宽度
	USHORT uWidth;                    ///<SENSOR宽度
	/// @brief SENSOR高度
	USHORT uHeight;                   ///<SENSOR高度
	/// @brief SENSOR数据类型
	IMAGE_FORMAT type;                  ///<SENSOR数据类型
	/// @brief SENSOR的RESET和PWDN引脚设置
	BYTE uPin;                       ///<SENSOR的RESET和PWDN引脚设置
	/// @brief phytype
	BYTE uPhyType;					///<Phy选择，dp/aci
	/// @brief lane num设置
	BYTE uLaneNum;					///<Lane个数设置
	/// @brief dp的不同的pin定义选择
	BYTE uPinType;					///<不同的pin定义选择
	/// @brief dp速度登记设置
	DpRecRateSel DpRate;				///<Dp速率等级
	/// @brief alp的使能
	bool bAlpEn;						///<Alp模式使能
	/// @brief SENSOR的器件地址
	BYTE uSlaveID;                   ///<SENSOR的器件地址
	/// @brief SENSOR的I2C模式
	BYTE uMode;                      ///<SENSOR的I2C模式
	/// @brief SENSOR的数据接口类型
	BYTE uPort;                      ///<SENSOR的数据接口类型
	/// @brief SENSOR标志寄存器1.
	UINT uFlagReg;                  ///<SENSOR标志寄存器1.
	/// @brief SENSOR标志寄存器1的值
	UINT uFlagData;                 ///<SENSOR标志寄存器1的值
	/// @brief SENSOR标志寄存器1的掩码值
	UINT uFlagMask;                 ///<SENSOR标志寄存器1的掩码值
	/// @brief SENSOR标志寄存器2.
	UINT uFlagReg1;                 ///<SENSOR标志寄存器2.
	/// @brief SENSOR标志寄存器2的值
	UINT uFlagData1;                ///<SENSOR标志寄存器2的值
	/// @brief SENSOR标志寄存器2的掩码值
	UINT uFlagMask1;                ///<SENSOR标志寄存器2的掩码值
	/// @brief SENSOR的名称
	char name[64];                      ///<SENSOR的名称

	/// @brief SENSOR输出数据格式
	OUTFORMAT_RGB uOutformat;
	/// @breif SENSOR的输入时钟，单位100Khz，如设置24Mhz，则nMclk=240
	int nMclk;                          ///<SENSOR的输入时钟MCLK,单位100Khz
	/// @brief SENSOR的Power1电压值
	int nPower1;                        ///<SENSOR的Power1电压值
	/// @brief SENSOR的Power2电压值
	int nPower2;                        ///<SENSOR的Power2电压值
	/// @brief SENSOR的Power3电压值		
	int nPower3;                        ///<SENSOR的Power3电压值
	/// @brief SENSOR的Power4电压值
	int nPower4;                        ///<SENSOR的Power4电压值
	/// @brief SENSOR的Power5电压值
	int nPower5;                        ///<SENSOR的Power5电压值
	/// @brief SENSOR的Power6电压值		
	int nPower6;                        ///<SENSOR的Power6电压值
	/// @brief SENSOR的Power7电压值		
	int nPower7;                        ///<SENSOR的Power7电压值
	/// @brief SENSOR的Power8电压值
	int nPower8;                        ///<SENSOR的Power8电压值
	/// @brief SENSOR的Power9电压值		
	int nPower9;                        ///<SENSOR的Power9电压值
	/// @brief SENSOR的Power10电压值		
	int nPower10;                       ///<SENSOR的Power10电压值
	/// @brief 保留字段
	UINT Rsv[8];

	SensorProp_s()
	{
		uWidth = 0;
		uHeight = 0;
		type = FORMAT_RAW10;
		uPin = 0;
		uPhyType = 0;
		uLaneNum = 0;
		uPinType = 0;
		DpRate = DP_REV_1_62G;
		bAlpEn = false;
		uSlaveID = 0;
		uMode = 0;
		uFlagReg = 0;
		uFlagData = 0;
		uFlagMask = 0;
		uFlagReg1 = 0;
		uFlagData1 = 0;
		uFlagMask1 = 0;

		memset(name, 0, sizeof(name));
		uOutformat = OUTFORMAT_BGGR;

		nMclk = 0;
		nPower1 = 0;
		nPower2 = 0;
		nPower3 = 0;
		nPower4 = 0;
		nPower5 = 0;
		nPower6 = 0;
		nPower7 = 0;
		nPower8 = 0;
		nPower9 = 0;
		nPower10 = 0;

		uPort = 0;
		Rsv[0] = 0;
		Rsv[1] = 0;
		Rsv[2] = 0;
		Rsv[3] = 0;
		Rsv[4] = 0;
		Rsv[5] = 0;
		Rsv[6] = 0;
		Rsv[7] = 0;
	}
}SensorProp_t;

/* sensor参数 */
typedef struct ParaList_s
{
	/// @brief 初始化SENSOR数据表大小，单位uint32
	UINT  uParaListSize;

	/// @brief 初始化SENSOR数据表
	UINT* ParaList;                   ///<初始化SENSOR数据表

	ParaList_s()
	{
		uParaListSize = 0;
		ParaList = NULL;
	}
}ParaList_t;

/*
全新的sensor传感器属性结构体V2版本
*/
typedef struct SensorTabV2_s
{
	/// @breif Sensor初始化特性结构体
	SensorProp_t sSensorProp;

	/// @brief 初始化SENSOR数据表
	ParaList_t sParaList;
}SensorTabV2_t, *pSensorTabV2;

/**************************************************************************************** 
*
* 常用错误码
*
****************************************************************************************/

#define DT_ERROR_NO_ACTION						8		///<无动作
#define DT_ERROR_IGNORED						7		///<操作忽略掉了，不须要任何动作
#define DT_ERROR_NEED_OTHER					    6		///<需要其他数据和操作	
#define DT_ERROR_NEXT_STAGE						5		///<还须进行下一阶段，只完成了部分动作     
#define DT_ERROR_BUSY					   		4		///<正忙(上一次操作还在进行中)，此次操作不能进行
#define DT_ERROR_WAIT                	   		3		///<需要等待(进行操作的条件不成立)，可以再次尝试
#define DT_ERROR_IN_PROCESS               		2		///<正在进行，已经被操作过
#define DT_ERROR_OK								1		///<操作成功
#define DT_ERROR_FAILED							0		///<操作失败
#define DT_ERROR_INTERNAL_ERROR					-1		///<内部错误
#define DT_ERROR_UNKNOW							-1		///<未知错误
#define DT_ERROR_NOT_SUPPORTED					-2		///<不支持该功能
#define DT_ERROR_NOT_INITIALIZED         		-3		///<初始化未完成
#define DT_ERROR_PARAMETER_INVALID       		-4		///<参数无效
#define DT_ERROR_PARAMETER_OUT_OF_BOUND  		-5		///<参数越界
#define DT_ERROR_UNENABLED  					-6		///<未使能
#define DT_ERROR_UNCONNECTED					-7		///<未连接到设备
#define DT_ERROR_NOT_VALID						-8		///<功能无效
#define DT_ERROR_UNPLAY							-9		///<设备没打开
#define DT_ERROR_NOT_STARTED					-10		///<未启动
#define DT_ERROR_NOT_STOPPED					-11		///<未停止
#define DT_ERROR_NOT_READY						-12		///<未准备好
#define DT_ERROR_DESCR_FAULT					-20		///<错误的描述
#define DT_ERROR_NAME_FAULT						-21		///<错误的名称
#define DT_ERROR_VALUE_FAULT					-22		///<错误的赋值
#define DT_ERROR_LIMITED						-28		///<被限制
#define DT_ERROR_FUNCTION_INVALID				-29		///<功能无效
#define DT_ERROR_IN_AUTO						-30		///<在自动进行中，手动方式无效
#define DT_ERROR_DENIED							-31     ///<操作被拒绝
#define DT_ERROR_BAD_ALIGNMENT					-40     ///<偏移或地址未对其
#define DT_ERROR_ADDRESS_INVALID				-41     ///<地址无效
#define DT_ERROR_SIZE_INVALID					-42     ///<数据块大小无效
#define DT_ERROR_OVER_LOAD						-43     ///<数据量过载
#define DT_ERROR_UNDER_LOAD						-44     ///<数据量不够
#define DT_ERROR_BUFFER_SMALL					-52		///<Buffer空间太小
#define DT_ERROR_CHECKED_FAILED					-50     ///<检查、校验失败
#define DT_ERROR_UNUSABLE						-51		///<不可用
#define DT_ERROR_BID_INVALID					-52		///<业务ID无效或不匹配

/*ae相关错误码*/
#define DT_ERROR_AE_OVEREXPOSE					-900	///<AE调节失败，过曝，可能环境亮度太亮
#define DT_ERROR_AE_TOODARK						-901	///<AE调节失败，过暗，可能环境亮度太暗


/* IO，存储，设备相关  */
#define DT_ERROR_TIME_OUT             	   		-1000	///<超时错误
#define DT_ERROR_IO_ERROR                 		-1001	///<硬件IO错误
#define DT_ERROR_COMM_ERROR						-1002	///<通讯错误
#define DT_ERROR_BUS_ERROR				   		-1003	///<总线错误
#define DT_ERROR_FORMAT_INVALID					-1004	///<格式错误
#define DT_ERROR_CONTENT_INVALID				-1005	///<内容无效
#define DT_ERROR_BAD_CHECKSUM					-1006   ///<数据校验错误
/*i2c总线错误*/
#define DT_ERROR_I2C_FAULT						-1010	///<I2C总线错误
#define DT_ERROR_I2C_ACK_TIMEOUT				-1011	///<I2C等待应答超时
#define DT_ERROR_I2C_BUS_TIMEOUT				-1012	///<I2C等待总线动作超时，例如SCL被外部器件拉为低电平
#define DT_ERROR_I2C_WR_SINGLE_REG_FAILED		-1013	///<I2C写单个寄存器失败，等待应答超时
#define DT_ERROR_I2C_RD_SINGLE_REG_FAILED		-1014	///<I2C读单个寄存器失败，等待应答超时
#define DT_ERROR_I2C_RW_BATCH_REG_FAILED		-1015	///<I2C读写多个寄存器失败，等待应答超时
#define DT_ERROR_I2C_WR_PAGE_FAILED				-1016	///<WritesensorI2C失败，等待应答超时
#define DT_ERROR_I2C_RD_PAGE_FAILED				-1017	///<ReadsensorI2C失败，等待应答超时

#define DT_ERROR_SPI_FAULT						-1020	///<SPI总线错误
#define DT_ERROR_UART_FAULT						-1030	///<UART总线错误
#define DT_ERROR_GPIO_FAULT						-1040	///<GPIO总线错误
#define DT_ERROR_USB_FAULT						-1050	///<USB总线错误
#define DT_ERROR_PCI_FAULT						-1060	///<PCI总线错误
#define DT_ERROR_PHY_FAULT						-1070	///<物理层错误
#define DT_ERROR_LINK_FAULT						-1080	///<链路层错误
#define DT_ERROR_TRANS_FAULT					-1090	///<传输层错误

#define DT_ERROR_NO_DEVICE_FOUND				-1100	///<没有发现设备/// @brief 未找到逻辑设备
#define DT_ERROR_NO_LOGIC_DEVICE_FOUND   		-1101	///<未找到逻辑设备
#define DT_ERROR_DEVICE_IS_OPENED				-1102	///<设备已经打开
#define DT_ERROR_DEVICE_IS_CLOSED				-1103	///<设备已经关闭
#define DT_ERROR_DEVICE_IS_DISCONNECTED    		-1104  	///<设备已经断开连接
#define DT_ERROR_DEVICE_IS_OPENED_BY_ANOTHER	-1105	///<设备已经被其他主机打开
#define DT_ERROR_KERNEL_DRIVER_PROBLEM			-1106	///<内核驱动出现问题
#define DT_ERROR_SOCKET_PROBLEM					-1107	///<网络套接出现问题

#define DT_ERROR_NO_MEMORY  	   		   		-1200	///<没有足够系统内存
#define DT_ERROR_MEM_FAULT						-1201	///<存储器读写出现误码或无法正常读写
#define DT_ERROR_WRITE_PROTECTED          		-1202  	///<写保护，不可写
#define DT_ERROR_FILE_CREATE_FAILED				-1300	///<创建文件失败
#define DT_ERROR_FILE_INVALID             		-1301	///<文件格式无效
#define DT_ERROR_FILE_READ_FAILED				-1302  	///<读取文件失败
#define DT_ERROR_FILE_WRITE_FAILED				-1303  	///<写入文件失败
#define DT_ERROR_FILE_OPEN_FAILED				-1304	///<打开文件失败
#define DT_ERROR_FILE_CHECKSUM_FAILED			-1305  	///<读取数据较检失败

#define DT_ERROR_GRAB_FAILED           	   		-1600	///<数据采集失败
#define DT_ERROR_LOST_DATA                		-1601	///<数据丢失，不完整
#define DT_ERROR_EOF_ERROR           	   		-1602	///<未接收到帧结束符
#define DT_ERROR_GRAB_IS_OPENED         		-1603  	///<数据采集功能已经打开
#define DT_ERROR_GRAB_IS_CLOSED         		-1604  	///<数据采集功能已经关闭
#define DT_ERROR_GRAB_IS_STARTED         		-1605  	///<数据采集已经启动
#define DT_ERROR_GRAB_IS_STOPPED         		-1606  	///<数据采集已经停止
#define DT_ERROR_GRAB_IS_RESTARTING				-1607	///<数据采集正在重启
#define DT_ERROR_GRAB_IS_HOLD                   -1608   ///<数据采集已经暂停
#define DT_ERROR_FRAME_IS_OUTDATED              -1609   ///<采集的帧已经过时
#define DT_ERROR_ROI_PARAM_INVALID				-1610   ///<设置的ROI参数无效
#define DT_ERROR_ROI_NOT_SUPPORTED				-1611   ///<ROI功能不支持
#define DT_ERROR_GRAB_IS_DROPPED				-1613	///<丢弃重启视频流后不需要的帧
#define DT_ERROR_CALIBRATE_FAILED               -1614   ///<校准失败


/****************************************************************************************
*
* 用于方便程序编写，遇到错误时立即返回错误码 
*
****************************************************************************************/
#define CHECK_RETURN(_FUN_) \
{\
	int iChkRet = _FUN_; \
	if (iChkRet != DT_ERROR_OK) \
	return iChkRet; \
	}



#endif // __IMAGEKIT_H__
