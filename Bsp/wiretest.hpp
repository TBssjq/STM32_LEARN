#ifndef BSP_WIRETEST_HPP
#define BSP_WIRETEST_HPP

#include "main.h"

namespace bsp
{

/* ------------------------------------------------------------------
 * 面包板接线自检（探针法）
 *
 * 用法：找一根杜邦线，一头插在 PB5（丝印 "B5" 那个脚），另一头空着当"探针"，
 *       拿探针去碰你想检查的点，看 LED 怎么闪。
 *       探针脚选用 PB5 是因为它耐 5V（PB0/PB1 是模拟脚，不耐 5V，
 *       若模块 VCC 接在 5V 上，用 PB0 当探针一碰就会打坏引脚）。
 *
 * 原理：同一个点用"内部上拉"和"内部下拉"各读一次：
 *         上拉读到 1 + 下拉读到 1 = 外部把它驱动成高  -> 这一点接在 3V3 上
 *         上拉读到 0 + 下拉读到 0 = 外部把它拉低      -> 这一点接在 GND 上
 *         上拉读到 1 + 下拉读到 0 = 既不高也不低      -> 悬空 / 开路（线没接通）
 *
 * 指示灯：外部 LED(PA0) 和板上 LED(PC13) 同时亮灭
 *         常亮          = 3V3
 *         慢闪 (1Hz)    = GND
 *         急快闪 (~8Hz) = 开路  <-- 看到这个，就是这根线没接好
 * ------------------------------------------------------------------ */
namespace wiretest
{

#define BSP_WIRETEST_PROBE_PORT   GPIOB
#define BSP_WIRETEST_PROBE_PIN    GPIO_PIN_5   /* 5V 容忍，别改成 PB0/PB1 */

enum class Result : uint8_t
{
    Open = 0,       /* 悬空 / 开路 */
    Gnd  = 1,       /* 接到 GND */
    V3v3 = 2,       /* 接到 3V3 */
};

Result probe();     /* 测一次探针 */
void   run();       /* 进入自检循环（不会返回） */

} // namespace wiretest
} // namespace bsp

#endif /* BSP_WIRETEST_HPP */
