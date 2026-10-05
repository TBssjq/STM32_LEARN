#include "board.hpp"

/* 板级接线（改接线只改这里）：
 *   LED1 -> PA0 （外部 LED，输出高电平点亮）
 *   LED2 -> PC13（板载 LED，低电平点亮）
 *   MOTOR A：AIN1=PB12, AIN2=PB13, PWMA=PB8(TIM4_CH3)
 *   MOTOR B：BIN1=PB14, BIN2=PB15, PWMB=PB9(TIM4_CH4)
 */
namespace bsp
{

drv::Led led1{drv::GpioPin{GPIOA, GPIO_PIN_0}, true};
drv::Led led2{drv::GpioPin{GPIOC, GPIO_PIN_13}, false};

drv::Motor motorA{drv::GpioPin{GPIOB, GPIO_PIN_12}, drv::GpioPin{GPIOB, GPIO_PIN_13}, 3U};
drv::Motor motorB{drv::GpioPin{GPIOB, GPIO_PIN_14}, drv::GpioPin{GPIOB, GPIO_PIN_15}, 4U};

} // namespace bsp
