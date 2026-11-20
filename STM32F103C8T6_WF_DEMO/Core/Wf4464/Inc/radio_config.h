#ifndef RADIO_CONFIG_H_
#define RADIO_CONFIG_H_

// 用户自定义参数
// 在这里定义你自己的参数

// 输入数据（生成下面配置时在 WDS 里填写的参数）
/*
// Crys_freq(Hz): 30000000    Crys_tol(ppm): 20    IF_mode: 2    High_perf_Ch_Fil: 1    OSRtune: 0    Ch_Fil_Bw_AFC: 0    ANT_DIV: 0    PM_pattern: 0    
// MOD_type: 2    Rsymb(sps): 10000    Fdev(Hz): 20000    RXBW(Hz): 150000    Manchester: 0    AFC_en: 0    Rsymb_error: 0.0    Chip-Version: 2    
// RF Freq.(MHz): 433.92    API_TC: 29    fhst: 250000    inputBW: 0    BERT: 0    RAW_dout: 0    D_source: 0    Hi_pfm_div: 1    
// 
// # 接收中频（RX IF）为 -468750 Hz
// # 宽带滤波器 4（BW = 82.64 kHz）；窄带滤波器 4（BW = 82.64 kHz）
// 
// 调制指数：4
*/

/*
// 命令：                  RF_POWER_UP
// 说明：              上电命令：唤醒芯片，并选择工作模式与功能。
 * 0x02 命令码；0x01启动选项；0x00时钟源选择，用外部晶振30MHz;
 * 0x01 0xC9 0xC3 0x80 晶振频率，十六进制0x01C9C380=30000000=30MHz
*/
#define RF_POWER_UP 0x02, 0x01, 0x00, 0x01, 0xC9, 0xC3, 0x80

/*
// 命令：                  RF_GPIO_PIN_CFG
// 说明：              配置 GPIO 引脚。
*/
/* 参数顺序：GPIO0, GPIO1, GPIO2, GPIO3, NIRQ, SDO, GEN_CONFIG
 * 每个 GPIO 字节：bit6 = PULL_EN，bit5:0 = MODE
 *   MODE 0x00 = DO NOTHING（保持上电默认）
 *   MODE 0x01 = TRISTATE （高阻，等于把这个脚悬空 —— 开关控制脚悬空就不会动作）
 *   MODE 0x20 = 32 = TX_STATE  -> 0x60 = PULL_EN | TX_STATE
 *   MODE 0x21 = 33 = RX_STATE  -> 0x61 = PULL_EN | RX_STATE
 *
 * Ebyte E10-433MD-SMA 手册 4.2 给的官方自控配置：
 *   WF44XX_GPIO_CONFIG(0, 0, 32|0x40, 33|0x40, 0, 0, 0)
 * 即 GPIO2 = 发射开关、GPIO3 = 接收开关。
 *
 *0x13命令码；
 *0x61=GPIO0,0x40(PULL_EN) + 0x21(33 = RX_STATE) → 芯片进 RX 时输出高，接接收开关
 *0x00=GPIO1,0 = DO_NOTHING，保持默认（直收模式会被改成 RX_DATA）
 *0x60=GPIO2,0x40(PULL_EN) + 0x20(32 = TX_STATE) → 芯片进 TX 时输出高，接发射开关
 *0x00=GPIO3,未用
 *0x00 NIRQ,中断脚驱动强度用默认值
 *0x00 SDO,SPI 数据输出驱动强度用默认值
 *0x00 GEN_CONFIG,通用配置位全 0（不额外输出时钟等）
 */
#define RF_GPIO_PIN_CFG 0x13, 0x61, 0x54, 0x60, 0x00, 0x00, 0x00, 0x00  // GPIO0=RX开关 GPIO2=TX开关
//#define RF_GPIO_PIN_CFG 0x13, 0x00, 0x00, 0x61, 0x60, 0x00, 0x00, 0x00  // 备选：两脚对调（改了还收不到就换这行）
//#define RF_GPIO_PIN_CFG 0x13, 0x61, 0x01, 0x60, 0x01, 0x00, 0x00, 0x00  // 旧 our（收不到，留档）
//#define RF_GPIO_PIN_CFG 0x13, 0x01, 0x01, 0x61, 0x60, 0x00, 0x00, 0x00  // 旧 ebyte

/*
// 设置属性：           RF_GLOBAL_XO_TUNE_2
// 属性个数：     2
// 分组 ID：                 0x00
// 起始 ID：                 0x00
// 默认值：           0x40, 0x00, 
// 属性说明：
//   GLOBAL_XO_TUNE - 配置晶振的内部电容调谐（频率微调）数组。
//   GLOBAL_CLK_CFG - 时钟配置选项。
*/
#define RF_GLOBAL_XO_TUNE_2 0x11, 0x00, 0x02, 0x00, 0x66, 0x00

/*
// 设置属性：           RF_GLOBAL_CONFIG_1
// 属性个数：     1
// 分组 ID：                 0x00
// 起始 ID：                 0x03
// 默认值：           0x20, 
// 属性说明：
//   GLOBAL_CONFIG - 全局配置设置。
*/
#define RF_GLOBAL_CONFIG_1 0x11, 0x00, 0x01, 0x03, 0x20

/*
// 设置属性：           RF_INT_CTL_ENABLE_4
// 属性个数：     4
// 分组 ID：                 0x01
// 起始 ID：                 0x00
// 默认值：           0x04, 0x00, 0x00, 0x04, 
// 属性说明：
//   INT_CTL_ENABLE - 本属性用于总开关三个中断组（芯片、调制解调器、包处理器），使它们能在 NIRQ 引脚上产生硬件中断。
//   INT_CTL_PH_ENABLE - 使能包处理器中断组内的各个中断源，使其能在 NIRQ 引脚上产生硬件中断。
//   INT_CTL_MODEM_ENABLE - 使能调制解调器中断组内的各个中断源，使其能在 NIRQ 引脚上产生硬件中断。
//   INT_CTL_CHIP_ENABLE - 使能芯片中断组内的各个中断源，使其能在 NIRQ 引脚上产生硬件中断。
*/
#define RF_INT_CTL_ENABLE_4 0x11, 0x01, 0x04, 0x00, 0x07, 0x3B, 0x03, 0x00

/*
// 设置属性：           RF_FRR_CTL_A_MODE_4
// 属性个数：     4
// 分组 ID：                 0x02
// 起始 ID：                 0x00
// 默认值：           0x01, 0x02, 0x09, 0x00, 
// 属性说明：
//   FRR_CTL_A_MODE - 快速响应寄存器 A 的配置。
//   FRR_CTL_B_MODE - 快速响应寄存器 B 的配置。
//   FRR_CTL_C_MODE - 快速响应寄存器 C 的配置。
//   FRR_CTL_D_MODE - 快速响应寄存器 D 的配置。
*/
#define RF_FRR_CTL_A_MODE_4 0x11, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_PREAMBLE_TX_LENGTH_9
// 属性个数：     9
// 分组 ID：                 0x10
// 起始 ID：                 0x00
// 默认值：           0x08, 0x14, 0x00, 0x0F, 0x21, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   PREAMBLE_TX_LENGTH - 配置 TX 前导码的长度。
//   PREAMBLE_CONFIG_STD_1 - 标准前导码格式包的接收配置。
//   PREAMBLE_CONFIG_NSTD - 非标准前导码格式包的收发配置。
//   PREAMBLE_CONFIG_STD_2 - 接收标准前导码格式包时的超时时间配置。
//   PREAMBLE_CONFIG - 前导码字段的通用配置位。
//   PREAMBLE_PATTERN_31_24 - 非标准前导码的比特图案配置（逐位定义）。
//   PREAMBLE_PATTERN_23_16 - 非标准前导码的比特图案配置（逐位定义）。
//   PREAMBLE_PATTERN_15_8 - 非标准前导码的比特图案配置（逐位定义）。
//   PREAMBLE_PATTERN_7_0 - 非标准前导码的比特图案配置（逐位定义）。
*/
#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0x08, 0x14, 0x00, 0x0F, 0x31, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_SYNC_CONFIG_5
// 属性个数：     5
// 分组 ID：                 0x11
// 起始 ID：                 0x00
// 默认值：           0x01, 0x2D, 0xD4, 0x2D, 0xD4, 
// 属性说明：
//   SYNC_CONFIG - 同步字（Sync Word）配置位。
//   SYNC_BITS_31_24 - 同步字。
//   SYNC_BITS_23_16 - 同步字。
//   SYNC_BITS_15_8 - 同步字。
//   SYNC_BITS_7_0 - 同步字。
*/
#define RF_SYNC_CONFIG_5 0x11, 0x11, 0x05, 0x00, 0x01, 0xB4, 0x2B, 0x00, 0x00

/*
// 设置属性：           RF_PKT_CRC_CONFIG_7
// 属性个数：     7
// 分组 ID：                 0x12
// 起始 ID：                 0x00
// 默认值：           0x00, 0x01, 0x08, 0xFF, 0xFF, 0x00, 0x00, 
// 属性说明：
//   PKT_CRC_CONFIG - 选择 CRC 多项式与初始种子。
//   PKT_WHT_POLY_15_8 - PN 生成器使用的 16 位多项式值（例如用于数据白化）
//   PKT_WHT_POLY_7_0 - PN 生成器使用的 16 位多项式值（例如用于数据白化）
//   PKT_WHT_SEED_15_8 - PN 生成器使用的 16 位种子值（例如用于数据白化）
//   PKT_WHT_SEED_7_0 - PN 生成器使用的 16 位种子值（例如用于数据白化）
//   PKT_WHT_BIT_NUM - 选择 LFSR（用于产生 PN／数据白化序列）的哪一位作为数据加扰的输出位。
//   PKT_CONFIG1 - 包收发相关的通用配置位。
*/
#define RF_PKT_CRC_CONFIG_7 0x11, 0x12, 0x07, 0x00, 0x04, 0x01, 0x08, 0xFF, 0xFF, 0x20, 0x02

/*
// 设置属性：           RF_PKT_LEN_12
// 属性个数：     12
// 分组 ID：                 0x12
// 起始 ID：                 0x08
// 默认值：           0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   PKT_LEN - 变长包接收的配置位。
//   PKT_LEN_FIELD_SOURCE - 指定哪个字段存放接收到的包长度字节。
//   PKT_LEN_ADJUST - 对收到的包长度值做调整/偏移（以兼容多种总包长定义方式）。
//   PKT_TX_THRESHOLD - TX FIFO 几乎空（almost empty）阈值。
//   PKT_RX_THRESHOLD - RX FIFO 几乎满（almost full）阈值。
//   PKT_FIELD_1_LENGTH_12_8 - Field 1 的 13 位无符号长度值。
//   PKT_FIELD_1_LENGTH_7_0 - Field 1 的 13 位无符号长度值。
//   PKT_FIELD_1_CONFIG - Field 1 的通用数据处理与包配置位。
//   PKT_FIELD_1_CRC_CONFIG - Field 1 范围内的 CRC 控制位配置。
//   PKT_FIELD_2_LENGTH_12_8 - Field 2 的 13 位无符号长度值。
//   PKT_FIELD_2_LENGTH_7_0 - Field 2 的 13 位无符号长度值。
//   PKT_FIELD_2_CONFIG - Field 2 的通用数据处理与包配置位。
*/
#define RF_PKT_LEN_12 0x11, 0x12, 0x0C, 0x08, 0x22, 0x01, 0x00, 0x30, 0x30, 0x00, 0x01, 0x04, 0x80, 0x1F, 0xFF, 0x00

/*
// 设置属性：           RF_PKT_FIELD_2_CRC_CONFIG_12
// 属性个数：     12
// 分组 ID：                 0x12
// 起始 ID：                 0x14
// 默认值：           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   PKT_FIELD_2_CRC_CONFIG - Field 2 范围内的 CRC 控制位配置。
//   PKT_FIELD_3_LENGTH_12_8 - Field 3 的 13 位无符号长度值。
//   PKT_FIELD_3_LENGTH_7_0 - Field 3 的 13 位无符号长度值。
//   PKT_FIELD_3_CONFIG - Field 3 的通用数据处理与包配置位。
//   PKT_FIELD_3_CRC_CONFIG - Field 3 范围内的 CRC 控制位配置。
//   PKT_FIELD_4_LENGTH_12_8 - Field 4 的 13 位无符号长度值。
//   PKT_FIELD_4_LENGTH_7_0 - Field 4 的 13 位无符号长度值。
//   PKT_FIELD_4_CONFIG - Field 4 的通用数据处理与包配置位。
//   PKT_FIELD_4_CRC_CONFIG - Field 4 范围内的 CRC 控制位配置。
//   PKT_FIELD_5_LENGTH_12_8 - Field 5 的 13 位无符号长度值。
//   PKT_FIELD_5_LENGTH_7_0 - Field 5 的 13 位无符号长度值。
//   PKT_FIELD_5_CONFIG - Field 5 的通用数据处理与包配置位。
*/
#define RF_PKT_FIELD_2_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_PKT_FIELD_5_CRC_CONFIG_12
// 属性个数：     12
// 分组 ID：                 0x12
// 起始 ID：                 0x20
// 默认值：           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   PKT_FIELD_5_CRC_CONFIG - Field 5 范围内的 CRC 控制位配置。
//   PKT_RX_FIELD_1_LENGTH_12_8 - RX Field 1 的 13 位无符号长度值。
//   PKT_RX_FIELD_1_LENGTH_7_0 - RX Field 1 的 13 位无符号长度值。
//   PKT_RX_FIELD_1_CONFIG - RX Field 1 的通用数据处理与包配置位。
//   PKT_RX_FIELD_1_CRC_CONFIG - RX Field 1 范围内的 CRC 控制位配置。
//   PKT_RX_FIELD_2_LENGTH_12_8 - RX Field 2 的 13 位无符号长度值。
//   PKT_RX_FIELD_2_LENGTH_7_0 - RX Field 2 的 13 位无符号长度值。
//   PKT_RX_FIELD_2_CONFIG - RX Field 2 的通用数据处理与包配置位。
//   PKT_RX_FIELD_2_CRC_CONFIG - RX Field 2 范围内的 CRC 控制位配置。
//   PKT_RX_FIELD_3_LENGTH_12_8 - RX Field 3 的 13 位无符号长度值。
//   PKT_RX_FIELD_3_LENGTH_7_0 - RX Field 3 的 13 位无符号长度值。
//   PKT_RX_FIELD_3_CONFIG - RX Field 3 的通用数据处理与包配置位。
*/
#define RF_PKT_FIELD_5_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_PKT_RX_FIELD_3_CRC_CONFIG_9
// 属性个数：     9
// 分组 ID：                 0x12
// 起始 ID：                 0x2C
// 默认值：           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   PKT_RX_FIELD_3_CRC_CONFIG - RX Field 3 范围内的 CRC 控制位配置。
//   PKT_RX_FIELD_4_LENGTH_12_8 - RX Field 4 的 13 位无符号长度值。
//   PKT_RX_FIELD_4_LENGTH_7_0 - RX Field 4 的 13 位无符号长度值。
//   PKT_RX_FIELD_4_CONFIG - RX Field 4 的通用数据处理与包配置位。
//   PKT_RX_FIELD_4_CRC_CONFIG - RX Field 4 范围内的 CRC 控制位配置。
//   PKT_RX_FIELD_5_LENGTH_12_8 - RX Field 5 的 13 位无符号长度值。
//   PKT_RX_FIELD_5_LENGTH_7_0 - RX Field 5 的 13 位无符号长度值。
//   PKT_RX_FIELD_5_CONFIG - RX Field 5 的通用数据处理与包配置位。
//   PKT_RX_FIELD_5_CRC_CONFIG - RX Field 5 范围内的 CRC 控制位配置。
*/
#define RF_PKT_RX_FIELD_3_CRC_CONFIG_9 0x11, 0x12, 0x09, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_MODEM_MOD_TYPE_12
// 属性个数：     12
// 分组 ID：                 0x20
// 起始 ID：                 0x00
// 默认值：           0x02, 0x80, 0x07, 0x0F, 0x42, 0x40, 0x01, 0xC9, 0xC3, 0x80, 0x00, 0x06, 
// 属性说明：
//   MODEM_MOD_TYPE - 选择调制方式。在 TX 模式下还同时选择调制数据的来源。
//   MODEM_MAP_CONTROL - 控制发送/接收比特的极性与映射。
//   MODEM_DSM_CTRL - PLL 频率合成器中 Delta-Sigma 调制器（DSM）的杂项控制位。
//   MODEM_DATA_RATE_2 - 用于确定 TX 速率的 24 位无符号数值
//   MODEM_DATA_RATE_1 - 用于确定 TX 速率的 24 位无符号数值
//   MODEM_DATA_RATE_0 - 用于确定 TX 速率的 24 位无符号数值
//   MODEM_TX_NCO_MODE_3 - TX 高斯滤波器的过采样比，以及 26 位无符号 TX 数控振荡器（NCO）模数的最高字节（bit25:24）。
//   MODEM_TX_NCO_MODE_2 - 26 位无符号 TX 数控振荡器（NCO）模数的第 2 字节（bit23:16）。
//   MODEM_TX_NCO_MODE_1 - 26 位无符号 TX 数控振荡器（NCO）模数的第 1 字节（bit15:8）。
//   MODEM_TX_NCO_MODE_0 - 26 位无符号 TX 数控振荡器（NCO）模数的第 0 字节（bit7:0）。
//   MODEM_FREQ_DEV_2 - 17 位无符号 TX 频偏字。
//   MODEM_FREQ_DEV_1 - 17 位无符号 TX 频偏字。
 * 数据率：0x0186A0 = 100000
*/
#define RF_MODEM_MOD_TYPE_12 0x11, 0x20, 0x0C, 0x00, 0x02, 0x00, 0x07, 0x01, 0x86, 0xA0, 0x01, 0xC9, 0xC3, 0x80, 0x00, 0x05

/*
// 设置属性：           RF_MODEM_FREQ_DEV_0_1
// 属性个数：     1
// 分组 ID：                 0x20
// 起始 ID：                 0x0C
// 默认值：           0xD3, 
// 属性说明：
//   MODEM_FREQ_DEV_0 - 17 位无符号 TX 频偏字。
*/
#define RF_MODEM_FREQ_DEV_0_1 0x11, 0x20, 0x01, 0x0C, 0x76

/*
// 设置属性：           RF_MODEM_TX_RAMP_DELAY_8
// 属性个数：     8
// 分组 ID：                 0x20
// 起始 ID：                 0x18
// 默认值：           0x01, 0x00, 0x08, 0x03, 0xC0, 0x00, 0x10, 0x20, 
// 属性说明：
//   MODEM_TX_RAMP_DELAY - TX 功率下降（ramp-down）延时设置。
//   MODEM_MDM_CTRL - MDM 控制。
//   MODEM_IF_CONTROL - 选择 RX 调制解调器的工作模式：固定中频、按比例中频或零中频。
//   MODEM_IF_FREQ_2 - 中频频率设置（18 位有符号数）。
//   MODEM_IF_FREQ_1 - 中频频率设置（18 位有符号数）。
//   MODEM_IF_FREQ_0 - 中频频率设置（18 位有符号数）。
//   MODEM_DECIMATION_CFG1 - 指定级联积分梳状（CIC）滤波器的三个抽取比。
//   MODEM_DECIMATION_CFG0 - 指定级联积分梳状（CIC）滤波器的杂项参数与抽取比。
*/
#define RF_MODEM_TX_RAMP_DELAY_8 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x20, 0x20

/*
// 设置属性：           RF_MODEM_BCR_OSR_1_9
// 属性个数：     9
// 分组 ID：                 0x20
// 起始 ID：                 0x22
// 默认值：           0x00, 0x4B, 0x06, 0xD3, 0xA0, 0x06, 0xD3, 0x02, 0xC0, 
// 属性说明：
//   MODEM_BCR_OSR_1 - RX 位时钟恢复（BCR）/判决器的过采样率（12 位无符号数）。
//   MODEM_BCR_OSR_0 - RX 位时钟恢复（BCR）/判决器的过采样率（12 位无符号数）。
//   MODEM_BCR_NCO_OFFSET_2 - RX BCR 数控振荡器（NCO）的偏置值（22 位无符号数）。
//   MODEM_BCR_NCO_OFFSET_1 - RX BCR 数控振荡器（NCO）的偏置值（22 位无符号数）。
//   MODEM_BCR_NCO_OFFSET_0 - RX BCR 数控振荡器（NCO）的偏置值（22 位无符号数）。
//   MODEM_BCR_GAIN_1 - RX BCR 环路的 11 位无符号增益值。
//   MODEM_BCR_GAIN_0 - RX BCR 环路的 11 位无符号增益值。
//   MODEM_BCR_GEAR - RX BCR 环路档位（gear）控制。
//   MODEM_BCR_MISC1 - RX BCR 环路的杂项控制位。
*/
#define RF_MODEM_BCR_OSR_1_9 0x11, 0x20, 0x09, 0x22, 0x01, 0x77, 0x01, 0x5D, 0x86, 0x00, 0xAF, 0x02, 0xC2

/*
// 设置属性：           RF_MODEM_AFC_GEAR_7
// 属性个数：     7
// 分组 ID：                 0x20
// 起始 ID：                 0x2C
// 默认值：           0x00, 0x23, 0x83, 0x69, 0x00, 0x40, 0xA0, 
// 属性说明：
//   MODEM_AFC_GEAR - RX AFC 环路档位（gear）控制。
//   MODEM_AFC_WAIT - RX AFC 环路等待时间控制。
//   MODEM_AFC_GAIN_1 - 设置基于 PLL 的 AFC 捕获环路增益，并提供 AFC 相关的杂项控制位。
//   MODEM_AFC_GAIN_0 - 设置基于 PLL 的 AFC 捕获环路增益，并提供 AFC 相关的杂项控制位。
//   MODEM_AFC_LIMITER_1 - 设置 AFC 限幅值。
//   MODEM_AFC_LIMITER_0 - 设置 AFC 限幅值。
//   MODEM_AFC_MISC - AFC 的杂项控制位。
*/
#define RF_MODEM_AFC_GEAR_7 0x11, 0x20, 0x07, 0x2C, 0x04, 0x23, 0x80, 0x1D, 0x10, 0x04, 0x80

/*
// 设置属性：           RF_MODEM_AGC_CONTROL_1
// 属性个数：     1
// 分组 ID：                 0x20
// 起始 ID：                 0x35
// 默认值：           0xE0, 
// 属性说明：
//   MODEM_AGC_CONTROL - RX 链路中自动增益控制（AGC）功能的杂项控制位。
*/
#define RF_MODEM_AGC_CONTROL_1 0x11, 0x20, 0x01, 0x35, 0xE0

/*
// 设置属性：           RF_MODEM_AGC_WINDOW_SIZE_9
// 属性个数：     9
// 分组 ID：                 0x20
// 起始 ID：                 0x38
// 默认值：           0x11, 0x10, 0x10, 0x0B, 0x1C, 0x40, 0x00, 0x00, 0x2B, 
// 属性说明：
//   MODEM_AGC_WINDOW_SIZE - 指定 AGC 算法中测量窗口与稳定窗口的长度。
//   MODEM_AGC_RFPD_DECAY - 设置射频峰值检波器的衰减时间。
//   MODEM_AGC_IFPD_DECAY - 设置中频峰值检波器的衰减时间。
//   MODEM_FSK4_GAIN1 - 指定 4(G)FSK 码间干扰（ISI）抑制中次支路的增益系数。
//   MODEM_FSK4_GAIN0 - 指定 4(G)FSK 码间干扰（ISI）抑制中主支路的增益系数。
//   MODEM_FSK4_TH1 - 4(G)FSK 判决器的 16 位门限。
//   MODEM_FSK4_TH0 - 4(G)FSK 判决器的 16 位门限。
//   MODEM_FSK4_MAP - 4(G)FSK 符号映射编码。
//   MODEM_OOK_PDTC - 配置 OOK 峰值检波器的建立（attack）与衰减时间。
*/
#define RF_MODEM_AGC_WINDOW_SIZE_9 0x11, 0x20, 0x09, 0x38, 0x11, 0x52, 0x52, 0x00, 0x02, 0xFF, 0xFF, 0x00, 0x2A

/*
// 设置属性：           RF_MODEM_OOK_CNT1_9
// 属性个数：     9
// 分组 ID：                 0x20
// 起始 ID：                 0x42
// 默认值：           0xA4, 0x03, 0x56, 0x02, 0x00, 0xA3, 0x02, 0x80, 0xFF, 
// 属性说明：
//   MODEM_OOK_CNT1 - OOK 控制。
//   MODEM_OOK_MISC - 选择 OOK 信号解调所用的检波器；使用异步解调器解调 (G)FSK 时同样适用。
//   MODEM_RAW_SEARCH - 定义并控制滑动平均检波器与最大/最小检波器的搜索周期长度。
//   MODEM_RAW_CONTROL - 定义原始／非标准模式的增益与使能控制。
//   MODEM_RAW_EYE_1 - 睁眼（eye-open）检测器的 11 位门限。
//   MODEM_RAW_EYE_0 - 睁眼（eye-open）检测器的 11 位门限。
//   MODEM_ANT_DIV_MODE - 天线分集模式设置。
//   MODEM_ANT_DIV_CONTROL - 天线分集算法的控制参数。
//   MODEM_RSSI_THRESH - 配置 RSSI 门限。
*/
#define RF_MODEM_OOK_CNT1_9 0x11, 0x20, 0x09, 0x42, 0xA4, 0x22, 0xD6, 0x83, 0x01, 0x55, 0x01, 0x00, 0xFF

/*
// 设置属性：           RF_MODEM_RSSI_CONTROL_1
// 属性个数：     1
// 分组 ID：                 0x20
// 起始 ID：                 0x4C
// 默认值：           0x01, 
// 属性说明：
//   MODEM_RSSI_CONTROL - 控制上报 RSSI 值时的平均方式与锁存时间。
*/
#define RF_MODEM_RSSI_CONTROL_1 0x11, 0x20, 0x01, 0x4C, 0x01

/*
// 设置属性：           RF_MODEM_RSSI_COMP_1
// 属性个数：     1
// 分组 ID：                 0x20
// 起始 ID：                 0x4E
// 默认值：           0x32, 
// 属性说明：
//   MODEM_RSSI_COMP - RSSI 补偿值。
*/
#define RF_MODEM_RSSI_COMP_1 0x11, 0x20, 0x01, 0x4E, 0x40

/*
// 设置属性：           RF_MODEM_CLKGEN_BAND_1
// 属性个数：     1
// 分组 ID：                 0x20
// 起始 ID：                 0x51
// 默认值：           0x08, 
// 属性说明：
//   MODEM_CLKGEN_BAND - 按频段选择 PLL 频率合成器的输出分频比。
*/
#define RF_MODEM_CLKGEN_BAND_1 0x11, 0x20, 0x01, 0x51, 0x0A

/*
// 设置属性：           RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12
// 属性个数：     12
// 分组 ID：                 0x21
// 起始 ID：                 0x00
// 默认值：           0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01, 
// 属性说明：
//   MODEM_CHFLT_RX1_CHFLT_COE13_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE12_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE11_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE10_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE9_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE8_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE7_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE6_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE5_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE4_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE3_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE2_7_0 - 第一组 RX 滤波器系数。
*/
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12 0x11, 0x21, 0x0C, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08

/*
// 设置属性：           RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12
// 属性个数：     12
// 分组 ID：                 0x21
// 起始 ID：                 0x0C
// 默认值：           0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F, 0xFF, 0xC4, 0x30, 0x7F, 0xF5, 0xB5, 
// 属性说明：
//   MODEM_CHFLT_RX1_CHFLT_COE1_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COE0_7_0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COEM0 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COEM1 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COEM2 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX1_CHFLT_COEM3 - 第一组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE13_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE12_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE11_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE10_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE9_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE8_7_0 - 第二组 RX 滤波器系数。
*/
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12 0x11, 0x21, 0x0C, 0x0C, 0x07, 0x03, 0x15, 0xFC, 0x0F, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE

/*
// 设置属性：           RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12
// 属性个数：     12
// 分组 ID：                 0x21
// 起始 ID：                 0x18
// 默认值：           0xB8, 0xDE, 0x05, 0x17, 0x16, 0x0C, 0x03, 0x00, 0x15, 0xFF, 0x00, 0x00, 
// 属性说明：
//   MODEM_CHFLT_RX2_CHFLT_COE7_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE6_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE5_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE4_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE3_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE2_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE1_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COE0_7_0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COEM0 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COEM1 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COEM2 - 第二组 RX 滤波器系数。
//   MODEM_CHFLT_RX2_CHFLT_COEM3 - 第二组 RX 滤波器系数。
*/
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12 0x11, 0x21, 0x0C, 0x18, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08, 0x07, 0x03, 0x15, 0xFC, 0x0F, 0x00

/*
// 设置属性：           RF_PA_MODE_4
// 属性个数：     4
// 分组 ID：                 0x22
// 起始 ID：                 0x00
// 默认值：           0x08, 0x7F, 0x00, 0x5D, 
// 属性说明：
//   PA_MODE - 选择 PA 工作模式，同时选择 PA 功率调节的分辨率（即步进大小）。
//   PA_PWR_LVL - PA 输出功率等级配置。
//   PA_BIAS_CLKDUTY - PA 偏置配置，以及 TX 时钟源的占空比配置。
//   PA_TC - PA 功率斜坡（ramping）参数配置。
*/
#define RF_PA_MODE_4 0x11, 0x22, 0x04, 0x00, 0x08, 0x7F, 0x00, 0x1D

/*
// 设置属性：           RF_SYNTH_PFDCP_CPFF_7
// 属性个数：     7
// 分组 ID：                 0x23
// 起始 ID：                 0x00
// 默认值：           0x2C, 0x0E, 0x0B, 0x04, 0x0C, 0x73, 0x03, 
// 属性说明：
//   SYNTH_PFDCP_CPFF - 前馈电荷泵电流选择。
//   SYNTH_PFDCP_CPINT - 积分电荷泵电流选择。
//   SYNTH_VCO_KV - 积分支路与前馈支路上 VCO 调谐变容管的增益缩放系数（Kv）。
//   SYNTH_LPFILT3 - 环路滤波器前馈路径中电阻 R2 的取值。
//   SYNTH_LPFILT2 - 环路滤波器前馈路径中电容 C2 的取值。
//   SYNTH_LPFILT1 - 环路滤波器前馈路径中电容 C1 与 C3 的取值。
//   SYNTH_LPFILT0 - 前馈环路滤波器中运放的偏置电流。
*/
#define RF_SYNTH_PFDCP_CPFF_7 0x11, 0x23, 0x07, 0x00, 0x2C, 0x0E, 0x0B, 0x04, 0x0C, 0x73, 0x03

/*
// 设置属性：           RF_MATCH_VALUE_1_12
// 属性个数：     12
// 分组 ID：                 0x30
// 起始 ID：                 0x00
// 默认值：           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 属性说明：
//   MATCH_VALUE_1 - 比较值：与“掩码 1 按位与收到的匹配字节 1”的结果作比较。
//   MATCH_MASK_1 - 掩码值：与收到的匹配字节 1 按位与。
//   MATCH_CTRL_1 - 使能包匹配（Packet Match）功能，并配置匹配字节 1。
//   MATCH_VALUE_2 - 比较值：与“掩码 2 按位与收到的匹配字节 2”的结果作比较。
//   MATCH_MASK_2 - 掩码值：与收到的匹配字节 2 按位与。
//   MATCH_CTRL_2 - 匹配字节 2 的配置。
//   MATCH_VALUE_3 - 比较值：与“掩码 3 按位与收到的匹配字节 3”的结果作比较。
//   MATCH_MASK_3 - 掩码值：与收到的匹配字节 3 按位与。
//   MATCH_CTRL_3 - 匹配字节 3 的配置。
//   MATCH_VALUE_4 - 比较值：与“掩码 4 按位与收到的匹配字节 4”的结果作比较。
//   MATCH_MASK_4 - 掩码值：与收到的匹配字节 4 按位与。
//   MATCH_CTRL_4 - 匹配字节 4 的配置。
*/
#define RF_MATCH_VALUE_1_12 0x11, 0x30, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// 设置属性：           RF_FREQ_CONTROL_INTE_8
// 属性个数：     8
// 分组 ID：                 0x40
// 起始 ID：                 0x00
// 默认值：           0x3C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x20, 0xFF, 
// 属性说明：
//   FREQ_CONTROL_INTE - 小数分频（Frac-N）PLL 频率合成器的整数分频值。
//   FREQ_CONTROL_FRAC_2 - 小数分频（Frac-N）PLL 的小数部分值。
//   FREQ_CONTROL_FRAC_1 - 小数分频（Frac-N）PLL 的小数部分值。
//   FREQ_CONTROL_FRAC_0 - 小数分频（Frac-N）PLL 的小数部分值。
//   FREQ_CONTROL_CHANNEL_STEP_SIZE_1 - EZ 频率规划（EZ Frequency Programming）的信道步进值。
//   FREQ_CONTROL_CHANNEL_STEP_SIZE_0 - EZ 频率规划（EZ Frequency Programming）的信道步进值。
//   FREQ_CONTROL_W_SIZE - 设置校准统计 VCO 频率时窗口的选通周期（以晶振参考时钟周期数计）。
//   FREQ_CONTROL_VCOCNT_RX_ADJ - 调整 RX 模式下 VCO 校准的目标计数值。
 * 0x38 = 56,PLL整数分频比（7位）
*0x0ED916=973073
*(56 + 1 + 973073/2^19)*7.5MHz = 433.92MHz
**/
#define RF_FREQ_CONTROL_INTE_8 0x11, 0x40, 0x08, 0x00, 0x38, 0x0E, 0xD9, 0x16, 0x44, 0x44, 0x20, 0xFE


// 以下为自动生成的代码！ 
// 请不要修改本行以下的内容！
// --------------------------------------------


/* ==================================================================
 * 供应商 DR433_92MHz_2FSK2_R2K_D20K 参数覆盖段
 *   2 kbps / ±20 kHz 频偏 / 76.31 kHz 接收带宽 / 433.920 MHz
 *
 * 只覆盖 MODEM(0x20) 与 MODEM_CHFLT(0x21) 两组；GPIO、前导、同步、包格式
 * 全部不动 —— RX_DATA 仍在 GPIO1（运行时由 wf4463_gpio_rawrx() 设成 0x54），
 * 收发开关仍由 GPIO0(RX_STATE)/GPIO2(TX_STATE) 控制。
 *
 * MOD_TYPE 写 0x02（包模式）而非供应商的 0x0A（直发）：直发会把 GPIO0
 * 征用为调制数据输入，与本板的收发开关冲突。
 * ================================================================== */

#define RF_OVR_MODEM_0 0x11, 0x20, 0x0C, 0x00, 0x02, 0x00, 0x07, 0x00, 0x4E, 0x20, 0x01, 0xC9, 0xC3, 0x80, 0x00, 0x05
#define RF_OVR_MODEM_1 0x11, 0x20, 0x01, 0x0C, 0x76
#define RF_OVR_MODEM_2 0x11, 0x20, 0x0C, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x20, 0x10, 0x0C, 0xE8, 0x04, 0xE2
#define RF_OVR_MODEM_3 0x11, 0x20, 0x0C, 0x24, 0x00, 0x68, 0xDC, 0x00, 0x34, 0x00, 0xC2, 0x00, 0x54, 0x23, 0x80, 0xE4
#define RF_OVR_MODEM_4 0x11, 0x20, 0x03, 0x30, 0x01, 0xE2, 0x80
#define RF_OVR_MODEM_5 0x11, 0x20, 0x01, 0x35, 0xE0
#define RF_OVR_MODEM_6 0x11, 0x20, 0x0C, 0x38, 0x11, 0xFF, 0xFF, 0x80, 0x02, 0xFF, 0xFF, 0x00, 0x2B, 0x0C, 0x84, 0x23
#define RF_OVR_MODEM_7 0x11, 0x20, 0x05, 0x45, 0x8F, 0x01, 0xFF, 0x01, 0x00
#define RF_OVR_MODEM_8 0x11, 0x20, 0x01, 0x4B, 0x06
#define RF_OVR_MODEM_9 0x11, 0x20, 0x02, 0x4D, 0x18, 0x40
#define RF_OVR_MODEM_10 0x11, 0x20, 0x02, 0x50, 0x94, 0x0A
#define RF_OVR_MODEM_11 0x11, 0x20, 0x02, 0x54, 0x07, 0x07
#define RF_OVR_MODEM_12 0x11, 0x20, 0x01, 0x57, 0x00
#define RF_OVR_MODEM_13 0x11, 0x20, 0x05, 0x5B, 0x40, 0x04, 0x10, 0x78, 0x20

#define RF_OVR_CHFLT_0 0x11, 0x21, 0x0C, 0x00, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01
#define RF_OVR_CHFLT_1 0x11, 0x21, 0x0C, 0x0C, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9
#define RF_OVR_CHFLT_2 0x11, 0x21, 0x0C, 0x18, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F
#ifndef FIRMWARE_LOAD_COMPILE
#define RADIO_CONFIGURATION_DATA_ARRAY { \
        0x07, RF_POWER_UP, \
        0x08, RF_GPIO_PIN_CFG, \
        0x06, RF_GLOBAL_XO_TUNE_2, \
        0x05, RF_GLOBAL_CONFIG_1, \
        0x08, RF_INT_CTL_ENABLE_4, \
        0x08, RF_FRR_CTL_A_MODE_4, \
        0x0D, RF_PREAMBLE_TX_LENGTH_9, \
        0x09, RF_SYNC_CONFIG_5, \
        0x0B, RF_PKT_CRC_CONFIG_7, \
        0x10, RF_PKT_LEN_12, \
        0x10, RF_PKT_FIELD_2_CRC_CONFIG_12, \
        0x10, RF_PKT_FIELD_5_CRC_CONFIG_12, \
        0x0D, RF_PKT_RX_FIELD_3_CRC_CONFIG_9, \
        0x10, RF_MODEM_MOD_TYPE_12, \
        0x05, RF_MODEM_FREQ_DEV_0_1, \
        0x0C, RF_MODEM_TX_RAMP_DELAY_8, \
        0x0D, RF_MODEM_BCR_OSR_1_9, \
        0x0B, RF_MODEM_AFC_GEAR_7, \
        0x05, RF_MODEM_AGC_CONTROL_1, \
        0x0D, RF_MODEM_AGC_WINDOW_SIZE_9, \
        0x0D, RF_MODEM_OOK_CNT1_9, \
        0x05, RF_MODEM_RSSI_CONTROL_1, \
        0x05, RF_MODEM_RSSI_COMP_1, \
        0x05, RF_MODEM_CLKGEN_BAND_1, \
        0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12, \
        0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12, \
        0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12, \
        0x08, RF_PA_MODE_4, \
        0x0B, RF_SYNTH_PFDCP_CPFF_7, \
        0x10, RF_MATCH_VALUE_1_12, \
        0x0C, RF_FREQ_CONTROL_INTE_8, \
        0x10, RF_OVR_MODEM_0, \
        0x05, RF_OVR_MODEM_1, \
        0x10, RF_OVR_MODEM_2, \
        0x10, RF_OVR_MODEM_3, \
        0x07, RF_OVR_MODEM_4, \
        0x05, RF_OVR_MODEM_5, \
        0x10, RF_OVR_MODEM_6, \
        0x09, RF_OVR_MODEM_7, \
        0x05, RF_OVR_MODEM_8, \
        0x06, RF_OVR_MODEM_9, \
        0x06, RF_OVR_MODEM_10, \
        0x06, RF_OVR_MODEM_11, \
        0x05, RF_OVR_MODEM_12, \
        0x09, RF_OVR_MODEM_13, \
        0x10, RF_OVR_CHFLT_0, \
        0x10, RF_OVR_CHFLT_1, \
        0x10, RF_OVR_CHFLT_2, \
        0x00 \
 }
#else
#define RADIO_CONFIGURATION_DATA_ARRAY { 0 }
#endif

// 配置参数的默认值
#define RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ_DEFAULT                     30000000L
#define RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER_DEFAULT                    0x00
#define RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH_DEFAULT               0x10
#define RADIO_CONFIGURATION_DATA_RADIO_STATE_AFTER_POWER_UP_DEFAULT        0x01
#define RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET_DEFAULT       0x1000

#define RADIO_CONFIGURATION_DATA_RADIO_PATCH_INCLUDED                      0x00
#define RADIO_CONFIGURATION_DATA_RADIO_PATCH_SIZE                          0x00
#define RADIO_CONFIGURATION_DATA_RADIO_PATCH                               {  }

#ifndef RADIO_CONFIGURATION_DATA_ARRAY
#error "This property must be defined!"
#endif

#ifndef RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ
#define RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ          RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ_DEFAULT 
#endif

#ifndef RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER
#define RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER         RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER_DEFAULT 
#endif

#ifndef RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH
#define RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH    RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH_DEFAULT 
#endif

#ifndef RADIO_CONFIGURATION_DATA_RADIO_STATE_AFTER_POWER_UP
#define RADIO_CONFIGURATION_DATA_RADIO_STATE_AFTER_POWER_UP   RADIO_CONFIGURATION_DATA_RADIO_STATE_AFTER_POWER_UP_DEFAULT 
#endif

#ifndef RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET
#define RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET  RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET_DEFAULT 
#endif

#define RADIO_CONFIGURATION_DATA { \
                            Radio_Configuration_Data_Array,                            \
                            RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER,                   \
                            RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH,              \
                            RADIO_CONFIGURATION_DATA_RADIO_STATE_AFTER_POWER_UP,       \
                            RADIO_CONFIGURATION_DATA_RADIO_DELAY_CNT_AFTER_RESET       \
                            }

#endif /* RADIO_CONFIG_H_ */
