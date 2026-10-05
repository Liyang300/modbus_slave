#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdbool.h>
#include "modbus_type.h"

void Modbus_Init(void);
void Modbus_Poll(void);

const ModbusSerialConfig_t *Modbus_GetSerialConfig(void);
const ModbusStatistics_t *Modbus_GetStatistics(void);


bool Modbus_SetSerialConfig(const ModbusSerialConfig_t *config, bool persist);

#endif
