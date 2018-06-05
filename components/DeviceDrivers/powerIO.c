#include <rtthread.h>
#include <stm32f10x.h>

#define powerio_rcc                    RCC_APB2Periph_GPIOC
#define powerio_gpio                   GPIOC
#define powerio_pin                    (GPIO_Pin_13)

#define ethio_rcc                    RCC_APB2Periph_GPIOA
#define ethio_gpio                   GPIOA
#define ethio_pin                    (GPIO_Pin_6)

#define DRM5_rcc                    RCC_APB2Periph_GPIOA
#define DRM5_gpio                   GPIOA
#define DRM5_pin                    (GPIO_Pin_0)

#define DRM6_rcc                    RCC_APB2Periph_GPIOA
#define DRM6_gpio                   GPIOA
#define DRM6_pin                    (GPIO_Pin_4)

#define DRM7_rcc                    RCC_APB2Periph_GPIOB
#define DRM7_gpio                   GPIOB
#define DRM7_pin                    (GPIO_Pin_0)

#define DRM8_rcc                    RCC_APB2Periph_GPIOB
#define DRM8_gpio                   GPIOB
#define DRM8_pin                    (GPIO_Pin_1)


void rt_hw_powerIO_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(powerio_rcc,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = powerio_pin;
    GPIO_Init(powerio_gpio, &GPIO_InitStructure);
}

void rt_hw_powerIO_on(void)
{
    GPIO_SetBits(powerio_gpio, powerio_pin);
}

void rt_hw_powerIO_off(void)
{
    GPIO_ResetBits(powerio_gpio, powerio_pin);
}


void rt_hw_ETHIO_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(ethio_rcc,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = ethio_pin;
    GPIO_Init(ethio_gpio, &GPIO_InitStructure);


}

int rt_hw_ETHIO_status(void)
{
    return GPIO_ReadInputDataBit(ethio_gpio,ethio_pin);
}

void rt_hw_DRM5678_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(DRM5_rcc,ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = DRM5_pin;
    GPIO_Init(DRM5_gpio, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(DRM6_rcc,ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = DRM6_pin;
    GPIO_Init(DRM6_gpio, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(DRM7_rcc,ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = DRM7_pin;
    GPIO_Init(DRM7_gpio, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(DRM8_rcc,ENABLE);
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = DRM8_pin;
    GPIO_Init(DRM8_gpio, &GPIO_InitStructure);
}


int rt_hw_DRM5_status(void)
{
    return GPIO_ReadInputDataBit(DRM5_gpio,DRM5_pin);
}

int rt_hw_DRM6_status(void)
{
    return GPIO_ReadInputDataBit(DRM6_gpio,DRM6_pin);
}

int rt_hw_DRM7_status(void)
{
    return GPIO_ReadInputDataBit(DRM7_gpio,DRM7_pin);
}

int rt_hw_DRM8_status(void)
{
    return GPIO_ReadInputDataBit(DRM8_gpio,DRM8_pin);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void power(rt_uint32_t value)
{
    rt_hw_powerIO_init();

    /* set led status */
    switch (value)
    {
    case 0:
        rt_hw_powerIO_off();
        break;
    case 1:
        rt_hw_powerIO_on();
        break;
    default:
        break;
    }

}
void ethio(void)
{
    rt_hw_ETHIO_init();
    rt_kprintf("ethio:%d\n",rt_hw_ETHIO_status());
}

void DRM_init(void)
{
	rt_hw_DRM5678_init();
}

void DRM5(void)
{

    rt_kprintf("DRM5:%d\n",rt_hw_DRM5_status());
}

void DRM6(void)
{

    rt_kprintf("DRM6:%d\n",rt_hw_DRM6_status());
}

void DRM7(void)
{

    rt_kprintf("DRM7:%d\n",rt_hw_DRM7_status());
}

void DRM8(void)
{

    rt_kprintf("DRM8:%d\n",rt_hw_DRM8_status());
}

FINSH_FUNCTION_EXPORT(power, set power on[1] or off[0].)
FINSH_FUNCTION_EXPORT(ethio, ethio.)

FINSH_FUNCTION_EXPORT(DRM_init, DRM_init.)
FINSH_FUNCTION_EXPORT(DRM5, DRM5.)
FINSH_FUNCTION_EXPORT(DRM6, DRM6.)
FINSH_FUNCTION_EXPORT(DRM7, DRM7.)
FINSH_FUNCTION_EXPORT(DRM8, DRM8.)
#endif
