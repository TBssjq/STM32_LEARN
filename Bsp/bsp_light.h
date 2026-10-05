#ifndef __LIGHTSENSOR_H
#define __LIGHTSENSOR_H

#include "main.h"

/* ------------------------------------------------------------------
 * 光敏电阻模块（LM393 比较器型，4 针：VCC / GND / DO / AO）
 *
 * 接线（模块 VCC 必须接 3.3V —— AO/DO 接的是 STM32 的模拟脚，不耐 5V）：
 *   VCC -> 3V3
 *   GND -> GND
 *   AO  -> PA1   （ADC1_IN1，读光照强度，0~4095）
 *   DO  -> PA2   （数字输入，比较器输出，阈值由模块上的蓝色电位器调）
 *
 * 说明：
 *   - AO 是分压节点的电压，光照越强电压一般越高；如果你的模块是反的，
 *     把 BSP_LIGHT_INVERT 改成 1
 *   - DO 是 LM393 的开漏输出，这里用 MCU 内部上拉读取（模块上有的自带
 *     上拉/指示灯，都兼容）
 *   - ANC 部分直接用寄存器配置 ADC1，不依赖 CubeMX，重新生成工程不会被冲掉
 * ------------------------------------------------------------------ */

#define BSP_LIGHT_AO_PORT       GPIOA
#define BSP_LIGHT_AO_PIN        GPIO_PIN_1
#define BSP_LIGHT_AO_CHANNEL    1           /* ADC1_IN1 = PA1 */

#define BSP_LIGHT_DO_PORT       GPIOA
#define BSP_LIGHT_DO_PIN        GPIO_PIN_2

#define BSP_LIGHT_INVERT        0           /* 1 = 光照越强读数越小 */
#define BSP_LIGHT_DO_ACTIVE     0           /* DO 的有效电平：0 = 低电平表示"越过阈值" */

void     BSP_LIGHT_Init(void);
uint16_t BSP_LIGHT_ReadRaw(void);               /* 原始值 0~4095 */
uint16_t BSP_LIGHT_ReadAvg(uint8_t samples);    /* 多次采样取平均（抗抖动） */
uint16_t BSP_LIGHT_ReadMv(void);                /* 换算成毫伏 0~3300 */
uint8_t  BSP_LIGHT_GetPercent(void);            /* 亮度百分比 0~100 */
uint8_t  BSP_LIGHT_ReadDO(void);                /* DO 引脚原始电平 0/1 */
uint8_t  BSP_LIGHT_IsTriggered(void);           /* DO 是否表示"越过阈值" */

#endif /* __LIGHTSENSOR_H */
