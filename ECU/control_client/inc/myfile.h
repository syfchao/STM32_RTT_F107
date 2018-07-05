#ifndef __MYFILE_H__
#define __MYFILE_H__
/*****************************************************************************/
/* File      : myfile.h                                                      */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-29 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "variation.h"

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/

char *file_get_one(char *s, int count, const char *filename);
int file_set_one(const char *s, const char *filename);
int clear_file(char *filename);
int delete_line(char* filename,char* temfilename,char* compareData,int len);
int get_num_from_id(char inverter_ids[MAXINVERTERCOUNT][13]);
int insert_line(char * filename,char *str);
int search_line(char* filename,char* compareData,int len);
int get_protection_from_file(const char pro_name[][32],float *pro_value,int *pro_flag,int num);
int read_line(char* filename,char *linedata,char* compareData,int len);
int read_line_end(char* filename,char *linedata,char* compareData,int len);
#endif	/*__MYFILE_H__*/
