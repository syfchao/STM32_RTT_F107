#include "wifi_comm.h"
#include "string.h"
#include "variation.h"
#include "stdio.h"
#include "version.h"
#include "file.h"
#include "rtc.h"
#include "usart5.h"
#include "stdlib.h"
#include "threadlist.h"
#include "dfs_posix.h"
#include "InternalFlash.h"


extern ecu_info ecu;
static char SendData[MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER] = {'\0'};
extern unsigned char rateOfProgress;
extern unsigned char processAllFlag;

int phone_add_inverter(int num,const char *uidstring)
{
    int i = 0;
    char buff[25] = { '\0' };
    char *allbuff = NULL;
    allbuff = malloc(2500);
    memset(allbuff,0x00,2500);
    for(i = 0; i < num; i++)
    {
        memset(buff,'\0',25);
        sprintf(buff,"%02x%02x%02x%02x%02x%02x,,,,,,\n",uidstring[0+i*6],uidstring[1+i*6],uidstring[2+i*6],uidstring[3+i*6],uidstring[4+i*6],uidstring[5+i*6]);
        memcpy(&allbuff[0+19*i],buff,19);
    }


    echo("/home/data/id",allbuff);
    WritePage(INTERNAL_FLASH_LIMITEID,"1",1);
    free(allbuff);
    allbuff = NULL;
    return 0;
}

//解析报文长度
unsigned short packetlen(unsigned char *packet)
{
    unsigned short len = 0;
    len = ((packet[0]-'0')*1000 +(packet[1]-'0')*100 + (packet[2]-'0')*10 + (packet[3]-'0'));
    return len;
}

//解析收到的数据
int Resolve_RecvData(char *RecvData,int* Data_Len,int* Command_Id)
{
    //APS
    if(strncmp(RecvData, "APS", 3))
        return -1;
    //版本号
    //长度
    *Data_Len = packetlen((unsigned char *)&RecvData[5]);
    //ID
    *Command_Id = (RecvData[9]-'0')*1000 + (RecvData[10]-'0')*100 + (RecvData[11]-'0')*10 + (RecvData[12]-'0');
    return 0;
}

int Resolve_Server(ECUServerInfo_t *serverInfo)
{
    //如果serverCmdType 为获取指令，解析对应文件到结构体中
    char IP_Str[20] = {'\0'};
    char temp[4] = {'\0'};
    unsigned char IP_Str_site = 0;
    unsigned char site = 0;
    int i = 0;

    if(serverInfo->serverCmdType == SERVER_UPDATE_GET)
    {
        memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
        memcpy(serverInfo->domain,ftp_arg.domain,32);
        strcpy(IP_Str, ftp_arg.ip);
        if('\n' == IP_Str[strlen(IP_Str)-1])
            IP_Str[strlen(IP_Str)-1] = '\0';
        //解析到serverInfo中去
        for(i = 0;i<(strlen(IP_Str)+1);i++)
        {
            if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
            {
                memset(temp,'\0',4);
                memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
                serverInfo->IP[site] = atoi(temp);
                IP_Str_site = i+1;
                site++;
            }
        }
        serverInfo->Port1 = ftp_arg.port1;
        serverInfo->Port2 = 0;
    }else if(serverInfo->serverCmdType == SERVER_CLIENT_GET)
    {
        memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
        memcpy(serverInfo->domain,client_arg.domain,32);
        strcpy(IP_Str, client_arg.ip);
        if('\n' == IP_Str[strlen(IP_Str)-1])
            IP_Str[strlen(IP_Str)-1] = '\0';
        //解析到serverInfo中去
        for(i = 0;i<(strlen(IP_Str)+1);i++)
        {
            if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
            {
                memset(temp,'\0',4);
                memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
                serverInfo->IP[site] = atoi(temp);
                IP_Str_site = i+1;
                site++;
            }
        }
        serverInfo->Port1 = client_arg.port1;
        serverInfo->Port2 = client_arg.port2;

    }else if(serverInfo->serverCmdType == SERVER_CONTROL_GET)
    {

        memset(serverInfo->domain,'\0',sizeof(serverInfo->domain)/sizeof(serverInfo->domain[0]));
        memcpy(serverInfo->domain,control_client_arg.domain,32);
        strcpy(IP_Str, control_client_arg.ip);
        if('\n' == IP_Str[strlen(IP_Str)-1])
            IP_Str[strlen(IP_Str)-1] = '\0';
        //解析到serverInfo中去
        for(i = 0;i<(strlen(IP_Str)+1);i++)
        {
            if((IP_Str[i] =='.') ||(IP_Str[i] =='\0'))
            {
                memset(temp,'\0',4);
                memcpy(temp,&IP_Str[IP_Str_site],i-IP_Str_site);
                serverInfo->IP[site] = atoi(temp);
                IP_Str_site = i+1;
                site++;
            }
        }
        serverInfo->Port1 = control_client_arg.port1;
        serverInfo->Port2 = control_client_arg.port2;
    }

    return 0;

}

int Save_Server(ECUServerInfo_t *serverInfo)
{	
    char *buff = NULL;
    buff = malloc(512);
    memset(buff,'\0',512);
    printf("%d\n",serverInfo->serverCmdType);
    printf("%s\n",serverInfo->domain);
    printf("%d,%d,%d,%d,\n",serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3]);
    printf("%d\n",serverInfo->Port1);
    printf("%d\n",serverInfo->Port2);
    if(serverInfo->serverCmdType == SERVER_UPDATE_SET)
    {
        memcpy(ftp_arg.domain,serverInfo->domain,32);
        sprintf(ftp_arg.ip,"%d.%d.%d.%d",serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3]);
        ftp_arg.port1 = serverInfo->Port1;
        memcpy(ftp_arg.user,"zhyf",4);
        memcpy(ftp_arg.passwd,"yuneng",6);
        UpdateServerInfo();
    }else if(serverInfo->serverCmdType == SERVER_CLIENT_SET)
    {
        memcpy(client_arg.domain,serverInfo->domain,32);
        sprintf(client_arg.ip,"%d.%d.%d.%d",serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3]);
        client_arg.port1 = serverInfo->Port1;
        client_arg.port2 = serverInfo->Port2;
        UpdateServerInfo();
    }else if(serverInfo->serverCmdType == SERVER_CONTROL_SET)
    {
        memcpy(control_client_arg.domain,serverInfo->domain,32);
        sprintf(control_client_arg.ip,"%d.%d.%d.%d",serverInfo->IP[0],serverInfo->IP[1],serverInfo->IP[2],serverInfo->IP[3]);
        control_client_arg.port1 = serverInfo->Port1;
        control_client_arg.port2 = serverInfo->Port2;
        UpdateServerInfo();
    }
    free(buff);
    buff = NULL;

    return 0;
}

void phone_turnOnOff_single(const char *recvbuffer,int length)
{
    int num = 0,index = 0;
    char buff[20] = {'\0'};
    int fd = 0;
    fd = open("/home/data/turnonof", O_WRONLY | O_TRUNC| O_CREAT,0);
    num = (length - 33)/7;
    if(fd >= 0)
    {
        for(index = 0;index < num;index++)
        {
            sprintf(buff,"%02x%02x%02x%02x%02x%02x,%c\n",recvbuffer[0+index*7],recvbuffer[1+index*7],recvbuffer[2+index*7],recvbuffer[3+index*7],recvbuffer[4+index*7],recvbuffer[5+index*7],(recvbuffer[6+index*7]+1));
            printf("%s\n",buff);
            write(fd,buff,strlen(buff));
        }
        close(fd);
    }


}


void phone_GFDI_single(const char *recvbuffer,int length)
{
    int num = 0,index = 0;
    char buff[20] = {'\0'};
    int fd = 0;
    fd = open("/home/data/clrgfdi", O_WRONLY | O_TRUNC| O_CREAT,0);
    num = (length - 33)/7;
    if(fd >= 0)
    {
        for(index = 0;index < num;index++)
        {
            sprintf(buff,"%02x%02x%02x%02x%02x%02x,%c\n",recvbuffer[0+index*7],recvbuffer[1+index*7],recvbuffer[2+index*7],recvbuffer[3+index*7],recvbuffer[4+index*7],recvbuffer[5+index*7],recvbuffer[6+index*7]);
            printf("%s\n",buff);
            write(fd,buff,strlen(buff));
        }
        close(fd);
    }


}


void phone_MaxPower_ALL(inverter_info *inverter,unsigned short maxPower)
{
    int index = 0;
    char buff[50] = {'\0'};
    inverter_info *curinverter = inverter;
    int fd = 0;
    fd = open("/home/data/power", O_WRONLY | O_TRUNC| O_CREAT,0);

    if(fd >= 0)
    {
        for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
        {
            sprintf(buff,"%s,%3d,,,,1\n",curinverter->id,maxPower);
            printf("%s\n",buff);
            write(fd,buff,strlen(buff));
        }
        close(fd);
    }


}

void phone_MaxPower_single(const char *recvbuffer,int length)
{
    int num = 0,index = 0;
    char buff[50] = {'\0'};
    int fd = 0;
    fd = open("/home/data/power", O_WRONLY | O_TRUNC| O_CREAT,0);
    num = (length - 33)/8;
    if(fd >= 0)
    {
        for(index = 0;index < num;index++)
        {
            sprintf(buff,"%02x%02x%02x%02x%02x%02x,%03d,,,,1\n",recvbuffer[0+index*8],recvbuffer[1+index*8],recvbuffer[2+index*8],recvbuffer[3+index*8],recvbuffer[4+index*8],recvbuffer[5+index*8],(recvbuffer[6+index*8]*256+recvbuffer[7+index*8]));
            printf("%s\n",buff);
            write(fd,buff,strlen(buff));
        }
        close(fd);
    }


}

void phone_getMaxPower(char *buff,int* length)
{
    FILE *fp;
    char data[200];
    char splitdata[6][32];
    char inv_buff[11] = { '\0' };
    int index = 0,limitedpower = 0,limitedresult = 0;
    *length = 0;
    fp = fopen("/home/data/power", "r");
    if(fp)
    {
        memset(data,0x00,200);

        while(NULL != fgets(data,200,fp))
        {
            memset(splitdata,0x00,6*32);
            splitString(data,splitdata);
            limitedpower = atoi(splitdata[1]);
            limitedresult = atoi(splitdata[2]);
            inv_buff[0] = ((splitdata[0][0] - '0') << 4) + (splitdata[0][1] - '0');
            inv_buff[1] = ((splitdata[0][2] - '0') << 4) + (splitdata[0][3] - '0');
            inv_buff[2] = ((splitdata[0][4] - '0') << 4) + (splitdata[0][5] - '0');
            inv_buff[3] = ((splitdata[0][6] - '0') << 4) + (splitdata[0][7] - '0');
            inv_buff[4] = ((splitdata[0][8] - '0') << 4) + (splitdata[0][9] - '0');
            inv_buff[5] = ((splitdata[0][10] - '0') << 4) + (splitdata[0][11] - '0');

            if(0 != limitedpower)
            {
                inv_buff[6] = limitedpower/256;
                inv_buff[7] = limitedpower%256;
            }else
            {
                inv_buff[6] = 0xff;
                inv_buff[7] = 0xff;
            }

            if(0 != limitedresult)
            {
                inv_buff[8] = limitedresult/256;
                inv_buff[9] = limitedresult%256;
            }else
            {
                inv_buff[8] = 0xff;
                inv_buff[9] = 0xff;
            }

            memcpy(&buff[index*10],inv_buff,10);
            *length+=10;
            index++;

        }
        fclose(fp);
    }


}

void phone_IRD_ALL(unsigned char ird)
{
    char buff[50] = {'\0'};
    sprintf(buff,"ALL,%d\n",ird);
    echo("/tmp/set_ird.con",buff);

}

void phone_IRD_single(const char *recvbuffer,int length)
{
    int num = 0,index = 0;
    char buff[50] = {'\0'};
    int fd = 0;
    fd = open("/home/data/ird", O_WRONLY | O_TRUNC| O_CREAT,0);
    num = (length - 33)/7;
    if(fd >= 0)
    {
        for(index = 0;index < num;index++)
        {
            sprintf(buff,"%02x%02x%02x%02x%02x%02x,,%d,1\n",recvbuffer[0+index*7],recvbuffer[1+index*7],recvbuffer[2+index*7],recvbuffer[3+index*7],recvbuffer[4+index*7],recvbuffer[5+index*7],recvbuffer[6+index*7]);
            printf("%s\n",buff);
            write(fd,buff,strlen(buff));
        }
        close(fd);
    }


}
void phone_getIRD(char *buff,int* length)
{
    FILE *fp;
    char data[200];
    char splitdata[4][32];
    char inv_buff[11] = { '\0' };
    int index = 0,IRD = 0,IRDresult = 0;
    *length = 0;
    fp = fopen("/home/data/ird", "r");
    if(fp)
    {
        memset(data,0x00,200);

        while(NULL != fgets(data,200,fp))
        {
            memset(splitdata,0x00,4*32);
            splitString(data,splitdata);
            IRDresult = atoi(splitdata[1]);
            IRD = atoi(splitdata[2]);
            inv_buff[0] = ((splitdata[0][0] - '0') << 4) + (splitdata[0][1] - '0');
            inv_buff[1] = ((splitdata[0][2] - '0') << 4) + (splitdata[0][3] - '0');
            inv_buff[2] = ((splitdata[0][4] - '0') << 4) + (splitdata[0][5] - '0');
            inv_buff[3] = ((splitdata[0][6] - '0') << 4) + (splitdata[0][7] - '0');
            inv_buff[4] = ((splitdata[0][8] - '0') << 4) + (splitdata[0][9] - '0');
            inv_buff[5] = ((splitdata[0][10] - '0') << 4) + (splitdata[0][11] - '0');

            if(0 != IRD)
            {
                inv_buff[6] = IRD;
            }else
            {
                inv_buff[6] = 0xff;
            }

            if(0 != IRDresult)
            {
                inv_buff[7] = IRDresult;
            }else
            {
                inv_buff[7] = 0xff;
            }

            memcpy(&buff[index*8],inv_buff,8);
            *length+=8;
            index++;

        }
        fclose(fp);
    }


}

int resolve_Event(char *data,char *event_buff)
{
    char UID[13] = {'\0'};
    char Event[64] = {'\0'};
    char Time[7] = {'\0'};
    int len = 0,i = 0;
    len  =strlen(data);

    if((data[12] == ',')&&(data[len-8] == ','))
    {
        memcpy(UID,data,12);
        memcpy(Event,&data[13],(len-21));
        memcpy(Time,&data[len-7],6);
        event_buff[0] = ((UID[0]-'0') << 4) + (UID[1]-'0');
        event_buff[1] = ((UID[2]-'0') << 4) + (UID[3]-'0');
        event_buff[2] = ((UID[4]-'0') << 4) + (UID[5]-'0');
        event_buff[3] = ((UID[6]-'0') << 4) + (UID[7]-'0');
        event_buff[4] = ((UID[8]-'0') << 4) + (UID[9]-'0');
        event_buff[5] = ((UID[10]-'0') << 4) + (UID[11]-'0');

        event_buff[6] = ((Time[0]-'0') << 4) + (Time[1]-'0');
        event_buff[7] = ((Time[2]-'0') << 4) + (Time[3]-'0');
        event_buff[8] = ((Time[4]-'0') << 4) + (Time[5]-'0');

        for(i  = 0;i<(len-21);i++)
        {
            event_buff[9+(i/8)] |= (Event[i]-'0') <<(7-i%8);
        }

        //printf("%s   %s   %s\n",UID,Event,Time);
        return 0;
    }else
    {
        return -1;
    }

}

void phone_getEvent(char *buff,int* length,const char *Date,const char *serial)
{
    FILE *fp;
    char path[100] = {'\0'};
    char data[200];
    char event_buff[18] = {0x00};
    int total = 0,serial_no=0,index = 0,temp = 0;	//获取一共有多少条数据

    *length = 0;
    serial_no = atoi(serial);
    sprintf(path,"/home/record/eventdir/%s.dat",Date);

    fp = fopen(path, "r");
    if(fp)
    {
        while(NULL != fgets(data,200,fp))
        {
            total++;
        }
        fclose(fp);
        fp = NULL;
    }
    printf("alarm event total:%d\n",total);

    if(total >= (1 + serial_no*200))	//读取到的条数大于要求序列号的长度
    {
        fp = fopen(path, "r");
        if(fp)
        {
            temp = total  - serial_no * 200;
            if(temp <=200)		//剩余条数小于等于200条
            {
                //直接读Temp条数据
                for(index = 0;index < temp;index++)
                {
                    if(NULL != fgets(data,200,fp))
                    {
                        memset(event_buff,0x00,18);
                        if(0 == resolve_Event(data,event_buff))
                        {
                            memcpy(&buff[*length],event_buff,17);
                            *length+=17;
                        }

                    }else
                    {
                        break;
                    }


                }
            }else				//剩余条数大于200条
            {
                //先跳过temp-200条数据   在读200条
                for(index = 0;index < (temp-200);index++)
                    fgets(data,200,fp);
                //读取200条数据，并解析
                for(index = 0;index < 200;index++)
                {
                    if(NULL != fgets(data,200,fp))
                    {
                        memset(event_buff,0x00,18);
                        if(0 == resolve_Event(data,event_buff))
                        {
                            memcpy(&buff[*length],event_buff,17);
                            *length+=17;
                        }

                    }else
                    {
                        break;
                    }
                }
            }
            fclose(fp);
            fp = NULL;
        }
    }

}

//01 COMMAND_BASEINFO 					//获取基本信息请求
void APP_Response_BaseInfo(stBaseInfo baseInfo)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    //拼接需要发送的报文
    sprintf(SendData,"APS1100000001%s",baseInfo.ECUID);
    packlength = 25;

    SendData[packlength++] = '0';
    SendData[packlength++] = '1';

    SendData[packlength++] = (baseInfo.LifttimeEnergy/16777216)%256;
    SendData[packlength++] = (baseInfo.LifttimeEnergy/65536)%256;
    SendData[packlength++] = (baseInfo.LifttimeEnergy/256)%256;
    SendData[packlength++] =  baseInfo.LifttimeEnergy%256;

    SendData[packlength++] = (baseInfo.LastSystemPower/16777216)%256;
    SendData[packlength++] = (baseInfo.LastSystemPower/65536)%256;
    SendData[packlength++] = (baseInfo.LastSystemPower/256)%256;
    SendData[packlength++] = baseInfo.LastSystemPower%256;

    SendData[packlength++] = (baseInfo.GenerationCurrentDay/16777216)%256;
    SendData[packlength++] = (baseInfo.GenerationCurrentDay/65536)%256;
    SendData[packlength++] = (baseInfo.GenerationCurrentDay/256)%256;
    SendData[packlength++] = baseInfo.GenerationCurrentDay%256;


    memcpy(&SendData[packlength],baseInfo.LastToEMA,7);
    packlength += 7;

    SendData[packlength++] = baseInfo.InvertersNum/256;
    SendData[packlength++] = baseInfo.InvertersNum%256;

    SendData[packlength++] = baseInfo.LastInvertersNum/256;
    SendData[packlength++] = baseInfo.LastInvertersNum%256;

    SendData[packlength++] = baseInfo.Channel[0];
    SendData[packlength++] = baseInfo.Channel[1];

    SendData[packlength++] = '0' + (ECU_VERSION_LENGTH/100)%10;
    SendData[packlength++] = '0'+ (ECU_VERSION_LENGTH/10)%10;
    SendData[packlength++] = '0'+ (ECU_VERSION_LENGTH%10);
    sprintf(&SendData[packlength],"%s_%s.%s",ECU_VERSION,MAJORVERSION,MINORVERSION);
    packlength += ECU_VERSION_LENGTH;

    SendData[packlength++] = '0';
    SendData[packlength++] = '0';
    SendData[packlength++] = '9';
    sprintf(&SendData[packlength],"Etc/GMT-8");
    packlength += 9;

    memcpy(&SendData[packlength],baseInfo.MacAddress,6);
    packlength += 6;

    memset(baseInfo.WifiMac,0x00,6);
    memcpy(&SendData[packlength],baseInfo.WifiMac,6);
    packlength += 6;

    SendData[packlength++] = 'E';
    SendData[packlength++] = 'N';
    SendData[packlength++] = 'D';
    SendData[5] = (packlength/1000) + '0';
    SendData[6] = ((packlength/100)%10) + '0';
    SendData[7] = ((packlength/10)%10) + '0';
    SendData[8] = ((packlength)%10) + '0';
    SendData[packlength++] = '\n';
    SendToSocketA(SendData ,packlength);
}

//02 COMMAND_POWERGENERATION		//逆变器发电数据请求  mapping :: 0x00 匹配  0x01 不匹配
void APP_Response_PowerGeneration(char mapping,inverter_info *inverter,int VaildNum)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //匹配不成功
    if(mapping == 0x01)
    {
        sprintf(SendData,"APS110015000201\n");
        packlength = 16;
        SendToSocketA(SendData ,packlength);
        return ;
    }

    //匹配成功
    if(ecu.had_data_broadcast_time[0] == '\0')
    {
        sprintf(SendData,"APS110015000202\n");
        packlength = 16;
        SendToSocketA(SendData ,packlength);
        return ;
    }

    //拼接需要发送的报文
    sprintf(SendData,"APS110000000200");
    packlength = 15;

    SendData[packlength++] = '0';
    SendData[packlength++] = '1';

    SendData[packlength++] = VaildNum/256;
    SendData[packlength++] = VaildNum%256;

    SendData[packlength++] = (ecu.had_data_broadcast_time[0] - '0')*16+(ecu.had_data_broadcast_time[1] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[2] - '0')*16+(ecu.had_data_broadcast_time[3] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[4] - '0')*16+(ecu.had_data_broadcast_time[5] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[6] - '0')*16+(ecu.had_data_broadcast_time[7] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[8] - '0')*16+(ecu.had_data_broadcast_time[9] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[10] - '0')*16+(ecu.had_data_broadcast_time[11] - '0');
    SendData[packlength++] = (ecu.had_data_broadcast_time[12] - '0')*16+(ecu.had_data_broadcast_time[13] - '0');

    for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
    {
        if((curinverter->model == 0x17))
        {
            //UID
            SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
            SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
            SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
            SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
            SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
            SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

            SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

            //逆变器类型
            SendData[packlength++] = '0';
            SendData[packlength++] = '3';

            //电网频率
            SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
            SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

            //机内温度
            SendData[packlength++] = (curinverter->it + 100) /256;
            SendData[packlength++] = (curinverter->it + 100) %256;

            //逆变器功率  A
            SendData[packlength++] = curinverter->op / 256;
            SendData[packlength++] = curinverter->op % 256;

            //电网电压    A
            SendData[packlength++] = curinverter->gv / 256;
            SendData[packlength++] = curinverter->gv % 256;

            //逆变器功率  B
            SendData[packlength++] = curinverter->opb / 256;
            SendData[packlength++] = curinverter->opb % 256;

            //逆变器功率  C
            SendData[packlength++] = curinverter->opc / 256;
            SendData[packlength++] = curinverter->opc % 256;

            //逆变器功率  D
            SendData[packlength++] = curinverter->opd / 256;
            SendData[packlength++] = curinverter->opd % 256;


        }else if((curinverter->model == 7)||(curinverter->model == 8))
        {
            //UID
            SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
            SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
            SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
            SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
            SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
            SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

            SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

            //逆变器类型
            SendData[packlength++] = '0';
            SendData[packlength++] = '1';

            //电网频率
            SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
            SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

            //机内温度
            SendData[packlength++] = (curinverter->it + 100) /256;
            SendData[packlength++] = (curinverter->it + 100) %256;

            //逆变器功率  A
            SendData[packlength++] = curinverter->op / 256;
            SendData[packlength++] = curinverter->op % 256;;

            //电网电压    A
            SendData[packlength++] = curinverter->gv / 256;
            SendData[packlength++] = curinverter->gv % 256;

            //逆变器功率  B
            SendData[packlength++] = curinverter->opb / 256;
            SendData[packlength++] = curinverter->opb % 256;

            //电网电压    B
            SendData[packlength++] = curinverter->gv / 256;
            SendData[packlength++] = curinverter->gv % 256;
        }else if((curinverter->model == 5) || (curinverter->model == 6))
        {
            //UID
            SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
            SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
            SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
            SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
            SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
            SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

            SendData[packlength++] = (curinverter->inverterstatus.dataflag & 0x01);

            //逆变器类型
            SendData[packlength++] = '0';
            SendData[packlength++] = '2';

            //电网频率
            SendData[packlength++] = (int)(curinverter->gf * 10) / 256;
            SendData[packlength++] = (int)(curinverter->gf * 10) % 256;

            //机内温度
            SendData[packlength++] = (curinverter->it + 100) /256;
            SendData[packlength++] = (curinverter->it + 100) %256;

            //逆变器功率  A
            SendData[packlength++] = curinverter->op / 256;
            SendData[packlength++] = curinverter->op % 256;;

            //电网电压    A
            SendData[packlength++] = curinverter->gv / 256;
            SendData[packlength++] = curinverter->gv % 256;

            //逆变器功率  B
            SendData[packlength++] = curinverter->opb / 256;
            SendData[packlength++] = curinverter->opb % 256;

            //电网电压    B
            SendData[packlength++] = curinverter->gvb / 256;
            SendData[packlength++] = curinverter->gvb % 256;


            //逆变器功率  C
            SendData[packlength++] = curinverter->opc / 256;
            SendData[packlength++] = curinverter->opc % 256;

            //电网电压    C
            SendData[packlength++] = curinverter->gvc / 256;
            SendData[packlength++] = curinverter->gvc % 256;

            //逆变器功率  D
            SendData[packlength++] = curinverter->opd / 256;
            SendData[packlength++] = curinverter->opd % 256;

        }else
        {
            //UID
            SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
            SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
            SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
            SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
            SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
            SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

            SendData[packlength++] = 0;

            //逆变器类型
            SendData[packlength++] = '0';
            SendData[packlength++] = '0';
        }


    }

    SendData[packlength++] = 'E';
    SendData[packlength++] = 'N';
    SendData[packlength++] = 'D';

    SendData[5] = (packlength/1000) + '0';
    SendData[6] = ((packlength/100)%10) + '0';
    SendData[7] = ((packlength/10)%10) + '0';
    SendData[8] = ((packlength)%10) + '0';
    SendData[packlength++] = '\n';
    SendToSocketA(SendData ,packlength);
}

//03 COMMAND_POWERCURVE					//功率曲线请求   mapping :: 0x00 匹配  0x01 不匹配   data 表示日期
void APP_Response_PowerCurve(char mapping,char * date)
{
    int packlength = 0,length = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //匹配不成功
    if(mapping == 0x01)
    {
        sprintf(SendData,"APS110015000301\n");
        packlength = 16;
        SendToSocketA(SendData ,packlength);
        return ;
    }

    //拼接需要发送的报文
    sprintf(SendData,"APS110015000300");
    packlength = 15;

    read_system_power(date,&SendData[15],&length);
    packlength += length;

    SendData[packlength++] = 'E';
    SendData[packlength++] = 'N';
    SendData[packlength++] = 'D';

    SendData[5] = (packlength/1000) + '0';
    SendData[6] = ((packlength/100)%10) + '0';
    SendData[7] = ((packlength/10)%10) + '0';
    SendData[8] = ((packlength)%10) + '0';
    SendData[packlength++] = '\n';

    SendToSocketA(SendData ,packlength);
}

//04 COMMAND_GENERATIONCURVE		//发电量曲线请求    mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_GenerationCurve(char mapping,char request_type)
{
    int packlength = 0,len_body = 0;
    char date_time[15] = { '\0' };
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    apstime(date_time);
    //匹配不成功
    if(mapping == 0x01)
    {
        sprintf(SendData,"APS110015000401\n");
        packlength = 16;
        SendToSocketA(SendData ,packlength);
        return ;
    }

    sprintf(SendData,"APS110015000400");
    packlength = 15;
    //拼接需要发送的报文
    if(request_type == '0')
    {//最近一周
        SendData[packlength++] = '0';
        SendData[packlength++] = '0';

        read_weekly_energy(date_time, &SendData[packlength],&len_body);
        packlength += len_body;

    }else if(request_type == '1')
    {//最近一个月
        SendData[packlength++] = '0';
        SendData[packlength++] = '1';
        read_monthly_energy(date_time, &SendData[packlength],&len_body);
        packlength += len_body;

    }else if(request_type == '2')
    {//最近一年
        SendData[packlength++] = '0';
        SendData[packlength++] = '2';
        read_yearly_energy(date_time, &SendData[packlength],&len_body);
        packlength += len_body;

    }

    SendData[packlength++] = 'E';
    SendData[packlength++] = 'N';
    SendData[packlength++] = 'D';

    SendData[5] = (packlength/1000) + '0';
    SendData[6] = ((packlength/100)%10) + '0';
    SendData[7] = ((packlength/10)%10) + '0';
    SendData[8] = ((packlength)%10) + '0';
    SendData[packlength++] = '\n';

    SendToSocketA(SendData ,packlength);
}

//05 COMMAND_REGISTERID 				//逆变器ID注册请求 		mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_RegisterID(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //拼接需要发送的报文
    sprintf(SendData,"APS1100150005%02d\n",mapping);
    packlength = 16;

    SendToSocketA(SendData ,packlength);
}

//06 COMMAND_SETTIME						//时间设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetTime(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //拼接需要发送的报文
    sprintf(SendData,"APS1100150006%02d\n",mapping);
    packlength = 16;

    SendToSocketA(SendData ,packlength);
}

//07 COMMAND_SETWIREDNETWORK		//有线网络设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetWiredNetwork(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //拼接需要发送的报文
    sprintf(SendData,"APS1100150007%02d\n",mapping);
    packlength = 16;

    SendToSocketA(SendData ,packlength);
}


//获取硬件信息
void APP_Response_GetECUHardwareStatus(char mapping)
{
    int packlength = 0;

    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110120000800%02d",WIFI_MODULE_TYPE);
        memset(&SendData[17],'0',100);
        SendData[117] = 'E';
        SendData[118] = 'N';
        SendData[119] = 'D';
        SendData[120] = '\n';
        packlength = 121;
    }else
    {
        sprintf(SendData,"APS110015000801\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);

}


//10 COMMAND_SETWIFIPASSWD			//AP密码设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_SetWifiPasswd(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    //拼接需要发送的报文
    sprintf(SendData,"APS1100150010%02d\n",mapping);
    packlength = 16;

    SendToSocketA(SendData ,packlength);
}

//11	AP密码设置请求
void APP_Response_GetIDInfo(char mapping,inverter_info *inverter)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    char uid[7];
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110015001100");
        packlength = 15;
        for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
        {

            uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
            uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
            uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
            uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
            uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
            uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
            //printf("%02x%02x%02x%02x%02x%02x\n",uid[0],uid[1],uid[2],uid[3],uid[4],uid[5]);
            memcpy(&SendData[packlength],uid,6);
            packlength += 6;
        }

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';


    }else
    {
        sprintf(SendData,"APS110015001101\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);

}
//12	AP密码设置请求
void APP_Response_GetTime(char mapping,char *Time)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110032001200%sEND\n",Time);
        packlength = 33;
    }else
    {
        sprintf(SendData,"APS110015001201\n");
        packlength = 16;
    }

    SendToSocketA(SendData ,packlength);

}


void APP_Response_FlashSize(char mapping,unsigned int Flashsize)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110000001300");
        packlength = 15;
        //printf("%d\n",Flashsize);
        SendData[packlength++] = (Flashsize/16777216)%256;
        SendData[packlength++] = (Flashsize/65536)%256;
        SendData[packlength++] = (Flashsize/256)%256;
        SendData[packlength++] = Flashsize%256;

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';

    }else
    {
        sprintf(SendData,"APS110015001301\n");
        packlength = 16;
    }

    SendToSocketA(SendData ,packlength);

}


//14 COMMAND_SETWIREDNETWORK		//有线网络设置请求			mapping :: 0x00 匹配  0x01 不匹配  
void APP_Response_GetWiredNetwork(char mapping,char dhcpStatus,IP_t IPAddr,IP_t MSKAddr,IP_t GWAddr,IP_t DNS1Addr,IP_t DNS2Addr)
{
    int packlength = 0;
    char MAC[13] = {'\0'};
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapping == 0x00)
    {
        //拼接需要发送的报文
        sprintf(SendData,"APS1100150014%02d\n",mapping);
        packlength = 15;
        if(dhcpStatus == 0)
        {
            SendData[packlength++] = '0';
            SendData[packlength++] = '0';
        }else
        {
            SendData[packlength++] = '0';
            SendData[packlength++] = '1';
        }
        SendData[packlength++] = IPAddr.IP1;
        SendData[packlength++] = IPAddr.IP2;
        SendData[packlength++] = IPAddr.IP3;
        SendData[packlength++] = IPAddr.IP4;

        SendData[packlength++] = MSKAddr.IP1;
        SendData[packlength++] = MSKAddr.IP2;
        SendData[packlength++] = MSKAddr.IP3;
        SendData[packlength++] = MSKAddr.IP4;

        SendData[packlength++] = GWAddr.IP1;
        SendData[packlength++] = GWAddr.IP2;
        SendData[packlength++] = GWAddr.IP3;
        SendData[packlength++] = GWAddr.IP4;

        SendData[packlength++] = DNS1Addr.IP1;
        SendData[packlength++] = DNS1Addr.IP2;
        SendData[packlength++] = DNS1Addr.IP3;
        SendData[packlength++] = DNS1Addr.IP4;

        SendData[packlength++] = DNS2Addr.IP1;
        SendData[packlength++] = DNS2Addr.IP2;
        SendData[packlength++] = DNS2Addr.IP3;
        SendData[packlength++] = DNS2Addr.IP4;
        sprintf(MAC,"%02x%02x%02x%02x%02x%02x",ecu.MacAddress[0],ecu.MacAddress[1],ecu.MacAddress[2],ecu.MacAddress[3],ecu.MacAddress[4],ecu.MacAddress[5]);
        memcpy(&SendData[packlength],MAC,12);
        packlength += 12;

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';
        SendToSocketA(SendData ,packlength);
    }else
    {
        sprintf(SendData,"APS110015001401\n");
        packlength = 16;
        SendToSocketA(SendData ,packlength);
    }

}


void APP_Response_SetChannel(unsigned char mapflag,char SIGNAL_CHANNEL,char SIGNAL_LEVEL)
{
    //char SendData[22] = {'\0'};
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapflag == 1)
    {
        sprintf(SendData,"APS110015001501\n");
        SendToSocketA(SendData ,16);
    }else{
        sprintf(SendData,"APS110023001500%02x%03dEND\n",SIGNAL_CHANNEL,SIGNAL_LEVEL);
        SendToSocketA(SendData ,24);
    }
}

void APP_Response_GetShortAddrInfo(char mapping,inverter_info *inverter)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    char uid[7];
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110015001800");
        packlength = 15;
        SendData[packlength++] = rateOfProgress;
        for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
        {

            uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
            uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
            uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
            uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
            uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
            uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
            memcpy(&SendData[packlength],uid,6);
            packlength += 6;
            SendData[packlength++] = curinverter->shortaddr/256;
            SendData[packlength++] = curinverter->shortaddr%256;
        }

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';


    }else
    {
        sprintf(SendData,"APS110015001801\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);

}

void APP_Response_GetECUAPInfo(char mapping,unsigned char connectStatus,char *info)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        if(0 == connectStatus)
        {
            sprintf(SendData,"APS110019002000%1dEND\n",connectStatus);
            packlength = 20;
        }else
        {
            sprintf(SendData,"APS11%04d002000%1d%sEND\n",(strlen(info) + 19),connectStatus,info);
            packlength = (strlen(info) + 20);
            printf("sendData:%s\n",SendData);
        }

    }else
    {
        sprintf(SendData,"APS110015002001\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);

}


//ECU-RS设置WIFI密码
void APP_Response_SetECUAPInfo(unsigned char result)
{
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    sprintf(SendData,"APS1100150021%02d\n",result);
    SendToSocketA(SendData ,16);
}


void APP_Response_GetECUAPList(char mapping,char *list)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        sprintf(SendData,"APS11%04d002200%sEND\n",(strlen(list) + 18),list);
        packlength = (strlen(list) + 19);
    }else
    {
        sprintf(SendData,"APS110015002201\n");
        packlength = 16;
    }

    SendToSocketA(SendData ,packlength);
}

void APP_Response_GetFunctionStatusInfo(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);


    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110015002300");
        packlength = 15;
        SendData[packlength++] = '0';
        memset(&SendData[packlength],'0',300);
        packlength += 300;
        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';


    }else
    {
        sprintf(SendData,"APS110015002301\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_ServerInfo(char mapping,ECUServerInfo_t *serverInfo)
{
    int packlength = 0;
    int domain_len = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        Resolve_Server(serverInfo);
        domain_len = strlen(serverInfo->domain);
        if(serverInfo->serverCmdType == SERVER_UPDATE_GET)
        {	//从文件中读取对应的信息
            sprintf(SendData,"APS1100150024%02d00%03d",SERVER_UPDATE_GET,domain_len);
            packlength = 20;
            //域名
            memcpy(&SendData[packlength],serverInfo->domain,domain_len);
            packlength += domain_len;
            //IP
            SendData[packlength++] = serverInfo->IP[0];
            SendData[packlength++] = serverInfo->IP[1];
            SendData[packlength++] = serverInfo->IP[2];
            SendData[packlength++] = serverInfo->IP[3];
            //Port1
            SendData[packlength++] = serverInfo->Port1/256;
            SendData[packlength++] = serverInfo->Port1%256;
            //Port2
            SendData[packlength++] = serverInfo->Port2/256;
            SendData[packlength++] = serverInfo->Port2%256;
            //Port3
            SendData[packlength++] = 0;
            SendData[packlength++] = 0;


        }else if(serverInfo->serverCmdType == SERVER_CLIENT_GET)
        {
            sprintf(SendData,"APS1100150024%02d00%03d",SERVER_CLIENT_GET,domain_len);
            packlength = 20;
            memcpy(&SendData[packlength],serverInfo->domain,domain_len);
            packlength += domain_len;
            //IP
            SendData[packlength++] = serverInfo->IP[0];
            SendData[packlength++] = serverInfo->IP[1];
            SendData[packlength++] = serverInfo->IP[2];
            SendData[packlength++] = serverInfo->IP[3];
            //Port1
            SendData[packlength++] = serverInfo->Port1/256;
            SendData[packlength++] = serverInfo->Port1%256;
            //Port2
            SendData[packlength++] = serverInfo->Port2/256;
            SendData[packlength++] = serverInfo->Port2%256;
            //Port3
            SendData[packlength++] = 0;
            SendData[packlength++] = 0;
        }else if(serverInfo->serverCmdType == SERVER_CONTROL_GET)
        {
            sprintf(SendData,"APS1100150024%02d00%03d",SERVER_CONTROL_GET,domain_len);
            packlength = 20;
            memcpy(&SendData[packlength],serverInfo->domain,domain_len);
            packlength += domain_len;
            //IP
            SendData[packlength++] = serverInfo->IP[0];
            SendData[packlength++] = serverInfo->IP[1];
            SendData[packlength++] = serverInfo->IP[2];
            SendData[packlength++] = serverInfo->IP[3];
            //Port1
            SendData[packlength++] = serverInfo->Port1/256;
            SendData[packlength++] = serverInfo->Port1%256;
            //Port2
            SendData[packlength++] = serverInfo->Port2/256;
            SendData[packlength++] = serverInfo->Port2%256;
            //Port3
            SendData[packlength++] = 0;
            SendData[packlength++] = 0;
        }else if(serverInfo->serverCmdType == SERVER_UPDATE_SET)
        {
            sprintf(SendData,"APS1100150024%02d00",SERVER_UPDATE_SET);
            Save_Server(serverInfo);
            packlength = 17;
        }else if(serverInfo->serverCmdType == SERVER_CLIENT_SET)
        {
            sprintf(SendData,"APS1100150024%02d00",SERVER_CLIENT_SET);
            Save_Server(serverInfo);
            packlength = 17;
        }else if(serverInfo->serverCmdType == SERVER_CONTROL_SET)
        {
            sprintf(SendData,"APS1100150024%02d00",SERVER_CONTROL_SET);
            Save_Server(serverInfo);
            packlength = 17;
        }else
        {
            return;
        }

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';

    }else
    {
        sprintf(SendData,"APS1100170024%02d01\n",serverInfo->serverCmdType );
        packlength = 18;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_InverterMaxPower(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length)
{
    int packlength = 0,index = 0,i = 0,total=0,flag = 0;
    unsigned short maxPower = 0;
    inverter_info *curinverter = inverter;
    unsigned char UID[7] = {'\0'};

    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        if(1 == cmd)
        {
            char * MaxPowerList = NULL;
            int length = 0;
            MaxPowerList = malloc(1300);
            memset(MaxPowerList,0x00,1300);
            //读取实际最大功率值
            //从文件系统中读取功率值
            phone_getMaxPower(MaxPowerList,&length);
            total = length /10;
            for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
            {
                flag = 0;	//未从文件读取到
                UID[0] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
                UID[1] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
                UID[2] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
                UID[3] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
                UID[4] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
                UID[5] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

                for(i = 0;i<total;i++)
                {
                    if(!memcmp(UID,&MaxPowerList[i*10],6))
                    {
                        flag = 1;
                        break;
                    }
                }

                if(0 == flag)
                {
                    memcpy(&MaxPowerList[length],UID,6);
                    MaxPowerList[length+6] = 0xff;
                    MaxPowerList[length+7] = 0xff;
                    MaxPowerList[length+8] = 0xff;
                    MaxPowerList[length+9] = 0xff;
                    length+=10;
                    total++;
                }

            }
            sprintf(SendData,"APS11000000250100");
            packlength = 17;
            memcpy(&SendData[packlength],MaxPowerList,length);
            packlength += length;
            SendData[packlength++] = 'E';
            SendData[packlength++] = 'N';
            SendData[packlength++] = 'D';

            SendData[5] = (packlength/1000) + '0';
            SendData[6] = ((packlength/100)%10) + '0';
            SendData[7] = ((packlength/10)%10) + '0';
            SendData[8] = ((packlength)%10) + '0';
            SendData[packlength++] = '\n';
            free(MaxPowerList);
            MaxPowerList = NULL;
        }else if(2 == cmd)
        {	//设置最大功率值(广播)
            maxPower = recvbuffer[30]*256 + recvbuffer[31];
            printf("maxPower:%d\n",maxPower);
            phone_MaxPower_ALL(inverter,maxPower);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000250200END\n");
            packlength = 21;
        }else if(3 == cmd)
        {	//设置最大功率值(单播)
            phone_MaxPower_single(&recvbuffer[30],length);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000250300END\n");
            packlength = 21;
        }
        else if(4 == cmd)
        {	//下发读取命令
            echo("/tmp/maxpower.con","ALL");
            processAllFlag = 1;
            sprintf(SendData,"APS11002000250400END\n");
            packlength = 21;
        }




    }else
    {
        sprintf(SendData,"APS1100170026%02d01\n",cmd);
        packlength = 18;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_InverterOnOff(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    char turnOnOff;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        if(1 == cmd)
        {
            //还没到第一次获取数据
            if(ecu.had_data_broadcast_time[0] == '\0')
            {
                sprintf(SendData,"APS11002000260100END\n");
                packlength = 21;
                SendToSocketA(SendData ,packlength);
                return ;
            }
            //获取当前的开关机状态
            sprintf(SendData,"APS11000000260100");
            packlength = 17;
            for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
            {
                SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
                SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
                SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
                SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
                SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
                SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

                if((curinverter->inverterstatus.dataflag & 0x01))
                {
                    SendData[packlength++] = curinverter->status_web[18];
                }
                else
                {
                    SendData[packlength++] = '2';
                }

            }

            SendData[packlength++] = 'E';
            SendData[packlength++] = 'N';
            SendData[packlength++] = 'D';

            SendData[5] = (packlength/1000) + '0';
            SendData[6] = ((packlength/100)%10) + '0';
            SendData[7] = ((packlength/10)%10) + '0';
            SendData[8] = ((packlength)%10) + '0';
            SendData[packlength++] = '\n';
        }else if(2 == cmd)
        {	//设置开关机状态(广播)
            turnOnOff = recvbuffer[30];
            printf("turnOnOff:%c\n",turnOnOff);
            if('0' == turnOnOff)
            {	//开机
                echo("/tmp/connect.con","connect all");
            }else
            {	//关机
                echo("/tmp/connect.con","disconnect all");
            }
            processAllFlag = 1;
            sprintf(SendData,"APS11002000260200END\n");
            packlength = 21;
        }else if(3 == cmd)
        {	//设置开关机状态(单播)
            phone_turnOnOff_single(&recvbuffer[30],length);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000260300END\n");
            packlength = 21;
        }



    }else
    {
        sprintf(SendData,"APS1100170026%02d01\n",cmd);
        packlength = 18;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_InverterGFDI(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        if(1 == cmd)
        {
            //还没到第一次获取数据
            if(ecu.had_data_broadcast_time[0] == '\0')
            {
                sprintf(SendData,"APS11002000270100END\n");
                packlength = 21;
                SendToSocketA(SendData ,packlength);
                return ;
            }
            //获取当前的开关机状态
            sprintf(SendData,"APS11000000270100");
            packlength = 17;
            for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
            {
                SendData[packlength++] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
                SendData[packlength++] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
                SendData[packlength++] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
                SendData[packlength++] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
                SendData[packlength++] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
                SendData[packlength++] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

                if((curinverter->inverterstatus.dataflag & 0x01))
                {
                    SendData[packlength++] = curinverter->status_web[17];
                }
                else
                {
                    SendData[packlength++] = '2';
                }

            }

            SendData[packlength++] = 'E';
            SendData[packlength++] = 'N';
            SendData[packlength++] = 'D';

            SendData[5] = (packlength/1000) + '0';
            SendData[6] = ((packlength/100)%10) + '0';
            SendData[7] = ((packlength/10)%10) + '0';
            SendData[8] = ((packlength)%10) + '0';
            SendData[packlength++] = '\n';
        }else if(2 == cmd)
        {	//设置开关机状态(单播)
            phone_GFDI_single(&recvbuffer[30],length);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000270200END\n");
            packlength = 21;
        }

    }else
    {
        sprintf(SendData,"APS1100170026%02d01\n",cmd);
        packlength = 18;
    }
    SendToSocketA(SendData ,packlength);
}

void APP_Response_InverterIRD(char mapping,int cmd,inverter_info *inverter,const char *recvbuffer,int length)
{
    int packlength = 0,index = 0,i = 0,total=0,flag = 0;
    unsigned char ird = 0;
    inverter_info *curinverter = inverter;
    unsigned char UID[7] = {'\0'};

    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        if(1 == cmd)
        {
            char * IRDList = NULL;
            int length = 0;
            IRDList = malloc(1024);
            memset(IRDList,0x00,1024);
            //读取实际IRD状态
            //从文件系统中读取IRD状态
            phone_getIRD(IRDList,&length);
            total = length /8;
            for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
            {
                flag = 0;	//未从文件读取到
                UID[0] = ((curinverter->id[0]-'0') << 4) + (curinverter->id[1]-'0');
                UID[1] = ((curinverter->id[2]-'0') << 4) + (curinverter->id[3]-'0');
                UID[2] = ((curinverter->id[4]-'0') << 4) + (curinverter->id[5]-'0');
                UID[3] = ((curinverter->id[6]-'0') << 4) + (curinverter->id[7]-'0');
                UID[4] = ((curinverter->id[8]-'0') << 4) + (curinverter->id[9]-'0');
                UID[5] = ((curinverter->id[10]-'0') << 4)+ (curinverter->id[11]-'0');

                for(i = 0;i<total;i++)
                {
                    if(!memcmp(UID,&IRDList[i*8],6))
                    {
                        flag = 1;
                        break;
                    }
                }

                if(0 == flag)
                {
                    memcpy(&IRDList[length],UID,6);
                    IRDList[length+6] = 0xff;
                    IRDList[length+7] = 0xff;
                    length+=8;
                    total++;
                }

            }
            sprintf(SendData,"APS11000000280100");
            packlength = 17;
            memcpy(&SendData[packlength],IRDList,length);
            packlength += length;
            SendData[packlength++] = 'E';
            SendData[packlength++] = 'N';
            SendData[packlength++] = 'D';

            SendData[5] = (packlength/1000) + '0';
            SendData[6] = ((packlength/100)%10) + '0';
            SendData[7] = ((packlength/10)%10) + '0';
            SendData[8] = ((packlength)%10) + '0';
            SendData[packlength++] = '\n';
            free(IRDList);
            IRDList = NULL;
        }else if(2 == cmd)
        {	//设置最大功率值(广播)
            ird = recvbuffer[30];
            printf("ird:%d\n",ird);
            phone_IRD_ALL(ird);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000280200END\n");
            packlength = 21;
        }else if(3 == cmd)
        {	//设置最大功率值(单播)
            phone_IRD_single(&recvbuffer[30],length);
            processAllFlag = 1;
            sprintf(SendData,"APS11002000280300END\n");
            packlength = 21;
        }
        else if(4 == cmd)
        {	//下发读取命令
            echo("/tmp/get_ird.con","ALL");
            processAllFlag = 1;
            sprintf(SendData,"APS11002000280400END\n");
            packlength = 21;
        }

    }else
    {
        sprintf(SendData,"APS1100170028%02d01\n",cmd);
        packlength = 18;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_RSSI(char mapping,inverter_info *inverter)
{
    int packlength = 0,index = 0;
    inverter_info *curinverter = inverter;
    char uid[7];
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);

    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110015003000");
        packlength = 15;
        for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)
        {

            uid[0] = (curinverter->id[0] - '0')*16+(curinverter->id[1] - '0');
            uid[1] = (curinverter->id[2] - '0')*16+(curinverter->id[3] - '0');
            uid[2] = (curinverter->id[4] - '0')*16+(curinverter->id[5] - '0');
            uid[3] = (curinverter->id[6] - '0')*16+(curinverter->id[7] - '0');
            uid[4] = (curinverter->id[8] - '0')*16+(curinverter->id[9] - '0');
            uid[5] = (curinverter->id[10] - '0')*16+(curinverter->id[11] - '0');
            memcpy(&SendData[packlength],uid,6);
            packlength += 6;
            SendData[packlength++] = curinverter->signalstrength;
        }

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';

    }else
    {
        sprintf(SendData,"APS110015003001\n");
        packlength = 16;
    }
    SendToSocketA(SendData ,packlength);
}


void APP_Response_ClearEnergy(char mapping)
{
    int packlength = 0;
    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapping == 0x00)
    {
        sprintf(SendData,"APS110015003100\n");
        packlength = 16;

    }else
    {
        sprintf(SendData,"APS110015003101\n");
        packlength = 16;
    }

    SendToSocketA(SendData ,packlength);
}

void APP_Response_AlarmEvent(char mapping,const char *Date,const char *serial)
{
    int packlength = 0;

    memset(SendData,'\0',MAXINVERTERCOUNT * INVERTER_PHONE_PER_LEN + INVERTER_PHONE_PER_OTHER);
    if(mapping == 0x00)
    {
        char * EventList = NULL;
        int length = 0;
        EventList = malloc(3500);
        memset(EventList,0x00,3500);

        phone_getEvent(EventList,&length,Date,serial);
        sprintf(SendData,"APS110015003200");
        packlength = 15;
        memcpy(&SendData[packlength],EventList,length);
        packlength += length;

        SendData[packlength++] = 'E';
        SendData[packlength++] = 'N';
        SendData[packlength++] = 'D';

        SendData[5] = (packlength/1000) + '0';
        SendData[6] = ((packlength/100)%10) + '0';
        SendData[7] = ((packlength/10)%10) + '0';
        SendData[8] = ((packlength)%10) + '0';
        SendData[packlength++] = '\n';
        free(EventList);
        EventList = NULL;

    }else
    {
        sprintf(SendData,"APS110015003201\n");
        packlength = 16;
    }

    SendToSocketA(SendData ,packlength);
}



