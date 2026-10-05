#ifndef __DEMO_H
#define __DEMO_H

#include "main.h"

/* 各种演示程序（都是"不返回"的循环，主程序里选一个调用即可）
 *   换演示只改 main.c 的 USER CODE 2 那一行
 */

void APP_DEMO_LightSensor(void);    /* 光敏传感器 + OLED 实时显示 */
void APP_DEMO_MotorTest(void);      /* TB6612 双电机正反转/调速/刹车演示 */
void APP_DEMO_BuzzerMelody(void);   /* 蜂鸣器《欢乐颂》+ SOS（暂时不用） */

#endif /* __DEMO_H */
