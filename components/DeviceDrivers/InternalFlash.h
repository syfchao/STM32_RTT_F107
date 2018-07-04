/*****************************************************************************/
/* File      : InternalFlash.h                                               */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2018-05-24 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#ifndef __INTERNALFLASH_H__
#define __INTERNALFLASH_H__


/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
typedef enum 
{
	INTERNAL_FALSH_Update = 0,
	INTERNAL_FALSH_ID = 1,
	INTERNAL_FALSH_MAC = 2,
	INTERNAL_FALSH_AREA = 3,		//区域NA SAA MX
	INTERNAL_FLASH_CHANNEL = 4,		//信道
	INTERNAL_FLASH_WIFI_PASSWORD = 5,//WiFi密码	
	INTERNAL_FLASH_INVERTER_ID = 6,	//逆变器ID
	INTERNAL_FLASH_CONFIG_INFO = 7,	//服务器配置信息
} eInternalFlashType;

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
int ErasePage(eInternalFlashType type);
int WritePage(eInternalFlashType type,char *Data,int Length);
int ReadPage(eInternalFlashType type,char *Data,int Length);
void detectionInternalFlash(char *ID,unsigned char *Mac);
#endif /*__INTERNALFLASH_H__*/

