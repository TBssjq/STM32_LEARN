#include "motor.hpp"

namespace drv
{

namespace
{
/* PWM 频率：20kHz，高于人耳听阈，电机不会啸叫 */
constexpr uint32_t kPwmHz    = 20000U;
constexpr int16_t  kSpeedMax = BSP_MOTOR_SPEED_MAX;

/* TIM4 通道 -> CCR 寄存器（本驱动只用 CH3/CH4） */
void writeChannelDuty(uint8_t channel, uint32_t duty)
{
    if (channel == 3U)
    {
        TIM4->CCR3 = static_cast<uint16_t>(duty);
    }
    else
    {
        TIM4->CCR4 = static_cast<uint16_t>(duty);
    }
}
} // namespace

void motorPwmInit()
{
    gpio::enableClock(GPIOB);
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* PWM 脚：复用推挽（TIM4_CH3 = PB8，TIM4_CH4 = PB9） */
    gpio::initAlternate(GpioPin{GPIOB, GPIO_PIN_8});
    gpio::initAlternate(GpioPin{GPIOB, GPIO_PIN_9});

#if (BSP_MOTOR_USE_STBY)
    gpio::enableClock(BSP_MOTOR_STBY_PORT);
    gpio::write(GpioPin{BSP_MOTOR_STBY_PORT, BSP_MOTOR_STBY_PIN}, false);
    gpio::initOutput(GpioPin{BSP_MOTOR_STBY_PORT, BSP_MOTOR_STBY_PIN});
#endif

    /* 定时器时钟（APB1 分频不为 1 时定时器时钟 = PCLK1 x2） */
    uint32_t clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        clk *= 2U;
    }

    /* 不预分频，直接按 20kHz 算周期，这样占空比分辨率最高 */
    uint32_t arr = clk / kPwmHz;
    if (arr == 0U)      { arr = 1U; }
    if (arr > 65536U)   { arr = 65536U; }   /* 16 位定时器上限 */

    TIM4->PSC = 0U;
    TIM4->ARR = static_cast<uint16_t>(arr - 1U);

    /* CH3/CH4 都配成 PWM 模式 1 + 预装载 */
    TIM4->CCMR2 = static_cast<uint16_t>((TIM4->CCMR2 & static_cast<uint16_t>(~(TIM_CCMR2_OC3M | TIM_CCMR2_OC4M)))
                                        | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2
                                        | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2
                                        | TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE);
    TIM4->CCER |= static_cast<uint16_t>(TIM_CCER_CC3E | TIM_CCER_CC4E);
    TIM4->CR1  |= TIM_CR1_ARPE;
    TIM4->EGR  |= TIM_EGR_UG;               /* 立刻装载 ARR */
    TIM4->CCR3  = 0U;
    TIM4->CCR4  = 0U;
    TIM4->CR1  |= TIM_CR1_CEN;              /* 计数器常开，靠 CCR 控制 */

#if (BSP_MOTOR_USE_STBY)
    motorEnable(true);
#endif
}

void Motor::init()
{
    /* 方向脚：推挽输出，先给"停止"电平(L/L) */
    gpio::enableClock(in1_.port);
    gpio::enableClock(in2_.port);
    gpio::write(in1_, false);
    gpio::write(in2_, false);
    gpio::initOutput(in1_, GPIO_SPEED_FREQ_HIGH);
    gpio::initOutput(in2_, GPIO_SPEED_FREQ_HIGH);
}

void Motor::writeDuty(uint32_t duty)
{
    writeChannelDuty(channel_, duty);
}

void Motor::setSpeed(int16_t speed)
{
    if (speed > kSpeedMax)  { speed = kSpeedMax; }
    if (speed < -kSpeedMax) { speed = static_cast<int16_t>(-kSpeedMax); }

    if (speed == 0)
    {
        stop();
        return;
    }

    if (speed > 0)
    {
        /* 正转：IN1 = L, IN2 = H */
        gpio::write(in1_, false);
        gpio::write(in2_, true);
    }
    else
    {
        /* 反转：IN1 = H, IN2 = L */
        gpio::write(in1_, true);
        gpio::write(in2_, false);
        speed = static_cast<int16_t>(-speed);
    }

    uint32_t arr  = static_cast<uint32_t>(TIM4->ARR) + 1U;
    uint32_t duty = (static_cast<uint32_t>(speed) * arr) / static_cast<uint32_t>(kSpeedMax);
    if (duty > static_cast<uint32_t>(TIM4->ARR))
    {
        duty = TIM4->ARR;                   /* 100% 时给满 */
    }
    writeDuty(duty);
}

void Motor::stop()
{
    writeDuty(0U);
    gpio::write(in1_, false);
    gpio::write(in2_, false);
}

void Motor::brake()
{
    writeDuty(0U);
    gpio::write(in1_, true);
    gpio::write(in2_, true);
}

#if (BSP_MOTOR_USE_STBY)
void motorEnable(bool on)
{
    gpio::write(GpioPin{BSP_MOTOR_STBY_PORT, BSP_MOTOR_STBY_PIN}, on);
}
#endif

} // namespace drv
