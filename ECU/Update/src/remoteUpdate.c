/*****************************************************************************/
/* File      : remoteUpdate.c                                                */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-11 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "remoteUpdate.h"
#include <rtthread.h>
#include "thftpapi.h"
#include "flash_if.h"
#include "dfs_posix.h"
#include "rthw.h"
#include "myfile.h"
#include "version.h"
#include "threadlist.h"
#include "debug.h"
#include "file.h"
#include "datetime.h"
#include "WiFiFileAPI.h"
#include "usart5.h"
#include "InternalFlash.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
#define UPDATE_PATH_SUFFIX "ecu-r-m3.bin"
#define UPDATE_PATH "/FTP/ecu.bin"

#define UPDATE_PATH_YC600_SUFFIX "YC600.BIN"
#define UPDATE_PATH_YC600_TEMP "/FTP/YC600UP.BIN"
#define UPDATE_PATH_YC600 "/ftp/UPYC600.BIN"

#define UPDATE_PATH_YC1000_SUFFIX "YC1000.BIN"
#define UPDATE_PATH_YC1000_TEMP "/FTP/YC1000UP.BIN"
#define UPDATE_PATH_YC1000 "/ftp/UPYC1000.BIN"

#define UPDATE_PATH_QS1200_SUFFIX "QS1200.BIN"
#define UPDATE_PATH_QS1200_TEMP "/FTP/QS1200.BIN"
#define UPDATE_PATH_QS1200 "/ftp/UPQS1200.BIN"

#define UPDATE_PATH_UPDATE_TXT_SUFFIX "update.txt"
#define UPDATE_PATH_UPDATE_TXT_TEMP "/FTP/update.1"
#define UPDATE_PATH_UPDATE_TXT "/ftp/update.txt"


/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern ecu_info ecu;
extern Socket_Cfg ftp_arg;

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
//IDWrite 本地升级ECU  
//返回0表示成功
int updateECU_Local(eUpdateRequest updateRequest,char *Domain,char *IP,int port,char *User,char *passwd)
{
    int ret = 0;
    char remote_path[100] = {'\0'};

    print2msg(ECU_DBG_UPDATE,"Domain",Domain);
    print2msg(ECU_DBG_UPDATE,"FTPIP",IP);
    printdecmsg(ECU_DBG_UPDATE,"port",port);
    print2msg(ECU_DBG_UPDATE,"user",User);
    print2msg(ECU_DBG_UPDATE,"password",passwd);

    if(updateRequest == EN_UPDATE_VERSION)
    {
        sprintf(remote_path,"/ECU_R_M3/V%s.%s/%s",MAJORVERSION,MINORVERSION,UPDATE_PATH_SUFFIX);
        print2msg(ECU_DBG_UPDATE,"VER Path",remote_path);
    }else if(updateRequest == EN_UPDATE_ID)
    {
        sprintf(remote_path,"/ECU_R_M3/%s/%s",ecu.id,UPDATE_PATH_SUFFIX);
        print2msg(ECU_DBG_UPDATE,"ID Path",remote_path);
    }else
    {
	return ret;
    }
    
    ret=ftpgetfile_InternalFlash(Domain,IP,port,User, passwd,remote_path,UPDATE_PATH);
    if(!ret)
    {
        //获取到文件，进行更新
        if(updateRequest == EN_UPDATE_ID)
        {
        	    deletefile(remote_path);
        	}
        UpdateFlag();
    }
    return ret;
}

int updateECU(eNetworkType networkType,eUpdateRequest updateRequest)
{
    int ret = 0;
    char remote_path[100] = {'\0'};

    //获取服务器IP地址
    if(updateRequest == EN_UPDATE_VERSION)
    {
        sprintf(remote_path,"/ECU_R_M3/V%s.%s/%s",MAJORVERSION,MINORVERSION,UPDATE_PATH_SUFFIX);
        print2msg(ECU_DBG_UPDATE,"VER Path",remote_path);
    }else if(updateRequest == EN_UPDATE_ID)
    {
        sprintf(remote_path,"/ECU_R_M3/%s/%s",ecu.id,UPDATE_PATH_SUFFIX);
        print2msg(ECU_DBG_UPDATE,"ID Path",remote_path);
    }else
    {
        return ret;
    }

    if(networkType == EN_WIRED)
    {
        ret=ftpgetfile_InternalFlash(ftp_arg.domain,ftp_arg.ip, ftp_arg.port1, ftp_arg.user, ftp_arg.passwd,remote_path,UPDATE_PATH);
    }else if(networkType == EN_WIFI)
    {
       ret=WiFiFile_GetFileInternalFlash(remote_path);
       printdecmsg(ECU_DBG_UPDATE,"ret",ret);
    }else
    {
    	return ret;
    }
    
    if(!ret)
    {
        //获取到文件，进行更新
        UpdateFlag();
        if(updateRequest == EN_UPDATE_ID)
        	{
        	    if(networkType == EN_WIRED)
        	    {
        	        deletefile(remote_path);
        	    }else
        	    {
        	        WiFiFile_DeleteFile(remote_path);
        	    }
        	}
        WritePage(INTERNAL_FLASH_ECUUPVER,"1",1);	
        reboot();
    }
    return ret;
}

int DownloadInverterFile(eNetworkType networkType,eUpdateRequest updateRequest,eDownloadType downloadType)	//获取逆变器升级包
{
    int ret = 0;
    char remote_path[100] = {'\0'};
    char savePath_temp[60] = {"\0"};
    char savePath[60] = {"\0"};

    //获取服务器IP地址
    if(updateRequest == EN_UPDATE_VERSION)
    {
        sprintf(remote_path,"/ECU_R_M3/V%s.%s/",MAJORVERSION,MINORVERSION);
    }else if(updateRequest == EN_UPDATE_ID)
    {
        sprintf(remote_path,"/ECU_R_M3/%s/",ecu.id);
    }else
    {
    	return ret;
    }

    if(downloadType == EN_INVERTER_YC600)
    {
        sprintf(remote_path,"%s%s",remote_path,UPDATE_PATH_YC600_SUFFIX);
        sprintf(savePath_temp,"%s",UPDATE_PATH_YC600_TEMP);
        sprintf(savePath,"%s",UPDATE_PATH_YC600);
    }else if(downloadType == EN_INVERTER_YC1000)
    {
        sprintf(remote_path,"%s%s",remote_path,UPDATE_PATH_YC1000_SUFFIX);
        sprintf(savePath_temp,"%s",UPDATE_PATH_YC1000_TEMP);
        sprintf(savePath,"%s",UPDATE_PATH_YC1000);
    }else if(downloadType == EN_INVERTER_QS1200)
    {
        sprintf(remote_path,"%s%s",remote_path,UPDATE_PATH_QS1200_SUFFIX);
        sprintf(savePath_temp,"%s",UPDATE_PATH_QS1200_TEMP);
        sprintf(savePath,"%s",UPDATE_PATH_QS1200);
    }else if(downloadType == EN_UPDATE_TXT)
    {
        sprintf(remote_path,"%s%s",remote_path,UPDATE_PATH_UPDATE_TXT_SUFFIX);
        sprintf(savePath_temp,"%s",UPDATE_PATH_UPDATE_TXT_TEMP);
        sprintf(savePath,"%s",UPDATE_PATH_UPDATE_TXT);
    }else
    {
    	return ret;
    }
	
    print2msg(ECU_DBG_UPDATE,"Path",remote_path);
    if(networkType == EN_WIRED)	//有线
    {
    	ret=ftpgetfile(ftp_arg.domain,ftp_arg.ip, ftp_arg.port1, ftp_arg.user, ftp_arg.passwd,remote_path,savePath_temp);
    }else if(networkType == EN_WIFI)	//无线
    {
    	ret=WiFiFile_GetFile(remote_path,savePath_temp);
	printdecmsg(ECU_DBG_UPDATE,"ret",ret);
    }else
    {
    	return ret;
    }
    
    if(!ret)
    {
        unlink(savePath);
        rename(savePath_temp,savePath);
	if(updateRequest == EN_UPDATE_ID)
	{
	    if(networkType == EN_WIRED)
        	    {
        	        deletefile(remote_path);
        	    }else
        	    {
        	        WiFiFile_DeleteFile(remote_path);
        	    }
	}
        
    }
    return ret;
}
void remote_update_thread_entry(void* parameter)
{
    int i = 0;
    rt_thread_delay(RT_TICK_PER_SECOND * START_TIME_UPDATE);

    while(1)
    {
    	//有线下载
        for(i = 0;i<5;i++)
        {
            if(-1 != updateECU(EN_WIRED,EN_UPDATE_VERSION))
                break;
        }
        for(i = 0;i<5;i++)
        {
            if(-1 != updateECU(EN_WIRED,EN_UPDATE_ID))
                break;
        }
        //根据当前的逆变器版本，下载对应逆变器升级包
       /* 
        for(i = 0;i<2;i++)
        {
            if(-1 != DownloadInverterFile())
                break;
        }
        for(i = 0;i<2;i++)
        {
            if(-1 != DownloadInverterFile())
                break;
        }
        for(i = 0;i<2;i++)
        {
            if(-1 != DownloadInverterFile())
                break;
        }
	*/
	rt_thread_delay(RT_TICK_PER_SECOND*20);
	//无线下载
	//无线按照版本升级
	for(i = 0;i<2;i++)
        {
            if(-1 != updateECU(EN_WIFI,EN_UPDATE_VERSION))
                break;
	    rt_thread_delay(RT_TICK_PER_SECOND*10);
        }
	//无线按照ID升级
	for(i = 0;i<2;i++)
        {
            if(-1 != updateECU(EN_WIFI,EN_UPDATE_ID))
                break;
	   rt_thread_delay(RT_TICK_PER_SECOND*10);
        }

	//无线下载600程序
	//无线下载1000程序
	//无线下载1200程序

	
        rt_thread_delay(RT_TICK_PER_SECOND*86400);
    }

}
