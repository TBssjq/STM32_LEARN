#ifndef __SSD1306_H
#define __SSD1306_H

#include "main.h"

/* ------------------------------------------------------------------
 * SSD1306 128x64 I2C 驱动
 *   接线：VDD->3.3V  GND->GND  SCK(SCL)->PB6  SDA->PB7
 *   坐标：x = 0~127（列），y = 0~63（行，像素单位）
 *
 * 说明：本驱动是纯过程式、单实例的显示驱动，改成 C++ 类没有实际收益，
 *       因此保留为 C 模块（与 HAL 同属 C 库边界），用 extern "C" 供 C++ 调用。
 * ------------------------------------------------------------------ */

#define BSP_OLED_I2C_ADDR     0x78   /* 7 位地址 0x3C 左移 1 位后给 HAL */
#define BSP_OLED_I2C_ADDR_ALT 0x7A   /* 有些模块是 0x3D，自动探测 */
#define BSP_OLED_I2C_TIMEOUT  100
#define BSP_OLED_WIDTH        128
#define BSP_OLED_HEIGHT       64

#define BSP_OLED_BLACK        0      /* 像素灭 */
#define BSP_OLED_WHITE        1      /* 像素亮 */

/* 字体大小：6 = 6x8 点阵，8 = 8x16 点阵（传其他值一律按 6x8 处理） */
#define BSP_OLED_FONT_6X8     6
#define BSP_OLED_FONT_8X16    8

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化：返回 HAL_OK 表示屏已就绪
 * 内部流程：优先硬件 I2C -> 不行再用软件位翻转(会自动试 SDA/SCL 对调)
 * 所以即使硬件 I2C 外设有问题、或者两根线接反了，也能点屏 */
HAL_StatusTypeDef BSP_OLED_Init(void);

/* 返回实际使用的通道描述，可显示到屏上确认：HW I2C / SW I2C (bit-bang) / ... */
const char *BSP_OLED_GetLinkStr(void);

/* 探测 I2C 上是否存在 SSD1306（会自动试 0x78 / 0x7A），返回实际地址，0 = 未找到 */
uint8_t BSP_OLED_Probe(void);

/* 读 SCK(SDA) 空闲电平：bit1 = SCL 为高，bit0 = SDA 为高，0x03 才正常 */
uint8_t BSP_OLED_BusLevels(void);

/* 扫描总线 0x08~0x77，找到的地址写入 list，返回找到的个数 */
uint8_t BSP_OLED_Scan(uint8_t *list, uint8_t max);

/* 用软件位翻转(不依赖 I2C 外设)再扫一遍
 *   swap = 0：PB6 = SCL，PB7 = SDA（正常接法）
 *   swap = 1：PB7 = SCL，PB6 = SDA（SDA/SCL 接反的情况）
 * 用来区分"线接反了"和"模块根本没接通" */
uint8_t BSP_OLED_SoftScan(uint8_t *list, uint8_t max, uint8_t swap);

/* 总线被从机拉死时手动发 9 个时钟解锁，并重新初始化 I2C */
void BSP_OLED_BusRecover(void);

void BSP_OLED_Clear(void);
void BSP_OLED_Fill(uint8_t color);
void BSP_OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
HAL_StatusTypeDef BSP_OLED_Refresh(void);
/* 只刷新一小块区域（比整屏快得多，100kHz 下 24 字节只要约 2ms）
 * 用来做"跟着声音跳"的实时指示，避免整屏 100ms 的刷新拖垮节奏 */
HAL_StatusTypeDef BSP_OLED_RefreshArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void BSP_OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

/* 下面三个字符函数是"不透明"的：会先把自己占的格子清成背景色再画字形，
   所以可以在同一位置反复刷新（比如显示变化的数字），不会残留旧笔画。
   需要在已有图形上叠加文字的话，直接用 BSP_OLED_DrawPixel 自己画。 */
void BSP_OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color);
void BSP_OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size, uint8_t color);
void BSP_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
