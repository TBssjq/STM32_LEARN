#ifndef BSP_BOARD_HPP
#define BSP_BOARD_HPP

#include "gpio.hpp"
#include "led.hpp"
#include "motor.hpp"

namespace bsp
{

/* ------------------------------------------------------------------
 * 板级对象图（Bsp 层：本板专有）
 *
 * 用 Drv 层的通用驱动类，实例化出这块板子上的具体器件。
 * 所有"物理器件实例"只在这里声明、在 board.cpp 里构造：
 * 引脚分配只有一处定义，看接线、改接线都不用翻遍各个驱动。
 * 换板子只改 board.cpp，Drv/ 与 App/ 不动。
 * ------------------------------------------------------------------ */

extern drv::Led   led1;        /* 外部 LED：PA0，高电平点亮 */
extern drv::Led   led2;        /* 板载 LED：PC13，低电平点亮 */

extern drv::Motor motorA;      /* TB6612 A 路：PB12/PB13 方向，PB8(TIM4_CH3) 调速 */
extern drv::Motor motorB;      /* TB6612 B 路：PB14/PB15 方向，PB9(TIM4_CH4) 调速 */

} // namespace bsp

#endif /* BSP_BOARD_HPP */
