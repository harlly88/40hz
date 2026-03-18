CH32V003F4P6 是一款性价比极高的 RISC-V 架构微控制器。在 TSSOP-20 封装下，它提供了 20 个引脚，其中包含电源、复位以及丰富的外设复用功能。

以下是 CH32V003F4P6 (TSSOP-20) 的详细引脚功能对照表：

### CH32V003F4P6 TSSOP-20 引脚定义

| **引脚编号** | **引脚名称** | **类型** | **主要功能 / 复用功能 (Default / Alternate Functions)** |
| -------- | -------- | ------ | ----------------------------------------------- |
| **1**    | **PD4**  | I/O    | GPIO, ADC_CH7, TIM1_CH4, T2CH3                  |
| **2**    | **PD5**  | I/O    | GPIO, ADC_CH5, **UART_TX**, TIM1_CH3            |
| **3**    | **PD6**  | I/O    | GPIO, ADC_CH6, **UART_RX**, TIM1_CH2            |
| **4**    | **PD7**  | I/O    | GPIO, **NRST (外部复位引脚，可配置为GPIO)**                |
| **5**    | **PA1**  | I/O    | GPIO, OSCI (外部晶振输入), TIM2_CH2                   |
| **6**    | **PA2**  | I/O    | GPIO, OSCO (外部晶振输出), TIM2_CH3                   |
| **7**    | **VSS**  | 电源     | **地 (GND)**                                     |
| **8**    | **PD0**  | I/O    | GPIO, **VCAP (外部稳压电容引脚)**                       |
| **9**    | **VDD**  | 电源     | **正电源 (3.3V / 5V)**                             |
| **10**   | **PC0**  | I/O    | GPIO, ADC_CH2, I2C_SCL                          |
| **11**   | **PC1**  | I/O    | GPIO, ADC_CH1, **I2C_SDA**, TIM1_CH4 (5V耐压)     |
| **12**   | **PC2**  | I/O    | GPIO, ADC_CH3, TIM1_CH1, TIM1_ETR (5V耐压)        |
| **13**   | **PC3**  | I/O    | GPIO, ADC_CH4, TIM1_CH3                         |
| **14**   | **PC4**  | I/O    | GPIO, ADC_CH0, **SPI_SCK**, TIM1_CH4            |
| **15**   | **PC5**  | I/O    | GPIO, **SPI_NSS**, TIM2_CH1, TIM2_ETR (5V耐压)    |
| **16**   | **PC6**  | I/O    | GPIO, **SPI_MOSI**, TIM1_CH1 (5V耐压)             |
| **17**   | **PC7**  | I/O    | GPIO, **SPI_MISO**, TIM1_CH2                    |
| **18**   | **PD1**  | I/O    | GPIO, **SWIO (单线调试接口)**                         |
| **19**   | **PD2**  | I/O    | GPIO, ADC_CH3, TIM1_CH1, TIM1_N                 |
| **20**   | **PD3**  | I/O    | GPIO, ADC_CH4, TIM1_CH2                         |

---

### 关键引脚说明

- **电源引脚 (VDD/VSS)**：支持 3.3V 或 5V 供电。通常在 VDD 与 VSS 之间建议并联一个 0.1μF 的退耦电容。

- **调试接口 (SWIO)**：CH32V003 使用单线 SDI 接口进行下载和调试，位于 **PD1**。

- **VCAP 引脚**：**PD0** 通常需要连接一个 1μF 左右的电容到地，用于内部核心电压稳压，除非在选项字节中将其配置为普通 GPIO。

- **复位引脚 (PD7/NRST)**：默认作为外部硬件复位。如果引脚资源紧张，可以通过烧写选项字节将其切换为普通 GPIO 使用。

- **ADC 通道**：分布在 PD 和 PC 端口上，最高支持 10-bit 分辨率。

---

希望这份列表能帮到你的硬件设计。如果你需要具体的 **UART 串口通信** 或 **PWM 配置** 代码示例，我也能为你提供。

这[段视频详细演示了 CH32V003F4P6 的基础 GPIO 操作和硬件连接方法](https://www.youtube.com/watch?v=vplR-9WMz4E)，适合初学者快速上手实操。
