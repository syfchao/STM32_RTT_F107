/*****************************************************************************/
/*  File      : zigbee.c                                                     */
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
#ifdef RT_USING_SERIAL
#include "serial.h"
#endif /* RT_USING_SERIAL */
#include <rtdef.h>
#include <rtthread.h>
#include "debug.h"
#include "zigbee.h"
#include <rthw.h>
#include <stm32f10x.h>
#include "resolve.h"
#include <dfs_posix.h> 
#include <stdio.h>
#include "ema_control.h"
#include "file.h"
#include "rthw.h"

#include "setpower.h"
#include "set_ird.h"
#include "turn_on_off.h"
#include "clear_gfdi.h"
#include "set_protection_parameters.h"
#include "set_protection_parameters_inverter.h"
#include "ZigBeeTransmission.h"

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern struct rt_device serial4;		//����4ΪZigbee�շ�����
extern ecu_info ecu;
extern int zigbeeReadFlag;
static int zigbeereadtimeoutflag = 0;
extern unsigned char processAllFlag;

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/

#define ZIGBEE_SERIAL (serial4)

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
//��ʱ����ʱ����
static void readtimeout_Zigbee(void* parameter)
{
    zigbeereadtimeoutflag = 1;
}

int selectZigbee(int timeout)			//10ms zigbee�������ݼ�� ����0 ��ʾ����û������  ����1��ʾ����������
{

    rt_timer_t readtimer;
    readtimer = rt_timer_create("read", /* ��ʱ������Ϊ read */
                                readtimeout_Zigbee, /* ��ʱʱ�ص��Ĵ����� */
                                RT_NULL, /* ��ʱ��������ڲ��� */
                                timeout*RT_TICK_PER_SECOND/100, /* ��ʱʱ�䳤��,��OS TickΪ��λ*/
                                RT_TIMER_FLAG_ONE_SHOT); /* �����ڶ�ʱ�� */
    if (readtimer != RT_NULL) rt_timer_start(readtimer);
    zigbeereadtimeoutflag = 0;

    while(1)
    {
        if(zigbeereadtimeoutflag)
        {
            rt_timer_delete(readtimer);
            return 0;
        }else
        {
            rt_thread_delay(1);
            if(zigbeeReadFlag == 1)	//�������ݼ��,����������򷵻�1
            {
                rt_timer_delete(readtimer);
                rt_thread_delay(20);
                return 1;
            }
        }
    }
}

void clear_zbmodem(void)		//��մ��ڻ�����������
{
    char data[256];
    //��ջ���������	ͨ�������ջ��������������ݶ���ȡ�������Ӷ��������
    ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data, 255);
    rt_hw_ms_delay(20);
    ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data, 255);
    rt_hw_ms_delay(20);
    ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data, 255);
    rt_hw_ms_delay(20);
}

int openzigbee(void)
{
    int result = 0;
    GPIO_InitTypeDef GPIO_InitStructure;
    rt_device_t new;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, GPIO_Pin_7);		//��������Ϊ�ߵ�ƽ�����ʹ��Zigbbeģ��

    new = rt_device_find("uart4");		//Ѱ��zigbee���ڲ�����ģʽ
    if (new != RT_NULL)
    {
        result = rt_device_open(new, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        if(result)
        {
            printdecmsg(ECU_DBG_MAIN,"open Zigbee failed ",result);
        }else
        {
            printmsg(ECU_DBG_MAIN,"open Zigbee success");
        }
    }

    return result;
}

//��λzigbeeģ��  ͨ��PC7�ĵ�ƽ�ø��õ�Ȼ��ﵽ��λ��Ч��
void zigbee_reset(void)
{
    GPIO_ResetBits(GPIOC, GPIO_Pin_7);		//��������Ϊ�͵�ƽ���
    rt_thread_delay(100);
    GPIO_SetBits(GPIOC, GPIO_Pin_7);		//��������Ϊ�ߵ�ƽ���
    rt_thread_delay(1000);
    printmsg(ECU_DBG_MAIN,"zigbee reset successful");
}

int zb_transmission_reply(char *buff)
{
	int temp_size = 0;
	if(selectZigbee(400) <= 0)
	{
		printmsg(ECU_DBG_MAIN,"Get reply time out");
		return -1;
	}
	else
	{
		temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, buff, 255);
		printhexmsg(ECU_DBG_MAIN,"Reply", buff, temp_size);
		return temp_size;

	}
}

int zb_transmission( char *buff, int length)
{
	
	ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, buff, length);
	printhexmsg(ECU_DBG_MAIN,"Send", (char *)buff, length);
	return 0;
}

int zb_shortaddr_cmd(int shortaddr, char *buff, int length)		//zigbee �̵�ַ��ͷ
{
    unsigned char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x55;
    sendbuff[5]  = shortaddr>>8;
    sendbuff[6]  = shortaddr;
    sendbuff[7]  = 0x00;
    sendbuff[8]  = 0x00;
    sendbuff[9]  = 0x00;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = length;

    for(i=0; i<length; i++)
    {
        sendbuff[15+i] = buff[i];
    }

    if(0!=shortaddr)
    {
        ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0,sendbuff, (length+15));
        printhexmsg(ECU_DBG_MAIN,"zb_shortaddr_cmd", (char *)sendbuff, 15);
        return 1;
    }
    else
        return -1;

}

int zb_shortaddr_reply(char *data,int shortaddr,char *id)			//��ȡ������ķ���֡,�̵�ַģʽ
{
    int i;
    char data_all[256];
    char inverterid[13] = {'\0'};
    int temp_size,size;

    if(selectZigbee(200) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");

        return -1;
    }
    else
    {
        temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data_all, 255);

        size = temp_size -12;

        for(i=0;i<size;i++)
        {
            data[i]=data_all[i+12];
        }
        printhexmsg(ECU_DBG_MAIN,"Reply", data_all, temp_size);
        rt_sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data_all[6],data_all[7],data_all[8],data_all[9],data_all[10],data_all[11]);
        if((size>0)&&(0xFC==data_all[0])&&(0xFC==data_all[1])&&(data_all[2]==shortaddr/256)&&(data_all[3]==shortaddr%256)&&(data_all[5]==0xA5)&&(0==rt_strcmp(id,inverterid)))
        {
            return size;
        }
        else
        {
            return -1;
        }
    }
}


int zb_get_reply(char *data,inverter_info *inverter)			//��ȡ������ķ���֡
{
    int i;
    char data_all[256];
    char inverterid[13] = {'\0'};
    int temp_size,size;

    if(selectZigbee(200) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        inverter->signalstrength=0;
        return -1;
    }
    else
    {
        temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data_all, 255);
        size = temp_size -12;

        for(i=0;i<size;i++)
        {
            data[i]=data_all[i+12];
        }
        printhexmsg(ECU_DBG_MAIN,"Reply", data_all, temp_size);
        rt_sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data_all[6],data_all[7],data_all[8],data_all[9],data_all[10],data_all[11]);
        if((size>0)&&(0xFC==data_all[0])&&(0xFC==data_all[1])&&(data_all[2]==inverter->shortaddr/256)&&(data_all[3]==inverter->shortaddr%256)&&(0==rt_strcmp(inverter->id,inverterid)))
        {
            inverter->raduis=data_all[5];
            inverter->signalstrength=data_all[4];
            return size;
        }
        else
        {
            inverter->signalstrength=0;
            return -1;
        }
    }

}

int zb_get_reply_new(char *data,inverter_info *inverter,int second)			//��ȡ������ķ���֡
{
    int i;
    char data_all[256];
    char inverterid[13] = {'\0'};
    int temp_size,size;


    if(selectZigbee(second*100) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        inverter->signalstrength=0;
        return -1;
    }
    else
    {
        temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data_all, 255);
        size = temp_size -12;

        for(i=0;i<size;i++)
        {
            data[i]=data_all[i+12];
        }
        printf("temp_size:%d \n",temp_size);
        printhexmsg(ECU_DBG_MAIN,"Reply", data_all, temp_size);
        sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data_all[6],data_all[7],data_all[8],data_all[9],data_all[10],data_all[11]);
        if((size>0)&&(0xFC==data_all[0])&&(0xFC==data_all[1])&&(data_all[2]==inverter->shortaddr/256)&&(data_all[3]==inverter->shortaddr%256)&&(0==strcmp(inverter->id,inverterid)))
        {
            inverter->raduis=data_all[5];
            inverter->signalstrength=data_all[4];
            return size;
        }
        else
        {
            inverter->signalstrength=0;
            return -1;
        }
    }
}

/*------------------------------------------------------------------*/
/*-------------------Զ��������ز���-------------------------*/
/*------------------------------------------------------------------*/
int zb_get_reply_update_start(char *data,inverter_info *inverter)			//��ȡ�����Զ�̸��µ�Update_start����֡��ZK��������Ӧʱ�䶨Ϊ10��
{
    int i;
    char data_all[256];
    char inverterid[13] = {'\0'};
    int temp_size,size;

    if(selectZigbee(1000) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        return -1;
    }
    else
    {
        temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data_all, 255);
        size = temp_size -12;

        for(i=0;i<size;i++)
        {
            data[i]=data_all[i+12];
        }
        printhexmsg(ECU_DBG_MAIN,"Reply", data_all, temp_size);
        rt_sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data_all[6],data_all[7],data_all[8],data_all[9],data_all[10],data_all[11]);
        if((size>0)&&(0xFC==data_all[0])&&(0xFC==data_all[1])&&(data_all[2]==inverter->shortaddr/256)&&(data_all[3]==inverter->shortaddr%256)&&(data_all[5]==0xA5)&&(0==rt_strcmp(inverter->id,inverterid)))
        {
            return size;
        }
        else
        {
            return -1;
        }
    }

}

int zb_get_reply_restore(char *data,inverter_info *inverter)			//��ȡ�����Զ�̸���ʧ�ܣ���ԭָ���ķ���֡��ZK����Ϊ��ԭʱ��Ƚϳ������Ե���дһ������
{
    int i;
    char data_all[256];
    char inverterid[13] = {'\0'};
    int temp_size,size;

    if(selectZigbee(20000) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        return -1;
    }
    else
    {
        temp_size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data_all, 255);
        size = temp_size -12;

        for(i=0;i<size;i++)
        {
            data[i]=data_all[i+12];
        }
        printhexmsg(ECU_DBG_MAIN,"Reply", data_all, temp_size);
        rt_sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data_all[6],data_all[7],data_all[8],data_all[9],data_all[10],data_all[11]);

        if((size>0)&&(0xFC==data_all[0])&&(0xFC==data_all[1])&&(data_all[2]==inverter->shortaddr/256)&&(data_all[3]==inverter->shortaddr%256)&&(data_all[5]==0xA5)&&(0==rt_strcmp(inverter->id,inverterid)))
        {
            return size;
        }
        else
        {
            return -1;
        }
    }
}

int zb_get_reply_from_module(char *data)			//��ȡzigbeeģ��ķ���֡
{
    int size = 0;

    if(selectZigbee(100) <= 0)
    {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        return -1;
    }else
    {
        size = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data, 255);
        if(size > 0)
        {
            printhexmsg(ECU_DBG_MAIN,"Reply", data, size);
            return size;
        }else
        {
            return -1;
        }
    }
}

int zb_turnoff_limited_rptid(int short_addr,inverter_info *inverter)			//�ر��޶�����������ϱ�ID����
{
    unsigned char sendbuff[256] = {'\0'};
    int i=0, ret;
    char data[256];
    int check=0;

    clear_zbmodem();			//����ָ��ǰ������ջ�����
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x08;
    sendbuff[5]  = short_addr>>8;
    sendbuff[6]  = short_addr;
    sendbuff[7]  = 0x08;//panid
    sendbuff[8]  = 0x88;
    sendbuff[9]  = 0x19;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0xA0;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;

    if(0!=inverter->shortaddr)
    {
        printmsg(ECU_DBG_MAIN,"Turn off limited report id");
        ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0,sendbuff, 15);
        printhexmsg(ECU_DBG_MAIN,"sendbuff",(char *)sendbuff,15);
        ret = zb_get_reply_from_module(data);
        if((11 == ret)&&(0xA5 == data[2])&&(0xA5 == data[3]))
        {
            if(inverter->zigbee_version!=data[9])
            {
                inverter->zigbee_version = data[9];
                //				update_zigbee_version(inverter);
            }
            return 1;
        }
        else
            return -1;
    }
    else
        return -1;

}

int zb_get_inverter_shortaddress_single(inverter_info *inverter)			//��ȡ��ָ̨��������̵�ַ��ZK
{
    unsigned char sendbuff[256] = {'\0'};
    int i=0, ret;
    char data[256];
    char inverterid[13] = {'\0'};
    int check=0;
    printmsg(ECU_DBG_MAIN,"Get inverter shortaddresssingle");

    clear_zbmodem();			//����ָ��ǰ����ջ�����
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x0E;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0x00;//panid
    sendbuff[8]  = 0x00;
    sendbuff[9]  = 0x00;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x06;

    sendbuff[15]=((inverter->id[0]-0x30)*16+(inverter->id[1]-0x30));
    sendbuff[16]=((inverter->id[2]-0x30)*16+(inverter->id[3]-0x30));
    sendbuff[17]=((inverter->id[4]-0x30)*16+(inverter->id[5]-0x30));
    sendbuff[18]=((inverter->id[6]-0x30)*16+(inverter->id[7]-0x30));
    sendbuff[19]=((inverter->id[8]-0x30)*16+(inverter->id[9]-0x30));
    sendbuff[20]=((inverter->id[10]-0x30)*16+(inverter->id[11]-0x30));

    //	strcpy(&sendbuff[15],inverter->inverterid);


    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 21);
    printhexmsg(ECU_DBG_MAIN,"sendbuff",(char *)sendbuff,21);
    ret = zb_get_reply_from_module(data);

    rt_sprintf(inverterid,"%02x%02x%02x%02x%02x%02x",data[4],data[5],data[6],data[7],data[8],data[9]);

    if((11 == ret)&&(0xFF == data[2])&&(0==rt_strcmp(inverter->id,inverterid)))
    {
        inverter->shortaddr = data[0]*256 + data[1];
        updateID();
        return 1;
    }
    else
        return -1;

}

int zb_change_inverter_panid_broadcast(void)	//�㲥�ı��������PANID��ZK
{
    char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    clear_zbmodem();
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x03;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = ecu.panid>>8;
    sendbuff[8]  = ecu.panid;
    sendbuff[9]  = ecu.channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 15);
    printhexmsg(ECU_DBG_MAIN,"sendbuff",sendbuff,15);

    return 1;
}

int zb_change_inverter_panid_single(inverter_info *inverter)	//����ı��������PANID���ŵ���ZK
{
    char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x0F;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = ecu.panid>>8;
    sendbuff[8]  = ecu.panid;
    sendbuff[9]  = ecu.channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0xA0;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x06;
    sendbuff[15]=((inverter->id[0]-0x30)*16+(inverter->id[1]-0x30));
    sendbuff[16]=((inverter->id[2]-0x30)*16+(inverter->id[3]-0x30));
    sendbuff[17]=((inverter->id[4]-0x30)*16+(inverter->id[5]-0x30));
    sendbuff[18]=((inverter->id[6]-0x30)*16+(inverter->id[7]-0x30));
    sendbuff[19]=((inverter->id[8]-0x30)*16+(inverter->id[9]-0x30));
    sendbuff[20]=((inverter->id[10]-0x30)*16+(inverter->id[11]-0x30));

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 21);
    printhexmsg(ECU_DBG_MAIN,"sendbuff",sendbuff,21);

    rt_thread_delay(100);
    return 1;

}

int zb_restore_inverter_panid_channel_single_0x8888_0x10(inverter_info *inverter)	//���㻹ԭ�������PANID��0X8888���ŵ�0X10��ZK
{
    char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x0F;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0x88;
    sendbuff[8]  = 0x88;
    sendbuff[9]  = 0x10;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0xA0;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x06;
    sendbuff[15]=((inverter->id[0]-0x30)*16+(inverter->id[1]-0x30));
    sendbuff[16]=((inverter->id[2]-0x30)*16+(inverter->id[3]-0x30));
    sendbuff[17]=((inverter->id[4]-0x30)*16+(inverter->id[5]-0x30));
    sendbuff[18]=((inverter->id[6]-0x30)*16+(inverter->id[7]-0x30));
    sendbuff[19]=((inverter->id[8]-0x30)*16+(inverter->id[9]-0x30));
    sendbuff[20]=((inverter->id[10]-0x30)*16+(inverter->id[11]-0x30));

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 21);
    printhexmsg(ECU_DBG_MAIN,"sendbuff",sendbuff,21);

    rt_thread_delay(100);
    return 1;

}

//����ECU��PANID���ŵ�
int zb_change_ecu_panid(void)
{
    unsigned char sendbuff[16] = {'\0'};
    char recvbuff[256] = {'\0'};
    int i;
    int check=0;
    clear_zbmodem();
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x05;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = ecu.panid>>8;
    sendbuff[8]  = ecu.panid;
    sendbuff[9]  = ecu.channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 15);
    printhexmsg(ECU_DBG_MAIN,"Set ECU PANID and Channel", (char *)sendbuff, 15);

    if ((3 == zb_get_reply_from_module(recvbuff))
            && (0xAB == recvbuff[0])
            && (0xCD == recvbuff[1])
            && (0xEF == recvbuff[2])) {
        rt_thread_delay(200); //��ʱ2S����Ϊ������ECU�ŵ���PANID��ᷢ6��FF
        return 1;
    }

    return -1;
}

int zb_restore_ecu_panid_0x8888(void)			//�ָ�ECU��PANIDΪ0x8888,ZK
{
    unsigned char sendbuff[256] = {'\0'};
    int i=0, ret;
    char data[256];
    int check=0;
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x05;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0x88;
    sendbuff[8]  = 0x88;
    sendbuff[9]  = ecu.channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0, sendbuff, 15);
    printhexmsg(ECU_DBG_MAIN,"sendbuff",(char *)sendbuff,15);
    ret = zb_get_reply_from_module(data);
    if((3 == ret)&&(0xAB == data[0])&&(0xCD == data[1])&&(0xEF == data[2]))
        return 1;
    else
        return -1;
}

//����ECU��PANIDΪ0xFFFF,�ŵ�Ϊָ���ŵ�(ע:�������������������ʱ,�轫ECU��PANID��Ϊ0xFFFF)
int zb_restore_ecu_panid_0xffff(int channel)
{
    unsigned char sendbuff[15] = {'\0'};
    char recvbuff[256];
    int i;
    int check=0;
    //��ECU��������
    clear_zbmodem();
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x05;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0xFF;
    sendbuff[8]  = 0xFF;
    sendbuff[9]  = channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;
    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, 15);
    printhexmsg(ECU_DBG_MAIN,"Change ECU channel (PANID:0xFFFF)", (char *)sendbuff, 15);

    //���շ���
    if ((3 == zb_get_reply_from_module(recvbuff))
            && (0xAB == recvbuff[0])
            && (0xCD == recvbuff[1])
            && (0xEF == recvbuff[2])) {
        return 1;
    }

    return -1;
}

int zb_send_cmd(inverter_info *inverter, char *buff, int length)		//zigbee��ͷ
{
    unsigned char sendbuff[256] = {'\0'};
    int i;
    int check=0;

    clear_zbmodem();			//��������ǰ,��ջ�����
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x55;
    sendbuff[5]  = inverter->shortaddr>>8;
    sendbuff[6]  = inverter->shortaddr;
    sendbuff[7]  = 0x00;
    sendbuff[8]  = 0x00;
    sendbuff[9]  = 0x00;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = length;

    //printdecmsg(ECU_DBG_MAIN,"shortaddr",inverter->shortaddr);
    for(i=0; i<length; i++)
    {
        sendbuff[15+i] = buff[i];
    }
    rt_thread_delay(1);
    if(0!=inverter->shortaddr)
    {
        ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, length+15);
        //printhexmsg("Send", (char *)sendbuff, length+15);
        return 1;
    }
    else
        return -1;
}

int zb_broadcast_cmd(char *buff, int length)		//zigbee�㲥��ͷ
{
    unsigned char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x55;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0x00;
    sendbuff[8]  = 0x00;
    sendbuff[9]  = 0x00;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = length;

    for(i=0; i<length; i++)
    {
        sendbuff[15+i] = buff[i];
    }

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, length+15);
    printhexmsg(ECU_DBG_MAIN,"Send", (char *)sendbuff, length+15);
    return 1;
}

int zb_query_inverter_info(inverter_info *inverter)		//����������Ļ�����
{
    int i = 0;
    int old_version = inverter->version;
    int old_model = inverter->model;
    char sendbuff[256];
    char recvbuff[256];

    clear_zbmodem();
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xDC;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xE2;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;
    zb_send_cmd(inverter, sendbuff, i);
    print2msg(ECU_DBG_MAIN,"Query Inverter's Model and Version", inverter->id);

    if ((16 == zb_get_reply(recvbuff, inverter))
            && (0xFB == recvbuff[0])
            && (0xFB == recvbuff[1])
            && (0x09 == recvbuff[2])
            && (0xDC == recvbuff[3])
            //&& (0xDC == recvbuff[5])
            && (0xFE == recvbuff[14])
            && (0xFE == recvbuff[15])) {
        inverter->model = recvbuff[4];
        inverter->version = (recvbuff[5]*256 + recvbuff[6])*1000+(recvbuff[8]*256+recvbuff[9]);
        if((old_version != inverter->version) ||(old_model != inverter->model))
        {
            ecu.idUpdateFlag = 1;
        }
        /*
        if(turn_on_flag==1)
        {
            if(recvbuff[7]==1)
                inverter->inverterstatus.fill_up_data_flag=1;
            else if(recvbuff[7]==0)
                inverter->inverterstatus.fill_up_data_flag=2;
            else
                inverter->inverterstatus.fill_up_data_flag=3;
            save_inverter_replacement_model(inverter->id,(inverter->inverterstatus.fill_up_data_flag%3));
        }
        */
        return 1;
    }

    return -1;
}

int zb_query_data(inverter_info *inverter)		//���������ʵʱ����
{
    int i=0, ret;
    char sendbuff[256];
    char data[256];
    int check=0;

    print2msg(ECU_DBG_MAIN,"Query inverter data",inverter->id);
    clear_zbmodem();			//����ָ��ǰ,����ջ�����
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xBB;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xC1;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_send_cmd(inverter, sendbuff, i);
    ret = zb_get_reply(data,inverter);

    if((0 != ret)&&(ret%88 == 0)&&(0xFB == data[0])&&(0xFB == data[1])&&(0xFE == data[86])&&(0xFE == data[87]))
    {
        for(i=2;i<84;i++)
            check=check+data[i];

        if(((data[84]==0x00)&&(data[85]==0x00))||((check/256 == data[84])&&(check%256 == data[85])))
        {
            inverter->no_getdata_num = 0;	//һ�����յ����ݾ���0,ZK
            inverter->inverterstatus.dataflag = 1;	//���յ�������Ϊ1
            if(0x17==inverter->model)
            {
                resolvedata_1200(&data[4], inverter);
            }
            else if((7==inverter->model))
            {
                if(0xBB == data[3])
                {
                    resolvedata_600(&data[4], inverter);
                    inverter->inverterstatus.deputy_model = 1;
                }
                else if(0xB1 == data[3])
                {
                    resolvedata_600_new(&data[4], inverter);
                    inverter->inverterstatus.deputy_model = 2;
                }
                else ;
            }else if(8==inverter->model)
            {
                resolvedata_600_60(&data[4], inverter);
            }

            else if(5==inverter->model)
                resolvedata_1000(&data[4], inverter);
            else if(6==inverter->model)
                resolvedata_1000(&data[4], inverter);
            else
            {;}

            return 1;
        }else
        {
            inverter->inverterstatus.dataflag = 0;
            return -1;
        }
    }
    else
    {
        inverter->inverterstatus.dataflag = 0;
        return -1;
    }

}
int zb_test_communication(void)		//zigbee����ͨ����û�жϿ�
{
    unsigned char sendbuff[256] = {'\0'};
    int i=0, ret = 0;
    char data[256] =  {'\0'};
    int check=0;

    printmsg(ECU_DBG_MAIN,"test zigbee communication");
    clear_zbmodem();			//����ָ��ǰ,����ջ�����
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x0D;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = 0x00;
    sendbuff[8]  = 0x00;
    sendbuff[9]  = 0x00;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0x00;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x00;

    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, 15);
    ret = zb_get_reply_from_module(data);
    if((3 == ret)&&(0xAB == data[0])&&(0xCD == data[1])&&(0xEF == data[2]))
        return 1;
    else
        return -1;

}

int zb_query_protect_parameter(inverter_info *inverter, char *protect_data_DD_reply)		//�洢������ѯDDָ��
{
    int i=0,ret;
    char sendbuff[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xDD;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xE3;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    print2msg(ECU_DBG_MAIN,inverter->id, "zb_query_protect_parameter");
    zb_send_cmd(inverter, sendbuff, i);
    ret = zb_get_reply(protect_data_DD_reply,inverter);
    if((33 == ret) && (0xDD == protect_data_DD_reply[3]) && (0xFB == protect_data_DD_reply[0]) && (0xFB == protect_data_DD_reply[1]) && (0xFE == protect_data_DD_reply[31]) && (0xFE == protect_data_DD_reply[32]))
        return 1;
    if((58 == ret) &&
            (0xFB == protect_data_DD_reply[0]) &&
            (0xFB == protect_data_DD_reply[1]) &&
            (0xDA == protect_data_DD_reply[3]) &&
            (0xFE == protect_data_DD_reply[56]) &&
            (0xFE == protect_data_DD_reply[57]))
        return 1;
    else
        return -1;
}


int zb_turnon_inverter_broadcast(void)		//����ָ��㲥,OK
{
    int i=0;
    char sendbuff[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xA1;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xA7;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_broadcast_cmd(sendbuff, i);
    return 1;
}

int zb_boot_single(inverter_info *inverter)		//����ָ���,OK
{
    int i=0, ret;
    char sendbuff[256];
    char data[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xC1;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xC7;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;
    printmsg(ECU_DBG_MAIN,"zb_boot_single");
    zb_send_cmd(inverter, sendbuff, i);
    ret = zb_get_reply(data,inverter);
    if((13 == ret) && (0xDE == data[3]) && (0xFB == data[0]) && (0xFB == data[1]) && (0xFE == data[11]) && (0xFE == data[12]))
        return 1;
    else
        return -1;
}

int zb_shutdown_broadcast(void)		//�ػ�ָ��㲥,OK
{
    int i=0;
    char sendbuff[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xA2;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xA8;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_broadcast_cmd(sendbuff, i);
    return 1;
}

int zb_shutdown_single(inverter_info *inverter)		//�ػ�ָ���,OK
{
    int i=0, ret;
    char sendbuff[256];
    char data[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xC2;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xC8;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;
    printmsg(ECU_DBG_MAIN,"zb_shutdown_single");
    zb_send_cmd(inverter, sendbuff, i);
    ret = zb_get_reply(data,inverter);
    if((13 == ret) && (0xDE == data[3]) && (0xFB == data[0]) && (0xFB == data[1]) && (0xFE == data[11]) && (0xFE == data[12]))
        return 1;
    else
        return -1;
}

int zb_boot_waitingtime_single(inverter_info *inverter)		//�����ȴ�ʱ���������Ƶ���,OK
{
    int i=0;
    char sendbuff[256];
    //	char data[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xCD;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xD3;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_send_cmd(inverter, sendbuff, i);
    return 1;
}

int zb_clear_gfdi(inverter_info *inverter)		//���GFDI,OK
{
    int i=0, ret;
    char sendbuff[256];
    char data[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xAF;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xB5;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_send_cmd(inverter, sendbuff, i);
    ret = zb_get_reply(data,inverter);
    if((13 == ret) && (0xDE == data[3]) && (0xFB == data[0]) && (0xFB == data[1]) && (0xFE == data[11]) && (0xFE == data[12]))
        return 1;
    else
        return -1;
}

int zb_ipp_broadcast(void)		//IPP�㲥
{
    int i=0;
    char sendbuff[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xA5;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xAB;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_broadcast_cmd(sendbuff, i);
    return 1;
}

int zb_ipp_single(inverter_info *inverter)		//IPP����
{
    int i=0;
    char sendbuff[256];

    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0xFB;
    sendbuff[i++] = 0x06;
    sendbuff[i++] = 0xC5;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0x00;
    sendbuff[i++] = 0xCB;
    sendbuff[i++] = 0xFE;
    sendbuff[i++] = 0xFE;

    zb_send_cmd(inverter, sendbuff, i);
    return 1;
}

int process_gfdi(inverter_info *firstinverter)
{
    int i,j;
    FILE *fp;
    char command[256] = {'\0'};
    inverter_info *curinverter = firstinverter;

    fp = fopen("/tmp/procgfdi.con", "r");
    while(1){
        curinverter = firstinverter;
        memset(command, '\0', 256);
        fgets(command, 256, fp);
        if(!strlen(command))
            break;
        if('\n' == command[strlen(command)-1])
            command[strlen(command)-1] = '\0';
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++){
            if(!strcmp(command, curinverter->id))
            {
                j=0;
                while(j<3)
                {
                    if(1 == zb_clear_gfdi(curinverter))
                    {
                        print2msg(ECU_DBG_MAIN,"Clear GFDI", curinverter->id);
                        break;
                    }
                    j++;
                }
                break;
            }
            curinverter++;
        }
    }

    fclose(fp);
    fp = fopen("/tmp/procgfdi.con", "w");
    fclose(fp);

    return 0;
}

int process_turn_on_off(inverter_info *firstinverter)
{
    int i, j;
    FILE *fp;
    char command[256] = {'\0'};
    inverter_info *curinverter = firstinverter;

    fp = fopen("/tmp/connect.con", "r");
    if(!fp)
        return -1;
    while(1){
        curinverter = firstinverter;
        memset(command, '\0', 256);
        fgets(command, 256, fp);
        if(!strlen(command))
            break;
        if('\n' == command[strlen(command)-1])
            command[strlen(command)-1] = '\0';
        if(!strncmp(command, "connect all", 11)){
            zb_turnon_inverter_broadcast();
            printmsg(ECU_DBG_MAIN,"turn on all");
            break;
        }
        if(!strncmp(command, "disconnect all", 14)){
            zb_shutdown_broadcast();
            printmsg(ECU_DBG_MAIN,"turn off all");
            break;
        }

        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++){
            if(!strncmp(command, curinverter->id, 12))
            {
                j = 0;
                if('c' == command[12])
                {
                    while(j<3)
                    {
                        if(1 == zb_boot_single(curinverter))
                        {
                            print2msg(ECU_DBG_MAIN,"turn on", curinverter->id);
                            break;
                        }
                        j++;
                    }
                }
                if('d' == command[12])
                {
                    while(j<3)
                    {
                        if(1 == zb_shutdown_single(curinverter))
                        {
                            print2msg(ECU_DBG_MAIN,"turn off", curinverter->id);
                            break;
                        }
                        j++;
                    }
                }
            }
            curinverter++;
        }
    }

    fclose(fp);
    fp = fopen("/tmp/connect.con", "w");
    fclose(fp);

    return 0;

}

int process_quick_boot(inverter_info *firstinverter)
{
    int i;
    FILE *fp;
    char flag_quickboot = '0';				//����������־
    inverter_info *curinverter = firstinverter;

    fp = fopen("/tmp/qckboot.con", "r");

    if(fp)
    {
        flag_quickboot = fgetc(fp);
        fclose(fp);
        if('1' == flag_quickboot)
        {
            curinverter = firstinverter;
            for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++)
            {
                zb_boot_waitingtime_single(curinverter);
                print2msg(ECU_DBG_MAIN,"quick boot",curinverter->id);
                curinverter++;
            }
        }
        fp = fopen("/tmp/qckboot.con", "w");
        if(fp)
        {
            fputs("0",fp);
            fclose(fp);
        }
    }
    return 0;

}

int process_ipp(inverter_info *firstinverter)
{
    int i, j;
    FILE *fp;
    char command[256] = {'\0'};
    inverter_info *curinverter = firstinverter;

    fp = fopen("/tmp/ipp.con", "r");

    if(fp)
    {
        while(1)
        {
            curinverter = firstinverter;
            memset(command, '\0', 256);
            fgets(command, 256, fp);
            if(!strlen(command))
                break;
            if('\n' == command[strlen(command)-1])
                command[strlen(command)-1] = '\0';
            if(!strncmp(command, "set ipp all", 11)){
                zb_ipp_broadcast();
                printmsg(ECU_DBG_MAIN,"set ipp all");
                break;
            }

            for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++){
                if(!strcmp(command, curinverter->id))
                {
                    j=0;
                    while(j<3)
                    {
                        if(1 == zb_ipp_single(curinverter))
                        {
                            print2msg(ECU_DBG_MAIN,"set ipp single", curinverter->id);
                            break;
                        }
                        j++;
                    }
                    break;
                }
                curinverter++;
            }
        }

        fclose(fp);
    }
    fp = fopen("/tmp/ipp.con", "w");
    fclose(fp);

    return 0;


}




int process_all(inverter_info *firstinverter)
{
    processpower(firstinverter);			//���ù���Ԥ��ֵ,ZK,3.10�иĶ�	     OK
    //	process_gfdi(firstinverter);			//��GFDI��־
    //	process_protect_data(firstinverter);	//����Ԥ��ֵ
    process_turn_on_off(firstinverter);		//���ػ� 	����DRM����
    process_quick_boot(firstinverter);		//��������
    process_ipp(firstinverter);				//IPP�趨
    process_ird_all(firstinverter);		//0K
    process_ird(firstinverter);
    turn_on_off(firstinverter);								//���ػ�,ZK,3.10����	   OK
    clear_gfdi(firstinverter);								//��GFDI��־,ZK,3.10���� OK
    set_protection_parameters(firstinverter);				//����Ԥ��ֵ�㲥,ZK,3.10����
    set_protection_parameters_inverter_one(firstinverter);  //����Ԥ��ֵ����,ZK,3.10����
    process_ZigBeeTransimission();

    //save_A145_inverter_to_all();
    processAllFlag = 0;
    return 0;
}

int getalldata(inverter_info *firstinverter,int time_linux)		//��ȡÿ�������������
{
    int i, j,dataflag_clear = 0;
    inverter_info *curinverter = firstinverter;
    int count=0, syspower=0;
    float curenergy=0;
    int out_flag = 0;	//������ѯ��־
#if 0
    char buff[50] = {'\0'};
    int fd;
#endif 

    for(i=0;i<3;i++)
    {
        if(-1==zb_test_communication())
        {
            zigbee_reset();
        }
        else
            break;
    }
    //calibration_time_broadcast(firstinverter, time_linux); 	//YC1000�������㲥У׼ʱ��
    process_all(firstinverter);
    for(j=0; j<5; j++)
    {
        out_flag = 0;
        curinverter = firstinverter;

        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++)			//ÿ����������Ҫ5������
        {
            if(dataflag_clear == 0)
            {
                curinverter->inverterstatus.dataflag = 0;		//û�н��յ����ݾ���Ϊ0
                curinverter->dv=0;
                curinverter->di=0;
                curinverter->op=0;
                curinverter->gf=0;
                curinverter->it=0;
                curinverter->gv=0;

                curinverter->dvb=0;
                curinverter->dib=0;
                curinverter->opb=0;

                curinverter->curgeneration = 0;
                curinverter->curgenerationb = 0;
                curinverter->status_send_flag=0;

                rt_memset(curinverter->status_web, '\0', sizeof(curinverter->status_web));		//????????
                rt_memset(curinverter->status, '\0', sizeof(curinverter->status));
                rt_memset(curinverter->statusb, '\0', sizeof(curinverter->statusb));
            }

            if((0 == curinverter->inverterstatus.dataflag) && (0 != curinverter->shortaddr))
            {
                if(1 != curinverter->inverterstatus.bindflag)
                {
                    if(1 == zb_turnoff_limited_rptid(curinverter->shortaddr,curinverter))
                    {
                        curinverter->inverterstatus.bindflag = 1;			//���������־λ1
                        updateID();
                    }else
                        out_flag = 1;

                }
                if((0 == curinverter->model) )//&& (1 == curinverter->inverterstatus.bindflag))
                {
                    if(1 == zb_query_inverter_info(curinverter))
                        updateID();
                    else
                        out_flag = 1;

                }

                if((0 != curinverter->model) )//&& (1 == curinverter->inverterstatus.bindflag))
                {
                    //print2msg(ECU_DBG_MAIN,"querydata",curinverter->id);
                    if(-1 == zb_query_data(curinverter))
                        out_flag = 1;
                    rt_thread_delay(20);
                }
            }
            curinverter++;
        }
        dataflag_clear = 1;
        if(out_flag == 0) break;
    }
    ecu.polling_total_times++;				//ECU����ѵ��1 ,ZK
#if 0	
    fd = open("/TMP/DISCON.TXT", O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd >= 0) {
        curinverter = firstinverter;
        for(i=0; i<MAXINVERTERCOUNT; i++, curinverter++)						//ͳ�Ƶ�ǰһ���������ECUûͨѶ�ϵ��ܴ���, ZK
        {
            if((0 == curinverter->inverterstatus.dataflag)&&(12 == strlen(curinverter->id)))
            {
                curinverter->disconnect_times++;
                if(curinverter->disconnect_times > 254)
                {
                    curinverter->disconnect_times= 254;
                }
                sprintf(buff, "%s-%d-%d\n", curinverter->id,curinverter->disconnect_times,ecu.polling_total_times);
                write(fd, buff, strlen(buff));
            }
            else if((1 == curinverter->inverterstatus.dataflag)&&(12 == strlen(curinverter->id)))
            {
                sprintf(buff, "%s-%d-%d\n", curinverter->id,curinverter->disconnect_times,ecu.polling_total_times);
                write(fd, buff, strlen(buff));
            }
        }
        close(fd);
    }
#endif 
    curinverter = firstinverter;
    for(i=0; i<MAXINVERTERCOUNT; i++, curinverter++)		//ͳ������û�л�ȡ�����ݵ������ ZK,һ�����յ����ݣ��˱�������
    {
        if((0 == curinverter->inverterstatus.dataflag)&&(12 == strlen(curinverter->id)))
        {
            curinverter->no_getdata_num++;
            if(curinverter->no_getdata_num>254)
            {
                curinverter->no_getdata_num = 254;
            }
        }
    }

    curinverter = firstinverter;
    for(i=0; i<MAXINVERTERCOUNT; i++, curinverter++){							//ͳ�Ƶ�ǰ���ٸ������
        if((1 == curinverter->inverterstatus.dataflag)&&(12 == strlen(curinverter->id)))
            count++;
    }
    ecu.count = count;

    curinverter = firstinverter;
    for(syspower=0, i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++){		//���㵱ǰһ��ϵͳ����
        if(1 == curinverter->inverterstatus.dataflag){
            syspower += curinverter->op;
            syspower += curinverter->opb;
            syspower += curinverter->opc;
            syspower += curinverter->opd;
        }
    }
    ecu.system_power = syspower;

    curinverter = firstinverter;
    for(curenergy=0, i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++){		//���㵱ǰһ�ַ�����
        if(1 == curinverter->inverterstatus.dataflag){
            if((curinverter->model==5) || (curinverter->model==6) || (curinverter->model==7)|| (curinverter->model==8) || (curinverter->model==0x17))
            {
                curenergy += curinverter->curgeneration;
                curenergy += curinverter->curgenerationb;
                curenergy += curinverter->curgenerationc;
                curenergy += curinverter->curgenerationd;
            }else
            {
                curenergy += curinverter->output_energy;
                curenergy += curinverter->output_energyb;
                curenergy += curinverter->output_energyc;
            }

        }
    }
    ecu.current_energy = curenergy;
    update_tmpdb(firstinverter);
#if 0
    fd = open("/TMP/IDNOBIND.TXT", O_WRONLY | O_CREAT | O_TRUNC, 0); 	//Ϊ��ͳ����ʾ�ж̵�ַ����û�а󶨵������ID
    if (fd >= 0)
    {

        curinverter = firstinverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)
        {
            if((1!=curinverter->inverterstatus.bindflag)&&(0!=curinverter->shortaddr))
            {
                memset(buff,0,50);
                sprintf(buff,"%s\n",curinverter->id);
                write(fd, buff, strlen(buff));
            }
        }
        close(fd);
    }

    fd = open("/TMP/SIGNALST.TXT", O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd >= 0)
    {
        curinverter = firstinverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)	 //ͳ��ÿ����������ź�ǿ��, ZK
        {
            memset(buff,0,50);
            sprintf(buff, "%s%X\n", curinverter->id,curinverter->signalstrength);
            write(fd, buff, strlen(buff));

        }
        close(fd);
    }

    fd = open("/TMP/RADUIS.TXT", O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd >= 0)
    {
        curinverter = firstinverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)	 //ͳ��ÿ̨��������ź�ǿ��, ZK
        {
            memset(buff,0,50);
            sprintf(buff, "%s%X\n", curinverter->id,curinverter->raduis);
            write(fd, buff, strlen(buff));
        }
        close(fd);
    }


#endif
    save_turn_on_off_changed_result(firstinverter);
    save_gfdi_changed_result(firstinverter);

    return ecu.count;
}

int get_inverter_shortaddress(inverter_info *firstinverter)		//��ȡû�����ݵ�������Ķ̵�ַ
{
    int i;
    inverter_info *curinverter = firstinverter;
    unsigned short current_panid;				//Zigbee��ʱ��PANID
    int flag = 0;


    curinverter = firstinverter;
    for(i=0; (i<MAXINVERTERCOUNT)&&(12==rt_strlen(curinverter->id)); i++)
    {
        if((0==curinverter->shortaddr)||(curinverter->no_getdata_num>5))
        {
            flag=1;
            break;
        }
        curinverter++;
    }

    if(1==flag)
    {
        if(1==zb_restore_ecu_panid_0xffff(0x10))		//��ECU��PANID����ΪĬ�ϵ�0xffff
            current_panid = 0xffff;
        printdecmsg(ECU_DBG_MAIN,"PANID",current_panid);
        rt_thread_delay(500);


        curinverter = firstinverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==rt_strlen(curinverter->id)); i++)
        {
            if((0==curinverter->shortaddr)||(curinverter->no_getdata_num>5))
            {
                zb_change_inverter_panid_single(curinverter);		//��û�����ݵ������PANID�޸ĳ�ECU��PANID
                rt_thread_delay(50);
            }
            curinverter++;
        }

        if(1==zb_change_ecu_panid())						//��ECU��panid�ĳɴ�̨ECUԭ��panid
            current_panid = ecu.panid;
        printdecmsg(ECU_DBG_MAIN,"PANID",current_panid);
        rt_thread_delay(500);

        curinverter = firstinverter;
        for(i=0; (i<MAXINVERTERCOUNT)&&(12==rt_strlen(curinverter->id)); i++)
        {
            printdecmsg(ECU_DBG_MAIN,"no_getdata_num",curinverter->no_getdata_num);
            if((0==curinverter->shortaddr)||(curinverter->no_getdata_num>5))
            {
                zb_get_inverter_shortaddress_single(curinverter);
            }
            curinverter++;
        }
    }
    return 1;
}

int bind_nodata_inverter(inverter_info *firstinverter)		//��û�����ݵ������,���һ�ȡ�̵�ַ
{
    int i;
    inverter_info *curinverter = firstinverter;

    curinverter = firstinverter;
    for(i=0; (i<MAXINVERTERCOUNT)&&(12==rt_strlen(curinverter->id)); i++)
    {
        if(curinverter->no_getdata_num>0)
        {
            //			zb_turnoff_limited_rptid(curinverter->shortaddr,curinverter);
            zb_get_inverter_shortaddress_single(curinverter);
        }
        curinverter++;
    }
    return 0;
}

//����ı��������PANIDΪECU��MAC��ַ����λ,�ŵ�Ϊָ���ŵ�(ע:��Ҫ��ECU��PANID��Ϊ0xFFFF(���ܷ���))
int zb_change_inverter_channel_one(char *inverter_id, int channel)
{
    char sendbuff[256] = {'\0'};
    int i;
    int check=0;
    rt_hw_ms_delay(500);

    clear_zbmodem();
    sendbuff[0]  = 0xAA;
    sendbuff[1]  = 0xAA;
    sendbuff[2]  = 0xAA;
    sendbuff[3]  = 0xAA;
    sendbuff[4]  = 0x0F;
    sendbuff[5]  = 0x00;
    sendbuff[6]  = 0x00;
    sendbuff[7]  = ecu.panid>>8;
    sendbuff[8]  = ecu.panid;
    sendbuff[9]  = channel;
    sendbuff[10] = 0x00;
    sendbuff[11] = 0xA0;
    for(i=4;i<12;i++)
        check=check+sendbuff[i];
    sendbuff[12] = check/256;
    sendbuff[13] = check%256;
    sendbuff[14] = 0x06;
    sendbuff[15]=((inverter_id[0]-0x30)*16+(inverter_id[1]-0x30));
    sendbuff[16]=((inverter_id[2]-0x30)*16+(inverter_id[3]-0x30));
    sendbuff[17]=((inverter_id[4]-0x30)*16+(inverter_id[5]-0x30));
    sendbuff[18]=((inverter_id[6]-0x30)*16+(inverter_id[7]-0x30));
    sendbuff[19]=((inverter_id[8]-0x30)*16+(inverter_id[9]-0x30));
    sendbuff[20]=((inverter_id[10]-0x30)*16+(inverter_id[11]-0x30));
    ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, 21);
    printhexmsg(ECU_DBG_MAIN,"Change Inverter Channel (one)", sendbuff, 21);

    rt_thread_delay(100); //�˴���ʱ�������1S
    return 0;
}

//�ر������ID�ϱ� + �������
int zb_off_report_id_and_bind(int short_addr)
{
    int times = 3;
    char sendbuff[16] = {'\0'};
    char recvbuff[256] = {'\0'};
    int i;
    int check=0;

    do {
        //���͹ر������ID�ϱ�+�󶨲���
        clear_zbmodem();
        sendbuff[0]  = 0xAA;
        sendbuff[1]  = 0xAA;
        sendbuff[2]  = 0xAA;
        sendbuff[3]  = 0xAA;
        sendbuff[4]  = 0x08;
        sendbuff[5]  = short_addr>>8;
        sendbuff[6]  = short_addr;
        sendbuff[7]  = 0x00; //PANID(�����������)
        sendbuff[8]  = 0x00; //PANID(�����������)
        sendbuff[9]  = 0x00; //�ŵ�(�����������)
        sendbuff[10] = 0x00; //����(�����������)
        sendbuff[11] = 0xA0;
        for(i=4;i<12;i++)
            check=check+sendbuff[i];
        sendbuff[12] = check/256;
        sendbuff[13] = check%256;
        sendbuff[14] = 0x00;
        ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL,0, sendbuff, 15);
        printhexmsg(ECU_DBG_MAIN,"Bind ZigBee", sendbuff, 15);

        //���������Ӧ��(�̵�ַ,ZigBee�汾��,�ź�ǿ��)
        if ((11 == zb_get_reply_from_module(recvbuff))
                && (0xA5 == recvbuff[2])
                && (0xA5 == recvbuff[3])) {
            //update_turned_off_rpt_flag(short_addr, (int)recvbuff[9]);
            //update_bind_zigbee_flag(short_addr);
            printmsg(ECU_DBG_MAIN,"Bind Successfully");
            return 1;
        }
    }while(--times);

    return 0;
}

int zigbeeRecvMsg(char *data, int timeout_sec)
{
    int count;
    if (selectZigbee(timeout_sec*100) <= 0) {
        printmsg(ECU_DBG_MAIN,"Get reply time out");
        return -1;
    } else {

        count = ZIGBEE_SERIAL.read(&ZIGBEE_SERIAL,0, data, 255);
        printhexmsg(ECU_DBG_MAIN,"Reply", data, count);
        return count;
    }
}

#if 0
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(zigbee_reset, reset zigbee module.)
void Zbtestcomm()		//����ͨ��
{
    zb_test_communication();
}
FINSH_FUNCTION_EXPORT(Zbtestcomm,zigbee test communication.)

void Zbecupanidffff(int channel)		//����ECU panidΪffff �ŵ�Ϊchannel
{
    zb_restore_ecu_panid_0xffff(channel);
}
FINSH_FUNCTION_EXPORT(Zbecupanidffff,zb_restore_ecu_panid_0xffff channel[int].)

void Zbecupanid()		//����ECU panid �Լ� �ŵ�   ΪECUȫ�ֱ�����ֵ	
{
    zb_change_ecu_panid();
}
FINSH_FUNCTION_EXPORT(Zbecupanid,zb_change_ecu_panid.)

void Zbecupanid8888(int channel)	//����ECU panidΪ0x8888  �ŵ���ECUȫ�ֱ�����ֵ
{
    zb_restore_ecu_panid_0x8888();
}
FINSH_FUNCTION_EXPORT(Zbecupanid8888,zb_restore_ecu_panid_0x8888.)

void Zbinvchone(char *inverter_id, int channel)	//���������panid���ŵ� ����panidΪȫ�ֱ���ecu���ŵ�
{
    zb_change_inverter_channel_one(inverter_id,channel);
}
FINSH_FUNCTION_EXPORT(Zbinvchone,zb_change_inverter_channel_one inverter_id[char *12] channel[int].)

void Zbgetshadd(char *inverter_id)	//��ȡ������̵�ַ
{
    inverter_info inverter;
    rt_memset(&inverter,0x00,sizeof(inverter));
    rt_memcpy(inverter.id,inverter_id,12);
    zb_get_inverter_shortaddress_single(&inverter);
}
FINSH_FUNCTION_EXPORT(Zbgetshadd,zb_get_inverter_shortaddress_single inverter_id[char *12].)

void Zbgetdata(char *inverter_id)	//��ȡ����
{
    inverter_info inverter;
    rt_memset(&inverter,0x00,sizeof(inverter));
    rt_memcpy(inverter.id,inverter_id,12);
    inverter.model = 7;
    zb_get_inverter_shortaddress_single(&inverter);
    zb_query_data(&inverter);
}
FINSH_FUNCTION_EXPORT(Zbgetdata,zb_query_data inverter_id[char *12].)
#endif
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(zigbee_reset , zigbee_reset.)
#endif

