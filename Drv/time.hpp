#ifndef DRV_TIME_HPP
#define DRV_TIME_HPP

#include <cstdint>
#include "main.h"

namespace drv
{

/* ------------------------------------------------------------------
 * 时间抽象
 *
 * 业务层通过它获取时间/延时，不直接依赖 STM32 HAL。
 * 换 MCU、或改用 RTOS tick / 定时器时，只需改这一处。
 * ------------------------------------------------------------------ */
namespace time
{

inline uint32_t millis()
{
    return HAL_GetTick();
}

inline void delayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

} // namespace time
} // namespace drv

#endif /* DRV_TIME_HPP */
