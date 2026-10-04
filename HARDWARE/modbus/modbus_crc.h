#ifndef _MODBUS_CRC_
#define _MODBUS_CRC_


#include <stdint.h>


uint16_t Modbus_CRC16(const uint8_t *data, uint16_t length);



#endif
