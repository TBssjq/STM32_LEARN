#include "wiretest.hpp"
#include "board.hpp"
#include "buzzer.hpp"
#include "gpio.hpp"
#include "time.hpp"

namespace bsp
{
namespace wiretest
{

namespace
{
constexpr drv::GpioPin kProbe{BSP_WIRETEST_PROBE_PORT, BSP_WIRETEST_PROBE_PIN};

/* 把探针脚按指定上下拉配成输入 */
void probePull(uint32_t pull)
{
    drv::gpio::initInput(kProbe, pull);
}
} // namespace

Result probe()
{
    probePull(GPIO_PULLUP);                     /* 外面接 GND 的话，会被拉低 */
    drv::time::delayMs(2);
    bool highWithPullup = drv::gpio::read(kProbe);

    probePull(GPIO_PULLDOWN);                   /* 外面接 3V3 的话，会被拉高 */
    drv::time::delayMs(2);
    bool highWithPulldown = drv::gpio::read(kProbe);

    if (highWithPullup && highWithPulldown)
    {
        return Result::V3v3;                    /* 被外部驱动为高 */
    }
    if (!highWithPullup && !highWithPulldown)
    {
        return Result::Gnd;                     /* 被外部拉低 */
    }
    return Result::Open;                        /* 悬空 / 开路 */
}

/* 接线自检主循环（不返回）
 *
 * 上电后把被测信号线 PA6 固定为有效电平：
 *   - 如果 I/O 线接好，有源模块会一直响（这本身就是一条证据）
 *   - 这样探针碰到模块的 I/O 脚或 VCC 脚，都应该读到 3.3V */
void run()
{
    /* 信号脚持续给"有效电平"（按 BSP_BUZZER_ACTIVE_LEVEL 决定高低）
     *   -> I/O 线接通的话，蜂鸣器会一直响，这本身就是 I/O 线的判据：
     *      一直响 = I/O 线通 ；一直不响 = I/O 线断 */
    drv::buzzer::init();
    drv::buzzer::on();

    probePull(GPIO_PULLUP);

    while (true)
    {
        Result   state = probe();
        uint32_t tick  = drv::time::millis();
        bool     on;

        if (state == Result::V3v3)
        {
            on = true;                                  /* 常亮 */
        }
        else if (state == Result::Gnd)
        {
            on = ((tick / 500U) & 1U) == 0U;            /* 慢闪 1Hz */
        }
        else
        {
            on = ((tick / 60U) & 1U) == 0U;             /* 急快闪 ~8Hz */
        }

        if (on)
        {
            led1.on();
            led2.on();
        }
        else
        {
            led1.off();
            led2.off();
        }
    }
}

} // namespace wiretest
} // namespace bsp
