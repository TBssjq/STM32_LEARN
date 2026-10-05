#include "app_main.h"
#include "bsp_led.h"
#include "bsp_buzzer.h"
#include "bsp_oled.h"
#include "bsp_light.h"
#include "bsp_motor.h"
#include "bsp_wiretest.h"
#include "app_demo.h"
#include "app_bomb.h"

/**
  * @brief  业务入口：初始化所有 BSP 模块，然后进入选定的演示
  *
  * 新功能一律放到 App/ 或 Bsp/ 的对应模块里，不要堆进 main.c；
  * 业务层只调用 Bsp/ 暴露的接口，不直接操作寄存器。
  */
void APP_Main(void)
{
    /* 当前只用 TB6612 电机模块，其余外设暂不初始化（要用时取消注释） */
    BSP_LED_Init();         /* 外部 LED(PA0) + 板上 LED(PC13) */
    BSP_MOTOR_Init();       /* TB6612 电机(PB8/PB9 调速, PB12~PB15 方向) */
    // BSP_BUZZER_Init();   /* 蜂鸣器(I/O = PA6) */
    // BSP_OLED_Init();     /* OLED(I2C1: PB6=SCL, PB7=SDA) */
    // BSP_LIGHT_Init();    /* 光敏传感器(AO=PA1, DO=PA2) */

    APP_DEMO_MotorTest();       /* TB6612 双电机演示（当前） */
    // APP_DEMO_LightSensor();  /* 光敏 + OLED 实时显示 */
    // APP_DEMO_BuzzerMelody(); /* 蜂鸣器《欢乐颂》+ SOS */
    // APP_BOMB_Run();          /* 定时炸弹 */
    // BSP_WIRETEST_Run();      /* 接线自检（探针插 PB5） */
}

/**
  * @brief  错误陷阱：HAL/时钟/外设初始化失败时进入，两个 LED 慢闪提示
  *
  * 中断已关，不能用 HAL_Delay（SysTick 不再更新），所以用忙等循环当延时。
  */
void APP_ErrorTrap(void)
{
    __disable_irq();
    BSP_LED_Init();
    BSP_LED2_Init();

    while (1)
    {
        for (volatile uint32_t i = 0U; i < 500000U; i++) { }   /* 约 0.2s */
        BSP_LED_Toggle();
        BSP_LED2_Toggle();
    }
}
