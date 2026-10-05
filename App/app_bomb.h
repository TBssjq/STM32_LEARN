#ifndef __BOMB_H
#define __BOMB_H

#include "main.h"

/* ------------------------------------------------------------------
 * 定时炸弹演示（循环执行，不会返回）
 *
 *   OLED   : 标题 "TIME BOMB" + 倒计时 MM:SS + 进度条
 *   蜂鸣器 : 滴答声，越接近 0 越急促；到 0 爆炸，3 秒后重新开始
 *   LED    : 板载 LED 心跳闪烁；爆炸时两个 LED 一起闪
 *
 * 音量：由 BSP_BUZZER_SetVolume() 控制，本模块默认调得很小（见 app_bomb.c 的 BOMB_VOLUME）
 * ------------------------------------------------------------------ */
void APP_BOMB_Run(void);

#endif /* __BOMB_H */
