#ifndef _MODBUS_PORT
#define _MODBUS_PORT

#include <stdint.h>
#include <stdbool.h>
#include "modbus_type.h"

#define MODBUS_PORT_EVENT_FRAME_END       ((uint16_t)0x0100U)
#define MODBUS_PORT_RX_FAULT_QUEUE_FULL   ((uint8_t)0x01U)
#define MODBUS_PORT_RX_FAULT_LINE_ERROR   ((uint8_t)0x02U)

void ModbusPort_Init(const ModbusSerialConfig_t *config);
void ModbusPort_Reconfigure(const ModbusSerialConfig_t *config);
void ModbusPort_Send(const uint8_t *data, uint16_t length);

/* Events contain 0x00..0xFF for received bytes and 0x100 for a T3.5 gap. */
bool ModbusPort_PopRxEvent(uint16_t *event);
uint8_t ModbusPort_TakeRxFaults(void);
void ModbusPort_ResetRxQueue(void);
uint32_t ModbusPort_GetT35Microseconds(void);


#endif
