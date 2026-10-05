#include "app_demo.hpp"

#include <cstdint>

#include "board.hpp"
#include "buzzer.hpp"
#include "light.hpp"
#include "motor.hpp"
#include "time.hpp"
#include "bsp_oled.h"   /* SSD1306 显示驱动：C 库，直接调用其 C 接口 */

namespace app
{
namespace demo
{

/* ==================== 光敏传感器演示 ==================== */

namespace
{
/* 显示用的一阶低通（指数平均）：0 = 不滤波，数值越大越平稳
 * 2 表示每次取 1/4 的新值，能把末位抖动压掉，同时对手遮挡仍有响应 */
constexpr uint8_t kEmaShift = 2;

/* 自动量程：把"见过的最暗~最亮"映射成 0~100.0%，这样随手遮一下就能跑满量程 */
constexpr bool     kAutoRange = true;
constexpr uint16_t kMinSpan   = 64U;      /* 观测区间小于这个值就先按绝对值显示 */
constexpr uint32_t kDecayMs   = 2000U;    /* 每隔多久把区间往当前值收缩一点 */
constexpr uint16_t kDecayStep = 8U;

/* 画一帧：标题 + 大字百分比(0.1% 分辨率) + 亮度条 + ADC/DO + 自动量程区间
 *   p1000：亮度千分比（0~1000，即 0.0%~100.0%）
 *   rmin/rmax：自动量程观测到的最暗/最亮原始值 */
void lightDraw(uint16_t raw, uint16_t mv, uint16_t p1000,
               uint16_t rmin, uint16_t rmax)
{
    const uint32_t barW = (108U * p1000) / 1000U;

    BSP_OLED_Clear();

    if (kAutoRange)
    {
        BSP_OLED_ShowString(10, 0, "LIGHT SENSOR AUTO", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    }
    else
    {
        BSP_OLED_ShowString(28, 0, "LIGHT SENSOR", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    }

    /* 大字百分比：整数 2 位 + 小数点 + 1 位小数，例如 42.3% */
    BSP_OLED_ShowNum(44, 12, p1000 / 10U, 2, BSP_OLED_FONT_8X16, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(60, 12, '.', BSP_OLED_FONT_8X16, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(68, 12, p1000 % 10U, 1, BSP_OLED_FONT_8X16, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(76, 12, '%', BSP_OLED_FONT_8X16, BSP_OLED_WHITE);

    /* 亮度条 */
    BSP_OLED_FillRect(8, 32, 112, 1, BSP_OLED_WHITE);
    BSP_OLED_FillRect(8, 41, 112, 1, BSP_OLED_WHITE);
    BSP_OLED_FillRect(8, 32, 1, 10, BSP_OLED_WHITE);
    BSP_OLED_FillRect(119, 32, 1, 10, BSP_OLED_WHITE);
    if (barW > 0U)
    {
        BSP_OLED_FillRect(10, 34, static_cast<uint8_t>(barW), 6, BSP_OLED_WHITE);
    }

    /* 原始值(顺便显示毫伏) / DO 电平 */
    BSP_OLED_ShowString(0, 46, "ADC:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(24, 46, raw, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowString(52, 46, "DO:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(70, 46, drv::light::readDO(), 1, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowString(82, 46, "mV:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(100, 46, mv, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    /* 自动量程区间（百分比就是在这个区间里取的比例） */
    BSP_OLED_ShowString(0, 56, "RNG:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(24, 56, rmin, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(48, 56, '-', BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(54, 56, rmax, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_Refresh();
}
} // namespace

/* 光敏 + OLED 演示主循环（不返回） */
void lightSensor()
{
    int32_t  filt      = static_cast<int32_t>(drv::light::readAvg(8U));   /* 低通状态 */
    uint16_t rmin      = 4095U;                 /* 自动量程：见过的最暗 */
    uint16_t rmax      = 0U;                    /* 自动量程：见过的最亮 */
    uint32_t lastDecay = drv::time::millis();

    while (true)
    {
        uint32_t now = drv::time::millis();

        /* 先做硬件多次平均，再走一级低通，末位就不跳了 */
        filt += (static_cast<int32_t>(drv::light::readAvg(8U)) - filt) >> kEmaShift;
        uint16_t raw = static_cast<uint16_t>(filt);

        /* 记录见过的最暗 / 最亮 */
        if (raw < rmin) { rmin = raw; }
        if (raw > rmax) { rmax = raw; }

        if (kAutoRange)
        {
            /* 慢慢收缩区间，以适应环境光的变化（否则一次强光会把量程永久拉宽） */
            if ((now - lastDecay) >= kDecayMs)
            {
                lastDecay = now;
                uint16_t span = static_cast<uint16_t>(rmax - rmin);
                if (span > (kMinSpan * 4U))
                {
                    if (static_cast<uint32_t>(raw) > (static_cast<uint32_t>(rmin) + kDecayStep))
                    {
                        rmin = static_cast<uint16_t>(rmin + kDecayStep);
                    }
                    if ((static_cast<uint32_t>(raw) + kDecayStep) < static_cast<uint32_t>(rmax))
                    {
                        rmax = static_cast<uint16_t>(rmax - kDecayStep);
                    }
                }
            }
        }

        uint16_t mv   = static_cast<uint16_t>((static_cast<uint32_t>(raw) * 3300U) / 4096U);
        uint16_t span = static_cast<uint16_t>(rmax - rmin);
        uint32_t p1000;

        if (kAutoRange && (span >= kMinSpan))
        {
            /* 在"见过的最暗~最亮"之间取比例，遮一下就能跑满量程 */
            p1000 = (static_cast<uint32_t>(raw - rmin) * 1000U) / span;
        }
        else
        {
            p1000 = (static_cast<uint32_t>(raw) * 1000U) / 4096U;    /* 按绝对值 */
        }

#if (BSP_LIGHT_INVERT)
        p1000 = 1000U - p1000;
#endif

        lightDraw(raw, mv, static_cast<uint16_t>(p1000), rmin, rmax);

        /* 两个 LED 跟随 DO（模块上的电位器就是它的阈值）：越过阈值就亮 */
        if (drv::light::isTriggered())
        {
            bsp::led1.on();
            bsp::led2.on();
        }
        else
        {
            bsp::led1.off();
            bsp::led2.off();
        }

        drv::time::delayMs(200U);
    }
}

/* ==================== TB6612 电机演示 ==================== */

/* 只接电机、没接 OLED 时设 0：跳过全部 OLED 调用，用板载 LED 表示"正在转" */
#define MOTOR_DEMO_USE_OLED   0

namespace
{
#if (MOTOR_DEMO_USE_OLED)
void motorShowSpeed(uint8_t x, uint8_t y, int16_t sp)
{
    if (sp == 0)
    {
        BSP_OLED_ShowString(x, y, "STOP", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
        return;
    }
    if (sp > 0)
    {
        BSP_OLED_ShowString(x, y, "FWD", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    }
    else
    {
        BSP_OLED_ShowString(x, y, "REV", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
        sp = static_cast<int16_t>(-sp);
    }
    BSP_OLED_ShowNum(static_cast<uint8_t>(x + 30U), y, static_cast<uint16_t>(sp / 10), 2, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(static_cast<uint8_t>(x + 42U), y, '%', BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
}

void motorDraw(int16_t a, int16_t b, const char *label)
{
    BSP_OLED_Clear();
    BSP_OLED_ShowString(13, 0, "TB6612 MOTOR TEST", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowString(0, 18, "A:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    motorShowSpeed(18, 18, a);
    BSP_OLED_ShowString(0, 32, "B:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    motorShowSpeed(18, 32, b);
    BSP_OLED_ShowString(0, 50, label, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_Refresh();
}
#endif  /* MOTOR_DEMO_USE_OLED */

void motorStep(int16_t a, int16_t b, const char *label, uint32_t ms)
{
    (void)label;

    bsp::motorA.setSpeed(a);
    bsp::motorB.setSpeed(b);

#if (MOTOR_DEMO_USE_OLED)
    motorDraw(a, b, label);
#else
    if ((a != 0) || (b != 0)) { bsp::led2.on(); }      /* 正在转：板上灯亮 */
    else                      { bsp::led2.off(); }
#endif

    drv::time::delayMs(ms);
}
} // namespace

void motorTest()
{
    bsp::motorA.stop();
    bsp::motorB.stop();

    while (true)
    {
        motorStep( 500,    0, "A forward 50%",    2000);
        motorStep( 900,    0, "A forward 90%",    2000);
        motorStep(   0,    0, "A stop (coast)",    800);
        motorStep(-600,    0, "A reverse 60%",    2000);
        motorStep(   0,    0, "A stop (coast)",    800);

        motorStep(   0,  500, "B forward 50%",    2000);
        motorStep(   0, -600, "B reverse 60%",    2000);
        motorStep(   0,    0, "B stop (coast)",    800);

        motorStep( 700,  700, "A+B forward 70%",  2500);

        /* 刹车对比：短路制动，停得干脆 */
        bsp::motorA.brake();
        bsp::motorB.brake();
#if (MOTOR_DEMO_USE_OLED)
        motorDraw(0, 0, "BRAKE 1s");
#endif
        drv::time::delayMs(1000);

        bsp::led1.toggle();
        bsp::led2.toggle();
    }
}

/* ==================== 蜂鸣器旋律演示（暂时不用） ==================== */

namespace
{
struct Note
{
    uint16_t freq;
    uint16_t ms;
};

/* 《欢乐颂》开头 */
constexpr Note kMelody[] =
{
    {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_F4, 300}, {NOTE_G4, 300},
    {NOTE_G4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300},
    {NOTE_C4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300}, {NOTE_E4, 300},
    {NOTE_E4, 450}, {NOTE_D4, 150}, {NOTE_D4, 600}, {NOTE_REST, 300},
};
} // namespace

void buzzerMelody()
{
    while (true)
    {
        /* 三声短鸣 */
        drv::buzzer::beepN(3, 80, 120);

        /* SOS：三短 / 三长 / 三短（有源模块靠长短区分信息） */
        drv::buzzer::beepN(3, 150, 150);
        drv::time::delayMs(300);
        drv::buzzer::beepN(3, 450, 150);
        drv::time::delayMs(300);
        drv::buzzer::beepN(3, 150, 150);

        /* 一小段曲子（按旋律表的时值走一遍） */
        for (const Note &n : kMelody)
        {
            drv::buzzer::playNote(n.freq, n.ms);
        }

        bsp::led1.toggle();
        bsp::led2.toggle();
        drv::time::delayMs(1500);
    }
}

} // namespace demo
} // namespace app
