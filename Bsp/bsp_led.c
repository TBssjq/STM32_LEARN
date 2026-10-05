#include "bsp_led.h"

/**
  * @brief  使能 GPIO 端口时钟
  */
static void Led_EnableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
}

/* ------------------------- LED1：外部 LED ------------------------- */

void BSP_LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    Led_EnableClock(BSP_LED_GPIO_PORT);

    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_RESET);  /* 先灭 */

    GPIO_InitStruct.Pin   = BSP_LED_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_LED_GPIO_PORT, &GPIO_InitStruct);
}

void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_SET);
}

void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN, GPIO_PIN_RESET);
}

void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT, BSP_LED_GPIO_PIN);
}

void BSP_LED_Blink(uint8_t times, uint32_t on_ms, uint32_t off_ms)
{
    while (times--)
    {
        BSP_LED_On();
        HAL_Delay(on_ms);
        BSP_LED_Off();
        HAL_Delay(off_ms);
    }
}

/* ------------------------- LED2：板上 LED ------------------------- */

void BSP_LED2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    Led_EnableClock(BSP_LED2_GPIO_PORT);

#if (BSP_LED2_ACTIVE_LOW)
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_SET);   /* 先灭 */
#else
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_RESET);
#endif

    GPIO_InitStruct.Pin   = BSP_LED2_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_LED2_GPIO_PORT, &GPIO_InitStruct);
}

void BSP_LED2_On(void)
{
#if (BSP_LED2_ACTIVE_LOW)
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_SET);
#endif
}

void BSP_LED2_Off(void)
{
#if (BSP_LED2_ACTIVE_LOW)
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN, GPIO_PIN_RESET);
#endif
}

void BSP_LED2_Toggle(void)
{
    HAL_GPIO_TogglePin(BSP_LED2_GPIO_PORT, BSP_LED2_GPIO_PIN);
}
