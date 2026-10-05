#ifndef MODBUS_STORAGE_H
#define MODBUS_STORAGE_H

#include <stdbool.h>
#include "modbus_type.h"

void ModbusStorage_GetDefaults(ModbusSerialConfig_t *config);
bool ModbusStorage_IsConfigValid(const ModbusSerialConfig_t *config);
bool ModbusStorage_Load(ModbusSerialConfig_t *config);
bool ModbusStorage_Save(const ModbusSerialConfig_t *config);

bool ModbusStorage_BaudrateToCode(uint32_t baudrate, uint16_t *code);
bool ModbusStorage_CodeToBaudrate(uint16_t code, uint32_t *baudrate);

#endif
