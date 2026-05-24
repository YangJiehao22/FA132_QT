#pragma once

#ifndef __DT_CAR_DEF__
#define __DT_CAR_DEF__

#include "dtccm2.h"
/* 支持的最大设备个数 */
#define MAX_CC16			4
#define MAX_DEV				8
#define MAX_VC				4

typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

/* 测试盒机型 */
enum Box_Type
{
	Box_CC16 = 1000,
	Box_CC16Pro,
	Box_DF104 = 2000,
	Box_DF108,
	Box_DF200,
	Box_FA132 = 3000,
	Box_FA132Pro,

	Box_UC930 =4000,

	Box_FM101 = 5000,

	Box_GQ2 = 6000,
	Box_GQ4,

	Box_F22 = 7000,
};



enum SAVE_IMG_TYPE
{
	RAW8,                   /**< raw 1个像素占1个字节 0  */
	RAW,					/**< raw 1个像素占2个字节 1  */
	BMP,					/**< BMP 2  */
	JPEG,					/**< JPEG 3  */
	PNG,					/**< PNG 4  */
	YUV						/**< 原始YUV 5  */
};


/* 数据 */
typedef struct VcData_s {

	int			iWidth;			//图像iWidth
	int			iHeight;		//图像iHeight
	int			iVoltage;		//电压
	int			iCurrent;		//电流
	int			iDelay;			//帧延迟
	int			iFrameGap;		//帧间隔
	int			iLossFrameCnt;	//丢帧数量
	int			iLastLossFrameCnt;	//上一次丢帧数量
	int			iOffsetLossFrameCnt;	//偏移丢帧数量
	int			iEccErrorCnt;	//Ecc
	int			iCrcErrorCnt;	//Crc
	int			iGrabErrCnt;	//采集Err
	int			iCurrentErrCnt;	//电流Err
	int			iSsrFrameRateErrCnt;	//sensor帧率Err
	int         iSizeChanged;
	int         iHChanged;
	int         iVChanged;
	int         iErrGapTime;
	int         iSizeError;
	int         iLastSizeErrCnt;
	int			iOffsetSizeErr;	//偏移size error数量
	int         iErrPinLevel;
	int         iLockPinLevel;
	int         iErrCnt;
	int         iLockCnt;
	double		dFrameRate;		//显示帧率
	double		dSsrFrameRate;	//sensor帧率
	double		dFrErrRate;		//错误帧率
	uint64_t	uFrameCount;	//帧计数
	uint64_t	uOldFrameCount;//Old帧计数
	uint64_t	uSsrFrameID;	//sensor帧ID
	uint64_t	uOldSsrFrameID;	//sensor帧ID
	uint64_t	uFETimeStamp;	//帧时间戳(us)
	uint64_t	uOldFETimeStamp;	//帧时间戳(us)
	bool		bSsrState;		//sensor状态，false = 未检测到模组
	bool		bVideoStart;	//录制
	bool		bSample;		//抽点状态
	bool		bDisplayHQ;		//高质量显示、插值
	unsigned char        Rvs[256];
	VcData_s() {
		iWidth = 0;
		iHeight = 0;
		iVoltage = 0;
		iCurrent = 0;
		iDelay = 0;
		iFrameGap = 0;
		iLossFrameCnt = 0;
		iLastLossFrameCnt = 0;
		iOffsetLossFrameCnt = 0;
		iEccErrorCnt = 0;
		iCrcErrorCnt = 0;
		iGrabErrCnt = 0;
		iCurrentErrCnt = 0;
		iSsrFrameRateErrCnt = 0;
		iSizeChanged = 0;
		iHChanged = 0;
		iVChanged = 0;
		iErrGapTime = 0;
		iSizeError = 0;
		iLastSizeErrCnt = 0;
		iOffsetSizeErr = 0;
		iErrPinLevel = 0;
		iLockPinLevel = 0;
		iErrCnt = 0;
		iLockCnt = 0;
		dFrameRate = 0.0;
		dSsrFrameRate = 0.0;
		dFrErrRate = 0.0;
		uFrameCount = 0;
		uOldFrameCount = 0;
		uSsrFrameID = 0;
		uOldSsrFrameID = 0;
		uFETimeStamp = 0;
		uOldFETimeStamp = 0;

		bSsrState = true;
		bVideoStart = false;
		bSample = false;
		bDisplayHQ = false;

		// 将 Rvs 数组初始化为 0
		for (int i = 0; i < 256; ++i) {
			Rvs[i] = 0;
		}
	}
}VcData_t;

/* 绘图相关设置结构体 */
typedef struct DrawImage_s {
	HWND hVideoWnd;				//控件句柄
	unsigned short nImgWndW;	//控件iWidth
	unsigned short nImgWndH;	//控件iHeight

	bool bShowImg;				//绘制图像
	bool bShowText;				//绘制字体
	const char * szShowData;	//需要绘制字体的内容
	unsigned char Rvs[256];
}DrawImage_t;

/* 录像设置 */
struct RecordVideo_t {
	int		iVideoFlieType; //视频文件格式
	int		iEncodeTool;    //编码工具
	int		iFrameFps;      //视频帧率
	int		iThreadsNum;	//编码线程数
	float	fCrfRate;		//编码码率
	const char *strVideoPath;	//视频文件路径
	unsigned char SamlpeMode;	//采样录制模式
	unsigned char Rvs[255];
};


typedef struct _PMU {
	unsigned short volt;                          ///<电压, 单位:mV
	unsigned short rise;						  ///<电源上升斜率, 单位:mV/mS
	unsigned char onOff;						  ///<电源开关, 开或关
	unsigned short ocpCurrentLimit;				  ///<电源软件限流, unit mA
	unsigned char rang_Work;					  ///<电源工作挡位, unit mV\uA\nA
	unsigned short sampleSpeed_Work;			  ///<工作电流采样速率
	unsigned char rang_Standby;				  ///<电源待机挡位
	unsigned short sampleSpeed_Standby;           ///<待机电流采样速率
	unsigned short time;						  ///<电源上电时序,单位:ms
	unsigned char resv[8];						  ///<保留
}PMU;

typedef struct _PARA_LIST {
	unsigned short size;             ///<寄存器行数
	unsigned char *slaveID;         ///<器件地址
	unsigned short *reg;             ///<寄存器
	unsigned short *data;            ///<值
	unsigned char *mode;            ///<IIC模式,参考 I2CMODE
	unsigned short *time;            ///<延时时间，当前寄存器写完后延时
}PARA_LIST;

typedef struct _SENSOR {
	unsigned short width;           ///<宽
	unsigned short height;          ///<高
	unsigned short frameRate;      ///<帧率
	unsigned char pin;			///<reset pin、pwnd pin
	unsigned char pwdn;			///<pwnd pin高低
	unsigned char reset;		///<reset pin高低
	unsigned char port;			///<硬件接口,参考枚举定义SENSOR_PORT
	unsigned char type;			///<原始数据类型,参考枚举定义IMAGE_FORMAT
	unsigned char outformat;	///<颜色顺序,参考枚举定义OUTFORMAT
	unsigned char bPoc;			///<打开测试盒POC供电
	unsigned char bDcPoc;		///<DF系列是使能外供电模式，CC16是使能AHD供电
	unsigned char resv[6];		///<保留
}SENSOR;

typedef struct _GPIO {
	unsigned char bGpioEn;		///<使能GPIO
	unsigned char gpio[5];		///<GPIO电平
}GPIO;

typedef struct _SENSOR_I2C {
	unsigned char slaveID;			///<器件地址
	unsigned char slaveID_Ser;		///<串行器件地址
	unsigned char slaveID_ImgSsr;	///<图像芯片地址
	unsigned char mode;			///<iic模式, 参考 I2CMODE
	unsigned int rate;			///<iic速率, 单位:KHz
	unsigned char rapid;			///<推挽模式
	int interval;		///<I2C的字节间隔
	int ackWait;			///<I2C通讯ACK超时等待时间
	unsigned char initRegMode;		///<初始化寄存器列表模式
	unsigned char bIgnoreInitSensorResult;		///<是否忽略初始化sensor I2C结果
	unsigned char bExternalI2CPullUp;		///<是否有外部上拉
	unsigned char resv[256];			///<保留
}SENSOR_I2C;


typedef struct _VC_INFO {
	unsigned char packID;			///<虚拟通道ID
	unsigned short height;			///<高度
	unsigned char resv[3];			///<保留
}VC_INFO;

typedef struct _MIPI {
	unsigned char phyType;			///<MIPI PHY类型, 参考 
	unsigned char laneCnt;			///<MIPI Lane数量, 1\2\3\4 Lane, 1\2\3 Trio
	unsigned char isMipiLpEn;		///<MIPI LP模式使能
	unsigned char isCheckMipiLP01;  ///<卡控MIPI LP01
	unsigned char byMipiMode;		///<1是多VC模式（该模式下，可指定哪个VC是帧开始或帧结束），0是普通模式（检测到哪个VC短包先出，就作为帧开始和帧结束）
	unsigned char isFullCap;		///<是否全包采集
	unsigned char isShortPacket;    ///<短包输出使能
	unsigned char bySetFsForVc;		///<开始VC
	unsigned char bySetFeForVc;		///<结束VC
	unsigned char packCount;		///<VC数量
	unsigned char vcCount;			///<虚拟通道数量
	int packID[6];					///<VC ID
	VC_INFO vcInfo[6];		///<虚拟通道信息，ID和高度
	unsigned char byLp00MinTime;	///<LP00的最小时间，单位是：1个时钟周期
	unsigned char byMipiAdjustMode;	///<Mipi调节模式
	unsigned char byMipiAdjustCalib;	///<Mipi高级校准开关
	unsigned char byMipiAdjustCoeff;	///<时钟调整时序
	int iMipiAdjustSetTime;
	unsigned char resv[180];			///<保留
}MIPI;

typedef struct _GrabTab {
	SENSOR sensor;					///<参考SENSOR
	SENSOR_I2C sensorI2C;			///<参考SENSOR_I2C
	MIPI   mipiPara;				///<参考MIPI
	unsigned char pinDef[26];		///<PIN定义
	unsigned char resv1[4];			///<保留
	GPIO   gpioPara;				///<GPIO高低
	unsigned char resv2[4];			///<保留
	PMU    pmuSensor0;				///<电源POC
	PMU    pmuSensor1;				///<电源POC
	PMU    pmuSensor2;				///<电源POC
	PMU    pmuSensor3;				///<电源POC
	PMU    pmuVDDIO;				///<电源VDDIO
	PMU    pmuResv[3];				///<保留
}GrabTab;

typedef struct CommonSetting_s
{
	unsigned char enable_vddio;		///<使能VDDIO: 1=ture, 0=false
	unsigned char resv[256];		///<保留
	CommonSetting_s() {				///<配置默认值
		enable_vddio = 1;
		for (int i = 0;i < 256;i++)
		{
			resv[i] = 0;
		}
	}
}CommonSetting_t;
#pragma endregion

#endif