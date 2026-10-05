#include "bsp_wiretest.h"
#include "bsp_led.h"
#include "bsp_buzzer.h"

/* 把探针脚按指定上下拉配成输入 */
static void Probe_Pull(uint32_t pull)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin   = BSP_WIRETEST_PROBE_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = pull;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_WIRETEST_PROBE_PORT, &gpio);
}

/**
  * @brief  测一次探针状态
  * @retval BSP_WIRETEST_3V3 / BSP_WIRETEST_GND / BSP_WIRETEST_OPEN
  */
uint8_t BSP_WIRETEST_Probe(void)
{
    uint8_t high_with_pullup;
    uint8_t high_with_pulldown;

    Probe_Pull(GPIO_PULLUP);                    /* 外面接 GND 的话，会被拉低 */
    HAL_Delay(2);
    high_with_pullup = (HAL_GPIO_ReadPin(BSP_WIRETEST_PROBE_PORT, BSP_WIRETEST_PROBE_PIN) == GPIO_PIN_SET) ? 1U : 0U;

    Probe_Pull(GPIO_PULLDOWN);                  /* 外面接 3V3 的话，会被拉高 */
    HAL_Delay(2);
    high_with_pulldown = (HAL_GPIO_ReadPin(BSP_WIRETEST_PROBE_PORT, BSP_WIRETEST_PROBE_PIN) == GPIO_PIN_SET) ? 1U : 0U;

    if (high_with_pullup && high_with_pulldown)
    {
        return BSP_WIRETEST_3V3;                    /* 被外部驱动为高 */
    }
    if (!high_with_pullup && !high_with_pulldown)
    {
        return BSP_WIRETEST_GND;                    /* 被外部拉低 */
    }
    return BSP_WIRETEST_OPEN;                       /* 悬空 / 开路 */
}

/**
  * @brief  接线自检主循环（不返回）
  *
  * 上电后把被测信号线 PA6 固定为高电平：
  *   - 如果 I/O 线接好，有源模块会一直响（这本身就是一条证据）
  *   - 这样探针碰到模块的 I/O 脚或 VCC 脚，都应该读到 3.3V
  */
void BSP_WIRETEST_Run(void)
{
    /* 信号脚持续给"有效电平"（按 BSP_BUZZER_ACTIVE_LEVEL 决定高低）
     *   -> I/O 线接通的话，蜂鸣器会一直响，这本身就是 I/O 线的判据：
     *      一直响 = I/O 线通 ；一直不响 = I/O 线断 */
    BSP_BUZZER_Init();
    BSP_BUZZER_On();

    /* 探针脚先配成输入 */
    Probe_Pull(GPIO_PULLUP);

    while (1)
    {
        uint8_t  state = BSP_WIRETEST_Probe();
        uint32_t tick  = HAL_GetTick();
        uint8_t  on;

        if (state == BSP_WIRETEST_3V3)
        {
            on = 1U;                                    /* 常亮 */
        }
        else if (state == BSP_WIRETEST_GND)
        {
            on = ((tick / 500U) & 1U) ? 0U : 1U;        /* 慢闪 1Hz */
        }
        else
        {
            on = ((tick / 60U) & 1U) ? 0U : 1U;         /* 急快闪 ~8Hz */
        }

        if (on)
        {
            BSP_LED_On();
            BSP_LED2_On();
        }
        else
        {
            BSP_LED_Off();
            BSP_LED2_Off();
        }
    }
}
