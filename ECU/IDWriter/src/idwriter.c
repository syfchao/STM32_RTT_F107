/*****************************************************************************/
/* File      : idwrite.c                                                     */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-04-05 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwip/sockets.h> 
#include <lwip/netdb.h> 
#include <stdio.h>
#include "debug.h"
#include "idwrite.h"
#include "rtc.h"
#include "inverter_id.h"
#include "threadlist.h"
#include "client.h"
#include "file.h"
#include <dfs_posix.h> 
#include "channel.h"
#include "led.h"
#include "version.h"
#include "debug.h"
#include "usart5.h"
#include "file.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/

#define SERVERPORT 4540
#define BACKLOG 2

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/

extern rt_mutex_t record_data_lock;
extern ecu_info ecu;

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/

int create_socket_idwrite(void)
{
	int sockfd;
	if(-1==(sockfd=socket(AF_INET,SOCK_STREAM,0))){
		printmsg(ECU_DBG_IDWRITE,"socket error");
		return -1;
	}
	printmsg(ECU_DBG_IDWRITE,"Create socket successfully!");
	return sockfd;
}

int bind_socket(int sockfd)
{
	struct sockaddr_in server_sockaddr;
	server_sockaddr.sin_family=AF_INET;
	server_sockaddr.sin_port=htons(SERVERPORT);
	server_sockaddr.sin_addr.s_addr=INADDR_ANY;
	memset(&(server_sockaddr.sin_zero),0x00,8);

	if(-1==bind(sockfd,(struct sockaddr *)&server_sockaddr,sizeof(struct sockaddr))){
		printmsg(ECU_DBG_IDWRITE,"bind error");
		return -1;
	}
	printmsg(ECU_DBG_IDWRITE,"Bind socket successfully!");
	return 0;
}

int listen_socket(int sockfd)
{
	if(-1==listen(sockfd,BACKLOG)){
		printmsg(ECU_DBG_IDWRITE,"listen error");
		return -1;
	}
	printmsg(ECU_DBG_IDWRITE,"Listen socket successfully!");
	return 0;
}

int accept_socket(int sockfd)
{
	int sin_size;
	int clientfd;
	struct sockaddr_in client_sockaddr;

	sin_size=sizeof(struct sockaddr_in);
	if(-1==(clientfd=accept(sockfd,(struct sockaddr *)&client_sockaddr,(socklen_t *)&sin_size))){
		printmsg(ECU_DBG_IDWRITE,"accept error");
		return -1;
	}

	return clientfd;
}

int recv_cmd(int fd_sock, char *readbuff)
{
	fd_set rd;
	struct timeval timeout;
	int recvbytes, readbytes = 0, res;
	char temp[16];

	while(1)
	{
		FD_ZERO(&rd);
		FD_SET(fd_sock, &rd);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		res = select(fd_sock+1, &rd, NULL, NULL, &timeout);
		if(res <= 0){
			//printmsg("Receive command timeout");
			return -1;
		}
		else{
			memset(readbuff, '\0', sizeof(readbuff));
			memset(temp, '\0', sizeof(temp));
			recvbytes = recv(fd_sock, readbuff, 200, 0);
			if(0 == recvbytes)
				return -1;
		
			readbytes += recvbytes;
			return readbytes;

		}
	}
}


int settime(char *datetime)
{
	return set_time(datetime);
}

int insertinverter(char *buff)
{

	FILE *fp;
	char id[16];
	int i;

	clear_id();
	for(i=0; i<(strlen(buff)+1)/13; i++)
	{

		strncpy(id, &buff[i*13], 12);
		id[12] = '\0';
		addInverter(id);
	}

	fp = fopen("/yuneng/autoflag.con", "w");
	fputs("0", fp);
	fclose(fp);
	fp = fopen("/yuneng/limiteid.con", "w");
	fputs("1", fp);
	fclose(fp);
	restartThread(TYPE_MAIN);

	return i;
}

int getrecord(char *record)
{
	char time[15];
	int flag;
	return search_readflag(record,time, &flag,'1');
}

int getevent(char *eve)
{
	FILE *fp;
	char data[200];
	char splitdata[3][32];
	int num = 0;
	
	memset(eve,'\0',sizeof(eve));
	fp = fopen("/home/record/event", "r");
	if(fp)
	{
		memset(data,0x00,200);
		
		while(NULL != fgets(data,200,fp))
		{
			memset(splitdata,0x00,3*32);
			splitString(data,splitdata);
			strcat(eve, splitdata[0]);	//12
			strcat(eve, splitdata[1]);	//14
			strcat(eve, splitdata[2]);	//24
			num++;
		}
		fclose(fp);
	}

	return num;
}

int clearrecord()
{
	unlink("/home/data/COLLECT.CON");
	unlink("/yuneng/AUTOFLAG.CON");

	clear_id();

	unlink("/home/record/event");

	echo("/home/data/ltpower","0.000000");	

	rm_dir("/home/record/data");
	rm_dir("/home/record/INVERSTA");
	rm_dir("/home/record/POWER");
	rm_dir("/home/record/ENERGY");
	unlink("/home/record/EVENT");

	rm_dir("/tmp");
	rm_dir("/home/data/PROC_RES");
	rm_dir("/home/data/IPROCRES");
	restartThread(TYPE_MAIN);
	return 0;
}

void idwrite_thread_entry(void* parameter)
{
	char recvbuff[200] = {'\0'};
	int sockfd,clientfd;
	FILE *fp;
	char mac[32] = {'\0'};
	char version[50] = {'\0'};
	char area[8] = {'\0'};
	char *record;
	char *eve;
	int row;
	char sendbuff[3];
	char gettime[14]={'\0'};
	char strportflag[2]={'\0'};
	int portflag = 0;
	int ret = 0;
	
	rt_thread_delay(START_TIME_IDWRITE * RT_TICK_PER_SECOND);
	sockfd=create_socket_idwrite();
	bind_socket(sockfd);
	listen_socket(sockfd);

	while(1){
		clientfd = accept_socket(sockfd);
		memset(recvbuff, '\0', sizeof(recvbuff));
		
		recv_cmd(clientfd, recvbuff);
		print2msg(ECU_DBG_IDWRITE,"recvbuff",recvbuff);
		rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
		if(!strncmp(recvbuff, "mkfs", 4))
		{
			dfs_mkfs("elm","flash");
			initPath();
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,"mkfs OK\n",8,0));
		}
		
		//烧写和读取ECU的ID
		if(!strncmp(recvbuff, "set_ecu_id", 10)){
			strncpy(ecu.id, &recvbuff[11], 12);
			print2msg(ECU_DBG_IDWRITE,"ECU id",ecu.id);
			printdecmsg(ECU_DBG_IDWRITE,"length",strlen(ecu.id));
			ecu.id[12] = '\0';
			memcpy(strportflag,&ecu.id[11],1);
			strportflag[1] = '\0';
			portflag = atoi(strportflag)%2;
			printf("portflag:%d\n",portflag);
			
			ret = 0;
			if(InitWorkMode(portflag) < 0){
				ret = -1;
			}
			if(setECUID(ecu.id) < 0){
				ret = -2;
			}
			
			fp=fopen("/yuneng/ecuid.con","r");
			fgets(ecu.id,13,fp);
			fclose(fp);
			restartThread(TYPE_MAIN);
			if(ret == 0)
			{
				printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,ecu.id,strlen(ecu.id),0));
			}else if (ret == -1)
			{
				printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,"error code: -1",14,0));
			}else if (ret == -2)
			{
				printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,"error code: -2",14,0));
			}
			
		}
		if(!strncmp(recvbuff, "get_ecu_id", 10)){
			memset(ecu.id,'\0',sizeof(ecu.id));
			fp=fopen("/yuneng/ecuid.con","r");
			fgets(ecu.id,13,fp);
			fclose(fp);
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,ecu.id,strlen(ecu.id),0));
		}

		//烧写和读取ECU有线网络的MAC
		if(!strncmp(recvbuff, "set_eth0_mac", 12)){
			mac[0] = recvbuff[13];
			mac[1] = recvbuff[14];
			mac[2] = ':';
			mac[3] = recvbuff[15];
			mac[4] = recvbuff[16];
			mac[5] = ':';
			mac[6] = recvbuff[17];
			mac[7] = recvbuff[18];
			mac[8] = ':';
			mac[9] = recvbuff[19];
			mac[10] = recvbuff[20];
			mac[11] = ':';
			mac[12] = recvbuff[21];
			mac[13] = recvbuff[22];
			mac[14] = ':';
			mac[15] = recvbuff[23];
			mac[16] = recvbuff[24];
			print2msg(ECU_DBG_IDWRITE,"ECU eth0 MAC address",mac);
			printdecmsg(ECU_DBG_IDWRITE,"length",strlen(mac));
			fp=fopen("/yuneng/ecumac.con","w");
			fputs(mac,fp);
			fclose(fp);
			memset(mac,'\0',sizeof(mac));
			
			fp=fopen("/yuneng/ecumac.con","r");
			fgets(mac,18,fp);
			fclose(fp);
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,mac,strlen(mac),0));
		}
		if(!strncmp(recvbuff, "get_eth0_mac", 12)){
			memset(mac,'\0',sizeof(mac));
			fp=fopen("/yuneng/ecumac.con","r");
			fgets(mac,18,fp);
			fclose(fp);
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,mac,strlen(mac),0));
		}

		//设置ECU的本地时间
		if(!strncmp(recvbuff, "set_time", 8)){
			settime(&recvbuff[9]);
			send(clientfd, &recvbuff[9], 14, 0);
			restartThread(TYPE_MAIN);
		}

		//设置逆变器的ID
		if(!strncmp(recvbuff, "set_inverter_id", 15)){
			/*
			if(1 == saveECUChannel(18))
				send(clientfd, "0x12", 4, 0);
			else
				send(clientfd, "change channel failed", 21, 0);
			*/
			row = insertinverter(&recvbuff[16]);
			snprintf(sendbuff, sizeof(sendbuff), "%02d", row);
			send(clientfd, sendbuff, 3, 0);
			restartThread(TYPE_MAIN);
		}

		//读取PLC的测试结果
		if(!strncmp(recvbuff, "query_result", 12)){
			record = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
			if(getrecord(record) > 0)
				send(clientfd, record, strlen(record), 0);
			else
				send(clientfd, "Failed", 6, 0);
			free(record);
		}
		
		//读取逆变器保护状态
		if(!strncmp(recvbuff, "query_protection", 16)){
			eve = malloc(MAXINVERTERCOUNT*50+1);
			if(getevent(eve) > 0)
				send(clientfd, eve, strlen(eve), 0);
			else
				send(clientfd, "Failed", 6, 0);
			free(eve);
		}

		//清空测试记录
		if(!strncmp(recvbuff, "clear", 5)){

			if(1 == saveECUChannel(16))
				send(clientfd, "0x10", 4, 0);
			else
				send(clientfd, "change channel failed", 21, 0);
			clearrecord();
			//system("rm /home/tmpdb");
			//system("rm /home/historical_data.db");
			//system("rm /home/record.db");
			send(clientfd, "clearok", 7, 0);
		}


		//烧写和读取ECU的地区
		if(!strncmp(recvbuff, "set_area", 8)){
			strncpy(area, &recvbuff[9], sizeof(area));
			fp=fopen("/yuneng/area.con", "w");
			fputs(area,fp);
			fclose(fp);

			memset(area,'\0',sizeof(area));
			
			fp=fopen("/yuneng/area.con","r");
			if(fp){
				fgets(area, sizeof(area), fp);
				fclose(fp);
			}
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,area,strlen(area),0));
		}

		if(!strncmp(recvbuff, "get_area", 8)){
			memset(area,'\0',sizeof(area));
			fp=fopen("/yuneng/area.con","r");
			if(fp){
				fgets(area, sizeof(area), fp);
				fclose(fp);
			}
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd,area,strlen(area),0));
		}

		//读取ECU软件版本号
		if(!strncmp(recvbuff, "get_version", 11)){
			memset(version, 0, sizeof(version));
			sprintf(version,"%s_%s_%s", ECU_VERSION,MAJORVERSION,MINORVERSION);
			memset(area, 0, sizeof(area));
			fp = fopen("/yuneng/area.con", "r");
			if(fp){
				fgets(area, sizeof(area), fp);
				fclose(fp);
			}
			strcat(version, area);
			printdecmsg(ECU_DBG_IDWRITE,"Send",send(clientfd, version, strlen(version), 0));
		}

		//读取本地时间
		if(!strncmp(recvbuff, "get_time", 8)){
			apstime(gettime);
			send(clientfd, gettime, 14, 0);
		}



		//测试LED灯
		if(!strncmp(recvbuff, "test_led", 8)){
			rt_hw_led_init();	//LED初始化
			rt_hw_us_delay(500000);
			rt_hw_led_on();		//LED亮
			rt_hw_us_delay(500000);
			rt_hw_led_off();		//LED熄灭
			rt_hw_us_delay(500000);
			rt_hw_led_on();		//LED亮
			rt_hw_us_delay(500000);
			rt_hw_led_off();		//LED熄灭
			rt_hw_us_delay(500000);
			rt_hw_led_on();		//LED亮
			rt_hw_us_delay(500000);
			rt_hw_led_off();		//LED熄灭
		}
		rt_mutex_release(record_data_lock);
		closesocket(clientfd);
	}
}
