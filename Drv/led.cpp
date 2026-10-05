#include "led.hpp"

namespace drv
{

void Led::init()
{
    gpio::enableClock(pin_.port);
    write(false);              /* 先给熄灭电平，避免上电瞬间闪一下 */
    gpio::initOutput(pin_);
}

void Led::write(bool on)
{
    on_ = on;
    gpio::write(pin_, activeHigh_ ? on : !on);
}

void Led::on()
{
    write(true);
}

void Led::off()
{
    write(false);
}

void Led::toggle()
{
    gpio::toggle(pin_);
    on_ = !on_;
}

void Led::blink(unsigned times, uint32_t onMs, uint32_t offMs)
{
    while (times-- > 0U)
    {
        on();
        HAL_Delay(onMs);
        off();
        HAL_Delay(offMs);
    }
}

} // namespace drv
