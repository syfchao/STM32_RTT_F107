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
int updateECUByVersion_Local(char *Domain,char *IP,int port,char *User,char *passwd);
int updateECUByID_Local(char *Domain,char *IP,int port,char *User,char *passwd);
void remote_update_thread_entry(void* parameter);
#endif /*__REMOTE_UPDATE_H__*/
