#ifndef RADIO_CONFIG_H_
#define RADIO_CONFIG_H_


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
 *0x54=GPIO1,0x40(PULL_EN) + 0x14(20 = RX_DATA)比特流输出（直收模式的 RX_DATA）
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
// RF_GLOBAL_XO_TUNE_2 —— 一条 6 字节 SPI 命令，逐字节对应：
//
//   0x11  0x00  0x02  0x00  0x66  0x00
//    |     |     |     |     |     +-- CLK_CFG=0x00   分频时钟关、32k 时钟关
//    |     |     |     |     +-------- XO_TUNE=0x66   晶振调谐值（默认 0x40）
//    |     |     |     +-------------- 从组内 0 号属性开始写
//    |     |     +-------------------- 一次写 2 个属性
//    |     +-------------------------- 属性组 GLOBAL(0x00)
//    +-------------------------------- 命令码 SET_PROPERTY
//
// 0x66 = WDS 按 30 MHz 晶振算出的调谐值，换了晶振要重算，别手改（影响频偏/灵敏度）。
// 名字末尾 _2 = 写 2 个属性；长度字节 0x06 在数组里（0x06, RF_GLOBAL_XO_TUNE_2），不在宏里。0X00~0X7F
*/
#define RF_GLOBAL_XO_TUNE_2 0x11, 0x00, 0x02, 0x00, 0x66, 0x00

/*
// RF_GLOBAL_CONFIG_1 —— 写 GLOBAL 组 3~3 号属性（共 1 个）
//   0x11 0x00 0x01 0x03  ← 命令码 / 属性组 / 个数 / 起始号
//    3 GLOBAL_CONFIG                      = 0x20
*/
#define RF_GLOBAL_CONFIG_1 0x11, 0x00, 0x01, 0x03, 0x20

/*
// RF_INT_CTL_ENABLE_4 —— 写 INT_CTL 组 0~3 号属性（共 4 个）
//   0x11 0x01 0x04 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 INT_CTL_ENABLE                     = 0x07
//    1 INT_CTL_PH_ENABLE                  = 0x3B
//    2 INT_CTL_MODEM_ENABLE               = 0x03
//    3 INT_CTL_CHIP_ENABLE                = 0x00
// 注：CHIP_ENABLE=0x00 表示芯片组中断当前全关
*/
#define RF_INT_CTL_ENABLE_4 0x11, 0x01, 0x04, 0x00, 0x07, 0x3B, 0x03, 0x00

/*
// RF_FRR_CTL_A_MODE_4 —— 写 FRR_CTL 组 0~3 号属性（共 4 个）
//   0x11 0x02 0x04 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 FRR_CTL_A_MODE                     = 0x00
//    1 FRR_CTL_B_MODE                     = 0x00
//    2 FRR_CTL_C_MODE                     = 0x00
//    3 FRR_CTL_D_MODE                     = 0x00
*/
#define RF_FRR_CTL_A_MODE_4 0x11, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// RF_PREAMBLE_TX_LENGTH_9 —— 写 PREAMBLE 组 0~8 号属性（共 9 个）
//   0x11 0x10 0x09 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 PREAMBLE_TX_LENGTH           = 0x08    5 PREAMBLE_PATTERN_31_24       = 0x00
//    1 PREAMBLE_CONFIG_STD_1        = 0x14    6 PREAMBLE_PATTERN_23_16       = 0x00
//    2 PREAMBLE_CONFIG_NSTD         = 0x00    7 PREAMBLE_PATTERN_15_8        = 0x00
//    3 PREAMBLE_CONFIG_STD_2        = 0x0F    8 PREAMBLE_PATTERN_7_0         = 0x00
//    4 PREAMBLE_CONFIG              = 0x31
*/
#define RF_PREAMBLE_TX_LENGTH_9 0x11, 0x10, 0x09, 0x00, 0x08, 0x14, 0x00, 0x0F, 0x31, 0x00, 0x00, 0x00, 0x00

/*
// RF_SYNC_CONFIG_5 —— 写 SYNC 组 0~4 号属性（共 5 个）
//   0x11 0x11 0x05 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 SYNC_CONFIG                        = 0x01
//    1 SYNC_BITS_31_24                    = 0xB4
//    2 SYNC_BITS_23_16                    = 0x2B
//    3 SYNC_BITS_15_8                     = 0x00
//    4 SYNC_BITS_7_0                      = 0x00
*/
#define RF_SYNC_CONFIG_5 0x11, 0x11, 0x05, 0x00, 0x01, 0xB4, 0x2B, 0x00, 0x00

/*
// RF_PKT_CRC_CONFIG_7 —— 写 PKT 组 0~6 号属性（共 7 个）
//   0x11 0x12 0x07 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 PKT_CRC_CONFIG               = 0x04    4 PKT_WHT_SEED_7_0             = 0xFF
//    1 PKT_WHT_POLY_15_8            = 0x01    5 PKT_WHT_BIT_NUM              = 0x20
//    2 PKT_WHT_POLY_7_0             = 0x08    6 PKT_CONFIG1                  = 0x02
//    3 PKT_WHT_SEED_15_8            = 0xFF
*/
#define RF_PKT_CRC_CONFIG_7 0x11, 0x12, 0x07, 0x00, 0x04, 0x01, 0x08, 0xFF, 0xFF, 0x20, 0x02

/*
// RF_PKT_LEN_12 —— 写 PKT 组 8~19 号属性（共 12 个）
//   0x11 0x12 0x0C 0x08  ← 命令码 / 属性组 / 个数 / 起始号
//    8 PKT_LEN                      = 0x22   14 PKT_FIELD_1_LENGTH_7_0       = 0x01
//    9 PKT_LEN_FIELD_SOURCE         = 0x01   15 PKT_FIELD_1_CONFIG           = 0x04
//   10 PKT_LEN_ADJUST               = 0x00   16 PKT_FIELD_1_CRC_CONFIG       = 0x80
//   11 PKT_TX_THRESHOLD             = 0x30   17 PKT_FIELD_2_LENGTH_12_8      = 0x1F
//   12 PKT_RX_THRESHOLD             = 0x30   18 PKT_FIELD_2_LENGTH_7_0       = 0xFF
//   13 PKT_FIELD_1_LENGTH_12_8      = 0x00   19 PKT_FIELD_2_CONFIG           = 0x00
// 注：第 17、18 号合为 PKT_FIELD_2_LENGTH=0x1FFF，是变长包上限
*/
#define RF_PKT_LEN_12 0x11, 0x12, 0x0C, 0x08, 0x22, 0x01, 0x00, 0x30, 0x30, 0x00, 0x01, 0x04, 0x80, 0x1F, 0xFF, 0x00

/*
// RF_PKT_FIELD_2_CRC_CONFIG_12 —— 写 PKT 组 20~31 号属性（共 12 个）
//   0x11 0x12 0x0C 0x14  ← 命令码 / 属性组 / 个数 / 起始号
//   20 PKT_FIELD_2_CRC_CONFIG       = 0x00   26 PKT_FIELD_4_LENGTH_7_0       = 0x00
//   21 PKT_FIELD_3_LENGTH_12_8      = 0x00   27 PKT_FIELD_4_CONFIG           = 0x00
//   22 PKT_FIELD_3_LENGTH_7_0       = 0x00   28 PKT_FIELD_4_CRC_CONFIG       = 0x00
//   23 PKT_FIELD_3_CONFIG           = 0x00   29 PKT_FIELD_5_LENGTH_12_8      = 0x00
//   24 PKT_FIELD_3_CRC_CONFIG       = 0x00   30 PKT_FIELD_5_LENGTH_7_0       = 0x00
//   25 PKT_FIELD_4_LENGTH_12_8      = 0x00   31 PKT_FIELD_5_CONFIG           = 0x00
*/
#define RF_PKT_FIELD_2_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// RF_PKT_FIELD_5_CRC_CONFIG_12 —— 写 PKT 组 32~43 号属性（共 12 个）
//   0x11 0x12 0x0C 0x20  ← 命令码 / 属性组 / 个数 / 起始号
//   32 PKT_FIELD_5_CRC_CONFIG       = 0x00   38 PKT_RX_FIELD_2_LENGTH_7_0    = 0x00
//   33 PKT_RX_FIELD_1_LENGTH_12_8   = 0x00   39 PKT_RX_FIELD_2_CONFIG        = 0x00
//   34 PKT_RX_FIELD_1_LENGTH_7_0    = 0x00   40 PKT_RX_FIELD_2_CRC_CONFIG    = 0x00
//   35 PKT_RX_FIELD_1_CONFIG        = 0x00   41 PKT_RX_FIELD_3_LENGTH_12_8   = 0x00
//   36 PKT_RX_FIELD_1_CRC_CONFIG    = 0x00   42 PKT_RX_FIELD_3_LENGTH_7_0    = 0x00
//   37 PKT_RX_FIELD_2_LENGTH_12_8   = 0x00   43 PKT_RX_FIELD_3_CONFIG        = 0x00
*/
#define RF_PKT_FIELD_5_CRC_CONFIG_12 0x11, 0x12, 0x0C, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/*
// RF_PKT_RX_FIELD_3_CRC_CONFIG_9 —— 写 PKT 组 44~52 号属性（共 9 个）
//   0x11 0x12 0x09 0x2C  ← 命令码 / 属性组 / 个数 / 起始号
//   44 PKT_RX_FIELD_3_CRC_CONFIG    = 0x00   49 PKT_RX_FIELD_5_LENGTH_12_8   = 0x00
//   45 PKT_RX_FIELD_4_LENGTH_12_8   = 0x00   50 PKT_RX_FIELD_5_LENGTH_7_0    = 0x00
//   46 PKT_RX_FIELD_4_LENGTH_7_0    = 0x00   51 PKT_RX_FIELD_5_CONFIG        = 0x00
//   47 PKT_RX_FIELD_4_CONFIG        = 0x00   52 PKT_RX_FIELD_5_CRC_CONFIG    = 0x00
//   48 PKT_RX_FIELD_4_CRC_CONFIG    = 0x00
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
// RF_MODEM_FREQ_DEV_0_1 —— 写 MODEM 组 12~12 号属性（共 1 个）
//   0x11 0x20 0x01 0x0C  ← 命令码 / 属性组 / 个数 / 起始号
//   12 MODEM_FREQ_DEV_0                   = 0x76
*/
#define RF_MODEM_FREQ_DEV_0_1 0x11, 0x20, 0x01, 0x0C, 0x76

/*
// RF_MODEM_TX_RAMP_DELAY_8 —— 写 MODEM 组 24~31 号属性（共 8 个）
//   0x11 0x20 0x08 0x18  ← 命令码 / 属性组 / 个数 / 起始号
//   24 MODEM_TX_RAMP_DELAY          = 0x01   28 MODEM_IF_FREQ_1              = 0x80
//   25 MODEM_MDM_CTRL               = 0x80   29 MODEM_IF_FREQ_0              = 0x00
//   26 MODEM_IF_CONTROL             = 0x08   30 MODEM_DECIMATION_CFG1        = 0x20
//   27 MODEM_IF_FREQ_2              = 0x03   31 MODEM_DECIMATION_CFG0        = 0x20
*/
#define RF_MODEM_TX_RAMP_DELAY_8 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x20, 0x20

/*
// RF_MODEM_BCR_OSR_1_9 —— 写 MODEM 组 34~42 号属性（共 9 个）
//   0x11 0x20 0x09 0x22  ← 命令码 / 属性组 / 个数 / 起始号
//   34 MODEM_BCR_OSR_1              = 0x01   39 MODEM_BCR_GAIN_1             = 0x00
//   35 MODEM_BCR_OSR_0              = 0x77   40 MODEM_BCR_GAIN_0             = 0xAF
//   36 MODEM_BCR_NCO_OFFSET_2       = 0x01   41 MODEM_BCR_GEAR               = 0x02
//   37 MODEM_BCR_NCO_OFFSET_1       = 0x5D   42 MODEM_BCR_MISC1              = 0xC2
//   38 MODEM_BCR_NCO_OFFSET_0       = 0x86
*/
#define RF_MODEM_BCR_OSR_1_9 0x11, 0x20, 0x09, 0x22, 0x01, 0x77, 0x01, 0x5D, 0x86, 0x00, 0xAF, 0x02, 0xC2

/*
// RF_MODEM_AFC_GEAR_7 —— 写 MODEM 组 44~50 号属性（共 7 个）
//   0x11 0x20 0x07 0x2C  ← 命令码 / 属性组 / 个数 / 起始号
//   44 MODEM_AFC_GEAR               = 0x04   48 MODEM_AFC_LIMITER_1          = 0x10
//   45 MODEM_AFC_WAIT               = 0x23   49 MODEM_AFC_LIMITER_0          = 0x04
//   46 MODEM_AFC_GAIN_1             = 0x80   50 MODEM_AFC_MISC               = 0x80
//   47 MODEM_AFC_GAIN_0             = 0x1D
*/
#define RF_MODEM_AFC_GEAR_7 0x11, 0x20, 0x07, 0x2C, 0x04, 0x23, 0x80, 0x1D, 0x10, 0x04, 0x80

/*
// RF_MODEM_AGC_CONTROL_1 —— 写 MODEM 组 53~53 号属性（共 1 个）
//   0x11 0x20 0x01 0x35  ← 命令码 / 属性组 / 个数 / 起始号
//   53 MODEM_AGC_CONTROL                  = 0xE0
*/
#define RF_MODEM_AGC_CONTROL_1 0x11, 0x20, 0x01, 0x35, 0xE0

/*
// RF_MODEM_AGC_WINDOW_SIZE_9 —— 写 MODEM 组 56~64 号属性（共 9 个）
//   0x11 0x20 0x09 0x38  ← 命令码 / 属性组 / 个数 / 起始号
//   56 MODEM_AGC_WINDOW_SIZE        = 0x11   61 MODEM_FSK4_TH1               = 0xFF
//   57 MODEM_AGC_RFPD_DECAY         = 0x52   62 MODEM_FSK4_TH0               = 0xFF
//   58 MODEM_AGC_IFPD_DECAY         = 0x52   63 MODEM_FSK4_MAP               = 0x00
//   59 MODEM_FSK4_GAIN1             = 0x00   64 MODEM_OOK_PDTC               = 0x2A
//   60 MODEM_FSK4_GAIN0             = 0x02
*/
#define RF_MODEM_AGC_WINDOW_SIZE_9 0x11, 0x20, 0x09, 0x38, 0x11, 0x52, 0x52, 0x00, 0x02, 0xFF, 0xFF, 0x00, 0x2A

/*
// RF_MODEM_OOK_CNT1_9 —— 写 MODEM 组 66~74 号属性（共 9 个）
//   0x11 0x20 0x09 0x42  ← 命令码 / 属性组 / 个数 / 起始号
//   66 MODEM_OOK_CNT1               = 0xA4   71 MODEM_RAW_EYE_0              = 0x55
//   67 MODEM_OOK_MISC               = 0x22   72 MODEM_ANT_DIV_MODE           = 0x01
//   68 MODEM_RAW_SEARCH             = 0xD6   73 MODEM_ANT_DIV_CONTROL        = 0x00
//   69 MODEM_RAW_CONTROL            = 0x83   74 MODEM_RSSI_THRESH            = 0xFF
//   70 MODEM_RAW_EYE_1              = 0x01
*/
#define RF_MODEM_OOK_CNT1_9 0x11, 0x20, 0x09, 0x42, 0xA4, 0x22, 0xD6, 0x83, 0x01, 0x55, 0x01, 0x00, 0xFF

/*
// RF_MODEM_RSSI_CONTROL_1 —— 写 MODEM 组 76~76 号属性（共 1 个）
//   0x11 0x20 0x01 0x4C  ← 命令码 / 属性组 / 个数 / 起始号
//   76 MODEM_RSSI_CONTROL                 = 0x01
*/
#define RF_MODEM_RSSI_CONTROL_1 0x11, 0x20, 0x01, 0x4C, 0x01

/*
// RF_MODEM_RSSI_COMP_1 —— 写 MODEM 组 78~78 号属性（共 1 个）
//   0x11 0x20 0x01 0x4E  ← 命令码 / 属性组 / 个数 / 起始号
//   78 MODEM_RSSI_COMP                    = 0x40
*/
#define RF_MODEM_RSSI_COMP_1 0x11, 0x20, 0x01, 0x4E, 0x40

/*
// RF_MODEM_CLKGEN_BAND_1 —— 写 MODEM 组 81~81 号属性（共 1 个）
//   0x11 0x20 0x01 0x51  ← 命令码 / 属性组 / 个数 / 起始号
//   81 MODEM_CLKGEN_BAND                  = 0x0A
*/
#define RF_MODEM_CLKGEN_BAND_1 0x11, 0x20, 0x01, 0x51, 0x0A

/*
// RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12 —— 写 MODEM_CHFLT 组 0~11 号属性（共 12 个）
//   0x11 0x21 0x0C 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 MODEM_CHFLT_RX1_CHFLT_COE13_7_0    = 0xA2
//    1 MODEM_CHFLT_RX1_CHFLT_COE12_7_0    = 0x81
//    2 MODEM_CHFLT_RX1_CHFLT_COE11_7_0    = 0x26
//    3 MODEM_CHFLT_RX1_CHFLT_COE10_7_0    = 0xAF
//    4 MODEM_CHFLT_RX1_CHFLT_COE9_7_0     = 0x3F
//    5 MODEM_CHFLT_RX1_CHFLT_COE8_7_0     = 0xEE
//    6 MODEM_CHFLT_RX1_CHFLT_COE7_7_0     = 0xC8
//    7 MODEM_CHFLT_RX1_CHFLT_COE6_7_0     = 0xC7
//    8 MODEM_CHFLT_RX1_CHFLT_COE5_7_0     = 0xDB
//    9 MODEM_CHFLT_RX1_CHFLT_COE4_7_0     = 0xF2
//   10 MODEM_CHFLT_RX1_CHFLT_COE3_7_0     = 0x02
//   11 MODEM_CHFLT_RX1_CHFLT_COE2_7_0     = 0x08
*/
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12 0x11, 0x21, 0x0C, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08

/*
// RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12 —— 写 MODEM_CHFLT 组 12~23 号属性（共 12 个）
//   0x11 0x21 0x0C 0x0C  ← 命令码 / 属性组 / 个数 / 起始号
//   12 MODEM_CHFLT_RX1_CHFLT_COE1_7_0     = 0x07
//   13 MODEM_CHFLT_RX1_CHFLT_COE0_7_0     = 0x03
//   14 MODEM_CHFLT_RX1_CHFLT_COEM0        = 0x15
//   15 MODEM_CHFLT_RX1_CHFLT_COEM1        = 0xFC
//   16 MODEM_CHFLT_RX1_CHFLT_COEM2        = 0x0F
//   17 MODEM_CHFLT_RX1_CHFLT_COEM3        = 0x00
//   18 MODEM_CHFLT_RX2_CHFLT_COE13_7_0    = 0xA2
//   19 MODEM_CHFLT_RX2_CHFLT_COE12_7_0    = 0x81
//   20 MODEM_CHFLT_RX2_CHFLT_COE11_7_0    = 0x26
//   21 MODEM_CHFLT_RX2_CHFLT_COE10_7_0    = 0xAF
//   22 MODEM_CHFLT_RX2_CHFLT_COE9_7_0     = 0x3F
//   23 MODEM_CHFLT_RX2_CHFLT_COE8_7_0     = 0xEE
*/
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12 0x11, 0x21, 0x0C, 0x0C, 0x07, 0x03, 0x15, 0xFC, 0x0F, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE

/*
// RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12 —— 写 MODEM_CHFLT 组 24~35 号属性（共 12 个）
//   0x11 0x21 0x0C 0x18  ← 命令码 / 属性组 / 个数 / 起始号
//   24 MODEM_CHFLT_RX2_CHFLT_COE7_7_0     = 0xC8
//   25 MODEM_CHFLT_RX2_CHFLT_COE6_7_0     = 0xC7
//   26 MODEM_CHFLT_RX2_CHFLT_COE5_7_0     = 0xDB
//   27 MODEM_CHFLT_RX2_CHFLT_COE4_7_0     = 0xF2
//   28 MODEM_CHFLT_RX2_CHFLT_COE3_7_0     = 0x02
//   29 MODEM_CHFLT_RX2_CHFLT_COE2_7_0     = 0x08
//   30 MODEM_CHFLT_RX2_CHFLT_COE1_7_0     = 0x07
//   31 MODEM_CHFLT_RX2_CHFLT_COE0_7_0     = 0x03
//   32 MODEM_CHFLT_RX2_CHFLT_COEM0        = 0x15
//   33 MODEM_CHFLT_RX2_CHFLT_COEM1        = 0xFC
//   34 MODEM_CHFLT_RX2_CHFLT_COEM2        = 0x0F
//   35 MODEM_CHFLT_RX2_CHFLT_COEM3        = 0x00
*/
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12 0x11, 0x21, 0x0C, 0x18, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08, 0x07, 0x03, 0x15, 0xFC, 0x0F, 0x00

/*
// RF_PA_MODE_4 —— 写 PA 组 0~3 号属性（共 4 个）
//   0x11 0x22 0x04 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 PA_MODE                            = 0x08
//    1 PA_PWR_LVL                         = 0x7F
//    2 PA_BIAS_CLKDUTY                    = 0x00
//    3 PA_TC                              = 0x1D
// 注：PA_PWR_LVL=0x7F 为最大功率档
*/
#define RF_PA_MODE_4 0x11, 0x22, 0x04, 0x00, 0x08, 0x7F, 0x00, 0x1D

/*
// RF_SYNTH_PFDCP_CPFF_7 —— 写 SYNTH 组 0~6 号属性（共 7 个）
//   0x11 0x23 0x07 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 SYNTH_PFDCP_CPFF             = 0x2C    4 SYNTH_LPFILT2                = 0x0C
//    1 SYNTH_PFDCP_CPINT            = 0x0E    5 SYNTH_LPFILT1                = 0x73
//    2 SYNTH_VCO_KV                 = 0x0B    6 SYNTH_LPFILT0                = 0x03
//    3 SYNTH_LPFILT3                = 0x04
*/
#define RF_SYNTH_PFDCP_CPFF_7 0x11, 0x23, 0x07, 0x00, 0x2C, 0x0E, 0x0B, 0x04, 0x0C, 0x73, 0x03

/*
// RF_MATCH_VALUE_1_12 —— 写 MATCH 组 0~11 号属性（共 12 个）
//   0x11 0x30 0x0C 0x00  ← 命令码 / 属性组 / 个数 / 起始号
//    0 MATCH_VALUE_1                = 0x00    6 MATCH_VALUE_3                = 0x00
//    1 MATCH_MASK_1                 = 0x00    7 MATCH_MASK_3                 = 0x00
//    2 MATCH_CTRL_1                 = 0x00    8 MATCH_CTRL_3                 = 0x00
//    3 MATCH_VALUE_2                = 0x00    9 MATCH_VALUE_4                = 0x00
//    4 MATCH_MASK_2                 = 0x00   10 MATCH_MASK_4                 = 0x00
//    5 MATCH_CTRL_2                 = 0x00   11 MATCH_CTRL_4                 = 0x00
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
