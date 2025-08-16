#include "main.h"
#include "TIM_Reg.h"
#include "buckboost.h"
#include "stdio.h"
void TIM6_Init(void)
{
  
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  
  //set prescaler 72 - 1  
  TIM6->PSC = 72 - 1;
  
  // Set auto-reload value to 100-1
  TIM6->ARR = 100 - 1;
  
  // Disable auto-reload preload
  TIM6->CR1 &= ~TIM_CR1_ARPE;
  
  // Enable update interrupt
  TIM6->DIER |= TIM_DIER_UIE;
  
  //Configure NVIC
  NVIC_SetPriority(TIM6_DAC1_IRQn, 0);
  
  //Enable TIM6 interrupt 
  NVIC_EnableIRQ(TIM6_DAC1_IRQn);
  
  
  TIM6->CR1 |= TIM_CR1_CEN;
}


void TIM6_DAC1_IRQHandler(void)
{
  
  if(TIM6->SR & TIM_SR_UIF)
  {
    //clear interrupt flag
    TIM6->SR &= ~TIM_SR_UIF;
    
    //interrupt execute 
		Power_Calculate();
		power_start();
		//pwm_update(0.6f*HRTIM1->sMasterRegs.MPER);
  }
}
void TIM7_Init(void)
{

  RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

  //72MHz -> 10kHz */
  TIM7->PSC = 7200 - 1;

  //10kHz / 10000 = 1Hz
  TIM7->ARR = 10000 - 1;

  TIM7->CR1 &= ~TIM_CR1_ARPE;

  TIM7->DIER |= TIM_DIER_UIE;

  NVIC_SetPriority(TIM7_IRQn, 0);     
  NVIC_EnableIRQ(TIM7_IRQn);     

  TIM7->CR1 |= TIM_CR1_CEN;
}

void TIM7_IRQHandler(void)
{
  if (TIM7->SR & TIM_SR_UIF)
  {
    TIM7->SR &= ~TIM_SR_UIF; 

    // send data to ESP32 web
    printf("Vol_Out:%.2f,Curr_out:%.2f\r\n", Referee_Vol, Cap_Curr);
		
    // pwm_update(0.4f*HRTIM1->sMasterRegs.MPER);
  }
}