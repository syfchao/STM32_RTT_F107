#include "InternalFlash.h"
#include <rtthread.h>
#include <stdio.h>
#include "stm32f10x.h"
#include <unistd.h>
#include "rtthread.h"

#define ECUINFOADDR_BASE    0x08004000
#define ECUINFOADDR_BASE2  0x080D8000
//²Á³ýÃüÁî
int ErasePage(eInternalFlashType type)
{
    if((type >= INTERNAL_FLASH_MAX))
        return -1;
    FLASH_Unlock();
    if((type <= INTERNAL_FLASH_CONFIG_INFO))
    {
        while(FLASH_COMPLETE != FLASH_ErasePage(ECUINFOADDR_BASE+0x800*type))
        {}
    }else if((type > INTERNAL_FLASH_CONFIG_INFO) &&(type < INTERNAL_FLASH_MAX))
    {
        while(FLASH_COMPLETE != FLASH_ErasePage(ECUINFOADDR_BASE2+0x1000*(type-INTERNAL_FLASH_A118)))
        {}
    }

    FLASH_Lock();
    return 0;
}

//Ð´ÈëÃüÁî
int WritePage(eInternalFlashType type,char *Data,int Length)
{
    unsigned int addr = 0;
    int i = 0;
    unsigned short sData = 0;
    if(0 == ErasePage(type))
    {
        if( (type <= INTERNAL_FLASH_CONFIG_INFO))
        {
            addr = ECUINFOADDR_BASE + 0x800*type;
        }else if((type > INTERNAL_FLASH_CONFIG_INFO) &&(type < INTERNAL_FLASH_MAX))
        {
           addr = ECUINFOADDR_BASE2 + 0x1000*(type-INTERNAL_FLASH_A118);
        }else
        {
            return -4;
        }
        FLASH_Unlock();

        for(i = 0;i < Length/2+(Length%2);i++)
        {
            sData = Data[i*2]*0x100 + Data[i*2+1];
            if(FLASH_ProgramHalfWord(addr, sData) == FLASH_COMPLETE)
            {
                if ((*(uint16_t *)(addr)) != (uint16_t)sData)
                {
                    /* Flash content doesn't match SRAM content */
                    return(-3);
                }
            }
            else
            {
                return (-2);
            }
            addr += 2;
        }
        FLASH_Lock();
        return 0;
    }else
    {
        return -1;
    }

}
//¶ÁÈ¡ÃüÁî
int ReadPage(eInternalFlashType type,char *Data,int Length)
{
    unsigned int addr = 0;
    int i = 0;
    unsigned short sData = 0;
    if( (type <= INTERNAL_FLASH_CONFIG_INFO))
    {
        addr = ECUINFOADDR_BASE+0x800*type;
    }else if((type > INTERNAL_FLASH_CONFIG_INFO) &&(type < INTERNAL_FLASH_MAX))
    {
        addr = ECUINFOADDR_BASE2 + 0x1000*(type-INTERNAL_FLASH_A118);
    }else
    {
        return -1;
    }

    for(i = 0;i < Length/2+(Length%2);i++)
    {
        sData = (*(uint16_t *)(addr));
        Data[i*2] = sData/0x100;
        Data[i*2 + 1] = sData%0x100;
        addr+=2;
    }
    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
void ReadP(eInternalFlashType type,int Length)
{
    char *Data = NULL;
    Data = malloc(Length+1);
    memset(Data,0x00,Length);
	
    ReadPage(type,Data,Length);
    Data[Length] = '\0';
    printf("%s\n",Data);
    free(Data);
    Data = NULL;
    

}

FINSH_FUNCTION_EXPORT(ErasePage, eg:ErasePage());
FINSH_FUNCTION_EXPORT(WritePage, eg:WritePage());
FINSH_FUNCTION_EXPORT(ReadP, eg:ReadP());
#endif
