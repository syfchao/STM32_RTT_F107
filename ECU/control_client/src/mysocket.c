/*****************************************************************************/
/* File      : mysocket.c                                                    */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-30 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <dfs_posix.h> 
#include <stdlib.h>
#include <string.h>
#include <lwip/netdb.h> 
#include <lwip/sockets.h> 
#include <unistd.h>
#include <fcntl.h>
#include "datetime.h"
#include "debug.h"
#include "lan8720rst.h"

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/

int msg_is_complete(const char *s)
{
	int i, msg_length = 18;
	char buffer[6] = {'\0'};

	//
	if(strlen(s) < 10)
		return 0;

	//从信息头获取信息长度
	strncpy(buffer, &s[5], 5);
	for(i=0; i<5; i++)
		if('A' == buffer[i])
			buffer[i] = '0';
	msg_length = atoi(buffer);

	//将实际收到的长度与信息中定义的长度做比较
	if(strlen(s) < msg_length){
		return 0;
	}
	return 1;
}

/* 建立Socket */
int create_socket(void)
{
	int sockfd;

	sockfd = socket(AF_INET,SOCK_STREAM,0);
//	iflags = fcntl(sockfd, F_GETFL, 0);
//	fcntl(sockfd, F_SETFL, O_NONBLOCK | iflags);
	if(sockfd == -1){
		printmsg(ECU_DBG_CONTROL_CLIENT,"socket failed");
        return -1;
	}
	return sockfd;
}

/* （客户端）连接服务器 */
int connect__socket(int sockfd, int port, const char *ip, const char *domain)
{
	/* 函数说明 */
	//htons():将16位主机字节序转换成网络字节序
	//inet_addr():将网络地址转换成二进制数字
	char ip_addr[32] = {'\0'};
	struct hostent *host;
	struct sockaddr_in serv_addr;
	char time[20];

	if(rt_hw_GetWiredNetConnect() == 0)
	{
		closesocket(sockfd);
		return -1;
	}
	print2msg(ECU_DBG_CONTROL_CLIENT,"IP",(char*)ip);
	print2msg(ECU_DBG_CONTROL_CLIENT,"Domain",(char*)domain);
	printdecmsg(ECU_DBG_CONTROL_CLIENT,"port",port);

	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET; //地址族
	serv_addr.sin_port = htons(port); //服务器端口号
	//解析域名
	host = gethostbyname(domain);
	if(host == NULL){
		print2msg(ECU_DBG_CONTROL_CLIENT,"Get host failed, use default IP : ",(char *)ip);
		serv_addr.sin_addr.s_addr = inet_addr(ip); //服务器IP地址
	}
	else{
		memset(ip_addr, '\0', sizeof(ip_addr));
		sprintf(ip_addr,"%s",ip_ntoa((ip_addr_t*)*host->h_addr_list));
		serv_addr.sin_addr.s_addr = inet_addr(ip_addr); //服务器IP地址
	}
	memset((serv_addr.sin_zero),0x00,8);

	getcurrenttime(time);
	//请求连接服务器
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
	{
		print2msg(ECU_DBG_CONTROL_CLIENT,"Failed to connect to ",time);
		printmsg(ECU_DBG_CONTROL_CLIENT,"connect");
		closesocket(sockfd);
		return -1;
	}
	print2msg(ECU_DBG_CONTROL_CLIENT,"Connecting EMA successfully", time);
	return 0;
}

/* 发送数据 */
int send_socket(int sockfd, char *sendbuffer, int size)
{
	int i, send_count;
	char msg_length[6] = {'\0'};

	if(sendbuffer[strlen(sendbuffer)-1] == '\n'){
		sprintf(msg_length, "%05d", strlen(sendbuffer)-1);
	}
	else{
		sprintf(msg_length, "%05d", strlen(sendbuffer));
		strcat(sendbuffer, "\n");
		size++;
	}
	strncpy(&sendbuffer[5], msg_length, 5);
	for(i=0; i<3; i++){
		send_count = send(sockfd, sendbuffer, size, 0);
		if(send_count >= 0){
			sendbuffer[strlen(sendbuffer)-1] = '\0';
			print2msg(ECU_DBG_CONTROL_CLIENT,"Sent", sendbuffer);
//			sleep(2);
			return send_count;
		}
	}
	printmsg(ECU_DBG_CONTROL_CLIENT,"Send failed");
	return -1;
}

/* 接收数据 */
int recv_socket(int sockfd, char *recvbuffer, int size, int timeout_s)
{	
	fd_set rd;
	struct timeval timeout = {10,0};
	int recv_each = 0, recv_count = 0,res = 0;
	char *recv_buffer = NULL;	//[4096] = {'\0'};
	recv_buffer = malloc(4096);
	memset(recv_buffer,'\0',4096);
	memset(recvbuffer, '\0', size);
	while(1)
	{
		FD_ZERO(&rd);
		FD_SET(sockfd, &rd);
		timeout.tv_sec = timeout_s;
		res = select(sockfd+1, &rd, NULL, NULL, &timeout);
		switch(res){
			case -1:
				printmsg(ECU_DBG_CONTROL_CLIENT,"select");
			case 0:
				printmsg(ECU_DBG_CONTROL_CLIENT,"Receive date from EMA timeout");
				closesocket(sockfd);
				printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
				free(recv_buffer);
				recv_buffer = NULL;
				return -1;
			default:
				if(FD_ISSET(sockfd, &rd)){
					memset(recv_buffer, '\0', sizeof(recv_buffer));
					recv_each = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);
					strcat(recvbuffer, recv_buffer);
					if(recv_each <= 0){
						
						printdecmsg(ECU_DBG_CONTROL_CLIENT,"Communication over", recv_each);
						free(recv_buffer);
						recv_buffer = NULL;
						return -1;
					}
					//printdecmsg(ECU_DBG_CONTROL_CLIENT,"Received each time", recv_each);
					recv_count += recv_each;
//					debug_msg("Received Total:%d", recv_count);
					
					if(msg_is_complete(recvbuffer)){
						print2msg(ECU_DBG_CONTROL_CLIENT,"Received", recvbuffer);
						free(recv_buffer);
						recv_buffer = NULL;
						return recv_count;
					}
				}
				break;
		}
	}

}

/* Socket客户端初始化 返回-1表示失败*/ 
int client_socket_init(int port, const char *ip, const char *domain)
{
	int sockfd;
	int ret;

	sockfd = create_socket();
	if(sockfd < 0) return sockfd;
	ret = connect__socket(sockfd, port, ip, domain);
	if(ret < 0) return ret;
	return sockfd;
}

