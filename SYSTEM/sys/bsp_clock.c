#include "bsp_clock.h"
#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

bool BSP_Clock_Init(void)
{
    ErrorStatus hse_status;
    uint32_t timeout;

    if ((uint32_t)HSE_VALUE != 8000000U)
    {
        SystemCoreClockUpdate();
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
        return false;
    }

    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    hse_status = RCC_WaitForHSEStartUp();

    if (hse_status == SUCCESS)
    {
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);

        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);

        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        timeout = 0x000FFFFFU;
        while ((RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) && (timeout > 0U))
        {
            --timeout;
        }
        if (timeout == 0U)
        {
            RCC_DeInit();
            SystemCoreClockUpdate();
            NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
            return false;
        }

        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        timeout = 0x000FFFFFU;
        while ((RCC_GetSYSCLKSource() != 0x08U) && (timeout > 0U))
        {
            --timeout;
        }
        if (timeout == 0U)
        {
            RCC_DeInit();
            SystemCoreClockUpdate();
            NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
            return false;
        }
    }

    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    return (hse_status == SUCCESS);
}
