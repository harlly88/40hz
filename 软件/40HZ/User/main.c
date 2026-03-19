/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *40Hz AM 1kHz Sine Wave Output via R-2R DAC
 *R-2R DAC: 6-bit (BIT0-PC0, BIT1-PC3, BIT2-PC4, BIT3-PC5, BIT4-PC6, BIT5-PC7)
 *TIM1 generates interrupt at 12.8kHz to update DAC output
 *AM modulation: 1kHz sine wave modulated by 40Hz
 */

#include "debug.h"
#include "ch32v00x_i2c.h"

#define SINE_TABLE_SIZE  64
#define OLED_I2C_ADDR    0x78  // OLED I2C地址

// ASCII字符集点阵数据声明
extern u8 asc2_0808[][8];
extern u8 asc2_0806[][6];
extern u8 asc2_1608[][16];

// OLED驱动函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size);
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size);
u32 OLED_Pow(u8 m, u8 n);

// 设置菜单函数声明
void Setting_Menu(void);

// I2C驱动函数声明
void I2C_Init_OLED(void);
void I2C_SendByte(u8 addr, u8 data);
void OLED_WriteCmd(u8 cmd);
void OLED_WriteData(u8 data);

const u8 sine_table[SINE_TABLE_SIZE] = {
    32, 35, 38, 41, 44, 47, 50, 52,
    55, 57, 59, 61, 62, 63, 63, 63,
    63, 63, 63, 62, 61, 59, 57, 55,
    52, 50, 47, 44, 41, 38, 35, 32,
    29, 26, 23, 20, 17, 14, 11, 8,
    5,  3,  1,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  1,  3,  5,  8,
    11, 14, 17, 20, 23, 26, 29, 32
};

volatile u16 sine_index = 0;
volatile u8 am_index = 0;

// 定时功能相关变量
volatile u16 timer_count = 0;  // 定时器计数
volatile u8 countdown_minutes = 30;  // 倒计时分钟数
volatile u8 countdown_seconds = 0;  // 倒计时秒数
volatile u8 timer_running = 1;  // 定时器运行标志

// 旋转编码器相关变量
volatile u8 encoder_state = 0;  // 编码器状态
volatile u16 encoder_count = 0;  // 编码器计数
volatile u8 encoder_key_pressed = 0;  // 按键按下标志
volatile u8 setting_mode = 0;  // 设置模式标志
volatile u8 current_setting = 0;  // 当前设置项
volatile u8 last_setting = 0;  // 上一次设置项

// 设置参数
volatile u8 volume = 32;  // 音量（0-63）
volatile u8 last_volume = 0;  // 上一次音量
volatile u8 last_countdown_minutes = 0;  // 上一次倒计时分钟数
volatile u16 carrier_freq = 1000;  // 载波频率（Hz）
volatile u16 last_carrier_freq = 0;  // 上一次载波频率
volatile u16 am_freq = 40;  // AM频率（Hz）
volatile u16 last_am_freq = 0;  // 上一次AM频率

void DAC_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIOC->BCR = GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
}

void Encoder_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    // PD4 - 按键输入（上拉）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // PD5 - A相输入（上拉）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // PD6 - B相输入（上拉）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      I2C_Init_OLED
 * @brief   初始化I2C总线，用于OLED显示
 * @return  none
 ********************************************************************/
void I2C_Init_OLED(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // 配置PC2为I2C_SCL
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 配置PC1为I2C_SDA
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 初始化I2C
    I2C_InitStructure.I2C_ClockSpeed = 400000;  // 400kHz
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
}

/*********************************************************************
 * @fn      I2C_SendByte
 * @brief   I2C发送一个字节
 * @param   addr - 设备地址
 * @param   data - 要发送的数据
 * @return  none
 ********************************************************************/
void I2C_SendByte(u8 addr, u8 data)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, data);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/*********************************************************************
 * @fn      OLED_WriteCmd
 * @brief   向OLED写入命令
 * @param   cmd - 命令字节
 * @return  none
 ********************************************************************/
void OLED_WriteCmd(u8 cmd)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, OLED_I2C_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x00);  // 命令标志
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, cmd);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/*********************************************************************
 * @fn      OLED_WriteData
 * @brief   向OLED写入数据
 * @param   data - 数据字节
 * @return  none
 ********************************************************************/
void OLED_WriteData(u8 data)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, OLED_I2C_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x40);  // 数据标志
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, data);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

void DAC_Output(u8 value)
{
    u32 temp = value & 0x3F;

    if(temp & 0x01) GPIOC->BSHR = GPIO_Pin_0;
    else           GPIOC->BCR  = GPIO_Pin_0;

    if(temp & 0x02) GPIOC->BSHR = GPIO_Pin_3;
    else           GPIOC->BCR  = GPIO_Pin_3;

    if(temp & 0x04) GPIOC->BSHR = GPIO_Pin_4;
    else           GPIOC->BCR  = GPIO_Pin_4;

    if(temp & 0x08) GPIOC->BSHR = GPIO_Pin_5;
    else           GPIOC->BCR  = GPIO_Pin_5;

    if(temp & 0x10) GPIOC->BSHR = GPIO_Pin_6;
    else           GPIOC->BCR  = GPIO_Pin_6;

    if(temp & 0x20) GPIOC->BSHR = GPIO_Pin_7;
    else           GPIOC->BCR  = GPIO_Pin_7;
}

void Encoder_Read(void)
{
    static u8 last_a = 1, last_b = 1;
    u8 current_a, current_b;

    // 读取A相和B相状态
    current_a = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_5);
    current_b = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_6);

    // 检测旋转方向（改进的AB相检测算法）
    if(current_a != last_a)
    {
        if(current_b != current_a)
        {
            encoder_count++;
        }
        else
        {
            encoder_count--;
        }
    }
    else if(current_b != last_b)
    {
        if(current_a == current_b)
        {
            encoder_count++;
        }
        else
        {
            encoder_count--;
        }
    }

    last_a = current_a;
    last_b = current_b;

    // 检测按键按下
    if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_4) == 0)
    {
        Delay_Ms(10);  // 消抖
        if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_4) == 0)
        {
            encoder_key_pressed = 1;
        }
    }
}

/*********************************************************************
 * @fn      OLED_Init
 * @brief   初始化SSD1315 OLED显示屏（根据官方数据手册）
 * @return  none
 ********************************************************************/
void OLED_Init(void)
{
    Delay_Ms(100);  // 电源稳定延迟

    OLED_WriteCmd(0xAE);  // 关闭显示
    OLED_WriteCmd(0xD5);  // 设置时钟分频因子,震荡频率
    OLED_WriteCmd(0x80);  // [3:0],分频因子;[7:4],震荡频率
    OLED_WriteCmd(0xA8);  // 设置驱动路数
    OLED_WriteCmd(0x3F);  // 1/64 duty (128*64分辨率)
    OLED_WriteCmd(0xD3);  // 设置显示偏移
    OLED_WriteCmd(0x00);  // 无偏移
    OLED_WriteCmd(0x40);  // 设置显示开始行 [5:0],行数.
    OLED_WriteCmd(0x8D);  // 电荷泵设置
    OLED_WriteCmd(0x14);  // 开启电荷泵
    OLED_WriteCmd(0x20);  // 设置内存地址模式
    OLED_WriteCmd(0x02);  // 页地址模式
    OLED_WriteCmd(0xA1);  // 段重定义设置,0->127 (左右翻转)
    OLED_WriteCmd(0xC8);  // COM扫描方向;COM[N-1]->COM0 (上下翻转)
    OLED_WriteCmd(0xDA);  // 设置COM硬件引脚配置
    OLED_WriteCmd(0x12);  // [5:4]配置
    OLED_WriteCmd(0x81);  // 对比度设置
    OLED_WriteCmd(0xCF);  // 128
    OLED_WriteCmd(0xD9);  // 设置预充电周期
    OLED_WriteCmd(0xF1);  // [3:0],PHASE 1;[7:4],PHASE 2;
    OLED_WriteCmd(0xDB);  // 设置VCOMH取消选择级别
    OLED_WriteCmd(0x30);  // [6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc
    OLED_WriteCmd(0xA4);  // 全局显示开启;bit0:1,开启;0,关闭
    OLED_WriteCmd(0xA6);  // 设置显示方式;bit0:1,反相显示;0,正常显示
    OLED_WriteCmd(0xAF);  // 开启显示

    OLED_Clear();  // 清屏
}

void TIM1_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 系统时钟24MHz，预分频器23，周期999
    // 中断频率 = 24,000,000 / (24 × 1000) = 1000Hz
    // 载波频率 = 1000Hz / 64点 = 15.625kHz (不影响波形输出质量)
    TIM_TimeBaseInitStructure.TIM_Period = 999;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 23;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

/*********************************************************************
 * @fn      main
 * @brief   Main program.
 * @return  none
 ********************************************************************/
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    // 初始化DAC GPIO
    DAC_GPIO_Init();

    // 初始化旋转编码器GPIO
    Encoder_GPIO_Init();

    // 初始化I2C和OLED
    I2C_Init_OLED();
    OLED_Init();

    // 显示信息（使用8x8字体）
    OLED_ShowString(0, 0, "40Hz AM Wave", 8);
    OLED_ShowString(0, 2, "Freq:1kHz", 8);
    OLED_ShowString(0, 4, "Mod:40Hz", 8);
    OLED_ShowString(0, 6, "Countdown:", 8);

    // 初始化TIM1
    TIM1_Init();

    while(1)
    {
        // 读取旋转编码器
        Encoder_Read();

        // 处理设置菜单
        Setting_Menu();

        // 延时
        Delay_Ms(10);
    }
}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void)
{
    u8 am_value;
    u16 temp;

    if(timer_running)
    {
        // 恢复AM调制
        am_value = sine_table[am_index];
        // 调整调制深度，避免输出饱和
        temp = (u16)sine_table[sine_index] * (am_value - 32) / 64 + volume;

        if(temp > 63) temp = 63;
        if(temp < 0) temp = 0;

        DAC_Output((u8)temp);

        sine_index++;
        if(sine_index >= SINE_TABLE_SIZE)
        {
            sine_index = 0;
        }

        // 调整AM调制索引更新速度，实现设置的AM频率调制
        static u8 am_step = 0;
        u16 am_update_interval = carrier_freq / am_freq;
        am_step++;
        if(am_step >= am_update_interval)
        {
            am_step = 0;
            am_index++;
            if(am_index >= SINE_TABLE_SIZE)
            {
                am_index = 0;
            }
        }

        // 定时器计数（1kHz中断，每1000次为1秒）
        timer_count++;
        if(timer_count >= 1000)
        {
            timer_count = 0;
            countdown_seconds--;
            if(countdown_seconds == 255)  // 下溢
            {
                countdown_seconds = 59;
                countdown_minutes--;
                if(countdown_minutes == 255)  // 下溢，定时结束
                {
                    timer_running = 0;
                    TIM_Cmd(TIM1, DISABLE);  // 关闭TIM1定时器
                    DAC_Output(0);  // 关闭DAC输出
                    OLED_ShowString(64, 6, " 00:00", 8);
                    return;
                }
            }
            // 更新倒计时显示
            if(!setting_mode)
            {
                OLED_ShowNum(64, 6, countdown_minutes, 2, 8);
                OLED_ShowChar(80, 6, ':', 8);
                OLED_ShowNum(88, 6, countdown_seconds, 2, 8);
            }
        }
    }

    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}

/*********************************************************************
 * @fn      OLED_Clear
 * @brief   清屏函数
 * @return  none
 ********************************************************************/
void OLED_Clear(void)
{
    u8 i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_WriteCmd(0xB0 + i);  // 设置页地址
        OLED_WriteCmd(0x00);      // 设置显示位置-列低地址
        OLED_WriteCmd(0x10);      // 设置显示位置-列高地址
        for(n = 0; n < 128; n++)
        {
            OLED_WriteData(0x00);  // 写入空数据
        }
    }
}

/*********************************************************************
 * @fn      OLED_ShowChar
 * @brief   显示一个字符（适配SSD1315 128*64）
 * @param   x - 起始列地址
 * @param   y - 起始页地址
 * @param   chr - 要显示的字符
 * @param   size - 字符大小
 * @return  none
 ********************************************************************/
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size)
{
    u8 c = 0, i = 0;
    c = chr - ' ';  // 得到偏移后的值
    if(x > 128 - size) { x = 0; y += 1; }

    if(size == 16)
    {
        // 设置页地址
        OLED_WriteCmd(0xB0 + y);
        // 设置列地址高4位
        OLED_WriteCmd(0x10 | (x >> 4));
        // 设置列地址低4位
        OLED_WriteCmd(x & 0x0F);
        
        // 显示上半部分
        for(i = 0; i < 8; i++)
        {
            OLED_WriteData(asc2_1608[c][i]);
        }
        
        // 设置下一页
        OLED_WriteCmd(0xB0 + y + 1);
        // 设置列地址高4位
        OLED_WriteCmd(0x10 | (x >> 4));
        // 设置列地址低4位
        OLED_WriteCmd(x & 0x0F);
        
        // 显示下半部分
        //for(i = 0; i < 8; i++)
        //{
        //    OLED_WriteData(asc2_1608[c][i + 8]);
        //}
    }
    else if(size == 8)
    {
        // 8x8字体
        OLED_WriteCmd(0xB0 + y);
        OLED_WriteCmd(0x10 | (x >> 4));
        OLED_WriteCmd(x & 0x0F);
        for(i = 0; i < 8; i++)
        {
            OLED_WriteData(asc2_0808[c][i]);
        }
    }
    else
    {
        // 8x6字体（暂时不使用）
        OLED_WriteCmd(0xB0 + y);
        OLED_WriteCmd(0x10 | (x >> 4));
        OLED_WriteCmd(x & 0x0F);
        for(i = 0; i < 6; i++)
        {
            OLED_WriteData(asc2_0806[c][i]);
        }
    }
}

/*********************************************************************
 * @fn      OLED_ShowString
 * @brief   显示字符串
 * @param   x - 起始列地址
 * @param   y - 起始页地址
 * @param   chr - 要显示的字符串
 * @param   size - 字符大小
 * @return  none
 ********************************************************************/
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size)
{
    while(*chr != '\0')
    {
        OLED_ShowChar(x, y, *chr, size);
        x += size;
        if(x > 128)
        {
            x = 0;
            y += 2;
        }
        chr++;
    }
}

/*********************************************************************
 * @fn      OLED_ShowNum
 * @brief   显示数字
 * @param   x - 起始列地址
 * @param   y - 起始页地址
 * @param   num - 要显示的数字
 * @param   len - 数字长度
 * @param   size - 字符大小
 * @return  none
 ********************************************************************/
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size)
{
    u8 t, temp;
    u8 enshow = 0;

    for(t = 0; t < len; t++)
    {
        temp = (num / OLED_Pow(10, len - t - 1)) % 10;
        if(enshow == 0 && t < len - 1)
        {
            if(temp == 0)
            {
                OLED_ShowChar(x + (size * t), y, ' ', size);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }
        OLED_ShowChar(x + (size * t), y, temp + '0', size);
    }
}

/*********************************************************************
 * @fn      OLED_Pow
 * @brief   求幂函数
 * @param   m - 底数
 * @param   n - 指数
 * @return  m^n
 ********************************************************************/
u32 OLED_Pow(u8 m, u8 n)
{
    u32 result = 1;
    while(n--)
    {
        result *= m;
    }
    return result;
}

void Setting_Menu(void)
{
    static u16 last_encoder_count = 0;

    // 检测按键按下
    if(encoder_key_pressed)
    {
        encoder_key_pressed = 0;
        if(setting_mode)
        {
            // 切换到下一个设置项
            current_setting++;
            if(current_setting >= 4)
            {
                setting_mode = 0;
                current_setting = 0;
                OLED_Clear();
                OLED_ShowString(0, 0, "40Hz AM Wave", 8);
                OLED_ShowString(0, 2, "Freq:1kHz", 8);
                OLED_ShowString(0, 4, "Mod:40Hz", 8);
                OLED_ShowString(0, 6, "Countdown:", 8);
                return;
            }
        }
        else
        {
            // 进入设置模式
            setting_mode = 1;
            current_setting = 0;
            last_encoder_count = encoder_count;
            last_setting = 0;
            last_volume = 0;
            last_countdown_minutes = 0;
            last_carrier_freq = 0;
            last_am_freq = 0;
            OLED_Clear();
            OLED_ShowString(0, 0, "Setting Mode", 8);
            OLED_ShowString(0, 6, "Press to exit", 8);
        }
    }

    if(setting_mode)
    {
        // 处理编码器旋转
        if(encoder_count != last_encoder_count)
        {
            u16 diff = encoder_count - last_encoder_count;
            last_encoder_count = encoder_count;

            switch(current_setting)
            {
                case 0:  // 音量设置
                    volume += diff / 2;
                    if(volume > 63) volume = 63;
                    if(volume < 0) volume = 0;
                    break;
                case 1:  // 定时时间设置
                    countdown_minutes += diff / 2;
                    if(countdown_minutes > 60) countdown_minutes = 60;
                    if(countdown_minutes < 0) countdown_minutes = 0;
                    break;
                case 2:  // 载波频率设置
                    carrier_freq += diff * 100;
                    if(carrier_freq > 2000) carrier_freq = 2000;
                    if(carrier_freq < 500) carrier_freq = 500;
                    break;
                case 3:  // AM频率设置
                    am_freq += diff;
                    if(am_freq > 100) am_freq = 100;
                    if(am_freq < 10) am_freq = 10;
                    break;
            }
        }

        // 只在设置项变化或参数变化时更新显示
        if(current_setting != last_setting || 
           (current_setting == 0 && volume != last_volume) ||
           (current_setting == 1 && countdown_minutes != last_countdown_minutes) ||
           (current_setting == 2 && carrier_freq != last_carrier_freq) ||
           (current_setting == 3 && am_freq != last_am_freq))
        {
            // 清除当前设置项区域
            OLED_WriteCmd(0xB0 + 2);
            OLED_WriteCmd(0x00);
            OLED_WriteCmd(0x10);
            for(u8 i = 0; i < 128; i++)
            {
                OLED_WriteData(0x00);
            }

            // 显示当前设置项
            switch(current_setting)
            {
                case 0:
                    OLED_ShowString(0, 2, "Volume:", 8);
                    OLED_ShowNum(48, 2, volume, 2, 8);
                    last_volume = volume;
                    break;
                case 1:
                    OLED_ShowString(0, 2, "Timer:", 8);
                    OLED_ShowNum(48, 2, countdown_minutes, 2, 8);
                    OLED_ShowString(64, 2, "min", 8);
                    last_countdown_minutes = countdown_minutes;
                    break;
                case 2:
                    OLED_ShowString(0, 2, "Carrier:", 8);
                    OLED_ShowNum(48, 2, carrier_freq, 4, 8);
                    OLED_ShowString(80, 2, "Hz", 8);
                    last_carrier_freq = carrier_freq;
                    break;
                case 3:
                    OLED_ShowString(0, 2, "AM Freq:", 8);
                    OLED_ShowNum(48, 2, am_freq, 3, 8);
                    OLED_ShowString(72, 2, "Hz", 8);
                    last_am_freq = am_freq;
                    break;
            }

            last_setting = current_setting;
        }
    }
}

// 标准8x8 ASCII字符点阵数据（SSD1315兼容，从空格(0x20)开始）
u8 asc2_0808[][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x20 (space)
    {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00}, // 0x21 !
    {0x00,0x07,0x00,0x07,0x00,0x00,0x00,0x00}, // 0x22 "
    {0x14,0x7F,0x14,0x7F,0x14,0x00,0x00,0x00}, // 0x23 #
    {0x24,0x2A,0x7F,0x2A,0x12,0x00,0x00,0x00}, // 0x24 $
    {0x23,0x13,0x08,0x64,0x62,0x00,0x00,0x00}, // 0x25 %
    {0x36,0x49,0x55,0x22,0x50,0x00,0x00,0x00}, // 0x26 &
    {0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00}, // 0x27 '
    {0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00}, // 0x28 (
    {0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00}, // 0x29 )
    {0x14,0x08,0x3E,0x08,0x14,0x00,0x00,0x00}, // 0x2A *
    {0x08,0x08,0x3E,0x08,0x08,0x00,0x00,0x00}, // 0x2B +
    {0x00,0x50,0x30,0x00,0x00,0x00,0x00,0x00}, // 0x2C ,
    {0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00}, // 0x2D -
    {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, // 0x2E .
    {0x20,0x10,0x08,0x04,0x02,0x00,0x00,0x00}, // 0x2F /
    {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00}, // 0x30 0
    {0x42,0x7F,0x40,0x00,0x00,0x00,0x00,0x00}, // 0x31 1
    {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00}, // 0x32 2
    {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00}, // 0x33 3
    {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00}, // 0x34 4
    {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00}, // 0x35 5
    {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00}, // 0x36 6
    {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00}, // 0x37 7
    {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, // 0x38 8
    {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00}, // 0x39 9
    {0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00}, // 0x3A :
    {0x00,0x56,0x36,0x00,0x00,0x00,0x00,0x00}, // 0x3B ;
    {0x08,0x14,0x22,0x41,0x00,0x00,0x00,0x00}, // 0x3C <
    {0x24,0x24,0x24,0x24,0x24,0x00,0x00,0x00}, // 0x3D =
    {0x00,0x41,0x22,0x14,0x08,0x00,0x00,0x00}, // 0x3E >
    {0x02,0x01,0x51,0x09,0x06,0x00,0x00,0x00}, // 0x3F ?
    {0x32,0x49,0x79,0x41,0x3E,0x00,0x00,0x00}, // 0x40 @
    {0x7E,0x09,0x09,0x09,0x7E,0x00,0x00,0x00}, // 0x41 A
    {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, // 0x42 B
    {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00}, // 0x43 C
    {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00}, // 0x44 D
    {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00}, // 0x45 E
    {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00}, // 0x46 F
    {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00}, // 0x47 G
    {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00}, // 0x48 H
    {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00}, // 0x49 I
    {0x20,0x40,0x40,0x3F,0x00,0x00,0x00,0x00}, // 0x4A J
    {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00}, // 0x4B K
    {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // 0x4C L
    {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00}, // 0x4D M
    {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00}, // 0x4E N
    {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00}, // 0x4F O
    {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00}, // 0x50 P
    {0x3E,0x41,0x51,0x61,0x3E,0x00,0x00,0x00}, // 0x51 Q
    {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00}, // 0x52 R
    {0x26,0x49,0x49,0x49,0x32,0x00,0x00,0x00}, // 0x53 S
    {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00}, // 0x54 T
    {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00}, // 0x55 U
    {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00}, // 0x56 V
    {0x7F,0x20,0x18,0x20,0x7F,0x00,0x00,0x00}, // 0x57 W
    {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00}, // 0x58 X
    {0x03,0x04,0x78,0x04,0x03,0x00,0x00,0x00}, // 0x59 Y
    {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00}, // 0x5A Z
    {0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00}, // 0x5B [
    {0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x00}, // 0x5C 
    {0x00,0x00,0x00,0x41,0x41,0x7F,0x00,0x00}, // 0x5D ]
    {0x04,0x02,0x01,0x02,0x04,0x00,0x00,0x00}, // 0x5E ^
    {0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // 0x5F _
    {0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00}, // 0x60 `
    {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00}, // 0x61 a
    {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00}, // 0x62 b
    {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00}, // 0x63 c
    {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00}, // 0x64 d
    {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00}, // 0x65 e
    {0x08,0x7E,0x09,0x09,0x00,0x00,0x00,0x00}, // 0x66 f
    {0x00,0x18,0xA4,0xA4,0x7C,0x00,0x00,0x00}, // 0x67 g
    {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, // 0x68 h
    {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00}, // 0x69 i
    {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00}, // 0x6A j
    {0x7F,0x10,0x28,0x44,0x00,0x00,0x00,0x00}, // 0x6B k
    {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00}, // 0x6C l
    {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00}, // 0x6D m
    {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, // 0x6E n
    {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00}, // 0x6F o
    {0xFC,0x18,0x24,0x24,0x18,0x00,0x00,0x00}, // 0x70 p
    {0x18,0x24,0x24,0x18,0xFC,0x00,0x00,0x00}, // 0x71 q
    {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00}, // 0x72 r
    {0x48,0x54,0x54,0x54,0x24,0x00,0x00,0x00}, // 0x73 s
    {0x04,0x3F,0x44,0x44,0x20,0x00,0x00,0x00}, // 0x74 t
    {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00}, // 0x75 u
    {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00}, // 0x76 v
    {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00}, // 0x77 w
    {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00}, // 0x78 x
    {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00}, // 0x79 y
    {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00}, // 0x7A z
    {0x00,0x08,0x36,0x41,0x00,0x00,0x00,0x00}, // 0x7B {
    {0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00}, // 0x7C |
    {0x00,0x41,0x36,0x08,0x00,0x00,0x00,0x00}, // 0x7D }
    {0x08,0x04,0x08,0x10,0x08,0x00,0x00,0x00}, // 0x7E ~
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // 0x7F (DEL)
};

// 简化的8x6点阵数据（暂时不使用）
u8 asc2_0806[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 仅保留空格
};

// 标准16x8 ASCII字符点阵数据（SSD1315兼容，从空格(0x20)开始）
u8 asc2_1608[][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x20 (space)
};
