/*****************************************************************************/
/* File      : myfile.c                                                      */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-29 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "myfile.h"
#include "file.h"
#include "datetime.h"
#include <dfs_posix.h> 
#include "debug.h"
#include <stdlib.h>
#include "rthw.h"

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
void delete_newline(char *s)
{
    if(10 == s[strlen(s)-1])
        s[strlen(s)-1] = '\0';
}

/* ��ȡ(�洢����������)�����ļ��е�ֵ */
char *file_get_one(char *s, int count, const char *filename)
{

    FILE *fp;

    fp = fopen(filename, "r");
    if(fp == NULL){
        print2msg(ECU_DBG_CONTROL_CLIENT,(char *)filename,"open error!");
        return NULL;
    }
    fgets(s, count, fp);
    if (10 == s[strlen(s) - 1])
        s[strlen(s) - 1] = '\0';
    fclose(fp);
    return s;
}

/* ����������ֵд�������ļ��� */
int file_set_one(const char *s, const char *filename)
{
    FILE *fp;
    fp = fopen(filename, "w");
    if(fp == NULL){
        printmsg(ECU_DBG_CONTROL_CLIENT,(char *)filename);
        return -1;
    }
    if(EOF == fputs(s, fp)){
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

/* ��ȡ(�洢���������)�����ļ��е�ֵ */
int file_get_array(MyArray *array, int num, const char *filename)
{
    FILE *fp;
    int count = 0;
    char buffer[128] = {'\0'};
    memset(array, 0 ,sizeof(MyArray)*num);
    fp = fopen(filename, "r");
    if(fp == NULL){
        printmsg(ECU_DBG_CONTROL_CLIENT,(char *)filename);
        return -1;
    }
    while(!feof(fp))
    {
        if(count >= num)
        {
            fclose(fp);
            return 0;
        }
        memset(buffer, 0 ,sizeof(buffer));
        fgets(buffer, 128, fp);
        if(!strlen(buffer))continue;

        strncpy(array[count].name, buffer, strcspn(buffer, "="));
        strncpy(array[count].value, &buffer[strlen(array[count].name)+1], 64);
        delete_newline(array[count].value);
        count++;
    }
    fclose(fp);
    return 0;
}

int file_set_array(const MyArray *array, int num, const char *filename)
{
    FILE *fp;
    int i;

    fp = fopen(filename, "w");
    if(fp == NULL){
        printmsg(ECU_DBG_CONTROL_CLIENT,(char *)filename);
        return -1;
    }
    for(i=0; i<num; i++){
        fprintf(fp, "%s=%s\n", array[i].name, array[i].value);
    }
    fclose(fp);
    return 0;
}

//����ļ�
int clear_file(char *filename)
{
    FILE *fp;
    fp = fopen(filename, "w");
    if(fp == NULL){
        printmsg(ECU_DBG_CONTROL_CLIENT,(char *)filename);
        return -1;
    }
    fclose(fp);
    return 0;
}


//ɾ���ļ�����
int delete_line(char* filename,char* temfilename,char* compareData,int len)
{
    FILE *fin,*ftp;
    char data[512] = {'\0'};
    fin=fopen(filename,"r");//����ԭ�ļ�123.txt
    if(fin == NULL)
    {
        print2msg(ECU_DBG_OTHER,"Open the file failure",filename);
        return -1;
    }

    ftp=fopen(temfilename,"w");
    if( ftp==NULL){
        print2msg(ECU_DBG_OTHER,"Open the filefailure",temfilename);
        fclose(fin);
        return -1;
    }
    while(fgets(data,512,fin))//��ԭ�ļ���ȡһ��
    {
        if(memcmp(data,compareData,len))
        {
            //print2msg(ECU_DBG_OTHER,"delete_line",data);
            fputs(data,ftp);//��������һ��д����ʱ�ļ�
        }
    }
    fclose(fin);
    fclose(ftp);
    remove(filename);//ɾ��ԭ�ļ�
    rename(temfilename,filename);//����ʱ�ļ�����Ϊԭ�ļ���
    return 0;

}


int get_num_from_id(char inverter_ids[MAXINVERTERCOUNT][13])
{
    int num=0,i,sameflag;
    FILE *fp;
    char data[200];
    fp = fopen("/home/data/id", "r");
    if(fp)
    {
        while(NULL != fgets(data,200,fp))
        {
            //��ǰ�漸�бȽ� �����ǰ�д������ݣ���ʾ����һ�������
            sameflag = 0;
            for(i = 0;i<num;i++)
            {
                if(!memcmp(data,inverter_ids[i],12))
                    sameflag = 1;
            }
            if(sameflag == 1)
            {
                continue;
            }
            memcpy(inverter_ids[num],data,12);
            inverter_ids[num][12] = '\0';
            num++;
        }
        fclose(fp);
    }


    return num;
}

int insert_line(char * filename,char *str)
{
    int fd;
    fd = open(filename, O_WRONLY | O_APPEND | O_CREAT,0);
    if (fd >= 0)
    {

        write(fd,str,strlen(str));
        close(fd);
    }

    return search_line(filename,str,strlen(str));

}

int search_line(char* filename,char* compareData,int len)
{
    FILE *fin;
    char data[300];
    fin=fopen(filename,"r");
    if(fin == NULL)
    {
        print2msg(ECU_DBG_OTHER,"search_line failure1",filename);
        return -1;
    }

    while(fgets(data,300,fin))//��ԭ�ļ���ȡһ��
    {
        if(!memcmp(data,compareData,len))
        {
            //������ͬ�У��ر��ļ�   Ȼ�󷵻�1  ��ʾ���ڸ���
            fclose(fin);
            return 1;
        }
    }
    fclose(fin);
    return -1;
}


int get_protection_from_file(const char pro_name[][32],float *pro_value,int *pro_flag,int num)
{
    FILE *fp;
    char list[3][32];
    char data[200];
    int j = 0;
    fp = fopen("/home/data/setpropa", "r");
    if(fp)
    {
        while(NULL != fgets(data,200,fp))
        {
            memset(list,0,sizeof(list));
            splitString(data,list);
            for(j=0; j<num; j++){
                if(!memcmp(list[0], pro_name[j],strlen(list[0]))){
                    pro_value[j] = atof(list[1]);
                    pro_flag[j] = 1;
                    break;
                }
            }
        }
        fclose(fp);
    }
    return -1;
}

//����1 ����Ѱ�ҵ���   ����-1��ʾδѰ�ҵ�
int read_line(char* filename,char *linedata,char* compareData,int len)
{
    FILE *fin;
    fin=fopen(filename,"r");
    if(fin == NULL)
    {
        //print2msg(ECU_DBG_OTHER,"read_line failure2",filename);
        return -1;
    }

    while(fgets(linedata,100,fin))//��ԭ�ļ���ȡһ��
    {
        if(!memcmp(linedata,compareData,len))
        {
            //������ͬ�У��ر��ļ�   Ȼ�󷵻�1  ��ʾ���ڸ���
            fclose(fin);
            return 1;
        }
    }
    fclose(fin);
    return -1;
}

int read_line_end(char* filename,char *linedata,char* compareData,int len)
{
    FILE *fin;
    fin=fopen(filename,"r");
    if(fin == NULL)
    {
        print2msg(ECU_DBG_OTHER,"read_line_end failure3",filename);
        return -1;
    }

    while(fgets(linedata,100,fin))//��ԭ�ļ���ȡһ��
    {
        if(!memcmp(&linedata[strlen(linedata)-len-1],compareData,len))
        {
            //������ͬ�У��ر��ļ�   Ȼ�󷵻�1  ��ʾ���ڸ���
            fclose(fin);
            return 1;
        }
    }
    fclose(fin);
    return -1;
}

