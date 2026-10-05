#ifndef APP_BOMB_HPP
#define APP_BOMB_HPP

namespace app
{
namespace bomb
{

/* ------------------------------------------------------------------
 * 定时炸弹演示（循环执行，不会返回）
 *
 *   OLED   : 标题 "TIME BOMB" + 倒计时 MM:SS + 进度条
 *   蜂鸣器 : 滴答声，越接近 0 越急促；到 0 爆炸，3 秒后重新开始
 *   LED    : 板载 LED 心跳闪烁；爆炸时两个 LED 一起闪
 * ------------------------------------------------------------------ */
void run();

} // namespace bomb
} // namespace app

#endif /* APP_BOMB_HPP */
