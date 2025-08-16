#ifndef __REG_I2C_H__
#define __REG_I2C_H__
#include "main.h"
#include "stm32f3xx_hal.h"
void I2C1_Init(void);
void I2C1_Start(void);
void I2C1_Stop(void);
void I2C1_WriteByte(uint8_t data);
uint8_t I2C1_WaitACK(void);
void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data);
void test();
void I2C_Master_receiver(uint8_t SADD, uint8_t Size);
void I2C_Master_transmit(uint8_t SADD, uint8_t Size);
void Master_init(void);
void I2C_Intit_Seting(void);
void I2C_Config();
#endif