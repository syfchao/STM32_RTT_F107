#ifndef __FILE_H__
#define __FILE_H__
/*****************************************************************************/
/*  File      : file.h                                                       */
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
#include "variation.h"
#include <rtthread.h>
#include "usart5.h"
/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
extern Socket_Cfg client_arg;
extern Socket_Cfg control_client_arg;
extern Socket_Cfg ftp_arg;
extern Socket_Cfg wifiserver_arg;

//�����Ƿ�������  �Ƿ���1    ���Ƿ���0  
int leap(int year);
int fileopen(const char *file, int flags, int mode);
int fileclose(int fd);
int fileWrite(int fd,char* buf,int len);
int fileRead(int fd,char* buf,int len);
int get_Passwd(char *PassWD);
int set_Passwd(char *PassWD,int length);
void dirDetection(char *path);
void sysDirDetection(void);
void splitSpace(char *data,char *sourcePath,char *destPath);
int setECUID(char *ECUID);
int get_DHCP_Status(void);
void rm_dir(char* dir);
void initPath(void);
int update_tmpdb(inverter_info *firstinverter);
void key_init(void);
//��ȡDRM���غ���   ����ֵΪ1����ʾ���ܴ�   ����ֵΪ-1��ʾ���ܹر�
int DRMFunction(void);
void get_ecuid(char *ecuid);
int get_ecu_type(void);
unsigned short get_panid(void);
char get_channel(void);
int strtohex(char str[2]);
float get_lifetime_power(void);	
void UpdateServerInfo(void);
void update_life_energy(float lifetime_power);
void updateID(void);
int splitString(char *data,char splitdata[][32]);
int get_id_from_file(inverter_info *firstinverter);
int save_process_result(int item, char *result);	//���ñ�������,���ʵ���ɺ�ѽ�������/home/data/proc_resĿ¼�£�control_client�ѽ�����͸�EMA
int save_inverter_parameters_result(inverter_info *inverter, int item, char *inverter_result);
int save_inverter_parameters_result2(char *id, int item, char *inverter_result);
void save_system_power(int system_power, char *date_time);
void delete_system_power_2_month_ago(char *date_time);
void delete_alarm_event_2_day_ago(char *date_time);
int read_system_power(char *date_time, char *power_buff,int *length);
void update_daily_energy(float current_energy, char *date_time);
void update_monthly_energy(float current_energy, char *date_time);
int read_weekly_energy(char *date_time, char *power_buff,int *length);
int read_monthly_energy(char *date_time, char *power_buff,int *length);
int read_yearly_energy(char *date_time, char *power_buff,int *length);
void save_record(char sendbuff[], char *date_time);			//ECU���ͼ�¼��EMA��ͬʱ,����Ҳ����һ��
int save_status(char *result, char *date_time);	//���ñ������������ʵ���ɺ󣬰ѽ�����浽�ļ��У�control_client�ѽ�����͸�EMA
void get_mac(rt_uint8_t  dev_addr[6]);
void echo(const char* filename,const char* string);
void addInverter(char *inverter_id);
void initFileSystem(void);
int getTimeZone(void);
int optimizeFileSystem(int capsize);
#endif /*__FILE_H__*/
