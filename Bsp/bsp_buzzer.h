#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/* ------------------------------------------------------------------
 * MH-FMD 蜂鸣器模块驱动（3 针：GND / VCC / I/O）
 *
 * 接线（3 针）：
 *   GND -> GND
 *   VCC -> 3.3V（或 5V，模块自带驱动管，I/O 是 3.3V 逻辑）
 *   I/O -> PA6
 *
 * 本模块确认为"有源"蜂鸣器：给电平就响，音调固定，所以：
 *   - 只用 GPIO 推挽输出，不占用定时器
 *   - BSP_BUZZER_Tone(freq) 里的频率被忽略，只当开关用
 *   - 想表达信息只能靠"长短节奏"（示例里用它发了 SOS）
 * 若以后换成无源模块，把 BSP_BUZZER_PASSIVE 改成 1（会改用 TIM3_CH1 硬件 PWM 发声）
 * ------------------------------------------------------------------ */
#define BSP_BUZZER_PASSIVE       0

/* 有源模块的触发电平：1 = 高电平响，0 = 低电平响
 * 本模块实测为"低电平触发"：把 I/O 与 GND 短接就会响，故设为 0 */
#define BSP_BUZZER_ACTIVE_LEVEL  0

/* 静音时 I/O 输出的电平：0 = 低电平静音（多数 MH-FMD）；1 = 高电平静音
   如果 BSP_BUZZER_Off() 之后还在响，把这里改成 1 */
#define BSP_BUZZER_MUTE_HIGH     0

/* 默认音调（Hz），BSP_BUZZER_On() 用的是它 */
#define BSP_BUZZER_DEFAULT_FREQ  1000

/* 硬件资源：TIM3_CH1 = PA6（默认映射，不需要重映射） */
#define BSP_BUZZER_PORT          GPIOA
#define BSP_BUZZER_PIN           GPIO_PIN_6
#define BSP_BUZZER_TIM           TIM3
#define BSP_BUZZER_TIM_CLK_ENABLE()   __HAL_RCC_TIM3_CLK_ENABLE()

/* 常用音名频率（Hz），配合 BSP_BUZZER_PlayNote() 写曲子 */
#define NOTE_REST  0
#define NOTE_C4    262
#define NOTE_CS4   277
#define NOTE_D4    294
#define NOTE_DS4   311
#define NOTE_E4    330
#define NOTE_F4    349
#define NOTE_FS4   370
#define NOTE_G4    392
#define NOTE_GS4   415
#define NOTE_A4    440
#define NOTE_AS4   466
#define NOTE_B4    494
#define NOTE_C5    523
#define NOTE_D5    587
#define NOTE_E5    659
#define NOTE_F5    698
#define NOTE_G5    784
#define NOTE_A5    880
#define NOTE_B5    988

/* 无仪器自检：依次输出"直流 1s"和"500Hz 方波 1s"，共 3 轮，用耳朵判断模块类型/好坏
   直流段响 = 有源；方波段响 = 无源；都不响 = 供电/接线/模块问题 */
void BSP_BUZZER_SelfTest(void);

/* 音量/响度控制（只有"有源模块"支持）
 *   实现：把 I/O 用 100Hz 的 PWM 门控，占空比 = 音量
 *     100 = 一直输出有效电平（最响，等同原来的行为）
 *      15 = 只输出 15% 的时间（明显小声，像"哒、哒"轻响）
 *       1 = 几乎只是"嗒"一下
 *   无源模块本函数无效（响度由驱动电压决定） */
void BSP_BUZZER_SetVolume(uint8_t percent);

void BSP_BUZZER_Init(void);
void BSP_BUZZER_On(void);                       /* 默认音调(1kHz) / 有源模块直接响 */
void BSP_BUZZER_Off(void);                      /* 静音 */
void BSP_BUZZER_Tone(uint32_t freq_hz);         /* 指定频率发声；0 = 静音 */
void BSP_BUZZER_Beep(uint32_t ms);              /* 默认音调响 ms 毫秒 */
void BSP_BUZZER_BeepN(uint8_t times, uint32_t on_ms, uint32_t off_ms);
void BSP_BUZZER_PlayNote(uint16_t freq_hz, uint32_t ms);   /* 播一个音（可配合 NOTE_xx） */

#endif /* __BUZZER_H */
