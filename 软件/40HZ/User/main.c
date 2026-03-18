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

#define SINE_TABLE_SIZE  64

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

void TIM1_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 系统时钟24MHz，预分频器0，周期374
    // 中断频率 = 24,000,000 / (1 × 375) = 64,000Hz
    // 载波频率 = 64,000Hz / 64点 = 1000Hz (精确1kHz)
    TIM_TimeBaseInitStructure.TIM_Period = 374;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
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

void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void)
{
    u8 am_value;
    u16 temp;

    // 恢复AM调制
    am_value = sine_table[am_index];
    // 调整调制深度，避免输出饱和
    // temp = (u16)sine_table[sine_index] * am_value / 32;
    temp = (u16)sine_table[sine_index] * (am_value - 32) / 64 + 32;
    // temp = sine_table[sine_index];

    if(temp > 63) temp = 63;
    if(temp < 0) temp = 0;

    DAC_Output((u8)temp);

    sine_index++;
    if(sine_index >= SINE_TABLE_SIZE)
    {
        sine_index = 0;
    }

    // 调整AM调制索引更新速度，实现40Hz调制
    // 1kHz / 40Hz = 25，每25次载波更新一次调制
    static u8 am_step = 0;
    am_step++;
    if(am_step >= 25)
    {
        am_step = 0;
        am_index++;
        if(am_index >= SINE_TABLE_SIZE)
        {
            am_index = 0;
        }
    }

    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();

    DAC_GPIO_Init();
    TIM1_Init();

    while(1)
    {
    }
}
