#include "modbus_storage.h"
#include "modbus_config.h"
#include "modbus_crc.h"
#include "stm32f10x_flash.h"
#include <string.h>

#define MODBUS_STORAGE_MAGIC_LOW   ((uint16_t)0x5254U)
#define MODBUS_STORAGE_MAGIC_HIGH  ((uint16_t)0x4D42U)
#define MODBUS_STORAGE_VERSION     ((uint16_t)0x0001U)

typedef struct
{
    uint16_t magic_low;
    uint16_t magic_high;
    uint16_t version;
    uint16_t slave_address;
    uint16_t baudrate_low;
    uint16_t baudrate_high;
    uint16_t serial_format;
    uint16_t crc;
} ModbusStoredRecord_t;

static const uint32_t s_supported_baudrates[] ={1200U, 2400U, 4800U, 9600U, 19200U, 38400U, 57600U, 115200U};

void ModbusStorage_GetDefaults(ModbusSerialConfig_t *config)
{
    if (config == 0)
    {
        return;
    }
    config->slave_address = MODBUS_DEFAULT_SLAVE_ADDRESS;
    config->baudrate = MODBUS_DEFAULT_BAUDRATE;
    config->parity = MODBUS_DEFAULT_PARITY;
    config->stop_bits = MODBUS_DEFAULT_STOP_BITS;
}

bool ModbusStorage_BaudrateToCode(uint32_t baudrate, uint16_t *code)
{
    uint16_t index;

    if (code == 0)
    {
        return false;
    }
    for (index = 0U; index < (uint16_t)(sizeof(s_supported_baudrates) / sizeof(s_supported_baudrates[0])); ++index)
    {
        if (s_supported_baudrates[index] == baudrate)
        {
            *code = index;
            return true;
        }
    }
    return false;
}

bool ModbusStorage_CodeToBaudrate(uint16_t code, uint32_t *baudrate)
{
    if ((baudrate == 0) || (code >= (uint16_t)(sizeof(s_supported_baudrates) / sizeof(s_supported_baudrates[0]))))
    {
        return false;
    }
    *baudrate = s_supported_baudrates[code];
    return true;
}

bool ModbusStorage_IsConfigValid(const ModbusSerialConfig_t *config)
{
    uint16_t baud_code;

    if (config == 0)
    {
        return false;
    }
    if ((config->slave_address == 0U) || (config->slave_address > 247U))
    {
        return false;
    }
    if (!ModbusStorage_BaudrateToCode(config->baudrate, &baud_code))
    {
        return false;
    }
    if (config->parity > MODBUS_PARITY_ODD)
    {
        return false;
    }
    if ((config->stop_bits != 1U) && (config->stop_bits != 2U))
    {
        return false;
    }
    return true;
}

bool ModbusStorage_Load(ModbusSerialConfig_t *config)
{
    const ModbusStoredRecord_t *stored = (const ModbusStoredRecord_t *)MODBUS_CONFIG_FLASH_PAGE_ADDRESS;
    ModbusStoredRecord_t record;
    uint16_t expected_crc;

    if (config == 0)
    {
        return false;
    }

    memcpy(&record, stored, sizeof(record));
    expected_crc = Modbus_CRC16((const uint8_t *)&record, (uint16_t)(sizeof(record) - sizeof(record.crc)));
    if ((record.magic_low != MODBUS_STORAGE_MAGIC_LOW) ||
        (record.magic_high != MODBUS_STORAGE_MAGIC_HIGH) ||
        (record.version != MODBUS_STORAGE_VERSION) ||
        (record.crc != expected_crc))
    {
        return false;
    }

    config->slave_address = (uint8_t)record.slave_address;
    config->baudrate = ((uint32_t)record.baudrate_high << 16U) | record.baudrate_low;
    config->parity = (ModbusParity_t)(record.serial_format & 0x00FFU);
    config->stop_bits = (uint8_t)((record.serial_format >> 8U) & 0x00FFU);
    return ModbusStorage_IsConfigValid(config);
}

bool ModbusStorage_Save(const ModbusSerialConfig_t *config)
{
    ModbusStoredRecord_t record;
    uint16_t words[sizeof(ModbusStoredRecord_t) / sizeof(uint16_t)];
    uint16_t index;
    FLASH_Status status;

    if (!ModbusStorage_IsConfigValid(config))
    {
        return false;
    }

    record.magic_low = MODBUS_STORAGE_MAGIC_LOW;
    record.magic_high = MODBUS_STORAGE_MAGIC_HIGH;
    record.version = MODBUS_STORAGE_VERSION;
    record.slave_address = config->slave_address;
    record.baudrate_low = (uint16_t)(config->baudrate & 0xFFFFU);
    record.baudrate_high = (uint16_t)(config->baudrate >> 16U);
    record.serial_format = (uint16_t)(((uint16_t)config->stop_bits << 8U) | (uint16_t)config->parity);
    record.crc = Modbus_CRC16((const uint8_t *)&record, (uint16_t)(sizeof(record) - sizeof(record.crc)));

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    status = FLASH_ErasePage(MODBUS_CONFIG_FLASH_PAGE_ADDRESS);
    if (status != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return false;
    }

    memcpy(words, &record, sizeof(words));
    for (index = 0U; index < (uint16_t)(sizeof(record) / sizeof(uint16_t)); ++index)
    {
        status = FLASH_ProgramHalfWord(MODBUS_CONFIG_FLASH_PAGE_ADDRESS + ((uint32_t)index * 2U), words[index]);
        if (status != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return false;
        }
    }
    FLASH_Lock();

    return (memcmp((const void *)MODBUS_CONFIG_FLASH_PAGE_ADDRESS, &record, sizeof(record)) == 0);
}
