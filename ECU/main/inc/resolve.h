#ifndef __RESOLVE_H__
#define __RESOLVE_H__
/*****************************************************************************/
/*  File      : resolve.h                                                    */
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
#include "variation.h"
#include "debug.h"
#include "checkdata.h"

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
int resolvedata_1000(char *data, struct inverter_info_t *inverter);
int resolvedata_600(char *data, struct inverter_info_t *inverter);
int resolvedata_600_new(char *data, struct inverter_info_t *inverter);

int yc600_status(struct inverter_info_t *inverter);

int yc1000_status(struct inverter_info_t *inverter);


#endif /*__RESOLVE_H__*/
