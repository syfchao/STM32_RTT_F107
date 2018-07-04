/*****************************************************************************/
/* File      : comm_config.c                                                 */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-04-03 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "remote_control_protocol.h"
#include "debug.h"
#include "myfile.h"
#include "threadlist.h"
#include "rtthread.h"
#include "mycommand.h"
#include  "file.h"
/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern ecu_info ecu;

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/

/* 【A106】ECU上报通信配置参数 */
int response_comm_config(const char *recvbuffer, char *sendbuffer)
{
    char timestamp[15] = {'\0'}; //时间戳
    //时间戳
    strncpy(timestamp, &recvbuffer[34], 14);

    /* 拼接协议 */
    msg_Header(sendbuffer, "A106");
    msgcat_s(sendbuffer, 12, ecu.id);
    msgcat_d(sendbuffer, 1, 2);
    msgcat_s(sendbuffer, 14, timestamp);
    strcat(sendbuffer, "END");
    strcat(sendbuffer, "1");
    msgcat_d(sendbuffer, 5, client_arg.port1);
    msgcat_d(sendbuffer, 5, client_arg.port2);
    msgcat_d(sendbuffer, 3, 10);
    msgcat_d(sendbuffer, 2, 5);
    if(strlen(client_arg.domain) > 0)
    {
        msgcat_d(sendbuffer, 1, 1);
        strcat(sendbuffer, client_arg.domain);
    }else
    {
        msgcat_d(sendbuffer, 1, 0);
        strcat(sendbuffer, client_arg.ip);
    }
    
    strcat(sendbuffer, "END");

    strcat(sendbuffer, "2");
    msgcat_d(sendbuffer, 5, control_client_arg.port1);
    msgcat_d(sendbuffer, 5, control_client_arg.port2);
    msgcat_d(sendbuffer, 3, control_client_arg.timeout);
    msgcat_d(sendbuffer, 2, control_client_arg.report_interval);
    if(strlen(control_client_arg.domain) > 0)
    {
        msgcat_d(sendbuffer, 1, 1);
        strcat(sendbuffer, control_client_arg.domain);
    }else
    {
        msgcat_d(sendbuffer, 1, 0);
        strcat(sendbuffer, control_client_arg.ip);
    }
    
    strcat(sendbuffer, "END");
 
    return 0;
}

/* 【A107】EMA设置通信配置参数 */
int set_comm_config(const char *recvbuffer, char *sendbuffer)
{
    int ack_flag = SUCCESS;
    int comm_cfg_num = 0;
    int comm_cfg_type = 0;
    int cfg_begin = 48;
    char timestamp[15] = {'\0'};
    char *buffer = NULL;
    buffer = malloc(256);
    memset(buffer,0x00,256);

    //获取通信配置数量
    comm_cfg_num = msg_get_int(&recvbuffer[30], 1);
    //获取时间戳
    strncpy(timestamp, &recvbuffer[31], 14);
    while(comm_cfg_num--){
        //复制出到"END"为止的字符串
        memset(buffer, 0, sizeof(buffer));
        cfg_begin += msg_get_one_section(buffer, &recvbuffer[cfg_begin]) + 3;

        //通信协议种类
        comm_cfg_type = msg_get_int(buffer, 1);
        printdecmsg(ECU_DBG_CONTROL_CLIENT,"comm_cfg_type",comm_cfg_type);
        //[1]逆变器运行数据通信配置
        if(comm_cfg_type == 1){
	   //保存数据上传配置信息
            client_arg.port1 = msg_get_int(&buffer[1], 5);
            client_arg.port2 = msg_get_int(&buffer[6], 5);

            if(msg_get_int(&buffer[16], 1) == 0)//IP
            {
                strncpy(client_arg.ip, &buffer[17], 16);
            }else			//Domain
            {
                strncpy(client_arg.domain, &buffer[17], 32);
            }
        }
        //[2]远程控制通信配置
        else if(comm_cfg_type == 2){
	   //保存远程控制配置信息
            control_client_arg.port1 = msg_get_int(&buffer[1], 5);
            control_client_arg.port2 = msg_get_int(&buffer[6], 5);
            control_client_arg.timeout = msg_get_int(&buffer[11], 3);
            control_client_arg.report_interval = msg_get_int(&buffer[14], 2);
            if(msg_get_int(&buffer[16], 1) == 0)//IP
            {
                strncpy(control_client_arg.ip, &buffer[17], 16);
            }else			//Domain
            {
                strncpy(control_client_arg.domain, &buffer[17], 32);
            }
        }
        else{
            ack_flag = FORMAT_ERROR;
        }
        UpdateServerInfo();
    }
    //拼接应答消息
    msg_ACK(sendbuffer, "A107", timestamp, ack_flag);
    free(buffer);
    buffer = NULL;
    return 106;

}
