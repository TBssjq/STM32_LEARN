#ifndef BSP_GPIO_HPP
#define BSP_GPIO_HPP

#include "main.h"

/* ------------------------------------------------------------------
 * GPIO 底层封装
 *
 * 把"端口时钟使能 / 引脚模式配置 / 读写"收敛到一处。
 * 原来每个驱动都自带一份"端口 -> 时钟"的 switch，现在只需一份，
 * 新增器件时不会再复制样板代码。
 * ------------------------------------------------------------------ */
namespace drv
{

/** 一个 GPIO 引脚的描述：端口 + 引脚位掩码。 */
struct GpioPin
{
    GPIO_TypeDef *port;
    uint16_t      pin;
};

namespace gpio
{

/** 使能某个 GPIO 端口时钟（对任意端口重复调用都安全）。 */
inline void enableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
#if defined(GPIOB)
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
#endif
#if defined(GPIOC)
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
#endif
#if defined(GPIOD)
    else if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
#endif
#if defined(GPIOE)
    else if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
#endif
}

/** 通用配置入口。 */
inline void configure(GpioPin pin, uint32_t mode, uint32_t pull, uint32_t speed)
{
    GPIO_InitTypeDef init = {0};

    init.Pin   = pin.pin;
    init.Mode  = mode;
    init.Pull  = pull;
    init.Speed = speed;
    HAL_GPIO_Init(pin.port, &init);
}

/** 配成推挽输出（自动使能时钟）。 */
inline void initOutput(GpioPin pin, uint32_t speed = GPIO_SPEED_FREQ_LOW)
{
    enableClock(pin.port);
    configure(pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, speed);
}

/** 配成输入（自动使能时钟）。 */
inline void initInput(GpioPin pin, uint32_t pull = GPIO_NOPULL)
{
    enableClock(pin.port);
    configure(pin, GPIO_MODE_INPUT, pull, GPIO_SPEED_FREQ_LOW);
}

/** 配成模拟输入（ADC 用）。 */
inline void initAnalog(GpioPin pin)
{
    enableClock(pin.port);
    configure(pin, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
}

/** 配成复用推挽输出（定时器/PWM 用）。 */
inline void initAlternate(GpioPin pin, uint32_t speed = GPIO_SPEED_FREQ_HIGH)
{
    enableClock(pin.port);
    configure(pin, GPIO_MODE_AF_PP, GPIO_NOPULL, speed);
}

inline void write(GpioPin pin, bool high)
{
    HAL_GPIO_WritePin(pin.port, pin.pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

inline bool read(GpioPin pin)
{
    return HAL_GPIO_ReadPin(pin.port, pin.pin) == GPIO_PIN_SET;
}

inline void toggle(GpioPin pin)
{
    HAL_GPIO_TogglePin(pin.port, pin.pin);
}

} // namespace gpio
} // namespace drv

#endif /* BSP_GPIO_HPP */
