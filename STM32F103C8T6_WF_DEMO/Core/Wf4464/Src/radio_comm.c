/*!
 * 文件：
 *  radio_comm.c
 *
 * 说明：
 *  本文件中包含无线芯片（RADIO）通信层的实现。
 *
 * Silicon Laboratories 机密资料
 * 版权所有 2012 Silicon Laboratories, Inc.
 */


#include "compiler_defs.h"
#include "wf446x_defs.h"
#include "radio_comm.h"
#include "radio_hal.h"


BIT ctsWentHigh = 0;


/*!
 * 从无线芯片读取命令的应答数据
 *
 * @param byteCount     要从芯片读取的字节数
 * @param pData         存放读回数据的缓冲区指针
 *
 * @return CTS 的值
 */
U8 radio_comm_GetResp(U8 byteCount, U8* pData)
{
  U8 ctsVal = 0;
  U16 errCnt = RADIO_CTS_TIMEOUT;
  //U16 u16Loop = 0;
  while (errCnt != 0)      //循环等待，直到芯片准备好返回数据
  {
    radio_hal_ClearNsel();
    //for(u16Loop = 0; u16Loop < 100; u16Loop ++);
    radio_hal_SpiWriteByte(0x44);    //读命令缓冲区
    ctsVal = radio_hal_SpiReadByte();
    if (ctsVal == 0xFF)
    {
      if (byteCount)
      {
        radio_hal_SpiReadData(byteCount, pData);
      }
      radio_hal_SetNsel();
      break;
    }
    radio_hal_SetNsel();
    errCnt--;
  }

  if (errCnt == 0)
  {
    #ifdef RADIO_COMM_ERROR_CALLBACK
      RADIO_COMM_ERROR_CALLBACK();
    #endif
    ctsWentHigh = 1;
    return 0x00;
  }

  if (ctsVal == 0xFF)
  {
    ctsWentHigh = 1;
  }

  return ctsVal;
}

/*!
 * 向无线芯片发送一条命令
 *
 * @param byteCount     要发送的命令字节数
 * @param pData         指向要发送的命令数据的指针
 */
void radio_comm_SendCmd(U8 byteCount, U8* pData)
{
    /* A1 版本芯片有个 bug：处理不了单字节命令。
       据说 B0 已经修好，但最后一刻没能进版本，所以这里还得再补一次 */
    if (byteCount == 1)
        byteCount++;

    while (!ctsWentHigh)
    {
        radio_comm_PollCTS();
    }
    radio_hal_ClearNsel();
    radio_hal_SpiWriteData(byteCount, pData);
    radio_hal_SetNsel();
    ctsWentHigh = 0;
}

/*!
 * 从无线芯片读取命令的应答数据
 *
 * @param cmd           命令 ID
 * @param pollCts       是否先轮询 CTS
 * @param byteCount     要从芯片读取的字节数
 * @param pData         存放读回数据的缓冲区指针
 */
void radio_comm_ReadData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData)
{
    if(pollCts)
    {
        while(!ctsWentHigh)
        {
            radio_comm_PollCTS();
        }
    }
    radio_hal_ClearNsel();
    radio_hal_SpiWriteByte(cmd);
    radio_hal_SpiReadData(byteCount, pData);
    radio_hal_SetNsel();
    ctsWentHigh = 0;
}


/*!
 * 向无线芯片写入数据（命令 + 数据一起发）
 *
 * @param cmd           命令 ID
 * @param pollCts       是否先轮询 CTS
 * @param byteCount     要写入的字节数
 * @param pData         指向要写入的数据的指针
 */
void radio_comm_WriteData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData)
{
    if(pollCts)
    {
        while(!ctsWentHigh)
        {
            radio_comm_PollCTS();
        }
    }
    radio_hal_ClearNsel();
    radio_hal_SpiWriteByte(cmd);
    radio_hal_SpiWriteData(byteCount, pData);
    radio_hal_SetNsel();
    ctsWentHigh = 0;
}

/*!
 * 等待 CTS 变为高电平（芯片就绪）
 *
 * @return CTS 的值
 */
U8 radio_comm_PollCTS(void)
{
    return radio_comm_GetResp(0, 0);
}

/**
 * 清除 CTS 状态标志变量。
 */
void radio_comm_ClearCTS()
{
  ctsWentHigh = 0;
}


/*!
 * 发送一条命令并读取应答
 *
 * @param cmdByteCount  要发送的命令字节数
 * @param pCmdData      指向命令数据的指针
 * @param respByteCount 要读取的应答字节数
 * @param pRespData     存放应答数据的缓冲区指针
 *
 * @return CTS 的值
 */
U8 radio_comm_SendCmdGetResp(U8 cmdByteCount, U8* pCmdData, U8 respByteCount, U8* pRespData)
{
    radio_comm_SendCmd(cmdByteCount, pCmdData);
    return radio_comm_GetResp(respByteCount, pRespData);
}

