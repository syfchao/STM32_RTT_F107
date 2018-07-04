#include "WiFiFileAPI.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "usart5.h"
#include "rtthread.h"
#include "threadlist.h"
#include "dfs_posix.h"
#include "stdlib.h"
#include "debug.h"
#include "flash_if.h"

#define WIFISERVER_CREATESOCKET2_TIMES		3		//WIFI服务器Socket2尝试重连次数
#define WIFISERVER_COMMAND_TIMES			3		//WIFI服务器尝试命令次数

const unsigned short CRC_table_16[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

//获取文件大小以及校验属性
int getFileAttribute(char *Path,unsigned int *FileSize,unsigned short *Check)
{
    //获取文件大小
    int fd = 0,readsize = 0;
    unsigned char *buff = NULL;
    struct stat *fileStat = NULL;
    fileStat = (struct stat *)malloc(sizeof(struct stat));
    memset(fileStat,0x00,sizeof(struct stat));
    stat(Path, fileStat);
    *FileSize = fileStat->st_size;
    free(fileStat);
    fileStat = NULL;

    //计算校验值
    fd =  open(Path, O_RDONLY, 0);
    if(fd>=0)
    {	//文件打开成功
        buff = malloc(1024);
        memset (buff,0x00,1024);
        *Check = 0;
        while((readsize = read(fd,buff,1024)) > 0)
        {
            *Check = GetCrc_16(buff, readsize, *Check, CRC_table_16);
            if(readsize < 1024)
                break;
        }
        close(fd);
        free(buff);
        buff = NULL;
        return 0;
    }

    //文件打开失败，直接返回-1
    return -1;
}

int getFileAttributeInternalFlash(unsigned int FileSize,unsigned short *Check)
{
    //获取文件大小
    unsigned char *buff = NULL;
    unsigned int app2addr = 0x08080000;
    int i = 0;
    buff = malloc(1024);
    memset (buff,0x00,1024);
    *Check = 0;
    while(1)
    {
        for(i = 0;i<1024;i++)
        {
            buff[i] = (*(unsigned char *)(app2addr));
            app2addr++;
        }
        if(FileSize > 1024)
        {
            *Check = GetCrc_16(buff, 1024, *Check, CRC_table_16);
        }else
        {
            *Check = GetCrc_16(buff, FileSize, *Check, CRC_table_16);
            break;
        }
        FileSize -=1024;
    }

    free(buff);
    buff = NULL;
    return 0;
}

unsigned short GetCrc_16(unsigned char * pData, unsigned short nLength, unsigned short init, const unsigned short *ptable)
{
    unsigned short cRc_16 = init;
    unsigned char  temp;

    while(nLength-- > 0)
    {
        temp = cRc_16 >> 8;
        cRc_16 = (cRc_16 << 8) ^ ptable[(temp ^ *pData++) & 0xFF];
    }

    return cRc_16;
}

static int analysisRequestData(unsigned char *WIFI_RecvWiFiFileData,StRequestRecvDataSt *RecvInfo)
{
    char cmd_str[5] = {'\0'};
    int cmd = 0;
    memcpy(cmd_str,&WIFI_RecvWiFiFileData[13],4);
    cmd = atoi(cmd_str);
    if(cmd == EN_CMD_DOWNLOAD_FILE)
    {
        RecvInfo->cmd = EN_CMD_DOWNLOAD_FILE;
        //文件存在标志
        if(WIFI_RecvWiFiFileData[20] == '1')
        {
            RecvInfo->RecvData.recvDownloadInfo.downloadStatus = EN_DOWNLOAD_FILE_EXIST;
        }else
        {
            RecvInfo->RecvData.recvDownloadInfo.downloadStatus = EN_DOWNLOAD_FILE_NOT_EXIST;
        }
        //文件大小
        RecvInfo->RecvData.recvDownloadInfo.File_Size = WIFI_RecvWiFiFileData[21]*16777216 + WIFI_RecvWiFiFileData[22]*65536 + WIFI_RecvWiFiFileData[23]*256+ WIFI_RecvWiFiFileData[24];
        //文件整体校验
        RecvInfo->RecvData.recvDownloadInfo.Check = WIFI_RecvWiFiFileData[25]*256+ WIFI_RecvWiFiFileData[26];
        printf("Download File Attribute File  Flag:%d FileSize:%d File Check:%02x\n",RecvInfo->RecvData.recvDownloadInfo.downloadStatus,RecvInfo->RecvData.recvDownloadInfo.File_Size,RecvInfo->RecvData.recvDownloadInfo.Check);
    }else if(cmd == EN_CMD_DELETE_FILE)
    {
        //文件删除标志
        RecvInfo->cmd = EN_CMD_DELETE_FILE;
        if(WIFI_RecvWiFiFileData[20] == '2')
        {
            RecvInfo->RecvData.recvDeleteInfo = EN_DELETE_FILE_FAILED;
        }else if(WIFI_RecvWiFiFileData[20] == '1')
        {
            RecvInfo->RecvData.recvDeleteInfo = EN_DELETE_FILE_SUCCESS;
        }else
        {
            RecvInfo->RecvData.recvDeleteInfo = EN_DELETE_FILE_NOT_EXIST;
        }
        printf("Delete File Attribute File Flag:%d\n",RecvInfo->RecvData.recvDeleteInfo);
    }else if(cmd == EN_CMD_UPLOAD_FILE)
    {
        //文件上传标志
        RecvInfo->cmd = EN_CMD_UPLOAD_FILE;
        if(WIFI_RecvWiFiFileData[20] == '1')
        {
            RecvInfo->RecvData.recvUploadInfo = EN_UPLOAD_FILE_PATH_EXIST;
        }else
        {
            RecvInfo->RecvData.recvUploadInfo = EN_UPLOAD_FILE_PATH_NOT_EXIST;
        }
        printf("Upload File Attribute File Flag:%d\n",RecvInfo->RecvData.recvUploadInfo);
    }else
    {
        return -1;
    }

    return 0;
}

static int analysisDownloadData(unsigned char *WIFI_RecvWiFiFileData,StDownloadRecvDataSt *RecvInfo)
{
    if('1' == WIFI_RecvWiFiFileData[13])
    {
        RecvInfo->Flag = 1;
    }else
    {
        RecvInfo->Flag = 0;
    }

    if('1' == WIFI_RecvWiFiFileData[14])
    {
        RecvInfo->Next_Flag = 1;
    }else
    {
        RecvInfo->Next_Flag = 0;
    }
    RecvInfo->Data_Len  = WIFI_RecvWiFiFileData[15] *256+WIFI_RecvWiFiFileData[16];

    if(RecvInfo->Data_Len > 0)
        memcpy(RecvInfo->Data,&WIFI_RecvWiFiFileData[17],RecvInfo->Data_Len);
    RecvInfo->Check = WIFI_RecvWiFiFileData[17+RecvInfo->Data_Len] *256+WIFI_RecvWiFiFileData[18+RecvInfo->Data_Len];
    //检验判断
    if(RecvInfo->Data_Len > 0)
    {
        if(RecvInfo->Check != GetCrc_16(RecvInfo->Data,RecvInfo->Data_Len,0,CRC_table_16))
            return -1;
    }
    return 0;
}

static int analysisUploadData(unsigned char *WIFI_RecvWiFiFileData,StUploadRecvDataSt *RecvInfo)
{
    if('1' == WIFI_RecvWiFiFileData[13])
    {
        RecvInfo->Flag = 1;
    }else
    {
        RecvInfo->Flag = 0;
    }
    return 0;
}

//创建2的SOCKET
static int WiFiSocketCreate(char *IP ,int port)
{
    AT_CIPCLOSE('2');
    if(!AT_CIPSTART('2',"TCP",IP ,port))
    {
        printmsg(ECU_DBG_WIFI,"Create File Socket Success");
        return 1;
    }else
    {
        printmsg(ECU_DBG_WIFI,"Create File Socket Failed");
        return -1;
    }
}


static int WifiFile_Request(StRequestSendDataSt *SendInfo,StRequestRecvDataSt *RecvInfo)
{
    unsigned char *Sendbuff = NULL;
    unsigned char *Recvbuff = NULL;
    int bufflen = 0,index = 0,index_create = 0,timeindex = 0;
    if((SendInfo->cmd != EN_CMD_DOWNLOAD_FILE)&&(SendInfo->cmd != EN_CMD_DELETE_FILE)&&(SendInfo->cmd != EN_CMD_UPLOAD_FILE))
    {
        printf("WifiFile_Request Input Arg Failed\n");
        return -1;
    }
    Sendbuff = malloc(1460);
    Recvbuff = malloc(1460);
    memset(Sendbuff,0x00,1460);
    memset(Recvbuff,0x00,1460);
    //组包发送
    sprintf((char *)Sendbuff,"ECU1100000001");
    bufflen = 13;
    sprintf((char *)&Sendbuff[bufflen],"%04d",SendInfo->cmd);
    bufflen += 4;

    if((SendInfo->cmd == EN_CMD_DOWNLOAD_FILE) ||(SendInfo->cmd == EN_CMD_DELETE_FILE))
    {
        sprintf((char *)&Sendbuff[bufflen],"%s",SendInfo->SendData.senddDownloadDeleteInfo.Path);
        bufflen += 60;
        sprintf((char *)&Sendbuff[bufflen],"END");
        bufflen += 3;
    }else
    {
        sprintf((char *)&Sendbuff[bufflen],"%s",SendInfo->SendData.sendUploadInfo.Path);
        bufflen += 60;
        sprintf((char *)&Sendbuff[bufflen],"END");
        bufflen += 3;
        //文件大小
        Sendbuff[bufflen++] = (SendInfo->SendData.sendUploadInfo.File_Size/16777216)%256;
        Sendbuff[bufflen++] = (SendInfo->SendData.sendUploadInfo.File_Size/65536)%256;
        Sendbuff[bufflen++] = (SendInfo->SendData.sendUploadInfo.File_Size/256)%256;
        Sendbuff[bufflen++] = SendInfo->SendData.sendUploadInfo.File_Size%256;

        //校验
        Sendbuff[bufflen++] = (SendInfo->SendData.sendUploadInfo.Check/256)%256;
        Sendbuff[bufflen++] = SendInfo->SendData.sendUploadInfo.Check%256;
        sprintf((char *)&Sendbuff[bufflen],"END");
        bufflen += 3;
    }
    Sendbuff[5] = (bufflen/1000) + '0';
    Sendbuff[6] = ((bufflen/100)%10) + '0';
    Sendbuff[7] = ((bufflen/10)%10) + '0';
    Sendbuff[8] = ((bufflen)%10) + '0';
    for(index_create = 0;index_create < WIFISERVER_CREATESOCKET2_TIMES;index_create++)
    {
        for( index = 0;index < WIFISERVER_COMMAND_TIMES;index++)
        {
            //printhexmsg(ECU_DBG_WIFI,"WifiFile_Request Send:",(char *)Sendbuff,bufflen);
            WIFI_Recv_WiFiFile_Event = 0;
            if(-1 == SendToSocket('2',(char*)Sendbuff,bufflen))
            {
                break;
            }

            for(timeindex = 0;timeindex < 300;timeindex++)
            {
                if(WIFI_Recv_WiFiFile_Event == 1)
                {
                    //解析数据到结构体中
                    printf("File Name:%s\n",SendInfo->SendData.senddDownloadDeleteInfo.Path);
                    //printhexmsg(ECU_DBG_WIFI,"WifiFile_Request Recv:",(char *)WIFI_RecvWiFiFileData,WIFI_Recv_WiFiFile_LEN);
                    if(0 == analysisRequestData(WIFI_RecvWiFiFileData,RecvInfo))
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return 0;
                    }else
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return -1;
                    }
                }
                rt_thread_delay(1);
            }
            rt_thread_delay(RT_TICK_PER_SECOND*1);
        }
        rt_thread_delay(RT_TICK_PER_SECOND*30);
        //失败重连
        WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1);
    }
    free(Sendbuff);
    free(Recvbuff);
    Sendbuff = NULL;
    Recvbuff = NULL;
    return -1;
}

static int WifiFile_Download(StDownloadSendDataSt *SendInfo,StDownloadRecvDataSt *RecvInfo)
{
    unsigned char *Sendbuff = NULL;
    unsigned char *Recvbuff = NULL;
    int bufflen = 0,index = 0,timeindex = 0;

    Sendbuff = malloc(1460);
    Recvbuff = malloc(1460);
    memset(Sendbuff,0x00,1460);
    memset(Recvbuff,0x00,1460);
    //组包发送
    sprintf((char *)Sendbuff,"ECU1100000002");
    bufflen = 13;
    sprintf((char *)&Sendbuff[bufflen],"%s",SendInfo->Path);
    bufflen += 60;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/16777216)%256;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/65536)%256;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/256)%256;
    Sendbuff[bufflen++] = SendInfo->Serial_Num%256;
    sprintf((char *)&Sendbuff[bufflen],"END");
    bufflen += 3;

    Sendbuff[5] = (bufflen/1000) + '0';
    Sendbuff[6] = ((bufflen/100)%10) + '0';
    Sendbuff[7] = ((bufflen/10)%10) + '0';
    Sendbuff[8] = ((bufflen)%10) + '0';

    for( index = 0;index < WIFISERVER_COMMAND_TIMES+2;index++)
    {
        for( index = 0;index < WIFISERVER_COMMAND_TIMES;index++)
        {
            //printhexmsg(ECU_DBG_WIFI,"WifiFile_Download Send:",(char *)Sendbuff,bufflen);
            WIFI_Recv_WiFiFile_Event = 0;
            if(-1 == SendToSocket('2',(char*)Sendbuff,bufflen))
            {
                  printf("SendToSocket 2 failed\n");
		break;
            }

            for(timeindex = 0;timeindex < 300;timeindex++)
            {
                if(WIFI_Recv_WiFiFile_Event == 1)
                {
                    //printhexmsg(ECU_DBG_WIFI,"WifiFile_Download Recv:",(char *)WIFI_RecvWiFiFileData,WIFI_Recv_WiFiFile_LEN);
                    //解析数据到结构体中
                    if(0 == analysisDownloadData(WIFI_RecvWiFiFileData,RecvInfo))
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return 0;
                    }else
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return -1;
                    }

                }
                rt_thread_delay(1);
            }
            rt_thread_delay(RT_TICK_PER_SECOND*1);
        }
        rt_thread_delay(RT_TICK_PER_SECOND*30);
        //失败重连
        WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1);
    }
    free(Sendbuff);
    free(Recvbuff);
    Sendbuff = NULL;
    Recvbuff = NULL;
    return -1;
}

static int WifiFile_Upload(StUploadSendDataSt *SendInfo,StUploadRecvDataSt *RecvInfo)
{
    unsigned char *Sendbuff = NULL;
    unsigned char *Recvbuff = NULL;
    int bufflen = 0,index = 0,timeindex = 0;

    Sendbuff = malloc(1460);
    Recvbuff = malloc(1460);
    memset(Sendbuff,0x00,1460);
    memset(Recvbuff,0x00,1460);
    //组包发送
    sprintf((char *)Sendbuff,"ECU1100000003");
    bufflen = 13;
    sprintf((char *)&Sendbuff[bufflen],"%s",SendInfo->Path);
    bufflen += 60;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/16777216)%256;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/65536)%256;
    Sendbuff[bufflen++] = (SendInfo->Serial_Num/256)%256;
    Sendbuff[bufflen++] = SendInfo->Serial_Num%256;
    if(SendInfo->Next_Flag == 1)
    {
        Sendbuff[bufflen++] = '1';
    }else
    {
        Sendbuff[bufflen++] = '0';
    }
    //数据长度
    Sendbuff[bufflen++] = SendInfo->Data_Len/256;
    Sendbuff[bufflen++] = SendInfo->Data_Len%256;
    //数据
    memcpy(&Sendbuff[bufflen],SendInfo->Data,SendInfo->Data_Len);
    bufflen += SendInfo->Data_Len;
    //校验
    Sendbuff[bufflen++] = SendInfo->Check/256;
    Sendbuff[bufflen++] = SendInfo->Check%256;
    sprintf((char *)&Sendbuff[bufflen],"END");
    bufflen += 3;

    Sendbuff[5] = (bufflen/1000) + '0';
    Sendbuff[6] = ((bufflen/100)%10) + '0';
    Sendbuff[7] = ((bufflen/10)%10) + '0';
    Sendbuff[8] = ((bufflen)%10) + '0';
    for( index = 0;index < WIFISERVER_COMMAND_TIMES;index++)
    {


        for( index = 0;index < WIFISERVER_COMMAND_TIMES;index++)
        {
            //printhexmsg(ECU_DBG_WIFI,"WifiFile_Upload Send:",(char *)Sendbuff,bufflen);
            WIFI_Recv_WiFiFile_Event = 0;
            if(-1 == SendToSocket('2',(char*)Sendbuff,bufflen))
            {
                break;
            }

            for(timeindex = 0;timeindex < 300;timeindex++)
            {
                if(WIFI_Recv_WiFiFile_Event == 1)
                {
                    //printhexmsg(ECU_DBG_WIFI,"WifiFile_Upload Recv:",(char *)WIFI_RecvWiFiFileData,WIFI_Recv_WiFiFile_LEN);
                    //解析数据到结构体中
                    if(0 == analysisUploadData(WIFI_RecvWiFiFileData,RecvInfo))
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return 0;
                    }else
                    {
                        free(Sendbuff);
                        free(Recvbuff);
                        Sendbuff = NULL;
                        Recvbuff = NULL;
                        return -1;
                    }
                }
                rt_thread_delay(1);
            }
            rt_thread_delay(RT_TICK_PER_SECOND*1);
        }
        rt_thread_delay(RT_TICK_PER_SECOND*20);
        //失败重连
        WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1);
    }
    free(Sendbuff);
    free(Recvbuff);
    Sendbuff = NULL;
    Recvbuff = NULL;
    return -1;
}

//下载文件
//返回值:
//    0:文件下载成功
//  -1:创建连接失败
//  -2:发送文件下载请求失败
//  -3:文件不存在
//  -4: 本地文件打开失败
//  -5:文件下载命令失败
//  -6:文件下载异常
//  -7:本地文件写入失败
//  -8:单条记录校验错误
//  -9:打开本地文件失败
//  -10:本地文件大小或校验失败
int WiFiFile_GetFile(char *remoteFile,char *localFile)
{
    unsigned int FileSize = 0;
    unsigned short Check = 0;
    StRequestSendDataSt *RequestSendInfo = NULL;
    StRequestRecvDataSt *RequestRecvInfo = NULL;
    RequestSendInfo = (StRequestSendDataSt *)malloc(sizeof(StRequestSendDataSt));
    RequestRecvInfo = (StRequestRecvDataSt *)malloc(sizeof(StRequestRecvDataSt));
    //问询远程文件是否存在
    if(-1 == WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1))
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        WiFiFileDownload_Status = WiFiFileNoConnect;
        return -1;
    }
    //问询文件是否存在
    RequestSendInfo->cmd = EN_CMD_DOWNLOAD_FILE;
    memset(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,0x00,60);
    memcpy(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,remoteFile,strlen(remoteFile));
    if(-1 == WifiFile_Request(RequestSendInfo,RequestRecvInfo))	//获取请求失败
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        AT_CIPCLOSE('2');
        WiFiFileDownload_Status = WiFiFileNoConnect;
        return -2;
    }else	//获取请求成功
    {
        if(RequestRecvInfo->RecvData.recvDownloadInfo.downloadStatus == EN_DOWNLOAD_FILE_NOT_EXIST)
        {	//文件不存在
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            WiFiFileDownload_Status = WiFiFileNoConnect;
            return -3;
        }else
        {	//文件存在
            StDownloadSendDataSt *DownloadSendInfo = NULL;
            StDownloadRecvDataSt *DownloadRecvInfo = NULL;
            int     handle;
            DownloadSendInfo = (StDownloadSendDataSt *)malloc(sizeof(StDownloadSendDataSt));
            DownloadRecvInfo = (StDownloadRecvDataSt *)malloc(sizeof(StDownloadRecvDataSt));
            memset(DownloadSendInfo->Path,0x00,60);
            memcpy(DownloadSendInfo->Path,remoteFile,strlen(remoteFile));
            DownloadSendInfo->Serial_Num = 1;
            //创建文件
            handle = open(localFile,  O_WRONLY | O_CREAT | O_TRUNC, 0);
            if ( handle == -1 )//打开文件失败
            {
                unlink(localFile);
                free(DownloadSendInfo);
                free(DownloadRecvInfo);
                free(RequestSendInfo);
                free(RequestRecvInfo);
                AT_CIPCLOSE('2');
                WiFiFileDownload_Status = WiFiFileNoConnect;
                return -4;
            }
            WiFiFileDownload_Status = WiFiFileConnect;
            //打开文件成功
            while(1)
            {
                //循环要数据，直到没有为止
                if(-1 == WifiFile_Download(DownloadSendInfo,DownloadRecvInfo))
                {//要数据失败
                    close(handle);
                    unlink(localFile);
                    free(DownloadSendInfo);
                    free(DownloadRecvInfo);
                    free(RequestSendInfo);
                    free(RequestRecvInfo);
                    AT_CIPCLOSE('2');
                    WiFiFileDownload_Status = WiFiFileNoConnect;
                    return -5;
                }else
                {//要数据成功
                    //对比校验，成功写入文件
                    if(DownloadRecvInfo->Flag == 1)	//下载异常
                    {
                        close(handle);
                        unlink(localFile);
                        free(DownloadSendInfo);
                        free(DownloadRecvInfo);
                        free(RequestSendInfo);
                        free(RequestRecvInfo);
                        AT_CIPCLOSE('2');
                        WiFiFileDownload_Status = WiFiFileNoConnect;
                        return -6;
                    }else
                    {
                        if(DownloadRecvInfo->Data_Len > 0)
                        {
                            if(DownloadRecvInfo->Check == GetCrc_16(DownloadRecvInfo->Data,DownloadRecvInfo->Data_Len,0,CRC_table_16))
                            {
                                if(DownloadRecvInfo->Data_Len != write( handle, DownloadRecvInfo->Data,DownloadRecvInfo->Data_Len))
                                {	//文件写入失败
                                    close(handle);
                                    unlink(localFile);
                                    free(DownloadSendInfo);
                                    free(DownloadRecvInfo);
                                    free(RequestSendInfo);
                                    free(RequestRecvInfo);
                                    AT_CIPCLOSE('2');
                                    WiFiFileDownload_Status = WiFiFileNoConnect;
                                    return -7;
                                }
                            }else
                            {
                                close(handle);
                                unlink(localFile);
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return -8;
                            }
                        }
                    }
                    //判断是否还有下一帧
                    if(DownloadRecvInfo->Next_Flag == 1)
                    {
                        DownloadSendInfo->Serial_Num++;
                    }else
                    {	//不存在下一帧，文件下载完毕
                        close(handle);
                        //计算全文检验，与开始的校验对比，正确表示下载成功
                        if(getFileAttribute(localFile,&FileSize,&Check) == 0)
                        {
                            printf("%d:FileSize:%d %d Check:%x %x\n",DownloadSendInfo->Serial_Num,FileSize,RequestRecvInfo->RecvData.recvDownloadInfo.File_Size,Check,RequestRecvInfo->RecvData.recvDownloadInfo.Check);
                            if((FileSize == RequestRecvInfo->RecvData.recvDownloadInfo.File_Size)&&(Check == RequestRecvInfo->RecvData.recvDownloadInfo.Check))
                            {
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return 0;
                            }
                            else
                            {
                                unlink(localFile);
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return -10;	//文件或校验失败
                            }

                        }else
                        {
                            unlink(localFile);
                            free(DownloadSendInfo);
                            free(DownloadRecvInfo);
                            free(RequestSendInfo);
                            free(RequestRecvInfo);
                            AT_CIPCLOSE('2');
                            WiFiFileDownload_Status = WiFiFileNoConnect;
                            return -9;	//打开本地文件失败
                        }

                    }
                }
            }
        }
    }
}



//下载文件
//返回值:
//    0:文件下载成功
//  -1:创建连接失败
//  -2:发送文件下载请求失败
//  -3:文件不存在
//  -4: 擦除内部Flash APP2区域失败
//  -5:文件下载命令失败
//  -6:文件下载异常
//  -7:本地文件写入失败
//  -8:单条记录校验错误
//  -9:打开本地文件失败
//  -10:本地文件大小或校验失败
int WiFiFile_GetFileInternalFlash(char *remoteFile)
{
    unsigned short Check = 0;
    StRequestSendDataSt *RequestSendInfo = NULL;
    StRequestRecvDataSt *RequestRecvInfo = NULL;
    RequestSendInfo = (StRequestSendDataSt *)malloc(sizeof(StRequestSendDataSt));
    RequestRecvInfo = (StRequestRecvDataSt *)malloc(sizeof(StRequestRecvDataSt));
    //问询远程文件是否存在
    if(-1 == WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1))
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        WiFiFileDownload_Status = WiFiFileNoConnect;
        return -1;
    }
    //问询文件是否存在
    RequestSendInfo->cmd = EN_CMD_DOWNLOAD_FILE;
    memset(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,0x00,60);
    memcpy(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,remoteFile,strlen(remoteFile));
    if(-1 == WifiFile_Request(RequestSendInfo,RequestRecvInfo))	//获取请求失败
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        AT_CIPCLOSE('2');
        WiFiFileDownload_Status = WiFiFileNoConnect;
        return -2;
    }else	//获取请求成功
    {
        if(RequestRecvInfo->RecvData.recvDownloadInfo.downloadStatus == EN_DOWNLOAD_FILE_NOT_EXIST)
        {	//文件不存在
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            WiFiFileDownload_Status = WiFiFileNoConnect;
            return -3;
        }else
        {	//文件存在
            StDownloadSendDataSt *DownloadSendInfo = NULL;
            StDownloadRecvDataSt *DownloadRecvInfo = NULL;
            unsigned int app2addr = 0x08080000;
            DownloadSendInfo = (StDownloadSendDataSt *)malloc(sizeof(StDownloadSendDataSt));
            DownloadRecvInfo = (StDownloadRecvDataSt *)malloc(sizeof(StDownloadRecvDataSt));
            memset(DownloadSendInfo->Path,0x00,60);
            memcpy(DownloadSendInfo->Path,remoteFile,strlen(remoteFile));
            DownloadSendInfo->Serial_Num = 1;
            //擦除内部Flash APP2空间
            //解锁内部Flash
            FLASH_Unlock();
            if(1 == FLASH_If_Erase_APP2())	//清除APP2区域
            {
                free(DownloadSendInfo);
                free(DownloadRecvInfo);
                free(RequestSendInfo);
                free(RequestRecvInfo);
                AT_CIPCLOSE('2');
                WiFiFileDownload_Status = WiFiFileNoConnect;
                return -4;
            }
            WiFiFileDownload_Status = WiFiFileConnect;
            //打开文件成功
            while(1)
            {
                //循环要数据，直到没有为止
                if(-1 == WifiFile_Download(DownloadSendInfo,DownloadRecvInfo))
                {//要数据失败
                    free(DownloadSendInfo);
                    free(DownloadRecvInfo);
                    free(RequestSendInfo);
                    free(RequestRecvInfo);
                    AT_CIPCLOSE('2');
                    WiFiFileDownload_Status = WiFiFileNoConnect;
                    return -5;
                }else
                {//要数据成功
                    //对比校验，成功写入文件
                    if(DownloadRecvInfo->Flag == 1)	//下载异常
                    {
                        free(DownloadSendInfo);
                        free(DownloadRecvInfo);
                        free(RequestSendInfo);
                        free(RequestRecvInfo);
                        AT_CIPCLOSE('2');
                        WiFiFileDownload_Status = WiFiFileNoConnect;
                        return -6;
                    }else
                    {
                        if(DownloadRecvInfo->Data_Len > 0)
                        {
                            if(DownloadRecvInfo->Check == GetCrc_16(DownloadRecvInfo->Data,DownloadRecvInfo->Data_Len,0,CRC_table_16))
                            {
                                if(DownloadRecvInfo->Data_Len != FLASH_If_WriteData(app2addr,(char *)DownloadRecvInfo->Data,DownloadRecvInfo->Data_Len))
                                {	//文件写入失败
                                    free(DownloadSendInfo);
                                    free(DownloadRecvInfo);
                                    free(RequestSendInfo);
                                    free(RequestRecvInfo);
                                    AT_CIPCLOSE('2');
                                    WiFiFileDownload_Status = WiFiFileNoConnect;
                                    return -7;
                                }
                                app2addr+=DownloadRecvInfo->Data_Len;

                            }else
                            {
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return -8;
                            }
                        }
                    }
                    //判断是否还有下一帧
                    if(DownloadRecvInfo->Next_Flag == 1)
                    {
                        DownloadSendInfo->Serial_Num++;
                    }else
                    {	//不存在下一帧，文件下载完毕
                        
                        //计算全文检验，与开始的校验对比，正确表示下载成功
                        if(getFileAttributeInternalFlash(RequestRecvInfo->RecvData.recvDownloadInfo.File_Size,&Check) == 0)
                        {
                            printf("Check:%x %x\n",Check,RequestRecvInfo->RecvData.recvDownloadInfo.Check);
                            if((Check == RequestRecvInfo->RecvData.recvDownloadInfo.Check))
                            {
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return 0;
                            }
                            else
                            {
                                free(DownloadSendInfo);
                                free(DownloadRecvInfo);
                                free(RequestSendInfo);
                                free(RequestRecvInfo);
                                AT_CIPCLOSE('2');
                                WiFiFileDownload_Status = WiFiFileNoConnect;
                                return -10;	//文件或校验失败
                            }

                        }else
                        {
                            free(DownloadSendInfo);
                            free(DownloadRecvInfo);
                            free(RequestSendInfo);
                            free(RequestRecvInfo);
                            AT_CIPCLOSE('2');
                            WiFiFileDownload_Status = WiFiFileNoConnect;
                            return -9;	//打开本地文件失败
                        }

                    }
                }
            }
        }
    }
}

//删除文件
//返回值:
//    0:文件删除成功
//  -1:创建连接失败
//  -2:发送文件删除请求失败
//  -3:文件不存在
//  -4: 文件删除失败
int WiFiFile_DeleteFile(char *remoteFile)
{
    StRequestSendDataSt *RequestSendInfo = NULL;
    StRequestRecvDataSt *RequestRecvInfo = NULL;
    RequestSendInfo = (StRequestSendDataSt *)malloc(sizeof(StRequestSendDataSt));
    RequestRecvInfo = (StRequestRecvDataSt *)malloc(sizeof(StRequestRecvDataSt));
    //问询远程文件是否存在
    if(-1 == WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1))
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        return -1;
    }
    //问询文件是否存在
    RequestSendInfo->cmd = EN_CMD_DELETE_FILE;
    memset(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,0x00,60);
    memcpy(RequestSendInfo->SendData.senddDownloadDeleteInfo.Path,remoteFile,strlen(remoteFile));
    if(-1 == WifiFile_Request(RequestSendInfo,RequestRecvInfo))	//获取请求失败
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        AT_CIPCLOSE('2');
        return -2;
    }else	//获取请求成功
    {
        if(RequestRecvInfo->RecvData.recvDeleteInfo == EN_DELETE_FILE_NOT_EXIST)
        {	//文件不存在
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            return -3;
        }else if(RequestRecvInfo->RecvData.recvDeleteInfo == EN_DELETE_FILE_FAILED)
        {	//文件删除失败
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            return -4;
        }else
        {	//文件删除成功
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            return 0;
        }
    }
}
//上传文件
//返回值:
//    0:文件删除成功
//  -1:创建连接失败
//  -2:发送上传文件请求失败
//  -3:本地文件不存在
//  -4:远程下载路径不存在
//  -5:上报数据失败
//  -6:上传回应异常
int WiFiFile_PutFile(char *remoteFile,char *localFile)
{
    unsigned int FileSize = 0;
    unsigned short Check = 0;
    StRequestSendDataSt *RequestSendInfo = NULL;
    StRequestRecvDataSt *RequestRecvInfo = NULL;

    if(getFileAttribute(localFile,&FileSize,&Check) != 0)
    {	//文件不存在
        return -3;
    }

    RequestSendInfo = (StRequestSendDataSt *)malloc(sizeof(StRequestSendDataSt));
    RequestRecvInfo = (StRequestRecvDataSt *)malloc(sizeof(StRequestRecvDataSt));

    //问询远程文件是否存在
    if(-1 == WiFiSocketCreate(WIFI_SERVER_IP ,WIFI_SERVER_PORT1))
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        return -1;
    }
    //上传文件问询帧
    RequestSendInfo->cmd = EN_CMD_UPLOAD_FILE;
    memset(RequestSendInfo->SendData.sendUploadInfo.Path,0x00,60);
    memcpy(RequestSendInfo->SendData.sendUploadInfo.Path,remoteFile,strlen(remoteFile));
    RequestSendInfo->SendData.sendUploadInfo.File_Size = FileSize;
    RequestSendInfo->SendData.sendUploadInfo.Check = Check;
    if(-1 == WifiFile_Request(RequestSendInfo,RequestRecvInfo))	//获取请求失败
    {
        free(RequestSendInfo);
        free(RequestRecvInfo);
        AT_CIPCLOSE('2');
        return -2;
    }else	//获取请求成功
    {
        if(RequestRecvInfo->RecvData.recvUploadInfo == EN_UPLOAD_FILE_PATH_NOT_EXIST)
        {	//文件路径不存在
            free(RequestSendInfo);
            free(RequestRecvInfo);
            AT_CIPCLOSE('2');
            return -4;
        }else
        {	//文件路径存在可以上传
            //上传文件
            StUploadSendDataSt *UploadSendInfo = NULL;
            StUploadRecvDataSt *UploadRecvInfo = NULL;
            int handle;
            UploadSendInfo = (StUploadSendDataSt *)malloc(sizeof(StUploadSendDataSt));
            UploadRecvInfo = (StUploadRecvDataSt *)malloc(sizeof(StUploadRecvDataSt));
            memset(UploadSendInfo->Path,0x00,60);
            memcpy(UploadSendInfo->Path,remoteFile,strlen(remoteFile));
            UploadSendInfo->Serial_Num = 0;

            handle = open(localFile, O_RDONLY, 0);
            if ( handle == -1 )//打开文件失败
            {
                free(UploadSendInfo);
                free(UploadRecvInfo);
                free(RequestSendInfo);
                free(RequestRecvInfo);
                AT_CIPCLOSE('2');
                return -4;
            }
            //打开文件成功
            while(1)
            {
                //从文件中读取字节
                UploadSendInfo->Data_Len = read(handle,UploadSendInfo->Data,1024);
                UploadSendInfo->Serial_Num++;
                if(UploadSendInfo->Data_Len < 1024)
                {
                    UploadSendInfo->Next_Flag = 0;
                }else
                {
                    UploadSendInfo->Next_Flag = 1;
                }
                UploadSendInfo->Check = GetCrc_16(UploadSendInfo->Data,UploadSendInfo->Data_Len,0,CRC_table_16);

                if(-1 == WifiFile_Upload(UploadSendInfo,UploadRecvInfo))
                {
                    //要数据失败
                    close(handle);
                    free(UploadSendInfo);
                    free(UploadRecvInfo);
                    free(RequestSendInfo);
                    free(RequestRecvInfo);
                    AT_CIPCLOSE('2');
                    return -5;
                }else
                {
                    if(UploadRecvInfo->Flag == 1)	//上传文件异常
                    {
                        close(handle);
                        free(UploadSendInfo);
                        free(UploadRecvInfo);
                        free(RequestSendInfo);
                        free(RequestRecvInfo);
                        AT_CIPCLOSE('2');
                        return -6;
                    }
                }

                if(UploadSendInfo->Next_Flag == 0)//不存在下一帧，表示文件上传成功
                {
                    close(handle);
                    free(UploadSendInfo);
                    free(UploadRecvInfo);
                    free(RequestSendInfo);
                    free(RequestRecvInfo);
                    AT_CIPCLOSE('2');
                    return 0;
                }

            }
        }
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void w_sc(char *IP ,int port)
{
    WiFiSocketCreate(IP ,port);
}

void w_r(void)
{
    StRequestSendDataSt *SendInfo = NULL;
    StRequestRecvDataSt *RecvInfo = NULL;
    SendInfo = (StRequestSendDataSt *)malloc(sizeof(StRequestSendDataSt));
    RecvInfo = (StRequestRecvDataSt *)malloc(sizeof(StRequestRecvDataSt));
    SendInfo->cmd = EN_CMD_DOWNLOAD_FILE;
    memset(SendInfo->SendData.senddDownloadDeleteInfo.Path,0x00,60);
    memcpy(SendInfo->SendData.senddDownloadDeleteInfo.Path,"/home/data/1",13);
    printf("w_r:%d\n",WifiFile_Request(SendInfo,RecvInfo));
    free(SendInfo);
    free(RecvInfo);
}

void w_d(void)
{
    StDownloadSendDataSt *SendInfo = NULL;
    StDownloadRecvDataSt *RecvInfo = NULL;
    SendInfo = (StDownloadSendDataSt *)malloc(sizeof(StDownloadSendDataSt));
    RecvInfo = (StDownloadRecvDataSt *)malloc(sizeof(StDownloadRecvDataSt));
    memset(SendInfo->Path,0x00,60);
    memcpy(SendInfo->Path,"/test",13);
    SendInfo->Serial_Num = 1;
    printf("w_d:%d\n",WifiFile_Download(SendInfo,RecvInfo));
    free(SendInfo);
    free(RecvInfo);
}


void w_u(void)
{
    StUploadSendDataSt *SendInfo = NULL;
    StUploadRecvDataSt *RecvInfo = NULL;
    SendInfo = (StUploadSendDataSt *)malloc(sizeof(StUploadSendDataSt));
    RecvInfo = (StUploadRecvDataSt *)malloc(sizeof(StUploadRecvDataSt));
    memset(SendInfo->Path,0x00,60);
    memcpy(SendInfo->Path,"/home/data/1",13);
    SendInfo->Serial_Num = 1;
    SendInfo->Next_Flag = 1;
    SendInfo->Data_Len = 1024;
    memset(SendInfo->Data,0x00,1024);
    SendInfo->Check = 0;

    printf("w_u:%d\n",WifiFile_Upload(SendInfo,RecvInfo));
    free(SendInfo);
    free(RecvInfo);
}

void Attribute(char *Path)
{
    unsigned int FileSize = 0;
    unsigned short Check = 0;
    getFileAttribute(Path,&FileSize,&Check);
}

void w_get(char *remoteFile,char *localFile)
{
    WiFiFile_GetFile(remoteFile,localFile);
}
void w_getI(char *remoteFile)
{
    WiFiFile_GetFileInternalFlash(remoteFile);
}


void w_delete(char *remoteFile)
{
    WiFiFile_DeleteFile(remoteFile);
}

void w_put(char *remoteFile,char *localFile)
{
    WiFiFile_PutFile(remoteFile,localFile);
}


FINSH_FUNCTION_EXPORT(w_sc , w_sc.)
FINSH_FUNCTION_EXPORT(w_r , w_r.)
FINSH_FUNCTION_EXPORT(w_d , w_d.)
FINSH_FUNCTION_EXPORT(w_u , w_u.)
FINSH_FUNCTION_EXPORT(Attribute , Attribute.)

FINSH_FUNCTION_EXPORT(w_get , w_get.)
FINSH_FUNCTION_EXPORT(w_getI , w_getI.)
FINSH_FUNCTION_EXPORT(w_delete , w_delete.)
FINSH_FUNCTION_EXPORT(w_put , w_put.)
#endif
