#include "clientSocket.h"
#include <lwip/netdb.h> 
#include <lwip/sockets.h> 
#include "lan8720rst.h"
#include <stdio.h>
#include "rtc.h"
#include "threadlist.h"
#include "usart5.h"
#include "debug.h"
#include "string.h"
#include "rthw.h"
#include "rtthread.h"
#include "rtc.h"
#include "datetime.h"

extern ecu_info ecu;
extern unsigned char LED_Status;


void showconnected(void)		//������EMA
{
    FILE *fp;

    fp = fopen("/tmp/conemafl.txt", "w");
    if(fp)
    {
        fputs("connected", fp);
        fclose(fp);
    }

}

void showdisconnected(void)		//�޷�����EMA
{
    FILE *fp;

    fp = fopen("/tmp/conemafl.txt", "w");
    if(fp)
    {
        fputs("disconnected", fp);
        fclose(fp);
    }

}


int createsocket(void)					//����SOCKET����
{
    int fd_sock;

    fd_sock=socket(AF_INET,SOCK_STREAM,0);
    if(-1==fd_sock)
        printmsg(ECU_DBG_OTHER,"Failed to create socket");
    else
        printmsg(ECU_DBG_OTHER,"Create socket successfully");

    return fd_sock;
}


int connect_client_socket(int fd_sock)				//ͨ�����ߵķ�ʽ���ӷ�����
{	
    struct sockaddr_in serv_addr;
    struct hostent * host;

    //�������������ӣ�ֱ�ӹر�socket
    if(rt_hw_GetWiredNetConnect() == 0)
    {
        closesocket(fd_sock);
        return -1;
    }

    host = gethostbyname(client_arg.domain);
    if(NULL == host)
    {
        printmsg(ECU_DBG_CLIENT,"Resolve domain failure");
    }
    else
    {
        memset(client_arg.ip, '\0', sizeof(client_arg.ip));
        sprintf(client_arg.ip,"%s",ip_ntoa((ip_addr_t*)*host->h_addr_list));
    }

    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;

    serv_addr.sin_port=htons(randport(client_arg));
    serv_addr.sin_addr.s_addr=inet_addr(client_arg.ip);
    memset(&(serv_addr.sin_zero),0,8);

    if(-1==connect(fd_sock,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))){
        //showdisconnected();
        printmsg(ECU_DBG_CLIENT,"Failed to connect to EMA");
        closesocket(fd_sock);
        return -1;
    }
    else{
        //showconnected();
        printmsg(ECU_DBG_CLIENT,"Connect to EMA Client successfully");
        return 1;
    }
}

void close_socket(int fd_sock)					//�ر�socket����
{
    closesocket(fd_sock);
    printmsg(ECU_DBG_OTHER,"Close socket");
}

//��Client������ͨѶ 
//sendbuff[in]:�������ݵ�buff
//sendLength[in]:�����ֽڳ���
//recvbuff[out]:��������buff
//recvLength[in/out]:in����������󳤶�  out�������ݳ���
//Timeout[in]	��ʱʱ�� ms
int serverCommunication_Client(char *sendbuff,int sendLength,char *recvbuff,int *recvLength,int Timeout)
{
    int socketfd = 0;
    int readbytes = 0,count =0,recvlen =0;
    char *readbuff = NULL;
    int length = 0,send_length = 0;
    length = sendLength;
    socketfd = createsocket();
    if(socketfd == -1) 	//����socketʧ��
    {
        LED_Status = 0;
        return -1;
    }

    //����socket�ɹ�
    if(1 == connect_client_socket(socketfd))
    {	//���ӷ������ɹ�
        int sendbytes = 0;
        int res = 0;
        fd_set rd;
        struct timeval timeout;

        while(length > 0)
        {
            if(length > SIZE_PER_SEND)
            {
                sendbytes = send(socketfd, &sendbuff[send_length], SIZE_PER_SEND, 0);
                send_length += SIZE_PER_SEND;
                length -= SIZE_PER_SEND;
            }else
            {
                sendbytes = send(socketfd, &sendbuff[send_length], length, 0);

                length -= length;
            }

            if(-1 == sendbytes)
            {
                close_socket(socketfd);
                LED_Status = 0;
                return -1;
            }

            rt_hw_ms_delay(500);
        }



        readbuff = malloc((4+99*14));
        memset(readbuff,'\0',(4+99*14));

        while(1)
        {
            FD_ZERO(&rd);
            FD_SET(socketfd, &rd);
            timeout.tv_sec = Timeout/1000;
            timeout.tv_usec = Timeout%1000;

            res = select(socketfd+1, &rd, NULL, NULL, &timeout);

            if(res <= 0){
                printmsg(ECU_DBG_CLIENT,"Receive data reply from Server timeout");
                close_socket(socketfd);
                free(readbuff);
                readbuff = NULL;
                LED_Status = 0;
                return -1;
            }else
            {
                memset(readbuff, '\0', sizeof(readbuff));
                readbytes = recv(socketfd, readbuff, *recvLength, 0);
                if(readbytes <= 0)
                {
                    free(readbuff);
                    readbuff = NULL;
                    *recvLength = 0;
                    close_socket(socketfd);
                    LED_Status = 0;
                    return -1;
                }
                strcat(recvbuff,readbuff);
                recvlen += readbytes;
                if(recvlen >= 3)
                {
                    print2msg(ECU_DBG_CLIENT,"recvbuff:",recvbuff);
                    printdecmsg(ECU_DBG_CLIENT,"recv length:",readbytes);
                    *recvLength = recvlen;
                    count = (recvbuff[1]-0x30)*10 + (recvbuff[2]-0x30);
                    if(count==((strlen(recvbuff)-3)/14))
                    {
                        free(readbuff);
                        readbuff = NULL;
                        close_socket(socketfd);
                        LED_Status = 1;
                        return *recvLength;
                    }else if(recvbuff[0] != '1')
                    {
                        free(readbuff);
                        readbuff = NULL;
                        close_socket(socketfd);
                        *recvLength = 0;
                        LED_Status = 0;
                        return *recvLength;
                    }
                }

            }
        }


    }else
    {
        //���ӷ�����ʧ��
        //ʧ�������ͨ�����߷���
#ifdef WIFI_USE
        int ret = 0,i = 0;
        ret = SendToSocketB(client_arg.ip,randport(client_arg),sendbuff, sendLength);
        if(ret == -1)
        {
            LED_Status = 0;
            return -1;
        }

        for(i = 0;i<(Timeout/10);i++)
        {
            if(WIFI_Recv_SocketB_Event == 1)
            {
                *recvLength = WIFI_Recv_SocketB_LEN;
                memcpy(recvbuff,WIFI_RecvSocketBData,*recvLength);
                recvbuff[*recvLength] = '\0';
                //sprint2msg(ECU_DBG_CLIENT,"serverCommunication_Client",recvbuff);
                WIFI_Recv_SocketB_Event = 0;
                LED_Status = 1;
                return 0;
            }
            rt_thread_delay(1);
        }
        LED_Status = 0;
        return -1;
#endif

#ifndef WIFI_USE
        LED_Status = 0;
        return -1;
#endif
    }

}


