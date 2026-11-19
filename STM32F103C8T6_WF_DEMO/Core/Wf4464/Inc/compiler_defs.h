#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 官方库使用的基本类型 ---------- */
typedef _Bool    BIT;      /* 官方库把它当 0/1 使用 */
typedef uint8_t  U8;
typedef uint16_t U16;      /* ★ 关键：必须是 16 位，不能是 unsigned int */
typedef uint32_t U32;

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;

/* ---------- 官方库使用的联合体（用于 16/32 位拆分） ---------- */
typedef union UU16
{
    U16 U16;
    S16 S16;
    U8  U8[2];
    S8  S8[2];
} UU16;

typedef union UU32
{
    U32  U32;
    S32  S32;
    UU16 UU16[2];
    U16  U16[2];
    S16  S16[2];
    U8   U8[4];
    S8   S8[4];
} UU32;


#ifndef LSB
#define LSB 0
#endif
#ifndef MSB
#define MSB 1
#endif
#ifndef b0
#define b0 0
#define b1 1
#define b2 2
#define b3 3
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ---------- NOP ---------- */
#if defined(__GNUC__)
  #define NOP() __asm volatile ("nop")
#else
  #define NOP() do { } while (0)
#endif

/* ---------- 官方库的功能开关 ----------
 * 这两个宏正常应加在工程编译选项里（-D）。
 * 为了让本工程"拷进来就能编"，这里用 #ifndef 兜底：
 * 以后你在 CubeIDE 的 Defined symbols 里也加了同样的宏，不会冲突。
 *
 *   RADIO_DRIVER_EXTENDED_SUPPORT : wf446x_write_tx_fifo / read_rx_fifo / fifo_info
 *   RADIO_DRIVER_FULL_SUPPORT     : wf446x_get_modem_status（RSSI）等
 */
#ifndef RADIO_DRIVER_EXTENDED_SUPPORT
  #define RADIO_DRIVER_EXTENDED_SUPPORT
#endif
#ifndef RADIO_DRIVER_FULL_SUPPORT
  #define RADIO_DRIVER_FULL_SUPPORT
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMPILER_DEFS_H */
