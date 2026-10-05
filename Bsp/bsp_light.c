#include "bsp_light.h"

/**
  * @brief  初始化：AO 配成模拟输入，DO 配成带上拉的数字输入，并配好 ADC1
  */
void BSP_LIGHT_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* AO -> 模拟输入 */
    gpio.Pin  = BSP_LIGHT_AO_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BSP_LIGHT_AO_PORT, &gpio);

    /* DO -> 数字输入 + 内部上拉（LM393 是开漏输出） */
    gpio.Pin  = BSP_LIGHT_DO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_LIGHT_DO_PORT, &gpio);

    /* ADC 时钟：APB2 再分频，必须 <= 14MHz（72/6 = 12MHz） */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV6;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ADC1：单通道、软件触发、右对齐 */
    ADC1->CR1  = 0x00000000U;
    ADC1->CR2  = 0x00000000U;
    ADC1->SQR1 = 0x00000000U;                               /* 规则组只转 1 个通道 */
    ADC1->SQR3 = BSP_LIGHT_AO_CHANNEL;                          /* 第一个转换的通道 */

    /* 采样时间取最长档(239.5 周期)：分压电阻阻抗偏高，采久一点更稳
       注意：两个分支都会被编译，所以移位量必须先算成安全的下标，
       不能只在对应分支里做减法（否则另一个分支会出现负数移位） */
    {
        uint32_t pos = (BSP_LIGHT_AO_CHANNEL < 10U) ? (uint32_t)BSP_LIGHT_AO_CHANNEL
                                                : (uint32_t)(BSP_LIGHT_AO_CHANNEL - 10U);
        uint32_t smp = (uint32_t)(0x07U << (pos * 3U));

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

    (void)BSP_LIGHT_ReadRaw();                                  /* 第一次转换丢掉 */
}

/**
  * @brief  读一次 ADC，返回 0~4095
  */
uint16_t BSP_LIGHT_ReadRaw(void)
{
    ADC1->CR2 |= ADC_CR2_ADON;                              /* 软件启动转换 */
    while ((ADC1->SR & ADC_SR_EOC) == 0U) { }               /* 等转换完成 */
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/**
  * @brief  多次采样取平均，抗抖动
  */
uint16_t BSP_LIGHT_ReadAvg(uint8_t samples)
{
    uint32_t sum = 0U;
    uint8_t  i;

    if (samples == 0U)
    {
        samples = 1U;
    }
    for (i = 0; i < samples; i++)
    {
        sum += BSP_LIGHT_ReadRaw();
    }
    return (uint16_t)(sum / samples);
}

/**
  * @brief  换算成毫伏（VREF = 3.3V）
  */
uint16_t BSP_LIGHT_ReadMv(void)
{
    return (uint16_t)(((uint32_t)BSP_LIGHT_ReadAvg(8U) * 3300U) / 4096U);
}

/**
  * @brief  亮度百分比 0~100
  */
uint8_t BSP_LIGHT_GetPercent(void)
{
    uint32_t pct = ((uint32_t)BSP_LIGHT_ReadAvg(8U) * 100U) / 4096U;

#if (BSP_LIGHT_INVERT)
    pct = 100U - pct;
#endif
    return (uint8_t)pct;
}

uint8_t BSP_LIGHT_ReadDO(void)
{
    return (HAL_GPIO_ReadPin(BSP_LIGHT_DO_PORT, BSP_LIGHT_DO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

uint8_t BSP_LIGHT_IsTriggered(void)
{
    return (BSP_LIGHT_ReadDO() == BSP_LIGHT_DO_ACTIVE) ? 1U : 0U;
}
