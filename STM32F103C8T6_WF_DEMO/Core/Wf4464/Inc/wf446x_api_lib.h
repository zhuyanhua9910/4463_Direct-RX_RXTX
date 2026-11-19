/*!
 * 文件：
 *  wf446x_api_lib.h
 *
 * 说明：
 *  本文件中包含 Wf446x 芯片的 API 库函数声明。
 *
 * Silicon Laboratories 机密资料
 * 版权所有 2011 Silicon Laboratories, Inc.
 */

#ifndef _WF446X_API_LIB_H_
#define _WF446X_API_LIB_H_

extern union wf446x_cmd_reply_union Wf446xCmd;
extern U8 Pro2Cmd[16];


#define WF466X_FIFO_SIZE 64

enum
{
    WF446X_SUCCESS,
    WF446X_NO_PATCH,
    WF446X_CTS_TIMEOUT,
    WF446X_PATCH_FAIL,
    WF446X_COMMAND_ERROR
};

/* 精简版驱动支持函数（最小功能集） */
void wf446x_reset(void);
void wf446x_power_up(U8 BOOT_OPTIONS, U8 XTAL_OPTIONS, U32 XO_FREQ);

U8 wf446x_configuration_init(const U8* pSetPropCmd);
U8 wf446x_apply_patch(void);
void wf446x_part_info(void);

void wf446x_start_tx(U8 CHANNEL, U8 CONDITION, U16 TX_LEN);
void wf446x_start_rx(U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3);

void wf446x_get_int_status(U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND);
void wf446x_gpio_pin_cfg(U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG);

void wf446x_set_property( U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... );
void wf446x_change_state(U8 NEXT_STATE1);

#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
  /* 扩展版驱动支持函数 */
  void wf446x_nop(void);

  void wf446x_fifo_info(U8 FIFO);
  void wf446x_write_tx_fifo( U8 numBytes, U8* pData );
  void wf446x_read_rx_fifo( U8 numBytes, U8* pRxData );

  void wf446x_get_property(U8 GROUP, U8 NUM_PROPS, U8 START_PROP);
  void reConfigPktHandler(void);
  #ifdef RADIO_DRIVER_FULL_SUPPORT
    /* 完整版驱动支持函数 */
    void wf446x_func_info(void);

    void wf446x_frr_a_read(U8 respByteCount);
    void wf446x_frr_b_read(U8 respByteCount);
    void wf446x_frr_c_read(U8 respByteCount);
    void wf446x_frr_d_read(U8 respByteCount);

    void wf446x_get_adc_reading(U8 ADC_EN);
    void wf446x_get_packet_info(U8 FIELD_NUMBER_MASK, U16 LEN, S16 DIFF_LEN );
    void wf446x_get_ph_status(U8 PH_CLR_PEND);
    void wf446x_get_modem_status( U8 MODEM_CLR_PEND );
    void wf446x_get_chip_status( U8 CHIP_CLR_PEND );

    void wf446x_ircal(U8 SEARCHING_STEP_SIZE, U8 SEARCHING_RSSI_AVG, U8 RX_CHAIN_SETTING1, U8 RX_CHAIN_SETTING2);
    void wf446x_protocol_cfg(U8 PROTOCOL);

    void wf446x_request_device_state(void);

    void wf446x_rx_hop(U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0);
    void wf446x_agc_override(U8 AGC_OVERRIDE);
  #endif
#endif


#endif //_WF446X_API_LIB_H_
