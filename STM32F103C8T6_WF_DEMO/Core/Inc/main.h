/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CHOOSE_MODE_Pin GPIO_PIN_2
#define CHOOSE_MODE_GPIO_Port GPIOA
#define SDN_Pin GPIO_PIN_1
#define SDN_GPIO_Port GPIOB
#define IRQ_Pin GPIO_PIN_10
#define IRQ_GPIO_Port GPIOB
#define IRQ_EXTI_IRQn EXTI15_10_IRQn
#define SPI_CS_Pin GPIO_PIN_12
#define SPI_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* ---- 用户按键 S2 / S3 ----------------------------------------------------
 * 硬件：S2 - PB6，S3 - PB7，按下接地（低电平有效），引脚内部上拉
 * 说明：若在 CubeMX 里也把这两个脚配成 GPIO_Input + Pull-up，
 *       重新生成的宏定义与此处取值完全相同，不会冲突。
 * ------------------------------------------------------------------------ */
#define S2_Pin            GPIO_PIN_6
#define S2_GPIO_Port      GPIOB
#define S3_Pin            GPIO_PIN_7
#define S3_GPIO_Port      GPIOB

/* ---- LED1 ---------------------------------------------------------------
 * 硬件：PA15，低电平点亮（串限流电阻到 3V3）。
 * 说明：PA15 复位后是 JTAG 的 JTDI，工程里 HAL_MspInit() 已调用
 *       __HAL_AFIO_REMAP_SWJ_NOJTAG() 把它释放成 GPIO，SWD 调试不受影响。
 * ------------------------------------------------------------------------ */
#define LED1_Pin          GPIO_PIN_15
#define LED1_GPIO_Port    GPIOA

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
