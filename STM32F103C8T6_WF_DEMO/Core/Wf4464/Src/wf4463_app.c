/**
 * @file    wf4463_app.c
 * @brief   Wf4463 应用层实现（ Wf446x API 库 + STM32 HAL）
 */

#include <string.h>
#include <stdio.h>

#include "compiler_defs.h"
#include "wf4463_port.h"
#include "wf4463_app.h"

#include "radio_hal.h"
#include "radio_comm.h"
#include "wf446x_api_lib.h"
#include "wf446x_defs.h"
#include "radio_config.h"        /* WDS 生成的射频配置 */

/* WDS 生成的配置数组（严格按 wf446x_configuration_init() 的格式：
   LEN | 数据…… ，最后以 0x00 结尾）。第一条就是 POWER_UP。 */
static const U8 Radio_Configuration[] = RADIO_CONFIGURATION_DATA_ARRAY;

/* 最近一次读到（并已清除）的中断挂起位 */
static U8 s_ph_pend;
static U8 s_modem_pend;
static U8 s_chip_pend;

/* ------------------------------------------------------------------ */
/* 内部：读中断并清除                                                  */
/* 注意：必须把挂起位写回 GET_INT_STATUS 的清除参数，nIRQ 才会回高。      */
/* 只读不清的话 nIRQ 一直是低，看起来像卡死。                            */
/* ------------------------------------------------------------------ */
static void wf4463_irq_read_clear(void)
{
    wf446x_get_int_status(0, 0, 0);          /* 先读 */
    s_ph_pend    = Wf446xCmd.GET_INT_STATUS.PH_PEND;
    s_modem_pend = Wf446xCmd.GET_INT_STATUS.MODEM_PEND;
    s_chip_pend  = Wf446xCmd.GET_INT_STATUS.CHIP_PEND;
    wf446x_get_int_status(s_ph_pend, s_modem_pend, s_chip_pend);  /* 再清 */
}

/* ------------------------------------------------------------------ */
/* 内部：等 nIRQ 变低                                                  */
/* ------------------------------------------------------------------ */
static bool wf4463_wait_irq(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();

    for (;;)
    {
#ifdef WF4463_NO_NIRQ_PIN
        /* 没接 nIRQ 引脚时的退路：轮询中断状态寄存器。
           注意这会持续占用 SPI，功耗和效率都不如用引脚，仅应急用。 */
        wf446x_get_int_status(0, 0, 0);
        if (Wf446xCmd.GET_INT_STATUS.INT_PEND != 0)
        {
            return true;
        }
#else
        if (wf4463_irq_asserted())   /* nIRQ 被拉低 = 有中断 */
        {
            return true;
        }
#endif
        if ((timeout_ms != 0u) && ((HAL_GetTick() - t0) >= timeout_ms))
        {
            return false;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 内部：硬件复位（SDN 拉高 -> 拉低，触发 POR）                        */
/* wf446x_reset() 里的延时只有几个 NOP，在 STM32 上远不够 10us，   */
/* 所以这里自己实现，用 HAL_Delay 保证时序。                            */
/* ------------------------------------------------------------------ */
static void wf4463_hw_reset(void)
{
    radio_hal_SetNsel();

    radio_hal_AssertShutdown();                 /* SDN = 1 -> 关断 */
    HAL_Delay(WF4463_SDN_HIGH_MS);              /* 手册要求 >= 10us */

    radio_hal_DeassertShutdown();               /* SDN = 0 -> 触发 POR */
    HAL_Delay(WF4463_POR_MS);                   /* 等内部校准完成 */

    radio_comm_ClearCTS();
}

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */
int8_t wf4463_init(void)
{
    wf4463_hw_reset();

    /* 配置数组第一条是 POWER_UP，所以这里不用再单独调 wf446x_power_up() */
    if (wf446x_configuration_init(Radio_Configuration) != WF446X_SUCCESS)
    {
        return WF4463_ERR_CONFIG;
    }

    /* 清一次可能残留的中断，避免上电就"看起来有中断" */
    wf4463_irq_read_clear();

    return WF4463_OK;
}

int8_t wf4463_reset(void)
{
    return wf4463_init();
}

/* ------------------------------------------------------------------ */
/*                                                               */
/* ------------------------------------------------------------------ */

/* 16 位字段拼装。
 * 在 STM32（小端）上直接读 .U16 会得到字节颠倒的值 —— PART 就成了 0x6344。
 * 按 "U8[MSB]<<8 | U8[LSB]" 读取，与头文件里 MSB/LSB 的取值无关，永远对。 */
static uint16_t wf4463_join16(const UU16 *v)
{
    return (uint16_t)(((uint16_t)v->U8[MSB] << 8) | (uint16_t)v->U8[LSB]);
}

int8_t wf4463_read_info(wf4463_info_t *info)
{
    if (info == NULL)
    {
        return WF4463_ERR_PARAM;
    }

    wf446x_part_info();          /* 0x01：只读命令，任何状态下都能发 */

    info->part     = wf4463_join16(&Wf446xCmd.PART_INFO.PART);
    info->id       = wf4463_join16(&Wf446xCmd.PART_INFO.ID);
    info->chiprev  = Wf446xCmd.PART_INFO.CHIPREV;
    info->pbuild   = Wf446xCmd.PART_INFO.PBUILD;
    info->customer = Wf446xCmd.PART_INFO.CUSTOMER;
    info->romid    = Wf446xCmd.PART_INFO.ROMID;

    return WF4463_OK;
}

int8_t wf4463_part_info(uint16_t *part, uint8_t *rev)
{
    wf4463_info_t info;
    int8_t        rc;

    if ((part == NULL) || (rev == NULL))
    {
        return WF4463_ERR_PARAM;
    }

    rc = wf4463_read_info(&info);
    if (rc != WF4463_OK)
    {
        return rc;
    }

    *part = info.part;
    *rev  = info.chiprev;

    return WF4463_OK;
}

int8_t wf4463_print_info(void)
{
    wf4463_info_t info;

    if (wf4463_read_info(&info) != WF4463_OK)
    {
        return WF4463_ERR_PARAM;
    }

    printf("PART=0x%04X CHIPREV=0x%02X PBUILD=0x%02X ID=0x%04X CUSTOMER=0x%02X ROMID=0x%02X\r\n",
           (unsigned)info.part, (unsigned)info.chiprev, (unsigned)info.pbuild,
           (unsigned)info.id, (unsigned)info.customer, (unsigned)info.romid);

    /*  Wf4463 的 PART 固定 0x4463*/
    if (info.part != 0x4463)
    {
        printf("  !! PART 应为 0x4463，实际 0x%04X（0x0000/0xFFFF=SPI 没通）\r\n",
               (unsigned)info.part);
    }

    return WF4463_OK;
}

bool wf4463_irq_asserted(void)
{
    return (radio_hal_NirqLevel() == (BIT)FALSE) ? true : false;
}

int8_t wf4463_device_state(uint8_t *state)
{
    if (state == NULL)
    {
        return WF4463_ERR_PARAM;
    }

    wf446x_request_device_state();      /* 0x33：只读，不会打断 RX */
    *state = Wf446xCmd.REQUEST_DEVICE_STATE.CURR_STATE;

    return WF4463_OK;
}

int8_t wf4463_diag(wf4463_diag_t *d)
{
    wf4463_info_t info;

    if (d == NULL)
    {
        return WF4463_ERR_PARAM;
    }

    /* 1) CTS：唯一能区分"芯片没应答"和"芯片应答了但内容异常"的指标 */
    d->cts = radio_comm_PollCTS();

    /* 2) 身份：SPI 读通路是否还活着 */
    (void)wf4463_read_info(&info);
    d->part    = info.part;
    d->chiprev = info.chiprev;

    /* 3) 配置是否还在芯片里（掉电/复位后芯片会回到默认值 0x40 / 0x00） */
    wf446x_get_property(WF446X_PROP_GRP_ID_GLOBAL, 1u,
                        WF446X_PROP_GRP_INDEX_GLOBAL_XO_TUNE);
    d->xo_tune = Wf446xCmd.GET_PROPERTY.DATA0;

    wf446x_get_property(WF446X_PROP_GRP_ID_PA, 1u,
                        WF446X_PROP_GRP_INDEX_PA_PWR_LVL);
    d->pa_pwr = Wf446xCmd.GET_PROPERTY.DATA0;

    /* 4) 芯片状态与 RSSI 原始值 */
    (void)wf4463_device_state(&d->state);

    wf446x_get_modem_status(0u);
    d->rssi_raw = Wf446xCmd.GET_MODEM_STATUS.CURR_RSSI;

    return WF4463_OK;
}

/* ------------------------------------------------------------------ */
/* 发送                                                              */
/* ------------------------------------------------------------------ */
int8_t wf4463_tx(uint8_t channel, const uint8_t *data, uint8_t len)
{
    U8 frame[WF4463_MAX_PAYLOAD + 1u];
    uint32_t t0;

    if ((data == NULL) || (len == 0u) || (len > WF4463_MAX_PAYLOAD))
    {
        return WF4463_ERR_PARAM;
    }

    /* 1) 不管当前在 RX 还是 TX，先回 READY。
       在 RX 状态下直接写 TX FIFO，命令可能被芯片丢掉，发出去就是 FIFO 下溢(-7)。 */
    wf446x_change_state(WF446X_CMD_CHANGE_STATE_ARG_NEW_STATE_ENUM_READY);
    (void)radio_comm_PollCTS();

    /* 2) 清 FIFO、清残留中断。
       RX FIFO 溢出和 TX FIFO 下溢共用同一个 FIFO_UNDERFLOW_OVERFLOW 标志，
       不清掉会被误判成本次发送失败。 */
    wf446x_fifo_info(0x03u);
    wf4463_irq_read_clear();

    /* 3) 装包：本工程 radio_config.h 是变长包，FIFO 首字节是长度。 */
    frame[0] = len;
    (void)memcpy(&frame[1], data, (size_t)len);

    /* 变长包 TX 的关键一步：把本包长度写进 PKT_FIELD_2_LENGTH。
       初始化时它是 0x1FFF（最大 8191），不改的话包处理器会一直从 FIFO 取字节，
       取完即报 FIFO 下溢，也就是 -7。 */
    wf446x_set_property(WF446X_PROP_GRP_ID_PKT, 2u,
                        WF446X_PROP_GRP_INDEX_PKT_FIELD_2_LENGTH_12_8,
                        (U8)((U16)len >> 8), (U8)len);

    wf446x_write_tx_fifo((U8)(len + 1u), frame);
    wf446x_start_tx(channel, WF4463_TX_DONE_STATE, 0u);

    /* 4) 只等 PACKET_SENT 或 FIFO 错误。
       radio_config.h 里 PKT_TX_THRESHOLD = 0x30(48)，小包一进 FIFO 就立即产生
       FIFO_ALMOST_EMPTY 中断把 nIRQ 拉低；若一看到 nIRQ 低就返回，
       永远等不到 PACKET_SENT，就会一直报 -4。 */
    /* 4) 不依赖 nIRQ 引脚，直接轮询中断状态寄存器判断发送完成 */
    t0 = HAL_GetTick();
    for (;;)
    {
        wf446x_get_int_status(0u, 0u, 0u);

        if ((Wf446xCmd.GET_INT_STATUS.CHIP_PEND &
             WF446X_CMD_GET_INT_STATUS_REP_FIFO_UNDERFLOW_OVERFLOW_ERROR_PEND_BIT) != 0u)
        {
            wf446x_get_int_status(0xFFu, 0xFFu, 0xFFu);
            return WF4463_ERR_FIFO;
        }

        if ((Wf446xCmd.GET_INT_STATUS.PH_PEND &
             WF446X_CMD_GET_INT_STATUS_REP_PACKET_SENT_PEND_BIT) != 0u)
        {
            wf446x_get_int_status(0xFFu, 0xFFu, 0xFFu);   /* 清标志 */
            return WF4463_OK;
        }

        if ((HAL_GetTick() - t0) >= WF4463_TX_TIMEOUT_MS)
        {
            wf446x_get_int_status(0xFFu, 0xFFu, 0xFFu);
            return WF4463_ERR_TIMEOUT;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 接收                                                              */
/* ------------------------------------------------------------------ */
int8_t wf4463_rx_start(uint8_t channel)
{
    /* 发包时 PKT_FIELD_2_LENGTH 被改成了实际长度，接收前必须恢复成
       变长包的最大值 0x1FFF，否则收长包会被截断。 */
    wf446x_set_property(WF446X_PROP_GRP_ID_PKT, 2,
                        WF446X_PROP_GRP_INDEX_PKT_FIELD_2_LENGTH_12_8,
                        0x1Fu, 0xFFu);

    /* TX -> RX 必须先回 READY（不可直接切换）。
       发射超时(-3)时芯片可能还停在 TX，此时直接 START_RX 会被忽略，
       表现就是"收不到、也没有任何错误"。
       同理，RX 之间的重启走一遍 READY 也无副作用。 */
    wf446x_change_state(WF446X_CMD_CHANGE_STATE_ARG_NEW_STATE_ENUM_READY);
    (void)radio_comm_PollCTS();

    /* 清掉 RX FIFO 里的残留字节（0x02 = RX FIFO reset）。
       不清的话下一次收包的长度字节可能是上一包的残留，长度全乱。 */
    wf446x_fifo_info(0x02);

    /* RX_LEN = 0：按包处理器配置来 */
    wf446x_start_rx(channel, 0, 0, 0, 0, 0);
    return WF4463_OK;
}

int16_t wf4463_rx_fetch(uint8_t *buf, uint8_t max_len, uint8_t *out_len,
                        uint32_t timeout_ms)
{
    U8 tmp[WF4463_MAX_PAYLOAD + 1];
    U8 count;
    U8 n;
    uint32_t t0;

    if ((buf == NULL) || (out_len == NULL) || (max_len == 0))
    {
        return WF4463_ERR_PARAM;
    }

    t0 = HAL_GetTick();

    /* ★关键：这里必须"原地等"，不能一看到 nIRQ 低就返回。
     * INT_CTL_MODEM_ENABLE = 0x03，前导码检测 / 同步字检测都会拉低 nIRQ。
     * 如果把它们当成"没收到包"返回给应用层，应用层就会 wf4463_rx_start()
     * 重启接收 —— 正好把正在收的这一个包掐断，于是永远收不到完整包，
     * 串口上只剩刷屏的 "RX err -6"。 */
    for (;;)
    {
        uint32_t elapsed = HAL_GetTick() - t0;
        uint32_t remain;

        if ((timeout_ms != 0) && (elapsed >= timeout_ms))
        {
            return WF4463_ERR_TIMEOUT;
        }
        remain = (timeout_ms == 0) ? 5 : (timeout_ms - elapsed);

        if (!wf4463_wait_irq(remain))
        {
            return WF4463_ERR_TIMEOUT;
        }

        wf4463_irq_read_clear();

        if ((s_ph_pend & WF446X_CMD_GET_INT_STATUS_REP_CRC_ERROR_PEND_BIT) != 0)
        {
            return WF4463_ERR_CRC;
        }
        if ((s_ph_pend & WF446X_CMD_GET_INT_STATUS_REP_PACKET_RX_PEND_BIT) != 0)
        {
            break;                              /* 完整一包已到，去读 FIFO */
        }
        /* 其它中断（前导码/同步字/FIFO 门限/芯片级错误）继续等 */
    }

    wf446x_fifo_info(0);                       /* 参数 0 = 不复位 FIFO，只读计数 */
    count = Wf446xCmd.FIFO_INFO.RX_FIFO_COUNT;

    if (count == 0)
    {
        return WF4463_ERR_NOPKT;
    }
    if (count > (U8)(WF4463_MAX_PAYLOAD + 1))
    {
        count = (U8)(WF4463_MAX_PAYLOAD + 1);  /* 防止缓冲溢出 */
    }

    wf446x_read_rx_fifo(count, tmp);

    /* 变长包模式下，长度字节由包处理器解析，不会出现在 RX FIFO 里，
       因此 FIFO 中的 count 个字节就是纯负载，从 tmp[0] 开始拷贝 */
    n = (count > max_len) ? max_len : count;
    (void)memcpy(buf, tmp, (size_t)n);
    *out_len = n;

    return (int16_t)n;
}

/* ------------------------------------------------------------------ */
/* RSSI                                                              */
/* ------------------------------------------------------------------ */
int16_t wf4463_rssi_dbm(void)
{
    U8 rssi;

    wf446x_get_modem_status(0);
    rssi = Wf446xCmd.GET_MODEM_STATUS.CURR_RSSI;

    /* Wf446x: RSSI_dBm = RSSI/2 - 130 */
    return (int16_t)(((int16_t)rssi / 2) - 130);
}
