#include "main.h"
#include "Reg_i2c.h"
/* Define OLED address - typical address for SSD1306 OLED displays */
//#define OLED_Addr 0x78 // 0x3C << 1
extern uint16_t temp;
/* Function prototypes */
void I2C1_Init(void);
void I2C1_Start(void);
void I2C1_Stop(void);
//static void I2C1_WriteByte(uint8_t data);
 uint8_t I2C1_WaitACK(void);
//static void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data);
uint32_t sysclk_freq ;
/**
 * @function: I2C1_Init
 * @description: Initialize I2C1 peripheral in register level for STM32F334
 * @param: None
 * @return: None
 */
void I2C1_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable clocks for GPIOB and I2C1 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    
    /* Configure GPIO pins for I2C1: PB6 (SCL) and PB7 (SDA) */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;         // Alternate function, Open Drain
    GPIO_InitStruct.Pull = GPIO_NOPULL;             // Pull-up resistors
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   // High speed
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;      // AF4 for I2C1
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		__HAL_RCC_I2C1_CLK_ENABLE();

		RCC->CFGR3 |= RCC_CFGR3_I2C1SW_SYSCLK;  //SYSCLK

		sysclk_freq = HAL_RCC_GetSysClockFreq();

    I2C1->CR1 &= ~I2C_CR1_PE;

    //I2C1->CR1 &= ~I2C_CR1_ANFOFF;  

    //I2C1->CR1 &= ~I2C_CR1_DNF;    

    I2C1->TIMINGR =   0x10808DD3;//datasheet p911


    I2C1->CR1 |= I2C_CR1_PE;

}

void I2C1_Start(void)
{
    // Clear configuration
    I2C1->CR2 = 0;
    //I2C1->CR2 |= (OLED_Addr << 1) & I2C_CR2_SADD;
    // Set slave address, number of bytes to send and generate START   (1 << I2C_CR2_NBYTES_Pos) |
		//I2C1->CR2 = (OLED_Addr << 1) |  (1 << I2C_CR2_NBYTES_Pos);
		//I2C1->CR2 = (OLED_Addr << 1) | (0 << 10) | (1 << 16) | (0 << 25);
    //I2C1->CR2 |= I2C_CR2_START;
    // Wait until START is sent
 	 I2C1->CR2 = ((uint32_t)0x3C << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;
    while(I2C1->CR2 & I2C_CR2_START);
 
}

void I2C1_Stop(void)
{
    // Generate STOP condition
    I2C1->CR2 |= I2C_CR2_STOP;
    
    // Wait until STOP is sent
    while((I2C1->ISR & I2C_ISR_STOPF) == 0);
    
    // Clear STOPF flag
    I2C1->ICR |= I2C_ICR_STOPCF;
}
 uint8_t I2C1_WaitACK(void)
{
    // Check if NACKF flag is set
    if(I2C1->ISR & I2C_ISR_NACKF)
    {
        // Clear NACKF flag
        I2C1->ICR |= I2C_ICR_NACKCF;
        return 1; // NACK received
    }
    return 0; // ACK received
}


void test(){
 I2C1_Start();
	I2C1->CR2 = (I2C1->CR2 & ~(I2C_CR2_NBYTES)) | (1 << I2C_CR2_NBYTES_Pos);
    
    // Send command/data control byte
    //I2C1_WriteByte(0x15);
	I2C1_Stop();
}

void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t length)
	{

	uint8_t send_arr[length+1];
	send_arr[0]=memadd;
	
	send_arr[1]= data ;
		
	//Enable I2C
	I2C1->CR1 |=I2C_CR1_PE;

	I2C1->CR2=(slav_add<<1);//11 1100 << 1
	//7-bit addressing
	I2C1->CR2&=~I2C_CR2_ADD10;
	
	I2C1->CR2|=((length+1)<<I2C_CR2_NBYTES_Pos);
	//Set the mode to write mode
	I2C1->CR2&=~I2C_CR2_RD_WRN;
	//hardware end
	I2C1->CR2|=I2C_CR2_AUTOEND;//datasheet P934
	//Generate start
	I2C1->CR2|=I2C_CR2_START;
	int i=0;
	while(!(I2C1->ISR & I2C_ISR_STOPF))
	{
		
		if(I2C1->ISR & I2C_ISR_TXE)
		{
			
			I2C1->TXDR = send_arr[i];
			i++;
		}
	}
    I2C1->CR1 &=~I2C_CR1_PE;
	}
void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data)
{
		i2c_write_memory(0x3C, cmd_or_data, WData, 1);
}
