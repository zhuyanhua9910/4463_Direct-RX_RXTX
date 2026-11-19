/*!
 * 文件：
 *  radio_hal.h
 *
 * 说明：
 *  本文件中包含无线芯片的硬件抽象层（HAL）：SPI 收发、SDN/CSN/NIRQ 引脚操作。
 *
 * Silicon Laboratories 机密资料
 * 版权所有 2011 Silicon Laboratories, Inc.
 */

#ifndef _RADIO_HAL_H_
#define _RADIO_HAL_H_


void radio_hal_AssertShutdown(void);
void radio_hal_DeassertShutdown(void);
void radio_hal_ClearNsel(void);
void radio_hal_SetNsel(void);
BIT radio_hal_NirqLevel(void);

void radio_hal_SpiWriteByte(U8 byteToWrite);
U8 radio_hal_SpiReadByte(void);
U8 SPI_RW(U8);

void radio_hal_SpiWriteData(U8 byteCount, U8* pData);
void radio_hal_SpiReadData(U8 byteCount, U8* pData);

#ifdef DRIVERS_EXTENDED_SUPPORT
  BIT radio_hal_Gpio0Level(void);
  BIT radio_hal_Gpio1Level(void);
  BIT radio_hal_Gpio2Level(void);
  BIT radio_hal_Gpio3Level(void);
#endif

#endif //_RADIO_HAL_H_
