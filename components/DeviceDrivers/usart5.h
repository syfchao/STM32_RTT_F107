/*****************************************************************************/
/* File      : usart.h                                                       */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-06-02 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/
#ifndef __USART5_H
#define __USART5_H
#include "stdio.h"
typedef enum 
{
    SOCKET_A = 1,
    SOCKET_B = 2,
    SOCKET_C = 3,
} eSocketType;

#define USART_REC_LEN  				4096  	//定义最大接收字节数 2048
#define SOCKETA_LEN						2048
#define SOCKETB_LEN						1460
#define SOCKETC_LEN						1460
#define WIFIFILE_LEN						1460
typedef struct socket_config
{
    char domain[32];	//域名
    char ip[16];		//IP
    int port1;			//端口1
    int port2;			//端口2
    int timeout;
    int report_interval;

}Socket_Cfg;

extern Socket_Cfg client_arg;
extern Socket_Cfg control_client_arg;

extern unsigned char WIFI_RecvSocketAData[SOCKETA_LEN];
extern unsigned char WIFI_Recv_SocketA_Event;
extern unsigned int WIFI_Recv_SocketA_LEN;

extern unsigned char WIFI_RecvSocketBData[SOCKETB_LEN];
extern unsigned char WIFI_Recv_SocketB_Event;
extern unsigned int WIFI_Recv_SocketB_LEN;

extern unsigned char WIFI_RecvSocketCData[SOCKETC_LEN];
extern unsigned char WIFI_Recv_SocketC_Event;
extern unsigned int WIFI_Recv_SocketC_LEN;

extern unsigned char WIFI_RecvWiFiFileData[WIFIFILE_LEN];
extern unsigned char WIFI_Recv_WiFiFile_Event;
extern unsigned int WIFI_Recv_WiFiFile_LEN;

unsigned short packetlen_A(unsigned char *packet);
unsigned short packetlen_B(unsigned char *packet);
unsigned short packetlen_C(unsigned char *packet);

void clear_WIFI(void);
void initSocketArgs(void);
int randport(Socket_Cfg cfg);
int WIFI_SendData(char *data, int num);
void uart5_init(unsigned int bound);
int SendToSocket(char connectID,char *data ,int length);
int WIFI_Reset(void);
int WIFI_Test(void);
int WIFI_Factory_Passwd(void);
int AT_RST(void);			//复位WIFI模块
int WIFI_Factory(char *ECUID12);
int WIFI_ChangePasswd(char *NewPasswd);

int SendToSocketA(char *data ,int length);
int SendToSocketB(char *IP ,int port,char *data ,int length);
int SendToSocketC(char *IP ,int port,char *sendbuffer ,int size);
int InitWorkMode(void);
int AT_CIPSEND(char ConnectID,int size);
int AT_CIPSTART(char ConnectID,char *connectType,char *IP,int port);
int AT_CIPCLOSE(char ConnectID);			//配置ECU连接无线路由器名

int AT_CIPMUX1(void);			//设置多连接AT命令
int AT_CIPSERVER(void);			//设置多连接AT命令
int AT_CIPSTO(void);		//配置WIFI模块作为TCP服务器时的超时时间
int InitWorkMode(void);
int AT_CWJAPStatus(char *info);			//查询ECU连接无线路由器名 返回1表示获取成功连接，返回0表示未连接
int AT_CWJAP(char *SSID,char *PASSWD);			//配置ECU连接无线路由器名
int AT_CWLAPList(char *liststr);
void WIFI_GetEvent_ESP07S(void);
int AT_CWMODE3(int mode);

#endif

