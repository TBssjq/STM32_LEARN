#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#include "main.h"

/* 业务入口：初始化各 BSP 模块 + 选择要跑的演示（内部是死循环，不返回）
 * CubeMX 的 main() 里只在 USER CODE 2 区调用这一个函数。 */
void APP_Main(void);

/* 错误陷阱：关中断后用两个 LED 慢闪提示，永不返回。
 * CubeMX 的 Error_Handler() 里只调用它，不放具体逻辑。 */
void APP_ErrorTrap(void);

#endif /* __APP_MAIN_H */
