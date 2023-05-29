#include "main.h"

extern TIM_HandleTypeDef htim1;

void PWM_double_TIM1_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

void GPIO_PWM_INIT(void);