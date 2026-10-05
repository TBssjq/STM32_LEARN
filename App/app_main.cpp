#include "app_main.h"

#include <cstdint>

#include "board.hpp"
#include "buzzer.hpp"
#include "light.hpp"
#include "motor.hpp"
#include "irq.hpp"
#include "bsp_oled.h"
#include "wiretest.hpp"
#include "app_demo.hpp"
#include "app_bomb.hpp"

/* ------------------------------------------------------------------
 * 业务入口（App 层：硬件无关）
 *
 * 新功能一律放到 App/ 或对应下层模块里，不要堆进 main.c；
 * App 层只依赖 Bsp/ 的对象与 Drv/ 的抽象，不直接操作寄存器、不调用 HAL。
 * ------------------------------------------------------------------ */
void APP_Main(void)
{
    /* 当前只用 TB6612 电机模块，其余外设暂不初始化（要用时取消注释） */
    bsp::led1.init();           /* 外部 LED(PA0) */
    bsp::led2.init();           /* 板上 LED(PC13) */

    drv::motorPwmInit();        /* TB6612 共用 PWM(TIM4) */
    bsp::motorA.init();         /* A 路方向脚(PB12/PB13) */
    bsp::motorB.init();         /* B 路方向脚(PB14/PB15) */

    // drv::buzzer::init();     /* 蜂鸣器(I/O = PA6) */
    // BSP_OLED_Init();         /* OLED(I2C1: PB6=SCL, PB7=SDA) */
    // drv::light::init();      /* 光敏传感器(AO=PA1, DO=PA2) */

    app::demo::motorTest();     /* TB6612 双电机演示（当前） */
    // app::demo::lightSensor();  /* 光敏 + OLED 实时显示 */
    // app::demo::buzzerMelody(); /* 蜂鸣器《欢乐颂》+ SOS */
    // app::bomb::run();          /* 定时炸弹 */
    // bsp::wiretest::run();      /* 接线自检（探针插 PB5） */
}

/* ------------------------------------------------------------------
 * 错误陷阱：HAL/时钟/外设初始化失败时进入，两个 LED 慢闪提示
 *
 * 中断已关，不能用延时（SysTick 不再更新），所以用忙等循环当延时。
 * ------------------------------------------------------------------ */
void APP_ErrorTrap(void)
{
    drv::irq::disable();
    bsp::led1.init();
    bsp::led2.init();

    while (1)
    {
        for (volatile uint32_t i = 0U; i < 500000U; i++) { }   /* 约 0.2s */
        bsp::led1.toggle();
        bsp::led2.toggle();
    }
}
