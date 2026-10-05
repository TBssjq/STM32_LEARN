#include "app_bomb.h"
#include "bsp_oled.h"
#include "bsp_buzzer.h"
#include "bsp_led.h"

#define BOMB_TOTAL_SEC     30U  /* 倒计时总时长（秒） */
#define BOMB_VOLUME         7U  /* 平时滴答的音量（占空比%）：越小越轻 */
#define BOMB_BOOM_VOLUME   50U  /* 爆炸那一下的音量（占空比%） */
#define BOMB_TICK_MS       25U  /* 每次滴答响多久（ms） */

/* 屏幕上的"节拍"标记：左下角一块，跟着滴答一起亮灭 */
#define BEAT_X     0U
#define BEAT_Y     56U
#define BEAT_W     24U
#define BEAT_H     8U

/**
  * @brief  让屏幕跟着声音跳：滴答响的时候在左下角画一块，停的时候擦掉
  *         最后 4 秒用整条长块，看上去更急
  */
static void Bomb_Beat(uint32_t remain_ms, uint8_t on)
{
    uint8_t w = (remain_ms <= 4000U) ? BEAT_W : 8U;

    if (on != 0U)
    {
        BSP_OLED_FillRect(BEAT_X, BEAT_Y, w, BEAT_H, BSP_OLED_WHITE);
    }
    else
    {
        BSP_OLED_FillRect(BEAT_X, BEAT_Y, BEAT_W, BEAT_H, BSP_OLED_BLACK);
    }
    BSP_OLED_RefreshArea(BEAT_X, BEAT_Y, BEAT_W, BEAT_H);   /* 只传 24 字节，约 2ms */
}

/**
  * @brief  剩余时间 -> 滴答间隔，越接近 0 越急促
  */
static uint32_t Bomb_TickInterval(uint32_t remain_ms)
{
    if (remain_ms > 15000U) { return 1000U; }
    if (remain_ms > 8000U)  { return 500U;  }
    if (remain_ms > 4000U)  { return 250U;  }
    return 125U;
}

/**
  * @brief  画一帧：标题 + 倒计时 MM:SS + 进度条 + 状态
  */
static void Bomb_Draw(uint32_t remain_ms)
{
    const uint32_t total   = BOMB_TOTAL_SEC * 1000U;
    const uint32_t passed  = (remain_ms <= total) ? (total - remain_ms) : total;
    uint32_t       sec     = (remain_ms + 999U) / 1000U;      /* 向上取整到秒 */
    uint32_t       bar_w   = (108U * passed) / total;

    BSP_OLED_Clear();                                          /* 整屏重画，避免残留 */

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
    if (bar_w > 0U)
    {
        BSP_OLED_FillRect(10, 40, (uint8_t)bar_w, 6, BSP_OLED_WHITE);
    }

    BSP_OLED_ShowString(49, 54, "ARMED", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_Refresh();
}

/**
  * @brief  爆炸：整屏反白 + 由密到疏的爆响 + LED 狂闪
  */
static void Bomb_Explode(void)
{
    uint8_t i;

    BSP_OLED_Fill(BSP_OLED_WHITE);
    BSP_OLED_ShowString(44, 24, "BOOM!", BSP_OLED_FONT_8X16, BSP_OLED_BLACK);
    BSP_OLED_Refresh();

    BSP_BUZZER_SetVolume(BOMB_BOOM_VOLUME);     /* 爆炸这一下稍响一点 */
    for (i = 0; i < 12U; i++)
    {
        BSP_BUZZER_On();
        HAL_Delay(40U - i * 2U);            /* 越来越短 */
        BSP_BUZZER_Off();
        HAL_Delay(30U + i * 8U);            /* 越来越疏 */
        BSP_LED_Toggle();
        BSP_LED2_Toggle();
    }
    BSP_BUZZER_Off();
    BSP_BUZZER_SetVolume(BOMB_VOLUME);
}

/**
  * @brief  定时炸弹主循环（不返回）
  */
void APP_BOMB_Run(void)
{
    const uint32_t total_ms = BOMB_TOTAL_SEC * 1000U;
    uint32_t start      = HAL_GetTick();
    uint32_t last_tick  = start;
    uint32_t tick_off   = start;
    uint32_t last_led   = start;
    uint32_t last_sec   = 0xFFFFFFFFU;      /* 让第一帧立刻画出来 */
    uint8_t  ticking    = 0U;

    BSP_BUZZER_SetVolume(BOMB_VOLUME);          /* 先调小 */
    BSP_BUZZER_Off();

    while (1)
    {
        uint32_t now     = HAL_GetTick();
        uint32_t elapsed = now - start;
        uint32_t remain  = (elapsed < total_ms) ? (total_ms - elapsed) : 0U;
        uint32_t sec     = (remain + 999U) / 1000U;

        /* --- 滴答：到点响一声短的，同时让屏幕跟着跳一下 --- */
        if (remain > 0U && (now - last_tick) >= Bomb_TickInterval(remain))
        {
            last_tick = now;
            tick_off  = now + BOMB_TICK_MS;
            ticking   = 1U;
            BSP_BUZZER_On();
            Bomb_Beat(remain, 1U);
        }
        if (ticking != 0U && (int32_t)(now - tick_off) >= 0)
        {
            BSP_BUZZER_Off();
            ticking = 0U;
            Bomb_Beat(remain, 0U);
        }

        /* --- 显示：秒数变化时才整屏重画（100kHz I2C 下约 100ms） --- */
        if (sec != last_sec)
        {
            last_sec = sec;
            Bomb_Draw(remain);
            if (ticking != 0U)
            {
                Bomb_Beat(remain, 1U);      /* 整屏重画会把标记擦掉，补一下 */
            }
        }

        /* --- 板载 LED 心跳 --- */
        if ((now - last_led) >= 500U)
        {
            last_led = now;
            BSP_LED2_Toggle();
        }

        /* --- 时间到 --- */
        if (elapsed >= total_ms)
        {
            BSP_BUZZER_Off();
            Bomb_Draw(0U);
            Bomb_Explode();
            HAL_Delay(3000);

            start     = HAL_GetTick();
            last_tick = start;
            last_led  = start;
            last_sec  = 0xFFFFFFFFU;
            BSP_LED_Off();
        }
    }
}
