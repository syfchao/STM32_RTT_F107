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
	INTERNAL_FLASH_A118 = 8,			//上报逆变器信息
	INTERNAL_FLASH_ECU_FLAG = 9,		//ECU通讯开关
	INTERNAL_FLASH_LIMITEID = 10,		//是否自动组网
	INTERNAL_FLASH_CHANGECHANNEL = 11,	//更改信道
	INTERNAL_FLASH_ECUUPVER= 12,		//ecu更新上报标志
	INTERNAL_FLASH_IPCONFIG = 13,		//ip配置 0:动态分配 1静态IP  IP MSK GW DNS1 DNS2 各4个字节
	INTERNAL_FLASH_14 = 14,
	INTERNAL_FLASH_15 = 15,
	INTERNAL_FLASH_16 = 16,
	INTERNAL_FLASH_MAX = 17,
} eInternalFlashType;

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
int ErasePage(eInternalFlashType type);
int WritePage(eInternalFlashType type,char *Data,int Length);
int ReadPage(eInternalFlashType type,char *Data,int Length);
void detectionInternalFlash(char *ID,unsigned char *Mac);
#endif /*__INTERNALFLASH_H__*/

