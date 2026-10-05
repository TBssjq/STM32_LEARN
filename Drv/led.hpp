#ifndef BSP_LED_HPP
#define BSP_LED_HPP

#include "gpio.hpp"

namespace drv
{

/* ------------------------------------------------------------------
 * 一个 LED：把"端口 / 引脚 / 有效电平"打包成对象。
 *
 * 原来 LED1(外部, 高电平点亮) 和 LED2(板载, 低电平点亮) 是两份几乎
 * 相同的代码；现在共用同一个类，只是构造参数不同，增删 LED 只改一行。
 * ------------------------------------------------------------------ */
class Led
{
public:
    constexpr Led(GpioPin pin, bool activeHigh)
        : pin_{pin}, activeHigh_{activeHigh} {}

    void init();                                          /* 配成推挽输出并熄灭 */
    void on();
    void off();
    void toggle();
    void blink(unsigned times, uint32_t onMs, uint32_t offMs);

private:
    void write(bool on);                                  /* 按有效电平输出 */

    GpioPin pin_;
    bool    activeHigh_;
    bool    on_{false};
};

} // namespace drv

#endif /* BSP_LED_HPP */
