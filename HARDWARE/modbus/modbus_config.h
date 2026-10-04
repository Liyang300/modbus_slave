#ifndef MODBUS_CONFIG_H
#define MODBUS_CONFIG_H

#include <stddef.h>    // NULL 宏定义
#include "stm32f10x.h"
#include "modbus_type.h"


#if !defined(STM32F10X_HD)
#error "STM32F103RCT6 requires the STM32F10X_HD device define"
#endif

//flash地址
#define MODBUS_CONFIG_FLASH_PAGE_ADDRESS   ((uint32_t)0x0803F800U)
#define MODBUS_CONFIG_FLASH_PAGE_SIZE      ((uint32_t)0x00000800U)

//串口配置保存
#define MODBUS_DEFAULT_SLAVE_ADDRESS       ((uint8_t)1U)        //本机地址
#define MODBUS_DEFAULT_BAUDRATE            ((uint32_t)9600U)
#define MODBUS_DEFAULT_PARITY              MODBUS_PARITY_NONE
#define MODBUS_DEFAULT_STOP_BITS           ((uint8_t)1U)

//接收缓冲区
#define MODBUS_ADU_MAX_LENGTH              256U      //帧队列缓冲区
#define MODBUS_RX_EVENT_QUEUE_SIZE         1024U     //字节缓冲区

//3.5字符是否计算所有波特率区间
#define MODBUS_DYNAMIC_T35_ALL_BAUDRATES   1U

//寄存器数量
#define MODBUS_COIL_COUNT                  64U
#define MODBUS_DISCRETE_INPUT_COUNT        64U
#define MODBUS_HOLDING_REGISTER_COUNT      64U
#define MODBUS_INPUT_REGISTER_COUNT        64U

//虚拟寄存器，用于更改串口配置
#define MODBUS_CFG_REG_SLAVE_ADDRESS       ((uint16_t)0xFF00U)
#define MODBUS_CFG_REG_BAUDRATE_CODE       ((uint16_t)0xFF01U)
#define MODBUS_CFG_REG_PARITY              ((uint16_t)0xFF02U)
#define MODBUS_CFG_REG_STOP_BITS           ((uint16_t)0xFF03U)
#define MODBUS_CFG_REG_FIRST               MODBUS_CFG_REG_SLAVE_ADDRESS
#define MODBUS_CFG_REG_LAST                MODBUS_CFG_REG_STOP_BITS

//波特率索引
#define MODBUS_BAUD_CODE_1200              0U
#define MODBUS_BAUD_CODE_2400              1U
#define MODBUS_BAUD_CODE_4800              2U
#define MODBUS_BAUD_CODE_9600              3U
#define MODBUS_BAUD_CODE_19200             4U
#define MODBUS_BAUD_CODE_38400             5U
#define MODBUS_BAUD_CODE_57600             6U
#define MODBUS_BAUD_CODE_115200            7U

//引脚设置
#define MODBUS_USART                       USART1
#define MODBUS_USART_IRQn                  USART1_IRQn
#define MODBUS_USART_IRQHandler            USART1_IRQHandler
#define MODBUS_USART_RCC                   RCC_APB2Periph_USART1
#define MODBUS_USART_GPIO_RCC              RCC_APB2Periph_GPIOA
#define MODBUS_USART_GPIO                  GPIOA
#define MODBUS_USART_TX_PIN                GPIO_Pin_9
#define MODBUS_USART_RX_PIN                GPIO_Pin_10

//串口配置
#define MODBUS_RS485_DE_RCC                RCC_APB2Periph_GPIOB
#define MODBUS_RS485_DE_GPIO               GPIOB
#define MODBUS_RS485_DE_PIN                GPIO_Pin_1
#define MODBUS_RS485_DE_ACTIVE_HIGH        1U

#define MODBUS_FRAME_TIMER                 TIM2
#define MODBUS_FRAME_TIMER_IRQn            TIM2_IRQn
#define MODBUS_FRAME_TIMER_IRQHandler      TIM2_IRQHandler
#define MODBUS_FRAME_TIMER_RCC             RCC_APB1Periph_TIM2

#endif
