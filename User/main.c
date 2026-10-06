#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "string.h"
#include "ds18b20.h"
#include "string.h" 	

#include "bsp_clock.h"
#include "modbus_rtu.h"
#include "modbus_data.h"

//#include "usart.h"
#include <stdio.h>


u8 temp;
u8 humi;

int main(void)
{
//	unsigned char p[16]=" ";

//	short temperature = 0; 				//温度值
//	delay_init(72);
//	LED_Init();
//	OLED_Init();
//	delay_ms(50);
//	OLED_Clear();
//	OLED_ShowChinese(0,0,0,16,1);
//	OLED_ShowChinese(16,0,1,16,1);
//	OLED_ShowChar(40,0,':',16,1);
//	while(DS18B20_Init())	//DS18B20初始化	
//	{
//		OLED_ShowString(0,0,"DS18B20 Error",16,1);
//		delay_ms(200);
//		OLED_ShowString(60,0,"        " ,16,1);	
//		delay_ms(200);
//	}
//	delay_ms(1000);

	if (!BSP_Clock_Init())
  {
        /* HSE 没起振或 PLL 没锁定时，时钟仍停在 HSI 8MHz，
         * 波特率和 T3.5 会全部算错。这里必须停机暴露问题，
         * 不要静默继续跑，否则排查时会被误导成协议层故障。 */
        while (1)
        {
        }
  }
	  
	  
  (void)ModbusData_SetInputRegister(4U, 2026U);
  (void)ModbusData_SetDiscreteInput(1U, true);
	Modbus_Init();

	
  while(1)
  {
//		
//		USART_SendData(USART2, 0x55);
//    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//		

		
   
		 Modbus_Poll();
//		temperature = DS18B20_Get_Temp();
//		sprintf((char*)p,"%4.1f    ",(float)temperature/10);
//		OLED_ShowString(60,0,p ,16,1);
//		
//		delay_ms(100);
//    LED_On();
//		delay_ms(500);
//		LED_Off();
//		delay_ms(500);
  }
}

