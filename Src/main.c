#include "main.h"
#include "bsp_rcc.h"
#include "bsp_lcd.h"
#include "bsp_key_led.h"
#include "bsp_tim.h"
#include "bsp_adc.h"
#include "bsp_i2c.h"

void Key_Proc(void);
void Led_Proc(void);
void Lcd_Proc(void);
void PWM_Proc(void);

//for delay
__IO uint32_t uwTick_key, uwTick_led, uwTick_lcd, uwTick_pwm;

//for lcd
uint8_t string_lcd[21];

//for led
uint8_t led;

//for key
uint8_t key_val, key_old, key_down, key_up;

//for R37
float R37_V;

//for PWM
float pwm_duty_input;
float pwm_duty_output;
float pwmn_duty_output;
uint8_t freq = 1;

//for eep
uint8_t first_flag[2] = {'f', 1};
uint8_t e2p_read_buf[2];
//for flags
uint8_t pwm_open_stop_flag;
uint8_t setting_flag;
char open[5] = {"open"};
char stop[5] = {"stop"};
char statue[5];

int main(void)
{
  HAL_Init();

  SystemClock_Config();
	
	//HAL_Delay();
	
	Key_Led_Init();
	
	I2CInit();
	
	PWM_double_TIM1_Init();
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	
	iic_24c02_read(e2p_read_buf, 0, 2);
	HAL_Delay(10);
	if(e2p_read_buf[0] != 'f')
		iic_24c02_write(first_flag, 0, 2);
	else 
		freq = e2p_read_buf[1];
	
	ADC2_Init();
	
	LCD_Init();
	LCD_Clear(Black);
	LCD_SetTextColor(White);
	LCD_SetBackColor(Black);


  while (1)
  {
		Key_Proc();
		Led_Proc();
		Lcd_Proc();
		PWM_Proc();
  }

}

void Key_Proc(void)
{
	if(uwTick - uwTick_key < 50)	return;
	uwTick_key = uwTick;
	
	key_val = Read_Key();
	key_down = key_val & (key_val ^ key_old);
	key_up  = ~key_val & (key_val ^ key_old);
	key_old = key_val;
	
	if(key_down == 1)
	{
		led ^= 0x01;
		pwm_open_stop_flag++;
	}
		
	else if(key_down == 2)
	{
		LCD_Clear(Black);
		setting_flag++;
		if(setting_flag%2 == 0)
			iic_24c02_write(&freq, 1, 1);
		//led = 0x02;
	}
		
	else if(key_down == 3)
	{
		if(setting_flag%2 != 0)
		{
			freq++;
			if(freq == 11)
				freq = 1;	
		}

		//led = 0x04;
	}
		//led = 0x08;
}

void Led_Proc(void)
{
	if(uwTick - uwTick_led < 50)	return;
	uwTick_led = uwTick;
	
	Led_Disp(led);
}


void PWM_Proc(void)
{
	if(uwTick - uwTick_pwm < 50)	return;
	uwTick_pwm = uwTick;
	
	uint8_t i;
	
	R37_V = Get_ADC2()*3.3/4096;
	pwm_duty_input = R37_V/3.3;
	
	//__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, (__HAL_TIM_GetAutoreload(&htim1)+1)*pwm_duty);
	
	if(pwm_open_stop_flag%2 != 0)
	{
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
		HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
		//__HAL_RCC_TIM1_CLK_DISABLE();
		GPIO_PWM_INIT();
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		
		pwm_duty_output = 0;
		pwmn_duty_output= 0;
		
		for(i=0; i<5; i++)
		{
			statue[i] = stop[i];
		}
		//pwm_open_stop_flag = 0;
	}
	
	else
	{
		HAL_TIM_MspPostInit(&htim1);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
		__HAL_TIM_SetAutoreload(&htim1, 1000000/(freq*1000)-1);
		__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, (__HAL_TIM_GetAutoreload(&htim1)+1)*pwm_duty_input);
		pwm_duty_output = ((float)__HAL_TIM_GetCompare(&htim1, TIM_CHANNEL_2)+1) / (__HAL_TIM_GetAutoreload(&htim1)+1);
		pwmn_duty_output = 1-pwm_duty_output;
		for(i=0; i<5; i++)
		{
			statue[i] = open[i];
		}
	
	}
		
}


void Lcd_Proc(void)
{
	if(uwTick - uwTick_lcd < 50)	return;
	uwTick_lcd = uwTick;
	
	if(setting_flag%2 == 0)
	{

		sprintf((char *)string_lcd, "        Para");
		LCD_DisplayStringLine(Line0, string_lcd);
		
		sprintf((char *)string_lcd, "collect val %.2f", R37_V);
		LCD_DisplayStringLine(Line2, string_lcd);
		
		sprintf((char *)string_lcd, "output statue :%s", statue);
		LCD_DisplayStringLine(Line4, string_lcd);	
		
		sprintf((char *)string_lcd, "singal para:PA9:%2d%% ", (uint32_t)(pwm_duty_output*100));
		LCD_DisplayStringLine(Line6, string_lcd);
		
//		sprintf((char *)string_lcd, "duty :%.2f", pwm_duty_output);
//		LCD_DisplayStringLine(Line4, string_lcd);
		
	
		
		sprintf((char *)string_lcd, "           PB14:%2d%% ", (uint32_t)(pwmn_duty_output*100));
		LCD_DisplayStringLine(Line7, string_lcd);
		
		sprintf((char *)string_lcd, "                %dKHz ", (uint32_t)freq);
		LCD_DisplayStringLine(Line8, string_lcd);
	}

	
	else
	{
		sprintf((char *)string_lcd, "        Setting");
		LCD_DisplayStringLine(Line0, string_lcd);
		
		sprintf((char *)string_lcd, "singal ferq %dKHz ", (uint32_t)freq);
		LCD_DisplayStringLine(Line2, string_lcd);	
	
	}
}








void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
