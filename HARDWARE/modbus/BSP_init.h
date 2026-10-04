#ifndef _BSP_INIT
#define _BSP_INIT


#include <stdbool.h>
#include <stdint.h>
#include "modbus_type.h"


void BSP_Gpio_Init(void);
void BSP_Gpio_SetDriverEnable(bool enabled);

void BSP_Usart_Init(const ModbusSerialConfig_t *config);
void BSP_Usart_EnableRxInterrupt(bool enabled);
void BSP_Usart_FlushReceiver(void);
void BSP_Usart_SendBuffer(const uint8_t *data, uint16_t length);

uint32_t BSP_Timer_Init(const ModbusSerialConfig_t *config);
void BSP_Timer_RestartFromISR(uint32_t t35_microseconds);
void BSP_Timer_Stop(void);
void BSP_Nvic_Init(void);


#endif
