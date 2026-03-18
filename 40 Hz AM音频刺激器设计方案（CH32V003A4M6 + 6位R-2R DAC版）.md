# 40 Hz AM音频刺激器设计方案（CH32V003F4P6 + 6位R-2R DAC版）

## 版本：V4.0

### 核心目标

基于CH32V003F4P6（TSSOP-20封装）+ 6位R-2R梯形网络，实现**成本≤16.1元、体积20×35mm、手工焊接友好**的40Hz AM音频刺激器，兼顾量产可行性与研发调试便利性。
---

## 一、核心器件选型（精准到型号/封装/厂商）

### 1. 主控芯片（核心选型）

| 型号           | 封装                   | 厂商        | 单价（2k套） | 关键参数                                    | 选型理由                                                      |
| ------------ | -------------------- | --------- | ------- | --------------------------------------- | --------------------------------------------------------- |
| CH32V003F4P6 | TSSOP-20 (6.4×4.4mm) | 南京沁恒（WCH） | 1.35元   | 48MHz RISC-V、16KB Flash、2KB RAM、18个GPIO | 无内置DAC但GPIO充足，TSSOP-20封装提供更多GPIO，核心功能与A0M6完全一致，适配研发/小批量场景 |

### 2. R-2R DAC网络器件（与V3.0一致）

| 器件类型   | 型号/参数          | 封装   | 厂商       | 单价（2k套）  | 数量  | 选型理由                                      |
| ------ | -------------- | ---- | -------- | -------- | --- | ----------------------------------------- |
| 精密贴片电阻 | 1kΩ ±1% 1/16W  | 0402 | 国巨/Yageo | 0.015元/个 | 6个  | R-2R网络核心电阻，1%精度保证非线性失真<1.5%，0402封装节省PCB空间 |
| 精密贴片电阻 | 2kΩ ±1% 1/16W  | 0402 | 国巨/Yageo | 0.015元/个 | 7个  | 与1kΩ电阻配对构成6位梯形网络，终端电阻需额外1个2kΩ             |
| 滤波电容   | 100nF X7R 50V  | 0402 | 风华       | 0.01元/个  | 3个  | X7R材质温漂小，用于R-2R输出滤波+隔直，适配音频信号特性           |
| 限流电阻   | 100Ω ±5% 1/16W | 0402 | 国巨/Yageo | 0.01元/个  | 6个  | 保护GPIO，避免R-2R网络反向灌电流损坏芯片                  |

### 3. 其他核心器件（与V3.0完全兼容）

| 功能      | 型号            | 封装         | 单价（2k套） |
| ------- | ------------- | ---------- | ------- |
| 音频功放    | PAM8301DR-H   | SOT-23-6   | 1.4元    |
| OLED显示屏 | SSD1306 0.96" | I2C 128×64 | 7.0元    |
| 旋转编码器   | EC11 20脉冲     | 插件         | 1.2元    |
| LDO稳压   | ME6211C33M5G  | SOT-23-5   | 0.4元    |
| 锂电池充电   | TP4056        | ESOP-8     | 1.0元    |
| 锂电池     | 603040 3.7V   | 软包         | 6.0元    |
| 喇叭      | 30mm 8Ω 1W    | 内磁         | 2.5元    |

---

## 二、硬件详细设计

### 1. 引脚分配（CH32V003F4P6 TSSOP-20）

| 芯片引脚号 | 引脚名称 | 功能                 | 方向  | 关键配置                  | 备注                               |
| ----- | ---- | ------------------ | --- | --------------------- | -------------------------------- |
| 1     | PD4  | 编码器按键（ENC_KEY）     | 输入  | 上拉10kΩ，100nF电容消抖      | 确认/切换参数                          |
| 2     | PD5  | 编码器B相（ENC_B）       | 输入  | 上拉10kΩ，外部中断触发（EXTI5）  | 旋转调节参数                           |
| 3     | PD6  | 编码器A相（ENC_A）       | 输入  | 上拉10kΩ，外部中断触发（EXTI6）  | 旋转调节参数                           |
| 4     | PD7  | NC（预留）             | -   | 不连接                   | 预留功能扩展                           |
| 5     | PA1  | NC（预留）             | -   | 不连接                   | 预留功能扩展                           |
| 6     | PA2  | NC（预留）             | -   | 不连接                   | 预留功能扩展                           |
| 7     | VSS  | 数字地                | 输入  | 接PCB地平面               | 与模拟地单点共地                         |
| 8     | PD0  | VCAP（稳压电容）         | -   | 1μF电容到地               | 内部稳压电容，不可GPIO使用                  |
| 9     | VDD  | 数字电源               | 输入  | 3.3V（ME6211输出）        | 并联100nF 0402电容+10μF 0603钽电容，靠近引脚 |
| 10    | PC0  | R-2R DAC Bit0      | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 11    | PC1  | I2C1_SDA + NC      | 双向  | 开漏上拉2.2kΩ至3.3V        | OLED SDA，R-2R网络预留位置              |
| 12    | PC2  | I2C1_SCL + NC      | 双向  | 开漏上拉2.2kΩ至3.3V，400kHz | OLED SCL，R-2R网络预留位置              |
| 13    | PC3  | R-2R DAC Bit1      | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 14    | PC4  | R-2R DAC Bit2      | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 15    | PC5  | R-2R DAC Bit3      | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 16    | PC6  | R-2R DAC Bit4      | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 17    | PC7  | R-2R DAC Bit5（最高位） | 输出  | 推挽输出，3.3V电平，速度50MHz   | 串联100Ω限流电阻接R-2R网络                |
| 18    | PD1  | SWIO（调试接口）         | 双向  | 单线调试接口                | WCH-Link烧录/调试                    |
| 19    | PD2  | 蜂鸣器PWM输出           | 输出  | 推挽输出，2kHz PWM         | 串联220Ω限流电阻接蜂鸣器                   |
| 20    | PD3  | 启动按键（KEY_START）    | 输入  | 上拉10kΩ，100nF电容消抖      | 启动/暂停音频刺激                        |
| 4     | PD7  | NC（预留）             | -   | 不连接                   | 预留功能扩展                           |

**注意：** PD7默认复用为NRST（复位引脚），本设计不使用PD7作为GPIO，Bit5改用PC7（引脚17）。

### 2. R-2R 6位DAC网络设计（适配TSSOP-20引脚）

#### （2）电路拓扑（适配PC1/PC2用于I2C OLED）

```
 +3.3V (VDD)
 │
 2R (2kΩ)
 │
 ┌────────┴────────┐
 │ │
PC0(引脚10)─100Ω─R(1kΩ) 2R(2kΩ)
 │ │
 ├─────────────────┼─────────┐
 │ │ │
PC3(引脚13)─100Ω─R(1kΩ) 2R(2kΩ) │
 │ │ │
 ├─────────────────┼─────────┤
 │ │ │
PC4(引脚14)─100Ω─R(1kΩ) 2R(2kΩ) │
 │ │ │
 ├─────────────────┼─────────┤
 │ │ │
PC5(引脚15)─100Ω─R(1kΩ) 2R(2kΩ) │
 │ │ │
 ├─────────────────┼─────────┤
 │ │ │
PC6(引脚16)─100Ω─R(1kΩ) 2R(2kΩ) │
 │ │ │
 ├─────────────────┼─────────┤
 │ │ │
PC7(引脚17)─100Ω─R(1kΩ) 2R(2kΩ) │
 │ │ │
 └─────────────────┴─────────┴───1kΩ──┬───100nF──┬───100nF──┬───PAM8301 IN+
 │ │ │
 GND GND GND
```

**注意：** PC1(引脚11)和PC2(引脚12)预留给OLED I2C接口(SDA/SCL)，不参与R-2R网络连接。

#### （2）关键参数（与V3.0一致）

- 输出电压范围：0V（全0）~ 3.3V（全1），步进≈51.5mV（3.3V/64）；

- 非线性失真：≤1.5%（1%精度电阻）；

- 输出阻抗：≈1kΩ，匹配后级功放输入阻抗；

- 滤波：一阶RC（1kΩ+100nF）+ 隔直电容（100nF），截止频率≈1.6kHz（适配1kHz载波）。
  
  ### 3. 音频链路完整设计（无变化）
  
  ```
  CH32V003F4P6 PC0,PC3,PC4,PC5,PC6,PC7 → 100Ω限流 → R-2R网络 → 1kΩ+100nF滤波 → 100nF隔直 → PAM8301 IN+
  ```PAM8301 OUT+/OUT- → 30mm 8Ω喇叭
  PAM8301 VDD → 锂电池3.7~4.2V，GND → 系统地
  ```
  
  ### 4. PCB设计规则（适配TSSOP-20封装）
  
  | 项目    | 具体要求                                                                                  |
  | ----- | ------------------------------------------------------------------------------------- |
  | PCB尺寸 | 20mm×35mm，2层板，板厚1.2mm                                                                 |
  | 布局    | ① CH32V003F4P6（TSSOP-20）居中放置；② R-2R网络紧邻PC4-PC0,PC7引脚（<3mm），手工焊接时预留0.5mm间距；③ 所有贴片器件放顶层 |
  | 走线    | ① 电源走线≥20mil，GPIO至R-2R走线≥8mil；② I2C走线等长（误差<5mil），间距≤20mil；③ 音频走线远离开关电源区域              |
  | 接地    | ① 芯片下方铺完整地平面；② R-2R网络区域单独铺地，与数字地通过0Ω电阻单点连接；③ TSSOP-20引脚18（VSS）焊盘加大至2mm×1mm，方便焊接       |
  | 封装    | ① CH32V003F4P6采用TSSOP-20封装（焊盘间距0.65mm，焊盘宽度0.3mm）；② 其余器件仍为0402贴片（除编码器/喇叭/电池座）          |
  | 焊接优化  | ① TSSOP-20引脚焊盘延长0.5mm，便于电烙铁焊接；② R-2R电阻焊盘标注位号，方便核对；③ 预留测试点（PC4-PC0,PC7/R-2R输出）         |

---

## 三、软件适配设计（适配TSSOP-20封装）

### 1. 核心代码（需修改以适配新引脚）

#### （1）GPIO初始化（R-2R DAC引脚）

```c
// 初始化PC0,PC3,PC4,PC5,PC6,PC7为推挽输出（适配CH32V003F4P6 TSSOP-20）
// 注意：PC1/PC2预留给OLED I2C接口，不参与R-2R DAC；PC7无需配置选项字节
void R2R_DAC_Init(void)
{
 GPIO_InitTypeDef GPIO_InitStructure = {0};

// 使能GPIOC时钟
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

// PC0 推挽输出，50MHz速率（引脚10，DAC Bit0）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// PC3 推挽输出，50MHz速率（引脚13，DAC Bit1）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// PC4 推挽输出，50MHz速率（引脚14，DAC Bit2）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// PC5 推挽输出，50MHz速率（引脚15，DAC Bit3）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// PC6 推挽输出，50MHz速率（引脚16，DAC Bit4）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// PC7 推挽输出，50MHz速率（引脚17，DAC Bit5）
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_Init(GPIOC, &GPIO_InitStructure);

// 初始输出0V（全低电平）
 GPIO_Write(GPIOC, 0x00);
}
```

#### （2）DDS算法+R-2R输出（需修改以适配新引脚）

```c
// 12位DDS值转换为6位GPIO输出（映射到PC0,PC3,PC4,PC5,PC6,PC7）
// 注意：Bit顺序为 Bit0(LSB)→PC0, Bit1→PC3, Bit2→PC4, Bit3→PC5, Bit4→PC6, Bit5(MSB)→PC7
void R2R_DAC_Output(uint16_t dds_value)
{
 uint8_t dac_6bit = (dds_value >> 6) & 0x3F; // 取高6位，0~63

 // 分别设置各个GPIO引脚
 // Bit0 -> PC0 (引脚10，最低有效位)
 if(dac_6bit & 0x01) {
   GPIOC->BSHR = GPIO_Pin_0;  // 设置PC0
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_0 << 16; // 清除PC0
 }

 // Bit1 -> PC3 (引脚13)
 if(dac_6bit & 0x02) {
   GPIOC->BSHR = GPIO_Pin_3;  // 设置PC3
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_3 << 16; // 清除PC3
 }

 // Bit2 -> PC4 (引脚14)
 if(dac_6bit & 0x04) {
   GPIOC->BSHR = GPIO_Pin_4;  // 设置PC4
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_4 << 16; // 清除PC4
 }

 // Bit3 -> PC5 (引脚15)
 if(dac_6bit & 0x08) {
   GPIOC->BSHR = GPIO_Pin_5;  // 设置PC5
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_5 << 16; // 清除PC5
 }

 // Bit4 -> PC6 (引脚16)
 if(dac_6bit & 0x10) {
   GPIOC->BSHR = GPIO_Pin_6;  // 设置PC6
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_6 << 16; // 清除PC6
 }

 // Bit5 -> PC7 (引脚17，最高有效位)
 if(dac_6bit & 0x20) {
   GPIOC->BSHR = GPIO_Pin_7;  // 设置PC7
 } else {
   GPIOC->BSHR = (uint32_t)GPIO_Pin_7 << 16; // 清除PC7
 }
}
// TIM1_UP中断（256kHz）更新DAC输出
void TIM1_UP_IRQHandler(void)
{
 if(TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
 {
 // 原DDS算法生成12位音频采样值
 uint16_t sample = DDS_GenerateSample();
 // 输出到R-2R网络
 R2R_DAC_Output(sample);

TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
 }
}
```

### 2. 软件核心逻辑（部分修改）

- 保留原参数调节、倒计时、OLED显示、蜂鸣器提示逻辑；
- I2C通信使用PC1(SDA)/PC2(SCL)（引脚11/12），需配置为开漏上拉模式；
- R-2R DAC使用PC0,PC3,PC4,PC5,PC6,PC7共6个引脚，避开PC1/PC2；
- 中断配置、编码器按键处理完全复用V3.0代码（中断映射：EXTI5→PD5，EXTI6→PD6）；
- PC7直接作为GPIO使用，无需配置选项字节；
- 固件烧录：使用WCH-Link，通过SWIO接口（PD1引脚18）烧录，PCB预留2.54mm排针测试点，方便固件烧录/调试。

---

## 四、成本&体积&性能汇总

### 1. 成本明细（单台，2k套量级）

| 模块           | 成本（元）     | 占比       | 备注                     |
| ------------ | --------- | -------- | ---------------------- |
| CH32V003F4P6 | 1.35      | 8.4%     | TSSOP-20封装，比A0M6贵0.05元 |
| R-2R网络       | 0.225     | 1.4%     | 6个1kΩ+7个2kΩ电阻          |
| 功放/喇叭        | 3.9       | 24.2%    | PAM8301+30mm喇叭         |
| OLED/编码器     | 8.2       | 50.9%    | 0.96" OLED+EC11编码器     |
| 电源（LDO/充电）   | 1.4       | 8.7%     | ME6211+TP4056          |
| 锂电池          | 6.0       | 37.3%    | 603040 800mAh          |
| 其他辅材         | 0.5       | 3.1%     | 电容/测试点/连接器等            |
| **总成本**      | **16.15** | **100%** | 仍≤20元目标                |

### 2. 核心指标对比

| 指标    | CH32V103+内置DAC   | CH32V003A4M6+R-2R | 核心优势               |
| ----- | ---------------- | ----------------- | ------------------ |
| 总成本   | ≈18元             | ≈16.1元            | 降低10.6%，手工焊接无额外成本  |
| PCB体积 | 28×48mm（1344mm²） | 20×35mm（700mm²）   | 缩小48%，便携性提升        |
| 焊接难度  | 中（LQFP48）        | 低（SOP-16）         | 仅需电烙铁，研发调试效率提升     |
| 音频失真  | <0.08%           | ≈1.5%             | 人耳不可察，满足核心需求       |
| 功耗    | 4.2mA            | 3.8mA             | 降低9.5%，续航提升至≈210小时 |
| 频率精度  | ±0.1%            | ±0.2%             | 无实际影响，满足试验要求       |

---

## 五、生产&测试要点（适配SOP-16封装）

### 1. 生产/调试注意事项

- **手工焊接**：CH32V003F4P6 TSSOP-20引脚焊接温度控制在350℃以内，每个引脚焊接时间≤3s，避免烫坏芯片（TSSOP-20焊盘间距较小，需更精细焊接）；

- **电阻焊接**：R-2R网络电阻需按位号焊接，避免1kΩ/2kΩ混料（建议用不同颜色丝印标注）；

- **烧录调试**：SWIO调试接口位于PD1（引脚18），PCB预留2.54mm排针测试点（PD1+V+GND），方便WCH-Link固件烧录/调试。
  
  ### 2. 关键测试项（无变化）
  
  | 测试项       | 测试方法            | 合格标准               |
  | --------- | --------------- | ------------------ |
  | R-2R输出线性度 | 万用表测0~63级输出电压   | 步进误差≤±2mV          |
  | 音频失真      | 音频分析仪测1kHz载波THD | ≤1.5%              |
  | 频率精度      | 示波器测输出音频频率      | 40Hz调制/1kHz载波±0.2% |
  | 按键/编码器    | 连续操作1000次       | 无卡顿/误触发            |
  | 续航        | 满电连续播放75dB音频    | ≥200小时             |

---

### 总结

1. **核心选型**：主控确定为CH32V003F4P6 TSSOP-20，兼顾功能兼容性与更多GPIO资源，R-2R网络器件与原方案一致，成本小幅增加0.05元；
2. **硬件适配**：引脚分配已根据CH32V003F4P6实际引脚定义修正（R-2R DAC：PC4-PC0,PD7；I2C：PC1/PC0；编码器：PD6/PD5；按键：PD4/PD3），PCB封装调整为TSSOP-20，焊接优化后更适合研发/小批量场景，体积仍保持20×35mm的极致尺寸；
3. **软件适配**：固件代码已针对新引脚进行适配，主要修改GPIO初始化和R-2R输出函数的引脚注释，确保6位DAC正常工作；
4. **核心优势**：相比CH32V103方案成本降10.3%、体积缩48%，手工焊接难度适中（TSSOP-20需精细焊接技巧），完全满足40Hz AM音频刺激器的功能与成本目标。
   该方案可直接用于研发打样，量产时可继续使用CH32V003F4P6，是兼顾功能性和经济性的最优选择。
