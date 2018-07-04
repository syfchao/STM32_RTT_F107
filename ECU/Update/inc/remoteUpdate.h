#ifndef __REMOTE_UPDATE_H__
#define __REMOTE_UPDATE_H__
/*****************************************************************************/
/* File      : remoteUpdate.h                                                */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-11 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
typedef enum 
{
    EN_UPDATE_VERSION  = 0,
    EN_UPDATE_ID      = 1,
    EN_UPDATE_UNKNOWN      = 2,
} eUpdateRequest; // 升级请求

typedef enum 
{
    EN_INVERTER_YC600      	= 0,
    EN_INVERTER_YC1000      	= 1,
    EN_INVERTER_QS1200      	= 2,
    EN_UPDATE_TXT			= 3,
    EN_INVERTER_UNKNOWN     = 4,
} eDownloadType; // 下载逆变器类型

typedef enum 
{
    EN_WIRED  = 0,
    EN_WIFI      = 1,
} eNetworkType; // 网络类型


int updateECU_Local(eUpdateRequest updateRequest,char *Domain,char *IP,int port,char *User,char *passwd);
void remote_update_thread_entry(void* parameter);
#endif /*__REMOTE_UPDATE_H__*/
