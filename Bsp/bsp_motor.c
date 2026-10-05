#include "bsp_motor.h"

/* PWM 频率：20kHz，高于人耳听阈，电机不会啸叫 */
#define BSP_MOTOR_PWM_HZ     20000U

/* 两路电机的引脚分配 */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      in1;      /* 方向脚 1 */
    uint16_t      in2;      /* 方向脚 2 */
    uint16_t      pwm_pin;  /* 复用为定时器通道的引脚 */
    uint8_t       ch;       /* TIM4 通道号（3 或 4） */
} MotorPin_t;

static const MotorPin_t g_motor[2] =
{
    { GPIOB, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_8, 3U },   /* A: AIN1/AIN2/PWMA */
    { GPIOB, GPIO_PIN_14, GPIO_PIN_15, GPIO_PIN_9, 4U },   /* B: BIN1/BIN2/PWMB */
};

/**
  * @brief  写某一路的 PWM 占空比（0 ~ ARR+1）
  */
static void Motor_WriteDuty(uint8_t ch, uint32_t duty)
{
    if (ch == 0U)
    {
        TIM4->CCR3 = (uint16_t)duty;
    }
    else
    {
        TIM4->CCR4 = (uint16_t)duty;
    }
}

/**
  * @brief  初始化方向脚、PWM 脚和 TIM4
  */
void BSP_MOTOR_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t clk, arr, i;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* 方向脚：推挽输出，先给"停止"电平(L/L) */
    for (i = 0U; i < 2U; i++)
    {
        HAL_GPIO_WritePin(g_motor[i].port, g_motor[i].in1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(g_motor[i].port, g_motor[i].in2, GPIO_PIN_RESET);

        gpio.Pin   = (uint32_t)(g_motor[i].in1 | g_motor[i].in2);
        gpio.Mode  = GPIO_MODE_OUTPUT_PP;
        gpio.Pull  = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(g_motor[i].port, &gpio);
    }

    /* PWM 脚：复用推挽（TIM4_CH3 = PB8，TIM4_CH4 = PB9） */
    gpio.Pin   = (uint32_t)(g_motor[0].pwm_pin | g_motor[1].pwm_pin);
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

#if (BSP_MOTOR_USE_STBY)
    HAL_GPIO_WritePin(BSP_MOTOR_STBY_PORT, BSP_MOTOR_STBY_PIN, GPIO_PIN_RESET);
    gpio.Pin  = BSP_MOTOR_STBY_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(BSP_MOTOR_STBY_PORT, &gpio);
#endif

    /* 定时器时钟（APB1 分频不为 1 时定时器时钟 = PCLK1 x2） */
    clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        clk *= 2U;
    }

    /* 不预分频，直接按 20kHz 算周期，这样占空比分辨率最高 */
    arr = clk / BSP_MOTOR_PWM_HZ;
    if (arr == 0U)      { arr = 1U; }
    if (arr > 65536U)   { arr = 65536U; }   /* 16 位定时器上限 */
    TIM4->PSC = 0U;
    TIM4->ARR = (uint16_t)(arr - 1U);

    /* CH3/CH4 都配成 PWM 模式 1 + 预装载 */
    TIM4->CCMR2 = (uint16_t)((TIM4->CCMR2 & (uint16_t)~(TIM_CCMR2_OC3M | TIM_CCMR2_OC4M))
                             | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2
                             | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2
                             | TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE);
    TIM4->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC4E);
    TIM4->CR1  |= TIM_CR1_ARPE;
    TIM4->EGR  |= TIM_EGR_UG;               /* 立刻装载 ARR */
    TIM4->CCR3  = 0U;
    TIM4->CCR4  = 0U;
    TIM4->CR1  |= TIM_CR1_CEN;              /* 计数器常开，靠 CCR 控制 */

#if (BSP_MOTOR_USE_STBY)
    BSP_MOTOR_Enable(1U);
#endif
}

/**
  * @brief  设置某一路的速度与方向
  * @param  speed -1000 ~ +1000；正负代表方向，0 = 停止（自由滑行）
  */
void BSP_MOTOR_SetSpeed(uint8_t ch, int16_t speed)
{
    uint32_t arr, duty;

    if (ch > 1U)
    {
        return;
    }
    if (speed > BSP_MOTOR_SPEED_MAX)  { speed = BSP_MOTOR_SPEED_MAX;  }
    if (speed < -BSP_MOTOR_SPEED_MAX) { speed = -BSP_MOTOR_SPEED_MAX; }

    if (speed == 0)
    {
        BSP_MOTOR_Stop(ch);
        return;
    }

    if (speed > 0)
    {
        /* 正转：IN1 = L, IN2 = H */
        HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in2, GPIO_PIN_SET);
    }
    else
    {
        /* 反转：IN1 = H, IN2 = L */
        HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in2, GPIO_PIN_RESET);
        speed = (int16_t)(-speed);
    }

    arr  = (uint32_t)TIM4->ARR + 1U;
    duty = ((uint32_t)speed * arr) / (uint32_t)BSP_MOTOR_SPEED_MAX;
    if (duty > (uint32_t)TIM4->ARR)
    {
        duty = (uint32_t)TIM4->ARR;         /* 100% 时给满 */
    }
    Motor_WriteDuty(ch, duty);
}

/**
  * @brief  停止（IN1 = IN2 = L，输出高阻，电机自由滑行）
  */
void BSP_MOTOR_Stop(uint8_t ch)
{
    if (ch > 1U)
    {
        return;
    }
    Motor_WriteDuty(ch, 0U);
    HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in2, GPIO_PIN_RESET);
}

/**
  * @brief  刹车（IN1 = IN2 = H，短路制动，停得干脆）
  */
void BSP_MOTOR_Brake(uint8_t ch)
{
    if (ch > 1U)
    {
        return;
    }
    Motor_WriteDuty(ch, 0U);
    HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_motor[ch].port, g_motor[ch].in2, GPIO_PIN_SET);
}

void BSP_MOTOR_StopAll(void)
{
    BSP_MOTOR_Stop(BSP_MOTOR_A);
    BSP_MOTOR_Stop(BSP_MOTOR_B);
}

#if (BSP_MOTOR_USE_STBY)
void BSP_MOTOR_Enable(uint8_t on)
{
    HAL_GPIO_WritePin(BSP_MOTOR_STBY_PORT, BSP_MOTOR_STBY_PIN,
                      (on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif
