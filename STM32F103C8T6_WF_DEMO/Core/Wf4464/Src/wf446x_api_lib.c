/*!
 * 文件：
 *  wf446x_api_lib.c
 *
 * 说明：
 *  本文件中包含 Wf446x 芯片 API 库的实现。
 *
 * Silicon Laboratories 机密资料
 * 版权所有 2011 Silicon Laboratories, Inc.
 */

#include "compiler_defs.h"
#include "wf446x_defs.h"
#include "wf446x_api_lib.h"
#include "radio_comm.h"
#include "radio_hal.h"



#include <stdarg.h>

union wf446x_cmd_reply_union Wf446xCmd;
U8 Pro2Cmd[16];

#ifdef WF446X_PATCH_CMDS
const U8 Wf446xPatchCommands[][8] = { WF446X_PATCH_CMDS };
#endif


/*!
 * 本函数通过对芯片拉关断脚再释放的方式，让 wf446x 复位。
 * 调用本函数之后应继续调用 @ref wf446x_boot（本库中对应 wf446x_power_up）。判断 POR（上电复位）是否完成，
 * 可以等 4 ms，也可以轮询 GPIO0、GPIO2 或 GPIO3；当这些脚变高时，
 * 就可以安全地调用 @ref wf446x_boot 了。
 */
void wf446x_reset(void)
{
    U8 loopCount;

    /* 先把芯片拉入关断状态，等一下再释放 */
    radio_hal_AssertShutdown();
    //! @todo 这里应该换成更靠谱的延时函数。
    for (loopCount = 255; loopCount != 0; loopCount--);
    radio_hal_DeassertShutdown();
    for (loopCount = 255; loopCount != 0; loopCount--);
    radio_comm_ClearCTS();
}

/*!
 * 本函数用于芯片上电后的初始化。
 * 调用本函数之前应先调用 @wf446x_reset。
 */
void wf446x_power_up(U8 BOOT_OPTIONS, U8 XTAL_OPTIONS, U32 XO_FREQ)
{
    Pro2Cmd[0] = WF446X_CMD_ID_POWER_UP;
    Pro2Cmd[1] = BOOT_OPTIONS;
    Pro2Cmd[2] = XTAL_OPTIONS;
    Pro2Cmd[3] = (U8)(XO_FREQ >> 24);
    Pro2Cmd[4] = (U8)(XO_FREQ >> 16);
    Pro2Cmd[5] = (U8)(XO_FREQ >> 8);
    Pro2Cmd[6] = (U8)(XO_FREQ);

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_POWER_UP, Pro2Cmd );
}

/*!
 * 本函数用一张以 0x00 结尾的命令表，把所有的属性和命令一次性写入芯片。
 * 调用本函数之前应先调用 @wf446x_reset。
 */
U8 wf446x_configuration_init(const U8* pSetPropCmd)
{
  U8 col;
  U8 numOfBytes;

  /* 只要指针还指向一条命令，就一直循环 */
  while (*pSetPropCmd != 0x00)
  {
    /* 数组中命令的格式：
     * --------------------------------
     * LEN | <长度为 LEN 的数据>
     */

    numOfBytes = *pSetPropCmd++;

    if (numOfBytes > 16u)
    {
      /* 命令字节数超过允许的最大长度 */
      return WF446X_COMMAND_ERROR;
    }

    for (col = 0u; col < numOfBytes; col++)
    {
      Pro2Cmd[col] = *pSetPropCmd;
      pSetPropCmd++;
    }

    if (radio_comm_SendCmdGetResp(numOfBytes, Pro2Cmd, 0, 0) != 0xFF)
    {
      /* 发生超时 */
      return WF446X_CTS_TIMEOUT;
    }

    if (radio_hal_NirqLevel() == 0)
    {
      /* 读取并清除所有中断。说明出错了…… */
      wf446x_get_int_status(0, 0, 0);
      if (Wf446xCmd.GET_INT_STATUS.CHIP_PEND & WF446X_CMD_GET_CHIP_STATUS_REP_CMD_ERROR_PEND_MASK)
      {
        return WF446X_COMMAND_ERROR;
      }
    }
  }

  return WF446X_SUCCESS;
}

/*!
 * 本函数用于给 wf446x 打固件补丁（patch）。补丁数据以代码形式
 * 保存在 wf446x_patch.h 文件中。
 *
 * @return  WF446X_CTS_TIMEOUT 发生 CTS 超时。
 *          WF446X_PATCH_FAIL  补丁加载失败。
 *          WF446X_SUCCESS     补丁加载成功。
 *          WF446X_NO_PATCH    Flash 中没有可加载的补丁。
 */
U8 wf446x_apply_patch(void)
{
#ifdef WF446X_PATCH_CMDS
    SEGMENT_VARIABLE(line, U16, SEG_DATA);
    SEGMENT_VARIABLE(row,  U8, SEG_DATA);

    /* 先判断是否真的需要打补丁 */
    wf446x_part_info();

    if ((Wf446xCmd.PART_INFO.ROMID == WF446X_PATCH_ROMID) && (Wf446xCmd.PART_INFO.ID.U8[MSB] < WF446X_PATCH_ID))
    {
      for (line = 0; line < (sizeof(Wf446xPatchCommands) / 8u); line++)
      {
        for (row=0; row<8; row++)
        {
          Pro2Cmd[row] = Wf446xPatchCommands[line][row];
        }
        if (radio_comm_SendCmdGetResp(8, Pro2Cmd, 0, 0) != 0xFF)
        {
          // 发生超时
          return WF446X_CTS_TIMEOUT;
        }
        if (radio_hal_NirqLevel() == 0)
        {
          /* 读取并清除所有中断。说明出错了…… */
          wf446x_get_int_status(0, 0, 0);
          return WF446X_PATCH_FAIL;
        }
      }
    }

    return WF446X_SUCCESS;
#else
    return WF446X_NO_PATCH;
#endif
}

/*! 本函数向芯片发送 PART_INFO 命令，并把应答读入 @Wf446xCmd 联合体。
 */
void wf446x_part_info(void)
{
    Pro2Cmd[0] = WF446X_CMD_ID_PART_INFO;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_PART_INFO,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_PART_INFO,
                              Pro2Cmd );

    Wf446xCmd.PART_INFO.CHIPREV         = Pro2Cmd[0];
    Wf446xCmd.PART_INFO.PART.U8[MSB]    = Pro2Cmd[1];
    Wf446xCmd.PART_INFO.PART.U8[LSB]    = Pro2Cmd[2];
    Wf446xCmd.PART_INFO.PBUILD          = Pro2Cmd[3];
    Wf446xCmd.PART_INFO.ID.U8[MSB]      = Pro2Cmd[4];
    Wf446xCmd.PART_INFO.ID.U8[LSB]      = Pro2Cmd[5];
    Wf446xCmd.PART_INFO.CUSTOMER        = Pro2Cmd[6];
    Wf446xCmd.PART_INFO.ROMID           = Pro2Cmd[7];
}

/*! 向芯片发送 START_TX 命令。
 *
 * @param CHANNEL   信道号。
 * @param CONDITION 发送启动条件。
 * @param TX_LEN    载荷长度（不含包处理器自动生成的 CRC）。
 */
void wf446x_start_tx(U8 CHANNEL, U8 CONDITION, U16 TX_LEN)
{
    Pro2Cmd[0] = WF446X_CMD_ID_START_TX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(TX_LEN >> 8);
    Pro2Cmd[4] = (U8)(TX_LEN);
    Pro2Cmd[5] = 0x00;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_START_TX, Pro2Cmd );
}

/*!
 * 向芯片发送 START_RX 命令。
 *
 * @param CHANNEL     信道号。
 * @param CONDITION   接收启动条件。
 * @param RX_LEN      载荷长度（不含包处理器自动生成的 CRC）。
 * @param NEXT_STATE1 前导码超时后进入的状态。
 * @param NEXT_STATE2 收到有效包后进入的状态。
 * @param NEXT_STATE3 收到无效包（如 CRC 错）后进入的状态。
 */
void wf446x_start_rx(U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3)
{
    Pro2Cmd[0] = WF446X_CMD_ID_START_RX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(RX_LEN >> 8);
    Pro2Cmd[4] = (U8)(RX_LEN);
    Pro2Cmd[5] = NEXT_STATE1;
    Pro2Cmd[6] = NEXT_STATE2;
    Pro2Cmd[7] = NEXT_STATE3;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_START_RX, Pro2Cmd );
}

/*!
 * 从芯片读取中断状态/挂起标志，并按需要清除这些标志。
 *
 * @param PH_CLR_PEND     包处理器（Packet Handler）挂起标志清除位。
 * @param MODEM_CLR_PEND  调制解调器（Modem）状态挂起标志清除位。
 * @param CHIP_CLR_PEND   芯片状态挂起标志清除位。
 */
void wf446x_get_int_status(U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND)
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_INT_STATUS;
    Pro2Cmd[1] = PH_CLR_PEND;
    Pro2Cmd[2] = MODEM_CLR_PEND;
    Pro2Cmd[3] = CHIP_CLR_PEND;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_INT_STATUS,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GET_INT_STATUS,
                              Pro2Cmd );

    Wf446xCmd.GET_INT_STATUS.INT_PEND       = Pro2Cmd[0];
    Wf446xCmd.GET_INT_STATUS.INT_STATUS     = Pro2Cmd[1];
    Wf446xCmd.GET_INT_STATUS.PH_PEND        = Pro2Cmd[2];
    Wf446xCmd.GET_INT_STATUS.PH_STATUS      = Pro2Cmd[3];
    Wf446xCmd.GET_INT_STATUS.MODEM_PEND     = Pro2Cmd[4];
    Wf446xCmd.GET_INT_STATUS.MODEM_STATUS   = Pro2Cmd[5];
    Wf446xCmd.GET_INT_STATUS.CHIP_PEND      = Pro2Cmd[6];
    Wf446xCmd.GET_INT_STATUS.CHIP_STATUS    = Pro2Cmd[7];
}

/*!
 * 向芯片发送 GPIO 引脚配置命令，并把应答读入
 * @Wf446xCmd 联合体。
 *
 * @param GPIO0       GPIO0 配置。
 * @param GPIO1       GPIO1 配置。
 * @param GPIO2       GPIO2 配置。
 * @param GPIO3       GPIO3 配置。
 * @param NIRQ        NIRQ 配置。
 * @param SDO         SDO 配置。
 * @param GEN_CONFIG  通用引脚配置。
 */
void wf446x_gpio_pin_cfg(U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG)
{
    Pro2Cmd[0] = WF446X_CMD_ID_GPIO_PIN_CFG;
    Pro2Cmd[1] = GPIO0;
    Pro2Cmd[2] = GPIO1;
    Pro2Cmd[3] = GPIO2;
    Pro2Cmd[4] = GPIO3;
    Pro2Cmd[5] = NIRQ;
    Pro2Cmd[6] = SDO;
    Pro2Cmd[7] = GEN_CONFIG;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GPIO_PIN_CFG,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GPIO_PIN_CFG,
                              Pro2Cmd );

    Wf446xCmd.GPIO_PIN_CFG.GPIO0        = Pro2Cmd[0];
    Wf446xCmd.GPIO_PIN_CFG.GPIO1        = Pro2Cmd[1];
    Wf446xCmd.GPIO_PIN_CFG.GPIO2        = Pro2Cmd[2];
    Wf446xCmd.GPIO_PIN_CFG.GPIO3        = Pro2Cmd[3];
    Wf446xCmd.GPIO_PIN_CFG.NIRQ         = Pro2Cmd[4];
    Wf446xCmd.GPIO_PIN_CFG.SDO          = Pro2Cmd[5];
    Wf446xCmd.GPIO_PIN_CFG.GEN_CONFIG   = Pro2Cmd[6];
}

/*!
 * 向芯片发送 SET_PROPERTY 命令。
 *
 * @param GROUP       属性分组。
 * @param NUM_PROPS   要设置的属性个数。这些属性按子属性地址必须递增排列。
 *                    一条命令最多可以设置 12 个属性。
 * @param START_PROP  起始子属性地址。
 */
#ifdef __C51__
#pragma maxargs (13)  /* 允许 13 个字节的参数 */
#endif
void wf446x_set_property( U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... )
{
    va_list argList;
    U8 cmdIndex;

    Pro2Cmd[0] = WF446X_CMD_ID_SET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    va_start (argList, START_PROP);
    cmdIndex = 4;
    while(NUM_PROPS--)
    {
        /* C 的默认实参提升会把 U8 提升为 int；按 int 取再截断，避免未定义行为 */
        Pro2Cmd[cmdIndex] = (U8)va_arg (argList, int);
        cmdIndex++;
    }
    va_end(argList);

    radio_comm_SendCmd( cmdIndex, Pro2Cmd );
}

/*!
 * 向芯片发送切换状态（CHANGE_STATE）命令。
 *
 * @param NEXT_STATE1 目标状态。
 */
void wf446x_change_state(U8 NEXT_STATE1)
{
    Pro2Cmd[0] = WF446X_CMD_ID_CHANGE_STATE;
    Pro2Cmd[1] = NEXT_STATE1;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_CHANGE_STATE, Pro2Cmd );
}


#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
/* 扩展版驱动支持函数 */
/*!
 * 向芯片发送 NOP 命令，可用来维持 SPI 通信（喂狗/保活）。
 */
void wf446x_nop(void)
{
    Pro2Cmd[0] = WF446X_CMD_ID_NOP;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_NOP, Pro2Cmd );
}

/*!
 * 向芯片发送 FIFO_INFO 命令，可选择同时复位 TX/RX FIFO。
 * 芯片的应答会被读入 @Wf446xCmd。
 *
 * @param FIFO  RX/TX FIFO 复位标志。
 */
void wf446x_fifo_info(U8 FIFO)
{
    Pro2Cmd[0] = WF446X_CMD_ID_FIFO_INFO;
    Pro2Cmd[1] = FIFO;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_FIFO_INFO,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_FIFO_INFO,
                              Pro2Cmd );

    Wf446xCmd.FIFO_INFO.RX_FIFO_COUNT   = Pro2Cmd[0];
    Wf446xCmd.FIFO_INFO.TX_FIFO_SPACE   = Pro2Cmd[1];
}

/*!
 * 本函数可用于把数据写入 TX FIFO。
 *
 * @param numBytes  要写入的数据长度。
 * @param pTxData   指向数据缓冲区的指针（U8*）。
 */
void wf446x_write_tx_fifo(U8 numBytes, U8* pTxData)
{
  radio_comm_WriteData( WF446X_CMD_ID_WRITE_TX_FIFO, 0, numBytes, pTxData );
}

/*!
 * 从芯片的 RX FIFO 中读出数据。
 *
 * @param numBytes  要读取的数据长度。
 * @param pRxData   指向接收缓冲区的指针。
 */
void wf446x_read_rx_fifo(U8 numBytes, U8* pRxData)
{
  radio_comm_ReadData( WF446X_CMD_ID_READ_RX_FIFO, 0, numBytes, pRxData );
}

/*!
 * 从芯片读取属性值，读回的数据放入 Wf446xCmd 联合体。
 *
 * @param GROUP       属性分组号。
 * @param NUM_PROPS   要读取的属性个数。
 * @param START_PROP  起始子属性编号。
 */
void wf446x_get_property(U8 GROUP, U8 NUM_PROPS, U8 START_PROP)
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_PROPERTY,
                              Pro2Cmd,
                              Pro2Cmd[2],
                              Pro2Cmd );

    Wf446xCmd.GET_PROPERTY.DATA0    = Pro2Cmd[0];
    Wf446xCmd.GET_PROPERTY.DATA1    = Pro2Cmd[1];
    Wf446xCmd.GET_PROPERTY.DATA2    = Pro2Cmd[2];
    Wf446xCmd.GET_PROPERTY.DATA3    = Pro2Cmd[3];
    Wf446xCmd.GET_PROPERTY.DATA4    = Pro2Cmd[4];
    Wf446xCmd.GET_PROPERTY.DATA5    = Pro2Cmd[5];
    Wf446xCmd.GET_PROPERTY.DATA6    = Pro2Cmd[6];
    Wf446xCmd.GET_PROPERTY.DATA7    = Pro2Cmd[7];
    Wf446xCmd.GET_PROPERTY.DATA8    = Pro2Cmd[8];
    Wf446xCmd.GET_PROPERTY.DATA9    = Pro2Cmd[9];
    Wf446xCmd.GET_PROPERTY.DATA10   = Pro2Cmd[10];
    Wf446xCmd.GET_PROPERTY.DATA11   = Pro2Cmd[11];
    Wf446xCmd.GET_PROPERTY.DATA12   = Pro2Cmd[12];
    Wf446xCmd.GET_PROPERTY.DATA13   = Pro2Cmd[13];
    Wf446xCmd.GET_PROPERTY.DATA14   = Pro2Cmd[14];
    Wf446xCmd.GET_PROPERTY.DATA15   = Pro2Cmd[15];
}


#ifdef RADIO_DRIVER_FULL_SUPPORT
/* 完整版驱动支持函数 */

/*!
 * 向芯片发送 FUNC_INFO 命令，并把应答读入 @Wf446xCmd 联合体。
 */
void wf446x_func_info(void)
{
    Pro2Cmd[0] = WF446X_CMD_ID_FUNC_INFO;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_FUNC_INFO,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_FUNC_INFO,
                              Pro2Cmd );

    Wf446xCmd.FUNC_INFO.REVEXT          = Pro2Cmd[0];
    Wf446xCmd.FUNC_INFO.REVBRANCH       = Pro2Cmd[1];
    Wf446xCmd.FUNC_INFO.REVINT          = Pro2Cmd[2];
    Wf446xCmd.FUNC_INFO.PATCH.U8[MSB]   = Pro2Cmd[3];
    Wf446xCmd.FUNC_INFO.PATCH.U8[LSB]   = Pro2Cmd[4];
    Wf446xCmd.FUNC_INFO.FUNC            = Pro2Cmd[5];
}

/*!
 * 从 A 寄存器开始读快速响应寄存器（FRR），结果放入 @Wf446xCmd 联合体。
 *
 * @param respByteCount 要读取的快速响应寄存器个数。
 */
void wf446x_frr_a_read(U8 respByteCount)
{
    radio_comm_ReadData(WF446X_CMD_ID_FRR_A_READ,
                            0,
                        respByteCount,
                        Pro2Cmd);

    Wf446xCmd.FRR_A_READ.FRR_A_VALUE = Pro2Cmd[0];
    Wf446xCmd.FRR_A_READ.FRR_B_VALUE = Pro2Cmd[1];
    Wf446xCmd.FRR_A_READ.FRR_C_VALUE = Pro2Cmd[2];
    Wf446xCmd.FRR_A_READ.FRR_D_VALUE = Pro2Cmd[3];
}

/*!
 * 从 B 寄存器开始读快速响应寄存器（FRR），结果放入 @Wf446xCmd 联合体。
 *
 * @param respByteCount 要读取的快速响应寄存器个数。
 */
void wf446x_frr_b_read(U8 respByteCount)
{
    radio_comm_ReadData(WF446X_CMD_ID_FRR_B_READ,
                            0,
                        respByteCount,
                        Pro2Cmd);

    Wf446xCmd.FRR_B_READ.FRR_B_VALUE = Pro2Cmd[0];
    Wf446xCmd.FRR_B_READ.FRR_C_VALUE = Pro2Cmd[1];
    Wf446xCmd.FRR_B_READ.FRR_D_VALUE = Pro2Cmd[2];
    Wf446xCmd.FRR_B_READ.FRR_A_VALUE = Pro2Cmd[3];
}

/*!
 * 从 C 寄存器开始读快速响应寄存器（FRR），结果放入 @Wf446xCmd 联合体。
 *
 * @param respByteCount 要读取的快速响应寄存器个数。
 */
void wf446x_frr_c_read(U8 respByteCount)
{
    radio_comm_ReadData(WF446X_CMD_ID_FRR_C_READ,
                            0,
                        respByteCount,
                        Pro2Cmd);

    Wf446xCmd.FRR_C_READ.FRR_C_VALUE = Pro2Cmd[0];
    Wf446xCmd.FRR_C_READ.FRR_D_VALUE = Pro2Cmd[1];
    Wf446xCmd.FRR_C_READ.FRR_A_VALUE = Pro2Cmd[2];
    Wf446xCmd.FRR_C_READ.FRR_B_VALUE = Pro2Cmd[3];
}

/*!
 * 从 D 寄存器开始读快速响应寄存器（FRR），结果放入 @Wf446xCmd 联合体。
 *
 * @param respByteCount 要读取的快速响应寄存器个数。
 */
void wf446x_frr_d_read(U8 respByteCount)
{
    radio_comm_ReadData(WF446X_CMD_ID_FRR_D_READ,
                            0,
                        respByteCount,
                        Pro2Cmd);

    Wf446xCmd.FRR_D_READ.FRR_D_VALUE = Pro2Cmd[0];
    Wf446xCmd.FRR_D_READ.FRR_A_VALUE = Pro2Cmd[1];
    Wf446xCmd.FRR_D_READ.FRR_B_VALUE = Pro2Cmd[2];
    Wf446xCmd.FRR_D_READ.FRR_C_VALUE = Pro2Cmd[3];
}

/*!
 * 从芯片读取 ADC 数值，结果放入 @Wf446xCmd 联合体。
 *
 * @param ADC_EN  ADC 使能参数。
 */
void wf446x_get_adc_reading(U8 ADC_EN)
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_ADC_READING;
    Pro2Cmd[1] = ADC_EN;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_ADC_READING,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GET_ADC_READING,
                              Pro2Cmd );

    Wf446xCmd.GET_ADC_READING.GPIO_ADC.U8[MSB]      = Pro2Cmd[0];
    Wf446xCmd.GET_ADC_READING.GPIO_ADC.U8[LSB]      = Pro2Cmd[1];
    Wf446xCmd.GET_ADC_READING.BATTERY_ADC.U8[MSB]   = Pro2Cmd[2];
    Wf446xCmd.GET_ADC_READING.BATTERY_ADC.U8[LSB]   = Pro2Cmd[3];
    Wf446xCmd.GET_ADC_READING.TEMP_ADC.U8[MSB]      = Pro2Cmd[4];
    Wf446xCmd.GET_ADC_READING.TEMP_ADC.U8[LSB]      = Pro2Cmd[5];
    Wf446xCmd.GET_ADC_READING.TEMP_SLOPE            = Pro2Cmd[6];
    Wf446xCmd.GET_ADC_READING.TEMP_INTERCEPT        = Pro2Cmd[7];
}

/*!
 * 从芯片获取当前包的信息，也可用来在收包过程中修改包处理器属性。
 *
 * @param FIELD_NUMBER_MASK 包字段编号掩码。
 * @param LEN               长度值。
 * @param DIFF_LEN          差值长度。
 */
void wf446x_get_packet_info(U8 FIELD_NUMBER_MASK, U16 LEN, S16 DIFF_LEN )
{
    Pro2Cmd[0] = WF446X_CMD_ID_PACKET_INFO;
    Pro2Cmd[1] = FIELD_NUMBER_MASK;
    Pro2Cmd[2] = (U8)(LEN >> 8);
    Pro2Cmd[3] = (U8)(LEN);
    // 这个差值是"字节差"，虽然带符号，但命令处理端
    // 完全可以按无符号来对待
    Pro2Cmd[4] = (U8)((U16)DIFF_LEN >> 8);
    Pro2Cmd[5] = (U8)(DIFF_LEN);

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_PACKET_INFO,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_PACKET_INFO,
                              Pro2Cmd );

    Wf446xCmd.PACKET_INFO.LENGTH_15_8   = Pro2Cmd[0];
    Wf446xCmd.PACKET_INFO.LENGTH_7_0    = Pro2Cmd[1];
}

/*!
 * 获取包处理器（Packet Handler）的状态标志，可按需要清除。
 *
 * @param PH_CLR_PEND 要清除的标志位。
 */
void wf446x_get_ph_status(U8 PH_CLR_PEND)
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_PH_STATUS;
    Pro2Cmd[1] = PH_CLR_PEND;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_PH_STATUS,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GET_PH_STATUS,
                              Pro2Cmd );

    Wf446xCmd.GET_PH_STATUS.PH_PEND        = Pro2Cmd[0];
    Wf446xCmd.GET_PH_STATUS.PH_STATUS      = Pro2Cmd[1];
}

/*!
 * 获取调制解调器（Modem）状态标志，可按需要清除。
 *
 * @param MODEM_CLR_PEND 要清除的标志位。
 */
void wf446x_get_modem_status( U8 MODEM_CLR_PEND )
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_MODEM_STATUS;
    Pro2Cmd[1] = MODEM_CLR_PEND;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_MODEM_STATUS,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GET_MODEM_STATUS,
                              Pro2Cmd );

    Wf446xCmd.GET_MODEM_STATUS.MODEM_PEND   = Pro2Cmd[0];
    Wf446xCmd.GET_MODEM_STATUS.MODEM_STATUS = Pro2Cmd[1];
    Wf446xCmd.GET_MODEM_STATUS.CURR_RSSI    = Pro2Cmd[2];
    Wf446xCmd.GET_MODEM_STATUS.LATCH_RSSI   = Pro2Cmd[3];
    Wf446xCmd.GET_MODEM_STATUS.ANT1_RSSI    = Pro2Cmd[4];
    Wf446xCmd.GET_MODEM_STATUS.ANT2_RSSI    = Pro2Cmd[5];
    Wf446xCmd.GET_MODEM_STATUS.AFC_FREQ_OFFSET.U8[MSB]  = Pro2Cmd[6];
    Wf446xCmd.GET_MODEM_STATUS.AFC_FREQ_OFFSET.U8[LSB]  = Pro2Cmd[7];
}

/*!
 * 获取芯片状态标志，可按需要清除。
 *
 * @param CHIP_CLR_PEND 要清除的标志位。
 */
void wf446x_get_chip_status( U8 CHIP_CLR_PEND )
{
    Pro2Cmd[0] = WF446X_CMD_ID_GET_CHIP_STATUS;
    Pro2Cmd[1] = CHIP_CLR_PEND;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_GET_CHIP_STATUS,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_GET_CHIP_STATUS,
                              Pro2Cmd );

    Wf446xCmd.GET_CHIP_STATUS.CHIP_PEND         = Pro2Cmd[0];
    Wf446xCmd.GET_CHIP_STATUS.CHIP_STATUS       = Pro2Cmd[1];
    Wf446xCmd.GET_CHIP_STATUS.CMD_ERR_STATUS    = Pro2Cmd[2];
}

/*!
 * 执行镜像抑制校准（Image Rejection Calibration）。可以通过轮询 CTS，
 * 或者等待 CHIP_READY 中断源来判断校准是否完成。
 *
 * @param SEARCHING_STEP_SIZE
 * @param SEARCHING_RSSI_AVG
 * @param RX_CHAIN_SETTING1
 * @param RX_CHAIN_SETTING2
 */
void wf446x_ircal(U8 SEARCHING_STEP_SIZE, U8 SEARCHING_RSSI_AVG, U8 RX_CHAIN_SETTING1, U8 RX_CHAIN_SETTING2)
{
    Pro2Cmd[0] = WF446X_CMD_ID_IRCAL;
    Pro2Cmd[1] = SEARCHING_STEP_SIZE;
    Pro2Cmd[2] = SEARCHING_RSSI_AVG;
    Pro2Cmd[3] = RX_CHAIN_SETTING1;
    Pro2Cmd[4] = RX_CHAIN_SETTING2;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_IRCAL,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_IRCAL,
                              Pro2Cmd );

    Wf446xCmd.IRCAL.CAL_STATE   = Pro2Cmd[0];
    Wf446xCmd.IRCAL.RSSI        = Pro2Cmd[1];
    Wf446xCmd.IRCAL.DIR_CH      = Pro2Cmd[2];
}

/*!
 * 把芯片配置成指定的协议格式。
 *
 * @param PROTOCOL    0 = 通用包格式，不会动态重新配置包处理器的属性。
                      1 = IEEE802.15.4g 兼容包格式。以下属性会被强制覆盖：
                      PKT_CRC_CONFIG，TX/RX 在 PKT_CONFG1 中的 CRC_ENDIAN/BIT_ORDER，RX 的 PKT_FIELD_1_CRC_CONFIG。
                      包处理器分组中其它相关属性仍然需要自行写入。Field 1 的长度应设为 16 位，
                      以便容纳 PHR；RX 时需把 PKT_LEN_FIELD_SOURCE 置 1。PSDU 应使用变长的 Field 2，
                      Field 2 的长度要设为允许的最大值（包含预期的 FCS 长度）。这里假定 FCS 由主机计算
                      并随数据一起在空中传输：Wf4440 只接收 PHR，并把 FCS 放进 FIFO 供主机读取和校验，
                      因此 Wf4440 上不应启用 CRC。
 */
void wf446x_protocol_cfg(U8 PROTOCOL)
{
    Pro2Cmd[0] = WF446X_CMD_ID_PROTOCOL_CFG;
    Pro2Cmd[1] = PROTOCOL;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_PROTOCOL_CFG, Pro2Cmd );
}

/*!
 * 查询芯片当前状态，同时列出挂起中的 TX / RX 请求
 */
void wf446x_request_device_state(void)
{
    Pro2Cmd[0] = WF446X_CMD_ID_REQUEST_DEVICE_STATE;

    radio_comm_SendCmdGetResp( WF446X_CMD_ARG_COUNT_REQUEST_DEVICE_STATE,
                              Pro2Cmd,
                              WF446X_CMD_REPLY_COUNT_REQUEST_DEVICE_STATE,
                              Pro2Cmd );

    Wf446xCmd.REQUEST_DEVICE_STATE.CURR_STATE       = Pro2Cmd[0];
    Wf446xCmd.REQUEST_DEVICE_STATE.CURRENT_CHANNEL  = Pro2Cmd[1];
}

/*!
 * 在 RX 状态下，按参数指定的频率跳频，并重新开始搜索前导码。
 *
 * @param INTE      新的 INTE 寄存器值。
 * @param FRAC2     新的 FRAC2 寄存器值。
 * @param FRAC1     新的 FRAC1 寄存器值。
 * @param FRAC0     新的 FRAC0 寄存器值。
 * @param VCO_CNT1  新的 VCO_CNT1 寄存器值。
 * @param VCO_CNT0  新的 VCO_CNT0 寄存器值。
 */
void wf446x_rx_hop(U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0)
{
    Pro2Cmd[0] = WF446X_CMD_ID_RX_HOP;
    Pro2Cmd[1] = INTE;
    Pro2Cmd[2] = FRAC2;
    Pro2Cmd[3] = FRAC1;
    Pro2Cmd[4] = FRAC0;
    Pro2Cmd[5] = VCO_CNT1;
    Pro2Cmd[6] = VCO_CNT0;

    radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_RX_HOP, Pro2Cmd );
}

/*!
 * 本命令用于覆盖 AGC（自动增益控制）的设置。
 *
 * @param AGC_OVERRIDE  AGC 覆盖参数。
 */
void wf446x_agc_override(U8 AGC_OVERRIDE)
{
  Pro2Cmd[0] = WF446X_CMD_ID_AGC_OVERRIDE;
  Pro2Cmd[1] = AGC_OVERRIDE;

  radio_comm_SendCmd( WF446X_CMD_ARG_COUNT_AGC_OVERRIDE, Pro2Cmd );
}

#endif /* RADIO_DRIVER_FULL_SUPPORT */
void reConfigPktHandler(void)
{
  Pro2Cmd[0] = 0x11;
  Pro2Cmd[1] = 0x12;
  Pro2Cmd[2] = 0x0C;
  Pro2Cmd[3] = 0x08;
  Pro2Cmd[4] = 0x22;
  Pro2Cmd[5] = 0x01;
  Pro2Cmd[6] = 0x00;
  Pro2Cmd[7] = 0x30;
  Pro2Cmd[8] = 0x30;
  Pro2Cmd[9] = 0x00;
  Pro2Cmd[10] = 0x01;
  Pro2Cmd[11] = 0x04;
  Pro2Cmd[12] = 0x80;
  Pro2Cmd[13] = 0x1F;
  Pro2Cmd[14] = 0xFF;
  Pro2Cmd[15] = 0x00;


  radio_comm_SendCmd( 0x10, Pro2Cmd );
}


#endif /* RADIO_DRIVER_EXTENDED_SUPPORT */
