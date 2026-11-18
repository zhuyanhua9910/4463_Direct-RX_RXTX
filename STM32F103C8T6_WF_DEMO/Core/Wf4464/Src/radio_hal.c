/**
 * @file    radio_hal.c
 * @brief   Wf446x 库的硬件适配层 —— STM32 HAL 实现
 *
 * 库（wf446x_api_lib.c / radio_comm.c）通过下面这几个函数访问硬件。
 * 你只需要实现这一层，协议、命令、CTS 轮询逻辑全部照用。
 *
 */

#include "compiler_defs.h"
#include "wf4463_port.h"
#include "radio_hal.h"

/* ---------------------------------------------------------------- */
/* 片选（nSEL，低有效）                                              */
/* ---------------------------------------------------------------- */
void radio_hal_ClearNsel(void)
{
    HAL_GPIO_WritePin(WF4463_NSEL_PORT, WF4463_NSEL_PIN, GPIO_PIN_RESET);
}

void radio_hal_SetNsel(void)
{
    HAL_GPIO_WritePin(WF4463_NSEL_PORT, WF4463_NSEL_PIN, GPIO_PIN_SET);
}

/* ---------------------------------------------------------------- */
/* 关断脚（SDN，高 = 关断）                                          */
/* ---------------------------------------------------------------- */
void radio_hal_AssertShutdown(void)
{
    HAL_GPIO_WritePin(WF4463_SDN_PORT, WF4463_SDN_PIN, GPIO_PIN_SET);
}

void radio_hal_DeassertShutdown(void)
{
    HAL_GPIO_WritePin(WF4463_SDN_PORT, WF4463_SDN_PIN, GPIO_PIN_RESET);
}

/* ---------------------------------------------------------------- */
/* 中断脚：返回 TRUE = nIRQ 为高（无中断），FALSE = nIRQ 为低（有中断）*/
/* 返回引脚电平。*/
/* ---------------------------------------------------------------- */
BIT radio_hal_NirqLevel(void)
{
    return (HAL_GPIO_ReadPin(WF4463_NIRQ_PORT, WF4463_NIRQ_PIN) == GPIO_PIN_SET)
           ? (BIT)TRUE : (BIT)FALSE;
}

/* ---------------------------------------------------------------- */
/* SPI 收发                                                          */
/* 注意：CubeMX 里 SPI 必须配成                                          */
/*   Mode 0（CPOL = Low, CPHA = 1 Edge）、MSB first、8 bit、NSS = Software*/
/*   时钟 ≤ 10 MHz（Wf4463 上限），建议先用 1–4 MHz 调通                */
/* ---------------------------------------------------------------- */
U8 SPI_RW(U8 byte)
{
    U8 rx = 0xFF;

    if (HAL_SPI_TransmitReceive(WF4463_SPI, &byte, &rx, 1,
                                WF4463_SPI_TIMEOUT_MS) != HAL_OK)
    {
        return 0xFF;   /* 失败时返回 0xFF，调用方的 CTS 轮询会超时并报错 */
    }
    return rx;
}

void radio_hal_SpiWriteByte(U8 byteToWrite)
{
    (void)SPI_RW(byteToWrite);
}

U8 radio_hal_SpiReadByte(void)
{
    return SPI_RW(0xFF);          /* 读的时候发 0xFF（dummy） */
}

/* 连续写：用 HAL 的块传输，比逐字节快很多（写 FIFO 时差别明显） */
void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
    if ((byteCount == 0) || (pData == NULL))
    {
        return;
    }
    (void)HAL_SPI_Transmit(WF4463_SPI, pData, (uint16_t)byteCount,
                           WF4463_SPI_TIMEOUT_MS);
}

/* 连续读：HAL 会在发送端自动填 0xFF，正好符合 Wf4463 的读时序 */
void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
    if ((byteCount == 0) || (pData == NULL))
    {
        return;
    }
    (void)HAL_SPI_Receive(WF4463_SPI, pData, (uint16_t)byteCount,
                          WF4463_SPI_TIMEOUT_MS);
}
