#ifndef APP_DEMO_HPP
#define APP_DEMO_HPP

/* ------------------------------------------------------------------
 * 各种演示程序（都是"不返回"的循环，主程序里选一个调用即可）
 *   换演示只改 app_main.cpp 里那一行
 * ------------------------------------------------------------------ */
namespace app
{
namespace demo
{

void lightSensor();     /* 光敏传感器 + OLED 实时显示 */
void motorTest();       /* TB6612 双电机正反转/调速/刹车演示 */
void buzzerMelody();    /* 蜂鸣器《欢乐颂》+ SOS */

} // namespace demo
} // namespace app

#endif /* APP_DEMO_HPP */
