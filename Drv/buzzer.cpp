#include "buzzer.hpp"

namespace drv
{
namespace buzzer
{

namespace
{
/* 无仪器自检：把 PA6 当普通 GPIO 用，依次输出"直流"和"方波"
 *   - 直流段：有源模块（高触发）会持续响
 *   - 方波段：无源模块会响（500Hz 方波）
 * 自检全程不依赖定时器，所以对两种模块都适用 */
void pinsToGpio()
{
    GPIO_InitTypeDef gpioInit = {0};

    /* 先把定时器输出断开，引脚才能当普通 GPIO 用（两种模式都用 TIM3） */
    BSP_BUZZER_TIM->CR1  &= static_cast<uint16_t>(~TIM_CR1_CEN);
    BSP_BUZZER_TIM->CCER &= static_cast<uint16_t>(~TIM_CCER_CC1E);

    gpioInit.Pin   = BSP_BUZZER_PIN;
    gpioInit.Mode  = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull  = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_BUZZER_PORT, &gpioInit);
    HAL_GPIO_WritePin(BSP_BUZZER_PORT, BSP_BUZZER_PIN, GPIO_PIN_RESET);
}
} // namespace

void selfTest()
{
    pinsToGpio();

    for (uint32_t round = 0U; round < 3U; round++)
    {
        /* A 段：直流高电平 1s（有源模块会响） */
        HAL_GPIO_WritePin(BSP_BUZZER_PORT, BSP_BUZZER_PIN, GPIO_PIN_SET);
        HAL_Delay(1000U);
        HAL_GPIO_WritePin(BSP_BUZZER_PORT, BSP_BUZZER_PIN, GPIO_PIN_RESET);
        HAL_Delay(800U);

        /* B 段：500Hz 方波 1s（无源模块会响） */
        for (uint32_t i = 0U; i < 500U; i++)
        {
            HAL_GPIO_WritePin(BSP_BUZZER_PORT, BSP_BUZZER_PIN, GPIO_PIN_SET);
            HAL_Delay(1U);
            HAL_GPIO_WritePin(BSP_BUZZER_PORT, BSP_BUZZER_PIN, GPIO_PIN_RESET);
            HAL_Delay(1U);
        }
        HAL_Delay(1500U);
    }

    init();                             /* 自检完恢复成配置的模式 */
}

#if (BSP_BUZZER_PASSIVE)

namespace
{
/* 取 TIM3 的实际计数时钟频率
 * APB1 预分频不为 1 时，定时器时钟 = PCLK1 x2（F1 的硬性规则），
 * 这里运行时算，换成内部 HSI 或者改主频都不用改驱动。 */
uint32_t timerClock()
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();

    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        pclk1 *= 2U;
    }
    return pclk1;
}
} // namespace

/* 初始化蜂鸣器 PWM：1MHz 计数时钟，PWM 模式 1，占空比 50% */
void init()
{
    GPIO_InitTypeDef gpioInit = {0};

    BSP_BUZZER_TIM_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6 -> TIM3_CH1，复用推挽输出 */
    gpioInit.Pin   = BSP_BUZZER_PIN;
    gpioInit.Mode  = GPIO_MODE_AF_PP;
    gpioInit.Pull  = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_BUZZER_PORT, &gpioInit);

    /* 计数时钟压到 1MHz，这样 ARR 的数值就等于"周期多少微秒" */
    uint32_t psc = timerClock() / 1000000U;
    if (psc == 0U)
    {
        psc = 1U;
    }
    BSP_BUZZER_TIM->PSC = static_cast<uint16_t>(psc - 1U);

    /* 先按默认音调把周期算好 */
    BSP_BUZZER_TIM->ARR = static_cast<uint16_t>((1000000U / BSP_BUZZER_DEFAULT_FREQ) - 1U);

    /* PWM 模式 1 + 预装载，CH1 使能 */
    BSP_BUZZER_TIM->CCMR1 = static_cast<uint16_t>((BSP_BUZZER_TIM->CCMR1 & static_cast<uint16_t>(~TIM_CCMR1_OC1M))
                                                  | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2
                                                  | TIM_CCMR1_OC1PE);
    BSP_BUZZER_TIM->CCER |= static_cast<uint16_t>(TIM_CCER_CC1E);
    BSP_BUZZER_TIM->CR1  |= TIM_CR1_ARPE;
    BSP_BUZZER_TIM->EGR  |= TIM_EGR_UG;         /* 立即把 PSC/ARR 装进影子寄存器 */

    /* 先静音：计数器不跑，输出停在静音电平 */
    BSP_BUZZER_TIM->CR1 &= static_cast<uint16_t>(~TIM_CR1_CEN);
#if (BSP_BUZZER_MUTE_HIGH)
    BSP_BUZZER_TIM->CCR1 = static_cast<uint16_t>(BSP_BUZZER_TIM->ARR + 1U);   /* 恒为有效电平 */
#else
    BSP_BUZZER_TIM->CCR1 = 0U;                                               /* 恒为无效电平(低) */
#endif
}

/* 按指定频率发声，freqHz = 0 则静音 */
void tone(uint32_t freqHz)
{
    if (freqHz < 20U || freqHz > 20000U)
    {
        off();
        return;
    }

    /* 1MHz 计数时钟：周期(us) = 1000000 / f */
    uint32_t arr = 1000000U / freqHz;
    if (arr < 2U)
    {
        arr = 2U;
    }

    BSP_BUZZER_TIM->ARR  = static_cast<uint16_t>(arr - 1U);
    BSP_BUZZER_TIM->CCR1 = static_cast<uint16_t>(arr / 2U);   /* 50% 占空比，最响 */
    BSP_BUZZER_TIM->EGR |= TIM_EGR_UG;                        /* 立刻把新 ARR 装进影子寄存器 */
    BSP_BUZZER_TIM->CR1 |= TIM_CR1_CEN;                       /* 开计数 = 发声 */
}

void off()
{
    BSP_BUZZER_TIM->CR1 &= static_cast<uint16_t>(~TIM_CR1_CEN);
#if (BSP_BUZZER_MUTE_HIGH)
    BSP_BUZZER_TIM->CCR1 = static_cast<uint16_t>(BSP_BUZZER_TIM->ARR + 1U);
#else
    BSP_BUZZER_TIM->CCR1 = 0U;
#endif
}

void on()
{
    tone(BSP_BUZZER_DEFAULT_FREQ);
}

void setVolume(uint8_t percent)
{
    (void)percent;      /* 无源模块的响度由驱动电压决定，软件改不了 */
}

#else   /* ------------------- 有源蜂鸣器（带音量控制） ------------------- */

namespace
{
/* 门控频率：把 I/O 按这个频率做 PWM，占空比就是"音量" */
constexpr uint32_t kGateHz = 100U;

uint8_t s_volume = 100U;     /* 音量 = 占空比(%) */
bool    s_on     = false;    /* 当前是否发声 */

/* 把"开关 + 音量"换算成 CCR1 写进去 */
void apply()
{
    if (!s_on || s_volume == 0U)
    {
        BSP_BUZZER_TIM->CCR1 = 0U;      /* 恒为无效电平 -> 静音 */
    }
    else
    {
        BSP_BUZZER_TIM->CCR1 = static_cast<uint16_t>((static_cast<uint32_t>(BSP_BUZZER_TIM->ARR + 1U) * s_volume) / 100U);
    }
}
} // namespace

void init()
{
    GPIO_InitTypeDef gpioInit = {0};

    BSP_BUZZER_TIM_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6 -> TIM3_CH1 复用推挽 */
    gpioInit.Pin   = BSP_BUZZER_PIN;
    gpioInit.Mode  = GPIO_MODE_AF_PP;
    gpioInit.Pull  = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_BUZZER_PORT, &gpioInit);

    /* 计数时钟压到 1MHz */
    uint32_t psc = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        psc *= 2U;                  /* APB1 分频不为 1 时，定时器时钟 = PCLK1 x2 */
    }
    psc /= 1000000U;
    if (psc == 0U)
    {
        psc = 1U;
    }
    BSP_BUZZER_TIM->PSC = static_cast<uint16_t>(psc - 1U);

    /* 门控周期 */
    BSP_BUZZER_TIM->ARR = static_cast<uint16_t>((1000000U / kGateHz) - 1U);

    /* PWM 模式 1 + 预装载；输出极性按模块触发极性设置：
       低触发时把输出反相，这样"占空比 = 拉低的时间比例" */
#if (BSP_BUZZER_ACTIVE_LEVEL)
    BSP_BUZZER_TIM->CCER = TIM_CCER_CC1E;                   /* 高触发：不反相 */
#else
    BSP_BUZZER_TIM->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P;   /* 低触发：反相 */
#endif
    BSP_BUZZER_TIM->CCMR1 = static_cast<uint16_t>((BSP_BUZZER_TIM->CCMR1 & static_cast<uint16_t>(~TIM_CCMR1_OC1M))
                                                  | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2
                                                  | TIM_CCMR1_OC1PE);
    BSP_BUZZER_TIM->CR1 |= TIM_CR1_ARPE;
    BSP_BUZZER_TIM->EGR |= TIM_EGR_UG;      /* 立刻把 PSC/ARR 装进影子寄存器 */

    s_on     = false;
    s_volume = 100U;
    apply();                                /* CCR1 = 0 -> 静音 */

    BSP_BUZZER_TIM->CR1 |= TIM_CR1_CEN;     /* 计数器常开，靠 CCR1 控制发声 */
}

void setVolume(uint8_t percent)
{
    if (percent > 100U)
    {
        percent = 100U;
    }
    s_volume = percent;
    apply();
}

void on()
{
    s_on = true;
    apply();
}

void off()
{
    s_on = false;
    apply();
}

void tone(uint32_t freqHz)
{
    (void)freqHz;                       /* 有源模块音调固定，只当开关用 */
    if (freqHz == 0U)
    {
        off();
    }
    else
    {
        on();
    }
}

#endif  /* BSP_BUZZER_PASSIVE */

/* ---------------------- 通用接口（两种模块都能用） ---------------------- */

void beep(uint32_t ms)
{
    tone(BSP_BUZZER_DEFAULT_FREQ);
    HAL_Delay(ms);
    off();
}

void beepN(uint8_t times, uint32_t onMs, uint32_t offMs)
{
    while (times-- > 0U)
    {
        tone(BSP_BUZZER_DEFAULT_FREQ);
        HAL_Delay(onMs);
        off();
        HAL_Delay(offMs);
    }
}

void playNote(uint16_t freqHz, uint32_t ms)
{
    if (freqHz == NOTE_REST)
    {
        off();
        HAL_Delay(ms);
        return;
    }

    tone(freqHz);
    HAL_Delay(static_cast<uint32_t>(ms * 9U / 10U));   /* 留 10% 空隙，音符之间才分得开 */
    off();
    HAL_Delay(static_cast<uint32_t>(ms / 10U));
}

} // namespace buzzer
} // namespace drv
