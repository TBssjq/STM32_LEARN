#ifndef DRV_IRQ_HPP
#define DRV_IRQ_HPP

#include "main.h"

namespace drv
{

/* ------------------------------------------------------------------
 * 中断控制抽象
 * ------------------------------------------------------------------ */
namespace irq
{

/* 关中断：错误陷阱用（此时 SysTick 已停，不能再用延时）。 */
inline void disable()
{
    __disable_irq();
}

} // namespace irq
} // namespace drv

#endif /* DRV_IRQ_HPP */
