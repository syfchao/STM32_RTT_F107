/*****************************************************************************/
/* File      : client.c                                                      */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-20 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
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
#include "threadlist.h"
#include "variation.h"
#include "usart5.h"
#include "lan8720rst.h"
#include "clientSocket.h"
#include "datelist.h"

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern rt_mutex_t record_data_lock;
extern ecu_info ecu;

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
int clear_send_flag(char *readbuff)
{
    int i, j, count;		//EMA���ض��ٸ�ʱ��(�м���ʱ��,��˵��EMA�����˶�������¼)
    char recv_date_time[16];

    if(strlen(readbuff) >= 3)
    {
        count = (readbuff[1] - 0x30) * 10 + (readbuff[2] - 0x30);
        if(count == (strlen(readbuff) - 3) / 14)
        {
            for(i=0; i<count; i++)
            {
                memset(recv_date_time, '\0', sizeof(recv_date_time));
                strncpy(recv_date_time, &readbuff[3+i*14], 14);

                for(j=0; j<3; j++)
                {
                    if(1 == change_resendflag(recv_date_time,'0'))
                    {
                        print2msg(ECU_DBG_CLIENT,"Clear send flag into database", "1");
                        break;
                    }
                    else
                        print2msg(ECU_DBG_CLIENT,"Clear send flag into database", "0");
                    //rt_thread_delay(100);
                }
            }
        }
    }

    return 0;
}

int update_send_flag(char *send_date_time)
{
    int i;
    for(i=0; i<3; i++)
    {
        if(1 == change_resendflag(send_date_time,'2'))
        {
            print2msg(ECU_DBG_CLIENT,"Update send flag into database", "1");
            break;
        }
        rt_thread_delay(100);
    }

    return 0;
}

int detection_resendflag2()		//���ڷ���1�������ڷ���0
{
    DIR *dirp;
    char dir[30] = "/home/record/data";
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
        dirp = opendir("/home/record/data");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CLIENT,"detection_resendflag2 open directory error");
            mkdir("/home/record/data",0);
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
            fp = fopen(path, "r");
            if(fp)
            {
                while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))
                {
                    if(strlen(buff) > 18)
                    {
                        //����Ƿ���ϸ�ʽ
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                        {
                            if(buff[strlen(buff)-2] == '2')			//������һ���ֽڵ�resendflag�Ƿ�Ϊ2   ���������2  �ر��ļ�����return 1
                            {
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

int change_resendflag(char *time,char flag)  //�ı�ɹ�����1��δ�ҵ���ʱ��㷵��0
{
    DIR *dirp;
    char dir[30] = "/home/record/data";
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
        dirp = opendir("/home/record/data");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CLIENT,"change_resendflag open directory error");
            mkdir("/home/record/data",0);
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
                        if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                        {
                            memset(filetime,0,15);
                            memcpy(filetime,&buff[strlen(buff)-17],14);				//��ȡÿ����¼��ʱ��
                            filetime[14] = '\0';
                            if(!memcmp(time,filetime,14))						//ÿ����¼��ʱ��ʹ����ʱ��Աȣ�����ͬ����flag
                            {
                                fseek(fp,-2L,SEEK_CUR);
                                fputc(flag,fp);
                                //print2msg(ECU_DBG_CLIENT,"change_resendflag",filetime);
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

//��ѯһ��resendflagΪ1������   ��ѯ���˷���1  ���û��ѯ������0
/*
data:��ʾ��ȡ��������
time����ʾ��ȡ����ʱ��
flag����ʾ�Ƿ�����һ������   ������һ��Ϊ1   ������Ϊ0
*/
int search_readflag(char *data,char * time, int *flag,char sendflag)
{
    DIR *dirp;
    char dir[30] = "/home/record/data";
    struct dirent *d;
    char path[100];
    struct list *Head = NULL;
    char *buff = NULL;
    struct list *tmp;
    FILE *fp;
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
        dirp = opendir("/home/record/data");
        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CLIENT,"search_readflag open directory error");
            mkdir("/home/record/data",0);
        }else
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
                                    //print2msg(ECU_DBG_CLIENT,"search_readflag time",time);
                                    //print2msg(ECU_DBG_CLIENT,"search_readflag data",data);
                                    while(NULL != fgets(buff,(MAXINVERTERCOUNT*RECORDLENGTH+RECORDTAIL+18),fp))	//�����¶����ݣ�Ѱ���Ƿ���Ҫ���͵�����
                                    {
                                        if(strlen(buff) > 18)
                                        {
                                            if((buff[strlen(buff)-3] == ',') && (buff[strlen(buff)-18] == ',') )
                                            {
                                                if(buff[strlen(buff)-2] == sendflag)
                                                {
                                                    *flag = 1;		//���ڻ���Ҫ���͵�����
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
                                    nextfileflag = 1;	//��ǰ�ļ������ڻ���Ҫ�ϴ������ݣ���Ҫ�ں����ļ��в���
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


void delete_file_resendflag0()		//�������resend��־ȫ��Ϊ0��Ŀ¼
{
    DIR *dirp;
    char dir[30] = "/home/record/data";
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
        dirp = opendir("/home/record/data");

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_CLIENT,"delete_file_resendflag0 open directory error");
            mkdir("/home/record/data",0);
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
            flag = 0;
            //print2msg(ECU_DBG_CLIENT,"delete_file_resendflag0 ",path);
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
                    print2msg(ECU_DBG_CLIENT,"unlink:",path);
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



int send_record(char *sendbuff, char *send_date_time)			//�������ݵ�EMA  ע���ڴ洢��ʱ���βδ���'\n'  �ڷ���ʱ��ʱ��ǵ����
{
    int sendbytes=0,readbytes = 4+99*14;
    char *readbuff = NULL;
    readbuff = malloc((4+99*14));
    memset(readbuff,'\0',(4+99*14));

    sendbytes = serverCommunication_Client(sendbuff,strlen(sendbuff),readbuff,&readbytes,10000);
    if(-1 == sendbytes)
    {
        free(readbuff);
        readbuff = NULL;
        return -1;
    }

    if(readbytes < 3)
    {
        free(readbuff);
        readbuff = NULL;
        return -1;
    }
    else
    {
        //print2msg(ECU_DBG_CLIENT,"readbuff",readbuff);
        if('1' == readbuff[0])
            update_send_flag(send_date_time);
        clear_send_flag(readbuff);
        free(readbuff);
        readbuff = NULL;
        return 0;
    }
}


int preprocess()			//����ͷ��Ϣ��EMA,��ȡ�Ѿ�����EMA�ļ�¼ʱ��
{
    int sendbytes = 0,readbytes = 0;
    char *readbuff = NULL;
    char sendbuff[50] = {'\0'};

    if(0 == detection_resendflag2())		//	����Ƿ���resendflag='2'�ļ�¼
        return 0;
    readbuff = malloc((4+99*14));
    memset(readbuff,0x00,(4+99*14));
    readbytes = 4+99*14;
    strcpy(sendbuff, "APS13AAA22");
    memcpy(&sendbuff[10],ecu.id,12);
    strcat(sendbuff, "\n");
    print2msg(ECU_DBG_CLIENT,"Sendbuff", sendbuff);

    //���͵�������
    sendbytes = serverCommunication_Client(sendbuff,strlen(sendbuff),readbuff,&readbytes,10000);
    if(-1 == sendbytes)
    {
        free(readbuff);
        readbuff = NULL;
        return -1;
    }
    if(readbytes >3)
    {
        clear_send_flag(readbuff);

    }else
    {
        free(readbuff);
        readbuff = NULL;
        return -1;
    }
    free(readbuff);
    readbuff = NULL;
    return 0;

}

int resend_record()
{
    char *data = NULL;//��ѯ��������
    char time[15] = {'\0'};
    int flag,res;

    data = malloc(CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER);
    memset(data,0x00,CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER);
    //��/home/record/data/Ŀ¼�²�ѯresendflagΪ2�ļ�¼
    while(search_readflag(data,time,&flag,'2'))		//	��ȡһ��resendflagΪ1������
    {
        //if(1 == flag)		// ��������Ҫ�ϴ�������
        //		data[78] = '1';
        printmsg(ECU_DBG_CLIENT,data);
        res = send_record(data, time);
        if(-1 == res)
            break;
        memset(data,0,(CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER));
        memset(time,0,15);
    }

    free(data);
    data = NULL;
    return 0;

}


void client_thread_entry(void* parameter)
{
    char broadcast_hour_minute[3]={'\0'};
    char broadcast_time[16];
    int thistime=0, lasttime=0,res,flag;
    char *data = NULL;//��ѯ��������
    char time[15] = {'\0'};
    rt_thread_delay(RT_TICK_PER_SECOND*START_TIME_CLIENT);
    printmsg(ECU_DBG_CLIENT,"Started");

    data = malloc(CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER);
    memset(data,0x00,CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER);
    while(1)
    {
        thistime = lasttime = acquire_time();
        printmsg(ECU_DBG_CLIENT,"Client Start****************************************");
        if((2 == get_hour())||(1 == get_hour()))
            //if(1)
        {
            preprocess();		//Ԥ����,�ȷ���һ��ͷ��Ϣ��EMA,��EMA�ѱ��Ϊ2�ļ�¼��ʱ�䷵��ECU,Ȼ��ECU�ٰѱ��Ϊ2�ļ�¼�ٴη��͸�EMA,��ֹEMA�յ���¼�����յ���־��û�д������ݿ�������
            resend_record();
            delete_file_resendflag0();		//�������resend��־ȫ��Ϊ0��Ŀ¼
        }

        get_time(broadcast_time, broadcast_hour_minute);
        print2msg(ECU_DBG_CLIENT,"time",broadcast_time);

        while(search_readflag(data,time,&flag,'1'))		//	��ȡһ��resendflagΪ1������
        {
            if(compareTime(thistime ,lasttime,300))
            {
                break;
            }
            //if(1 == flag)		// ��������Ҫ�ϴ�������
            //data[78] = '1';
            printmsg(ECU_DBG_CLIENT,data);
            res = send_record( data, time);
            if(-1 == res)
                break;
            thistime = acquire_time();
            memset(data,0,(CLIENT_RECORD_HEAD+CLIENT_RECORD_ECU_HEAD+CLIENT_RECORD_INVERTER_LENGTH*MAXINVERTERCOUNT+CLIENT_RECORD_OTHER));
            memset(time,0,15);
        }
        delete_file_resendflag0();		//�������resend��־ȫ��Ϊ0��Ŀ¼
        printmsg(ECU_DBG_CLIENT,"Client End****************************************");
        if((thistime < 300) && (lasttime > 300))
        {
            if((thistime+(24*60*60+1)-lasttime) < 300)
            {
                rt_thread_delay(300 - ((thistime+(24*60*60+1)) + lasttime) * RT_TICK_PER_SECOND);
            }

        }else
        {
            if((thistime-lasttime) < 300)
            {
                rt_thread_delay((300 + lasttime - thistime) * RT_TICK_PER_SECOND);
            }
        }

    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
int testDIR(void)
{
    DIR *dirp;
    struct dirent *d;
    struct list *Head = NULL;

    char data_str[9] = {'\0'};
    Head = list_create(-1);
    dirp = opendir("/test");
    if(dirp == RT_NULL)
    {
        printmsg(ECU_DBG_CLIENT,"search_readflag open directory error");
        mkdir("/test",0);
    }else
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
    list_print(Head);
    //��ȡ����

    list_destroy(&Head);
    return 0;
}
FINSH_FUNCTION_EXPORT(testDIR, eg:testDIR());

#endif
