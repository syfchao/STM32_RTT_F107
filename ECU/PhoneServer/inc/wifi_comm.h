#ifndef __WIFI_COMM_H__
#define __WIFI_COMM_H__

#include "variation.h"
#include "arch/sys_arch.h"
#include "phoneServer.h"

typedef struct
{
    char ECUID[13];											//ECU ID
    unsigned int LifttimeEnergy;				//ECU 历史发电量
    unsigned int LastSystemPower;			//ECU 当前系统功率
    unsigned int GenerationCurrentDay;//ECU 当天发电量
    char LastToEMA[8];									//ECU 最后一次连接EMA的时间
    unsigned short InvertersNum;				//ECU 逆变器总数
    unsigned short LastInvertersNum;		//ECU 当前连接的逆变器总数
    unsigned char TimeZoneLength;				//ECU 时区长度
    char TimeZone[20];									//ECU 时区
    char MacAddress[7];									//ECU 有线Mac地址
    char WifiMac[7];										//ECU 无线Mac地址
    char Channel[3];										//信道
} stBaseInfo;

unsigned short packetlen(unsigned char *packet);

int Resolve_RecvData(char *RecvData,int* Data_Len,int* Command_Id);
int phone_add_inverter(int num,const char *uidstring);
//01	获取基本信息请求
void APP_Response_BaseInfo(stBaseInfo baseInfo);
//02	逆变器发电数据请求
void APP_Response_PowerGeneration(char mapping,inverter_info *inverter,int VaildNum);
//03	功率曲线请求
void APP_Response_PowerCurve(char mapping,char * date);
//04	发电量曲线请求
void APP_Response_GenerationCurve(char mapping,char request_type);
//05	逆变器ID注册请求
void APP_Response_RegisterID(char mapping);
//06	时间设置请求
void APP_Response_SetTime(char mapping);
//07	有线网络设置请求
void APP_Response_SetWiredNetwork(char mapping);
//08 	获取ECU硬件信息
void APP_Response_GetECUHardwareStatus(char mapping);
//10	AP密码设置请求
void APP_Response_SetWifiPasswd(char mapping);
//11	AP密码设置请求
void APP_Response_GetIDInfo(char mapping,inverter_info *inverter);
//12	AP密码设置请求
void APP_Response_GetTime(char mapping,char *Time);
//13	FlashSize 判断
void APP_Response_FlashSize(char mapping,unsigned int Flashsize);
//14	获取有线网络设置
void APP_Response_GetWiredNetwork(char mapping,char dhcpStatus,IP_t IPAddr,IP_t MSKAddr,IP_t GWAddr,IP_t DNS1Addr,IP_t DNS2Addr);
//15 	设置信道
void APP_Response_SetChannel(unsigned char mapflag,char SIGNAL_CHANNEL,char SIGNAL_LEVEL);
//18 	获取短地址
void APP_Response_GetShortAddrInfo(char mapping,inverter_info *inverter);
//20 命令回应
void APP_Response_GetECUAPInfo(char mapping,unsigned char connectStatus,char *info);
//21 命令回应
void APP_Response_SetECUAPInfo(unsigned char result);
//22 命令回应
void APP_Response_GetECUAPList(char mapping,char *list);
//23 命令回应
void APP_Response_GetFunctionStatusInfo(char mapping);
//24 命令回应
void APP_Response_ServerInfo(char mapping,ECUServerInfo_t *serverInfo);
//25 命令回应
void APP_Response_InverterMaxPower(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length);
//26 命令回应
void APP_Response_InverterOnOff(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length);
//27 命令回应
void APP_Response_InverterGFDI(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length);
//28 命令回应
void APP_Response_InverterIRD(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length);
// 30 命令回应
void APP_Response_RSSI(char mapping,inverter_info *inverter);
// 31 命令回应
void APP_Response_ClearEnergy(char mapping);
// 32命令回应
void APP_Response_AlarmEvent(char mapping,const char *Date,const char *serial);
#endif /*__WIFI_COMM_H__*/
