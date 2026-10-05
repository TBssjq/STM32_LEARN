#ifndef __LED_H
#define __LED_H

#include "main.h"

/* ------------------------------------------------------------------
 * LED1：外部 LED（需要接线）
 *   GPIO(PA0) ---> LED 阳极(长脚) ---> LED 阴极(短脚) ---> GND
 *   输出高电平点亮
 * ------------------------------------------------------------------ */
#define BSP_LED_GPIO_PORT   GPIOA
#define BSP_LED_GPIO_PIN    GPIO_PIN_0

/* ------------------------------------------------------------------
 * LED2：板上自带 LED（PC13），零接线，低电平点亮
 *   用来做状态/报错指示，没接外部 LED 时也能看到
 * ------------------------------------------------------------------ */
#define BSP_LED2_GPIO_PORT   GPIOC
#define BSP_LED2_GPIO_PIN    GPIO_PIN_13
#define BSP_LED2_ACTIVE_LOW  1

void BSP_LED_Init(void);                   /* 初始化外部 LED */
void BSP_LED_On(void);
void BSP_LED_Off(void);
void BSP_LED_Toggle(void);
void BSP_LED_Blink(uint8_t times, uint32_t on_ms, uint32_t off_ms);

void BSP_LED2_Init(void);                  /* 初始化板上 LED */
void BSP_LED2_On(void);
void BSP_LED2_Off(void);
void BSP_LED2_Toggle(void);

#endif /* __LED_H */
