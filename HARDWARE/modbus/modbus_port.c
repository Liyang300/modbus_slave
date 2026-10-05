#include "modbus_port.h"
#include "BSP_init.h"

#include "modbus_config.h"
#include "stm32f10x_tim.h"

static volatile uint16_t s_rx_events[MODBUS_RX_EVENT_QUEUE_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static volatile bool     s_rx_queue_overflow;
static volatile bool     s_rx_line_error;
static volatile bool     s_receiving_frame;
static volatile uint32_t s_t35_microseconds;



//帧队列索引
static uint16_t ModbusPort_NextQueueIndex(uint16_t index)
{
	++index;
	if(index >= MODBUS_RX_EVENT_QUEUE_SIZE)
	{
		index = 0U;
	}
	return index;
}



//入队
static void ModbusPort_PushRxEventFromISR(uint16_t event)
{
	uint16_t next = ModbusPort_NextQueueIndex(s_rx_head);
	
	if(next ==s_rx_tail)
	{
		s_rx_queue_overflow = true;
		return;
	}
	s_rx_events[s_rx_head] = event;
	s_rx_head = next;
}

//上电初始化
void ModbusPort_Init(const ModbusSerialConfig_t *config)
{
	s_rx_head = 0U;
	s_rx_tail = 0U;
	s_rx_queue_overflow = false;
	s_rx_line_error     = false;
  s_receiving_frame   = false;
	
	BSP_Gpio_Init();
	BSP_Usart_Init(config);
	s_t35_microseconds = BSP_Timer_Init(config);
  BSP_Usart_EnableRxInterrupt(true);
  BSP_Nvic_Init();
} 

//运行时更改
void ModbusPort_Reconfig(const ModbusSerialConfig_t *config)
{
	NVIC_DisableIRQ(MODBUS_USART_IRQn);
	NVIC_DisableIRQ(MODBUS_FRAME_TIMER_IRQn);
	BSP_Timer_Stop();
	BSP_Usart_EnableRxInterrupt(false);
	
	s_rx_head = 0U;
	s_rx_tail = 0U;
	s_rx_queue_overflow = false;
	s_rx_line_error     = false;
  s_receiving_frame   = false;
	
	BSP_Usart_Init(config);
	s_t35_microseconds = BSP_Timer_Init(config);
  BSP_Usart_EnableRxInterrupt(true);
  BSP_Nvic_Init();
	NVIC_ClearPendingIRQ(MODBUS_USART_IRQn);
	NVIC_ClearPendingIRQ(MODBUS_FRAME_TIMER_IRQn);
	
}

void ModbusPort_Send(const uint8_t *data, uint16_t length)
{
	if((length == 0U) || (data == NULL))
	{
		return;
	}
	BSP_Gpio_SetDriverEnable(true);
	BSP_Usart_SendBuffer(data, length);
	BSP_Gpio_SetDriverEnable(false);
}


bool ModbusPort_PopRxEvent(uint16_t *event)
{
	if((event == NULL) || (s_rx_tail == s_rx_head))
	{
		return false;
	}
	*event = s_rx_events[s_rx_tail];
	s_rx_tail = ModbusPort_NextQueueIndex(s_rx_tail);
	
	
	return true;
}

uint8_t ModbusPort_TackRxFault(void)
{
	uint8_t fault = 0;
	NVIC_DisableIRQ(MODBUS_USART_IRQn);
	NVIC_DisableIRQ(MODBUS_FRAME_TIMER_IRQn);
	
	 if (s_rx_queue_overflow)
    {
        fault |= MODBUS_PORT_RX_FAULT_QUEUE_FULL;
    }
    if (s_rx_line_error)
    {
        fault |= MODBUS_PORT_RX_FAULT_LINE_ERROR;
    }
    s_rx_queue_overflow = false;
    s_rx_line_error = false;
    NVIC_EnableIRQ(MODBUS_FRAME_TIMER_IRQn);
    NVIC_EnableIRQ(MODBUS_USART_IRQn);
    return fault;
	
}


void ModbusPort_ResetRxQueue(void)
{
    NVIC_DisableIRQ(MODBUS_USART_IRQn);
    NVIC_DisableIRQ(MODBUS_FRAME_TIMER_IRQn);
    s_rx_tail = s_rx_head;
    s_receiving_frame = false;
    BSP_Timer_Stop();
    NVIC_EnableIRQ(MODBUS_FRAME_TIMER_IRQn);
    NVIC_EnableIRQ(MODBUS_USART_IRQn);
}


uint32_t ModbusPort_GetT35Microseconds(void)
{
    return s_t35_microseconds;
}



void MODBUS_USART_IRQHandler(void)
{
	uint16_t status = MODBUS_USART->SR;
	uint8_t byte;
	
	if ((status & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) != 0U)
    {
        BSP_Usart_FlushReceiver();
        s_rx_line_error = true;
        return;
    }
		
		if((status & USART_SR_RXNE) != 0)
		{
			byte = (uint8_t)MODBUS_USART->DR;
			ModbusPort_PushRxEventFromISR(byte);
			s_receiving_frame = true;
			BSP_Timer_RestartFromISR(s_t35_microseconds);
			
		}
	
}


void MODBUS_FRAME_TIMER_IRQHandler(void)
{
    if (TIM_GetITStatus(MODBUS_FRAME_TIMER, TIM_IT_Update) != RESET)
    {
        BSP_Timer_Stop();
        if (s_receiving_frame)
        {
            ModbusPort_PushRxEventFromISR(MODBUS_PORT_EVENT_FRAME_END);
            s_receiving_frame = false;
        }
    }
}

