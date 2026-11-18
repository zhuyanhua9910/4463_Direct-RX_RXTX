/**
 * @file    wf4463_port.h
 * @brief   Wf4463 移植配置：SPI 实例、引脚、超时
 *
 * 目标板：STM32F103C8T6
 *   SPI2  : PB13 SCK / PB14 MISO / PB15 MOWF（AF0，Mode 0，3 MHz）
 *   nSEL  : PB12（输出，初值高）
 *   SDN   : PB1 （输出，初值低，低=工作）
 *   nIRQ  : PB10（上拉输入 / EXTI 下降沿）
 *
 * 换板子/换 SPI：只改这个文件。
 */

#ifndef WF4463_PORT_H
#define WF4463_PORT_H

/* ===== 1. MCU 系列头文件 ===== */
#include "stm32f1xx_hal.h"

/* ===== 2. SPI 句柄 ===== */
extern SPI_HandleTypeDef hspi2;
#define WF4463_SPI                (&hspi2)

/* ===== 3. 引脚 ===== */
#define WF4463_NSEL_PORT          GPIOB
#define WF4463_NSEL_PIN           GPIO_PIN_12

#define WF4463_SDN_PORT           GPIOB
#define WF4463_SDN_PIN            GPIO_PIN_1

#define WF4463_NIRQ_PORT          GPIOB
#define WF4463_NIRQ_PIN           GPIO_PIN_10

/* ===== 4. 时序参数（物理时间，和主频无关） ===== */
#define WF4463_SPI_TIMEOUT_MS     10u   /* 单次 SPI 传输超时 */
#define WF4463_SDN_HIGH_MS        20u   /* 手册要求 >=10us，这里给足 */
#define WF4463_POR_MS             100u  /* SDN 释放后等 POR + 内部校准
                                         * 原来 20ms：冷启动后约 10 秒内 TX 会一直返回 -3
                                         * （START_TX 成功但等不到 PACKET_SENT），之后自愈。
                                         * 先把稳定时间给足，再看是否还复发。 */

/* ===== 5. 应用层参数 ===== */
#define WF4463_MAX_PAYLOAD        63u   /* FIFO 64 字节，变长包首字节占 1 */
#define WF4463_TX_TIMEOUT_MS      1000u /* 等 TX 完成中断 */
#define WF4463_RX_TIMEOUT_MS      0u    /* 0 = 一直等 */
#define WF4463_TX_DONE_STATE      0x30u /* 发完回 READY；0x00 = 保持 TX */

/* 没接 nIRQ 时打开这个宏：驱动改为轮询中断状态寄存器 */
/* #define WF4463_NO_NIRQ_PIN */

#endif /* WF4463_PORT_H */
