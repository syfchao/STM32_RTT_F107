/*****************************************************************************/
/* File      : phoneServer.c                                                 */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-05-19 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "phoneServer.h"
#include "rtthread.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "debug.h"
#include "threadlist.h"
#include "wifi_comm.h"
#include "variation.h"
#include "main-thread.h"
#include "file.h"
#include "rtc.h"
#include "version.h"
#include "arch/sys_arch.h"
#include "usart5.h"
#include "client.h"
#include "myfile.h"
#include <lwip/netdb.h> /* Ϊ�˽�������������Ҫ����netdb.hͷ�ļ� */
#include <lwip/sockets.h> /* ʹ��BSD socket����Ҫ����sockets.hͷ�ļ� */
#include <dfs_fs.h>
#include <dfs_file.h>
#include "file.h"
#include <dfs_posix.h> 
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "channel.h"
#include "key.h"
#include "timer.h"
#include "usart5.h"
#include "InternalFlash.h"

#define MIN_FUNCTION_ID 0
#define MAX_FUNCTION_ID 14


extern rt_mutex_t wifi_uart_lock;
extern ecu_info ecu;
extern inverter_info inverter[MAXINVERTERCOUNT];
extern unsigned char WIFI_RST_Event;
extern unsigned char APKEY_EVENT;
unsigned char APSTA_Status = 0;		//��ǰAP,STAģʽ   0��ʾSTAģʽ��1��ʾSTA+APģʽ
static rt_timer_t APSTAtimer;	//�߳�������ʱ��
unsigned char  switchSTAModeFlag = 0;	//�л�ΪSTAģʽ��־

enum CommandID{
    P0000, P0001, P0002, P0003, P0004, P0005, P0006, P0007, P0008, P0009, //0-9
    P0010, P0011, P0012, P0013, P0014, P0015, P0016, P0017, P0018, P0019, //10-19
    P0020, P0021, P0022, P0023, P0024, P0025, P0026, P0027, P0028, P0029, //20-29
    P0030, P0031, P0032, P0033, P0034, P0035, P0036, P0037, P0038, P0039, //30-39
    P0040, P0041, P0042, P0043, P0044, P0045, P0046, P0047, P0048, P0049, //40-49
    P0050, P0051, P0052, P0053, P0054, P0055, P0056, P0057, P0058, P0059, //50-59
};


void (*pfun_Phone[100])(int Data_Len,const char *recvbuffer);

void add_Phone_functions(void)
{
    pfun_Phone[P0001] = Phone_GetBaseInfo; 				//��ȡ������Ϣ����
    pfun_Phone[P0002] = Phone_GetGenerationData; 	//��ȡ���������������
    pfun_Phone[P0003] = Phone_GetPowerCurve; 			//��ȡ��������
    pfun_Phone[P0004] = Phone_GetGenerationCurve; 			//��ȡ����������
    pfun_Phone[P0005] = Phone_RegisterID; 			//�����IDע��
    pfun_Phone[P0006] = Phone_SetTime; 			//ECUʱ������
    pfun_Phone[P0007] = Phone_SetWiredNetwork; 			//������������
    pfun_Phone[P0008] = Phone_GetECUHardwareStatus; 	//�鿴��ǰECUӲ��״̬
    //pfun_Phone[P0009] = Phone_SearchWIFIStatus; 			//������������״̬
    pfun_Phone[P0010] = Phone_SetWIFIPasswd; 			//AP��������
    pfun_Phone[P0011] = Phone_GetIDInfo; 			//��ȡID��Ϣ
    pfun_Phone[P0012] = Phone_GetTime; 			//��ȡʱ��
    pfun_Phone[P0013] = Phone_FlashSize; 			//��ȡFLASH�ռ�
    pfun_Phone[P0014] = Phone_GetWiredNetwork; 			//��ȡ��������
    pfun_Phone[P0015] = Phone_SetChannel;			//�����ŵ�
    pfun_Phone[P0018] = Phone_GetShortAddrInfo;		//���ʵ�����ѹ����

    pfun_Phone[P0020] = Phone_GetECUAPInfo;		//���ʵ�����ѹ����
    pfun_Phone[P0021] = Phone_SetECUAPInfo;		//���ʵ�����ѹ����
    pfun_Phone[P0022] = Phone_ListECUAPInfo;		//���ʵ�����ѹ����
    pfun_Phone[P0023] = Phone_GetFunctionStatusInfo;
    pfun_Phone[P0024] = Phone_ServerInfo;				//�鿴��������ط�������Ϣ
    pfun_Phone[P0025] = Phone_InverterMaxPower;		//��ȡ��������󱣻�����ֵ
    pfun_Phone[P0026] = Phone_InverterOnOff;		//��ȡ��������������ػ�
    pfun_Phone[P0027] = Phone_InverterGFDI;		//��ȡ������GFDI����
    pfun_Phone[P0028] = Phone_InverterIRD;		//��ȡ������IRD���Ʊ�־
    //pfun_Phone[P0029] = Phone_ACProtection;		//��ȡ�����ñ�������
    pfun_Phone[P0030] = Phone_RSSI;			//��ȡ������ź�ǿ��
    pfun_Phone[P0031] = Phone_ClearEnergy;		//������ݿ⣨�����ʷ��������
    pfun_Phone[P0032] = Phone_AlarmEvent;		//�鿴���������״̬

}



//����0 ��ʾ��ʽ��ȷ������1��ʾ���͸�ʽ����
int strToHex(const char *recvbuff,unsigned char *buff,int length)
{
	int i =0;
	for(i = 0;i<length;i++)
	{
		if((recvbuff[i*2] >= 'a') && (recvbuff[i*2] <= 'f'))
		{
			buff[i] = (recvbuff[i*2] -'a' + 10)*0x10;
		}else if((recvbuff[i*2] >= 'A') && (recvbuff[i*2] <= 'F'))
		{
			buff[i] = (recvbuff[i*2] -'A' + 10)*0x10;
		}else if((recvbuff[i*2] >= '0') && (recvbuff[i*2] <= '9'))
		{
			buff[i] = (recvbuff[i*2] -'0' )*0x10;
		}else
		{
			return -1;
		}

		if((recvbuff[i*2+1] >= 'a') && (recvbuff[i*2+1] <= 'f'))
		{
			buff[i] += (recvbuff[i*2+1] -'a' + 10);
		}else if((recvbuff[i*2+1] >= 'A') && (recvbuff[i*2+1] <= 'F'))
		{
			buff[i] += (recvbuff[i*2+1] -'A' + 10);
		}else if((recvbuff[i*2+1] >= '0') && (recvbuff[i*2+1] <= '9'))
		{
			buff[i] += (recvbuff[i*2+1] -'0');
		}else
		{
			return -1;
		}
		
	}
	return 0;
}

int getAddr(char *buff,IPConfig_t *IPconfig)
{
    IPconfig->IPAddr.IP1 = buff[0];
    IPconfig->IPAddr.IP2 = buff[1];
    IPconfig->IPAddr.IP3 = buff[2];
    IPconfig->IPAddr.IP4 = buff[3];
    
    IPconfig->MSKAddr.IP1 = buff[4];
    IPconfig->MSKAddr.IP2 = buff[5];
    IPconfig->MSKAddr.IP3 = buff[6];
    IPconfig->MSKAddr.IP4 = buff[7];
   
    IPconfig->GWAddr.IP1 = buff[8];
    IPconfig->GWAddr.IP2 = buff[9];
    IPconfig->GWAddr.IP3 = buff[10];
    IPconfig->GWAddr.IP4 = buff[11];
       
    IPconfig->DNS1Addr.IP1 = buff[12];
    IPconfig->DNS1Addr.IP2 = buff[13];
    IPconfig->DNS1Addr.IP3 = buff[14];
    IPconfig->DNS1Addr.IP4 = buff[15];
       
    IPconfig->DNS2Addr.IP1 = buff[16];
    IPconfig->DNS2Addr.IP2 = buff[17];
    IPconfig->DNS2Addr.IP3 = buff[18];
    IPconfig->DNS2Addr.IP4 = buff[19];
      
    return 0;
}


int Resolve2Str(char *firstStr,int *firstLen,char *secondStr,int *secondLen,char *string)
{
    *firstLen = (string[0]-'0')*10+(string[1]-'0');
    memcpy(firstStr,&string[2],*firstLen);
    *secondLen = (string[2+(*firstLen)]-'0')*10+(string[3+(*firstLen)]-'0');
    memcpy(secondStr,&string[4+*firstLen],*secondLen);

    return 0;
}

int ResolveWifiSSID(char *SSID,int *SSIDLen,char *Auth,char *Encry,char *PassWD,int *PassWDLen,char *string)
{
    *SSIDLen = (string[0]-'0')*100+(string[1]-'0')*10+(string[2]-'0');
    memcpy(SSID,&string[3],*SSIDLen);
    *Auth = string[3+(*SSIDLen)];
    *Encry = string[4+(*SSIDLen)];
    *PassWDLen = (string[5+(*SSIDLen)]-'0')*100+(string[6+(*SSIDLen)]-'0')*10+(string[7+(*SSIDLen)]-'0');
    memcpy(PassWD,&string[8+*SSIDLen],*PassWDLen);
    return 0;
}	

//����0 ��ʾ��̬IP  ����1 ��ʾ�̶�IP   ����-1��ʾʧ��
int ResolveWired(const char *string,IP_t *IPAddr,IP_t *MSKAddr,IP_t *GWAddr,IP_t *DNS1Addr,IP_t *DNS2Addr)
{
    if(string[1] == '0')	//��̬IP
    {
        return 0;
    }else if(string[1] == '1') //��̬IP
    {
        IPAddr->IP1 = string[2];
        IPAddr->IP2 = string[3];
        IPAddr->IP3 = string[4];
        IPAddr->IP4 = string[5];
        MSKAddr->IP1 = string[6];
        MSKAddr->IP2 = string[7];
        MSKAddr->IP3 = string[8];
        MSKAddr->IP4 = string[9];
        GWAddr->IP1 = string[10];
        GWAddr->IP2 = string[11];
        GWAddr->IP3 = string[12];
        GWAddr->IP4 = string[13];
        DNS1Addr->IP1 = string[14];
        DNS1Addr->IP2 = string[15];
        DNS1Addr->IP3 = string[16];
        DNS1Addr->IP4 = string[17];
        DNS2Addr->IP1 = string[18];
        DNS2Addr->IP2 = string[19];
        DNS2Addr->IP3 = string[20];
        DNS2Addr->IP4 = string[21];
        return 1;
    }else
    {
        return -1;
    }

}

int ResolveServerInfo(const char *string,ECUServerInfo_t *serverInfo)
{
    char cmd[3] = {'\0'};
    unsigned char cmdNO = 0;
    char domainLenStr[4] = {'\0'};
    unsigned char domainLen = 0;

    memcpy(cmd,&string[25],2);
    cmdNO = atoi(cmd);
    if((cmdNO<=0) ||(cmdNO>=7))
        return -1;
    serverInfo->serverCmdType = (eServerCmdType)atoi(cmd);
    if((serverInfo->serverCmdType == SERVER_UPDATE_GET)||(serverInfo->serverCmdType == SERVER_CLIENT_GET)||(serverInfo->serverCmdType == SERVER_CONTROL_GET))
        return 0;
    memcpy(domainLenStr,&string[30],3);
    domainLen = atoi(domainLenStr);
    memcpy(serverInfo->domain,&string[33],domainLen);
    serverInfo->domain[domainLen] = '\0';
    memcpy(serverInfo->IP,&string[33+domainLen],4);

    serverInfo->Port1 = string[37+domainLen]*256 + string[38+domainLen];
    serverInfo->Port2 = string[39+domainLen]*256 + string[40+domainLen];
    return 0;

}

//��ȡ�������� 
void Phone_GetBaseInfo(int Data_Len,const char *recvbuffer) 				//��ȡ������Ϣ����
{
    stBaseInfo baseInfo;
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 01 ",(char *)recvbuffer);

    memcpy(baseInfo.ECUID,ecu.id,13);											//ECU ID		OK
    baseInfo.LifttimeEnergy = (int)(ecu.life_energy*10);				//ECU ��ʷ������		OK
    baseInfo.LastSystemPower = ecu.system_power;			//ECU ��ǰϵͳ����		OK
    baseInfo.GenerationCurrentDay = (unsigned int)(ecu.today_energy * 100);//ECU ���췢����

    memset(baseInfo.LastToEMA,'\0',8);	//ECU ���һ������EMA��ʱ��
    baseInfo.LastToEMA[0] = (ecu.last_ema_time[0] - '0')*16+(ecu.last_ema_time[1] - '0');
    baseInfo.LastToEMA[1] = (ecu.last_ema_time[2] - '0')*16+(ecu.last_ema_time[3] - '0');
    baseInfo.LastToEMA[2] = (ecu.last_ema_time[4] - '0')*16+(ecu.last_ema_time[5] - '0');
    baseInfo.LastToEMA[3] = (ecu.last_ema_time[6] - '0')*16+(ecu.last_ema_time[7] - '0');
    baseInfo.LastToEMA[4] = (ecu.last_ema_time[8] - '0')*16+(ecu.last_ema_time[9] - '0');
    baseInfo.LastToEMA[5] = (ecu.last_ema_time[10] - '0')*16+(ecu.last_ema_time[11] - '0');
    baseInfo.LastToEMA[6] = (ecu.last_ema_time[12] - '0')*16+(ecu.last_ema_time[13] - '0');

    baseInfo.InvertersNum = ecu.total;				//ECU ���������
    baseInfo.LastInvertersNum = ecu.count;		//ECU ��ǰ���ӵ����������
    baseInfo.TimeZoneLength = 9;				//ECU ʱ������
    sprintf(baseInfo.TimeZone,"Etc/GMT-8");									//ECU ʱ��
    memset(baseInfo.MacAddress,'\0',7);
    memcpy(baseInfo.MacAddress,ecu.MacAddress,7);//ECU ����Mac��ַ
    sprintf(baseInfo.Channel,"%02x",ecu.channel);
    APP_Response_BaseInfo(baseInfo);
}

//��ȡ���������������
void Phone_GetGenerationData(int Data_Len,const char *recvbuffer) 	//��ȡ���������������
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 02 ",(char *)recvbuffer);

    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {	//ƥ��ɹ�������Ӧ����
        APP_Response_PowerGeneration(0x00,inverter,ecu.total);
    }else
    {
        APP_Response_PowerGeneration(0x01,inverter,ecu.total);
    }
}

//��ȡ��������
void Phone_GetPowerCurve(int Data_Len,const char *recvbuffer) 			//��ȡ��������
{
    char date[9] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 03 ",(char *)recvbuffer);
    memset(date,'\0',9);
    memcpy(date,&recvbuffer[28],8);

    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {	//ƥ��ɹ�������Ӧ����
        APP_Response_PowerCurve(0x00,date);
    }else
    {
        APP_Response_PowerCurve(0x01,date);
    }

}

//��ȡ����������
void Phone_GetGenerationCurve(int Data_Len,const char *recvbuffer) 			//��ȡ����������
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event04 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {	//ƥ��ɹ�������Ӧ����
        APP_Response_GenerationCurve(0x00,recvbuffer[29]);
    }else
    {
        APP_Response_GenerationCurve(0x01,recvbuffer[29]);
    }
}

//�����IDע��
void Phone_RegisterID(int Data_Len,const char *recvbuffer) 			//�����IDע��
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 05 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        int i = 0;
        inverter_info *curinverter = inverter;
        int AddNum = 0;
        //ƥ��ɹ�������Ӧ����
        //����̨��
        AddNum = (Data_Len - 31)/6;
        APP_Response_RegisterID(0x00);
        //���ID���ļ�
        phone_add_inverter(AddNum,&recvbuffer[28]);

        for(i=0; i<MAXINVERTERCOUNT; i++, curinverter++)
        {
            rt_memset(curinverter->id, '\0', sizeof(curinverter->id));		//��������UI
            curinverter->model = 0;

            curinverter->dv=0;			//��յ�ǰһ��ֱ����ѹ
            curinverter->di=0;			//��յ�ǰһ��ֱ������
            curinverter->op=0;			//��յ�ǰ������������
            curinverter->gf=0;			//��յ���Ƶ��
            curinverter->it=0;			//���������¶�
            curinverter->gv=0;			//��յ�����ѹ
            curinverter->dvb=0;			//B·��յ�ǰһ��ֱ����ѹ
            curinverter->dib=0;			//B·��յ�ǰһ��ֱ������
            curinverter->opb=0;			//B·��յ�ǰ������������
            curinverter->gvb=0;
            curinverter->dvc=0;
            curinverter->dic=0;
            curinverter->opc=0;
            curinverter->gvc=0;
            curinverter->dvd=0;
            curinverter->did=0;
            curinverter->opd=0;
            curinverter->gvd=0;

            curinverter->curgeneration = 0;	//����������ǰһ�ַ�����
            curinverter->curgenerationb = 0;	//B·��յ�ǰһ�ַ�����

            curinverter->preaccgen = 0;
            curinverter->preaccgenb = 0;
            curinverter->curaccgen = 0;
            curinverter->curaccgenb = 0;
            curinverter->preacctime = 0;
            curinverter->curacctime = 0;

            rt_memset(curinverter->status_web, '\0', sizeof(curinverter->status_web));		//��������״̬
            rt_memset(curinverter->status, '\0', sizeof(curinverter->status));		//��������״̬
            rt_memset(curinverter->statusb, '\0', sizeof(curinverter->statusb));		//B·��������״̬

            curinverter->inverterstatus.dataflag = 0;		//��һ�������ݵı�־��λ
            curinverter->no_getdata_num=0;	//ZK,���������ȡ�����Ĵ���
            curinverter->disconnect_times=0;		//û���������ͨ���ϵĴ�����0, ZK

            curinverter->inverterstatus.updating=0;
        }

        get_id_from_file(inverter);
        unlink("/home/data/power");
        unlink("/home/data/ird");
        restartThread(TYPE_DRM);
        //����main�߳�
        restartThread(TYPE_MAIN);

    }else
    {
        APP_Response_RegisterID(0x01);
    }
}

//ECUʱ������
void Phone_SetTime(int Data_Len,const char *recvbuffer) 			//ECUʱ������
{
    char setTime[15];
    char getTime[15];
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 06 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        //ƥ��ɹ�������Ӧ����
        memset(setTime,'\0',15);
        memcpy(setTime,&recvbuffer[28],14);
        apstime(getTime);
        if(!memcmp("99999999",setTime,8))
        {	//��������ն���9 ������ȡ��ǰ��
            memcpy(setTime,getTime,8);
        }

        if(!memcmp("999999",&setTime[8],6))
        {	//���ʱ�䶼��9 ʱ��ȡ��ǰ��
            memcpy(&setTime[8],&getTime[8],6);
        }

        //����ʱ��
        set_time(setTime);
        //����main�߳�
        restartThread(TYPE_MAIN);
        APP_Response_SetTime(0x00);
    }else
    {
        APP_Response_SetTime(0x01);
    }

}

//������������
void Phone_SetWiredNetwork(int Data_Len,const char *recvbuffer)			//������������
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 07 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        int ModeFlag = 0;
        char buff[200] = {'\0'};
        IP_t IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr;
        //ƥ��ɹ�������Ӧ����
        APP_Response_SetWiredNetwork(0x00);
        //�����DHCP  ���ǹ̶�IP
        ModeFlag = ResolveWired(&recvbuffer[28],&IPAddr,&MSKAddr,&GWAddr,&DNS1Addr,&DNS2Addr);
        if(ModeFlag == 0x00)		//DHCP
        {
            printmsg(ECU_DBG_WIFI,"dynamic IP");
            WritePage(INTERNAL_FLASH_IPCONFIG,"0",1);
            dhcp_reset();
        }else if (ModeFlag == 0x01)		//�̶�IP
        {
            printmsg(ECU_DBG_WIFI,"static IP");
            //���������ַ
            buff[0] = '1';
	   buff[1] = IPAddr.IP1;
	   buff[2] = IPAddr.IP2;
	   buff[3] = IPAddr.IP3;
	   buff[4] = IPAddr.IP4;
	   buff[5] = MSKAddr.IP1;
	   buff[6] = MSKAddr.IP2;
	   buff[7] = MSKAddr.IP3;
	   buff[8] = MSKAddr.IP4;
	   buff[9] = GWAddr.IP1;
	   buff[10] = GWAddr.IP2;
	   buff[11] = GWAddr.IP3;
	   buff[12] = GWAddr.IP4;
	   buff[13] = DNS1Addr.IP1;
	   buff[14] = DNS1Addr.IP2;
	   buff[15] = DNS1Addr.IP3;
	   buff[16] = DNS1Addr.IP4;
	   buff[17] = DNS2Addr.IP1;
	   buff[18] = DNS2Addr.IP2;
	   buff[19] = DNS2Addr.IP3;
	   buff[20] = DNS2Addr.IP4;
            WritePage(INTERNAL_FLASH_IPCONFIG,buff,21);
            //���ù̶�IP
            StaticIP(IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr);
        }
    }
    else
    {
        APP_Response_SetWiredNetwork(0x01);
    }
}

//��ȡӲ����Ϣ
void Phone_GetECUHardwareStatus(int Data_Len,const char *recvbuffer) 
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 08 ",(char *)recvbuffer);
    if(!memcmp(&WIFI_RecvSocketAData[13],ecu.id,12))
    {	//ƥ��ɹ�������Ӧ�Ĳ���
        APP_Response_GetECUHardwareStatus(0x00);
    }else
    {
        APP_Response_GetECUHardwareStatus(0x01);
    }

}

//AP��������
void Phone_SetWIFIPasswd(int Data_Len,const char *recvbuffer) 			//AP��������
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 10 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        char OldPassword[100] = {'\0'};
        char NewPassword[100] = {'\0'};
        char EEPROMPasswd[100] = {'\0'};
        int oldLen,newLen;

        //ƥ��ɹ�������Ӧ����
        //printf("COMMAND_SETWIFIPASSWD  Mapping\n");
        //��ȡ����
        Resolve2Str(OldPassword,&oldLen,NewPassword,&newLen,(char *)&recvbuffer[28]);
        //��ȡ�����룬�����������ͬ������������
        get_Passwd(EEPROMPasswd);

        if((!memcmp(EEPROMPasswd,OldPassword,oldLen))&&(oldLen == strlen(EEPROMPasswd)))
        {
            APP_Response_SetWifiPasswd(0x00);
            WIFI_ChangePasswd(NewPassword);

            set_Passwd(NewPassword,newLen);
        }else
        {
            APP_Response_SetWifiPasswd(0x02);
        }
    }	else
    {
        APP_Response_SetWifiPasswd(0x01);
    }

}

//��ȡID��Ϣ
void Phone_GetIDInfo(int Data_Len,const char *recvbuffer) 			//��ȡID��Ϣ
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 11 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        APP_Response_GetIDInfo(0x00,inverter);
    }else
    {
        APP_Response_GetIDInfo(0x01,inverter);
    }
}

//��ȡʱ��
void Phone_GetTime(int Data_Len,const char *recvbuffer) 			//��ȡʱ��
{
    char Time[15] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 12 ",(char *)recvbuffer);
    apstime(Time);
    Time[14] = '\0';
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        APP_Response_GetTime(0x00,Time);
    }else
    {
        APP_Response_GetTime(0x01,Time);
    }
}

//��ȡFLASH�ռ�
void Phone_FlashSize(int Data_Len,const char *recvbuffer) 			//��ȡʱ��
{
    int result;
    long long cap;
    struct statfs buffer;

    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 13 ",(char *)recvbuffer);
    result = dfs_statfs("/", &buffer);
    if (result != 0)
    {
        APP_Response_FlashSize(0x00,0);
        return;
    }
    cap = buffer.f_bsize * buffer.f_bfree / 1024;
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        APP_Response_FlashSize(0x00,(unsigned int)cap);
    }else
    {
        APP_Response_FlashSize(0x01,(unsigned int)cap);
    }
}

//��ȡ��������
void Phone_GetWiredNetwork(int Data_Len,const char *recvbuffer)			//��ȡ������������
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 14 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        unsigned int addr = 0;
        extern struct netif *netif_list;
        struct netif * netif;
        int ModeFlag = 0;
        ip_addr_t DNS;
        IP_t IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr;
        //ƥ��ɹ�������Ӧ����
        //printf("COMMAND_GETWIREDNETWORK  Mapping\n");
        netif = netif_list;
        addr = ip4_addr_get_u32(&netif->ip_addr);
        IPAddr.IP4 = (addr/16777216)%256;
        IPAddr.IP3 = (addr/65536)%256;
        IPAddr.IP2 = (addr/256)%256;
        IPAddr.IP1 = (addr/1)%256;

        addr = ip4_addr_get_u32(&netif->netmask);
        MSKAddr.IP4 = (addr/16777216)%256;
        MSKAddr.IP3 = (addr/65536)%256;
        MSKAddr.IP2 = (addr/256)%256;
        MSKAddr.IP1 = (addr/1)%256;

        addr = ip4_addr_get_u32(&netif->gw);
        GWAddr.IP4 = (addr/16777216)%256;
        GWAddr.IP3 = (addr/65536)%256;
        GWAddr.IP2 = (addr/256)%256;
        GWAddr.IP1 = (addr/1)%256;

        DNS = dns_getserver(0);
        addr = ip4_addr_get_u32(&DNS);
        DNS1Addr.IP4 = (addr/16777216)%256;
        DNS1Addr.IP3 = (addr/65536)%256;
        DNS1Addr.IP2 = (addr/256)%256;
        DNS1Addr.IP1 = (addr/1)%256;

        DNS = dns_getserver(1);
        addr = ip4_addr_get_u32(&DNS);
        DNS2Addr.IP4 = (addr/16777216)%256;
        DNS2Addr.IP3 = (addr/65536)%256;
        DNS2Addr.IP2 = (addr/256)%256;
        DNS2Addr.IP1 = (addr/1)%256;

        //�����DHCP  ���ǹ̶�IP
        ModeFlag = get_DHCP_Status();

        APP_Response_GetWiredNetwork(0x00,ModeFlag,IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr);

    }	else
    {
        int ModeFlag = 0;
        IP_t IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr;
        APP_Response_GetWiredNetwork(0x01,ModeFlag,IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr);
    }
}


int switchChannel(unsigned char *buff)
{
    int ret=0x10;
    if((buff[0]>='0') && (buff[0]<='9'))
        buff[0] -= 0x30;
    if((buff[0]>='A') && (buff[0]<='F'))
        buff[0] -= 0x37;
    if((buff[0]>='a') && (buff[0]<='f'))
        buff[0] -= 0x57;
    if((buff[1]>='0') && (buff[1]<='9'))
        buff[1] -= 0x30;
    if((buff[1]>='A') && (buff[1]<='F'))
        buff[1] -= 0x37;
    if((buff[1]>='a') && (buff[1]<='f'))
        buff[1] -= 0x57;
    ret = (buff[0]*16+buff[1]);
    return ret;
}

//�����ŵ�
void Phone_SetChannel(int Data_Len,const char *recvbuffer)
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 15 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {	//ƥ��ɹ�������Ӧ�Ĳ���
        unsigned char old_channel = 0x00;
        unsigned char new_channel = 0x00;

        APP_Response_SetChannel(0x00,ecu.channel,0);
        old_channel = switchChannel(&WIFI_RecvSocketAData[28]);
        new_channel = switchChannel(&WIFI_RecvSocketAData[30]);

        saveOldChannel(old_channel);
        saveNewChannel(new_channel);
        saveChannel_change_flag();
        //����main�߳�
        restartThread(TYPE_MAIN);
    }else
    {
        APP_Response_SetChannel(0x01,ecu.channel,0);
    }

}

void Phone_GetShortAddrInfo(int Data_Len,const char *recvbuffer) 			//��ȡID��Ϣ
{


    //printf("WIFI_Recv_Event%d %s\n",18,recvbuffer);
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 18",(char *)recvbuffer);
    if(!memcmp(&WIFI_RecvSocketAData[13],ecu.id,12))
    {
        APP_Response_GetShortAddrInfo(0x00,inverter);
    }else
    {
        APP_Response_GetShortAddrInfo(0x01,inverter);
    }

}

void Phone_GetECUAPInfo(int Data_Len,const char *recvbuffer) 			//��ȡECU����AP��Ϣ
{
    unsigned char connectStatus = 0;
    char info[100] = {'\0'};

    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 20 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        connectStatus = AT_CWJAPStatus(info);
        printf("connectStatus:%d\n",connectStatus);
        APP_Response_GetECUAPInfo(0x00,connectStatus,info);
    }else
    {
        APP_Response_GetECUAPInfo(0x01,connectStatus,info);
    }

}

void Phone_SetECUAPInfo(int Data_Len,const char *recvbuffer) 			//����ECU����AP
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 21 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        //ƥ��ɹ�������Ӧ�Ĳ���
        char SSID[100] = {'\0'};
        char Password[100] = {'\0'};

        int ssidLen,passwdLen;

        Resolve2Str(SSID,&ssidLen,Password,&passwdLen,(char *)&recvbuffer[30]);
        printf("SSID:  %s   ,PASSWD: %s \n",SSID,Password);

        APP_Response_SetECUAPInfo(0x00);
        AT_CWJAP(SSID,Password);
    }else
    {
        APP_Response_SetECUAPInfo(0x01);
    }

}

void Phone_ListECUAPInfo(int Data_Len,const char *recvbuffer) 			//�о�ECU ��ѯ����AP��Ϣ
{
    unsigned char ret = 0;
    char *list = NULL;

    list = malloc(4096);
    memset(list,'\0',4096);

    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 22 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        ret = AT_CWLAPList(list);
        if(0 == ret)
        {
            APP_Response_GetECUAPList(0x00,list);
        }else
        {
            APP_Response_GetECUAPList(0x00,"");
        }

    }else
    {
        APP_Response_GetECUAPList(0x01,list);
    }
    free(list);
    list = NULL;

}

void Phone_GetFunctionStatusInfo(int Data_Len,const char *recvbuffer) 			//�о�ECU ��ѯ����AP��Ϣ
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 23 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        APP_Response_GetFunctionStatusInfo(0x00);
    }else
    {
        APP_Response_GetFunctionStatusInfo(0x01);
    }

}

void Phone_ServerInfo(int Data_Len,const char *recvbuffer) 			
{
    int ret = 0;
    ECUServerInfo_t serverInfo;
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 24 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        //�ж��Ǿ�����������
        ret = ResolveServerInfo(recvbuffer,&serverInfo);
        if(ret == 0)
        {
            APP_Response_ServerInfo(0x00,&serverInfo);
        }else
        {
            return;
        }
    }else
    {
        APP_Response_ServerInfo(0x01,&serverInfo);
    }

}


void Phone_InverterMaxPower(int Data_Len,const char *recvbuffer) 
{
    int cmd = 0;
    char cmd_str[3] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 25 ",(char *)recvbuffer);
    memcpy(cmd_str,&recvbuffer[25],2);
    cmd_str[2] = '\0';
    cmd = atoi(cmd_str);

    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        if(1 == cmd)
        {	//��ȡ�����ֵ
            APP_Response_InverterMaxPower(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(2 == cmd)
        {	//���������ֵ(�㲥)
            APP_Response_InverterMaxPower(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(3 == cmd)
        {	//���������ֵ(����)
            APP_Response_InverterMaxPower(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(4 == cmd)
        {	//���������ֵ(����)
            APP_Response_InverterMaxPower(0x00,cmd,inverter,recvbuffer,Data_Len);
        }
        {
            return;
        }
    }else
    {
        APP_Response_InverterMaxPower(0x01,cmd,inverter,recvbuffer,Data_Len);
    }

}

void Phone_InverterOnOff(int Data_Len,const char *recvbuffer) 
{
    int cmd = 0;
    char cmd_str[3] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 26 ",(char *)recvbuffer);
    memcpy(cmd_str,&recvbuffer[25],2);
    cmd_str[2] = '\0';
    cmd = atoi(cmd_str);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        if(1 == cmd)
        {
            APP_Response_InverterOnOff(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(2 == cmd)
        {
            APP_Response_InverterOnOff(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(3 == cmd)
        {
            APP_Response_InverterOnOff(0x00,cmd,inverter,recvbuffer,Data_Len);
        }
        {
            return;
        }
    }else
    {
        APP_Response_InverterOnOff(0x01,cmd,inverter,recvbuffer,Data_Len);
    }

}


void Phone_InverterGFDI(int Data_Len,const char *recvbuffer) 
{
    int cmd = 0;
    char cmd_str[3] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 27 ",(char *)recvbuffer);
    memcpy(cmd_str,&recvbuffer[25],2);
    cmd_str[2] = '\0';
    cmd = atoi(cmd_str);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        if(1 == cmd)
        {
            APP_Response_InverterGFDI(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(2 == cmd)
        {
            APP_Response_InverterGFDI(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(3 == cmd)
        {
            APP_Response_InverterGFDI(0x00,cmd,inverter,recvbuffer,Data_Len);
        }
        {
            return;
        }
    }else
    {
        APP_Response_InverterGFDI(0x01,cmd,inverter,recvbuffer,Data_Len);
    }

}

void Phone_InverterIRD(int Data_Len,const char *recvbuffer) 
{
    int cmd = 0;
    char cmd_str[3] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 28 ",(char *)recvbuffer);
    memcpy(cmd_str,&recvbuffer[25],2);
    cmd_str[2] = '\0';
    cmd = atoi(cmd_str);

    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        if(1 == cmd)
        {	//��ȡIRD״̬
            APP_Response_InverterIRD(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(2 == cmd)
        {	//����IRD״̬(�㲥)
            APP_Response_InverterIRD(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(3 == cmd)
        {	//����IRD״̬(����)
            APP_Response_InverterIRD(0x00,cmd,inverter,recvbuffer,Data_Len);
        }else if(4 == cmd)
        {	//�·���ȡIRD״̬����
            APP_Response_InverterIRD(0x00,cmd,inverter,recvbuffer,Data_Len);
        }
        {
            return;
        }
    }else
    {
        APP_Response_InverterIRD(0x01,cmd,inverter,recvbuffer,Data_Len);
    }

}

//��ȡ�ź�ǿ��
void Phone_RSSI(int Data_Len,const char *recvbuffer) 			
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 30 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        APP_Response_RSSI(0x00,inverter);
    }else
    {
        APP_Response_RSSI(0x01,inverter);
    }

}

//�����ʷ������
void Phone_ClearEnergy(int Data_Len,const char *recvbuffer) 
{
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 31 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        //ɾ����ʷ������
        echo("/home/data/ltpower","0.000000");
        rm_dir("/home/record/ENERGY");
        rm_dir("/home/record/POWER");
        ecu.life_energy = 0;
        ecu.today_energy = 0;
        APP_Response_ClearEnergy(0x00);
    }else
    {
        APP_Response_ClearEnergy(0x01);
    }
}

//�ϱ��澯�¼�
void Phone_AlarmEvent(int Data_Len,const char *recvbuffer) 
{
    char Date[9] = {'\0'};
    char serial[4] = {'\0'};
    print2msg(ECU_DBG_WIFI,"WIFI_Recv_Event 32 ",(char *)recvbuffer);
    if(!memcmp(&recvbuffer[13],ecu.id,12))
    {
        memcpy(Date,&recvbuffer[28],8);
        memcpy(serial,&recvbuffer[36],3);
        APP_Response_AlarmEvent(0x00,Date,serial);
    }else
    {
        APP_Response_AlarmEvent(0x01,Date,serial);
    }
}



//WIFI�¼�����
void process_WIFI(void)
{
    //#ifdef WIFI_USE
    int ResolveFlag = 0,Data_Len = 0,Command_Id = 0;
    ResolveFlag =  Resolve_RecvData((char *)WIFI_RecvSocketAData,&Data_Len,&Command_Id);
    if(ResolveFlag == 0)
    {
        add_Phone_functions();
        if(pfun_Phone[Command_Id])
        {
            (*pfun_Phone[Command_Id])(Data_Len,(char *)WIFI_RecvSocketAData);
        }

    }
    //#endif
}

//������ʼ�������¼�����
void process_KEYEvent(void)
{
    int ret =0,i = 0;
    printf("KEY_FormatWIFI_Event Start\n");

    for(i = 0;i<3;i++)
    {

        ret = WIFI_Factory_Passwd();
        if(ret == 0) break;
    }

    if(ret == 0) 	//д��WIFI����
    {
        key_init();
        set_Passwd("88888888",8);	//WIFI����
    }


    printf("KEY_FormatWIFI_Event End\n");
}


//���߸�λ����
int process_WIFI_RST(void)
{
    if(AT_RST() != 0)
    {
        WIFI_Reset();
    }

    if(APSTA_Status == 1)
    {
        AT_CWMODE3(3);
        AT_CIPMUX1();
        AT_CIPSERVER();
        AT_CIPSTO();
    }else
    {
        AT_CWMODE3(1);
        AT_CIPMUX1();
    }
    return 0;
}

void process_WIFIEvent_ESP07S(void)
{
    rt_mutex_take(wifi_uart_lock, RT_WAITING_FOREVER);
    //���WIFI�¼�
    WIFI_GetEvent_ESP07S();
    rt_mutex_release(wifi_uart_lock);
    //�ж��Ƿ���WIFI�����¼�
    if(WIFI_Recv_SocketA_Event == 1)
    {
        process_WIFI();
        WIFI_Recv_SocketA_Event = 0;
    }
}


static void APSTATimeout(void* parameter)
{
    //�л�ΪAP+STAģʽ
    APSTA_Status = 0;
    switchSTAModeFlag = 1;
}

void process_switchSTAMode(void)
{
    if(switchSTAModeFlag == 1)
    {
        printf("STA\n");
        AT_CWMODE3(1);
        AT_CIPMUX1();
        switchSTAModeFlag = 0;
    }
}

//�л�ΪAP+STAģʽ��1Сʱ���л�ΪSTAģʽ
void process_APKEYEvent(void)
{
    //�л���AP+STAģʽ
    rt_thread_delay(RT_TICK_PER_SECOND*2);
    AT_CWMODE3(3);
    AT_CIPMUX1();
    AT_CIPSERVER();
    AT_CIPSTO();
    printf("AP\n");
    APSTA_Status = 1;

    if(APSTAtimer == RT_NULL)
    {
        //��ʱ��1Сʱ���л�ΪSTAģʽ
        APSTAtimer = rt_timer_create("APSTA",
                                     APSTATimeout,
                                     RT_NULL,
                                     3600*RT_TICK_PER_SECOND,
                                     RT_TIMER_FLAG_ONE_SHOT);
    }

    if (APSTAtimer != RT_NULL)
    {
        rt_timer_stop(APSTAtimer);
        rt_timer_start(APSTAtimer);
    }

}
/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   Phone Server Application program entry                                  */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   parameter  unused                                                       */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
void phone_server_thread_entry(void* parameter)
{
    int ret = 0;
    char buff[50] = {0x00};
    IPConfig_t IPconfig;

    get_ecuid(ecu.id);
    get_mac((unsigned char*)ecu.MacAddress);			//ECU ����Mac��ַ
    rt_thread_delay(RT_TICK_PER_SECOND*START_TIME_PHONE_SERVER);
    add_Phone_functions();
    ReadPage(INTERNAL_FLASH_IPCONFIG,buff,21);
    if(buff[0] == '1')
    {
        getAddr(&buff[1],&IPconfig);
        StaticIP(IPconfig.IPAddr,IPconfig.MSKAddr,IPconfig.GWAddr,IPconfig.DNS1Addr,IPconfig.DNS2Addr);
    }
#if 1
    AT_CWMODE3(1);
    AT_CIPMUX1();
#else 
    AT_CWMODE3(3);
    AT_CIPMUX1();
    AT_CIPSERVER();
    AT_CIPSTO();
#endif
    while(1)
    {
        //����
        rt_mutex_take(wifi_uart_lock, RT_WAITING_FOREVER);
        //��ȡWIFI�¼�
        process_WIFIEvent_ESP07S();
        //����
        rt_mutex_release(wifi_uart_lock);

        //��ⰴ���¼�
        if(KEY_FormatWIFI_Event == 1)
        {
            process_KEYEvent();
            KEY_FormatWIFI_Event = 0;
        }

        if(APKEY_EVENT == 1)
        {
            process_APKEYEvent();	//�л���AP+STAģʽ����ʱ1Сʱ�л���STAģʽ
            APKEY_EVENT = 0;
        }

        //WIFI��λ�¼�
        if(WIFI_RST_Event == 1)
        {
            ret = process_WIFI_RST();
            if(ret == 0)
                WIFI_RST_Event = 0;
        }
        process_switchSTAMode();
        rt_thread_delay(RT_TICK_PER_SECOND/100);
    }

}
