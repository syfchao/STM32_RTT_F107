/*****************************************************************************/
/* File      : control_client.c                                              */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-28 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "control_client.h"
#include <board.h>
#include "checkdata.h"
#include "datetime.h"
#include <dfs_posix.h> 
#include <rtthread.h>
#include "file.h"
#include "debug.h"
#include "datetime.h"
#include <lwip/netdb.h> 
#include <lwip/sockets.h> 
#include "myfile.h"
#include "mysocket.h"
#include "remote_control_protocol.h"
#include "threadlist.h"

#include "inverter_id.h"
#include "comm_config.h"
#include "inverter_maxpower.h"
#include "inverter_onoff.h"
#include "inverter_gfdi.h"
#include "ecu_flag.h"
#include "inverter_ird.h"
#include "inverter_update.h"
#include "inverter_ac_protection.h"
#include "custom_command.h"
#include "usart5.h"
#include "power_factor.h"
#include "datelist.h"
#include "ZigBeeTransmission.h"
#include "ZigBeeChannel.h"
#include "InternalFlash.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
#define ARRAYNUM 6
#define MAXBUFFER (MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18)
#define FIRST_TIME_CMD_NUM 8

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern rt_mutex_t record_data_lock;
extern ecu_info ecu;

enum CommandID{
    A100, A101, A102, A103, A104, A105, A106, A107, A108, A109, //0-9
    A110, A111, A112, A113, A114, A115, A116, A117, A118, A119, //10-19
    A120, A121, A122, A123, A124, A125, A126, A127, A128, A129, //20-29
    A130, A131, A132, A133, A134, A135, A136, A137, A138, A139, //30-39
    A140, A141, A142, A143, A144, A145, A146, A147, A148, A149,
    A150, A151, A152, A153, A154, A155, A156, A157, A158, A159,
    A160, A161, A162, A163, A164, A165, A166, A167, A168, A169,
    A170, A171, A172, A173, A174, A175, A176, A177, A178, A179,
};
int (*pfun[100])(const char *recvbuffer, char *sendbuffer);

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/

void add_functions()
{

    pfun[A102] = response_inverter_id; 			//�ϱ������ID  										OK
    pfun[A103] = set_inverter_id; 				//���������ID												OK
    pfun[A106] = response_comm_config;			//�ϱ�ECU��ͨ�����ò���
    pfun[A107] = set_comm_config;				//����ECU��ͨ�����ò���
    pfun[A108] = custom_command;				//��ECU�����Զ�������
    pfun[A110] = set_inverter_maxpower;			//��������������
    pfun[A111] = set_inverter_onoff;			//������������ػ�
    pfun[A112] = clear_inverter_gfdi;			//���������GFDI
    pfun[A117] = response_inverter_maxpower;	//�ϱ����������ʼ���Χ
    pfun[A119] = set_ecu_flag;					//����ECU��EMA��ͨ�ſ���
    pfun[A126] = read_inverter_ird;				//��ȡ�������IRDѡ��
    pfun[A127] = set_inverter_ird;				//�����������IRDѡ��
    pfun[A130] = response_ecu_ac_protection_17;	//�ϱ�ECU��������������(17��)
    pfun[A131] = read_inverter_ac_protection_17;//��ȡ������Ľ�����������(17��)
    pfun[A132] = set_inverter_ac_protection_17;	//����������Ľ�����������(17��)
    pfun[A136] = set_inverter_update;			//�����������������־
    pfun[A145] = response_inverter_power_factor;//�ϱ����������������
    pfun[A146] = set_all_inverter_power_factor; //����ecu����������

    pfun[A161] = response_inverter_protection_all;//��ȡ�������������
    pfun[A162] = set_inverter_protection_new;	//�����������������
    pfun[A168] = query_ecu_ac_protection_all;	//���õ��ͷ���������
    pfun[A171] = set_ZigBeeChannel;			//�����ŵ�
    pfun[A172] = response_ZigBeeChannel_Result;	//�ϱ��ŵ����ý��
    pfun[A173] = transmission_ZigBeeInfo;		//ZigBee����͸��

}

/* [A118] ECU��������EMA��Ҫִ�еĴ������ */
int first_time_info(const char *recvbuffer, char *sendbuffer)
{
    static int command_id = 0;
    int functions[FIRST_TIME_CMD_NUM] = {
        A102, A106, A117,A126, A130, A131,A161,A168
    };
    printdecmsg(ECU_DBG_CONTROL_CLIENT,"first_time_info",(command_id));
    printdecmsg(ECU_DBG_CONTROL_CLIENT,"first_time_info A",functions[(command_id)]+100);

    //���ú���
    (*pfun[functions[command_id++]%100])(recvbuffer, sendbuffer);
    //	debug_msg("cmd:A%d", functions[command_id - 1] + 100);
    rt_hw_us_delay(100000);

    if(command_id < FIRST_TIME_CMD_NUM)
        return 118;
    else{
        command_id = 0;
        return 0;
    }

}

int detection_statusflag(char flag)		//���/home/record/inverstaĿ¼���Ƿ����flag�ļ�¼    ���ڷ���1�������ڷ���0
{
    DIR *dirp;
    char dir[30] = "/home/record/inversta";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    char *buff = NULL;
    struct list *tmp;
    FILE *fp;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/record/inversta");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"detection_statusflag open directory error");
            mkdir("/home/record/inversta",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }

        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);
            //print2msg(ECU_DBG_CONTROL_CLIENT,"detection_statusflag",path);
            //���ļ�һ�����ж��Ƿ���flag=2��  �������ֱ�ӹر��ļ�������1
            fp = fopen(path, "r");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if(buff[strlen(buff)-2] == flag)			//������һ���ֽڵ�resendflag�Ƿ�Ϊflag   ���������flag  �ر��ļ�����return 1
                        {
                            fclose(fp);
                            free(buff);
                            buff = NULL;
                            list_destroy(&Head);
                            rt_mutex_release(record_data_lock);
                            return 1;
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return 0;
}

//��status״̬Ϊ2�ı�־��Ϊ1
int change_statusflag1()  //�ı�ɹ�����1
{
    DIR *dirp;
    char dir[30] = "/home/record/inversta";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    char *buff = NULL;
    struct list *tmp;
    FILE *fp;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/record/inversta");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"change_statusflag1 open directory error");
            mkdir("/home/record/inversta",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%s",dir,d->d_name);
            //���ļ�һ�����ж��Ƿ���flag=2��  �������ֱ�ӹر��ļ�������1
            fp = fopen(path, "r+");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if(buff[strlen(buff)-2] == '2')
                        {
                            fseek(fp,-2L,SEEK_CUR);
                            fputc('1',fp);
                            fclose(fp);
                            free(buff);
                            buff = NULL;
                            list_destroy(&Head);
                            rt_mutex_release(record_data_lock);
                            return 1;
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
            }

        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return 0;
}	

void delete_statusflag0()		//�������flag��־ȫ��Ϊ0��Ŀ¼
{
    DIR *dirp;
    char dir[30] = "/home/record/inversta";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    char *buff = NULL;
    struct list *tmp;
    FILE *fp;
    int flag = 0;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/record/inversta");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"delete_statusflag0 open directory error");
            mkdir("/home/record/inversta",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);
            flag = 0;
            //print2msg(ECU_DBG_CONTROL_CLIENT,"delete_file_resendflag0 ",path);
            //���ļ�һ�����ж��Ƿ���flag!=0��  �������ֱ�ӹر��ļ�������,��������ڣ�ɾ���ļ�
            fp = fopen(path, "r");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                        {
                            if(buff[strlen(buff)-2] != '0')			//����Ƿ����resendflag != 0�ļ�¼   ��������ֱ���˳�����
                            {
                                flag = 1;
                                break;
                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
                if(flag == 0)
                {
                    print2msg(ECU_DBG_CONTROL_CLIENT,"unlink:",path);
                    //�������ļ���û����flag != 0�ļ�¼ֱ��ɾ���ļ�
                    unlink(path);
                }
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return;

}

//����ʱ����޸ı�־
int change_statusflag(char *time,char flag)  //�ı�ɹ�����1��δ�ҵ���ʱ��㷵��0
{
    DIR *dirp;
    char dir[30] = "/home/record/inversta";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    char filetime[15] = {'\0'};
    char *buff = NULL;
    struct list *tmp;
    FILE *fp;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/record/inversta");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"change_statusflag open directory error");
            mkdir("/home/record/inversta",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }

        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);
            //���ļ�һ�����ж��Ƿ���flag=2��  �������ֱ�ӹر��ļ�������1
            fp = fopen(path, "r+");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                        {
                            memset(filetime,0,15);
                            memcpy(filetime,&buff[strlen(buff)-17],14);				//��ȡÿ����¼��ʱ��
                            filetime[14] = '\0';
                            if(!memcmp(time,filetime,14))						//ÿ����¼��ʱ��ʹ����ʱ��Աȣ�����ͬ����flag
                            {
                                fseek(fp,-2L,SEEK_CUR);
                                fputc(flag,fp);
                                //print2msg(ECU_DBG_CONTROL_CLIENT,"change_resendflag",filetime);
                                fclose(fp);
                                free(buff);
                                buff = NULL;
                                list_destroy(&Head);
                                rt_mutex_release(record_data_lock);
                                return 1;
                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return 0;

}

//��ѯһ��flagΪ1������   ��ѯ���˷���1  ���û��ѯ������0
/*
data:��ʾ��ȡ��������
time����ʾ��ȡ����ʱ��
flag����ʾ�Ƿ�����һ������   ������һ��Ϊ1   ������Ϊ0
*/
int search_statusflag(char *data,char * time, int *flag,char sendflag)	
{
    DIR *dirp = NULL;
    char dir[30] = "/home/record/inversta";
    struct dirent *d = NULL;
    char path[100] = {'\0'};
    struct list *Head = NULL;
    char *buff = NULL;
    struct list *tmp;
    FILE *fp = NULL;
    int nextfileflag = 0;	//0��ʾ��ǰ�ļ��ҵ������ݣ�1��ʾ��Ҫ�Ӻ�����ļ���������
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(data,'\0',sizeof(data)/sizeof(data[0]));
    *flag = 0;
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/record/inversta");
        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"search_statusflag open directory error");
            mkdir("/home/record/inversta",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            if(tmp->date <= 20100101)
            {
                unlink(path);
                continue;	//���ʱ���С��2010 01 01 �Ļ����������ļ�
            }

            fp = fopen(path, "r");
            if(fp)
            {
                if(0 == nextfileflag)
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))  //��ȡһ������
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                            {
                                if(buff[strlen(buff)-2] == sendflag)			//������һ���ֽڵ�resendflag�Ƿ�Ϊ1
                                {
                                    memcpy(time,&buff[strlen(buff)-17],14);				//��ȡÿ����¼��ʱ��
                                    memcpy(data,buff,(strlen(buff)-18));
                                    data[strlen(buff)-18] = '\n';
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag time",time);
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag data",data);
                                    //rt_hw_s_delay(1);
                                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))	//�����¶����ݣ�Ѱ���Ƿ���Ҫ���͵�����
                                    {
                                        if(strlen(buff) > 18)
                                        {
                                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                                            {
                                                if(buff[strlen(buff)-2] == sendflag)
                                                {
                                                    *flag = 1;
                                                    fclose(fp);
                                                    free(buff);
                                                    buff = NULL;
                                                    list_destroy(&Head);
                                                    rt_mutex_release(record_data_lock);
                                                    return 1;
                                                }
                                            }
                                        }
                                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                                    }
                                    nextfileflag = 1;
                                    break;

                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }else
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))  //��ȡһ������
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                            {
                                if(buff[strlen(buff)-2] == sendflag)
                                {
                                    *flag = 1;
                                    fclose(fp);
                                    free(buff);
                                    buff = NULL;
                                    list_destroy(&Head);
                                    rt_mutex_release(record_data_lock);
                                    return 1;
                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }

                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);

    return nextfileflag;

}

void delete_pro_result_flag0()		//�������flag��־ȫ��Ϊ0��Ŀ¼
{
    DIR *dirp;
    char dir[30] = "/home/data/proc_res";
    struct dirent *d;
    char path[100];
    char *buff = NULL;
    FILE *fp;
    int flag = 0;
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/data/proc_res");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"delete_pro_result_flag0 open directory error");
            mkdir("/home/data/proc_res",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            flag = 0;
            //print2msg(ECU_DBG_CONTROL_CLIENT,"delete_file_resendflag0 ",path);
            //���ļ�һ�����ж��Ƿ���flag!=0��  �������ֱ�ӹر��ļ�������,��������ڣ�ɾ���ļ�
            fp = fopen(path, "r");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') )
                        {
                            if(buff[strlen(buff)-2] != '0')			//����Ƿ����resendflag != 0�ļ�¼   ��������ֱ���˳�����
                            {
                                flag = 1;
                                break;

                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);

                }
                fclose(fp);
                if(flag == 0)
                {
                    print2msg(ECU_DBG_CONTROL_CLIENT,"unlink:",path);
                    //�������ļ���û����flag != 0�ļ�¼ֱ��ɾ���ļ�
                    unlink(path);
                }
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return;

}

void delete_inv_pro_result_flag0()		//�������flag��־ȫ��Ϊ0��Ŀ¼
{
    DIR *dirp;
    char dir[30] = "/home/data/iprocres";
    struct dirent *d;
    char path[100];
    char *buff = NULL;
    FILE *fp;
    int flag = 0;
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/data/iprocres");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"delete_inv_pro_result_flag0 open directory error");
            mkdir("/home/data/iprocres",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            flag = 0;
            //print2msg(ECU_DBG_CONTROL_CLIENT,"delete_file_resendflag0 ",path);
            //���ļ�һ�����ж��Ƿ���flag!=0��  �������ֱ�ӹر��ļ�������,��������ڣ�ɾ���ļ�
            fp = fopen(path, "r");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') && (buff[strlen(buff)-20] == ','))
                        {
                            if(buff[strlen(buff)-2] != '0')			//����Ƿ����resendflag != 0�ļ�¼   ��������ֱ���˳�����
                            {
                                flag = 1;
                                break;
                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
                if(flag == 0)
                {
                    print2msg(ECU_DBG_CONTROL_CLIENT,"unlink:",path);
                    //�������ļ���û����flag != 0�ļ�¼ֱ��ɾ���ļ�
                    unlink(path);
                }
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return;

}

//����ʱ����޸ı�־
int change_pro_result_flag(char *item,char flag)  //�ı�ɹ�����1��δ�ҵ���ʱ��㷵��0
{
    DIR *dirp;
    char dir[30] = "/home/data/proc_res";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    char fileitem[4] = {'\0'};
    char *buff = NULL;
    FILE *fp;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/data/proc_res");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"change_pro_result_flag open directory error");
            mkdir("/home/data/proc_res",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            //���ļ�һ�����ж��Ƿ���flag=2��  �������ֱ�ӹر��ļ�������1
            fp = fopen(path, "r+");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') )
                        {
                            if(buff[strlen(buff)-2] == '1')
                            {
                                memset(fileitem,0,4);
                                memcpy(fileitem,&buff[strlen(buff)-6],3);				//��ȡÿ����¼��ʱ��
                                fileitem[3] = '\0';
                                if(!memcmp(item,fileitem,4))						//ÿ����¼��ʱ��ʹ����ʱ��Աȣ�����ͬ����flag
                                {
                                    fseek(fp,-2L,SEEK_CUR);
                                    fputc(flag,fp);
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"filetime",filetime);
                                    fclose(fp);
                                    free(buff);
                                    buff = NULL;
                                    list_destroy(&Head);
                                    rt_mutex_release(record_data_lock);
                                    return 1;
                                }
                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);

                }
                fclose(fp);
            }

        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return 0;

}

int change_inv_pro_result_flag(char *item,char flag)  //�ı�ɹ�����1��δ�ҵ���ʱ��㷵��0
{
    DIR *dirp;
    char dir[30] = "/home/data/iprocres";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    char fileitem[4] = {'\0'};
    char *buff = NULL;
    FILE *fp;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir("/home/data/iprocres");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"change_inv_pro_result_flag open directory error");
            mkdir("/home/data/iprocres",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        //��ȡ����
        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            //���ļ�һ�����ж��Ƿ���flag=2��  �������ֱ�ӹر��ļ�������1
            fp = fopen(path, "r+");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') && (buff[strlen(buff)-20] == ',') )
                        {
                            if(buff[strlen(buff)-2] == '1')
                            {
                                memset(fileitem,0,4);
                                memcpy(fileitem,&buff[strlen(buff)-6],3);				//��ȡÿ����¼��ʱ��
                                fileitem[3] = '\0';
                                if(!memcmp(item,fileitem,4))						//ÿ����¼��ʱ��ʹ����ʱ��Աȣ�����ͬ����flag
                                {
                                    fseek(fp,-2L,SEEK_CUR);
                                    fputc(flag,fp);
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"filetime",filetime);
                                    fclose(fp);
                                    free(buff);
                                    buff = NULL;
                                    list_destroy(&Head);
                                    rt_mutex_release(record_data_lock);
                                    return 1;
                                }
                            }
                        }
                    }
                    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                }
                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);
    return 0;

}


//��ѯһ��flagΪ1������   ��ѯ���˷���1  ���û��ѯ������0
/*
data:��ʾ��ȡ��������
item����ʾ��ȡ������֡
flag����ʾ�Ƿ�����һ������   ������һ��Ϊ1   ������Ϊ0
*/
int search_pro_result_flag(char *data,char * item, int *flag,char sendflag)	
{
    DIR *dirp;
    char dir[30] = "/home/data/proc_res";
    struct dirent *d;
    char path[100];
    char *buff = NULL;
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    FILE *fp;
    int nextfileflag = 0;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(data,'\0',sizeof(data)/sizeof(data[0]));
    *flag = 0;
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        dirp = opendir("/home/data/proc_res");
        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"search_pro_result_flag open directory error");
            mkdir("/home/data/proc_res",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        //��ȡ����
        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);


            if(tmp->date <= 20100101)
            {
                unlink(path);
                continue;	//���ʱ���С��2010 01 01 �Ļ����������ļ�
            }

            fp = fopen(path, "r");
            if(fp)
            {
                if(0 == nextfileflag)
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') )
                            {
                                if(buff[strlen(buff)-2] == sendflag)
                                {
                                    memcpy(item,&buff[strlen(buff)-6],3);				//��ȡÿ����¼��item
                                    memcpy(data,buff,(strlen(buff)-7));
                                    data[strlen(buff)-7] = '\n';
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag time",time);
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag data",data);
                                    //rt_hw_s_delay(1);
                                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                                    {
                                        if(strlen(buff) > 18)
                                        {
                                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') )
                                            {
                                                if(buff[strlen(buff)-2] == sendflag)
                                                {
                                                    *flag = 1;
                                                    fclose(fp);
                                                    free(buff);
                                                    buff = NULL;
                                                    list_destroy(&Head);
                                                    rt_mutex_release(record_data_lock);
                                                    return 1;
                                                }
                                            }
                                        }
                                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                                    }
                                    nextfileflag = 1;
                                    break;
                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }else
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                            {
                                if(buff[strlen(buff)-2] == sendflag)
                                {
                                    *flag = 1;
                                    fclose(fp);
                                    free(buff);
                                    buff = NULL;
                                    list_destroy(&Head);
                                    rt_mutex_release(record_data_lock);
                                    return 1;
                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }

                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);

    return nextfileflag;

}


//��ѯһ��flagΪ1������   ��ѯ���˷���1  ���û��ѯ������0
/*
data:��ʾ��ȡ��������
item����ʾ��ȡ������֡
flag����ʾ�Ƿ�����һ������   ������һ��Ϊ1   ������Ϊ0
*/
int search_inv_pro_result_flag(char *data,char * item,char *inverterid, int *flag,char sendflag)	
{
    DIR *dirp;
    char dir[30] = "/home/data/iprocres";
    struct dirent *d;
    char path[100];
    char *buff = NULL;
    struct list *Head = NULL;
    struct list *tmp;
    char data_str[9] = {'\0'};
    FILE *fp;
    int nextfileflag = 0;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    buff = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
    memset(data,'\0',sizeof(data)/sizeof(data[0]));
    *flag = 0;
    Head = list_create(-1);
    if(result == RT_EOK)
    {
        dirp = opendir("/home/data/iprocres");
        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,"search_inv_pro_result_flag open directory error");
            mkdir("/home/data/iprocres",0);
        }
        else
        {
            while ((d = readdir(dirp)) != RT_NULL)
            {
                memcpy(data_str,d->d_name,8);
                data_str[8] = '\0';
                list_addhead(Head,atoi(data_str));
            }
            closedir(dirp);
        }
        list_sort(Head);

        //��ȡ����
        for(tmp = Head->next; tmp != Head; tmp = tmp->next)
        {
            memset(path,0,100);
            memset(buff,0,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18));
            sprintf(path,"%s/%d.dat",dir,tmp->date);

            if(tmp->date <= 20100101)
            {
                unlink(path);
                continue;	//���ʱ���С��2010 01 01 �Ļ����������ļ�
            }
            fp = fopen(path, "r");
            if(fp)
            {
                if(0 == nextfileflag)
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') && (buff[strlen(buff)-20] == ',') )
                            {
                                if(buff[strlen(buff)-2] == sendflag)
                                {
                                    memcpy(item,&buff[strlen(buff)-6],3);				//��ȡÿ����¼��item
                                    memcpy(inverterid,&buff[strlen(buff)-19],12);
                                    memcpy(data,buff,(strlen(buff)-20));
                                    data[strlen(buff)-20] = '\n';
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag time",time);
                                    //print2msg(ECU_DBG_CONTROL_CLIENT,"search_readflag data",data);
                                    //rt_hw_s_delay(1);
                                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                                    {
                                        if(strlen(buff) > 18)
                                        {
                                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                                            {
                                                if(buff[strlen(buff)-2] == sendflag)
                                                {
                                                    *flag = 1;
                                                    fclose(fp);
                                                    free(buff);
                                                    buff = NULL;
                                                    list_destroy(&Head);
                                                    rt_mutex_release(record_data_lock);
                                                    return 1;
                                                }
                                            }
                                        }
                                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                                    }
                                    nextfileflag = 1;
                                    break;
                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }else
                {
                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))  //?����?��?DD��y?Y
                    {
                        if(strlen(buff) > 18)
                        {
                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-7] == ',') && (buff[strlen(buff)-20] == ',')  )
                            {
                                if(buff[strlen(buff)-2] == sendflag)
                                {
                                    *flag = 1;
                                    fclose(fp);
                                    free(buff);
                                    buff = NULL;
                                    list_destroy(&Head);
                                    rt_mutex_release(record_data_lock);
                                    return 1;
                                }
                            }
                        }
                        memset(buff,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18);
                    }
                }

                fclose(fp);
            }
        }

    }
    free(buff);
    buff = NULL;
    list_destroy(&Head);
    rt_mutex_release(record_data_lock);

    return nextfileflag;

}


//���������쳣״̬
int check_inverter_abnormal_status_sent(int hour)
{
    int sockfd;
    int i, flag, num = 0;
    char datetime[15] = {'\0'};
    char *recv_buffer = NULL;
    char *send_buffer = NULL;

    recv_buffer = rt_malloc(2048);
    send_buffer = rt_malloc(MAXBUFFER);
    if(get_hour() != hour)
    {
        rt_free(recv_buffer);
        rt_free(send_buffer);
        return 0;
    }
    //��ѯ�Ƿ���flag=2������
    if(0 == detection_statusflag('2'))
    {
        rt_free(recv_buffer);
        rt_free(send_buffer);
        return 0;
    }
    //��flag=2������,����һ����ȡEMA�Ѵ�ʱ�������
    printmsg(ECU_DBG_CONTROL_CLIENT,">>Start Check abnormal status sent");
    sockfd = client_socket_init(randport(control_client_arg), control_client_arg.ip, control_client_arg.domain);
    if(sockfd < 0)
    {
#ifdef WIFI_USE	
        //��������ʧ�ܣ�ʹ��wifi����
        {
            int j = 0,flag_failed = 0,ret = 0;
            strcpy(send_buffer, "APS13AAA51A123AAA0");
            strcat(send_buffer, ecu.id);
            strcat(send_buffer, "000000000000000000END\n");
            ret = SendToSocketC(control_client_arg.ip,randport(control_client_arg),send_buffer, strlen(send_buffer));
            if(ret == -1)
            {
                rt_free(recv_buffer);
                rt_free(send_buffer);
                return -1;
            }

            for(j=0;j<800;j++)
            {
                if(WIFI_Recv_SocketC_Event == 1)
                {
                    flag_failed = 1;
                    WIFI_Recv_SocketC_Event = 0;
                    break;
                }
                rt_thread_delay(1);
            }

            if(flag_failed == 0)
            {
                rt_free(recv_buffer);
                rt_free(send_buffer);
                return -1;
            }


            //ȥ��usr WIFI���ĵ�ͷ��
            memcpy(recv_buffer,WIFI_RecvSocketCData,WIFI_Recv_SocketC_LEN);
            recv_buffer[WIFI_Recv_SocketC_LEN] = '\0';
            //У������
            if(msg_format_check(recv_buffer) < 0){
                rt_free(recv_buffer);
                rt_free(send_buffer);
                return 0;
            }
            //�����յ���ʱ���,��ɾ��EMA�Ѵ������(�����Ϊ0)
            flag = msg_get_int(&recv_buffer[18], 1);
            num = 0;
            if(flag){
                num = msg_get_int(&recv_buffer[19], 2);
                for(i=0; i<num; i++){
                    strncpy(datetime, &recv_buffer[21 + i*14], 14);
                    change_statusflag(datetime,'0');
                }
            }

            //��flag=2�����ݸ�Ϊflag=1
            change_statusflag1();

            //������б�־Ϊ0�����������
            delete_statusflag0();
            rt_free(recv_buffer);
            rt_free(send_buffer);
            return 0;
        }
#endif		

#ifndef WIFI_USE
        rt_free(recv_buffer);
        rt_free(send_buffer);
        return -1;
#endif		
    }else
    {
        strcpy(send_buffer, "APS13AAA51A123AAA0");
        strcat(send_buffer, ecu.id);
        strcat(send_buffer, "000000000000000000END\n");
        send_socket(sockfd, send_buffer, strlen(send_buffer));
        //����EMAӦ��
        if(recv_socket(sockfd, recv_buffer, sizeof(recv_buffer), control_client_arg.timeout) <= 0){
            closesocket(sockfd);
            rt_free(recv_buffer);
            rt_free(send_buffer);
            return -1;
        }
        //У������
        if(msg_format_check(recv_buffer) < 0){
            closesocket(sockfd);
            rt_free(recv_buffer);
            rt_free(send_buffer);
            return 0;
        }
        //�����յ���ʱ���,��ɾ��EMA�Ѵ������(�����Ϊ0)
        flag = msg_get_int(&recv_buffer[18], 1);
        num = 0;
        if(flag){
            num = msg_get_int(&recv_buffer[19], 2);
            for(i=0; i<num; i++){
                strncpy(datetime, &recv_buffer[21 + i*14], 14);
                change_statusflag(datetime,'0');
            }
        }

        //��flag=2�����ݸ�Ϊflag=1
        change_statusflag1();
        closesocket(sockfd);

        //������б�־Ϊ0�����������
        delete_statusflag0();
        rt_free(recv_buffer);
        rt_free(send_buffer);
        return 0;
    }

}

/* ���ļ��в�ѯ�Ƿ����������쳣״̬ */
int exist_inverter_abnormal_status()
{
    int result = 0;
    //��ѯ�������ID�ڱ����Ƿ����
    if(1 == detection_statusflag('1')){
        result = 1;
    }
    return result;
}

/* [A123]ECU�ϱ���������쳣״̬ */
int response_inverter_abnormal_status()
{
    int result = 0;
    int  j, sockfd, flag, num = 0, cmd_id, next_cmd_id,havaflag;
    char datetime[15] = {'\0'};
    char *save_buffer = NULL;
    char *recv_buffer = NULL;
    char *command = NULL;
    char *send_buffer = NULL;
    char da_time[20]={'\0'};
    char *data = NULL;//��ѯ��������
    char time[15] = {'\0'};

    printmsg(ECU_DBG_CONTROL_CLIENT,">>Start Response Abnormal Status");

    recv_buffer = (char *)rt_malloc(4096);
    command = (char *)rt_malloc(4096);
    send_buffer = (char *)rt_malloc(1024);
    data = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
    save_buffer = malloc(MAXBUFFER);
    memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
    memset(save_buffer,'\0',MAXBUFFER);
    //����socket����
    sockfd = client_socket_init(randport(control_client_arg), control_client_arg.ip, control_client_arg.domain);
    if(sockfd < 0)
    {
#ifdef WIFI_USE	

        //��������ʧ�ܣ�ʹ��wifi����
        {
            int j = 0,flag_failed = 0;
            //����SOCKET�ɹ�
            //��������������쳣״̬
            while(search_statusflag(data,time,&havaflag,'1'))		//	��ȡһ��resendflagΪ1������
            {
                //����һ��������쳣״̬��Ϣ
                if(SendToSocketC(control_client_arg.ip,randport(control_client_arg),data, strlen(data)) < 0){
                    rt_free(recv_buffer);
                    rt_free(command);
                    rt_free(send_buffer);
                    free(data);
                    data = NULL;
                    free(save_buffer);
                    save_buffer = NULL;
                    return -1;
                }
                for(j=0;j<800;j++)
                {
                    if(WIFI_Recv_SocketC_Event == 1)
                    {
                        flag_failed = 1;
                        WIFI_Recv_SocketC_Event = 0;
                        break;
                    }
                    rt_thread_delay(1);
                }
                if(flag_failed == 0)
                {
                    rt_free(recv_buffer);
                    rt_free(command);
                    rt_free(send_buffer);
                    free(data);
                    data = NULL;
                    free(save_buffer);
                    save_buffer = NULL;
                    return -1;
                }

                //ȥ��usr WIFI���ĵ�ͷ��
                memcpy(recv_buffer,WIFI_RecvSocketCData,WIFI_Recv_SocketC_LEN);
                recv_buffer[WIFI_Recv_SocketC_LEN] = '\0';
                //У������
                if(msg_format_check(recv_buffer) < 0){
                    continue;
                }
                //�����ͺͽ��ܶ��ɹ�����һ��״̬�ı�־��2
                change_statusflag(time,'2');
                //�����յ���ʱ���,��ɾ��EMA�Ѵ�����
                flag = msg_get_int(&recv_buffer[18], 1);
                num = 0;
                if(flag){
                    num = msg_get_int(&recv_buffer[19], 2);
                    for(j=0; j<num; j++){
                        strncpy(datetime, &recv_buffer[21 + j*14], 14);
                        change_statusflag(datetime,'0');
                    }
                }

                //�ж�Ӧ��֡�Ƿ񸽴�����
                if(strlen(recv_buffer) > (24 + 14*num)){
                    memset(command, '\0', sizeof(command));
                    strncpy(command, &recv_buffer[24 + 14*num], (strlen(recv_buffer) - (24+14*num)));
                    command[strlen(recv_buffer) - (24+14*num)] = '\0';
                    print2msg(ECU_DBG_CONTROL_CLIENT,"Command", command);
                    //У������
                    if(msg_format_check(command) < 0)
                        continue;
                    //���������
                    cmd_id = msg_cmd_id(command);

                    if(cmd_id==118)
                    {
                        strncpy(da_time, &recv_buffer[72],14);

                        WritePage(INTERNAL_FLASH_A118,"1",1);
                        memset(send_buffer,0x00,1024);
                        msg_ACK(send_buffer, "A118", da_time, 0);
                        SendToSocketC(control_client_arg.ip,randport(control_client_arg),send_buffer, strlen(send_buffer));
                        printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
                        printdecmsg(ECU_DBG_CONTROL_CLIENT,"socked",sockfd);
                        result=1;break;
                    }
                    //���ú���
                    else if(pfun[cmd_id%100]){
                        next_cmd_id = (*pfun[cmd_id%100])(command, save_buffer);
                        save_process_result(cmd_id, save_buffer);
                        if(next_cmd_id > 0){
                            memset(command, 0, sizeof(command));
                            snprintf(recv_buffer, 51+1, "APS13AAA51A101AAA0000000000000A%3d00000000000000END", next_cmd_id);
                            (*pfun[next_cmd_id%100])(command, save_buffer);
                            save_process_result(next_cmd_id, save_buffer);
                        }
                        else if(next_cmd_id < 0){
                            result = -1;
                        }
                    }
                }
                memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
            }
            //���inversta��flag��־λΪ0�ı�־
            delete_statusflag0();
            rt_free(recv_buffer);
            rt_free(command);
            rt_free(send_buffer);
            free(data);
            data = NULL;
            free(save_buffer);
            save_buffer = NULL;
            return result;

        }
#endif
#ifndef WIFI_USE
        rt_free(recv_buffer);
        rt_free(command);
        rt_free(send_buffer);
        free(data);
        data = NULL;
        free(save_buffer);
        save_buffer = NULL;
        return 0;
#endif
    }
    else
    {
        //��������������쳣״̬
        while(search_statusflag(data,time,&havaflag,'1'))		//	��ȡһ��resendflagΪ1������
        {
            //����һ��������쳣״̬��Ϣ
            if(send_socket(sockfd, data, strlen(data)) < 0){
                continue;
            }
            //����EMAӦ��
            if(recv_socket(sockfd, recv_buffer, sizeof(recv_buffer), control_client_arg.timeout) <= 0){
                closesocket(sockfd);
                rt_free(recv_buffer);
                rt_free(command);
                rt_free(send_buffer);
                free(data);
                data = NULL;
                free(save_buffer);
                save_buffer = NULL;
                return -1;
            }
            //У������
            if(msg_format_check(recv_buffer) < 0){
                continue;
            }
            //�����ͺͽ��ܶ��ɹ�����һ��״̬�ı�־��2
            change_statusflag(time,'2');
            //�����յ���ʱ���,��ɾ��EMA�Ѵ�����
            flag = msg_get_int(&recv_buffer[18], 1);
            num = 0;
            if(flag){
                num = msg_get_int(&recv_buffer[19], 2);
                for(j=0; j<num; j++){
                    strncpy(datetime, &recv_buffer[21 + j*14], 14);
                    change_statusflag(datetime,'0');
                }
            }

            //�ж�Ӧ��֡�Ƿ񸽴�����
            if(strlen(recv_buffer) > (24 + 14*num)){
                memset(command, '\0', sizeof(command));
                strncpy(command, &recv_buffer[24 + 14*num], (strlen(recv_buffer) - (24+14*num)));
                command[strlen(recv_buffer) - (24+14*num)] = '\0';
                print2msg(ECU_DBG_CONTROL_CLIENT,"Command", command);

                //У������
                if(msg_format_check(command) < 0)
                    continue;
                //���������
                cmd_id = msg_cmd_id(command);

                if(cmd_id==118)
                {
                    strncpy(da_time, &recv_buffer[72],14);

                    WritePage(INTERNAL_FLASH_A118,"1",1);
                    memset(send_buffer,0x00,1024);
                    msg_ACK(send_buffer, "A118", da_time, 0);
                    send_socket(sockfd, send_buffer, strlen(send_buffer));
                    printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
                    printdecmsg(ECU_DBG_CONTROL_CLIENT,"socked",sockfd);
                    result=1;break;
                }
                //���ú���
                else if(pfun[cmd_id%100]){
                    next_cmd_id = (*pfun[cmd_id%100])(command, save_buffer);
                    save_process_result(cmd_id, save_buffer);
                    if(next_cmd_id > 0){
                        memset(command, 0, sizeof(command));
                        snprintf(recv_buffer, 51+1, "APS13AAA51A101AAA0000000000000A%3d00000000000000END", next_cmd_id);
                        (*pfun[next_cmd_id%100])(command, save_buffer);
                        save_process_result(next_cmd_id, save_buffer);
                    }
                    else if(next_cmd_id < 0){
                        result = -1;
                    }
                }
            }
            memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
        }
        //���inversta��flag��־λΪ0�ı�־
        delete_statusflag0();
        closesocket(sockfd);
        rt_free(recv_buffer);
        rt_free(command);
        rt_free(send_buffer);
        free(data);
        data = NULL;
        free(save_buffer);
        save_buffer = NULL;
        return result;
    }

}

/* ��EMA����ͨѶ */
int communication_with_EMA(int next_cmd_id)
{
    int sockfd;
    int cmd_id;
    char timestamp[15] = "00000000000000";
    int one_a118=0;
    char *recv_buffer = NULL;
    char *send_buffer = NULL;

    recv_buffer = rt_malloc(2048);
    send_buffer = rt_malloc(MAXBUFFER);

    while(1)
    {
        printmsg(ECU_DBG_CONTROL_CLIENT,"Start Communication with EMA");
        sockfd = client_socket_init(randport(control_client_arg), control_client_arg.ip, control_client_arg.domain);
        if(sockfd < 0)
        {
#ifdef WIFI_USE	

            //��������ʧ�ܣ�ʹ��wifi����
            {
                int j =0,flag_failed = 0,ret = 0;
                if(next_cmd_id <= 0)
                {
                    //ECU��EMA������������ָ��
                    msg_REQ(send_buffer);
                    ret = SendToSocketC(control_client_arg.ip,randport(control_client_arg),send_buffer, strlen(send_buffer));
                    if(ret == -1)
                    {
                        rt_free(recv_buffer);
                        rt_free(send_buffer);
                        return -1;
                    }
                    memset(send_buffer, '\0', sizeof(send_buffer));
                    for(j = 0;j<800;j++)
                    {
                        if(WIFI_Recv_SocketC_Event == 1)
                        {
                            flag_failed = 1;
                            WIFI_Recv_SocketC_Event = 0;
                            break;
                        }
                        rt_thread_delay(1);
                    }
                    if(flag_failed == 0)
                    {
                        rt_free(recv_buffer);
                        rt_free(send_buffer);
                        return -1;
                    }

                    //ȥ��usr WIFI���ĵ�ͷ��
                    memcpy(recv_buffer,WIFI_RecvSocketCData,WIFI_Recv_SocketC_LEN);
                    recv_buffer[WIFI_Recv_SocketC_LEN] = '\0';
                    print2msg(ECU_DBG_CONTROL_CLIENT,"communication_with_EMA recv",recv_buffer);
                    //У������
                    if(msg_format_check(recv_buffer) < 0){
                        continue;
                    }
                    //���������
                    cmd_id = msg_cmd_id(recv_buffer);
                }
                else{
                    //������һ������(�����������������,�ϱ����ú��ECU״̬)
                    cmd_id = next_cmd_id;
                    next_cmd_id = 0;
                    memset(recv_buffer, 0, sizeof(recv_buffer));
                    snprintf(recv_buffer, 51+1, "APS13AAA51A101AAA0%.12sA%3d%.14sEND",
                             ecu.id, cmd_id, timestamp);
                }
                //ECUע�����κ�EMAͨѶ
                if(cmd_id == 118){
                    if(one_a118==0){
                        one_a118=1;
                    }
                    strncpy(timestamp, &recv_buffer[34], 14);
                    next_cmd_id = first_time_info(recv_buffer, send_buffer);
                    if(next_cmd_id == 0){
                        strncpy(timestamp, "00000000000000", 14);
                        //break;
                    }
                }
                //��������ŵ��ú���
                else if(pfun[cmd_id%100]){
                    //�����ú���������Ϻ���Ҫִ���ϱ�,��᷵���ϱ������������,���򷵻�0
                    next_cmd_id = (*pfun[cmd_id%100])(recv_buffer, send_buffer);
                }
                //EMA��������
                else if(cmd_id == 100){
                    break;
                }
                else{
                    //������Ų�����,��������ʧ��Ӧ��(ÿ������Э���ʱ���λ�ò�ͳһ,����ʱ����Ǹ�����...)
                    memset(send_buffer, 0, sizeof(send_buffer));
                    snprintf(send_buffer, 52+1, "APS13AAA52A100AAA0%sA%3d000000000000002END\n",
                             ecu.id, cmd_id);
                }
                //����Ϣ���͸�EMA(�Զ����㳤��,���ϻس�)
                SendToSocketC(control_client_arg.ip,randport(control_client_arg),send_buffer, strlen(send_buffer));
                printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
                //������ܺ�������ֵС��0,�򷵻�-1,������Զ��˳�
                if(next_cmd_id < 0){
                    rt_free(recv_buffer);
                    rt_free(send_buffer);
                    return -1;
                }
            }
#endif

#ifndef WIFI_USE
            break;
#endif
        }
        else
        {
            if(next_cmd_id <= 0)
            {
                //ECU��EMA������������ָ��
                msg_REQ(send_buffer);
                send_socket(sockfd, send_buffer, strlen(send_buffer));
                memset(send_buffer, '\0', sizeof(send_buffer));

                //����EMA����������
                if(recv_socket(sockfd, recv_buffer, sizeof(recv_buffer), control_client_arg.timeout) < 0){
                    closesocket(sockfd);
                    rt_free(recv_buffer);
                    rt_free(send_buffer);
                    return -1;
                }
                //У������
                if(msg_format_check(recv_buffer) < 0){
                    closesocket(sockfd);
                    continue;
                }
                //���������
                cmd_id = msg_cmd_id(recv_buffer);
            }
            else{
                //������һ������(�����������������,�ϱ����ú��ECU״̬)
                cmd_id = next_cmd_id;
                next_cmd_id = 0;
                memset(recv_buffer, 0, sizeof(recv_buffer));
                snprintf(recv_buffer, 51+1, "APS13AAA51A101AAA0%.12sA%3d%.14sEND",
                         ecu.id, cmd_id, timestamp);
            }

            //ECUע�����κ�EMAͨѶ
            if(cmd_id == 118){
                if(one_a118==0){
                    one_a118=1;
                }
                strncpy(timestamp, &recv_buffer[34], 14);
                next_cmd_id = first_time_info(recv_buffer, send_buffer);
                if(next_cmd_id == 0){
                    strncpy(timestamp, "00000000000000", 14);
                }
            }
            //��������ŵ��ú���
            else if(pfun[cmd_id%100]){
                //�����ú���������Ϻ���Ҫִ���ϱ�,��᷵���ϱ������������,���򷵻�0
                next_cmd_id = (*pfun[cmd_id%100])(recv_buffer, send_buffer);
            }
            //EMA��������
            else if(cmd_id == 100){
                closesocket(sockfd);
                break;
            }
            else{
                //������Ų�����,��������ʧ��Ӧ��(ÿ������Э���ʱ���λ�ò�ͳһ,����ʱ����Ǹ�����...)
                memset(send_buffer, 0, sizeof(send_buffer));
                snprintf(send_buffer, 52+1, "APS13AAA52A100AAA0%sA%3d000000000000002END",
                         ecu.id, cmd_id);
            }
            //����Ϣ���͸�EMA(�Զ����㳤��,���ϻس�)
            send_socket(sockfd, send_buffer, strlen(send_buffer));
            printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
            closesocket(sockfd);

            //������ܺ�������ֵС��0,�򷵻�-1,������Զ��˳�
            if(next_cmd_id < 0){
                rt_free(recv_buffer);
                rt_free(send_buffer);
                return -1;
            }
        }
    }
    printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
    rt_free(recv_buffer);
    rt_free(send_buffer);
    return 0;
}

/* �ϱ�process_result���е���Ϣ */
int response_process_result()
{
    char *sendbuffer = NULL;
    int sockfd, flag;
    char inverterId[13] = {'\0'};
    //int item_num[32] = {0};
    char *data = NULL;//��ѯ��������
    char item[4] = {'\0'};

    sendbuffer = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
    data = malloc(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
    memset(sendbuffer,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
    memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);

    {
        //��ѯ����[ECU����]������
        //�����ϱ�ECU��������
        while(search_pro_result_flag(data,item,&flag,'1'))
        {
            printmsg(ECU_DBG_CONTROL_CLIENT,">>Start Response ECU Process Result");
            sockfd = client_socket_init(randport(control_client_arg), control_client_arg.ip, control_client_arg.domain);
            if(sockfd < 0)
            {
#ifdef WIFI_USE	

                //��������ʧ�ܣ�ʹ��wifi����
                {
                    //����һ����¼
                    if(SendToSocketC(control_client_arg.ip,randport(control_client_arg),data, strlen(data)) < 0){
                        break;
                    }
                    //���ͳɹ��򽫱�־λ��0
                    change_pro_result_flag(item,'0');
                    printmsg(ECU_DBG_CONTROL_CLIENT,">>End");

                }
#endif		

#ifndef WIFI_USE
                break;
#endif

            }else
            {
                //����һ����¼
                if(send_socket(sockfd, data, strlen(data)) < 0){
                    closesocket(sockfd);
                    continue;
                }
                //���ͳɹ��򽫱�־λ��0
                change_pro_result_flag(item,'0');
                closesocket(sockfd);
                printmsg(ECU_DBG_CONTROL_CLIENT,">>End");
            }
            memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
        }
        delete_pro_result_flag0();

        while(search_inv_pro_result_flag(data,item,inverterId,&flag,'1'))
        {
            char msg_length[6] = {'\0'};
            //ƴ������

            memset(sendbuffer, 0, sizeof(sendbuffer));
            sprintf(sendbuffer, "APS1300000A%03dAAA0%.12s%04d00000000000000END", atoi(item), ecu.id, 1);
            strcat(sendbuffer, data);

            if(sendbuffer[strlen(sendbuffer)-1] == '\n'){
                sprintf(msg_length, "%05d", strlen(sendbuffer)-1);
            }
            else{
                sprintf(msg_length, "%05d", strlen(sendbuffer));
                strcat(sendbuffer, "\n");
            }
            strncpy(&sendbuffer[5], msg_length, 5);
            //��������
            printmsg(ECU_DBG_CONTROL_CLIENT,">>Start Response Inverter Process Result");
            sockfd = client_socket_init(randport(control_client_arg), control_client_arg.ip, control_client_arg.domain);
            if(sockfd < 0)
            {
#ifdef WIFI_USE	

                //��������ʧ�ܣ�ʹ��wifi����
                {
                    if(SendToSocketC(control_client_arg.ip,randport(control_client_arg),sendbuffer, strlen(sendbuffer)) < 0){
                        break;
                    }
                    change_inv_pro_result_flag(item,'0');

                }
#endif	

#ifndef WIFI_USE
                break;
#endif
            }else
            {
                if(send_socket(sockfd, sendbuffer, strlen(sendbuffer)) < 0){
                    closesocket(sockfd);
                    continue;
                }
                change_inv_pro_result_flag(item,'0');
                closesocket(sockfd);
            }
            memset(data,'\0',MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL);
        }
        delete_inv_pro_result_flag0();

    }
    free(sendbuffer);
    sendbuffer = NULL;
    free(data);
    data = NULL;
    return 0;
}

void control_client_thread_entry(void* parameter)
{
    int result, ecu_time = 0, ecu_flag = 1;
    char c='0';
    rt_thread_delay(RT_TICK_PER_SECOND*START_TIME_CONTROL_CLIENT);
    //��ӹ��ܺ���
    add_functions();

    ReadPage(INTERNAL_FLASH_ECU_FLAG,&c,1);
    if(c == '0')
    {
        ecu_flag = 0;
    }

    printdecmsg(ECU_DBG_CONTROL_CLIENT,"ecu_flag", ecu_flag);

    /* ECU��ѵ��ѭ�� */
    while(1)
    {
        //ÿ��һ��ʱ��EMAȷ��������쳣״̬�Ƿ񱻴洢
        check_inverter_abnormal_status_sent(1);

        ReadPage(INTERNAL_FLASH_A118,&c,1);
        if(c=='1')
        {
            printf("A118:%c\n",c);
            result = communication_with_EMA(118);
            if(result != -1)
            {
                WritePage(INTERNAL_FLASH_A118,"0",1);
            }

        }


        ReadPage(INTERNAL_FLASH_ECUUPVER,&c,1);
        if(c=='1')
        {
            printf("A102:%c\n",c);
            result = communication_with_EMA(102);
            if(result != -1)
            {
                WritePage(INTERNAL_FLASH_ECUUPVER,"0",1);
            }
        }

        if(exist_inverter_abnormal_status() && ecu_flag){
            ecu_time =  acquire_time();
            result = response_inverter_abnormal_status();
            printdecmsg(ECU_DBG_CONTROL_CLIENT,"result",result);
            response_process_result();
        }
        else if(compareTime(acquire_time() ,ecu_time,60*control_client_arg.report_interval)){
            ecu_time = acquire_time();
            if(ecu_flag){ //���ecu_flag = 0 ���ϱ�������
                response_process_result();
            }
            result = communication_with_EMA(0);
        }
        //����������������ѭ��
        if(result < 0){
            result = 0;
            printmsg(ECU_DBG_CONTROL_CLIENT,"Quit control_client");
            rt_thread_delay(300 * RT_TICK_PER_SECOND);
            continue;
        }
        rt_thread_delay(RT_TICK_PER_SECOND*control_client_arg.report_interval*60/3);

    }

}

#ifdef RT_USING_FINSH
#include <finsh.h>
void commEMA(int i)
{
    communication_with_EMA(i);
}

void prealarmprocess(void)
{

    check_inverter_abnormal_status_sent(get_hour());
}

void respabnormal(void)
{
    response_inverter_abnormal_status();
}
FINSH_FUNCTION_EXPORT(commEMA, eg:commEMA());
FINSH_FUNCTION_EXPORT(prealarmprocess, eg:prealarmprocess());
FINSH_FUNCTION_EXPORT(respabnormal, eg:respabnormal());
#endif

