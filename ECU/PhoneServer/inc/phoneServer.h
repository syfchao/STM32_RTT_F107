#ifndef __PHONESERVER_H__
#define	__PHONESERVER_H__
/*****************************************************************************/
/* File      : phoneServer.h                                                 */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-05-19 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
typedef enum  {
  SERVER_UPDATE_GET = 1,
  SERVER_CLIENT_GET = 2,
  SERVER_CONTROL_GET = 3,
  SERVER_UPDATE_SET = 4,
  SERVER_CLIENT_SET = 5,
  SERVER_CONTROL_SET = 6
}eServerCmdType;
typedef struct ECUServerInfo {
	eServerCmdType serverCmdType;
	char domain[100];
	unsigned char IP[4];
	unsigned short Port1;
	unsigned short Port2;
		  
} ECUServerInfo_t;

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/

//获取基本信息
void Phone_GetBaseInfo(int Data_Len,const char *recvbuffer); 				//获取基本信息请求
//获取发电量数据
void Phone_GetGenerationData(int Data_Len,const char *recvbuffer); 	//获取逆变器发电量数据
//获取功率曲线
void Phone_GetPowerCurve(int Data_Len,const char *recvbuffer); 			//获取功率曲线
//获取发电量曲线
void Phone_GetGenerationCurve(int Data_Len,const char *recvbuffer); 			//获取发电量曲线
//注册ID
void Phone_RegisterID(int Data_Len,const char *recvbuffer); 			//逆变器ID注册
//设置时间
void Phone_SetTime(int Data_Len,const char *recvbuffer); 			//ECU时间设置
//设置有线网络
void Phone_SetWiredNetwork(int Data_Len,const char *recvbuffer); 			//有线网络设置
//获取硬件信息
void Phone_GetECUHardwareStatus(int Data_Len,const char *recvbuffer);
//设置WIFI AP 密码
void Phone_SetWIFIPasswd(int Data_Len,const char *recvbuffer); 			//AP密码设置
//获取ID信息
void Phone_GetIDInfo(int Data_Len,const char *recvbuffer); 			//获取ID信息
//获取时间
void Phone_GetTime(int Data_Len,const char *recvbuffer); 			//获取时间
//获取Flash空间
void Phone_FlashSize(int Data_Len,const char *recvbuffer);
//获取有线网络信息
void Phone_GetWiredNetwork(int Data_Len,const char *recvbuffer);			//有线网络设置
//设置信道
void Phone_SetChannel(int Data_Len,const char *recvbuffer);
//获取短地址
void Phone_GetShortAddrInfo(int Data_Len,const char *recvbuffer);
//获取ECU连接AP信息，   仅限于ESP07S
void Phone_GetECUAPInfo(int Data_Len,const char *recvbuffer) ;	
//设置ECU连接AP，   仅限于ESP07S
void Phone_SetECUAPInfo(int Data_Len,const char *recvbuffer); 	
//获取ECU可连接AP列表，仅限于ESP07S
void Phone_ListECUAPInfo(int Data_Len,const char *recvbuffer); 
void Phone_GetFunctionStatusInfo(int Data_Len,const char *recvbuffer);
void Phone_ServerInfo(int Data_Len,const char *recvbuffer);
//Phone Server线程
void phone_server_thread_entry(void* parameter);

#endif /*__PHONESERVER_H__*/
