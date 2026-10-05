#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

/* ------------------------------------------------------------------
 * TB6612FNG 双路直流电机驱动
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
 *    L     H    L        短路刹车
 *
 * 说明：PWM 频率固定 20kHz（高于人耳，电机不会啸叫）。
 * ------------------------------------------------------------------ */

#define BSP_MOTOR_A             0
#define BSP_MOTOR_B             1

#define BSP_MOTOR_SPEED_MAX     1000    /* 速度量程：-1000 ~ +1000，正负代表方向 */

/* STBY 用 GPIO 控制（1）还是直接跳线接 3.3V（0，默认） */
#define BSP_MOTOR_USE_STBY      0
#if (BSP_MOTOR_USE_STBY)
#define BSP_MOTOR_STBY_PORT     GPIOA
#define BSP_MOTOR_STBY_PIN      GPIO_PIN_15
#endif

void BSP_MOTOR_Init(void);
void BSP_MOTOR_SetSpeed(uint8_t ch, int16_t speed);  /* -1000~1000，0 = 停止 */
void BSP_MOTOR_Stop(uint8_t ch);                     /* 停止：输出高阻，自由滑行 */
void BSP_MOTOR_Brake(uint8_t ch);                    /* 刹车：短路制动，停得干脆 */
void BSP_MOTOR_StopAll(void);

#if (BSP_MOTOR_USE_STBY)
void BSP_MOTOR_Enable(uint8_t on);                   /* 0 = 待机（整个模块输出关闭） */
#endif

#endif /* __MOTOR_H */
