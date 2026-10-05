#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

#include <stdbool.h>

/* Configures an 8 MHz HSE for a 72 MHz system clock and NVIC priority grouping.
 * If HSE startup fails, the MCU remains on HSI and the drivers still calculate
 * their timing from the actual clock tree. */
bool BSP_Clock_Init(void);

#endif
