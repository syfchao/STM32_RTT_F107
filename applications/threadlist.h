#ifndef __THREADLIST_H
#define __THREADLIST_H

#include <rtthread.h>

typedef enum THREADTYPE {
	TYPE_LED = 1,
	TYPE_LANRST = 2,
  TYPE_UPDATE = 3,
  TYPE_MAIN = 4,
  TYPE_CLIENT = 5,
  TYPE_CONTROL_CLIENT = 6
}threadType;
//每个线程的优先级   宏打开表示线程打开
#define THREAD_PRIORITY_INIT							10
#define THREAD_PRIORITY_LED               20
#define THREAD_PRIORITY_IDWRITER					20
#define THREAD_PRIORITY_LAN8720_RST				21
#define THREAD_PRIORITY_WIFI_TEST					22
#define THREAD_PRIORITY_NET_TEST					22
//#define THREAD_PRIORITY_UPDATE						21
//#define THREAD_PRIORITY_MAIN	            24
//#define THREAD_PRIORITY_CONTROL_CLIENT		26
//#define THREAD_PRIORITY_CLIENT						26

//每个线程在开机后的启动时间,单位S
#define START_TIME_UPDATE									0
#define START_TIME_MAIN										10
#define START_TIME_CONTROL_CLIENT					30
#define START_TIME_CLIENT									60
void tasks_new(void);//创建系统需要的线程

void restartThread(threadType type);

#endif
