#ifndef _buckboost_h
#define _buckboost_h

#include "main.h"

#define adc1_channalNum 4
#define adc2_channalNum 1

//pwm
#define DP_PWM_PER   HRTIM1->sMasterRegs.MPER //主定时器周期
#define MAX_PWM_CMP   (uint32_t)(0.95F * HRTIM1->sMasterRegs.MPER) //PWM最大比较值
#define MIN_PWM_CMP   (uint32_t)(0.04F * HRTIM1->sMasterRegs.MPER) //PWM最小比较值

//F334用
extern uint16_t adc1_in_buff[adc1_channalNum];
extern uint16_t adc2_in_buff[adc2_channalNum];

typedef struct
{
	float Votage_ratio; //运放增益
	float Current_ratio; //电流采样增益(没用)
	float LimitRefereePower; //裁判系统限制功率(从上位机获取)
  float TargetCapVoatge; //电容目标电压
  float TargetCapCurrent; //电容目标电流
	float TargetPower; //电容目标功率
	float MaximumPower; //极限放电功率(发给上位机)
}ELEC_value_struct;

extern ELEC_value_struct PSM;

extern float Init_PWM_CMP;
extern float Init_Referee_Vol, Init_Cap_Vol;

extern float Referee_Vol, Cap_Vol; //实际值
extern float Referee_Curr, Cap_Curr, Chassis_Curr;
extern float Referee_Power, Cap_Power, Load_Power; 
void Send_Float_String(float Referee_Curr, float Cap_Curr);
void Send_Float_String_IT(float Referee_Curr, float Cap_Curr);
void pwm_update(uint32_t compareValue); //PWM更新函数
void pwm_brake(void); //PWM刹车 
void buckboost_init(void); //BUCK-BOOST环路初始化
void power_start(void); //BUCK-BOOST环路控制
void Power_Calculate(void); //功率计算
void LED(void); //LED指示灯
void OLED_Display_Start(void); //OLED初始显示
void OLED_Display_Refresh(void); //OLED更新显示
void lowVol_and_highVol_protect(void);
void dr_pwm_init(uint32_t fsw);
void oled_display();
#endif
