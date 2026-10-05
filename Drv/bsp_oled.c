#include "bsp_oled.h"
#include <string.h>
#include "bsp_oledfont.h"   /* 复用原有的 ASCII 点阵表 F6x8 / F8X16 */

extern I2C_HandleTypeDef hi2c1;

/* 1024 字节显存：每 8 行组成 1 页，共 8 页，每页 128 字节 */
static uint8_t s_buf[BSP_OLED_WIDTH * BSP_OLED_HEIGHT / 8];

/* 实际使用的器件地址，初始化时自动探测 */
static uint8_t s_addr = BSP_OLED_I2C_ADDR;

/* 传输方式：0 = 硬件 I2C，1 = 软件位翻转 */
static uint8_t s_soft = 0;
/* 软件位翻转时是否交换 SDA/SCL（0 = PB6=SCL/PB7=SDA） */
static uint8_t s_soft_swap = 0;

/* ---------------------- 引脚模式切换 ---------------------- */

/**
  * @brief  把 PB6/PB7 配成普通开漏 GPIO（软件位翻转用），并关掉 I2C 外设
  */
static void BSP_OLED_PinsToGpio(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_I2C_DISABLE(&hi2c1);

    gpio.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);  /* 释放两条线 */
}

/**
  * @brief  重新初始化硬件 I2C，并把 PB6/PB7 配回 AF_OD
  *
  * 关键：HAL_I2C_Init() 只在 State == HAL_I2C_STATE_RESET 时才会调用 MspInit()，
  *       所以引脚被我们改成 GPIO 之后，必须先 HAL_I2C_DeInit()（内部会把 State
  *       置回 RESET）再 HAL_I2C_Init()，否则引脚永远回不到复用功能，
  *       表现就是硬件 I2C 完全发不出数据。
  */
static void BSP_OLED_I2CReinit(void)
{
    HAL_I2C_DeInit(&hi2c1);
    hi2c1.State = HAL_I2C_STATE_RESET;
    HAL_I2C_Init(&hi2c1);
}

/* ---------------------- 软件位翻转 I2C ---------------------- */

static void Soft_Delay(void)
{
    for (volatile uint32_t i = 0; i < 60; i++) { }   /* 约 2~3us */
}

static void Soft_W(uint16_t pin, uint8_t level)
{
    HAL_GPIO_WritePin(GPIOB, pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Soft_Start(uint16_t scl, uint16_t sda)
{
    Soft_W(sda, 1); Soft_W(scl, 1); Soft_Delay();
    Soft_W(sda, 0); Soft_Delay();
    Soft_W(scl, 0); Soft_Delay();
}

static void Soft_Stop(uint16_t scl, uint16_t sda)
{
    Soft_W(sda, 0); Soft_W(scl, 1); Soft_Delay();
    Soft_W(sda, 1); Soft_Delay();
}

/* 返回 1 = 收到 ACK */
static uint8_t Soft_WriteByte(uint16_t scl, uint16_t sda, uint8_t byte)
{
    uint8_t i, ack;

    for (i = 0; i < 8; i++)
    {
        Soft_W(sda, (byte & 0x80) ? 1 : 0);
        Soft_Delay();
        Soft_W(scl, 1); Soft_Delay();
        Soft_W(scl, 0); Soft_Delay();
        byte = (uint8_t)(byte << 1);
    }

    Soft_W(sda, 1);                 /* 释放 SDA，读从机 ACK */
    Soft_Delay();
    Soft_W(scl, 1); Soft_Delay();
    ack = (HAL_GPIO_ReadPin(GPIOB, sda) == GPIO_PIN_RESET) ? 1 : 0;
    Soft_W(scl, 0); Soft_Delay();
    return ack;
}

/* 发一整帧：ctrl = 0x00(命令) 或 0x40(数据) */
static void Soft_WriteFrame(uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t scl = s_soft_swap ? GPIO_PIN_7 : GPIO_PIN_6;
    uint16_t sda = s_soft_swap ? GPIO_PIN_6 : GPIO_PIN_7;

    Soft_Start(scl, sda);
    if (!Soft_WriteByte(scl, sda, s_addr) ||
        !Soft_WriteByte(scl, sda, ctrl))
    {
        Soft_Stop(scl, sda);
        return;
    }
    for (i = 0; i < len; i++)
    {
        if (!Soft_WriteByte(scl, sda, data[i]))
        {
            break;
        }
    }
    Soft_Stop(scl, sda);
}

/* ---------------------- 底层写命令 / 写数据 ---------------------- */

static HAL_StatusTypeDef BSP_OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};   /* 0x00 = 命令 */

    if (s_soft)
    {
        Soft_WriteFrame(0x00, &cmd, 1);
        return HAL_OK;
    }
    return HAL_I2C_Master_Transmit(&hi2c1, s_addr, buf, 2, BSP_OLED_I2C_TIMEOUT);
}

static HAL_StatusTypeDef BSP_OLED_WriteData(const uint8_t *data, uint16_t len)
{
    uint8_t buf[BSP_OLED_WIDTH + 1];

    if (s_soft)
    {
        Soft_WriteFrame(0x40, data, len);
        return HAL_OK;
    }

    buf[0] = 0x40;                  /* 0x40 = 数据 */
    memcpy(&buf[1], data, len);
    return HAL_I2C_Master_Transmit(&hi2c1, s_addr, buf, len + 1, BSP_OLED_I2C_TIMEOUT);
}

/* ---------------------- 探测 / 扫描 / 总线恢复 ---------------------- */

uint8_t BSP_OLED_Probe(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, BSP_OLED_I2C_ADDR, 3, BSP_OLED_I2C_TIMEOUT) == HAL_OK)
    {
        return BSP_OLED_I2C_ADDR;
    }
    if (HAL_I2C_IsDeviceReady(&hi2c1, BSP_OLED_I2C_ADDR_ALT, 3, BSP_OLED_I2C_TIMEOUT) == HAL_OK)
    {
        return BSP_OLED_I2C_ADDR_ALT;
    }
    return 0;
}

uint8_t BSP_OLED_Scan(uint8_t *list, uint8_t max)
{
    uint8_t addr;
    uint8_t count = 0;

    for (addr = 0x08; addr <= 0x77; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2, 50) == HAL_OK)
        {
            if (count < max)
            {
                list[count] = (uint8_t)(addr << 1);
            }
            count++;
        }
    }
    return count;
}

uint8_t BSP_OLED_SoftScan(uint8_t *list, uint8_t max, uint8_t swap)
{
    uint16_t scl = swap ? GPIO_PIN_7 : GPIO_PIN_6;
    uint16_t sda = swap ? GPIO_PIN_6 : GPIO_PIN_7;
    uint8_t addr, i, count = 0;

    BSP_OLED_PinsToGpio();

    /* 先发 9 个时钟把可能被拉死的从机顶出来 */
    Soft_W(sda, 1);
    Soft_W(scl, 1);
    for (i = 0; i < 9; i++)
    {
        Soft_W(scl, 0); Soft_Delay();
        Soft_W(scl, 1); Soft_Delay();
    }

    for (addr = 0x08; addr <= 0x77; addr++)
    {
        Soft_Start(scl, sda);
        if (Soft_WriteByte(scl, sda, (uint8_t)(addr << 1)))
        {
            if (count < max)
            {
                list[count] = (uint8_t)(addr << 1);
            }
            count++;
        }
        Soft_Stop(scl, sda);
    }
    return count;
}

uint8_t BSP_OLED_BusLevels(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t levels = 0;

    BSP_OLED_PinsToGpio();

    gpio.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLUP;       /* 用内部上拉(约40k)，线被拉低就说明有问题 */
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_Delay(2);

    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_SET) { levels |= 0x02; }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET) { levels |= 0x01; }

    BSP_OLED_I2CReinit();            /* 引脚交还硬件 I2C */
    return levels;
}

void BSP_OLED_BusRecover(void)
{
    uint8_t i;

    BSP_OLED_PinsToGpio();

    /* SDA 保持高，发 9 个 SCL 时钟，再补一个 STOP */
    Soft_W(GPIO_PIN_7, 1);
    Soft_W(GPIO_PIN_6, 1);
    for (i = 0; i < 9; i++)
    {
        Soft_W(GPIO_PIN_6, 0); HAL_Delay(1);
        Soft_W(GPIO_PIN_6, 1); HAL_Delay(1);
    }
    Soft_W(GPIO_PIN_7, 0); HAL_Delay(1);
    Soft_W(GPIO_PIN_7, 1); HAL_Delay(1);

    BSP_OLED_I2CReinit();
}

/* ---------------------- 初始化 ---------------------- */

HAL_StatusTypeDef BSP_OLED_Init(void)
{
    uint8_t ids[4];
    uint8_t n;

    HAL_Delay(100);                 /* 等屏内部上电复位完成 */

    /* --- 1. 先用硬件 I2C（快）--- */
    s_soft = 0;
    BSP_OLED_BusRecover();           /* 解总线死锁 + 引脚配回 AF_OD */
    s_addr = BSP_OLED_Probe();

    if (s_addr == 0)
    {
        /* 0x3C/0x3D 都没应答，扫全总线；只有唯一器件时直接采用它的地址 */
        n = BSP_OLED_Scan(ids, 4);
        if (n == 1)
        {
            s_addr = ids[0];
        }
    }

    /* --- 2. 硬件 I2C 彻底不通，退回软件位翻转（引脚接法不变）--- */
    if (s_addr == 0)
    {
        s_soft_swap = 0;
        n = BSP_OLED_SoftScan(ids, 4, 0);
        if (n == 0)
        {
            s_soft_swap = 1;        /* 正常接法不通，试试 SDA/SCL 对调 */
            n = BSP_OLED_SoftScan(ids, 4, 1);
        }
        if (n == 0)
        {
            return HAL_ERROR;       /* 两种方式都找不到屏 */
        }
        s_addr = ids[0];
        s_soft = 1;
    }

    /* --- 3. 初始化命令序列 --- */
    BSP_OLED_WriteCmd(0xAE);                                 /* 关显示 */
    BSP_OLED_WriteCmd(0xD5); BSP_OLED_WriteCmd(0x80);         /* 时钟分频 */
    BSP_OLED_WriteCmd(0xA8); BSP_OLED_WriteCmd(0x3F);         /* 多路复用 64 */
    BSP_OLED_WriteCmd(0xD3); BSP_OLED_WriteCmd(0x00);         /* 显示偏移 0 */
    BSP_OLED_WriteCmd(0x40);                                 /* 起始行 0 */
    BSP_OLED_WriteCmd(0x8D); BSP_OLED_WriteCmd(0x14);         /* 打开电荷泵 */
    BSP_OLED_WriteCmd(0x20); BSP_OLED_WriteCmd(0x02);         /* 页寻址模式 */
    BSP_OLED_WriteCmd(0xA1);                                 /* 段重映射 */
    BSP_OLED_WriteCmd(0xC8);                                 /* COM 扫描方向 */
    BSP_OLED_WriteCmd(0xDA); BSP_OLED_WriteCmd(0x12);         /* COM 引脚配置 */
    BSP_OLED_WriteCmd(0x81); BSP_OLED_WriteCmd(0xCF);         /* 对比度 */
    BSP_OLED_WriteCmd(0xD9); BSP_OLED_WriteCmd(0xF1);         /* 预充电周期 */
    BSP_OLED_WriteCmd(0xDB); BSP_OLED_WriteCmd(0x30);         /* VCOMH */
    BSP_OLED_WriteCmd(0xA4);                                 /* 输出跟随显存 */
    BSP_OLED_WriteCmd(0xA6);                                 /* 正常显示 */
    BSP_OLED_WriteCmd(0xAF);                                 /* 开显示 */

    BSP_OLED_Clear();
    return BSP_OLED_Refresh();
}

const char *BSP_OLED_GetLinkStr(void)
{
    if (!s_soft)
    {
        return "HW I2C";
    }
    return s_soft_swap ? "SW I2C SWAPPED" : "SW I2C (bit-bang)";
}

/* ---------------------- 显存操作 ---------------------- */

void BSP_OLED_Clear(void)
{
    memset(s_buf, 0x00, sizeof(s_buf));
}

void BSP_OLED_Fill(uint8_t color)
{
    memset(s_buf, color ? 0xFF : 0x00, sizeof(s_buf));
}

/**
  * @brief  用指定颜色填充一个矩形区域（用于清掉一块旧内容再重画）
  */
void BSP_OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    uint8_t i, j;

    for (i = 0; i < w; i++)
    {
        for (j = 0; j < h; j++)
        {
            BSP_OLED_DrawPixel((uint8_t)(x + i), (uint8_t)(y + j), color);
        }
    }
}

void BSP_OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= BSP_OLED_WIDTH || y >= BSP_OLED_HEIGHT)
    {
        return;
    }

    if (color)
    {
        s_buf[(y / 8) * BSP_OLED_WIDTH + x] |= (uint8_t)(1 << (y % 8));
    }
    else
    {
        s_buf[(y / 8) * BSP_OLED_WIDTH + x] &= (uint8_t)~(1 << (y % 8));
    }
}

HAL_StatusTypeDef BSP_OLED_Refresh(void)
{
    uint8_t page;

    for (page = 0; page < BSP_OLED_HEIGHT / 8; page++)
    {
        BSP_OLED_WriteCmd(0xB0 + page);      /* 页地址 */
        BSP_OLED_WriteCmd(0x00);             /* 列低 4 位 */
        BSP_OLED_WriteCmd(0x10);             /* 列高 4 位 */

        if (BSP_OLED_WriteData(&s_buf[page * BSP_OLED_WIDTH], BSP_OLED_WIDTH) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

/**
  * @brief  只刷新一小块区域（用于实时指示，别用整屏刷新去追快速变化的内容）
  * @param  x,y 左上角，w,h 尺寸（内部按 8 行为一页对齐）
  */
HAL_StatusTypeDef BSP_OLED_RefreshArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t page, page_last;

    if (x >= BSP_OLED_WIDTH || y >= BSP_OLED_HEIGHT || w == 0U || h == 0U)
    {
        return HAL_ERROR;
    }
    if ((uint16_t)x + w > BSP_OLED_WIDTH)  { w = (uint8_t)(BSP_OLED_WIDTH - x); }
    if ((uint16_t)y + h > BSP_OLED_HEIGHT) { h = (uint8_t)(BSP_OLED_HEIGHT - y); }

    page_last = (uint8_t)((y + h - 1U) / 8U);

    for (page = (uint8_t)(y / 8U); page <= page_last; page++)
    {
        if (BSP_OLED_WriteCmd((uint8_t)(0xB0 + page)) != HAL_OK) { return HAL_ERROR; }
        BSP_OLED_WriteCmd((uint8_t)(0x00 | (x & 0x0FU)));            /* 列低 4 位 */
        BSP_OLED_WriteCmd((uint8_t)(0x10 | ((x >> 4) & 0x0FU)));     /* 列高 4 位 */
        if (BSP_OLED_WriteData(&s_buf[page * BSP_OLED_WIDTH + x], w) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

/* ---------------------- 字符 / 字符串 ---------------------- */

/**
  * @brief  在指定位置显示一个 ASCII 字符（不透明：先清掉自己的格子再画字形）
  * @param  x,y  左上角像素坐标
  * @param  size 6 = 6x8 点阵，8 = 8x16 点阵（其他值按 6x8 处理）
  * @param  color BSP_OLED_WHITE = 白字黑底，BSP_OLED_BLACK = 黑字白底
  */
void BSP_OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color)
{
    uint8_t i, j, data;
    uint8_t idx = (uint8_t)ch;
    uint8_t w, h, bg;

    if (size != BSP_OLED_FONT_8X16)
    {
        size = BSP_OLED_FONT_6X8;            /* 只支持 6 / 8 两种字号 */
    }

    if (idx < 32 || idx > 126)
    {
        idx = 32;                           /* 不可显示的字符按空格处理 */
    }
    idx -= 32;

    w  = (size == BSP_OLED_FONT_8X16) ? 8 : 6;
    h  = (size == BSP_OLED_FONT_8X16) ? 16 : 8;
    bg = color ? BSP_OLED_BLACK : BSP_OLED_WHITE;

    /* 关键：先把字符格子清成背景色。
       否则原地刷新数字时，旧笔画不会被覆盖，几帧后就会糊成一团。 */
    BSP_OLED_FillRect(x, y, w, h, bg);

    if (size == BSP_OLED_FONT_8X16)
    {
        for (i = 0; i < 8; i++)
        {
            data = F8X16[idx * 16 + i];     /* 上半 8 行 */
            for (j = 0; j < 8; j++)
            {
                if ((data >> j) & 0x01)
                {
                    BSP_OLED_DrawPixel(x + i, y + j, color);
                }
            }
            data = F8X16[idx * 16 + i + 8]; /* 下半 8 行 */
            for (j = 0; j < 8; j++)
            {
                if ((data >> j) & 0x01)
                {
                    BSP_OLED_DrawPixel(x + i, y + 8 + j, color);
                }
            }
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            data = F6x8[idx][i];
            for (j = 0; j < 8; j++)
            {
                if ((data >> j) & 0x01)
                {
                    BSP_OLED_DrawPixel(x + i, y + j, color);
                }
            }
        }
    }
}

void BSP_OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size, uint8_t color)
{
    if (size != BSP_OLED_FONT_8X16)
    {
        size = BSP_OLED_FONT_6X8;
    }

    while (*str != '\0')
    {
        if (*str == '\n')
        {
            x = 0;
            y += size;
        }
        else
        {
            BSP_OLED_ShowChar(x, y, *str, size, color);
            x += size;
        }
        str++;
    }
}

void BSP_OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    uint32_t pow = 1;
    uint8_t i;

    if (size != BSP_OLED_FONT_8X16)
    {
        size = BSP_OLED_FONT_6X8;
    }

    for (i = 1; i < len; i++)
    {
        pow *= 10;
    }

    for (i = 0; i < len; i++)
    {
        BSP_OLED_ShowChar(x + size * i, y, (char)('0' + (num / pow) % 10), size, color);
        pow /= 10;
    }
}
