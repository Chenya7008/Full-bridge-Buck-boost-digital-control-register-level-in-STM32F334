#include "buckboost.h"
//#include "adc.h"
//#include "can.h"
#include "pid.h"
#include "Filter.h"
#include "math.h"
#include "stm32f3xx.h"
#include "OLED_SSD1306.h"
#include "string.h"
#include "usart.h"
#include "stdio.h"
//#include "hrtim.h"
#define PWM_DEADTIME    (60)
extern uint16_t adc1_value[4]; //  store adc value from DMA
extern uint16_t adc2_value[1]; 
float Referee_Vol, Cap_Vol; 
float Referee_Curr, Cap_Curr, Chassis_Curr;
float Referee_Power, Cap_Power, Load_Power, Topu_Power;
float Topu_Efficienty; 
char uart_tx_buf[64];
volatile uint8_t uart_tx_busy = 0;


float temp_CMP;
float Init_PWM_CMP;
float Init_Referee_Vol, Init_Cap_Vol;
uint8_t Init_Flag = 0;
float temp_vol,temp_curr;

ELEC_value_struct PSM; 

uint8_t Curr_Direction = 0; 
uint8_t Last_Curr_Direction = 0;
uint8_t Direction_Change_Flag = 0;

uint8_t DCDC_Mode = 0; 		
uint8_t Last_DCDC_Mode = 0;
uint8_t Mode_Change_Flag = 0;

uint8_t DCDC_State = 0;			
uint8_t protect_flag = 0;
PID_STRUCT Vcap_Loop;
PID_STRUCT Ccap_In_Loop;
PID_STRUCT Ccap_Out_Loop;

 
uint16_t adc1_in_buff[adc1_channalNum] = {0,0,0,0}; 
uint16_t adc2_in_buff[adc2_channalNum] = {0};

float buff_1,buff_2,buff_3,buff_4,buff_5; 


void buckboost_init(void)
{

	PSM.LimitRefereePower = 80.0f; 
	PSM.Votage_ratio = 20.0f;
	
	
	pid_func.reset(&Vcap_Loop); 
  Vcap_Loop.T       = 0.50f;
  Vcap_Loop.Kp      = 15.70f;      
  Vcap_Loop.Ti      = 3.00f;
  Vcap_Loop.Td      = 0.01f;
  Vcap_Loop.Ek_Dead = 0.01f;
  Vcap_Loop.OutMin  = 0.04f * DP_PWM_PER;
  Vcap_Loop.OutMax  = 1.70f * DP_PWM_PER;
  pid_func.init(&Vcap_Loop); 
		
	
	pid_func.reset(&Ccap_In_Loop);
  Ccap_In_Loop.T       = 0.50f;
  Ccap_In_Loop.Kp      = 8.0f;
  Ccap_In_Loop.Ti      = 3.00f;
  Ccap_In_Loop.Td      = 0.01f;
  Ccap_In_Loop.Ek_Dead = 0.01f;
  Ccap_In_Loop.OutMin  = 0.04f * DP_PWM_PER;
  Ccap_In_Loop.OutMax  = 1.70f * DP_PWM_PER;
  pid_func.init(&Ccap_In_Loop);
	

}

void pwm_update(uint32_t pwm_cmp_value)
{

  uint32_t buck_duty  = 0; //buck_duty 
  uint32_t boost_duty = 0; //boost_duty

  if( pwm_cmp_value >= MAX_PWM_CMP)
  {
		DCDC_Mode = 1;
		buck_duty  = MAX_PWM_CMP;   //BOOST mode 
    boost_duty  = pwm_cmp_value - MAX_PWM_CMP + 0.06F * HRTIM1->sMasterRegs.MPER; 
  }																									
  else
  {
		DCDC_Mode = 0;
		boost_duty  = 0.06F * HRTIM1->sMasterRegs.MPER; //BUCK mode,boost_duty is soild
		buck_duty  = pwm_cmp_value; 
  }
	if(DCDC_Mode != Last_DCDC_Mode)
		Mode_Change_Flag = 1; 
	Last_DCDC_Mode = DCDC_Mode;
	
	//to limit the duty
  if( boost_duty > MAX_PWM_CMP )
		boost_duty = MAX_PWM_CMP;
  if( boost_duty < MIN_PWM_CMP )
		boost_duty = MIN_PWM_CMP;
  if( buck_duty  > MAX_PWM_CMP )
		buck_duty  = MAX_PWM_CMP;
  if( buck_duty  < MIN_PWM_CMP )
		buck_duty  = MIN_PWM_CMP;

	// update the compare register in master timer 
  HRTIM1->sMasterRegs.MCMP1R  = ( DP_PWM_PER - buck_duty  ) >> 1; //for adc trigger
  HRTIM1->sMasterRegs.MCMP2R  = ( DP_PWM_PER + buck_duty  ) >> 1;
  HRTIM1->sMasterRegs.MCMP3R  = ( DP_PWM_PER + boost_duty ) >> 1;
  HRTIM1->sMasterRegs.MCMP4R  = ( DP_PWM_PER - boost_duty ) >> 1;
}


void pwm_brake(void)
{
	HRTIM1->sMasterRegs.MCMP1R  = DP_PWM_PER >> 1; 
  HRTIM1->sMasterRegs.MCMP2R  = DP_PWM_PER >> 1;
  HRTIM1->sMasterRegs.MCMP3R  = DP_PWM_PER >> 1;
  HRTIM1->sMasterRegs.MCMP4R  = DP_PWM_PER >> 1;
}


void power_start(void)
{
	float m_pwm_cmp; 
	
 
		PSM.LimitRefereePower = 30; 
	if (temp_vol > 0)
	{
		PSM.TargetCapVoatge = temp_vol;
	}
	else{PSM.TargetCapVoatge = 23.0f;}
 

	
	buff_1 = GetAverage(adc1_value, 0);
  buff_2 = GetAverage(adc1_value, 1);
	buff_3 = GetAverage(adc1_value, 2);
	buff_4 = GetAverage(adc1_value, 3);
	buff_5 = GetAverage(adc2_value, 0);


    Referee_Vol = (buff_1 / 4095.0f * 3.298f * PSM.Votage_ratio)*0.9948f - 0.6311f;
    Cap_Vol = (buff_2 / 4095.0f * 3.298f * PSM.Votage_ratio)* 1.0025f - 0.663;
	 Referee_Curr = (buff_3 / 4095.0f * 3.298f)*9.9216f - 16.517f;
	 Cap_Curr = (buff_4 / 4095.0f * 3.298f)*9.8452f - 16.49f;
	 Chassis_Curr = (buff_5 / 4095.0f * 3.298f)*9.5966f - 15.966f; 
     

	
	PSM.TargetPower = PSM.LimitRefereePower - Load_Power; 
	if(PSM.TargetPower >= 0)
		PSM.TargetPower = PSM.TargetPower * Topu_Efficienty; 
	else
		PSM.TargetPower = PSM.TargetPower / Topu_Efficienty; 

	PSM.TargetCapCurrent = PSM.TargetPower / Cap_Vol; 
	if(PSM.TargetCapCurrent >= 0) 
		Curr_Direction = 0;
	else
		Curr_Direction = 1;
	if(Curr_Direction != Last_Curr_Direction)
		Direction_Change_Flag = 1;
	Last_Curr_Direction = Curr_Direction;
	
	if(PSM.TargetCapCurrent > 8) 
		PSM.TargetCapCurrent = 8;
	if(PSM.TargetCapCurrent < -8) 
		PSM.TargetCapCurrent = -8;
	if(Cap_Vol >= 23.5f && Curr_Direction == 0) 
		PSM.TargetCapCurrent = 0.01f;

	
  // for protection
	if(Init_Flag == 0)
	{
		Vcap_Loop.Output = Init_PWM_CMP;
		Ccap_In_Loop.Output = Init_PWM_CMP;
		Init_Flag = 1;
	}
	
	
	Ccap_In_Loop.Ref = PSM.TargetCapCurrent;
	Ccap_In_Loop.Fdb = Cap_Curr;
	pid_func.calc( &Ccap_In_Loop );

	if(Curr_Direction == 0)
	{
		
		Vcap_Loop.Ref = PSM.TargetCapVoatge;
		Vcap_Loop.Fdb = Cap_Vol;
		pid_func.calc( &Vcap_Loop );	
		m_pwm_cmp = fminf(Vcap_Loop.Output, Ccap_In_Loop.Output); 
	}
	else
		m_pwm_cmp = Ccap_In_Loop.Output;

	Vcap_Loop.Output = m_pwm_cmp; 	
	Ccap_In_Loop.Output = m_pwm_cmp;	
	temp_CMP = m_pwm_cmp;
  pwm_update( m_pwm_cmp );      
//	pwm_update(0.3F * HRTIM1->sMasterRegs.MPER); //Fixed duty test
}

void Power_Calculate(void)
{
	Referee_Power = Referee_Vol *Referee_Curr;	
	Cap_Power = Cap_Vol * Cap_Curr;							
	Load_Power = Referee_Vol * Chassis_Curr; 
	Topu_Power = Referee_Power - Load_Power; 
	PSM.MaximumPower = 15 * Cap_Vol - 5;				
	
	if(Cap_Power >= 0)
		Topu_Efficienty = Cap_Power / Topu_Power;	
	else
		Topu_Efficienty = Topu_Power / Cap_Power;
	if(Topu_Efficienty <= 0.8)
		Topu_Efficienty = 0.8;
    if(Topu_Efficienty > 1)
        Topu_Efficienty = 0.95;
}

void LED(void)
{	
	static uint8_t cnt = 0;
	
	if(Cap_Vol >= 20) 
	{
		cnt++;
		if(cnt >= 50)
		{
			cnt = 0;
			HAL_GPIO_TogglePin(GPIOC, LED2_Pin);
		}
	}
	else if(Cap_Vol < 20) 
	{
		HAL_GPIO_WritePin(GPIOC, LED2_Pin, GPIO_PIN_SET);
	}	
}

//OLED初始显示
void OLED_Display_Start(void)
{
	OLED_ShowStr(0, 0, (uint8_t *)"Vol_In:", FontSize6x8, 0); 
	OLED_ShowStr(0, 2, (uint8_t *)"Vol_Out:", FontSize6x8, 0); 
	OLED_ShowStr(0, 4, (uint8_t *)"Power_Lim:", FontSize6x8, 0); 
	
	OLED_ShowChar(14, 1, '.', FontSize6x8, 0);
	OLED_ShowChar(14, 3, '.', FontSize6x8, 0);
	
	OLED_ShowStr(60, 0, (uint8_t *)"Curr_In:", FontSize6x8, 0); 
	OLED_ShowStr(60, 2, (uint8_t *)"Curr_Out:", FontSize6x8, 0); 
	OLED_ShowStr(60, 4, (uint8_t *)"Pout_Act:", FontSize6x8, 0); 
	
	OLED_ShowChar(74, 1, '.', FontSize6x8, 0);
	OLED_ShowChar(74, 3, '.', FontSize6x8, 0);
	OLED_ShowChar(82, 5, '.', FontSize6x8, 0);
	
	
	OLED_ShowStr(0, 6, (uint8_t *)"targrt_vol ", FontSize8x16, 0);
//	if(DCDC_Mode == 0) //dispiay topo-mode
//		OLED_ShowStr(0, 6, (uint8_t *)"BUCK ", FontSize8x16, 0);  
//	else
//		OLED_ShowStr(0, 6, (uint8_t *)"BOOST", FontSize8x16, 0);
//	
//	if(Curr_Direction == 0) //display current-direction
//		OLED_ShowStr(60, 6, (uint8_t *)"======>", FontSize8x16, 0);	 
//	else 
//		OLED_ShowStr(60, 6, (uint8_t *)"<======", FontSize8x16, 0);
}

void OLED_Display_Refresh(void)
{
	OLED_ShowNum(0, 1, (uint32_t)Referee_Vol, 2, 16); 
	OLED_ShowNum_0(20, 1, (uint32_t)((Referee_Vol - (uint32_t)Referee_Vol) * 1000), 3, 16);
		
	OLED_ShowNum(0, 3, (uint32_t)Cap_Vol, 2, 16);
	OLED_ShowNum_0(20, 3, (uint32_t)((Cap_Vol - (uint32_t)Cap_Vol) * 1000), 3, 16);
	  
	OLED_ShowNum(0, 5, (uint32_t)PSM.LimitRefereePower, 3, 16);
	  
	OLED_ShowNum(60, 1, (uint32_t)Referee_Curr, 2, 16);
	OLED_ShowNum_0(80, 1, (uint32_t)((Referee_Curr - (uint32_t)Referee_Curr) * 1000), 3, 16);
	  
	if(Cap_Curr < 0)
	{
		OLED_ShowChar(52, 3, '-', FontSize6x8, 0);
		OLED_ShowNum(60, 3, (uint32_t)(-Cap_Curr), 2, 16);
		OLED_ShowNum_0(80, 3, (uint32_t)(((-Cap_Curr) - (uint32_t)(-Cap_Curr)) * 1000), 3, 16);
	}
	else
	{
		OLED_ShowChar(52, 3, ' ', FontSize6x8, 0);
		OLED_ShowNum(60, 3, (uint32_t)Cap_Curr, 2, 16);
		OLED_ShowNum_0(80, 3, (uint32_t)((Cap_Curr - (uint32_t)Cap_Curr) * 1000), 3, 16);
	}
	
	if(Cap_Power < 0)
	{
		OLED_ShowChar(52, 5, '-', FontSize6x8, 0);
		OLED_ShowNum(60, 5, (uint32_t)(-Cap_Power), 3, 16);
		OLED_ShowNum_0(88, 5, (uint32_t)(((-Cap_Power) - (uint32_t)(-Cap_Power)) * 1000), 3, 16);
	}
	else
	{
		OLED_ShowChar(52, 5, ' ', FontSize6x8, 0);
		OLED_ShowNum(60, 5, (uint32_t)Cap_Power, 3, 16);
		OLED_ShowNum_0(88, 5, (uint32_t)((Cap_Power - (uint32_t)Cap_Power) * 1000), 3, 16);
	}
	
	OLED_ShowNum(77, 7, (uint32_t)PSM.TargetCapVoatge, 3, 16);
	//OLED_ShowNum_0(82, 7, (uint32_t)((PSM.TargetCapVoatge - (uint32_t)PSM.TargetCapVoatge) * 1000), 3, 16);
//	if(Mode_Change_Flag == 1) //When topo-mode changed,refresh display
//	{
//		if(DCDC_Mode == 0) //dispiay topo-mode
//			OLED_ShowStr(0, 6, (uint8_t *)"BUCK ", FontSize8x16, 0);
//		else
//			OLED_ShowStr(0, 6, (uint8_t *)"BOOST", FontSize8x16, 0);
//		Mode_Change_Flag = 0;
//	}

//	if(Direction_Change_Flag == 1) //When current-direction changed,refresh display
//	{
//		if(Curr_Direction == 0)
//			OLED_ShowStr(60, 6, (uint8_t *)"======>", FontSize8x16, 0);	
//		else 
//			OLED_ShowStr(60, 6, (uint8_t *)"<======", FontSize8x16, 0);
//		Direction_Change_Flag = 0;
//	}
}
void oled_display()
{
	OLED_ShowStr(0, 3, (uint8_t *)"mode01", FontSize6x8, 0);
 
}

void lowVol_and_highVol_protect(void)
{
     
     if(Referee_Vol< 17.0f || Referee_Vol> 30.0f)
     {
        protect_flag = 1; 
        //HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2 | HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
     }
     if(protect_flag == 1 && Referee_Vol>20.0f && Referee_Vol < 27.0f)
     {
        Init_PWM_CMP = (Cap_Vol/Referee_Vol) * DP_PWM_PER;
        pwm_update(Init_PWM_CMP);
        //HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2 | HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
        Init_Flag = 0;
        protect_flag = 0;     
     }
}

void dr_pwm_init(uint32_t fsw)
{


__HAL_RCC_HRTIM1_CLK_ENABLE();
	
		GPIO_InitTypeDef GPIO_InitStruct = {0};
__HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_HRTIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	
	HRTIM1->sCommonRegs.DLLCR |= 0x01UL << 1;	 //DLL Calibration Enable
	HRTIM1->sCommonRegs.DLLCR |= 0x03UL << 2; 	//DLL Calibration rate
	HAL_Delay(100);
	//while((HRTIM1->sCommonRegs.ISR & (1UL << 16)) == 0);
//	HRTIM1->sCommonRegs.DLLCR |= 0x01UL << 0; //DLL Calibration Start
	

//uint32_t tmpDLLCR = HRTIM1->sCommonRegs.DLLCR;
//tmpDLLCR &= ~(HRTIM_DLLCR_CALRTE | HRTIM_DLLCR_CAL);
//tmpDLLCR |= (0x03UL << 2) | (0x01UL << 1);          
//HRTIM1->sCommonRegs.DLLCR = tmpDLLCR;
	
  //HAL_Delay(10);
  //while( (HRTIM1->sCommonRegs.ISR & 0x00010000UL) != 0x00010000UL )
 
	
//	 if (HAL_HRTIM_DLLCalibrationStart(&hhrtim1, HRTIM_CALIBRATIONRATE_14) != HAL_OK)
//  {
//    Error_Handler();
//  }
	 //HRTIM_DLLCalibrationStart(HRTIM1, HRTIM_CALIBRATIONRATE_14);
		 

																							
  HRTIM1->sMasterRegs.MCR   &= 0x00UL;       //clear HRTIM_MCR
	HRTIM1->sMasterRegs.MCR   |= 0x00UL << 5; 	//HALF MODE DISABLE
	HRTIM1->sMasterRegs.MCR   |= 0x00UL << 11;   //start sync disable
	HRTIM1->sMasterRegs.MCR   |= 0x00UL << 10;  //reset sync disable
	HRTIM1->sMasterRegs.MCR   |= 0x00UL << 25; //dac sync none
  HRTIM1->sMasterRegs.MCR   |= 0x00UL << 29; //disable repition update
  HRTIM1->sMasterRegs.MCR   |= 0x00UL << 27; //reload disable
  HRTIM1->sMasterRegs.MCR   |= 0x01UL << 3;  //continus mode 
																											
	//HRTIM1->sMasterRegs.MCR &= ~(0x07UL << 0);
  HRTIM1->sMasterRegs.MCR   |= 0x00UL << 0;  //fHRCK: 1.36 GHz
 
 

  HRTIM1->sMasterRegs.MPER = (uint32_t)15360;//Period = fHRCK / fsw
  //HRTIM1->sMasterRegs.MREP = (uint32_t)0x00;    
  
	
	
  HRTIM1->sMasterRegs.MCMP1R  =  0.7f*HRTIM1->sMasterRegs.MPER;
  HRTIM1->sMasterRegs.MCMP2R  =  0.7f*HRTIM1->sMasterRegs.MPER;
  HRTIM1->sMasterRegs.MCMP3R  =  0.7f*HRTIM1->sMasterRegs.MPER;
  HRTIM1->sMasterRegs.MCMP4R  =  0.7f*HRTIM1->sMasterRegs.MPER;
	//HRTIM1->sCommonRegs.CR2 |= 0x01UL << 0;
	// HRTIM_Timer_A configure

  HRTIM1->sTimerxRegs[0].TIMxCR  &= 0x00UL;       //clear HRTIM_TIMCCR
  HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x00UL << 27; //preload 
  HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x01UL << 24; //trigger by master timer 
  HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x01UL << 3;  //continus mode 
	HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x00UL << 25; //dac disable 
	HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x00UL << 26; //push poll
	//HRTIM1->sTimerxRegs[1].TIMxCR &= ~(0x07UL << 0);
  HRTIM1->sTimerxRegs[0].TIMxCR  |= 0x00UL << 0;  //fHRCK

  HRTIM1->sTimerxRegs[0].CMP3xR = 0.5f*HRTIM1->sMasterRegs.MPER;
  HRTIM1->sTimerxRegs[0].RSTxR   = 0x00UL << 4;   //sync

  HRTIM1->sTimerxRegs[0].PERxR = (uint32_t)15360;

  HRTIM1->sTimerxRegs[0].REPxR = (uint32_t)0x00;  //reptition period

  HRTIM1->sTimerxRegs[0].SETx1R = 0x01UL << 8;  //HRTIM_Master_COMP1 tigger 
  HRTIM1->sTimerxRegs[0].RSTx1R = 0x01UL << 9;  //HRTIM_Master_COMP2 tigger 

  //dead time configure for TIMER A
  HRTIM1->sTimerxRegs[0].OUTxR  = 0x100; // enable and positive 

  HRTIM1->sTimerxRegs[0].DTxR  &= 0x00UL;       //clear HRTIM_DTBR
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 10; //Prescaler
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 31; // Deadtime falling value and sign is writable
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 30; //Deadtime falling sign is writable
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 25; //Positive deadtime on falling
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 15; //Deadtime rising value and sign is writable
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 14; //Deadtime rising sign is writable
  HRTIM1->sTimerxRegs[0].DTxR  |= 0x00UL << 9;  // Positive deadtime on rising edge
  HRTIM1->sTimerxRegs[0].DTxR  |= PWM_DEADTIME << 16; //falling edge 
  HRTIM1->sTimerxRegs[0].DTxR  |= PWM_DEADTIME << 0;  //rising edge
	
	
  // HRTIM_Timer_B configure

  HRTIM1->sTimerxRegs[1].TIMxCR  &= 0x00UL;       //clear HRTIM_TIMBCR
  HRTIM1->sTimerxRegs[1].TIMxCR  |= 0x00UL << 27; //disable preload 
  HRTIM1->sTimerxRegs[1].TIMxCR  |= 0x01UL << 24; //trigger by HRTIM_MASTER
  HRTIM1->sTimerxRegs[1].TIMxCR  |= 0x01UL << 3;  //continus mode 
	
  HRTIM1->sTimerxRegs[1].TIMxCR  |= 0x00UL << 0;  


  HRTIM1->sTimerxRegs[1].RSTxR   = 0x00UL << 4;   

  HRTIM1->sTimerxRegs[1].PERxR = (uint32_t)15360;//Period 

  HRTIM1->sTimerxRegs[1].REPxR = (uint32_t)0x00; //petition value 

  HRTIM1->sTimerxRegs[1].SETx1R |= 0x01UL << 10;  //trigger by HRTIM_Master_COMP3
  HRTIM1->sTimerxRegs[1].RSTx1R |= 0x01UL << 11;  //trigger by HRTIM_Master_COMP4

	
  //dead time timer B
  HRTIM1->sTimerxRegs[1].OUTxR  = 0x100; 
	
  HRTIM1->sTimerxRegs[1].DTxR  &= 0x00UL;     
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 10;
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 31; 
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 30; 
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 25;
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 15; 
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 14; 
  HRTIM1->sTimerxRegs[1].DTxR  |= 0x00UL << 9;  
  HRTIM1->sTimerxRegs[1].DTxR  |= PWM_DEADTIME << 16; 
  HRTIM1->sTimerxRegs[1].DTxR  |= PWM_DEADTIME << 0;  

  

  //HRTIM1->sCommonRegs.ADC1R  = HRTIM_ADC1R_AD1MPER; //MPER
   //HRTIM1->sCommonRegs.CR1   |= 0x00UL << 16;        //MASTER
	 
	HRTIM1->sCommonRegs.ADC1R  = HRTIM_ADC3R_AD3TAC3; //TIME A_CMP3
  HRTIM1->sCommonRegs.CR1   |= 0x01UL << 22;        //TIMEA 
  //HRTIM1->sTimerxRegs[1].CMP3xR = 0.5f*HRTIM1->sMasterRegs.MPER;//




}
void Send_Float_String(float Referee_Curr, float Cap_Curr)
{
    char msg[64];
    snprintf(msg, sizeof(msg), "Vol_Out:%.2f, Curr_out:%.2f\r\n", Referee_Curr, Cap_Curr);
    HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), 100);
}
void Send_Float_String_IT(float Referee_Curr, float Cap_Curr)
{
    if (uart_tx_busy == 0)  
    {
        snprintf(uart_tx_buf, sizeof(uart_tx_buf), "Vol_Out:%.2f, Curr_out:%.2f\r\n", Referee_Curr, Cap_Curr);
        uart_tx_busy = 1;
        HAL_UART_Transmit_IT(&huart3, (uint8_t*)uart_tx_buf, strlen(uart_tx_buf));
    }
}








