/*****************************************************************************/
/* File      : threadlist.c                                                 */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-02-20 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <threadlist.h>
#include <board.h>
#include <rtthread.h>
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <lwip/netdb.h> /* 为了解析主机名，需要包含netdb.h头文件 */
#include <lwip/sockets.h> /* 使用BSD socket，需要包含sockets.h头文件 */
#include <zigbee.h>
#include "debug.h"
#include "SEGGER_RTT.h"
#include "myfile.h"
#include "mcp1316.h"
#include "rtc.h"
#include "watchdog.h"
#include "timer.h"

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include <dfs_init.h>
#include <dfs_elm.h>
#endif

#ifdef RT_USING_LWIP
#include <stm32_eth.h>
#include <netif/ethernetif.h>
extern int lwip_system_init(void);
#endif

#ifdef RT_USING_FINSH
#include <shell.h>
#include <finsh.h>
#endif

#ifdef EEPROM		
#include "Flash_24L512.h"
#endif
#include "ds1302z_rtc.h"
#include "usart5.h"
/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
#ifdef THREAD_PRIORITY_LED
#include "led.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[500];
static struct rt_thread led_thread;
#endif

#ifdef THREAD_PRIORITY_DRM
#include "DRMConnect.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t DRM_stack[1024];
static struct rt_thread DRM_thread;
#endif

#ifdef THREAD_PRIORITY_MAIN
#include "main-thread.h"
ALIGN(RT_ALIGN_SIZE)
rt_uint8_t main_stack[ 4096 ];
struct rt_thread main_thread;
#endif

#ifdef THREAD_PRIORITY_CLIENT
#include "client.h"
ALIGN(RT_ALIGN_SIZE)
rt_uint8_t client_stack[ 4096 ];
struct rt_thread client_thread;
#endif

#ifdef THREAD_PRIORITY_CONTROL_CLIENT
#include "control_client.h"
ALIGN(RT_ALIGN_SIZE)
rt_uint8_t control_client_stack[ 4096 ];
struct rt_thread control_client_thread;
#endif

#ifdef THREAD_PRIORITY_UPDATE
#include "remoteUpdate.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t update_stack[2048];
static struct rt_thread update_thread;
#endif

#ifdef THREAD_PRIORITY_IDWRITE
#include "idwrite.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t idwrite_stack[2048];
static struct rt_thread idwrite_thread;
#endif

#ifdef THREAD_PRIORITY_LAN8720_RST
#include "lan8720rst.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t lan8720_rst_stack[400];
static struct rt_thread lan8720_rst_thread;
#endif 

#ifdef THREAD_PRIORITY_NTP
#include "ntpapp.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t ntp_stack[1024];
static struct rt_thread ntp_thread;
#endif 

#ifdef THREAD_PRIORITY_PHONE_SERVER
#include "phoneServer.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t phone_server_stack[4096];
static struct rt_thread phone_server_thread;
#endif


rt_mutex_t record_data_lock = RT_NULL;
rt_mutex_t usr_wifi_lock = RT_NULL;
extern rt_mutex_t wifi_uart_lock;

unsigned char LED_Status = 0;

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
extern void cpu_usage_init(void);
extern void cpu_usage_get(rt_uint8_t *major, rt_uint8_t *minor);

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   Device Init program entry                                               */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   parameter[in]   unused                                                  */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
void rt_init_thread_entry(void* parameter)
{
    {
        extern void rt_platform_init(void);
        rt_platform_init();
    }

#ifdef EEPROM		
	  EEPROM_Init();
#endif
		
	/* Filesystem Initialization */
#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
	/* initialize the device file system */
	dfs_init();

	/* initialize the elm chan FatFS file system*/
	elm_init();
    
    /* mount flash fat partition 1 as root directory */
    if (dfs_mount("flash", "/", "elm", 0, 0) == 0)
    {
#if ECU_JLINK_DEBUG	
    	SEGGER_RTT_printf(0,"File System initialized!\n");
#endif
        rt_kprintf("File System initialized!\n");
    }
    else
    {
#if ECU_JLINK_DEBUG	
    	SEGGER_RTT_printf(0,"File System initialzation failed!\n");
#endif
		rt_kprintf("File System initialzation failed!\n");
		dfs_mkfs("elm","flash");
		if (dfs_mount("flash", "/", "elm", 0, 0) == 0)
		{
#if ECU_JLINK_DEBUG	
			SEGGER_RTT_printf(0,"File System initialized!\n");
#endif
			rt_kprintf("File System initialized!\n");
		}
		initPath();
#if ECU_JLINK_DEBUG	
		SEGGER_RTT_printf(0,"PATH initialized!\n");
#endif
		rt_kprintf("PATH initialized!\n");
    }
#endif /* RT_USING_DFS && RT_USING_DFS_ELMFAT */

#ifdef RT_USING_LWIP
	/* initialize eth interface */
	rt_hw_stm32_eth_init();

	/* initialize lwip stack */
	/* register ethernetif device */
	eth_system_device_init();

	/* initialize lwip system */
	lwip_system_init();
	
#if ECU_JLINK_DEBUG	
	SEGGER_RTT_printf(0,"TCP/IP initialized!\n");
#endif
	rt_kprintf("TCP/IP initialized!\n");
#endif


#ifdef RT_USING_FINSH
	/* initialize finsh */
	finsh_system_init();
	finsh_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
	/* initialize rtc */
	rt_hw_rtc_init();		
		
	/* initialize lock for home/record/data */
	record_data_lock = rt_mutex_create("record_data_lock", RT_IPC_FLAG_FIFO);
	if (record_data_lock != RT_NULL)
	{
#if ECU_JLINK_DEBUG
		SEGGER_RTT_printf(0,"Initialize record_data_lock successful!\n");
#endif 
		rt_kprintf("Initialize record_data_lock successful!\n");
	}
	/* WiFi Serial Initialize*/
	if(usr_wifi_lock == NULL)
	{
		usr_wifi_lock = rt_mutex_create("usr_wifi_lock", RT_IPC_FLAG_FIFO);
	}
	
	/* WiFi Serial Initialize*/
	if(wifi_uart_lock == NULL)
	{
		wifi_uart_lock = rt_mutex_create("wifi_uart_lock", RT_IPC_FLAG_FIFO);
	}
	
	TIM2_Int_Init(14999,7199);    //心跳包超时事件定时器初始化
	cpu_usage_init();
	WIFI_Reset();
	sysDirDetection();
	
}

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   LED program entry                                                       */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   parameter[in]   unused                                                  */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
#ifdef THREAD_PRIORITY_LED
static void led_thread_entry(void* parameter)
{	
	rt_uint8_t major,minor;
	/* Initialize led */
	rt_hw_led_init();
	rt_hw_watchdog_init();
	MCP1316_init();

	while (1)
	{
		kickwatchdog();
		MCP1316_kickwatchdog();
		if(LED_Status == 0)
		{
			rt_hw_led_off();
		}else
		{
			rt_hw_led_on();
		}
		
		rt_thread_delay( RT_TICK_PER_SECOND);
		cpu_usage_get(&major, &minor);
		//printf("CPU : %d.%d%\n", major, minor);
    }
}
#endif

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   Lan8720 Reset program entry                                             */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   parameter[in]   unused                                                  */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
#ifdef THREAD_PRIORITY_LAN8720_RST
static void lan8720_rst_thread_entry(void* parameter)
{
	int value;
	char Time[15] = {'\0'};
	while (1)
	{
		value = ETH_ReadPHYRegister(0x00, 0);
			
		if(0 == value)	//判断控制寄存器是否变为0  表示断开
		{
			//printf("reg 0:%x\n",value);
			rt_hw_lan8720_rst();
		}

		apstime(Time);
		if(!memcmp(&Time[8],"0200",4))
		{
			printf("reboot :%s\n",Time);
			reboot();
		}
			
      	rt_thread_delay( RT_TICK_PER_SECOND*50 );
	}


}
#endif

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   NTP program entry                                                       */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   parameter[in]   unused                                                  */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
#ifdef THREAD_PRIORITY_NTP
static void ntp_thread_entry(void* parameter)
{
	int i = 0;
	rt_thread_delay(START_TIME_NTP * RT_TICK_PER_SECOND);
	while(1)
	{
		printmsg(ECU_DBG_NTP,"start--------------------------------------------------------");
		i = 0;
		for(i = 0;i < 5;i++)
		{
			if(0 == get_time_from_NTP())
				break;
			rt_thread_delay(RT_TICK_PER_SECOND * 10);
		}
		printmsg(ECU_DBG_NTP,"end----------------------------------------------------------");
		//rt_thread_delay(RT_TICK_PER_SECOND * 60);
		rt_thread_delay(RT_TICK_PER_SECOND * 86400);
		
	}

}
#endif

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   Create Application ALL Tasks                                            */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
void tasks_new(void)
{
	rt_err_t result;
	rt_thread_t tid;
	
	/* init init thread */
  tid = rt_thread_create("init",rt_init_thread_entry, RT_NULL,1024, THREAD_PRIORITY_INIT, 20);
	if (tid != RT_NULL) rt_thread_startup(tid);
	
#ifdef THREAD_PRIORITY_LED
  /* init led thread */
  result = rt_thread_init(&led_thread,"led",led_thread_entry,RT_NULL,(rt_uint8_t*)&led_stack[0],sizeof(led_stack),THREAD_PRIORITY_LED,5);
  if (result == RT_EOK)	rt_thread_startup(&led_thread);
#endif
	
#ifdef	THREAD_PRIORITY_DRM
  result = rt_thread_init(&DRM_thread,"DRM",DRM_Connect_thread_entry,RT_NULL,(rt_uint8_t*)&DRM_stack[0],sizeof(DRM_stack),THREAD_PRIORITY_DRM,5);
  if (result == RT_EOK)	rt_thread_startup(&DRM_thread);
#endif

#ifdef THREAD_PRIORITY_LAN8720_RST
  /* init LAN8720RST thread */
  result = rt_thread_init(&lan8720_rst_thread,"lanrst",lan8720_rst_thread_entry,RT_NULL,(rt_uint8_t*)&lan8720_rst_stack[0],sizeof(lan8720_rst_stack),THREAD_PRIORITY_LAN8720_RST,5);
  if (result == RT_EOK)	rt_thread_startup(&lan8720_rst_thread);
#endif
	
#ifdef THREAD_PRIORITY_NTP
  /* init ntp thread */
  result = rt_thread_init(&ntp_thread,"ntp",ntp_thread_entry,RT_NULL,(rt_uint8_t*)&ntp_stack[0],sizeof(ntp_stack),THREAD_PRIORITY_NTP,5);
  if (result == RT_EOK) rt_thread_startup(&ntp_thread);
#endif
	
#ifdef THREAD_PRIORITY_UPDATE	
  /* init update thread */
	result = rt_thread_init(&update_thread,"update",remote_update_thread_entry,RT_NULL,(rt_uint8_t*)&update_stack[0],sizeof(update_stack),THREAD_PRIORITY_UPDATE,5);
  if (result == RT_EOK) rt_thread_startup(&update_thread);
#endif
	
#ifdef THREAD_PRIORITY_IDWRITE	
  /* init idwrite thread */
	result = rt_thread_init(&idwrite_thread,"idwrite",idwrite_thread_entry,RT_NULL,(rt_uint8_t*)&idwrite_stack[0],sizeof(idwrite_stack),THREAD_PRIORITY_IDWRITE,5);
  if (result == RT_EOK) rt_thread_startup(&idwrite_thread);
#endif
		
#ifdef THREAD_PRIORITY_MAIN
	/* init main thread */
	result = rt_thread_init(&main_thread,"main",main_thread_entry,RT_NULL,(rt_uint8_t*)&main_stack[0],sizeof(main_stack),THREAD_PRIORITY_MAIN,5);
  if (result == RT_EOK) rt_thread_startup(&main_thread);
#endif
	
#ifdef THREAD_PRIORITY_CLIENT
	/* init client thread */
	result = rt_thread_init(&client_thread,"client",client_thread_entry,RT_NULL,(rt_uint8_t*)&client_stack[0],sizeof(client_stack),THREAD_PRIORITY_CLIENT,5);
  if (result == RT_EOK)	rt_thread_startup(&client_thread);
#endif
	
#ifdef THREAD_PRIORITY_CONTROL_CLIENT
	result = rt_thread_init(&control_client_thread,"control",control_client_thread_entry,RT_NULL,(rt_uint8_t*)&control_client_stack[0],sizeof(control_client_stack),THREAD_PRIORITY_CONTROL_CLIENT,5);
  if (result == RT_EOK)	rt_thread_startup(&control_client_thread);
#endif	
	
#ifdef THREAD_PRIORITY_PHONE_SERVER
	result = rt_thread_init(&phone_server_thread,"phone",phone_server_thread_entry,RT_NULL,(rt_uint8_t*)&phone_server_stack[0],sizeof(phone_server_stack),THREAD_PRIORITY_PHONE_SERVER,5);
  if (result == RT_EOK)	rt_thread_startup(&phone_server_thread);
#endif	
}

/*****************************************************************************/
/* Function Description:                                                     */
/*****************************************************************************/
/*   Reset Application Tasks                                                 */
/*****************************************************************************/
/* Parameters:                                                               */
/*****************************************************************************/
/*   type[In]:                                                               */
/*            TYPE_LED           :  Reset Led Task                           */
/*            TYPE_LANRST        :  Reset Lan8720 reset Task                 */
/*            TYPE_UPDATE        :  Reset Update Task                        */
/*            TYPE_IDWRITE       :  Reset Idwrote Task                       */
/*            TYPE_MAIN          :  Reset Main Task                          */
/*            TYPE_CLIENT        :  Reset Client Task                        */
/*            TYPE_CONTROL_CLIENT:  Reset Control client Task                */
/*****************************************************************************/
/* Return Values:                                                            */
/*****************************************************************************/
/*   void                                                                    */
/*****************************************************************************/
void restartThread(threadType type)
{
	rt_err_t result;
	switch(type)
	{
#ifdef THREAD_PRIORITY_LED
		case TYPE_LED:
			rt_thread_detach(&led_thread);
			/* init led thread */
			result = rt_thread_init(&led_thread,"led",led_thread_entry,RT_NULL,(rt_uint8_t*)&led_stack[0],sizeof(led_stack),THREAD_PRIORITY_LED,5);
			if (result == RT_EOK)	rt_thread_startup(&led_thread);
			break;
#endif 

#ifdef THREAD_PRIORITY_LAN8720_RST
		case TYPE_LANRST:
			rt_thread_detach(&lan8720_rst_thread);
			/* init LAN8720RST thread */
			result = rt_thread_init(&lan8720_rst_thread,"lanrst",lan8720_rst_thread_entry,RT_NULL,(rt_uint8_t*)&lan8720_rst_stack[0],sizeof(lan8720_rst_stack),THREAD_PRIORITY_LAN8720_RST,5);
			if (result == RT_EOK)	rt_thread_startup(&lan8720_rst_thread);
			break;
#endif 			
			
#ifdef THREAD_PRIORITY_UPDATE
		case TYPE_UPDATE:
			rt_thread_detach(&update_thread);
		  /* init update thread */
			result = rt_thread_init(&update_thread,"update",remote_update_thread_entry,RT_NULL,(rt_uint8_t*)&update_stack[0],sizeof(update_stack),THREAD_PRIORITY_UPDATE,5);
			if (result == RT_EOK)	rt_thread_startup(&update_thread);
			break;
#endif
			
#ifdef THREAD_PRIORITY_IDWRITE
		case TYPE_IDWRITE:
			rt_thread_detach(&idwrite_thread);
			/* init idwrite thread */
			result = rt_thread_init(&idwrite_thread,"idwrite",idwrite_thread_entry,RT_NULL,(rt_uint8_t*)&idwrite_stack[0],sizeof(idwrite_stack),THREAD_PRIORITY_IDWRITE,5);
			if (result == RT_EOK)	rt_thread_startup(&idwrite_thread);
			break;
#endif
			
#ifdef THREAD_PRIORITY_MAIN
		case TYPE_MAIN:
			rt_thread_detach(&main_thread);
			/* init main thread */
			result = rt_thread_init(&main_thread,"main",main_thread_entry,RT_NULL,(rt_uint8_t*)&main_stack[0],sizeof(main_stack),THREAD_PRIORITY_MAIN,5);
			if (result == RT_EOK)	rt_thread_startup(&main_thread);
			break;
#endif
		
#ifdef THREAD_PRIORITY_CLIENT
		case TYPE_CLIENT:
			rt_thread_detach(&client_thread);
			/* init client thread */
			result = rt_thread_init(&client_thread,"client",client_thread_entry,RT_NULL,(rt_uint8_t*)&client_stack[0],sizeof(client_stack),THREAD_PRIORITY_CLIENT,5);
			if (result == RT_EOK)	rt_thread_startup(&client_thread);
			break;
#endif
		
#ifdef THREAD_PRIORITY_CONTROL_CLIENT
		case TYPE_CONTROL_CLIENT:
			rt_thread_detach(&control_client_thread);
			result = rt_thread_init(&control_client_thread,"control",control_client_thread_entry,RT_NULL,(rt_uint8_t*)&control_client_stack[0],sizeof(control_client_stack),THREAD_PRIORITY_CONTROL_CLIENT,5);
			if (result == RT_EOK)	rt_thread_startup(&control_client_thread);
			break;
#endif 
			
#ifdef THREAD_PRIORITY_NTP
		case TYPE_NTP:
			rt_thread_detach(&ntp_thread);
			result = rt_thread_init(&ntp_thread,"ntp",ntp_thread_entry,RT_NULL,(rt_uint8_t*)&ntp_stack[0],sizeof(ntp_stack),THREAD_PRIORITY_NTP,5);
			if (result == RT_EOK)	rt_thread_startup(&ntp_thread);
			break;
#endif 

		default:
			break;
	}
}





//定时器启动
rt_timer_t threadRestarttimer;	//线程重启定时器
threadType threadRestartType;



//定时器超时函数   复位超时
static void threadRestartTimeout(void* parameter)
{
	restartThread(threadRestartType);
}

//定时重启某个线程
void threadRestartTimer(int timeout,threadType Type)			
{
	threadRestartType = Type;
	threadRestarttimer = rt_timer_create("Restart", /* 定时器名字为 read */
					threadRestartTimeout, /* 超时时回调的处理函数 */
					RT_NULL, /* 超时函数的入口参数 */
					timeout*RT_TICK_PER_SECOND, /* 定时时间长度,以OS Tick为单位*/
					 RT_TIMER_FLAG_ONE_SHOT); /* 单周期定时器 */
	if (threadRestarttimer != RT_NULL) rt_timer_start(threadRestarttimer);
}


#ifdef RT_USING_FINSH
#include <finsh.h>
void restart(int type)
{
	restartThread((threadType)type);
}
FINSH_FUNCTION_EXPORT(restart, eg:restart());

#include "arch/sys_arch.h"
void dhcpreset(void)
{
	dhcp_reset();
}
FINSH_FUNCTION_EXPORT(dhcpreset, eg:dhcpreset());

void teststaticIP(void)
{
	IP_t IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr;
	IPAddr.IP1 = 192;
	IPAddr.IP2 = 168;
	IPAddr.IP3 = 1;
	IPAddr.IP4 = 192;

	MSKAddr.IP1 = 255;
	MSKAddr.IP2 = 255;
	MSKAddr.IP3 = 255;
	MSKAddr.IP4 = 0;

	GWAddr.IP1 = 192;
	GWAddr.IP2 = 168;
	GWAddr.IP3 = 1;
	GWAddr.IP4 = 1;
	
	DNS1Addr.IP1 = 0;
	DNS1Addr.IP2 = 0;
	DNS1Addr.IP3 = 0;
	DNS1Addr.IP4 = 0;

	DNS1Addr.IP1 = 0;
	DNS1Addr.IP2 = 0;
	DNS1Addr.IP3 = 0;
	DNS1Addr.IP4 = 0;
	
	StaticIP(IPAddr,MSKAddr,GWAddr,DNS1Addr,DNS2Addr);	

}
FINSH_FUNCTION_EXPORT(teststaticIP, eg:teststaticIP());

#endif
