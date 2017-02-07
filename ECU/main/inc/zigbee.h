#ifndef __ZIGBEE_H__
#define __ZIGBEE_H__
#include "variation.h"

extern int openzigbee(void);
extern int zb_shortaddr_cmd(int shortaddr, char *buff, int length);		//zigbee 短地址报头
extern int zb_shortaddr_reply(char *data,int shortaddr,char *id);			//读取逆变器的返回帧,短地址模式
extern int zb_get_reply(char *data,inverter_info *inverter);			//读取逆变器的返回帧
extern int zb_get_reply_update_start(char *data,inverter_info *inverter);			//读取逆变器远程更新的Update_start返回帧，ZK，返回响应时间定为10秒
extern int zb_get_reply_restore(char *data,inverter_info *inverter);			//读取逆变器远程更新失败，还原指令后的返回帧，ZK，因为还原时间比较长，所以单独写一个函数
extern int zb_get_reply_from_module(char *data);			//读取zigbee模块的返回帧
extern int zb_get_id(char *data);			//获取逆变器ID
extern int zb_turnoff_limited_rptid(int short_addr,inverter_info *inverter);			//关闭限定单个逆变器上报ID功能
extern int zb_turnoff_rptid(int short_addr);			//关闭单个逆变器上报ID功能
extern int zb_get_inverter_shortaddress_single(inverter_info *inverter);			//获取单台指定逆变器短地址，ZK
extern int zb_turnon_rtpid(inverter_info *firstinverter);			//开启逆变器自动上报ID
extern int zb_change_inverter_panid_broadcast(void);	//广播改变逆变器的PANID，ZK
extern int zb_change_inverter_panid_single(inverter_info *inverter);	//单点改变逆变器的PANID和信道，ZK
extern int zb_restore_inverter_panid_channel_single_0x8888_0x10(inverter_info *inverter);	//单点还原逆变器的PANID到0X8888和信道0X10，ZK
extern int zb_change_ecu_panid(void);		//设置ECU的PANID和信道
extern int zb_restore_ecu_panid_0x8888(void);			//恢复ECU的PANID为0x8888,ZK
extern int zb_restore_ecu_panid_0xffff(int channel); 		//设置ECU的PANID为0xFFFF,信道为指定信道(注:向逆变器发送设置命令时,需将ECU的PANID设为0xFFFF)
extern int zb_send_cmd(inverter_info *inverter, char *buff, int length);		//zigbee包头
extern int zb_broadcast_cmd(char *buff, int length);		//zigbee广播包头
extern int zb_query_inverter_info(inverter_info *inverter);		//请求逆变器的机型码
extern int zb_query_data(inverter_info *inverter);		//请求逆变器实时数据
extern int zb_test_communication(void);		//zigbee测试通信有没有断开
extern int zb_set_protect_parameter(inverter_info *inverter, char *protect_parameter);		//参数修改CC指令
extern int zb_query_protect_parameter(inverter_info *inverter, char *protect_data_DA_reply);		//存储参数查询DD指令
extern int zb_afd_broadcast(void);		//AFD广播指令
extern int zb_turnon_inverter_broadcast(void);		//开机指令广播,OK
extern int zb_boot_single(inverter_info *inverter);		//开机指令单播,OK
extern int zb_shutdown_broadcast(void);		//关机指令广播,OK
extern int zb_shutdown_single(inverter_info *inverter);		//关机指令单播,OK
extern int zb_boot_waitingtime_single(inverter_info *inverter);		//开机等待时间启动控制单播,OK
extern int zb_clear_gfdi_broadcast(void);		//清除GFDI广播,OK
extern int zb_clear_gfdi(inverter_info *inverter);		//清除GFDI,OK
extern int zb_ipp_broadcast(void);		//IPP广播
extern int zb_ipp_single(inverter_info *inverter);		//IPP单播,暂时不用,ZK
extern int zb_frequency_protectime_broadcast(void);		//欠频保护时间广播
extern int zb_frequency_protectime_single(inverter_info *inverter);		//欠频保护时间单播
extern int zb_voltage_protectime_broadcast(void);		//欠压保护时间广播
extern int zb_voltage_protectime_single(inverter_info *inverter);		//欠压保护时间单播

		
#endif /*__ZIGBEE_H__*/
