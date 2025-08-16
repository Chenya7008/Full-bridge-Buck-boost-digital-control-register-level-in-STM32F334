#include "stm32f3xx.h"
#include "main.h"
#include "ADC_Reg.h"
uint16_t adc1_value[4]; 
uint16_t adc2_value[1]; 
uint16_t adctest;
void ADC1_DMA_Init(void)
{
   
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;     //  enable clock for gpioA
    RCC->AHBENR |= RCC_AHBENR_ADC12EN;     //  enable clock for adc1 adc2
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;      //  enable clock for DMA1
    
		GPIOA->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | 
                      GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    
    // set pin into analog mode
    GPIOA->MODER |= (GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | 
                     GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
     
    ADC12_COMMON->CCR &= ~ADC12_CCR_CKMODE;
    ADC12_COMMON->CCR |= ADC12_CCR_CKMODE_0; //HCLK/2
		ADC12_COMMON->CCR  |= 0<<0;
    
    // disable adc
    ADC1->CR &= ~ADC_CR_ADEN;
 	 
		//enable the ADC voltage regulator
		ADC1->CR &= ~ADC_CR_ADVREGEN; 
		ADC1->CR |= ADC_CR_ADVREGEN_0; 
		
    // set working mode 
    ADC1->CFGR |= ADC_CFGR_CONT;     //continus 
    ADC1->CFGR &= ~ ADC_CFGR_DISCEN;
		// trigger mode 
     ADC1->CFGR &= ~ADC_CFGR_EXTEN;      
     ADC1->CFGR |= ADC_CFGR_EXTEN_0; //rising edge
    // external trigger
    ADC1->CFGR &= ~ADC_CFGR_EXTSEL; 
    ADC1->CFGR |= 7 << 6; //HRTIM_TRG1 0111 datasheet page232
     //ADC1->CFGR &= ~ADC_CFGR_EXTEN; 

    // right align
      ADC1->CFGR &= ~ADC_CFGR_ALIGN;      
    
    // enable dma
    ADC1->CFGR |= ADC_CFGR_DMAEN;   
    ADC1->CFGR |=ADC_CFGR_DMACFG;        //  DMA Circular Mode selected 
   
    // sample time
    //ADC1->SMPR1 &= ~ADC_SMPR1_SMP1;       
    //ADC1->SMPR1 |= (0x7 << ADC_SMPR1_SMP1_Pos);
    

    ADC1->CR |= ADC_CR_ADCAL;
 
    while(ADC1->CR & ADC_CR_ADCAL);
		

    ADC1->CR |= ADC_CR_ADEN;
		//set the conversion sequence
		ADC1->SQR1 |= (3 << ADC_SQR1_L_Pos);  // number of conversion times n-1
		ADC1->SQR1 |= (1 << ADC_SQR1_SQ1_Pos);
		ADC1->SQR1 |= (2 << ADC_SQR1_SQ2_Pos);  
		ADC1->SQR1 |= (3 << ADC_SQR1_SQ3_Pos);  
		ADC1->SQR1 |= (4 << ADC_SQR1_SQ4_Pos); 
		
    // wait for adc ready
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
		
		
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;    // peripheral address is the ADC2 data register
    DMA1_Channel1->CMAR = (uint32_t)&adc1_value;   // Memory address for adc_value variable
    DMA1_Channel1->CNDTR = 4;                     // number
    
    // config the control reg for DMA
    DMA1_Channel1->CCR &= ~DMA_CCR_DIR;           // direction
    DMA1_Channel1->CCR &= ~DMA_CCR_MSIZE;         // memory size
    DMA1_Channel1->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel1->CCR &= ~DMA_CCR_PSIZE;         //  data widewith peripheral
    DMA1_Channel1->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel1->CCR |= DMA_CCR_MINC;           // memory increment mode
    DMA1_Channel1->CCR &= ~DMA_CCR_PINC;          // peripheral increment mode
    DMA1_Channel1->CCR |= DMA_CCR_CIRC;           // circular mode
    DMA1_Channel1->CCR &= ~DMA_CCR_PL;            // priority level
    DMA1_Channel1->CCR |= DMA_CCR_PL_1;           // priority level

    DMA1_Channel1->CCR |= DMA_CCR_EN;
    
    ADC1->CR |= ADC_CR_ADSTART;
}

void ADC2_DMA_Init(void)
{
	
		GPIOA->MODER &= ~(0x3 << GPIO_MODER_MODER4_Pos);
    GPIOA->MODER |=  (0x3 << GPIO_MODER_MODER4_Pos); // analog mode

    // disable abc
    ADC2->CR &= ~ADC_CR_ADEN;
 
		//enable the ADC voltage regulator
		ADC2->CR &= ~ADC_CR_ADVREGEN; 
		ADC2->CR |= ADC_CR_ADVREGEN_0; 
		
 
    ADC2->CFGR |= ADC_CFGR_CONT;          //  continus mode
    ADC2->CFGR &= ~ ADC_CFGR_DISCEN;
		
		ADC2->CFGR &= ~ADC_CFGR_EXTEN;      
     ADC2->CFGR |= ADC_CFGR_EXTEN_0;      
    // set external tigger as the HRTIM_TRG1
    ADC2->CFGR &= ~ADC_CFGR_EXTSEL;       
    ADC2->CFGR |= 7 << 6; 
     //ADC2->CFGR &= ~ADC_CFGR_EXTEN; 

    ADC2->CFGR &= ~ADC_CFGR_ALIGN;        
    
    ADC2->CFGR |= ADC_CFGR_DMAEN;       
    ADC2->CFGR &= ~ADC_CFGR_DMACFG;   //different from ADC1  
    
  
    ADC2->CR |= ADC_CR_ADCAL;
   
    while(ADC2->CR & ADC_CR_ADCAL);
		
 
    ADC2->CR |= ADC_CR_ADEN;
		
	  ADC2->SQR1 &= ~ADC_SQR1_L;
		ADC2->SQR1 &= ~ADC_SQR1_SQ1; 
		ADC2->SQR1 |= 1 << 6;  
		
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    
    DMA1_Channel2->CPAR = (uint32_t)&ADC2->DR;    //  set perherial address as data register 
    DMA1_Channel2->CMAR = (uint32_t)&adc2_value;    
    DMA1_Channel2->CNDTR = 1;                     // determine the number of data
    
    DMA1_Channel2->CCR &= ~DMA_CCR_DIR;         
    DMA1_Channel2->CCR &= ~DMA_CCR_MSIZE;       
    DMA1_Channel2->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel2->CCR &= ~DMA_CCR_PSIZE;       
    DMA1_Channel2->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel2->CCR |= DMA_CCR_MINC;          
    DMA1_Channel2->CCR &= ~DMA_CCR_PINC;         
    DMA1_Channel2->CCR |= DMA_CCR_CIRC;        
    DMA1_Channel2->CCR &= ~DMA_CCR_PL;         
    DMA1_Channel2->CCR |= DMA_CCR_PL_1;         
  
		//   enable DMA channel
    DMA1_Channel2->CCR |= DMA_CCR_EN;
 
    ADC2->CR |= ADC_CR_ADSTART;

		adctest = ADC2->DR;
}
int readADC2()
{
	ADC2->CR |= ADC_CR_ADSTART; // start conversion
	while (ADC1->CR & BIT2); // wait for end of conversion
	return ADC1->DR;
}
 
