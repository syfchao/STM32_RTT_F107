#ifndef __CLIENT_SOCKET_H__
#define __CLIENT_SOCKET_H__

void showconnected(void);		//已连接EMA
void showdisconnected(void);		//无法连接EMA	

int serverCommunication_Client(char *sendbuff,int sendLength,char *recvbuff,int *recvLength,int Timeout);

#endif /*__CLIENT_SOCKET_H__*/
