#ifndef MODBUS_DATA_H
#define MODBUS_DATA_H

#include <stdint.h>
#include <stdbool.h>

/* Zero-based example register map initialized by ModbusData_Init(). */
#define MODBUS_SAMPLE_HOLDING_0            ((uint16_t)0x0000U)  /* 0x1234 */
#define MODBUS_SAMPLE_HOLDING_1            ((uint16_t)0x0001U)  /* 0x5678 */
#define MODBUS_SAMPLE_INPUT_TEMPERATURE    ((uint16_t)0x0000U)  /* 250 = 25.0 C */
#define MODBUS_SAMPLE_INPUT_HUMIDITY       ((uint16_t)0x0001U)  /* 500 = 50.0 % */
#define MODBUS_SAMPLE_INPUT_MILLIVOLTS     ((uint16_t)0x0002U)  /* 3300 mV */

void ModbusData_Init(void);

bool ModbusData_ReadCoil(uint16_t address, bool *value);
bool ModbusData_WriteCoil(uint16_t address, bool value);
bool ModbusData_ReadDiscreteInput(uint16_t address, bool *value);
bool ModbusData_ReadHoldingRegister(uint16_t address, uint16_t *value);
bool ModbusData_WriteHoldingRegister(uint16_t address, uint16_t value);
bool ModbusData_ReadInputRegister(uint16_t address, uint16_t *value);

/* Application-facing update interfaces. */
bool ModbusData_SetDiscreteInput(uint16_t address, bool value);
bool ModbusData_SetInputRegister(uint16_t address, uint16_t value);

/* Optional direct buffers for tightly controlled application code. */
uint8_t *ModbusData_CoilBuffer(uint16_t *count);
uint8_t *ModbusData_DiscreteInputBuffer(uint16_t *count);
uint16_t *ModbusData_HoldingRegisterBuffer(uint16_t *count);
uint16_t *ModbusData_InputRegisterBuffer(uint16_t *count);

#endif
