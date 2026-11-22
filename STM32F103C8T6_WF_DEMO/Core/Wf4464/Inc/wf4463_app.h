/**
 * @file    wf4463_app.h
 * @brief   Wf4463 应用层接口（基于 Wf446x API 库）
 *
 * 依赖：
 *   -  wf446x_api_lib.c / radio_comm.c
 *   - 本目录 Src/radio_hal.c（STM32 HAL 适配层）
 *   - 编译时定义：RADIO_DRIVER_EXTENDED_SUPPORT（必需，FIFO 读写函数）
 *                 RADIO_DRIVER_FULL_SUPPORT    （可选，RSSI/状态查询）
 */

#ifndef WF4463_APP_H
#define WF4463_APP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 返回值：0 = 成功，负数 = 错误 */
#define WF4463_OK             0
#define WF4463_ERR_PARAM    (-1)   /* 参数错误 */
#define WF4463_ERR_CONFIG   (-2)   /* 写配置失败（CTS 超时/命令错误） */
#define WF4463_ERR_TIMEOUT  (-3)   /* 等 nIRQ 超时 */
#define WF4463_ERR_TX       (-4)   /* 没有收到 PACKET_SENT */
#define WF4463_ERR_CRC      (-5)   /* 收到包但 CRC 错 */
#define WF4463_ERR_NOPKT    (-6)   /* 没有收到包（可能是无效同步字/前导码） */
#define WF4463_ERR_FIFO     (-7)   /* FIFO 上溢/下溢 */

/**
 * @brief  初始化：硬件复位 + 上电 + 写 WDS 配置
 * @retval WF4463_OK 或错误码
 * @note   调用前请确保：SPI 已初始化、nSEL/SDN 引脚已配成推挽输出、
 *         nIRQ 已配成输入（上拉）。
 */
int8_t wf4463_init(void);

/**
 * @brief  芯片身份信息（PART_INFO 命令 0x01 的完整回包）
 * @note   16 位字段已按"与编译器字节序无关"的方式拼好，可直接打印
 */
typedef struct
{
    uint16_t part;      /* 型号：Wf4463 = 0x4463 */
    uint8_t  chiprev;   /* 芯片掩膜版本（B1 常见 0x03） */
    uint8_t  pbuild;    /* Part Build，一般 0x00 */
    uint16_t id;        /* 芯片 ID（Wf4463 常见 0x0086） */
    uint8_t  customer;  /* Customer ID，一般 0x00 */
    uint8_t  romid;     /* ROM ID（Wf4463 常见 0x06） */
} wf4463_info_t;

/**
 * @brief  读芯片型号与版本
 * @param  part 输出：0x4463 表示 Wf4463
 * @param  rev  输出：芯片版本（如 0x03 = B1/C）
 */
int8_t wf4463_part_info(uint16_t *part, uint8_t *rev);

/**
 * @brief  读完整的 PART_INFO（型号/版本/PBUILD/ID/CUSTOMER/ROMID）
 * @param  info 输出结构体
 * @note   16 位字段用 U8[MSB]/U8[LSB] 自行拼装：官方库的 UU16 联合体
 *         在小端 MCU 上直接读 .U16 会得到字节颠倒的值（0x6344）。
 */
int8_t wf4463_read_info(wf4463_info_t *info);

/**
 * @brief  自检：一次读全"SPI 通不通 + 配置还在不在 + 芯片在什么状态"
 */
typedef struct
{
    uint8_t  cts;       /* 0xFF = 芯片就绪；其它值 = CTS 超时，SPI 其实没通 */
    uint16_t part;      /* 0x4463 */
    uint8_t  chiprev;   /* 0x22 */
    uint8_t  xo_tune;   /* 读回 GLOBAL_XO_TUNE，配置生效时应为 0x66 */
    uint8_t  pa_pwr;    /* 读回 PA_PWR_LVL，配置生效时应为 0x7F */
    uint8_t  state;     /* REQUEST_DEVICE_STATE：3=READY 7=TX 8=RX；0 或 >8 都是异常 */
    uint8_t  rssi_raw;  /* GET_MODEM_STATUS.CURR_RSSI，0 = 射频前端没工作 */
} wf4463_diag_t;

/**
 * @brief  读一次自检（全部是只读命令，不会打断 RX）
 */
int8_t wf4463_diag(wf4463_diag_t *d);

/**
 * @brief  读完整 PART_INFO 并打印一行到串口（初始化后用）
 * @retval WF4463_OK 或错误码
 */
int8_t wf4463_print_info(void);

/**
 * @brief  nIRQ 是否为低（有中断未取）
 */
bool wf4463_irq_asserted(void);

/**
 * @brief  查询芯片当前状态（排查"收不到"用）
 * @param  state 输出：AN625 MAIN_STATE
 *         0=NoChange 1=Sleep 2=SpiActive 3=Ready 4=Ready2
 *         5=TX Tune 6=RX Tune 7=TX 8=RX
 * @note   RX 期间读这个不会打断接收
 */
int8_t wf4463_device_state(uint8_t *state);

/**
 * @brief  发送一包（阻塞，直到发完或超时）
 * @param  channel 信道号（WDS 里配的频率表序号，单频应用填 0）
 * @param  data    载荷
 * @param  len     载荷长度（1 ~ WF4463_MAX_PAYLOAD）
 * @note   FIFO 会自动加上"长度字节"（本工程配置为变长包）。
 */
int8_t wf4463_tx(uint8_t channel, const uint8_t *data, uint8_t len);

/**
 * @brief  启动接收
 */
int8_t wf4463_rx_start(uint8_t channel);

/**
 * @brief  直收（Raw RX）：芯片只做解调，比特流直接从 GPIO1 输出
 * @param  channel 信道号（单频应用填 0）
 * @note   不经过包处理器、MCU 侧不取 FIFO，芯片常驻 RX 状态。
 *         与双向包模式共用同一份射频配置（radio_config.h），
 *         切换只重配 GPIO 和接收状态，不重写属性表。
 */
int8_t wf4463_rawrx_start(uint8_t channel);

/**
 * @brief  从直收切回双向包：恢复 GPIO 配置并挂接收
 * @param  channel 信道号（单频应用填 0）
 */
int8_t wf4463_packet_resume(uint8_t channel);

/**
 * @brief  等待并取出一包（阻塞）
 * @param  buf        输出缓冲区
 * @param  max_len    buf 容量
 * @param  out_len    输出：实际载荷长度
 * @param  timeout_ms 0 = 永久等待
 * @retval >0 表示收到的字节数，<0 为错误码
 */
int16_t wf4463_rx_fetch(uint8_t *buf, uint8_t max_len, uint8_t *out_len,
                        uint32_t timeout_ms);

/**
 * @brief  读当前 RSSI（dBm），需要 RADIO_DRIVER_FULL_SUPPORT
 */
int16_t wf4463_rssi_dbm(void);

/**
 * @brief  软件复位（重新走一遍 SDN -> POR -> 写配置）
 */
int8_t wf4463_reset(void);

/* ==================================================================
 * 低功耗（休眠档）
 *
 * 芯片状态机里只有两个能主动进入的低功耗状态：SHUTDOWN（SDN 引脚）
 * 和 SLEEP（CHANGE_STATE 0x01）。下面四档是"可落地的四种配置"：
 *
 *   STANDBY   SLEEP + WUT 关 + 32k 关      → 40 nA
 *   SLEEP_RC  SLEEP + WUT + 32k 内部 RC    → 740 nA
 *   SENSOR    SLEEP + 32k RC + LBD         → 约 1 µA
 *   SHUTDOWN  SDN 拉高                      → 30 nA（配置全丢）
 *
 * 第五种 SLEEP + 外部 32.768 kHz 晶振本板做不了：官方要求晶振跨接
 * GPIO0/GPIO1，而这两脚已固定给 RX 开关和直收数据。
 * ================================================================== */

typedef enum
{
    WF4463_LP_STANDBY = 0,   /* 40 nA，唤醒最快 */
    WF4463_LP_SLEEP_RC,      /* 740 nA，WUT 与 32k RC 在跑 */
    WF4463_LP_SENSOR,        /* 约 1 µA，另加低电压检测 */
    WF4463_LP_SHUTDOWN       /* 30 nA，寄存器全丢 */
} wf4463_lp_mode_t;

/**
 * @brief  进入指定休眠档（含 SHUTDOWN）
 * @param  mode     档位
 * @param  sleep_ms WUT 周期（毫秒）。STANDBY / SHUTDOWN 忽略此参数
 * @note   本工程的用法是"上电睡一次、按键后不再睡"，所以 WUT 到期
 *         一律留在 SLEEP 只置中断标志，不会自己跳回 READY(1.8 mA)。
 * @note   SLEEP_RC / SENSOR 的 WUT 中断不接到 nIRQ，避免到期后
 *         nIRQ 一直为低把电流顶上去。SENSOR 的 LBD 中断是接的。
 */
int8_t wf4463_lp_enter(wf4463_lp_mode_t mode, uint32_t sleep_ms);

/**
 * @brief  把芯片从休眠档叫醒，回到可收发的工作态
 * @note   SLEEP 档：任何一次 SPI 访问都会让它自动回到 SPI ACTIVE，
 *         这里读一次中断状态落实并清掉残留标志。
 * @note   SHUTDOWN 档：寄存器已丢失，必须重走 wf4463_init()（约 120 ms）。
 */
int8_t wf4463_lp_wake(void);

/**
 * @brief  设置 LBD 低电压阈值（GLOBAL_LOW_BATT_THRESH，5 bit）
 * @param  thresh 原始值；芯片默认 0x18。电压换算见手册该属性
 */
int8_t wf4463_lp_lowbatt_thresh(uint8_t thresh);

/**
 * @brief  档位名字，打印用
 */
const char *wf4463_lp_name(wf4463_lp_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* WF4463_APP_H */
