#include <stdio.h>
#include <string.h>
#include "remote_control_protocol.h"
#include "debug.h"
#include "myfile.h"
#include "rtthread.h"

/*********************************************************************
restinv表格字段：
id,restore_result,restore_time,restore_flag
**********************************************************************/
extern rt_mutex_t record_data_lock;

/* 设置指定台数逆变器的还原标志 */
int set_restore_num(const char *msg, int num)
{

	int i, err_count = 0;
	char inverter_id[13] = {'\0'};
	char str[100];
	
	for(i=0; i<num; i++)
	{
		//获取一台逆变器的ID号
		strncpy(inverter_id, &msg[i*12], 12);

			
		//如果存在该逆变器数据则删除该记录
		delete_line("/home/data/restinv","/home/data/restinv.t",inverter_id,12);
		sprintf(str,"%s,,,1\n",inverter_id);
		//插入数据
		if(-1 == insert_line("/home/data/restinv",str))
		{
			err_count++;
		}

	}

	return err_count;
}

/* 【A134】EMA设置逆变器还原 */
int set_inverter_restore(const char *recvbuffer, char *sendbuffer)
{
	int ack_flag = SUCCESS;
	int type, num;
	char timestamp[15] = {'\0'};
	rt_err_t result = rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
	
	//获取设置类型标志位: 0还原所有逆变器（注：目前没有还原所有这个功能），1还原指定逆变器
	type = msg_get_int(&recvbuffer[30], 1);
	//获取逆变器数量
	num = msg_get_int(&recvbuffer[31], 4);
	//获取时间戳
	strncpy(timestamp, &recvbuffer[35], 14);

	switch(type)
	{
		case 0:
			//不能还原所有逆变器
			ack_flag = UNSUPPORTED;
			break;
		case 1:
			//检查格式（逆变器数量）
			if(!msg_num_check(&recvbuffer[52], num, 12, 0)){
				ack_flag = FORMAT_ERROR;
			}
			else{
				//设置指定逆变器，存入数据库
				if(set_restore_num(&recvbuffer[52], num) > 0)
					ack_flag = DB_ERROR;
			}
			break;
		default:
			ack_flag = FORMAT_ERROR;
			break;
	}
	//拼接应答消息
	msg_ACK(sendbuffer, "A134", timestamp, ack_flag);
	rt_mutex_release(record_data_lock);
	return 0;
}