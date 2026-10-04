#ifndef MODBUS_TYPES_H
#define MODBUS_TYPES_H

#include <stdint.h>
#include <stdbool.h>



typedef enum
{
	MODBUS_PARITY_NONE = 0,
	MODBUS_PARITY_EVEN = 1,
	MODBUS_PARITY_ODD  = 2
} ModbusParity_t;

typedef struct
{
	uint8_t        slave_address;
	uint32_t       baudrate;
	ModbusParity_t parity;
	uint8_t        stop_bits;
} ModbusSerialConfig_t;


typedef struct
{
    uint32_t received_frames;
    uint32_t valid_frames;
    uint32_t crc_errors;
    uint32_t address_misses;
    uint32_t exceptions_sent;
    uint32_t queue_overflows;
    uint32_t line_errors;
    uint32_t flash_write_errors;
} ModbusStatistics_t;

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




#endif
