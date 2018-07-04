/*****************************************************************************/
/*  File      : channel.c                                                    */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "variation.h"
#include <dfs_posix.h> 
#include "channel.h"
#include "zigbee.h"
#include "file.h"
#include "InternalFlash.h"

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern ecu_info ecu;
extern inverter_info inverter[MAXINVERTERCOUNT];
extern unsigned char rateOfProgress;


/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
/* �ŵ����� */
int process_channel()
{
    int oldChannel, newChannel;

    if (channel_need_change()) {

        //��ȡ���ǰ����ŵ�
        oldChannel = getOldChannel();
        newChannel = getNewChannel();

        //�޸��ŵ�
        changeChannelOfInverters(oldChannel, newChannel);

        //��ECU�����ŵ����������ļ�
        saveECUChannel(newChannel);

        //��ձ�־λ
        unlink("/tmp/changech.con");
        unlink("/tmp/old_chan.con");
        unlink("/tmp/new_chan.con");
    }
    return 0;
}

/* �ж��Ƿ���Ҫ�ı��ŵ� */
int channel_need_change()
{
    FILE *fp;
    char buff[2] = {'\0'};

    fp = fopen("/tmp/changech.con", "r");
    if (fp) {
        fgets(buff, 2, fp);
        fclose(fp);
    }

    return ('1' == buff[0]);
}

int saveChannel_change_flag()
{
    echo("/tmp/changech.con","1");
    echo("/yuneng/limiteid.con","1");
    return 0;
}


// ��ȡ�ŵ�����Χ��11~26��16���ŵ�
int getOldChannel()
{
    FILE *fp;
    char buffer[4] = {'\0'};

    fp = fopen("/tmp/old_chan.con", "r");
    if (fp) {
        fgets(buffer, 4, fp);
        fclose(fp);
        return atoi(buffer);
    }
    return 0; //δ֪�ŵ�
}
int getNewChannel()
{
    FILE *fp;
    char buffer[4] = {'\0'};

    fp = fopen("/tmp/new_chan.con", "r");
    if (fp) {
        fgets(buffer, 4, fp);
        fclose(fp);
        return atoi(buffer);
    }
    return 16; //Ĭ���ŵ�
}


// ��ȡ�ŵ�����Χ��11~26��16���ŵ�
int saveOldChannel(unsigned char oldChannel)
{
    FILE *fp;
    char buffer[3] = {'\0'};

    fp = fopen("/tmp/old_chan.con", "w");
    if (fp) {
        sprintf(buffer,"%d",oldChannel);
        fputs(buffer, fp);
        fclose(fp);
    }
    return 0;
}
int saveNewChannel(unsigned char newChannel)
{
    FILE *fp;
    char buffer[3] = {'\0'};

    fp = fopen("/tmp/new_chan.con", "w");
    if (fp) {
        sprintf(buffer,"%d",newChannel);
        fputs(buffer, fp);
        fclose(fp);
    }
    return 0;
}



int saveECUChannel(int channel)
{
    char buffer[5] = {'\0'};

    snprintf(buffer, sizeof(buffer), "0x%02X", channel);
    WritePage(INTERNAL_FLASH_CHANNEL,buffer,5);
    echo("/yuneng/limiteid.con","1");

    ecu.channel = channel;
    return 1;

}


void changeChannelOfInverters(int oldChannel, int newChannel)
{
    int num = 0,i = 0,nChannel;
    inverter_info *curinverter = inverter;

    //��ȡ�����ID
    for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)			//��Ч�������ѵ
    {
        curinverter->inverterstatus.flag = 1;
        curinverter->inverterstatus.bindflag= 0;
        num++;
    }

    //�����ŵ�
    if (num > 0) {
        //ԭ�ŵ���֪
        if (oldChannel) {
            //����ECU�ŵ�Ϊԭ�ŵ�
            zb_restore_ecu_panid_0xffff(oldChannel);
            rateOfProgress = 5;
            //����ÿ̨������ŵ�
            curinverter = inverter;
            for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)			//��Ч�������ѵ
            {
                if(curinverter->inverterstatus.flag == 1)
                {
                    rateOfProgress = 5+35*(i+1)/num;
                    zb_change_inverter_channel_one(curinverter->id, newChannel);
                }

            }
        }
        //ԭ�ŵ�δ֪
        else {
            //ECU��ÿһ���ŵ�����ÿһ̨��������͸����ŵ���ָ��
            for (nChannel=11; nChannel<=26; nChannel++ ) {
                zb_restore_ecu_panid_0xffff(nChannel);
                curinverter = inverter;
                for(i=0; (i<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); i++, curinverter++)			//��Ч�������ѵ
                {
                    if(curinverter->inverterstatus.flag == 1)
                    {
                        zb_change_inverter_channel_one(curinverter->id, newChannel);
                    }
                }
                rateOfProgress = 40*(nChannel-10)/16;
            }
        }
    }


}
