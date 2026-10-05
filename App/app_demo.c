#include "app_demo.h"
#include "bsp_led.h"
#include "bsp_buzzer.h"
#include "bsp_oled.h"
#include "bsp_light.h"
#include "bsp_motor.h"

/* ==================== 光敏传感器演示 ==================== */

/* 显示用的一阶低通（指数平均）：0 = 不滤波，数值越大越平稳
 * 2 表示每次取 1/4 的新值，能把末位抖动压掉，同时对手遮挡仍有响应 */
#define BSP_LIGHT_DEMO_EMA_SHIFT    2

/* 自动量程：把"见过的最暗~最亮"映射成 0~100.0%，这样随手遮一下就能跑满量程
 * （百分比变成"相对当前环境的亮度"；改成 0 就回到按 0~4096 的绝对值） */
#define BSP_LIGHT_DEMO_AUTORANGE    1
#define BSP_LIGHT_DEMO_MIN_SPAN     64U     /* 观测区间小于这个值就先按绝对值显示 */
#define BSP_LIGHT_DEMO_DECAY_MS     2000U   /* 每隔多久把区间往当前值收缩一点（适应环境光变化） */
#define BSP_LIGHT_DEMO_DECAY_STEP   8U

/**
  * @brief  画一帧：标题 + 大字百分比(0.1% 分辨率) + 亮度条 + ADC/DO + 自动量程区间
  * @param  p1000 亮度千分比（0~1000，即 0.0%~100.0%）
  * @param  rmin,rmax 自动量程观测到的最暗/最亮原始值
  */
static void LightDemo_Draw(uint16_t raw, uint16_t mv, uint16_t p1000,
                           uint16_t rmin, uint16_t rmax)
{
    uint32_t bar_w = (108U * p1000) / 1000U;

    BSP_OLED_Clear();

#if (BSP_LIGHT_DEMO_AUTORANGE)
    BSP_OLED_ShowString(10, 0, "LIGHT SENSOR AUTO", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
#else
    BSP_OLED_ShowString(28, 0, "LIGHT SENSOR", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
#endif

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
    if (bar_w > 0U)
    {
        BSP_OLED_FillRect(10, 34, (uint8_t)bar_w, 6, BSP_OLED_WHITE);
    }

    /* 原始值(顺便显示毫伏) / DO 电平 */
    BSP_OLED_ShowString(0, 46, "ADC:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(24, 46, raw, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowString(52, 46, "DO:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(70, 46, BSP_LIGHT_ReadDO(), 1, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowString(82, 46, "mV:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(100, 46, mv, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    /* 自动量程区间（百分比就是在这个区间里取的比例） */
    BSP_OLED_ShowString(0, 56, "RNG:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(24, 56, rmin, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowChar(48, 56, '-', BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowNum(54, 56, rmax, 4, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_Refresh();
}

/**
  * @brief  光敏 + OLED 演示主循环（不返回）
  */
void APP_DEMO_LightSensor(void)
{
    int32_t  filt       = (int32_t)BSP_LIGHT_ReadAvg(8U);   /* 低通状态 */
    uint16_t rmin       = 4095U;                        /* 自动量程：见过的最暗 */
    uint16_t rmax       = 0U;                           /* 自动量程：见过的最亮 */
    uint32_t last_decay = HAL_GetTick();

    while (1)
    {
        uint16_t raw;
        uint16_t mv;
        uint16_t span;
        uint32_t p1000;
        uint32_t now = HAL_GetTick();

        /* 先做硬件多次平均，再走一级低通，末位就不跳了 */
        filt += ((int32_t)BSP_LIGHT_ReadAvg(8U) - filt) >> BSP_LIGHT_DEMO_EMA_SHIFT;
        raw = (uint16_t)filt;

        /* 记录见过的最暗 / 最亮 */
        if (raw < rmin) { rmin = raw; }
        if (raw > rmax) { rmax = raw; }

#if (BSP_LIGHT_DEMO_AUTORANGE)
        /* 慢慢收缩区间，以适应环境光的变化（否则一次强光会把量程永久拉宽） */
        if ((now - last_decay) >= BSP_LIGHT_DEMO_DECAY_MS)
        {
            last_decay = now;
            span = (uint16_t)(rmax - rmin);
            if (span > (BSP_LIGHT_DEMO_MIN_SPAN * 4U))
            {
                if ((uint32_t)raw > ((uint32_t)rmin + BSP_LIGHT_DEMO_DECAY_STEP))
                {
                    rmin = (uint16_t)(rmin + BSP_LIGHT_DEMO_DECAY_STEP);
                }
                if (((uint32_t)raw + BSP_LIGHT_DEMO_DECAY_STEP) < (uint32_t)rmax)
                {
                    rmax = (uint16_t)(rmax - BSP_LIGHT_DEMO_DECAY_STEP);
                }
            }
        }
#endif

        mv   = (uint16_t)(((uint32_t)raw * 3300U) / 4096U);
        span = (uint16_t)(rmax - rmin);

#if (BSP_LIGHT_DEMO_AUTORANGE)
        if (span >= BSP_LIGHT_DEMO_MIN_SPAN)
        {
            /* 在"见过的最暗~最亮"之间取比例，遮一下就能跑满量程 */
            p1000 = ((uint32_t)(raw - rmin) * 1000U) / span;
        }
        else
        {
            p1000 = ((uint32_t)raw * 1000U) / 4096U;    /* 区间还太小，先按绝对值 */
        }
#else
        p1000 = ((uint32_t)raw * 1000U) / 4096U;        /* 绝对值：0~4096 对应 0~100% */
#endif

#if (BSP_LIGHT_INVERT)
        p1000 = 1000U - p1000;
#endif

        LightDemo_Draw(raw, mv, (uint16_t)p1000, rmin, rmax);

        /* 两个 LED 跟随 DO（模块上的电位器就是它的阈值）：越过阈值就亮 */
        if (BSP_LIGHT_IsTriggered() != 0U)
        {
            BSP_LED_On();
            BSP_LED2_On();
        }
        else
        {
            BSP_LED_Off();
            BSP_LED2_Off();
        }

        HAL_Delay(200);
    }
}

/* ==================== TB6612 电机演示 ==================== */

/* 只接电机、没接 OLED 时设 0：跳过全部 OLED 调用，用板载 LED 表示"正在转" */
#define MOTOR_DEMO_USE_OLED   0

#if (MOTOR_DEMO_USE_OLED)
static void MotorDemo_ShowSpeed(uint8_t x, uint8_t y, int16_t sp)
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
        sp = (int16_t)(-sp);
    }
    BSP_OLED_ShowNum((uint8_t)(x + 30U), y, (uint16_t)(sp / 10), 2, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    BSP_OLED_ShowChar((uint8_t)(x + 42U), y, '%', BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
}

static void MotorDemo_Draw(int16_t a, int16_t b, const char *label)
{
    BSP_OLED_Clear();

    BSP_OLED_ShowString(13, 0, "TB6612 MOTOR TEST", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_ShowString(0, 18, "A:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    MotorDemo_ShowSpeed(18, 18, a);

    BSP_OLED_ShowString(0, 32, "B:", BSP_OLED_FONT_6X8, BSP_OLED_WHITE);
    MotorDemo_ShowSpeed(18, 32, b);

    BSP_OLED_ShowString(0, 50, label, BSP_OLED_FONT_6X8, BSP_OLED_WHITE);

    BSP_OLED_Refresh();
}
#endif  /* MOTOR_DEMO_USE_OLED */

static void MotorDemo_Step(int16_t a, int16_t b, const char *label, uint32_t ms)
{
    (void)label;

    BSP_MOTOR_SetSpeed(BSP_MOTOR_A, a);
    BSP_MOTOR_SetSpeed(BSP_MOTOR_B, b);

#if (MOTOR_DEMO_USE_OLED)
    MotorDemo_Draw(a, b, label);
#else
    if ((a != 0) || (b != 0)) { BSP_LED2_On(); }      /* 正在转：板上灯亮 */
    else                      { BSP_LED2_Off(); }
#endif

    HAL_Delay(ms);
}

void APP_DEMO_MotorTest(void)
{
    BSP_MOTOR_StopAll();

    while (1)
    {
        MotorDemo_Step( 500,    0, "A forward 50%",    2000);
        MotorDemo_Step( 900,    0, "A forward 90%",    2000);
        MotorDemo_Step(   0,    0, "A stop (coast)",    800);
        MotorDemo_Step(-600,    0, "A reverse 60%",    2000);
        MotorDemo_Step(   0,    0, "A stop (coast)",    800);

        MotorDemo_Step(   0,  500, "B forward 50%",    2000);
        MotorDemo_Step(   0, -600, "B reverse 60%",    2000);
        MotorDemo_Step(   0,    0, "B stop (coast)",    800);

        MotorDemo_Step( 700,  700, "A+B forward 70%",  2500);

        /* 刹车对比：短路制动，停得干脆 */
        BSP_MOTOR_Brake(BSP_MOTOR_A);
        BSP_MOTOR_Brake(BSP_MOTOR_B);
#if (MOTOR_DEMO_USE_OLED)
        MotorDemo_Draw(0, 0, "BRAKE 1s");
#endif
        HAL_Delay(1000);

        BSP_LED_Toggle();
        BSP_LED2_Toggle();
    }
}

/* ==================== 蜂鸣器旋律演示（暂时不用） ==================== */

typedef struct
{
    uint16_t freq;
    uint16_t ms;
} Note_t;

/* 《欢乐颂》开头 */
static const Note_t s_melody[] =
{
    {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_F4, 300}, {NOTE_G4, 300},
    {NOTE_G4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300},
    {NOTE_C4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300}, {NOTE_E4, 300},
    {NOTE_E4, 450}, {NOTE_D4, 150}, {NOTE_D4, 600}, {NOTE_REST, 300},
};

void APP_DEMO_BuzzerMelody(void)
{
    while (1)
    {
        /* 三声短鸣 */
        BSP_BUZZER_BeepN(3, 80, 120);

        /* SOS：三短 / 三长 / 三短（有源模块靠长短区分信息） */
        BSP_BUZZER_BeepN(3, 150, 150);
        HAL_Delay(300);
        BSP_BUZZER_BeepN(3, 450, 150);
        HAL_Delay(300);
        BSP_BUZZER_BeepN(3, 150, 150);

        /* 一小段曲子（按旋律表的时值走一遍） */
        for (uint8_t i = 0; i < (sizeof(s_melody) / sizeof(s_melody[0])); i++)
        {
            BSP_BUZZER_PlayNote(s_melody[i].freq, s_melody[i].ms);
        }

        BSP_LED_Toggle();
        BSP_LED2_Toggle();
        HAL_Delay(1500);
    }
}
