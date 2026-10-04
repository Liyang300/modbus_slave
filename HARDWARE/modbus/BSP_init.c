#include "BSP_init.h"
#include "modbus_config.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"



void BSP_Gpio_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(MODBUS_USART_GPIO_RCC | MODBUS_RS485_DE_RCC | RCC_APB2Periph_AFIO, ENABLE);

    gpio.GPIO_Pin = MODBUS_USART_TX_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(MODBUS_USART_GPIO, &gpio);

    gpio.GPIO_Pin = MODBUS_USART_RX_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(MODBUS_USART_GPIO, &gpio);

    gpio.GPIO_Pin = MODBUS_RS485_DE_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(MODBUS_RS485_DE_GPIO, &gpio);
    BSP_Gpio_SetDriverEnable(false);
}

void BSP_Gpio_SetDriverEnable(bool enabled)
{
#if MODBUS_RS485_DE_ACTIVE_HIGH
    GPIO_WriteBit(MODBUS_RS485_DE_GPIO, MODBUS_RS485_DE_PIN, enabled ? Bit_SET : Bit_RESET);
#else
    GPIO_WriteBit(MODBUS_RS485_DE_GPIO, MODBUS_RS485_DE_PIN, enabled ? Bit_RESET : Bit_SET);
#endif
}


//中断
void BSP_Nvic_Init(void)
{
    NVIC_InitTypeDef nvic;

    /* Identical preemption priority prevents the two queue producers from
     * preempting each other; subpriority only chooses pending-service order. */
    nvic.NVIC_IRQChannel = MODBUS_USART_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = MODBUS_FRAME_TIMER_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}



//定时器---------------------------------------------------------------------------

static uint8_t BSP_Timer_GetBitsPerCharacter(const ModbusSerialConfig_t *config)
{
    uint8_t bits = 1U + 8U + config->stop_bits;

    if (config->parity != MODBUS_PARITY_NONE)
    {
        ++bits;
    }
    return bits;
}

static uint32_t BSP_Timer_CalculateT35Microseconds(const ModbusSerialConfig_t *config)
{
    uint32_t value;

#if MODBUS_DYNAMIC_T35_ALL_BAUDRATES
    uint64_t numerator = (uint64_t)BSP_Timer_GetBitsPerCharacter(config) * 3500000ULL;
    value = (uint32_t)((numerator + config->baudrate - 1U) / config->baudrate);
#else
    if (config->baudrate > 19200U)
    {
        value = 1750U;
    }
    else
    {
        uint64_t numerator = (uint64_t)BSP_Timer_GetBitsPerCharacter(config) * 3500000ULL;
        value = (uint32_t)((numerator + config->baudrate - 1U) / config->baudrate);
    }
#endif

    if (value < 2U)
    {
        value = 2U;
    }
    if (value > 65535U)
    {
        value = 65535U;
    }
    return value;
}



uint32_t BSP_Timer_Init(const ModbusSerialConfig_t *config)
{
    RCC_ClocksTypeDef clocks;
    TIM_TimeBaseInitTypeDef timer;
    uint32_t timer_clock;
    uint32_t prescaler;
    uint32_t t35_microseconds;

    RCC_APB1PeriphClockCmd(MODBUS_FRAME_TIMER_RCC, ENABLE);
    RCC_GetClocksFreq(&clocks);
    timer_clock = clocks.PCLK1_Frequency;
    if (clocks.PCLK1_Frequency != clocks.HCLK_Frequency)
    {
        timer_clock *= 2U;
    }

    prescaler = timer_clock / 1000000U;
    if (prescaler == 0U)
    {
        prescaler = 1U;
    }

    t35_microseconds = BSP_Timer_CalculateT35Microseconds(config);

    TIM_DeInit(MODBUS_FRAME_TIMER);
    timer.TIM_Prescaler = (uint16_t)(prescaler - 1U);
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = (uint16_t)(t35_microseconds - 1U);
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(MODBUS_FRAME_TIMER, &timer);
    TIM_SelectOnePulseMode(MODBUS_FRAME_TIMER, TIM_OPMode_Single);
    TIM_ClearITPendingBit(MODBUS_FRAME_TIMER, TIM_IT_Update);
    TIM_ITConfig(MODBUS_FRAME_TIMER, TIM_IT_Update, ENABLE);

    return t35_microseconds;
}

void BSP_Timer_RestartFromISR(uint32_t t35_microseconds)
{
    TIM_Cmd(MODBUS_FRAME_TIMER, DISABLE);
    TIM_SetCounter(MODBUS_FRAME_TIMER, 0U);
    TIM_SetAutoreload(MODBUS_FRAME_TIMER, (uint16_t)(t35_microseconds - 1U));
    TIM_ClearITPendingBit(MODBUS_FRAME_TIMER, TIM_IT_Update);
    TIM_Cmd(MODBUS_FRAME_TIMER, ENABLE);
}

void BSP_Timer_Stop(void)
{
    TIM_Cmd(MODBUS_FRAME_TIMER, DISABLE);
    TIM_ClearITPendingBit(MODBUS_FRAME_TIMER, TIM_IT_Update);
}



//串口-------------------------------------------------------------------------------
void BSP_Usart_Init(const ModbusSerialConfig_t *config)
{
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(MODBUS_USART_RCC, ENABLE);
    USART_Cmd(MODBUS_USART, DISABLE);

    USART_StructInit(&usart);
    usart.USART_BaudRate = config->baudrate;
    usart.USART_StopBits = (config->stop_bits == 2U) ? USART_StopBits_2 : USART_StopBits_1;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    if (config->parity == MODBUS_PARITY_NONE)
    {
        usart.USART_WordLength = USART_WordLength_8b;
        usart.USART_Parity = USART_Parity_No;
    }
    else
    {
        /* On STM32F1 the parity bit occupies the ninth configured bit. */
        usart.USART_WordLength = USART_WordLength_9b;
        usart.USART_Parity = (config->parity == MODBUS_PARITY_EVEN) ? USART_Parity_Even : USART_Parity_Odd;
    }

    USART_Init(MODBUS_USART, &usart);
    
    USART_Cmd(MODBUS_USART, ENABLE);
		
		BSP_Usart_FlushReceiver();
}

void BSP_Usart_EnableRxInterrupt(bool enabled)
{
    USART_ITConfig(MODBUS_USART, USART_IT_RXNE, enabled ? ENABLE : DISABLE);
}

void BSP_Usart_FlushReceiver(void)
{
    /* Reading SR followed by DR clears RXNE and the line error flags. */
    (void)MODBUS_USART->SR;
    (void)MODBUS_USART->DR;
}

/**
 * @brief  通过 Modbus 串口阻塞发送一段数据
 *
 * 逐字节等待发送数据寄存器为空（TXE）后写入数据，全部写完后等待发送完成
 * 标志（TC），确保数据已完整移出移位寄存器后再返回。
 *
 * @param  data   待发送数据缓冲区首地址，不可为 NULL
 * @param  length 待发送数据字节数，为 0 时不发送
 * @retval 无
 */
void BSP_Usart_SendBuffer(const uint8_t *data, uint16_t length)
{
    uint16_t index;

    if ((data == 0) || (length == 0U))
    {
        return;
    }

    USART_ClearFlag(MODBUS_USART, USART_FLAG_TC);
    for (index = 0U; index < length; ++index)
    {
        while (USART_GetFlagStatus(MODBUS_USART, USART_FLAG_TXE) == RESET)
        {
        }
        USART_SendData(MODBUS_USART, data[index]);
    }
    while (USART_GetFlagStatus(MODBUS_USART, USART_FLAG_TC) == RESET)
    {
    }
}
