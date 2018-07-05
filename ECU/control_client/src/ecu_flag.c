#include <string.h>
#include "remote_control_protocol.h"
#include "debug.h"
#include "myfile.h"
#include "rtthread.h"
#include "mycommand.h"
#include "InternalFlash.h"

extern rt_mutex_t record_data_lock;
/* ��A119��EMA����ECU��ͨ�ſ��� */
int set_ecu_flag(const char *recvbuffer, char *sendbuffer)
{
    int ack_flag = SUCCESS;
    int ecu_flag = 1;
    char timestamp[15] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    //��ȡʱ���
    strncpy(timestamp, &recvbuffer[30], 14);
    //ͨ�ű�־λ
    ecu_flag = msg_get_int(&recvbuffer[44], 1);

    if(ecu_flag == 0)
        WritePage(INTERNAL_FLASH_ECU_FLAG,"0",1);
    else if(ecu_flag == 1)
        WritePage(INTERNAL_FLASH_ECU_FLAG,"1",1);
    else
        ack_flag = FORMAT_ERROR;

    reboot_timer(10);
    //ƴ��Ӧ����Ϣ
    msg_ACK(sendbuffer, "A119", timestamp, ack_flag);
    rt_mutex_release(record_data_lock);
    return -1;
}
