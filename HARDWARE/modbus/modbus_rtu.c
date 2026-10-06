#include "modbus_rtu.h"
#include "modbus_config.h"
#include "modbus_crc.h"
#include "modbus_data.h"
#include "modbus_port.h"
#include "modbus_storage.h"
#include <string.h>

#define MB_FC_READ_COILS                 0x01U
#define MB_FC_READ_DISCRETE_INPUTS       0x02U
#define MB_FC_READ_HOLDING_REGISTERS     0x03U
#define MB_FC_READ_INPUT_REGISTERS       0x04U
#define MB_FC_WRITE_SINGLE_COIL          0x05U
#define MB_FC_WRITE_SINGLE_REGISTER      0x06U
#define MB_FC_DIAGNOSTICS                0x08U
#define MB_FC_WRITE_MULTIPLE_COILS       0x0FU
#define MB_FC_WRITE_MULTIPLE_REGISTERS   0x10U

#define MB_EX_NONE                       0x00U
#define MB_EX_ILLEGAL_FUNCTION           0x01U
#define MB_EX_ILLEGAL_DATA_ADDRESS       0x02U
#define MB_EX_ILLEGAL_DATA_VALUE         0x03U

static ModbusSerialConfig_t s_config;
static ModbusStatistics_t   s_statistics;
static uint8_t  s_rx_frame[MODBUS_ADU_MAX_LENGTH];
static uint16_t s_rx_length;
static bool     s_rx_too_long;
static uint8_t  s_response[MODBUS_ADU_MAX_LENGTH];

static uint16_t Modbus_GetU16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void Modbus_PutU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)(value & 0x00FFU);
}

static bool Modbus_IsConfigRegister(uint16_t address)
{
    return (address >= MODBUS_CFG_REG_FIRST) && (address <= MODBUS_CFG_REG_LAST);
}

static bool Modbus_ReadHolding(uint16_t address, uint16_t *value)
{
    if (!Modbus_IsConfigRegister(address))
    {
        return ModbusData_ReadHoldingRegister(address, value);
    }

    switch (address)
    {
        case MODBUS_CFG_REG_SLAVE_ADDRESS:
            *value = s_config.slave_address;
            return true;

        case MODBUS_CFG_REG_BAUDRATE_CODE:
            return ModbusStorage_BaudrateToCode(s_config.baudrate, value);

        case MODBUS_CFG_REG_PARITY:
            *value = (uint16_t)s_config.parity;
            return true;

        case MODBUS_CFG_REG_STOP_BITS:
            *value = s_config.stop_bits;
            return true;

        default:
            return false;
    }
}

static bool Modbus_StageConfigRegister(ModbusSerialConfig_t *staged, uint16_t address, uint16_t value)
{
    uint32_t baudrate;

    switch (address)
    {
        case MODBUS_CFG_REG_SLAVE_ADDRESS:
            if ((value == 0U) || (value > 247U))
            {
                return false;
            }
            staged->slave_address = (uint8_t)value;
            return true;

        case MODBUS_CFG_REG_BAUDRATE_CODE:
            if (!ModbusStorage_CodeToBaudrate(value, &baudrate))
            {
                return false;
            }
            staged->baudrate = baudrate;
            return true;

        case MODBUS_CFG_REG_PARITY:
            if (value > (uint16_t)MODBUS_PARITY_ODD)
            {
                return false;
            }
            staged->parity = (ModbusParity_t)value;
            return true;

        case MODBUS_CFG_REG_STOP_BITS:
            if ((value != 1U) && (value != 2U))
            {
                return false;
            }
            staged->stop_bits = (uint8_t)value;
            return true;

        default:
            return false;
    }
}

static void Modbus_SendResponse(uint16_t length_without_crc)
{
    uint16_t crc = Modbus_CRC16(s_response, length_without_crc);

    s_response[length_without_crc] = (uint8_t)(crc & 0x00FFU);
    s_response[length_without_crc + 1U] = (uint8_t)(crc >> 8U);
    ModbusPort_Send(s_response, (uint16_t)(length_without_crc + 2U));
}

static void Modbus_SendException(uint8_t request_address, uint8_t function, uint8_t exception)
{
    s_response[0] = request_address;
    s_response[1] = function | 0x80U;
    s_response[2] = exception;
    ++s_statistics.exceptions_sent;
    Modbus_SendResponse(3U);
}

static uint8_t Modbus_HandleReadBits(const uint8_t *frame, uint16_t length, bool discrete, uint16_t *response_length)
{
    uint16_t start;
    uint16_t quantity;
    uint16_t index;
    uint8_t byte_count;
    bool value;

    if (length != 8U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    start = Modbus_GetU16(&frame[2]);
    quantity = Modbus_GetU16(&frame[4]);
    if ((quantity == 0U) || (quantity > 2000U) || (((uint32_t)start + quantity) > 65536UL))
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    byte_count = (uint8_t)((quantity + 7U) / 8U);
    memset(&s_response[3], 0, byte_count);
    for (index = 0U; index < quantity; ++index)
    {
        bool ok = discrete ?
            ModbusData_ReadDiscreteInput((uint16_t)(start + index), &value) :
            ModbusData_ReadCoil((uint16_t)(start + index), &value);
        if (!ok)
        {
            return MB_EX_ILLEGAL_DATA_ADDRESS;
        }
        if (value)
        {
            s_response[3U + (index / 8U)] |= (uint8_t)(1U << (index % 8U));
        }
    }

    s_response[2] = byte_count;
    *response_length = (uint16_t)(3U + byte_count);
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleReadRegisters(const uint8_t *frame, uint16_t length, bool input_registers, uint16_t *response_length)
{
    uint16_t start;
    uint16_t quantity;
    uint16_t index;
    uint16_t value;
    bool ok;

    if (length != 8U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    start = Modbus_GetU16(&frame[2]);
    quantity = Modbus_GetU16(&frame[4]);
    if ((quantity == 0U) || (quantity > 125U) || (((uint32_t)start + quantity) > 65536UL))
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    for (index = 0U; index < quantity; ++index)
    {
        ok = input_registers ?
            ModbusData_ReadInputRegister((uint16_t)(start + index), &value) :
            Modbus_ReadHolding((uint16_t)(start + index), &value);
        if (!ok)
        {
            return MB_EX_ILLEGAL_DATA_ADDRESS;
        }
        Modbus_PutU16(&s_response[3U + (index * 2U)], value);
    }

    s_response[2] = (uint8_t)(quantity * 2U);
    *response_length = (uint16_t)(3U + (quantity * 2U));
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleWriteSingleCoil(const uint8_t *frame, uint16_t length, uint16_t *response_length)
{
    uint16_t address;
    uint16_t value;

    if (length != 8U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    address = Modbus_GetU16(&frame[2]);
    value = Modbus_GetU16(&frame[4]);
    if ((value != 0x0000U) && (value != 0xFF00U))
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }
    if (!ModbusData_WriteCoil(address, value == 0xFF00U))
    {
        return MB_EX_ILLEGAL_DATA_ADDRESS;
    }

    memcpy(s_response, frame, 6U);
    *response_length = 6U;
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleWriteSingleRegister(const uint8_t *frame,
                                                 uint16_t length,
                                                 uint16_t *response_length,
                                                 ModbusSerialConfig_t *pending_config,
                                                 bool *config_changed)
{
    uint16_t address;
    uint16_t value;
    ModbusSerialConfig_t staged;

    if (length != 8U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    address = Modbus_GetU16(&frame[2]);
    value = Modbus_GetU16(&frame[4]);

    if (Modbus_IsConfigRegister(address))
    {
        staged = s_config;
        if (!Modbus_StageConfigRegister(&staged, address, value) || !ModbusStorage_IsConfigValid(&staged))
        {
            return MB_EX_ILLEGAL_DATA_VALUE;
        }
        *pending_config = staged;
        *config_changed = true;
    }
    else if (!ModbusData_WriteHoldingRegister(address, value))
    {
        return MB_EX_ILLEGAL_DATA_ADDRESS;
    }

    memcpy(s_response, frame, 6U);
    *response_length = 6U;
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleDiagnostics(const uint8_t *frame, uint16_t length, uint16_t *response_length)
{
    if (length != 8U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }
    if (Modbus_GetU16(&frame[2]) != 0x0000U)
    {
        return MB_EX_ILLEGAL_FUNCTION;
    }

    memcpy(s_response, frame, 6U);
    *response_length = 6U;
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleWriteMultipleCoils(const uint8_t *frame, uint16_t length, uint16_t *response_length)
{
    uint16_t start;
    uint16_t quantity;
    uint16_t index;
    uint8_t byte_count;
    bool dummy;

    if (length < 9U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    start = Modbus_GetU16(&frame[2]);
    quantity = Modbus_GetU16(&frame[4]);
    byte_count = frame[6];
    if ((quantity == 0U) || (quantity > 1968U) ||
        (byte_count != (uint8_t)((quantity + 7U) / 8U)) ||
        (length != (uint16_t)(9U + byte_count)) ||
        (((uint32_t)start + quantity) > 65536UL))
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    for (index = 0U; index < quantity; ++index)
    {
        if (!ModbusData_ReadCoil((uint16_t)(start + index), &dummy))
        {
            return MB_EX_ILLEGAL_DATA_ADDRESS;
        }
    }
    for (index = 0U; index < quantity; ++index)
    {
        bool value = (frame[7U + (index / 8U)] & (uint8_t)(1U << (index % 8U))) != 0U;
        (void)ModbusData_WriteCoil((uint16_t)(start + index), value);
    }

    memcpy(s_response, frame, 6U);
    *response_length = 6U;
    return MB_EX_NONE;
}

static uint8_t Modbus_HandleWriteMultipleRegisters(const uint8_t *frame,
                                                    uint16_t length,
                                                    uint16_t *response_length,
                                                    ModbusSerialConfig_t *pending_config,
                                                    bool *config_changed)
{
    uint16_t start;
    uint16_t quantity;
    uint16_t index;
    uint8_t byte_count;
    uint16_t value;
    uint16_t dummy;
    bool config_range;
    ModbusSerialConfig_t staged;

    if (length < 9U)
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    start = Modbus_GetU16(&frame[2]);
    quantity = Modbus_GetU16(&frame[4]);
    byte_count = frame[6];
    if ((quantity == 0U) || (quantity > 123U) ||
        (byte_count != (uint8_t)(quantity * 2U)) ||
        (length != (uint16_t)(9U + byte_count)) ||
        (((uint32_t)start + quantity) > 65536UL))
    {
        return MB_EX_ILLEGAL_DATA_VALUE;
    }

    config_range = Modbus_IsConfigRegister(start) &&
                   Modbus_IsConfigRegister((uint16_t)(start + quantity - 1U));

    if (config_range)
    {
        staged = s_config;
        for (index = 0U; index < quantity; ++index)
        {
            value = Modbus_GetU16(&frame[7U + (index * 2U)]);
            if (!Modbus_StageConfigRegister(&staged, (uint16_t)(start + index), value))
            {
                return MB_EX_ILLEGAL_DATA_VALUE;
            }
        }
        if (!ModbusStorage_IsConfigValid(&staged))
        {
            return MB_EX_ILLEGAL_DATA_VALUE;
        }
        *pending_config = staged;
        *config_changed = true;
    }
    else
    {
        for (index = 0U; index < quantity; ++index)
        {
            if (!ModbusData_ReadHoldingRegister((uint16_t)(start + index), &dummy))
            {
                return MB_EX_ILLEGAL_DATA_ADDRESS;
            }
        }
        for (index = 0U; index < quantity; ++index)
        {
            value = Modbus_GetU16(&frame[7U + (index * 2U)]);
            (void)ModbusData_WriteHoldingRegister((uint16_t)(start + index), value);
        }
    }

    s_response[0] = frame[0];
    s_response[1] = frame[1];
    Modbus_PutU16(&s_response[2], start);
    Modbus_PutU16(&s_response[4], quantity);
    *response_length = 6U;
    return MB_EX_NONE;
}

static void Modbus_ProcessFrame(const uint8_t *frame, uint16_t length)
{
    uint16_t received_crc;
    uint16_t calculated_crc;
    uint16_t response_length = 0U;
    uint8_t exception = MB_EX_NONE;
    uint8_t function;
    bool broadcast;
    bool config_changed = false;
    ModbusSerialConfig_t pending_config = s_config;

    ++s_statistics.received_frames;
    if (length < 4U)
    {
        return;
    }

    received_crc = (uint16_t)(((uint16_t)frame[length - 1U] << 8U) | frame[length - 2U]);
    calculated_crc = Modbus_CRC16(frame, (uint16_t)(length - 2U));
    if (received_crc != calculated_crc)
    {
        ++s_statistics.crc_errors;
        return;
    }

    broadcast = (frame[0] == 0U);
    if (!broadcast && (frame[0] != s_config.slave_address))
    {
        ++s_statistics.address_misses;
        return;
    }

    function = frame[1];
    if (broadcast && ((function == MB_FC_READ_COILS) ||
                      (function == MB_FC_READ_DISCRETE_INPUTS) ||
                      (function == MB_FC_READ_HOLDING_REGISTERS) ||
                      (function == MB_FC_READ_INPUT_REGISTERS) ||
                      (function == MB_FC_DIAGNOSTICS)))
    {
        return;
    }

    ++s_statistics.valid_frames;
    s_response[0] = frame[0];
    s_response[1] = function;

    switch (function)
    {
        case MB_FC_READ_COILS:
            exception = Modbus_HandleReadBits(frame, length, false, &response_length);
            break;

        case MB_FC_READ_DISCRETE_INPUTS:
            exception = Modbus_HandleReadBits(frame, length, true, &response_length);
            break;

        case MB_FC_READ_HOLDING_REGISTERS:
            exception = Modbus_HandleReadRegisters(frame, length, false, &response_length);
            break;

        case MB_FC_READ_INPUT_REGISTERS:
            exception = Modbus_HandleReadRegisters(frame, length, true, &response_length);
            break;

        case MB_FC_WRITE_SINGLE_COIL:
            exception = Modbus_HandleWriteSingleCoil(frame, length, &response_length);
            break;

        case MB_FC_WRITE_SINGLE_REGISTER:
            exception = Modbus_HandleWriteSingleRegister(frame, length, &response_length, &pending_config, &config_changed);
            break;

        case MB_FC_DIAGNOSTICS:
            exception = Modbus_HandleDiagnostics(frame, length, &response_length);
            break;

        case MB_FC_WRITE_MULTIPLE_COILS:
            exception = Modbus_HandleWriteMultipleCoils(frame, length, &response_length);
            break;

        case MB_FC_WRITE_MULTIPLE_REGISTERS:
            exception = Modbus_HandleWriteMultipleRegisters(frame, length, &response_length, &pending_config, &config_changed);
            break;

        default:
            exception = MB_EX_ILLEGAL_FUNCTION;
            break;
    }

    if (!broadcast)
    {
        if (exception == MB_EX_NONE)
        {
            Modbus_SendResponse(response_length);
        }
        else
        {
            Modbus_SendException(frame[0], function, exception);
        }
    }

    /* The response is transmitted using the old address and serial settings.
     * Flash programming is intentionally delayed until TC, avoiding a long
     * pre-response CPU stall. The new settings activate only after save. */
    if ((exception == MB_EX_NONE) && config_changed)
    {
        if (ModbusStorage_Save(&pending_config))
        {
            s_config = pending_config;
            ModbusPort_Reconfig(&s_config);
        }
        else
        {
            ++s_statistics.flash_write_errors;
        }
    }
}

void Modbus_Init(void)
{
    memset(&s_statistics, 0, sizeof(s_statistics));
    s_rx_length = 0U;
    s_rx_too_long = false;
    ModbusData_Init();

    if (!ModbusStorage_Load(&s_config))
    {
        ModbusStorage_GetDefaults(&s_config);
        (void)ModbusStorage_Save(&s_config);
    }
    ModbusPort_Init(&s_config);
}

void Modbus_Poll(void)
{
    uint16_t event;
    uint8_t rx_faults = ModbusPort_TakeRxFault();

    if (rx_faults != 0U)
    {
        if ((rx_faults & MODBUS_PORT_RX_FAULT_QUEUE_FULL) != 0U)
        {
            ++s_statistics.queue_overflows;
        }
        if ((rx_faults & MODBUS_PORT_RX_FAULT_LINE_ERROR) != 0U)
        {
            ++s_statistics.line_errors;
        }
        s_rx_length = 0U;
        s_rx_too_long = false;
        ModbusPort_ResetRxQueue();
        return;
    }

    while (ModbusPort_PopRxEvent(&event))
    {
        if (event == MODBUS_PORT_EVENT_FRAME_END)
        {
            if ((s_rx_length > 0U) && !s_rx_too_long)
            {
                Modbus_ProcessFrame(s_rx_frame, s_rx_length);
            }
            s_rx_length = 0U;
            s_rx_too_long = false;
        }
        else if (event <= 0x00FFU)
        {
            if (s_rx_length < MODBUS_ADU_MAX_LENGTH)
            {
                s_rx_frame[s_rx_length++] = (uint8_t)event;
            }
            else
            {
                s_rx_too_long = true;
            }
        }
    }
}

const ModbusSerialConfig_t *Modbus_GetSerialConfig(void)
{
    return &s_config;
}

const ModbusStatistics_t *Modbus_GetStatistics(void)
{
    return &s_statistics;
}

bool Modbus_SetSerialConfig(const ModbusSerialConfig_t *config, bool persist)
{
    if (!ModbusStorage_IsConfigValid(config))
    {
        return false;
    }
    if (persist && !ModbusStorage_Save(config))
    {
        return false;
    }

    s_config = *config;
    ModbusPort_Reconfig(&s_config);
    return true;
}
