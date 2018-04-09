#include "wifi_comm.h"
#include "string.h"
#include "variation.h"
#include "stdio.h"
#include "version.h"
#include "file.h"
#include "rtc.h"
#include "usart5.h"
#include "stdlib.h"
#include "threadlist.h"

extern ecu_info ecu;
static char SendData[MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER] = {'\0'};
extern unsigned char rateOfProgress;

int phone_add_inverter(int num,const char *uidstring)
{
	int i = 0;
	char buff[25] = { '\0' };
	char *allbuff = NULL;
	allbuff = malloc(2500);
	memset(allbuff,0x00,2500);
	for(i = 0; i < num; i++)
	{
		memset(buff,'\0',25);
		sprintf(buff,"%02x%02x%02x%02x%02x%02x,,,,,,\n",uidstring[0+i*6],uidstring[1+i*6],uidstring[2+i*6],uidstring[3+i*6],uidstring[4+i*6],uidstring[5+i*6]);
		memcpy(&allbuff[0+19*i],buff,19);
	}
	
	
	echo("/home/data/id",allbuff);
	echo("/yuneng/limiteid.con","1");
	free(allbuff);
	allbuff = NULL;
	return 0;
}

//解析报文长度
unsigned short packetlen(unsigned char *packet)
{
	unsigned short len = 0;
	len = ((packet[0]-'0')*1000 +(packet[1]-'0')*100 + (packet[2]-'0')*10 + (packet[3]-'0'));
	return len;
}

//解析收到的数据
int Resolve_RecvData(char *RecvData,int* Data_Len,int* Command_Id)
{
	//APS
	if(strncmp(RecvData, "APS", 3))
		return -1;
	//版本号 
	//长度
	*Data_Len = packetlen((unsigned char *)&RecvData[5]);
	//ID
	*Command_Id = (RecvData[9]-'0')*1000 + (RecvData[10]-'0')*100 + (RecvData[11]-'0')*10 + (RecvData[12]-'0');
	return 0;
}

int Resolve_Server(ECUServerInfo_t *serverInfo)
{
	//如果serverCmdType 为获取指令，解析对应文件到结构体中
	char IP_Str[20] = {'\0'};
	char temp[4] = {'\0'};
	unsigned char IP_Str_site = 0;
	unsigned char site = 0;
	int i = 0;
	FILE *fp;
	char *buff = NULL;
	buff = malloc(512);
	memset(buff,'\0',512);
	if(serverInfo->serverCmdType == SERVER_UPDATE_GET)
	{
		//初始化电量上传参数
		fp = fopen("/yuneng/FTPADD.CON", "r");
		if(fp)
		{
			while(1)
			{
				memset(buff, '\0', 512);
				fgets(buff, 512, fp);
				if(!strlen(buff))
					break;
				if(!strncmp(buff, "Domain", 6))
				{
					memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
					strcpy(serverInfo->domain, &buff[7]);
					if('\n' == serverInfo->domain[strlen(serverInfo->domain)-1])
					{
						serverInfo->domain[strlen(serverInfo->domain)-1] = '\0';
					}
						
				}
				if(!strncmp(buff, "IP", 2))
				{
					strcpy(IP_Str, &buff[3]);
					if('\n' == IP_Str[strlen(IP_Str)-1])
						IP_Str[strlen(IP_Str)-1] = '\0';
					//解析到serverInfo中去
					for(i = 0;i<(strlen(IP_Str)+1);i++)
					{
						if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
						{
							memset(temp,'\0',4);
							memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
							serverInfo->IP[site] = atoi(temp);
							IP_Str_site = i+1;
							site++;
						}
					}
					
				}
				if(!strncmp(buff, "Port", 4))
					serverInfo->Port1=atoi(&buff[5]);

				serverInfo->Port2=0;

			}
			fclose(fp);
		}
		else
		{
			memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
			memcpy(serverInfo->domain,UPDATE_SERVER_DOMAIN,strlen(UPDATE_SERVER_DOMAIN));
			serverInfo->IP[0] = 60;
			serverInfo->IP[1] = 190;
			serverInfo->IP[2] = 131;
			serverInfo->IP[3] = 190;
			serverInfo->Port1 = UPDATE_SERVER_PORT1;
			serverInfo->Port2 = 0;			
		}
	}else if(serverInfo->serverCmdType == SERVER_CLIENT_GET)
	{
		//初始化电量上传参数
		fp = fopen("/yuneng/datacent.con", "r");
		if(fp)
		{
			while(1)
			{
				memset(buff, '\0', 512);
				fgets(buff, 512, fp);
				if(!strlen(buff))
					break;
				if(!strncmp(buff, "Domain", 6))
				{
					memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
					strcpy(serverInfo->domain, &buff[7]);
					if('\n' == serverInfo->domain[strlen(serverInfo->domain)-1])
					{
						serverInfo->domain[strlen(serverInfo->domain)-1] = '\0';
					}
						
				}
				if(!strncmp(buff, "IP", 2))
				{
					strcpy(IP_Str, &buff[3]);
					if('\n' == IP_Str[strlen(IP_Str)-1])
						IP_Str[strlen(IP_Str)-1] = '\0';
					//解析到serverInfo中去
					for(i = 0;i<(strlen(IP_Str)+1);i++)
					{
						if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
						{
							memset(temp,'\0',4);
							memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
							serverInfo->IP[site] = atoi(temp);
							IP_Str_site = i+1;
							site++;
						}
					}
					
				}
				if(!strncmp(buff, "Port1", 5))
					serverInfo->Port1=atoi(&buff[6]);
				if(!strncmp(buff, "Port2", 5))
					serverInfo->Port2=atoi(&buff[6]);
			}
			fclose(fp);	
		}
		else
		{
			memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
			memcpy(serverInfo->domain,CLIENT_SERVER_DOMAIN,strlen(CLIENT_SERVER_DOMAIN));
			serverInfo->IP[0] = 60;
			serverInfo->IP[1] = 190;
			serverInfo->IP[2] = 131;
			serverInfo->IP[3] = 190;
			serverInfo->Port1 = CLIENT_SERVER_PORT1;
			serverInfo->Port2 = CLIENT_SERVER_PORT2;			
		}
		
	}else if(serverInfo->serverCmdType == SERVER_CONTROL_GET)
	{
		//初始化电量上传参数
		fp = fopen("/yuneng/CONTROL.CON", "r");
		if(fp)
		{
			while(1)
			{
				memset(buff, '\0', 512);
				fgets(buff, 512, fp);
				if(!strlen(buff))
					break;
				if(!strncmp(buff, "Domain", 6))
				{
					memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
					strcpy(serverInfo->domain, &buff[7]);
					if('\n' == serverInfo->domain[strlen(serverInfo->domain)-1])
					{
						serverInfo->domain[strlen(serverInfo->domain)-1] = '\0';
					}
						
				}
				if(!strncmp(buff, "IP", 2))
				{
					strcpy(IP_Str, &buff[3]);
					if('\n' == IP_Str[strlen(IP_Str)-1])
						IP_Str[strlen(IP_Str)-1] = '\0';
					//解析到serverInfo中去
					for(i = 0;i<(strlen(IP_Str)+1);i++)
					{
						if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
						{
							memset(temp,'\0',4);
							memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
							serverInfo->IP[site] = atoi(temp);
							IP_Str_site = i+1;
							site++;
						}
					}
					
				}
				if(!strncmp(buff, "Port1", 5))
					serverInfo->Port1=atoi(&buff[6]);
				if(!strncmp(buff, "Port2", 5))
					serverInfo->Port2=atoi(&buff[6]);

			}
			fclose(fp);
		}
		else
		{
			memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
			memcpy(serverInfo->domain,CONTROL_SERVER_DOMAIN,strlen(CONTROL_SERVER_DOMAIN));
			serverInfo->IP[0] = 60;
			serverInfo->IP[1] = 190;
			serverInfo->IP[2] = 131;
			serverInfo->IP[3] = 190;
			serverInfo->Port1 = CONTROL_SERVER_PORT1;
			serverInfo->Port2 = CONTROL_SERVER_PORT2;			
		}
	}

	free(buff);
	buff = NULL;
	
	return 0;
	
}

int Save_Server(ECUServerInfo_t *serverInfo)
{	
	char *buff = NULL;
	buff = malloc(512);
	memset(buff,'\0',512);
	printf("%d\n",serverInfo->serverCmdType);
	printf("%s\n",serverInfo->domain);
	printf("%d,%d,%d,%d,\n",serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3]);
	printf("%d\n",serverInfo->Port1);
	printf("%d\n",serverInfo->Port2);
	if(serverInfo->serverCmdType == SERVER_UPDATE_SET)
	{
		sprintf(buff,"Domain=%s\nIP=%d.%d.%d.%d\nPort=%d\nuser=zhyf\npassword=yuneng\n",serverInfo->domain,serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3],serverInfo->Port1);
		echo("/yuneng/ftpadd.con", buff);
	}else if(serverInfo->serverCmdType == SERVER_CLIENT_SET)
	{
		sprintf(buff,"Domain=%s\nIP=%d.%d.%d.%d\nPort1=%d\nPort2=%d\n",serverInfo->domain,serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3],serverInfo->Port1,serverInfo->Port2);
		echo("/yuneng/datacent.con",buff);
	}else if(serverInfo->serverCmdType == SERVER_CONTROL_SET)
	{
		sprintf(buff,"Timeout=10\nReport_Interval=15\nDomain=%s\nIP=%d.%d.%d.%d\nPort1=%d\nPort2=%d\n",serverInfo->domain,serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3],serverInfo->Port1,serverInfo->Port2);
		echo("/yuneng/control.con",buff);
	}
	free(buff);
	buff = NULL;
	
	return 0;
}

//01 COMMAND_BASEINFO 					//获取基本信息请求
void APP_Response_BaseInfo(stBaseInfo baseInfo)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	//拼接需要发送的报文
	sprintf(SendData,"APS1100000001%s",baseInfo.ECUID);
	packlength = 25;

	SendData[packlength++] = '0';
	SendData[packlength++] = '1';
	
	SendData[packlength++] = (baseInfo.LifttimeEnergy/16777216)%256;
	SendData[packlength++] = (baseInfo.LifttimeEnergy/65536)%256;
	SendData[packlength++] = (baseInfo.LifttimeEnergy/256)%256;
	SendData[packlength++] =  baseInfo.LifttimeEnergy%256;

	SendData[packlength++] = (baseInfo.LastSystemPower/16777216)%256;
	SendData[packlength++] = (baseInfo.LastSystemPower/65536)%256;
	SendData[packlength++] = (baseInfo.LastSystemPower/256)%256;
	SendData[packlength++] = baseInfo.LastSystemPower%256;

	SendData[packlength++] = (baseInfo.GenerationCurrentDay/16777216)%256;
	SendData[packlength++] = (baseInfo.GenerationCurrentDay/65536)%256;
	SendData[packlength++] = (baseInfo.GenerationCurrentDay/256)%256;
	SendData[packlength++] = baseInfo.GenerationCurrentDay%256;
	
	
	memcpy(&SendData[packlength],baseInfo.LastToEMA,7);
	packlength += 7;
	
	SendData[packlength++] = baseInfo.InvertersNum/256;
	SendData[packlength++] = baseInfo.InvertersNum%256;
	
	SendData[packlength++] = baseInfo.LastInvertersNum/256;
	SendData[packlength++] = baseInfo.LastInvertersNum%256;

	SendData[packlength++] = baseInfo.Channel[0];
	SendData[packlength++] = baseInfo.Channel[1];
	
	SendData[packlength++] = '0' + (ECU_VERSION_LENGTH/100)%10;
	SendData[packlength++] = '0'+ (ECU_VERSION_LENGTH/10)%10;
	SendData[packlength++] = '0'+ (ECU_VERSION_LENGTH%10);
	sprintf(&SendData[packlength],"%s_%s.%s",ECU_VERSION,MAJORVERSION,MINORVERSION);
	packlength += ECU_VERSION_LENGTH;
	
	SendData[packlength++] = '0';
	SendData[packlength++] = '0';
	SendData[packlength++] = '9';
	sprintf(&SendData[packlength],"Etc/GMT-8");
	packlength += 9;	
	
	memcpy(&SendData[packlength],baseInfo.MacAddress,6);
	packlength += 6;
	
	memset(baseInfo.WifiMac,0x00,6);
	memcpy(&SendData[packlength],baseInfo.WifiMac,6);
	packlength += 6;
	
	SendData[packlength++] = 'E';
	SendData[packlength++] = 'N';
	SendData[packlength++] = 'D';
	SendData[5] = (packlength/1000) + '0';
	SendData[6] = ((packlength/100)%10) + '0';
	SendData[7] = ((packlength/10)%10) + '0';
	SendData[8] = ((packlength)%10) + '0';
	SendData[packlength++] = '\n';
	SendToSocketA(SendData ,packlength);
}

//02 COMMAND_POWERGENERATION		//逆变器发电数据请求  mapping :: 0x00 匹配  0x01 不匹配
void APP_Response_PowerGeneration(char mapping,inverter_info *inverter,int VaildNum)
{
	int packlength = 0,index = 0;
	inverter_info *curinverter = inverter;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//匹配不成功
	if(mapping == 0x01)
	{
		sprintf(SendData,"APS110015000201\n");
		packlength = 16;
		SendToSocketA(SendData ,packlength);
		return ;
	}
	
	//匹配成功 
	if(ecu.had_data_broadcast_time[0] == '\0')
	{
		sprintf(SendData,"APS110015000202\n");
		packlength = 16;
		SendToSocketA(SendData ,packlength);
		return ;
	}
	
	//拼接需要发送的报文
	sprintf(SendData,"APS110000000200");
	packlength = 15;

	SendData[packlength++] = '0';
	SendData[packlength++] = '1';
	
	SendData[packlength++] = VaildNum/256;
	SendData[packlength++] = VaildNum%256;
	
	SendData[packlength++] = (ecu.had_data_broadcast_time[0] - '0')*16+(ecu.had_data_broadcast_time[1] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[2] - '0')*16+(ecu.had_data_broadcast_time[3] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[4] - '0')*16+(ecu.had_data_broadcast_time[5] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[6] - '0')*16+(ecu.had_data_broadcast_time[7] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[8] - '0')*16+(ecu.had_data_broadcast_time[9] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[10] - '0')*16+(ecu.had_data_broadcast_time[11] - '0');
	SendData[packlength++] = (ecu.had_data_broadcast_time[12] - '0')*16+(ecu.had_data_broadcast_time[13] - '0');
		
	for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
	{
		if((curinverter->model == 0x17))
		{
			//UID
			SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
			SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
			SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
			SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
			SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
			SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

			SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

			//逆变器类型
			SendData[packlength++] = '0';
			SendData[packlength++] = '3';
			
			//电网频率
			SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
			SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

			//机内温度
			SendData[packlength++] = (curinverter->it + 100) /256;
			SendData[packlength++] = (curinverter->it + 100) %256;		
			
			//逆变器功率  A 
			SendData[packlength++] = curinverter->op / 256;
			SendData[packlength++] = curinverter->op % 256;
				
			//电网电压    A
			SendData[packlength++] = curinverter->gv / 256;
			SendData[packlength++] = curinverter->gv % 256;
			
			//逆变器功率  B 
			SendData[packlength++] = curinverter->opb / 256;
			SendData[packlength++] = curinverter->opb % 256;

			//逆变器功率  C 
			SendData[packlength++] = curinverter->opc / 256;
			SendData[packlength++] = curinverter->opc % 256;

			//逆变器功率  D
			SendData[packlength++] = curinverter->opd / 256;
			SendData[packlength++] = curinverter->opd % 256;

			
		}else if((curinverter->model == 7))
		{
			//UID
			SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
			SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
			SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
			SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
			SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
			SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

			SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

			//逆变器类型
			SendData[packlength++] = '0';
			SendData[packlength++] = '1';
			
			//电网频率
			SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
			SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

			//机内温度
			SendData[packlength++] = (curinverter->it + 100) /256;
			SendData[packlength++] = (curinverter->it + 100) %256;		
			
			//逆变器功率  A 
			SendData[packlength++] = curinverter->op / 256;
			SendData[packlength++] = curinverter->op % 256;;
				
			//电网电压    A
			SendData[packlength++] = curinverter->gv / 256;
			SendData[packlength++] = curinverter->gv % 256;
			
			//逆变器功率  B 
			SendData[packlength++] = curinverter->opb / 256;
			SendData[packlength++] = curinverter->opb % 256;
				
			//电网电压    B
			SendData[packlength++] = curinverter->gv / 256;
			SendData[packlength++] = curinverter->gv % 256;
		}else if((curinverter->model == 5) || (curinverter->model == 6))
		{
			//UID
			SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
			SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
			SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
			SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
			SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
			SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

			SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

			//逆变器类型
			SendData[packlength++] = '0';
			SendData[packlength++] = '2';
			
			//电网频率
			SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
			SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

			//机内温度
			SendData[packlength++] = (curinverter->it + 100) /256;
			SendData[packlength++] = (curinverter->it + 100) %256;		
			
			//逆变器功率  A 
			SendData[packlength++] = curinverter->op / 256;
			SendData[packlength++] = curinverter->op % 256;;
				
			//电网电压    A
			SendData[packlength++] = curinverter->gv / 256;
			SendData[packlength++] = curinverter->gv % 256;
			
			//逆变器功率  B 
			SendData[packlength++] = curinverter->opb / 256;
			SendData[packlength++] = curinverter->opb % 256;
				
			//电网电压    B
			SendData[packlength++] = curinverter->gvb / 256;
			SendData[packlength++] = curinverter->gvb % 256;


			//逆变器功率  C 
			SendData[packlength++] = curinverter->opc / 256;
			SendData[packlength++] = curinverter->opc % 256;
				
			//电网电压    C
			SendData[packlength++] = curinverter->gvc / 256;
			SendData[packlength++] = curinverter->gvc % 256;

			//逆变器功率  D 
			SendData[packlength++] = curinverter->opd / 256;
			SendData[packlength++] = curinverter->opd % 256;
				
		}else
		{
			//UID
			SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
			SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
			SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
			SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
			SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
			SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

			SendData[packlength++] = 0;
			
			//逆变器类型
			SendData[packlength++] = '0';
			SendData[packlength++] = '0';
		}
		
		
	}
	
	SendData[packlength++] = 'E';
	SendData[packlength++] = 'N';
	SendData[packlength++] = 'D';
	
	SendData[5] = (packlength/1000) + '0';
	SendData[6] = ((packlength/100)%10) + '0';
	SendData[7] = ((packlength/10)%10) + '0';
	SendData[8] = ((packlength)%10) + '0';
	SendData[packlength++] = '\n';
	SendToSocketA(SendData ,packlength);
}

//03 COMMAND_POWERCURVE					//功率曲线请求   mapping :: 0x00 匹配  0x01 不匹配   data 表示日期
void APP_Response_PowerCurve(char mapping,char * date)
{
	int packlength = 0,length = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//匹配不成功
	if(mapping == 0x01)
	{
		sprintf(SendData,"APS110015000301\n");
		packlength = 16;
		SendToSocketA(SendData ,packlength);
		return ;
	}

	//拼接需要发送的报文
	sprintf(SendData,"APS110015000300");
	packlength = 15;
	
	read_system_power(date,&SendData[15],&length);
	packlength += length;
	
	SendData[packlength++] = 'E';
	SendData[packlength++] = 'N';
	SendData[packlength++] = 'D';
	
	SendData[5] = (packlength/1000) + '0';
	SendData[6] = ((packlength/100)%10) + '0';
	SendData[7] = ((packlength/10)%10) + '0';
	SendData[8] = ((packlength)%10) + '0';
	SendData[packlength++] = '\n';
	
	SendToSocketA(SendData ,packlength);
}

//04 COMMAND_GENERATIONCURVE		//发电量曲线请求    mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_GenerationCurve(char mapping,char request_type)
{
	int packlength = 0,len_body = 0;
	char date_time[15] = { '\0' };
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	apstime(date_time);
	//匹配不成功
	if(mapping == 0x01)
	{
		sprintf(SendData,"APS110015000401\n");
		packlength = 16;
		SendToSocketA(SendData ,packlength);
		return ;
	}

	sprintf(SendData,"APS110015000400");
	packlength = 15;
	//拼接需要发送的报文
	if(request_type == '0')
	{//最近一周
		SendData[packlength++] = '0';
		SendData[packlength++] = '0';
		
		read_weekly_energy(date_time, &SendData[packlength],&len_body);
		packlength += len_body;
		
	}else if(request_type == '1')
	{//最近一个月
		SendData[packlength++] = '0';
		SendData[packlength++] = '1';
		read_monthly_energy(date_time, &SendData[packlength],&len_body);
		packlength += len_body;
		
	}else if(request_type == '2')
	{//最近一年
		SendData[packlength++] = '0';
		SendData[packlength++] = '2';
		read_yearly_energy(date_time, &SendData[packlength],&len_body);
		packlength += len_body;
		
	}

	SendData[packlength++] = 'E';
	SendData[packlength++] = 'N';
	SendData[packlength++] = 'D';
	
	SendData[5] = (packlength/1000) + '0';
	SendData[6] = ((packlength/100)%10) + '0';
	SendData[7] = ((packlength/10)%10) + '0';
	SendData[8] = ((packlength)%10) + '0';
	SendData[packlength++] = '\n';
	
	SendToSocketA(SendData ,packlength);
}

//05 COMMAND_REGISTERID 				//逆变器ID注册请求 		mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_RegisterID(char mapping)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//拼接需要发送的报文
	sprintf(SendData,"APS1100150005%02d\n",mapping);
	packlength = 16;
	
	SendToSocketA(SendData ,packlength);
}

//06 COMMAND_SETTIME						//时间设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetTime(char mapping)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//拼接需要发送的报文
	sprintf(SendData,"APS1100150006%02d\n",mapping);
	packlength = 16;
	
	SendToSocketA(SendData ,packlength);
}

//07 COMMAND_SETWIREDNETWORK		//有线网络设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetWiredNetwork(char mapping)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//拼接需要发送的报文
	sprintf(SendData,"APS1100150007%02d\n",mapping);
	packlength = 16;
	
	SendToSocketA(SendData ,packlength);
}


//获取硬件信息
void APP_Response_GetECUHardwareStatus(char mapping)
{
	int packlength = 0;
	
	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110120000800%02d",WIFI_MODULE_TYPE);
		memset(&SendData[17],'0',100);
		SendData[117] = 'E';
		SendData[118] = 'N';
		SendData[119] = 'D';
		SendData[120] = '\n';
		packlength = 121;
	}else
	{
		sprintf(SendData,"APS110015000801\n");
		packlength = 16;
	}
	SendToSocketA(SendData ,packlength);

}


//10 COMMAND_SETWIFIPASSWD			//AP密码设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetWifiPasswd(char mapping)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	
	//拼接需要发送的报文
	sprintf(SendData,"APS1100150010%02d\n",mapping);
	packlength = 16;
	
	SendToSocketA(SendData ,packlength);
}

//11	AP密码设置请求
void APP_Response_GetIDInfo(char mapping,inverter_info *inverter)
{
	int packlength = 0,index = 0;
	inverter_info *curinverter = inverter;
	char uid[7];
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	

	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110015001100");
		packlength = 15;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
		{
			
			uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
			uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
			uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
			uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
			uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
			uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
			//printf("%02x%02x%02x%02x%02x%02x\n",uid[0],uid[1],uid[2],uid[3],uid[4],uid[5]);
			memcpy(&SendData[packlength],uid,6);	
			packlength += 6;
		}
		
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';

		
	}else
	{
		sprintf(SendData,"APS110015001101\n");
		packlength = 16;
	}		
	SendToSocketA(SendData ,packlength);

}
//12	AP密码设置请求
void APP_Response_GetTime(char mapping,char *Time)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110032001200%sEND\n",Time);
		packlength = 33;
	}else
	{
		sprintf(SendData,"APS110015001201\n");
		packlength = 16;
	}	
	
	SendToSocketA(SendData ,packlength);

}


void APP_Response_FlashSize(char mapping,unsigned int Flashsize)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110000001300");
		packlength = 15;
		//printf("%d\n",Flashsize);
		SendData[packlength++] = (Flashsize/16777216)%256;
		SendData[packlength++] = (Flashsize/65536)%256;
		SendData[packlength++] = (Flashsize/256)%256;
		SendData[packlength++] = Flashsize%256;
		
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';
		
	}else
	{
		sprintf(SendData,"APS110015001301\n");
		packlength = 16;
	}	
	
	SendToSocketA(SendData ,packlength);

}


//14 COMMAND_SETWIREDNETWORK		//有线网络设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_GetWiredNetwork(char mapping,char dhcpStatus,IP_t IPAddr,IP_t MSKAddr,IP_t GWAddr,IP_t DNS1Addr,IP_t DNS2Addr)
{
	int packlength = 0;
	char MAC[13] = {'\0'};
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);	
	if(mapping == 0x00)
	{
		//拼接需要发送的报文
		sprintf(SendData,"APS1100150014%02d\n",mapping);
		packlength = 15;
		if(dhcpStatus == 0)
		{
			SendData[packlength++] = '0';
			SendData[packlength++] = '0';
		}else
		{
			SendData[packlength++] = '0';
			SendData[packlength++] = '1';
		}
		SendData[packlength++] = IPAddr.IP1;
		SendData[packlength++] = IPAddr.IP2;
		SendData[packlength++] = IPAddr.IP3;
		SendData[packlength++] = IPAddr.IP4;
				
		SendData[packlength++] = MSKAddr.IP1;
		SendData[packlength++] = MSKAddr.IP2;
		SendData[packlength++] = MSKAddr.IP3;
		SendData[packlength++] = MSKAddr.IP4;
		
		SendData[packlength++] = GWAddr.IP1;
		SendData[packlength++] = GWAddr.IP2;
		SendData[packlength++] = GWAddr.IP3;
		SendData[packlength++] = GWAddr.IP4;

		SendData[packlength++] = DNS1Addr.IP1;
		SendData[packlength++] = DNS1Addr.IP2;
		SendData[packlength++] = DNS1Addr.IP3;
		SendData[packlength++] = DNS1Addr.IP4;

		SendData[packlength++] = DNS2Addr.IP1;
		SendData[packlength++] = DNS2Addr.IP2;
		SendData[packlength++] = DNS2Addr.IP3;
		SendData[packlength++] = DNS2Addr.IP4;
		sprintf(MAC,"%02x%02x%02x%02x%02x%02x",ecu.MacAddress[0],ecu.MacAddress[1],ecu.MacAddress[2],ecu.MacAddress[3],ecu.MacAddress[4],ecu.MacAddress[5]);
		memcpy(&SendData[packlength],MAC,12);
		packlength += 12;

		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';
		SendToSocketA(SendData ,packlength);
	}else
	{
		sprintf(SendData,"APS110015001401\n");
		packlength = 16;
		SendToSocketA(SendData ,packlength);
	}
	
}


void APP_Response_SetChannel(unsigned char mapflag,char SIGNAL_CHANNEL,char SIGNAL_LEVEL)
{
	//char SendData[22] = {'\0'};
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
	if(mapflag == 1)
	{
		sprintf(SendData,"APS110015001501\n");
		SendToSocketA(SendData ,16);
	}else{
		sprintf(SendData,"APS110023001500%02x%03dEND\n",SIGNAL_CHANNEL,SIGNAL_LEVEL);		
		SendToSocketA(SendData ,24);
	}
}

void APP_Response_GetShortAddrInfo(char mapping,inverter_info *inverter)
{
	int packlength = 0,index = 0;
	inverter_info *curinverter = inverter;
	char uid[7];
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110015001800");
		packlength = 15;
		SendData[packlength++] = rateOfProgress;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
		{
			
			uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
			uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
			uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
			uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
			uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
			uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
			memcpy(&SendData[packlength],uid,6);	
			packlength += 6;
			SendData[packlength++] = curinverter->shortaddr/256;
			SendData[packlength++] = curinverter->shortaddr%256;
		}
		
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';

		
	}else
	{
		sprintf(SendData,"APS110015001801\n");
		packlength = 16;
	}		
	SendToSocketA(SendData ,packlength);

}

void APP_Response_GetECUAPInfo(char mapping,unsigned char connectStatus,char *info)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

	if(mapping == 0x00)
	{
		if(0 == connectStatus)
		{
			sprintf(SendData,"APS110019002000%1dEND\n",connectStatus);
			packlength = 20;
		}else
		{
			sprintf(SendData,"APS11%04d002000%1d%sEND\n",(strlen(info) + 19),connectStatus,info);
			packlength = (strlen(info) + 20);
			printf("sendData:%s\n",SendData);
		}
		
	}else
	{
		sprintf(SendData,"APS110015002001\n");
		packlength = 16;
	}	
	SendToSocketA(SendData ,packlength);

}


//ECU-RS设置WIFI密码
void APP_Response_SetECUAPInfo(unsigned char result)
{
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
	sprintf(SendData,"APS1100150021%02d\n",result);
	SendToSocketA(SendData ,16);
}


void APP_Response_GetECUAPList(char mapping,char *list)
{
	int packlength = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

	if(mapping == 0x00)
	{
		sprintf(SendData,"APS11%04d002200%sEND\n",(strlen(list) + 18),list);
		packlength = (strlen(list) + 19);		
	}else
	{
		sprintf(SendData,"APS110015002201\n");
		packlength = 16;
	}	
	
	SendToSocketA(SendData ,packlength);
}

void APP_Response_GetFunctionStatusInfo(char mapping)
{
	int packlength = 0;
		memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
	

	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110015002300");
		packlength = 15;
		SendData[packlength++] = '0';
		memset(&SendData[packlength],'0',300);
		packlength += 300;
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';

		
	}else
	{
		sprintf(SendData,"APS110015002301\n");
		packlength = 16;
	}		
	SendToSocketA(SendData ,packlength);
}


void APP_Response_ServerInfo(char mapping,ECUServerInfo_t *serverInfo)
{
	int packlength = 0;
	int domain_len = 0;
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
	
	if(mapping == 0x00)
	{
		Resolve_Server(serverInfo);
		domain_len = strlen(serverInfo->domain);
		if(serverInfo->serverCmdType == SERVER_UPDATE_GET)
		{	//从文件中读取对应的信息
			sprintf(SendData,"APS1100150024%02d00%03d",SERVER_UPDATE_GET,domain_len);
			packlength = 20;
			//域名
			memcpy(&SendData[packlength],serverInfo->domain,domain_len);
			packlength += domain_len;
			//IP
			SendData[packlength++] = serverInfo->IP[0];
			SendData[packlength++] = serverInfo->IP[1];
			SendData[packlength++] = serverInfo->IP[2];
			SendData[packlength++] = serverInfo->IP[3];
			//Port1
			SendData[packlength++] = serverInfo->Port1/256;
			SendData[packlength++] = serverInfo->Port1%256;
			//Port2
			SendData[packlength++] = serverInfo->Port2/256;
			SendData[packlength++] = serverInfo->Port2%256;
			//Port3
			SendData[packlength++] = 0;
			SendData[packlength++] = 0;
			
			
		}else if(serverInfo->serverCmdType == SERVER_CLIENT_GET)
		{
			sprintf(SendData,"APS1100150024%02d00%03d",SERVER_CLIENT_GET,domain_len);
			packlength = 20;
			memcpy(&SendData[packlength],serverInfo->domain,domain_len);
			packlength += domain_len;
			//IP
			SendData[packlength++] = serverInfo->IP[0];
			SendData[packlength++] = serverInfo->IP[1];
			SendData[packlength++] = serverInfo->IP[2];
			SendData[packlength++] = serverInfo->IP[3];
			//Port1
			SendData[packlength++] = serverInfo->Port1/256;
			SendData[packlength++] = serverInfo->Port1%256;
			//Port2
			SendData[packlength++] = serverInfo->Port2/256;
			SendData[packlength++] = serverInfo->Port2%256;
			//Port3
			SendData[packlength++] = 0;
			SendData[packlength++] = 0;
		}else if(serverInfo->serverCmdType == SERVER_CONTROL_GET)
		{
			sprintf(SendData,"APS1100150024%02d00%03d",SERVER_CONTROL_GET,domain_len);
			packlength = 20;
			memcpy(&SendData[packlength],serverInfo->domain,domain_len);
			packlength += domain_len;
			//IP
			SendData[packlength++] = serverInfo->IP[0];
			SendData[packlength++] = serverInfo->IP[1];
			SendData[packlength++] = serverInfo->IP[2];
			SendData[packlength++] = serverInfo->IP[3];
			//Port1
			SendData[packlength++] = serverInfo->Port1/256;
			SendData[packlength++] = serverInfo->Port1%256;
			//Port2
			SendData[packlength++] = serverInfo->Port2/256;
			SendData[packlength++] = serverInfo->Port2%256;
			//Port3
			SendData[packlength++] = 0;
			SendData[packlength++] = 0;
		}else if(serverInfo->serverCmdType == SERVER_UPDATE_SET)
		{
			sprintf(SendData,"APS1100150024%02d00",SERVER_UPDATE_SET);
			Save_Server(serverInfo);
			packlength = 17;
		}else if(serverInfo->serverCmdType == SERVER_CLIENT_SET)
		{
			sprintf(SendData,"APS1100150024%02d00",SERVER_CLIENT_SET);
			Save_Server(serverInfo);
			packlength = 17;
		}else if(serverInfo->serverCmdType == SERVER_CONTROL_SET)
		{
			sprintf(SendData,"APS1100150024%02d00",SERVER_CONTROL_SET);
			Save_Server(serverInfo);
			packlength = 17;
		}else
		{
			return;
		}
		
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';
		
	}else
	{
		sprintf(SendData,"APS1100170024%02d01\n",serverInfo->serverCmdType );
		packlength = 18;
	}		
	SendToSocketA(SendData ,packlength);
}


void APP_Response_RSSI(char mapping)
{
	int packlength = 0,index = 0;
	inverter_info *curinverter = inverter;
	char uid[7];
	memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
	
	if(mapping == 0x00)
	{
		sprintf(SendData,"APS110015003000");
		packlength = 15;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
		{
			
			uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
			uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
			uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
			uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
			uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
			uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
			memcpy(&SendData[packlength],uid,6);	
			packlength += 6;
			SendData[packlength++] = curinverter->shortaddr/256;
			SendData[packlength++] = curinverter->shortaddr%256;
		}
		
		SendData[packlength++] = 'E';
		SendData[packlength++] = 'N';
		SendData[packlength++] = 'D';
		
		SendData[5] = (packlength/1000) + '0';
		SendData[6] = ((packlength/100)%10) + '0';
		SendData[7] = ((packlength/10)%10) + '0';
		SendData[8] = ((packlength)%10) + '0';
		SendData[packlength++] = '\n';
		
	}else
	{
		sprintf(SendData,"APS110015003001\n");
		packlength = 16;
	}		
	SendToSocketA(SendData ,packlength);
}

