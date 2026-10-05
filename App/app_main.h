#ifndef __APP_MAIN_H
#define __APP_MAIN_H

/* ------------------------------------------------------------------
 * C / C++ 边界头（App 层唯一对外暴露的 C 接口）
 *
 * CubeMX 生成的 main.c 是 C，我们自己的业务是 C++。这里用 extern "C"
 * 暴露两个 C 链接的入口，main.c 保持原样即可。
 * 注意：本头文件刻意不包含 main.h / HAL，保证 App 层与 STM32 无关。
 * ------------------------------------------------------------------ */
#ifdef __cplusplus
extern "C" {
#endif

/* 业务入口：初始化各 BSP 模块 + 选择要跑的演示（内部是死循环，不返回）
 * CubeMX 的 main() 里只在 USER CODE 2 区调用这一个函数。 */
void APP_Main(void);

/* 错误陷阱：关中断后用两个 LED 慢闪提示，永不返回。
 * CubeMX 的 Error_Handler() 里只调用它，不放具体逻辑。 */
void APP_ErrorTrap(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_H */
