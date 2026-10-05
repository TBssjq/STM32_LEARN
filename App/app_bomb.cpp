#include "app_bomb.hpp"

#include <cstdint>

#include "board.hpp"
#include "buzzer.hpp"
#include "time.hpp"
#include "bsp_oled.h"

namespace app
{
namespace bomb
{

namespace
{
constexpr uint32_t kTotalSec   = 30U;              /* 倒计时总时长（秒） */
constexpr uint32_t kTotalMs    = kTotalSec * 1000U;
constexpr uint8_t  kVolume     = 7U;               /* 平时滴答的音量（占空比%） */
constexpr uint8_t  kBoomVolume = 50U;              /* 爆炸那一下的音量（占空比%） */
constexpr uint32_t kTickMs     = 25U;              /* 每次滴答响多久（ms） */

/* 屏幕上的"节拍"标记：左下角一块，跟着滴答一起亮灭 */
constexpr uint8_t  kBeatX = 0U;
constexpr uint8_t  kBeatY = 56U;
constexpr uint8_t  kBeatW = 24U;
constexpr uint8_t  kBeatH = 8U;

/* 让屏幕跟着声音跳：滴答响的时候在左下角画一块，停的时候擦掉
 * 最后 4 秒用整条长块，看上去更急 */
void beat(uint32_t remainMs, bool on)
{
    uint8_t w = (remainMs <= 4000U) ? kBeatW : 8U;

    BSP_OLED_FillRect(kBeatX, kBeatY, on ? w : kBeatW, kBeatH,
                      on ? BSP_OLED_WHITE : BSP_OLED_BLACK);
    BSP_OLED_RefreshArea(kBeatX, kBeatY, kBeatW, kBeatH);   /* 只传 24 字节，约 2ms */
}

/* 剩余时间 -> 滴答间隔，越接近 0 越急促 */
uint32_t tickInterval(uint32_t remainMs)
{
    if (remainMs > 15000U) { return 1000U; }
    if (remainMs > 8000U)  { return 500U;  }
    if (remainMs > 4000U)  { return 250U;  }
    return 125U;
}

/* 画一帧：标题 + 倒计时 MM:SS + 进度条 + 状态 */
void draw(uint32_t remainMs)
{
    const uint32_t passed = (remainMs <= kTotalMs) ? (kTotalMs - remainMs) : kTotalMs;
    const uint32_t sec    = (remainMs + 999U) / 1000U;      /* 向上取整到秒 */
    const uint32_t barW   = (108U * passed) / kTotalMs;

    BSP_OLED_Clear();                                       /* 整屏重画，避免残留 */

    BSP_OLED_ShowString(37, 0, "TIME BOMB", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_ShowNum(44, 14, sec / 60U, 2, BSP_OLED_FONT_8X16, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(60, 14, ':', BSP_OLED_FONT_8X16, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(68, 14, sec % 60U, 2, BSP_OLED_FONT_8X16, BSP_OLED_WHITE);

    /* 进度条外框 */
    BSP_OLED_FillRect(8, 38, 112, 1, BSP_OLED_WHITE);
    BSP_OLED_FillRect(8, 47, 112, 1, BSP_OLED_WHITE);
    BSP_OLED_FillRect(8, 38, 1, 10, BSP_OLED_WHITE);
    BSP_OLED_FillRect(119, 38, 1, 10, BSP_OLED_WHITE);
    /* 进度条内部：随时间填充 */
    if (barW > 0U)
    {
        BSP_OLED_FillRect(10, 40, static_cast<uint8_t>(barW), 6, BSP_OLED_WHITE);
    }

    BSP_OLED_ShowString(49, 54, "ARMED", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_Refresh();
}

/* 爆炸：整屏反白 + 由密到疏的爆响 + LED 狂闪 */
void explode()
{
    BSP_OLED_Fill(BSP_OLED_WHITE);
    BSP_OLED_ShowString(44, 24, "BOOM!", BSP_OLED_FONT_8X16, BSP_OLED_BLACK);
    BSP_OLED_Refresh();

    drv::buzzer::setVolume(kBoomVolume);        /* 爆炸这一下稍响一点 */
    for (uint8_t i = 0U; i < 12U; i++)
    {
        drv::buzzer::on();
        drv::time::delayMs(40U - i * 2U);       /* 越来越短 */
        drv::buzzer::off();
        drv::time::delayMs(30U + i * 8U);       /* 越来越疏 */
        bsp::led1.toggle();
        bsp::led2.toggle();
    }
    drv::buzzer::off();
    drv::buzzer::setVolume(kVolume);
}
} // namespace

/* 定时炸弹主循环（不返回） */
void run()
{
    uint32_t start    = drv::time::millis();
    uint32_t lastTick = start;
    uint32_t tickOff  = start;
    uint32_t lastLed  = start;
    uint32_t lastSec  = 0xFFFFFFFFU;            /* 让第一帧立刻画出来 */
    bool     ticking  = false;

    drv::buzzer::setVolume(kVolume);            /* 先调小 */
    drv::buzzer::off();

    while (true)
    {
        uint32_t now     = drv::time::millis();
        uint32_t elapsed = now - start;
        uint32_t remain  = (elapsed < kTotalMs) ? (kTotalMs - elapsed) : 0U;
        uint32_t sec     = (remain + 999U) / 1000U;

        /* --- 滴答：到点响一声短的，同时让屏幕跟着跳一下 --- */
        if (remain > 0U && (now - lastTick) >= tickInterval(remain))
        {
            lastTick = now;
            tickOff  = now + kTickMs;
            ticking  = true;
            drv::buzzer::on();
            beat(remain, true);
        }
        if (ticking && (static_cast<int32_t>(now - tickOff) >= 0))
        {
            drv::buzzer::off();
            ticking = false;
            beat(remain, false);
        }

        /* --- 显示：秒数变化时才整屏重画（100kHz I2C 下约 100ms） --- */
        if (sec != lastSec)
        {
            lastSec = sec;
            draw(remain);
            if (ticking)
            {
                beat(remain, true);             /* 整屏重画会把标记擦掉，补一下 */
            }
        }

        /* --- 板载 LED 心跳 --- */
        if ((now - lastLed) >= 500U)
        {
            lastLed = now;
            bsp::led2.toggle();
        }

        /* --- 时间到 --- */
        if (elapsed >= kTotalMs)
        {
            drv::buzzer::off();
            draw(0U);
            explode();
            drv::time::delayMs(3000U);

            start    = drv::time::millis();
            lastTick = start;
            lastLed  = start;
            lastSec  = 0xFFFFFFFFU;
            bsp::led1.off();
        }
    }
}

} // namespace bomb
} // namespace app
