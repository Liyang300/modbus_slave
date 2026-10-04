#include "modbus_data.h"
#include "modbus_config.h"
#include <string.h>

static uint8_t s_coils[MODBUS_COIL_COUNT];
static uint8_t s_discrete_inputs[MODBUS_DISCRETE_INPUT_COUNT];
static uint16_t s_holding_registers[MODBUS_HOLDING_REGISTER_COUNT];
static uint16_t s_input_registers[MODBUS_INPUT_REGISTER_COUNT];

void ModbusData_Init(void)
{
    memset(s_coils, 0, sizeof(s_coils));
    memset(s_discrete_inputs, 0, sizeof(s_discrete_inputs));
    memset(s_holding_registers, 0, sizeof(s_holding_registers));
    memset(s_input_registers, 0, sizeof(s_input_registers));

    /* Fixed example values; application code may update them through the APIs. */
    s_coils[0] = 1U;
    s_discrete_inputs[0] = 1U;
    s_holding_registers[0] = 0x1234U;
    s_holding_registers[1] = 0x5678U;
    s_holding_registers[2] = 0x0001U;
    s_input_registers[0] = 250U;      /* Example: 25.0 degrees C. */
    s_input_registers[1] = 500U;      /* Example: 50.0 percent. */
    s_input_registers[2] = 3300U;     /* Example: 3300 mV. */
    s_input_registers[3] = 0xF103U;
}

bool ModbusData_ReadCoil(uint16_t address, bool *value)
{
    if ((address >= MODBUS_COIL_COUNT) || (value == 0))
    {
        return false;
    }
    *value = (s_coils[address] != 0U);
    return true;
}

bool ModbusData_WriteCoil(uint16_t address, bool value)
{
    if (address >= MODBUS_COIL_COUNT)
    {
        return false;
    }
    s_coils[address] = value ? 1U : 0U;
    return true;
}

bool ModbusData_ReadDiscreteInput(uint16_t address, bool *value)
{
    if ((address >= MODBUS_DISCRETE_INPUT_COUNT) || (value == 0))
    {
        return false;
    }
    *value = (s_discrete_inputs[address] != 0U);
    return true;
}

bool ModbusData_ReadHoldingRegister(uint16_t address, uint16_t *value)
{
    if ((address >= MODBUS_HOLDING_REGISTER_COUNT) || (value == 0))
    {
        return false;
    }
    *value = s_holding_registers[address];
    return true;
}

bool ModbusData_WriteHoldingRegister(uint16_t address, uint16_t value)
{
    if (address >= MODBUS_HOLDING_REGISTER_COUNT)
    {
        return false;
    }
    s_holding_registers[address] = value;
    return true;
}

bool ModbusData_ReadInputRegister(uint16_t address, uint16_t *value)
{
    if ((address >= MODBUS_INPUT_REGISTER_COUNT) || (value == 0))
    {
        return false;
    }
    *value = s_input_registers[address];
    return true;
}

bool ModbusData_SetDiscreteInput(uint16_t address, bool value)
{
    if (address >= MODBUS_DISCRETE_INPUT_COUNT)
    {
        return false;
    }
    s_discrete_inputs[address] = value ? 1U : 0U;
    return true;
}

bool ModbusData_SetInputRegister(uint16_t address, uint16_t value)
{
    if (address >= MODBUS_INPUT_REGISTER_COUNT)
    {
        return false;
    }
    s_input_registers[address] = value;
    return true;
}

uint8_t *ModbusData_CoilBuffer(uint16_t *count)
{
    if (count != 0)
    {
        *count = MODBUS_COIL_COUNT;
    }
    return s_coils;
}

uint8_t *ModbusData_DiscreteInputBuffer(uint16_t *count)
{
    if (count != 0)
    {
        *count = MODBUS_DISCRETE_INPUT_COUNT;
    }
    return s_discrete_inputs;
}

uint16_t *ModbusData_HoldingRegisterBuffer(uint16_t *count)
{
    if (count != 0)
    {
        *count = MODBUS_HOLDING_REGISTER_COUNT;
    }
    return s_holding_registers;
}

uint16_t *ModbusData_InputRegisterBuffer(uint16_t *count)
{
    if (count != 0)
    {
        *count = MODBUS_INPUT_REGISTER_COUNT;
    }
    return s_input_registers;
}
