/*!
 * 文件：
 *  radio_comm.h
 *
 * 说明：
 *  本文件中包含无线芯片（RADIO）通信层。
 *
 * Silicon Laboratories 机密资料
 * 版权所有 2011 Silicon Laboratories, Inc.
 */
#ifndef _RADIO_COMM_H_
#define _RADIO_COMM_H_

#include "compiler_defs.h"

//#define RADIO_CTS_TIMEOUT 255
#define RADIO_CTS_TIMEOUT 10000

extern U8 radioCmd[16u];

U8 radio_comm_GetResp(U8 byteCount, U8* pData);/*读应答*/
void radio_comm_SendCmd(U8 byteCount, U8* pData);/*只发命令，不读应答*/
void radio_comm_ReadData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData);
void radio_comm_WriteData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData);

U8 radio_comm_PollCTS(void);/*本质是 GetResp(0, 0)，只问“好了没”，不取数据*/
U8 radio_comm_SendCmdGetResp(U8 cmdByteCount, U8* pCmdData, \
                             U8 respByteCount, U8* pRespData);
void radio_comm_ClearCTS(void);

#endif //_RADIO_COMM_H_
