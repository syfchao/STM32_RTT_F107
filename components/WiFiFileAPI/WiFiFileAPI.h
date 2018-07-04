#ifndef __WIFIFILEAPI_H__
#define __WIFIFILEAPI_H__

typedef enum 
{
    EN_CMD_DOWNLOAD_FILE  = 1,
    EN_CMD_DELETE_FILE    = 2,
    EN_CMD_UPLOAD_FILE    = 3
} eRequestCMD; // Request command	

typedef enum 
{
    EN_DOWNLOAD_FILE_NOT_EXIST  = 0,
    EN_DOWNLOAD_FILE_EXIST      = 1,
} eDownloadStatus; // Request command	

typedef enum 
{
    EN_DELETE_FILE_NOT_EXIST  = 0,
    EN_DELETE_FILE_SUCCESS    = 1,
    EN_DELETE_FILE_FAILED     = 2
} eDeleteStatus; // Request command	


typedef enum 
{
    EN_UPLOAD_FILE_PATH_NOT_EXIST  = 0,
    EN_UPLOAD_FILE_PATH_EXIST    = 1,
} eUploadStatus; // Request command	

//���ͽṹ��
typedef struct
{
    char  Path[60];          // PATH
} StSendDownloadDeleteInfo;

typedef struct
{
    char  Path[60];          // PATH
    unsigned int File_Size;  //�ļ���С
    unsigned short Check;	//У��
} StSendUploadInfo;

//���սṹ��
typedef struct
{
    eDownloadStatus downloadStatus;
    unsigned int File_Size;  //�ļ���С
    unsigned short Check;	//У��
} StRecvDownloadInfo;

//01�����͡����սṹ��
//���ͽṹ��
typedef struct
{
    eRequestCMD cmd;

    union UnRequestSendData
    {
        StSendDownloadDeleteInfo senddDownloadDeleteInfo;
        StSendUploadInfo sendUploadInfo;
    } SendData;

} StRequestSendDataSt;

//���սṹ��
typedef struct
{
    eRequestCMD cmd;
    union UnRequestRecvData
    {
        StRecvDownloadInfo recvDownloadInfo;
        eDeleteStatus recvDeleteInfo;
        eUploadStatus recvUploadInfo;
    } RecvData;
} StRequestRecvDataSt;

//02 ���ؽṹ��
//���ͽṹ��
typedef struct
{
    char  Path[60];          // PATH
    unsigned int Serial_Num;  //�������к�
} StDownloadSendDataSt;

//���սṹ��
typedef struct
{
    unsigned char Flag;
    unsigned char Next_Flag;
    unsigned short Data_Len;
    unsigned char Data[1024];
    unsigned short Check;	//У��
} StDownloadRecvDataSt;

//03 �ϴ��ṹ��
//���ͽṹ��
typedef struct
{
    char  Path[60];          // PATH
    unsigned int Serial_Num;  //�������к�
    unsigned char Next_Flag;
    unsigned short Data_Len;
    unsigned char Data[1024];
    unsigned short Check;	//У��
} StUploadSendDataSt;
//���սṹ��
typedef struct
{
    unsigned char Flag;
} StUploadRecvDataSt;

unsigned short GetCrc_16(unsigned char * pData, unsigned short nLength, unsigned short init, const unsigned short *ptable);
int WiFiFile_GetFile(char *remoteFile,char *localFile);
int WiFiFile_GetFileInternalFlash(char *remoteFile);
int WiFiFile_DeleteFile(char *remoteFile);
int WiFiFile_PutFile(char *remoteFile,char *localFile);

#endif /*__WIFIFILEAPI_H__*/
