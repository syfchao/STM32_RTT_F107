/*****************************************************************************/
/*  File      : file.c                                                       */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-05 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "file.h"
#include "checkdata.h"
#include <dfs_posix.h> 
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datetime.h"
#include "SEGGER_RTT.h"
#include "dfs_fs.h"
#include "rthw.h"
#include "myfile.h"
#include "variation.h"
#include "usart5.h"
#include "arch/sys_arch.h"
#include "InternalFlash.h"

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern rt_mutex_t record_data_lock;
extern inverter_info inverter[MAXINVERTERCOUNT];
extern ecu_info ecu;

int ecu_type;	//1:SAA; 2:NA; 3:MX

#define EPSILON 0.000000001

//day_tab[0]   ƽ��ÿ��������    day_tab[1]   ����ÿ��������   
int day_tab[2][12]={{31,28,31,30,31,30,31,31,30,31,30,31},{31,29,31,30,31,30,31,31,30,31,30,31}}; 

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
int fileopen(const char *file, int flags, int mode)
{
    return open(file,flags,mode);
}

int fileclose(int fd)
{
    return close(fd);
}

int fileWrite(int fd,char* buf,int len)
{
    return write( fd, buf, len );
}

int fileRead(int fd,char* buf,int len)
{
    return read( fd, buf, len );
}

//Ŀ¼��⣬��������ھʹ������ڵ�Ŀ¼
void dirDetection(char *path)
{
    DIR *dirp;

    //���ж��Ƿ�������Ŀ¼������������򴴽���Ŀ¼
    dirp = opendir(path);
    if(dirp == RT_NULL){	//��Ŀ¼ʧ�ܣ���Ҫ������Ŀ¼
        mkdir(path,0);
    }else{					//��Ŀ¼�ɹ����ر�Ŀ¼��������
        closedir(dirp);
    }
}

void sysDirDetection(void)
{
    rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    dirDetection("/home");
    dirDetection("/tmp");
    dirDetection("/ftp");
    dirDetection("/yuneng");
    dirDetection("/home/data");
    dirDetection("/home/record");
    dirDetection("/home/data/proc_res");
    dirDetection("/home/data/IPROCRES");
    dirDetection("/home/record/data");
    dirDetection("/home/record/inversta");
    dirDetection("/home/record/power");
    dirDetection("/home/record/eventdir");
    dirDetection("/home/record/energy");
    rt_mutex_release(record_data_lock);
}


int get_Passwd(char *PassWD)
{
    int ret = 0;
    char lenstr[4] = {'\0'};
    int len = 0;
    char *passwd = NULL;
    passwd = malloc(128);
	
    ret = ReadPage(INTERNAL_FLASH_WIFI_PASSWORD,passwd,128);
    if(0 == ret)
    {
        memcpy(lenstr,passwd,3);
        len = atoi(lenstr);
        memcpy(PassWD,&passwd[3],len);
    }
    

    free(passwd);
    passwd = NULL;
    return ret;

}

int set_Passwd(char *PassWD,int length)
{
    int ret = 0;
    char *passwd = NULL;
    passwd = malloc(128);
    memset(passwd,0x00,128);
    sprintf(passwd,"%03x%s",length,PassWD);
    ret = WritePage(INTERNAL_FLASH_WIFI_PASSWORD,passwd,length+3);
    free(passwd);
    passwd = NULL;
    return ret;
   
}

//�ָ�ո�
void splitSpace(char *data,char *sourcePath,char *destPath)
{
    int i,j = 0,k = 0;
    char splitdata[3][50]={0x00};
    for(i=0;i<strlen(data);++i){
        if(data[i] == ' ') {
            splitdata[j][k] = 0;
            ++j;
            k = 0;
        }
        else{
            splitdata[j][k] = data[i];
            ++k;
        }
    }

    memcpy(sourcePath,splitdata[1],strlen(splitdata[1]));
    sourcePath[strlen(splitdata[1])] = '\0';
    memcpy(destPath,splitdata[2],strlen(splitdata[2]));
    destPath[strlen(splitdata[2])] = '\0';
}


//����0��ʾDHCPģʽ  ����1��ʾ��̬IPģʽ
int get_DHCP_Status(void)
{
    char buff[50] = {0x00};
    ReadPage(INTERNAL_FLASH_IPCONFIG,buff,50);
    if (buff[0] == '1')
    {
        return 1;
    }else
    {
        return 0;
    }

}

int get_ecu_type()			
{
    char buff[13] = {'\0'};
    ecu_type = 1;

    ReadPage(INTERNAL_FALSH_AREA,buff,10);
    if(buff[0] != 0xff)
    {
        if(!strncmp(&buff[0], "MX", 2))
            ecu_type = 3;
        else if(!strncmp(&buff[0], "NA", 2))
            ecu_type = 2;
        else
            ecu_type = 1;
    }
    printdecmsg(ECU_DBG_MAIN,"ecu_type",ecu_type);
    return 0;
}

void get_ecuid(char *ecuid)
{
    char internalECUID[13] = {'\0'};

    ReadPage(INTERNAL_FALSH_ID,internalECUID,12);
    if(internalECUID[0] == '2')
    {
        memcpy(ecuid,internalECUID,12);
    }
    return;
}
//��ȡDRM���غ���   ����ֵΪ1����ʾ���ܴ�   ����ֵΪ-1��ʾ���ܹر�
int DRMFunction(void)
{
    char buff[8] = {'\0'};
    ReadPage(INTERNAL_FALSH_AREA,buff,8);
    if(!memcmp(buff,"SAA",3))
    {
        return 1;
    }
    return -1;
}

unsigned short get_panid(void)
{
    unsigned short ret = 0;
    char buff[17] = {'\0'};
    ReadPage(INTERNAL_FALSH_MAC,buff,17);

    if((buff[12]>='0') && (buff[12]<='9'))
        buff[12] -= 0x30;
    if((buff[12]>='A') && (buff[12]<='F'))
        buff[12] -= 0x37;
    if((buff[13]>='0') && (buff[13]<='9'))
        buff[13] -= 0x30;
    if((buff[13]>='A') && (buff[13]<='F'))
        buff[13] -= 0x37;
    if((buff[15]>='0') && (buff[15]<='9'))
        buff[15] -= 0x30;
    if((buff[15]>='A') && (buff[15]<='F'))
        buff[15] -= 0x37;
    if((buff[16]>='0') && (buff[16]<='9'))
        buff[16] -= 0x30;
    if((buff[16]>='A') && (buff[16]<='F'))
        buff[16] -= 0x37;
    ret = ((buff[12]) * 16 + (buff[13])) * 256 + (buff[15]) * 16 + (buff[16]);

    return ret;
}
char get_channel(void)
{
    char ret = 0;
    char buff[5] = {'\0'};
	
    ReadPage(INTERNAL_FLASH_CHANNEL,buff,5);

    if((buff[2]>='0') && (buff[2]<='9'))
        buff[2] -= 0x30;
    if((buff[2]>='A') && (buff[2]<='F'))
        buff[2] -= 0x37;
    if((buff[2]>='a') && (buff[2]<='f'))
        buff[2] -= 0x57;
    if((buff[3]>='0') && (buff[3]<='9'))
        buff[3] -= 0x30;
    if((buff[3]>='A') && (buff[3]<='F'))
        buff[3] -= 0x37;
    if((buff[3]>='a') && (buff[3]<='f'))
        buff[3] -= 0x57;
    ret = (buff[2]*16+buff[3]);

    return ret;
}

float get_lifetime_power(void)
{
    int fd;
    float lifetime_power = 0.0;
    char buff[20] = {'\0'};
    fd = open("/HOME/DATA/LTPOWER", O_RDONLY, 0);
    if (fd >= 0)
    {
        memset(buff, '\0', sizeof(buff));
        read(fd, buff, 20);
        close(fd);

        if('\n' == buff[strlen(buff)-1])
            buff[strlen(buff)-1] = '\0';
        lifetime_power = (float)atof(buff);
    }
    return lifetime_power;
}

void update_life_energy(float lifetime_power)
{
    int fd;
    char buff[20] = {'\0'};
    fd = open("/HOME/DATA/LTPOWER", O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd >= 0) {
        sprintf(buff, "%f", lifetime_power);
        write(fd, buff, 20);
        close(fd);
    }
}
void updateID(void)
{
    FILE *fp;
    int i;
    inverter_info *curinverter = inverter;
    //����/home/data/id����
    fp = fopen("/home/data/id","w");
    if(fp)
    {
        curinverter = inverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)			//��Ч�������ѵ
        {
            fprintf(fp,"%s,%d,%d,%d,%d,%d,%d\n",curinverter->id,curinverter->shortaddr,curinverter->model,curinverter->version,curinverter->inverterstatus.bindflag,curinverter->zigbee_version,curinverter->inverterstatus.flag);

        }
        fclose(fp);
    }
}

int update_tmpdb(inverter_info *firstinverter)
{
    int fd;
    char str[300] = {'\0'};
    inverter_info *curinverter = firstinverter;
    int i = 0;
    fd = fileopen("/home/data/collect.con",O_WRONLY | O_APPEND | O_CREAT|O_TRUNC,0);
    for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)
    {
        if(curinverter->inverterstatus.dataflag == 1)
        {
            sprintf(str,"%s,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d\n",curinverter->id,curinverter->preaccgen,curinverter->preaccgenb,curinverter->preaccgenc,curinverter->preaccgend,curinverter->pre_output_energy,curinverter->pre_output_energyb,curinverter->pre_output_energyc,curinverter->preacctime);
            fileWrite(fd,str,strlen(str));
        }
    }

    fileclose(fd);
    return 0;
}


int splitString(char *data,char splitdata[][32])
{
    int i,j = 0,k=0;

    for(i=0;i<strlen(data);++i){

        if(data[i] == ',') {
            splitdata[j][k] = 0;
            ++j;
            k = 0;
        }
        else{
            splitdata[j][k] = data[i];
            ++k;
        }
    }
    return j+1;

}

int get_id_from_file(inverter_info *firstinverter)
{

    int i,j,sameflag;
    inverter_info *inverter = firstinverter;
    inverter_info *curinverter = firstinverter;
    char list[20][32];
    char data[200];
    int num =0;
    FILE *fp;

    fp = fopen("/home/data/id", "r");
    if(fp)
    {
        while(NULL != fgets(data,200,fp))
        {
            //print2msg(ECU_DBG_MAIN,"ID",data);
            memset(list,0,sizeof(list));
            splitString(data,list);
            //�ж��Ƿ���ڸ������
            curinverter = firstinverter;
            sameflag=0;
            for(j=0; (j<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); j++)
            {
                if(!memcmp(list[0],curinverter->id,12))
                    sameflag = 1;
                curinverter++;
            }
            if(sameflag == 1)
            {
                continue;
            }

            strcpy(inverter->id, list[0]);

            if(0==strlen(list[1]))
            {
                inverter->shortaddr = 0;		//δ��ȡ���̵�ַ���������ֵΪ0.ZK
            }
            else
            {
                inverter->shortaddr = atoi(list[1]);
            }
            if(0==strlen(list[2]))
            {
                inverter->model = 0;		//δ��û�������������ֵΪ0.ZK
            }
            else
            {
                inverter->model = atoi(list[2]);
            }

            if(0==strlen(list[3]))
            {
                inverter->version = 0;		//����汾��û�б�־Ϊ0
            }
            else
            {
                inverter->version = atoi(list[3]);
            }

            if(0==strlen(list[4]))
            {
                inverter->inverterstatus.bindflag = 0;		//δ�󶨵�������ѱ�־λ��ֵΪ0.ZK
            }
            else
            {
                inverter->inverterstatus.bindflag = atoi(list[4]);
            }

            if(0==strlen(list[5]))
            {
                inverter->zigbee_version = 0;		//û�л�ȡ��zigbee�汾�ŵ��������ֵΪ0.ZK
            }
            else
            {
                inverter->zigbee_version = atoi(list[5]);
            }
            if(0==strlen(list[6]))
            {
                inverter->inverterstatus.flag = 0;
            }
            else
            {
                inverter->inverterstatus.flag = atoi(list[6]);
            }

            inverter++;
            num++;
            if(num >= MAXINVERTERCOUNT)
            {
                break;
            }
        }
        fclose(fp);
    }


    inverter = firstinverter;
    printmsg(ECU_DBG_MAIN,"--------------");
    for(i=1; i<=num; i++, inverter++)
        printdecmsg(ECU_DBG_MAIN,inverter->id, inverter->shortaddr);
    printmsg(ECU_DBG_MAIN,"--------------");
    printdecmsg(ECU_DBG_MAIN,"total", num);


    inverter = firstinverter;
    printmsg(ECU_DBG_MAIN,"--------------");

    return num;
}

int save_process_result(int item, char *result)
{
    char dir[50] = "/home/data/proc_res/";
    char file[9];
    int fd;
    char time[20];
    if('\n' == result[strlen(result)-1])
    {
        result[strlen(result)-1] = '\0';
    }

    getcurrenttime(time);
    memcpy(file,&time[0],8);
    file[8] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    print2msg(ECU_DBG_MAIN,"save_process_result DIR",dir);
    fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        write(fd,"\n",1);
        sprintf(result,"%s,%3d,1\n",result,item);
        print2msg(ECU_DBG_MAIN,"result",result);
        write(fd,result,strlen(result));
        close(fd);
    }

    return 0;

}

int save_inverter_parameters_result(inverter_info *inverter, int item, char *inverter_result)
{
    char dir[50] = "/home/data/iprocres/";
    char file[9];
    int fd;
    char time[20];

    getcurrenttime(time);
    memcpy(file,&time[0],8);
    file[8] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    print2msg(ECU_DBG_MAIN,"save_inverter_parameters_result dir",dir);
    fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        write(fd,"\n",1);
        sprintf(inverter_result,"%s,%s,%3d,1\n",inverter_result,inverter->id,item);
        print2msg(ECU_DBG_MAIN,"inverter_result",inverter_result);
        write(fd,inverter_result,strlen(inverter_result));
        close(fd);
    }

    return 0;

}

int save_inverter_parameters_result2(char *id, int item, char *inverter_result)
{
    char dir[50] = "/home/data/iprocres/";
    char file[9];
    int fd;
    char time[20];

    getcurrenttime(time);
    memcpy(file,&time[0],8);
    file[8] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    print2msg(ECU_DBG_MAIN,"save_inverter_parameters_result2 DIR",dir);
    fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        write(fd,"\n",1);
        sprintf(inverter_result,"%s,%s,%3d,1\n",inverter_result,id,item);
        print2msg(ECU_DBG_MAIN,"inverter_result",inverter_result);
        write(fd,inverter_result,strlen(inverter_result));
        close(fd);
    }

    return 0;

}

void save_system_power(int system_power, char *date_time)
{
    char dir[50] = "/home/record/power/";
    char file[9];
    char sendbuff[50] = {'\0'};
    char date_time_tmp[14] = {'\0'};
    rt_err_t result;
    int fd;
    //if(system_power == 0) return;

    memcpy(date_time_tmp,date_time,14);
    memcpy(file,&date_time[0],6);
    file[6] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    date_time_tmp[12] = '\0';
    print2msg(ECU_DBG_MAIN,"save_system_power DIR",dir);
    result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
        if (fd >= 0)
        {
            sprintf(sendbuff,"%s,%d\n",date_time_tmp,system_power);
            //print2msg(ECU_DBG_MAIN,"save_system_power",sendbuff);
            write(fd,sendbuff,strlen(sendbuff));
            close(fd);
        }
    }
    rt_mutex_release(record_data_lock);

}

//����������ǰ���·�
int calculate_earliest_2_month_ago(char *date,int *earliest_data)
{
    char year_s[5] = {'\0'};
    char month_s[3] = {'\0'};
    int year = 0,month = 0;	//yearΪ��� monthΪ�·� dayΪ����  flag Ϊ�����жϱ�־ count��ʾ�ϸ��º���Ҫ�������� number_of_days:�ϸ��µ�������

    memcpy(year_s,date,4);
    year_s[4] = '\0';
    year = atoi(year_s);

    memcpy(month_s,&date[4],2);
    month_s[2] = '\0';
    month = atoi(month_s);

    if(month >= 3)
    {
        month -= 2;
        *earliest_data = (year * 100 + month);
        //printf("calculate_earliest_2_month_ago:%d %d    %d \n",year,month,*earliest_data);
        return 0;
    }else if(month == 2)
    {
        month = 12;
        year = year - 1;
        *earliest_data = (year * 100 + month);
        //printf("calculate_earliest_2_month_ago:%d %d    %d \n",year,month,*earliest_data);
        return 0;
    }else if(month == 1)
    {
        month = 11;
        year = year - 1;
        *earliest_data = (year * 100 + month);
        //printf("calculate_earliest_2_month_ago:%d %d    %d \n",year,month,*earliest_data);
        return 0;
    }

    return -1;
}


//ɾ��������֮ǰ������
void delete_system_power_2_month_ago(char *date_time)
{
    DIR *dirp;
    char dir[30] = "/home/record/power";
    struct dirent *d;
    char path[100];
    int earliest_data,file_data;
    char fileTime[20] = {'\0'};

    /* ��dirĿ¼*/
    rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    dirp = opendir("/home/record/power");
    if(dirp == RT_NULL)
    {
        printmsg(ECU_DBG_CLIENT,"delete_system_power_2_month_ago open directory error");
        mkdir("/home/record/power",0);
    }
    else
    {
        calculate_earliest_2_month_ago(date_time,&earliest_data);
        //printf("calculate_earliest_2_month_ago:::::%d\n",earliest_data);
        /* ��ȡdirĿ¼*/
        while ((d = readdir(dirp)) != RT_NULL)
        {


            memcpy(fileTime,d->d_name,6);
            fileTime[6] = '\0';
            file_data = atoi(fileTime);
            if(file_data <= earliest_data)
            {
                sprintf(path,"%s/%s",dir,d->d_name);
                unlink(path);
            }


        }
        /* �ر�Ŀ¼ */
        closedir(dirp);
    }
    rt_mutex_release(record_data_lock);


}

//��������ǰ��ʱ��
int calculate_earliest_2_day_ago(char *date,int *earliest_data)
{
    char year_s[5] = {'\0'};
    char month_s[3] = {'\0'};
    char day_s[3] = {'\0'};
    int year = 0,month = 0,day = 0;	//yearΪ��� monthΪ�·� dayΪ����  flag Ϊ�����жϱ�־ count��ʾ�ϸ��º���Ҫ�������� number_of_days:�ϸ��µ�������

    memcpy(year_s,date,4);
    year_s[4] = '\0';
    year = atoi(year_s);

    memcpy(month_s,&date[4],2);
    month_s[2] = '\0';
    month = atoi(month_s);

    memcpy(day_s,&date[6],2);
    day_s[2] = '\0';
    day = atoi(day_s);

    if(day >= 3)
    {
        day -= 2;
        *earliest_data = (year * 10000 + month*100+day);

        return 0;
    }else
    {
        month -= 1;
        if(month == 0)
        {
            month =12;
            year = year - 1;
        }
        day=day_tab[ leap(year)][month-1]+day-2;
        *earliest_data = (year * 10000 + month*100+day);

        return 0;
    }
}


void delete_alarm_event_2_day_ago(char *date_time)
{
    DIR *dirp;
    char dir[30] = "/home/record/eventdir";
    struct dirent *d;
    char path[100];
    int earliest_data,file_data;
    char fileTime[20] = {'\0'};

    /* ��dirĿ¼*/
    rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    dirp = opendir("/home/record/eventdir");
    if(dirp == RT_NULL)
    {
        printmsg(ECU_DBG_CLIENT,"delete_system_power_2_month_ago open directory error");
        mkdir("/home/record/eventdir",0);
    }
    else
    {
        calculate_earliest_2_day_ago(date_time,&earliest_data);
        //printf("calculate_earliest_2_month_ago:::::%d\n",earliest_data);
        /* ��ȡdirĿ¼*/
        while ((d = readdir(dirp)) != RT_NULL)
        {


            memcpy(fileTime,d->d_name,8);
            fileTime[8] = '\0';
            file_data = atoi(fileTime);
            if(file_data <= earliest_data)
            {
                sprintf(path,"%s/%s",dir,d->d_name);
                unlink(path);
            }


        }
        /* �ر�Ŀ¼ */
        closedir(dirp);
    }
    rt_mutex_release(record_data_lock);


}


//��ȡĳ�յĹ������߲���   ����   ����
int read_system_power(char *date_time, char *power_buff,int *length)
{
    char dir[30] = "/home/record/power";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    int power = 0;
    FILE *fp;
    //rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);

    memset(path,0,100);
    memset(buff,0,100);
    memcpy(date_tmp,date_time,8);
    date_tmp[6] = '\0';
    sprintf(path,"%s/%s.dat",dir,date_tmp);

    *length = 0;
    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {	//��8���ֽ�Ϊ��  ��ʱ����ͬ
            if((buff[12] == ',') && (!memcmp(buff,date_time,8)))
            {
                power = (int)atoi(&buff[13]);
                power_buff[(*length)++] = (buff[8]-'0') * 16 + (buff[9]-'0');
                power_buff[(*length)++] = (buff[10]-'0') * 16 + (buff[11]-'0');
                power_buff[(*length)++] = power/256;
                power_buff[(*length)++] = power%256;
            }

        }
        fclose(fp);
        //rt_mutex_release(record_data_lock);
        return 0;
    }

    //rt_mutex_release(record_data_lock);

    return -1;


}


int search_daily_energy(char *date,float *daily_energy)	
{
    char dir[30] = "/home/record/energy";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    FILE *fp;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);

    memset(path,0,100);
    memset(buff,0,100);
    memcpy(date_tmp,date,8);
    date_tmp[6] = '\0';
    sprintf(path,"%s/%s.dat",dir,date_tmp);

    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {	//��8���ֽ�Ϊ��  ��ʱ����ͬ
            if((buff[8] == ',') && (!memcmp(buff,date,8)))
            {
                *daily_energy = (float)atof(&buff[9]);
                fclose(fp);
                rt_mutex_release(record_data_lock);
                return 0;
            }

        }

        fclose(fp);
    }

    rt_mutex_release(record_data_lock);

    return -1;
}


void update_daily_energy(float current_energy, char *date_time)
{
    char dir[50] = "/home/record/energy/";
    char file[9];
    char sendbuff[50] = {'\0'};
    char date_time_tmp[14] = {'\0'};
    rt_err_t result;
    float energy_tmp = current_energy;
    int fd;
    //��ǰһ�ַ�����Ϊ0 �����·�����
    //if(current_energy <= EPSILON && current_energy >= -EPSILON) return;

    memcpy(date_time_tmp,date_time,14);
    memcpy(file,&date_time[0],6);
    file[6] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    date_time_tmp[8] = '\0';	//�洢ʱ��Ϊ������ ����:20170718

    //����Ƿ��Ѿ����ڸ�ʱ��������
    if (0 == search_daily_energy(date_time_tmp,&energy_tmp))
    {
        energy_tmp = current_energy + energy_tmp;
        delete_line(dir,"/home/record/energy/1.tmp",date_time,8);
    }

    print2msg(ECU_DBG_MAIN,"update_daily_energy DIR",dir);
    result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
        if (fd >= 0)
        {
            ecu.today_energy = energy_tmp;
            sprintf(sendbuff,"%s,%f\n",date_time_tmp,energy_tmp);
            //print2msg(ECU_DBG_MAIN,"update_daily_energy",sendbuff);
            write(fd,sendbuff,strlen(sendbuff));
            close(fd);
        }
    }
    rt_mutex_release(record_data_lock);

}

int leap(int year) 
{ 
    if(year%4==0 && year%100!=0 || year%400==0)
        return 1;
    else
        return 0;
} 

//����һ��������һ���ʱ��		����ֵ��0��ʾ����һ���ڵ���   1����ʾ����һ�����ϸ���
int calculate_earliest_week(char *date,int *earliest_data)
{
    char year_s[5] = {'\0'};
    char month_s[3] = {'\0'};
    char day_s[3] = {'\0'};
    int year = 0,month = 0,day = 0,flag = 0,count = 0;	//yearΪ��� monthΪ�·� dayΪ����  flag Ϊ�����жϱ�־ count��ʾ�ϸ��º���Ҫ�������� number_of_days:�ϸ��µ�������

    memcpy(year_s,date,4);
    year_s[4] = '\0';
    year = atoi(year_s);

    memcpy(month_s,&date[4],2);
    month_s[2] = '\0';
    month = atoi(month_s);

    memcpy(day_s,&date[6],2);
    day_s[2] = '\0';
    day = atoi(day_s);

    if(day > 7)
    {
        *earliest_data = (year * 10000 + month * 100 + day) - 6;
        return 0;
    }else
    {
        count = 7 - day;
        //�ж��ϸ����Ǽ���
        month = month - 1;
        if(month == 0)
        {
            year = year - 1;
            month = 12;
        }
        //�����Ƿ�������
        flag = leap(year);
        day = day_tab[flag][month-1]+1-count;
        *earliest_data = (year * 10000 + month * 100 + day);

        return 1;
    }

}

//��ȡ���һ�ܵķ�����    ��ÿ�ռ���
int read_weekly_energy(char *date_time, char *power_buff,int *length)
{
    char dir[30] = "/home/record/energy";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    int earliest_date = 0,compare_time = 0,flag = 0;
    char energy_tmp[20] = {'\0'};
    int energy = 0;
    FILE *fp;

    memset(path,0,100);
    memset(buff,0,100);

    *length = 0;
    //����ǰ�����������һ��   ���flagΪ0  ��ʾ����һ���ڵ���  ���Ϊ1��ʾ���ϸ���
    flag = calculate_earliest_week(date_time,&earliest_date);

    if(flag == 1)
    {
        sprintf(date_tmp,"%d",earliest_date);
        date_tmp[6] = '\0';
        //����ļ�Ŀ¼
        sprintf(path,"%s/%s.dat",dir,date_tmp);
        //���ļ�
        print2msg(ECU_DBG_OTHER,"path",path);
        fp = fopen(path, "r");
        if(fp)
        {
            while(NULL != fgets(buff,100,fp))  //��ȡһ������
            {
                //��ʱ��ת��Ϊint��   Ȼ����бȽ�
                memcpy(date_tmp,buff,8);
                date_tmp[8] = '\0';
                compare_time = atoi(date_tmp);
                //printf("compare_time %d     earliest_date %d\n",compare_time,earliest_date);
                if(compare_time >= earliest_date)
                {
                    memcpy(energy_tmp,&buff[9],(strlen(buff)-9));
                    energy = (int)(atof(energy_tmp)*100);
                    //print2msg(ECU_DBG_OTHER,"buff",buff);
                    //printdecmsg(ECU_DBG_OTHER,"energy",energy);
                    power_buff[(*length)++] = (date_tmp[0]-'0')*16 + (date_tmp[1]-'0');
                    power_buff[(*length)++] = (date_tmp[2]-'0')*16 + (date_tmp[3]-'0');
                    power_buff[(*length)++] = (date_tmp[4]-'0')*16 + (date_tmp[5]-'0');
                    power_buff[(*length)++] = (date_tmp[6]-'0')*16 + (date_tmp[7]-'0');
                    power_buff[(*length)++] = energy/256;
                    power_buff[(*length)++] = energy%256;
                }

            }
            fclose(fp);
        }
    }


    memcpy(date_tmp,date_time,8);
    date_tmp[6] = '\0';
    sprintf(path,"%s/%s.dat",dir,date_tmp);

    print2msg(ECU_DBG_OTHER,"path",path);
    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {
            //��ʱ��ת��Ϊint��   Ȼ����бȽ�
            memcpy(date_tmp,buff,8);
            date_tmp[8] = '\0';
            compare_time = atoi(date_tmp);
            //printf("compare_time %d     earliest_date %d\n",compare_time,earliest_date);
            if(compare_time >= earliest_date)
            {
                memcpy(energy_tmp,&buff[9],(strlen(buff)-9));
                energy = (int)(atof(energy_tmp)*100);
                //print2msg(ECU_DBG_OTHER,"buff",buff);
                //printdecmsg(ECU_DBG_OTHER,"energy",energy);
                power_buff[(*length)++] = (date_tmp[0]-'0')*16 + (date_tmp[1]-'0');
                power_buff[(*length)++] = (date_tmp[2]-'0')*16 + (date_tmp[3]-'0');
                power_buff[(*length)++] = (date_tmp[4]-'0')*16 + (date_tmp[5]-'0');
                power_buff[(*length)++] = (date_tmp[6]-'0')*16 + (date_tmp[7]-'0');
                power_buff[(*length)++] = energy/256;
                power_buff[(*length)++] = energy%256;
            }

        }
        fclose(fp);
    }
    return 0;

}

//����һ����������һ���ʱ��
int calculate_earliest_month(char *date,int *earliest_data)
{
    char year_s[5] = {'\0'};
    char month_s[3] = {'\0'};
    char day_s[3] = {'\0'};
    int year = 0,month = 0,day = 0;	//yearΪ��� monthΪ�·� dayΪ����  flag Ϊ�����жϱ�־ count��ʾ�ϸ��º���Ҫ�������� number_of_days:�ϸ��µ�������

    memcpy(year_s,date,4);
    year_s[4] = '\0';
    year = atoi(year_s);

    memcpy(month_s,&date[4],2);
    month_s[2] = '\0';
    month = atoi(month_s);

    memcpy(day_s,&date[6],2);
    day_s[2] = '\0';
    day = atoi(day_s);

    //�ж��Ƿ����28��
    if(day >= 28)
    {
        day = 1;
        *earliest_data = (year * 10000 + month * 100 + day);
        //printf("calculate_earliest_month:%d %d  %d    %d \n",year,month,day,*earliest_data);
        return 0;
    }else
    {
        //���С��28�ţ�ȡ�ϸ��µĸ���ĺ�һ��
        day = day + 1;
        month = month - 1;
        if(month == 0)
        {
            year = year - 1;
            month = 12;
        }
        *earliest_data = (year * 10000 + month * 100 + day);
        //printf("calculate_earliest_month:%d %d  %d    %d \n",year,month,day,*earliest_data);
        return 1;
    }

}

//��ȡ���һ���µķ�����    ��ÿ�ռ���
int read_monthly_energy(char *date_time, char *power_buff,int *length)
{
    char dir[30] = "/home/record/energy";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    int earliest_date = 0,compare_time = 0,flag = 0;
    char energy_tmp[20] = {'\0'};
    int energy = 0;
    FILE *fp;

    memset(path,0,100);
    memset(buff,0,100);

    *length = 0;
    //����ǰ�����������һ��   ���flagΪ0  ��ʾ����һ���ڵ���  ���Ϊ1��ʾ���ϸ���
    flag = calculate_earliest_month(date_time,&earliest_date);

    if(flag == 1)
    {
        sprintf(date_tmp,"%d",earliest_date);
        date_tmp[6] = '\0';
        //����ļ�Ŀ¼
        sprintf(path,"%s/%s.dat",dir,date_tmp);
        //���ļ�
        //printf("path:%s\n",path);
        fp = fopen(path, "r");
        if(fp)
        {
            while(NULL != fgets(buff,100,fp))  //��ȡһ������
            {
                //��ʱ��ת��Ϊint��   Ȼ����бȽ�
                memcpy(date_tmp,buff,8);
                date_tmp[8] = '\0';
                compare_time = atoi(date_tmp);
                //printf("compare_time %d     earliest_date %d\n",compare_time,earliest_date);
                if(compare_time >= earliest_date)
                {
                    memcpy(energy_tmp,&buff[9],(strlen(buff)-9));
                    energy = (int)(atof(energy_tmp)*100);
                    //printf("buff:%s\n energy:%d\n",buff,energy);
                    power_buff[(*length)++] = (date_tmp[0]-'0')*16 + (date_tmp[1]-'0');
                    power_buff[(*length)++] = (date_tmp[2]-'0')*16 + (date_tmp[3]-'0');
                    power_buff[(*length)++] = (date_tmp[4]-'0')*16 + (date_tmp[5]-'0');
                    power_buff[(*length)++] = (date_tmp[6]-'0')*16 + (date_tmp[7]-'0');
                    power_buff[(*length)++] = energy/256;
                    power_buff[(*length)++] = energy%256;
                }

            }
            fclose(fp);
        }
    }


    memcpy(date_tmp,date_time,8);
    date_tmp[6] = '\0';
    sprintf(path,"%s/%s.dat",dir,date_tmp);

    //printf("path:%s\n",path);
    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {
            //��ʱ��ת��Ϊint��   Ȼ����бȽ�
            memcpy(date_tmp,buff,8);
            date_tmp[8] = '\0';
            compare_time = atoi(date_tmp);
            //printf("compare_time %d     earliest_date %d\n",compare_time,earliest_date);
            if(compare_time >= earliest_date)
            {
                memcpy(energy_tmp,&buff[9],(strlen(buff)-9));
                energy = (int)(atof(energy_tmp)*100);
                //printf("buff:%s\n energy:%d\n",buff,energy);
                power_buff[(*length)++] = (date_tmp[0]-'0')*16 + (date_tmp[1]-'0');
                power_buff[(*length)++] = (date_tmp[2]-'0')*16 + (date_tmp[3]-'0');
                power_buff[(*length)++] = (date_tmp[4]-'0')*16 + (date_tmp[5]-'0');
                power_buff[(*length)++] = (date_tmp[6]-'0')*16 + (date_tmp[7]-'0');
                power_buff[(*length)++] = energy/256;
                power_buff[(*length)++] = energy%256;
            }

        }
        fclose(fp);
    }
    return 0;
}

//����һ��������һ���µ�ʱ��
int calculate_earliest_year(char *date,int *earliest_data)
{
    char year_s[5] = {'\0'};
    char month_s[3] = {'\0'};
    int year = 0,month = 0;	//yearΪ��� monthΪ�·� dayΪ����  flag Ϊ�����жϱ�־ count��ʾ�ϸ��º���Ҫ�������� number_of_days:�ϸ��µ�������

    memcpy(year_s,date,4);
    year_s[4] = '\0';
    year = atoi(year_s);

    memcpy(month_s,&date[4],2);
    month_s[2] = '\0';
    month = atoi(month_s);

    if(month == 12)
    {
        month = 1;
        *earliest_data = (year * 100 + month);
        //printf("calculate_earliest_month:%d %d    %d \n",year,month,*earliest_data);
        return 0;
    }else
    {
        month = month + 1;
        year = year - 1;
        *earliest_data = (year * 100 + month);
        //printf("calculate_earliest_month:%d %d    %d \n",year,month,*earliest_data);
        return 1;
    }

}

//��ȡ���һ��ķ�����    ��ÿ�¼���
int read_yearly_energy(char *date_time, char *power_buff,int *length)
{
    char dir[30] = "/home/record/energy";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    int earliest_date = 0,compare_time = 0;
    char energy_tmp[20] = {'\0'};
    int energy = 0;
    FILE *fp;

    memset(path,0,100);
    memset(buff,0,100);

    *length = 0;
    //����ǰ�����������һ��   ���flagΪ0  ��ʾ����һ���ڵ���  ���Ϊ1��ʾ���ϸ���
    calculate_earliest_year(date_time,&earliest_date);

    sprintf(path,"%s/year.dat",dir);

    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {
            //��ʱ��ת��Ϊint��   Ȼ����бȽ�
            memcpy(date_tmp,buff,6);
            date_tmp[6] = '\0';
            compare_time = atoi(date_tmp);
            //printf("compare_time %d     earliest_date %d\n",compare_time,earliest_date);
            if(compare_time >= earliest_date)
            {
                memcpy(energy_tmp,&buff[7],(strlen(buff)-7));
                energy = (int)(atof(energy_tmp)*100);
                //printf("buff:%s\n energy:%d\n",buff,energy);
                power_buff[(*length)++] = (date_tmp[0]-'0')*16 + (date_tmp[1]-'0');
                power_buff[(*length)++] = (date_tmp[2]-'0')*16 + (date_tmp[3]-'0');
                power_buff[(*length)++] = (date_tmp[4]-'0')*16 + (date_tmp[5]-'0');
                power_buff[(*length)++] = 0x01;
                power_buff[(*length)++] = energy/256;
                power_buff[(*length)++] = energy%256;
            }

        }
        fclose(fp);
    }
    return 0;
}


int search_monthly_energy(char *date,float *daily_energy)	
{
    char dir[30] = "/home/record/energy";
    char path[100];
    char buff[100]={'\0'};
    char date_tmp[9] = {'\0'};
    FILE *fp;
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);

    memset(path,0,100);
    memset(buff,0,100);
    memcpy(date_tmp,date,6);
    date_tmp[4] = '\0';
    sprintf(path,"%s/year.dat",dir);

    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(buff,100,fp))  //��ȡһ������
        {	//��8���ֽ�Ϊ��  ��ʱ����ͬ
            if((buff[6] == ',') && (!memcmp(buff,date,6)))
            {
                *daily_energy = (float)atof(&buff[7]);
                fclose(fp);
                rt_mutex_release(record_data_lock);
                return 0;
            }

        }

        fclose(fp);
    }

    rt_mutex_release(record_data_lock);

    return -1;
}

void update_monthly_energy(float current_energy, char *date_time)
{
    char dir[50] = "/home/record/energy/";
    char file[9];
    char sendbuff[50] = {'\0'};
    char date_time_tmp[14] = {'\0'};
    rt_err_t result;
    float energy_tmp = current_energy;
    int fd;
    //��ǰһ�ַ�����Ϊ0 �����·�����
    //if(current_energy <= EPSILON && current_energy >= -EPSILON) return;

    memcpy(date_time_tmp,date_time,14);
    memcpy(file,&date_time[0],4);
    file[4] = '\0';
    sprintf(dir,"%syear.dat",dir);
    date_time_tmp[6] = '\0';	//�洢ʱ��Ϊ������ ����:20170718

    //����Ƿ��Ѿ����ڸ�ʱ��������
    if (0 == search_monthly_energy(date_time_tmp,&energy_tmp))
    {
        energy_tmp = current_energy + energy_tmp;
        delete_line(dir,"/home/record/energy/2.tmp",date_time,6);
    }

    print2msg(ECU_DBG_MAIN,"update_monthly_energy DIR",dir);
    result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
        if (fd >= 0)
        {
            sprintf(sendbuff,"%s,%f\n",date_time_tmp,energy_tmp);
            //print2msg(ECU_DBG_MAIN,"update_daily_energy",sendbuff);
            write(fd,sendbuff,strlen(sendbuff));
            close(fd);
        }
    }
    rt_mutex_release(record_data_lock);

}

void save_record(char sendbuff[], char *date_time)
{
    char dir[50] = "/home/record/data/";
    char file[9];
    rt_err_t result;
    int fd;

    memcpy(file,&date_time[0],8);
    file[8] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    print2msg(ECU_DBG_MAIN,"save_record DIR",dir);
    result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
        if (fd >= 0)
        {
            write(fd,"\n",1);
            sprintf(sendbuff,"%s,%s,1\n",sendbuff,date_time);
            //print2msg(ECU_DBG_MAIN,"save_record",sendbuff);
            write(fd,sendbuff,strlen(sendbuff));
            close(fd);
        }
    }
    rt_mutex_release(record_data_lock);

}

int save_status(char *result, char *date_time)
{
    char dir[50] = "/home/record/inversta/";
    char file[9];
    int fd;

    memcpy(file,&date_time[0],8);
    file[8] = '\0';
    sprintf(dir,"%s%s.dat",dir,file);
    print2msg(ECU_DBG_MAIN,"save_status DIR",dir);
    fd = open(dir, O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        write(fd,"\n",1);
        sprintf(result,"%s,%s,1\n",result,date_time);
        print2msg(ECU_DBG_MAIN,"Status",result);
        write(fd,result,strlen(result));
        close(fd);
    }

    return 0;
}


void echo(const char* filename,const char* string)
{
    int fd;
    int length;
    if((filename == NULL) ||(string == NULL))
    {
        printmsg(ECU_DBG_OTHER,"para error");
        return ;
    }

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0)
    {
        printmsg(ECU_DBG_OTHER,"open file for write failed");
        return;
    }
    length = write(fd, string, strlen(string));
    if (length != strlen(string))
    {
        printmsg(ECU_DBG_OTHER,"check: read file failed");
        close(fd);
        return;
    }
    close(fd);
}

//�������ֽڵ��ַ���ת��Ϊ16������
int strtohex(char str[2])
{
    int ret=0;

    ((str[0]-'A') >= 0)? (ret =ret+(str[0]-'A'+10)*16 ):(ret = ret+(str[0]-'0')*16);
    ((str[1]-'A') >= 0)? (ret =ret+(str[1]-'A'+10) ):(ret = ret+(str[1]-'0'));
    return ret;
}

void get_mac(rt_uint8_t  dev_addr[6])
{
    char InternalMac[18] = {'\0'};

    ReadPage(INTERNAL_FALSH_MAC,InternalMac,17);
    if(!memcmp(InternalMac,"80:97:1B",8))
    {
        dev_addr[0]=strtohex(&InternalMac[0]);
        dev_addr[1]=strtohex(&InternalMac[3]);
        dev_addr[2]=strtohex(&InternalMac[6]);
        dev_addr[3]=strtohex(&InternalMac[9]);
        dev_addr[4]=strtohex(&InternalMac[12]);
        dev_addr[5]=strtohex(&InternalMac[15]);
        return;
    }
}

void addInverter(char *inverter_id)
{
    int fd;
    char buff[50];
    fd = open("/home/data/id", O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {
        sprintf(buff,"%s,,,,,,\n",inverter_id);
        write(fd,buff,strlen(buff));
        close(fd);
    }
    WritePage(INTERNAL_FLASH_LIMITEID,"1",1);
}

//��ʼ����������Ϣ
void InitServerInfo(void)
{
    char *serverinfo = NULL;
    serverinfo = malloc(512);
    memset(serverinfo,0x00,512);
    //100�ֽ�Զ�̿��Ʒ�����     ����:48 IP :16 �˿�1:2  �˿�2:2  ��ʱʱ��:2 ѭ������:2
    memcpy(serverinfo,"ecu.apsema.com",14);
    memcpy(&serverinfo[48],"60.190.131.190",14);
    serverinfo[64] = 8997/256;
    serverinfo[65] = 8997%256;
    serverinfo[66] = 8997/256;
    serverinfo[67] = 8997%256;
    serverinfo[68] = 0;
    serverinfo[69] = 10;
    serverinfo[70] = 0;
    serverinfo[71] = 15;
    //100�ֽ� �����ϴ�������     ����:48 IP :16 �˿�1:2  �˿�2:2
    memcpy(&serverinfo[100],"ecu.apsema.com",14);
    memcpy(&serverinfo[148],"60.190.131.190",14);
    serverinfo[164] = 8995/256;
    serverinfo[165] = 8995%256;
    serverinfo[166] = 8996/256;
    serverinfo[167] = 8996%256;
    //200�ֽ� FTP������       ����:48 IP :16   �˿�1:2  �û�:40    ����:40
    memcpy(&serverinfo[200],"ecu.apsema.com",14);
    memcpy(&serverinfo[248],"60.190.131.190",14);
    serverinfo[264] = 9219/256;
    serverinfo[265] = 9219%256;
    memcpy(&serverinfo[266],"zhyf",4);
    memcpy(&serverinfo[306],"yuneng",6);
    //100 �ֽ�WiFi�ļ�ϵͳ������ IP :16 �˿�1:2  �˿�2:2
    memcpy(&serverinfo[400],"192.168.1.19",14);
    serverinfo[416] = 9220/256;
    serverinfo[417] = 9220%256;
    serverinfo[418] = 9220/256;
    serverinfo[419] = 9220%256;
    //2�ֽ�
    serverinfo[500] = 1;

    WritePage(INTERNAL_FLASH_CONFIG_INFO,serverinfo,512);	
    free(serverinfo);
    serverinfo = NULL;
}

//���·�������Ϣ
void UpdateServerInfo(void)
{
    char *serverinfo = NULL;
    serverinfo = malloc(512);
    memset(serverinfo,0x00,512);
    //100�ֽ�Զ�̿��Ʒ�����     ����:48 IP :16 �˿�1:2  �˿�2:2  ��ʱʱ��:2 ѭ������:2
    memcpy(serverinfo,control_client_arg.domain,32);
    memcpy(&serverinfo[48],control_client_arg.ip,16);
    serverinfo[64] = control_client_arg.port1/256;
    serverinfo[65] = control_client_arg.port1%256;
    serverinfo[66] = control_client_arg.port2/256;
    serverinfo[67] = control_client_arg.port2%256;
    serverinfo[68] = control_client_arg.timeout/256;
    serverinfo[69] = control_client_arg.timeout%256;
    serverinfo[70] = control_client_arg.report_interval/256;
    serverinfo[71] = control_client_arg.report_interval%256;
    //100�ֽ� �����ϴ�������     ����:48 IP :16 �˿�1:2  �˿�2:2
    memcpy(&serverinfo[100],client_arg.domain,32);
    memcpy(&serverinfo[148],client_arg.ip,16);
    serverinfo[164] = client_arg.port1/256;
    serverinfo[165] = client_arg.port1%256;
    serverinfo[166] = client_arg.port2/256;
    serverinfo[167] = client_arg.port2%256;
    //200�ֽ� FTP������       ����:48 IP :16   �˿�1:2  �û�:40    ����:40
    memcpy(&serverinfo[200],ftp_arg.domain,32);
    memcpy(&serverinfo[248],ftp_arg.ip,16);
    serverinfo[264] = ftp_arg.port1/256;
    serverinfo[265] = ftp_arg.port1%256;
    memcpy(&serverinfo[266],ftp_arg.user,4);
    memcpy(&serverinfo[306],ftp_arg.passwd,6);
    //100 �ֽ�WiFi�ļ�ϵͳ������ IP :16 �˿�1:2  �˿�2:2
    memcpy(&serverinfo[400],wifiserver_arg.ip,16);
    serverinfo[416] = wifiserver_arg.port1/256;
    serverinfo[417] = wifiserver_arg.port1%256;
    serverinfo[418] = wifiserver_arg.port2/256;
    serverinfo[419] = wifiserver_arg.port2%256;
    //2�ֽ�
    serverinfo[500] = 1;

    WritePage(INTERNAL_FLASH_CONFIG_INFO,serverinfo,512);	
    free(serverinfo);
    serverinfo = NULL;
}


void key_init(void)
{
    InitServerInfo();
    WritePage(INTERNAL_FLASH_IPCONFIG,"0",1);
    dhcp_reset();
}

void initPath(void)
{
    mkdir("/home",0x777);
    mkdir("/tmp",0x777);
    mkdir("/yuneng",0x777);
    mkdir("/home/data",0x777);
    mkdir("/home/record",0x777);
    mkdir("/home/data/proc_res",0x777);
    mkdir("/home/data/iprocres",0x777);
    mkdir("/home/record/data",0x777);
    mkdir("/home/record/inversta",0x777);
    mkdir("/home/record/power",0x777);
    mkdir("/home/record/eventdir",0x777);
    mkdir("/home/record/energy",0x777);
    mkdir("/ftp",0x777);

    echo("/home/data/ltpower","0.000000");
    
    
}

void initFileSystem(void)
{
    char area[2] = {0x00};
    WritePage(INTERNAL_FLASH_CHANNEL,"0x10",4);
    WritePage(INTERNAL_FLASH_LIMITEID,"1",1);
    InitServerInfo();
    WritePage(INTERNAL_FLASH_A118,"1",1);
    WritePage(INTERNAL_FALSH_AREA,area,2);
    initPath();
}

int getTimeZone()
{
    int timeZone = 8;
    char s[10] = { '\0' };
    FILE *fp;

    fp = fopen("/yuneng/timezone.con", "r");
    if(fp == NULL){
        print2msg(ECU_DBG_NTP,"/yuneng/timezone.con","open error!");
        return timeZone;
    }
    fgets(s, 10, fp);
    //print2msg(ECU_DBG_NTP,"getTimeZone",s);
    //s[strlen(s) - 1] = '\0';
    fclose(fp);

    timeZone = atoi(&s[7]);
    if(timeZone >= 12 || timeZone <= -12)
    {
        return 8;
    }
    return timeZone;

}

//���Ŀ¼��ʱ��������ļ�,���ڷ���1�������ڷ���0
//dirΪĿ¼�����ƣ����������    oldfileΪ�����ļ����ļ���������������
static int checkOldFile(char *dir,char *oldFile)
{
    DIR *dirp;
    struct dirent *d;
    char path[100] , fullpath[100] = {'\0'};
    int fileDate = 0,temp = 0;
    char tempDate[9] = {'\0'};
    rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        /* ��dirĿ¼*/
        dirp = opendir(dir);

        if(dirp == RT_NULL)
        {
            printmsg(ECU_DBG_OTHER,"check Old File open directory error");
            mkdir(dir,0);
        }
        else
        {
            /* ��ȡdirĿ¼*/
            while ((d = readdir(dirp)) != RT_NULL)
            {

                memcpy(tempDate,d->d_name,8);
                tempDate[8] = '\0';
                if(((temp = atoi(tempDate)) < fileDate) || (fileDate == 0))
                {
                    fileDate = temp;
                    memset(path,0,100);
                    strcpy(path,d->d_name);
                }

            }
            if(fileDate != 0)
            {
                sprintf(fullpath,"%s/%s",dir,path);
                strcpy(oldFile,fullpath);
                closedir(dirp);
                rt_mutex_release(record_data_lock);
                return 1;
            }
            /* �ر�Ŀ¼ */
            closedir(dirp);
        }
    }
    rt_mutex_release(record_data_lock);
    return 0;
}


//���������ļ�ϵͳ���ж�ʣ��ռ�洢�������ʣ��ɴ洢�ռ��С���������Ӧ��Ŀ¼����������Ӧ��ɾ������
int optimizeFileSystem(int capsize)
{
    int result;
    long long cap;
    struct statfs buffer;
    char oldFile[100] = {'\0'};

    result = dfs_statfs("/", &buffer);
    if (result != 0)
    {
        printmsg(ECU_DBG_OTHER,"dfs_statfs failed.\n");
        return -1;
    }
    cap = buffer.f_bsize * buffer.f_bfree / 1024;

    printdecmsg(ECU_DBG_MAIN,"disk free size",(unsigned long)cap);
    //��flashоƬ��ʣ�µ�����С��40KB��ʱ�����һЩ��Ҫ���ļ�ɾ��������
    if (cap < capsize)
    {
        rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
        //ɾ����ǰ��һ���ECU������������    �����Ŀ¼�´����ļ��Ļ�
        if(1 == checkOldFile("/home/data/proc_res",oldFile))
        {
            unlink(oldFile);
        }

        //ɾ����ǰ��һ��������������������  �����Ŀ¼�´����ļ��Ļ�
        memset(oldFile,0x00,100);
        if(1 == checkOldFile("/home/data/iprocres",oldFile))
        {
            unlink(oldFile);
        }

        //ɾ����ǰ��һ��������״̬���� �����Ŀ¼�´����ļ��Ļ�
        memset(oldFile,0x00,100);
        if(1 == checkOldFile("/home/record/inversta",oldFile))
        {
            unlink(oldFile);
        }

        //ɾ����ǰ��һ��ķ���������   �����Ŀ¼�´����ļ��Ļ�
        memset(oldFile,0x00,100);
        if(1 == checkOldFile("/home/record/data",oldFile))
        {
            unlink(oldFile);
        }
        rt_mutex_release(record_data_lock);
    }

    return 0;

}


int setECUID(char *ECUID)
{
    int ret = 0;
    char ecuid[13] = {'\0'};
    memcpy(ecuid,ECUID,12);
    ecuid[12] = '\0';

    WritePage(INTERNAL_FALSH_ID,ecuid,12);
    ret = WIFI_Factory(ecuid);
    if(ret == -1)
    {
        return -1;
    }
    printf("set_Passwd(\"88888888\",8);\n");
    set_Passwd("88888888",8);
    return 0;
}

void rm_dir(char* dir)
{
    DIR *dirp;
    struct dirent *d;
    /* ��dirĿ¼*/
    dirp = opendir(dir);

    if(dirp == RT_NULL)
    {
        printmsg(ECU_DBG_OTHER,"open directory error!");
    }
    else
    {
        /* ��ȡĿ¼*/
        while ((d = readdir(dirp)) != RT_NULL)
        {
            char buff[100];
            print2msg(ECU_DBG_OTHER,"delete", d->d_name);
            sprintf(buff,"%s/%s",dir,d->d_name);
            unlink(buff);
        }
        /* �ر�Ŀ¼ */
        closedir(dirp);
    }
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(setECUID, eg:set ECU ID("213000000001"));
FINSH_FUNCTION_EXPORT(echo, eg:echo("/test","test"));
#if 0
void splitSt(char * str)
{
    int i = 0 , num;
    char list[20][32];
    num = splitString(str,list);
    for(i = 0;i<num;i++)
    {
        printmsg(ECU_DBG_OTHER,list[i]);
        if(strlen(list[i]) == 0)
        {
            printmsg(ECU_DBG_OTHER,"NULL");
        }
    }
    printdecmsg(ECU_DBG_OTHER,"num",num);

}
FINSH_FUNCTION_EXPORT(splitSt, eg:splitSt());

void testfile()
{
    get_id_from_file(inverter);
}
FINSH_FUNCTION_EXPORT(testfile, eg:testfile());
#endif


FINSH_FUNCTION_EXPORT(rm_dir, eg:rm_dir("/home/record/data"));


int cal(char * date)
{
    int length = 0;
    char power_buff[1024] = { '\0' };
    //calculate_earliest_week(date,&earliest_data);
    read_yearly_energy(date, power_buff,&length);
    //printf("length:%d \n",length);
    //calculate_earliest_month(date,&earliest_data);
    //calculate_earliest_year(date,&earliest_data);
    return 0;
}
FINSH_FUNCTION_EXPORT(cal, eg:cal("20170721"));

int initsystem(char *mac)
{

    initFileSystem();
    rt_hw_ms_delay(20);

    WritePage(INTERNAL_FALSH_MAC,mac,17);
    return 0;
}
FINSH_FUNCTION_EXPORT(initsystem, eg:initsystem("123456789012","80:97:1B:00:72:1C"));

FINSH_FUNCTION_EXPORT(addInverter, eg:addInverter("201703150001"));

FINSH_FUNCTION_EXPORT(sysDirDetection, eg:sysDirDetection());

#endif
