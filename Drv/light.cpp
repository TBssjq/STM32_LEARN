#include "light.hpp"
#include "gpio.hpp"

namespace drv
{
namespace light
{

/* 初始化：AO 配成模拟输入，DO 配成带上拉的数字输入，并配好 ADC1 */
void init()
{
    gpio::initAnalog(GpioPin{BSP_LIGHT_AO_PORT, BSP_LIGHT_AO_PIN});
    gpio::initInput(GpioPin{BSP_LIGHT_DO_PORT, BSP_LIGHT_DO_PIN}, GPIO_PULLUP);

    /* ADC 时钟：APB2 再分频，必须 <= 14MHz（72/6 = 12MHz） */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV6;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ADC1：单通道、软件触发、右对齐 */
    ADC1->CR1  = 0x00000000U;
    ADC1->CR2  = 0x00000000U;
    ADC1->SQR1 = 0x00000000U;                               /* 规则组只转 1 个通道 */
    ADC1->SQR3 = BSP_LIGHT_AO_CHANNEL;                      /* 第一个转换的通道 */

    /* 采样时间取最长档(239.5 周期)：分压电阻阻抗偏高，采久一点更稳
       注意：两个分支都会被编译，所以移位量必须先算成安全的下标，
       不能只在对应分支里做减法（否则另一个分支会出现负数移位） */
    {
        uint32_t pos = (BSP_LIGHT_AO_CHANNEL < 10U) ? static_cast<uint32_t>(BSP_LIGHT_AO_CHANNEL)
                                                    : static_cast<uint32_t>(BSP_LIGHT_AO_CHANNEL - 10U);
        uint32_t smp = static_cast<uint32_t>(0x07U << (pos * 3U));

        if (BSP_LIGHT_AO_CHANNEL < 10U)
        {
            ADC1->SMPR2 |= smp;
        }
        else
        {
            ADC1->SMPR1 |= smp;
        }
    }

    ADC1->CR2 |= ADC_CR2_ADON;                              /* 给 ADC 上电 */

    /* 校准 */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0U) { }
    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0U) { }

    (void)readRaw();                                        /* 第一次转换丢掉 */
}

/* 读一次 ADC，返回 0~4095 */
uint16_t readRaw()
{
    ADC1->CR2 |= ADC_CR2_ADON;                              /* 软件启动转换 */
    while ((ADC1->SR & ADC_SR_EOC) == 0U) { }               /* 等转换完成 */
    return static_cast<uint16_t>(ADC1->DR & 0x0FFFU);
}

/* 多次采样取平均，抗抖动 */
uint16_t readAvg(uint8_t samples)
{
    uint32_t sum = 0U;

    if (samples == 0U)
    {
        samples = 1U;
    }
    for (uint8_t i = 0U; i < samples; i++)
    {
        sum += readRaw();
    }
    return static_cast<uint16_t>(sum / samples);
}

/* 换算成毫伏（VREF = 3.3V） */
uint16_t readMv()
{
    return static_cast<uint16_t>((static_cast<uint32_t>(readAvg(8U)) * 3300U) / 4096U);
}

/* 亮度百分比 0~100 */
uint8_t getPercent()
{
    uint32_t pct = (static_cast<uint32_t>(readAvg(8U)) * 100U) / 4096U;

#if (BSP_LIGHT_INVERT)
    pct = 100U - pct;
#endif
    return static_cast<uint8_t>(pct);
}

uint8_t readDO()
{
    return gpio::read(GpioPin{BSP_LIGHT_DO_PORT, BSP_LIGHT_DO_PIN}) ? 1U : 0U;
}

bool isTriggered()
{
    return readDO() == (BSP_LIGHT_DO_ACTIVE != 0 ? 1U : 0U);
}

} // namespace light
} // namespace drv
