/*****************************************************************************/
/* File      : inverter_id.c                                                 */
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
#include "remote_control_protocol.h"
#include "debug.h"
#include "rthw.h"
#include "file.h"
#include "myfile.h"
#include "threadlist.h"
#include "rtthread.h"
#include "version.h"
#include "main-thread.h"
#include "dfs_posix.h"
#include "InternalFlash.h"

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern rt_mutex_t record_data_lock;
extern inverter_info inverter[MAXINVERTERCOUNT];
extern ecu_info ecu;


/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
/* Э���ECU���� */
int ecu_msg(char *sendbuffer, int num, const char *recvbuffer)
{
    char version_msg[16] = {'\0'};	//�汾��Ϣ������������+�汾��+���ְ汾�ţ�
    char version[16] = {'\0'};		//�汾��
    char area[16] = {'\0'};
    char version_number[2] = {'\0'};//���ְ汾��
    char timestamp[16] = {'\0'};	//ʱ���

    /* �������� */
    sprintf(version,"%s%s.%s",ECU_EMA_VERSION,MAJORVERSION,MINORVERSION);
    version_number[0] = '3';
    version_number[1] = '\0';
    ReadPage(INTERNAL_FALSH_AREA,area,8);
    if(strlen(area) < 2)
    {
        memset(area,0x00,2);
    }
    if(strlen(version_number)){
        sprintf(version_msg, "%02d%s%s--%s",
                strlen(version) + strlen(area) + 2 + strlen(version_number),
                version,
                area,
                version_number);
    }
    else{
        sprintf(version_msg, "%02d%s%s", strlen(version), version, area);
    }
    strncpy(timestamp, &recvbuffer[34], 14);

    /* ƴ��ECU��Ϣ */
    msgcat_s(sendbuffer, 12, ecu.id);
    strcat(sendbuffer, version_msg);
    msgcat_d(sendbuffer, 3, num);
    msgcat_s(sendbuffer, 14, timestamp);
    msgcat_s(sendbuffer, 3, "END");

    return 0;
}

/* Э������������ */
int inverter_msg(char *sendbuffer, char* id,char* inverter_version)
{
    //��������ID
    strcat(sendbuffer, id); //�����ID
    strcat(sendbuffer, "00"); 	 //���������
    strcat(sendbuffer, inverter_version); //what guess
    strcat(sendbuffer, "END"); 	 //������

    return 0;
}

/* ����������������ӳɹ���̨���� */
int add_id(const char *msg, int num)
{
    int i, count = 0;
    char inverter_id[13] = {'\0'};
    int fd;
    char buff[50];
    fd = open("/home/data/id", O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        for(i=0; i<num; i++)
        {
            strncpy(inverter_id, &msg[i*15], 12);
            inverter_id[12] = '\0';
            sprintf(buff,"%s,,,,,,\n",inverter_id);
            write(fd,buff,strlen(buff));
            count++;
        }

        close(fd);
    }

    WritePage(INTERNAL_FLASH_LIMITEID,"1",1);
    return count;

}

/* ɾ�������������ɾ���ɹ���̨���� */
int delete_id(const char *msg, int num)
{
    int i, count = 0;
    char inverter_id[13] = {'\0'};

    for(i=0; i<num; i++)
    {
        //��ȡһ̨�������ID��
        strncpy(inverter_id, &msg[i*15], 12);
        inverter_id[12] = '\0';
        //ɾ��һ�������ID
        delete_line("/home/data/id","/home/data/idtmp",inverter_id,12);
        count++;
    }
    return count;
}

/* �������� */
int clear_id()
{
    unlink("/home/data/id");
    return 0;
}


/* ��A102��ECU�ϱ������ID */
int response_inverter_id(const char *recvbuffer, char *sendbuffer)
{
    //��¼���������
    int i = 0;
    char UID[13];
    char inverter_version[6] = {'\0'};

    /* Head */
    strcpy(sendbuffer, "APS13AAAAAA102AAA0"); //����Э�麯��

    {

        /* ECU Message */
        ecu_msg(sendbuffer, ecu.total, recvbuffer);

        for(i = 0; i < ecu.total;i++)
        {
            sprintf(UID,"%s",inverter[i].id);
            sprintf(inverter_version,"%05d",inverter[i].version);
            UID[12] = '\0';
            /* Inverter Message */
            inverter_msg(sendbuffer,UID,inverter_version);

        }

    }
    return 0;
}

/* ��A103��EMA���������ID */
int set_inverter_id(const char *recvbuffer, char *sendbuffer)
{
    int flag, num;
    int ack_flag = SUCCESS;
    char timestamp[15] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);

    //��ȡ�������ͱ�־λ: 0��������; 1��������; 2ɾ�������
    sscanf(&recvbuffer[30], "%1d", &flag);
    //��ȡ���������
    num = msg_get_int(&recvbuffer[31], 3);
    //��ȡʱ���
    strncpy(timestamp, &recvbuffer[34], 14);

    //����ʽ
    if(!msg_num_check(&recvbuffer[51], num, 12, 1))
    {
        ack_flag = FORMAT_ERROR;
    }
    else
    {
        {
            //���ݿ�򿪳ɹ����������ݲ���
            switch(flag)
            {
            case 0:
                //��������
                if(clear_id())
                    ack_flag = DB_ERROR;
                break;
            case 1:
                //��������
                if(add_id(&recvbuffer[51], num) < num)
                    ack_flag = DB_ERROR;
                break;
            case 2:
                //ɾ�������
                if(delete_id(&recvbuffer[51], num) < num)
                    ack_flag = DB_ERROR;
                break;
            default:
                ack_flag = FORMAT_ERROR; //��ʽ����
                break;
            }
        }
        //�������߳�
        init_inverter_A103(inverter);
        threadRestartTimer(20,TYPE_MAIN);
        //restartThread(TYPE_MAIN);
    }

    //ƴ��Ӧ����Ϣ
    msg_ACK(sendbuffer, "A103", timestamp, ack_flag);
    rt_mutex_release(record_data_lock);
    return 102; //������һ��ִ������������
}
