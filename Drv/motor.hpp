#ifndef BSP_MOTOR_HPP
#define BSP_MOTOR_HPP

#include "gpio.hpp"

namespace drv
{

/* ------------------------------------------------------------------
 * TB6612FNG 双路直流电机驱动（TIM4 PWM）
 *
 * 模块引脚与接线（全部在蓝板"右列"排针，都是 5V 容忍引脚）：
 *   VCC  -> 3V3        逻辑电源（必须 3.3V，和 MCU 电平匹配）
 *   GND  -> GND        ← 必须和 MCU 共地
 *   VM   -> 电机电源正极（2.5~13.5V，例如 4 节电池或板上的 5V）
 *   STBY -> 3V3        使能，模块上通常有下拉，不接就是待机（电机不动）
 *   AIN1 -> PB12       A 路方向 1
 *   AIN2 -> PB13       A 路方向 2
 *   PWMA -> PB8        A 路调速（TIM4_CH3）
 *   BIN1 -> PB14       B 路方向 1
 *   BIN2 -> PB15       B 路方向 2
 *   PWMB -> PB9        B 路调速（TIM4_CH4）
 *   AO1/AO2 -> 电机 A 的两根线
 *   BO1/BO2 -> 电机 B 的两根线
 *
 * 控制逻辑（TB6612 真值表，以 A 路为例）：
 *   AIN1  AIN2  PWM      结果
 *    L     H    H        正转（另一方向把 IN1/IN2 对调）
 *    H     L    H        反转
 *    L     L    H        停止（输出高阻，自由滑行）
 *    H     H    H/L      短路刹车
 *
 * 说明：PWM 频率固定 20kHz（高于人耳，电机不会啸叫）。
 *
 * 设计：每个对象代表一路电机，方向脚与 TIM4 通道在构造时确定，
 *       不再用"通道下标 + 运行时越界检查"来分派。
 * ------------------------------------------------------------------ */

/* STBY 用 GPIO 控制（1）还是直接跳线接 3.3V（0，默认） */
#define BSP_MOTOR_USE_STBY      0
#if (BSP_MOTOR_USE_STBY)
#define BSP_MOTOR_STBY_PORT     GPIOA
#define BSP_MOTOR_STBY_PIN      GPIO_PIN_15
#endif

#define BSP_MOTOR_SPEED_MAX     1000    /* 速度量程：-1000 ~ +1000，正负代表方向 */

class Motor
{
public:
    constexpr Motor(GpioPin in1, GpioPin in2, uint8_t timChannel)
        : in1_{in1}, in2_{in2}, channel_{timChannel} {}

    void init();                        /* 方向脚配成推挽输出，先给"停止"电平 */
    void setSpeed(int16_t speed);       /* -1000 ~ +1000，0 = 停止（自由滑行） */
    void stop();                        /* 停止：输出高阻，自由滑行 */
    void brake();                       /* 刹车：短路制动，停得干脆 */

private:
    void writeDuty(uint32_t duty);      /* 写本路 PWM 占空比 */

    GpioPin in1_;
    GpioPin in2_;
    uint8_t channel_;                   /* TIM4 通道号：3 或 4 */
};

/** 初始化两路电机共用的 TIM4 PWM（20kHz），调用一次即可。 */
void motorPwmInit();

#if (BSP_MOTOR_USE_STBY)
/** 0 = 待机（整个模块输出关闭），1 = 使能。 */
void motorEnable(bool on);
#endif

} // namespace drv

#endif /* BSP_MOTOR_HPP */
