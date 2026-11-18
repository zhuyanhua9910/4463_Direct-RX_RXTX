/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Wf4463 无线收发
 *                   发送端 / 接收端 由 CHOOSE_MODE 引脚电平切换（高=TX，低=RX）
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include "wf4463_app.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* ============================================================
 * 角色选择（硬件跳线，单脚）：
 *   CHOOSE_MODE 高电平 → 发送端 (TX)
 *   CHOOSE_MODE 低电平 → 接收端 (RX)
 *   引脚内部上拉，悬空默认即为高电平 = 发送端
 *
 * 引脚由 main.h 中的 CHOOSE_MODE_Pin / CHOOSE_MODE_GPIO_Port
 * 决定，换板换脚只改 main.h，无需改本文件。
 * ============================================================ */
#define ROLE_TX           1
#define ROLE_RX           2

#define APP_PACKET_LEN    12        /* "WF HELLO 001" = 12 字节 */
#define APP_CHANNEL       0

//应答帧定义
#define ACK_HEAD_OK  0xA5  /*帧头：接收成功*/
#define ACK_HEAD_FAIL 0x5A /*帧头：接收失败*/
#define ACK_SEQ_OFS 9 /*WF  HELLO,共九字节，序号从第九字节开始*/
#define ACK_TIMEOUT_MS 100 /*主机等应答的时间*/
#define ACK_ERR_LEN 0x01 /*错误码：长度/格式错误*/
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint8_t  txBuf[APP_PACKET_LEN + 1];   /* +1 给字符串结束符 */
static uint8_t  rxBuf[64];
static uint16_t txSeq = 0;
static uint8_t  gRole = ROLE_RX;             /* 当前角色：ROLE_TX / ROLE_RX */
static uint8_t ackBuf[8];/*应答帧缓冲区*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static uint8_t Role_Detect(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* ---- 角色选择：CHOOSE_MODE 高 = 发送，低 = 接收 ----
   * CHOOSE_MODE 已在 MX_GPIO_Init() 中配置为输入 + 上拉，这里只需读取 */
  gRole = Role_Detect();

  printf("ROLE pin: CHOOSE_MODE=%d -> %s\r\n",
         (int)HAL_GPIO_ReadPin(CHOOSE_MODE_GPIO_Port, CHOOSE_MODE_Pin),
         (gRole == ROLE_TX) ? "TRANSMITTER" : "RECEIVER");

  printf("\r\n===== Wf4463 %s =====\r\n",
         (gRole == ROLE_TX) ? "TRANSMITTER" : "RECEIVER");

  int8_t rc = wf4463_init();
  printf("wf4463_init rc=%d\r\n", rc);
  if (rc != WF4463_OK)
  {
      printf("init failed, check wiring/power\r\n");
      while (1) { HAL_Delay(1000); }
  }

  wf4463_print_info();

  wf4463_diag_t diag;
  if (wf4463_diag(&diag) == WF4463_OK)
  {
      printf("DIAG cts=0x%02X part=0x%04X rev=0x%02X xo=0x%02X pa=0x%02X state=%u rssi=%u\r\n",
             diag.cts, diag.part, diag.chiprev, diag.xo_tune, diag.pa_pwr,
             diag.state, diag.rssi_raw);
  }


  if (gRole == ROLE_RX)
  {
      rc = wf4463_rx_start(APP_CHANNEL);
      printf("rx_start rc=%d\r\n", rc);
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (gRole == ROLE_TX)
    {
        /* ================= 发送端 ================= */
        static uint32_t nextTx = 0;
        uint32_t now = HAL_GetTick();

        if ((int32_t)(now - nextTx) >= 0)
        {
            txSeq++;
            if (txSeq > 999) { txSeq = 1; }   /* 序号循环 001~999，保证包长固定 */

            /* 负载：WF HELLO 001 / 002 / 003 ... */
            memset(txBuf, 0, sizeof(txBuf));
            snprintf((char *)txBuf, sizeof(txBuf), "WF HELLO %03u", (unsigned)txSeq);

            printf("TX #%u > %s\r\n", txSeq, (char *)txBuf);

            int8_t txRc = wf4463_tx(APP_CHANNEL, txBuf, APP_PACKET_LEN);
            if (txRc != WF4463_OK)
            {
                printf("  tx rc=%d\r\n", txRc);
            }
            /*等待从机应答*/
            {
            	uint8_t ackLen = 0;
            	int16_t ackN;
            	wf4463_rx_start(APP_CHANNEL); /* 发完芯片在 READY，必须先切 RX */
            	ackN = wf4463_rx_fetch(ackBuf,sizeof(ackBuf),&ackLen,ACK_TIMEOUT_MS);
            	if((ackN > 0) && (ackLen == 4) && (ackBuf[0] == ACK_HEAD_OK)&&(memcmp(&ackBuf[1],&txBuf[ACK_SEQ_OFS],3) == 0))
            	{
            		printf("  ACK  OK  < 0x%02x  %c%c%c\r\n",ackBuf[0],ackBuf[1],ackBuf[2],ackBuf[3] );
            	}
            	else if((ackN > 0) && (ackLen == 5) && (ackBuf[0] == ACK_HEAD_FAIL) && (memcmp(&ackBuf[1],&txBuf[ACK_SEQ_OFS],3) == 0) )
            	{
            		printf(" ACK  FAIL code = 0x%02x\r\n ",ackBuf[4]);
            	}
            	else if(ackN == WF4463_ERR_TIMEOUT)
				{
            		printf(" ACK timeout \r\n");
				}
            	else
            	{
            		printf("  ACK question n=%d len=%u head=0x%02X\r\n",
            		                           ackN, ackLen, (ackN > 0) ? ackBuf[0] : 0);
            	}
            }
            nextTx = now + 500;
        }
    }
    else
    {
        /* ================= 接收端 ================= */
        static uint16_t rxCnt = 0;
//      static uint32_t lastHb = 0;
        uint8_t  outLen = 0;
        int16_t  n;

        n = wf4463_rx_fetch(rxBuf, sizeof(rxBuf), &outLen, 50);

        if (n > 0)
        {
        	/*先回应答，在打印日志*/
        	uint8_t ackLen = 0;
        	if((outLen >= APP_PACKET_LEN) && (memcmp(rxBuf,"WF HELLO ",9) == 0))
        	{
        		ackBuf[0] = ACK_HEAD_OK;
        		ackBuf[1] = rxBuf[ACK_SEQ_OFS + 0];
        		ackBuf[2] = rxBuf[ACK_SEQ_OFS +1 ];
        	 	ackBuf[3] = rxBuf[ACK_SEQ_OFS + 2];
        	 	ackLen = 4;
        	}
        	else
        	{
        		ackBuf[0] = ACK_HEAD_FAIL;
        		ackBuf[1] = '?';
        		ackBuf[2] = '?';
        		ackBuf[3] = '?';
        		ackBuf[4] = ACK_ERR_LEN;
        		ackLen = 5;
        	}
        	wf4463_tx(APP_CHANNEL, ackBuf,ackLen);

        	wf4463_rx_start(APP_CHANNEL);/*进入接收*/

            /* 判断是否全为可打印 ASCII，是则按文本显示，否则按十六进制 */
            bool printable = (outLen > 0);
            for (uint8_t i = 0; i < outLen; i++)
            {
                if ((rxBuf[i] < 0x20) || (rxBuf[i] > 0x7E))
                {
                    printable = false;
                    break;
                }
            }

            if (printable)
            {
                rxCnt++;
                rxBuf[outLen] = '\0';
                printf("RX #%u < \"%s\" | len=%u  | ack=%s \r\n",
                       rxCnt, (char *)rxBuf, outLen,(ackLen==4) ? "ok" : "fail");
            }
            else
            {
                printf("BAD < ");
                for (uint8_t i = 0; (i < outLen) && (i < 8); i++) { printf("%02X ", rxBuf[i]); }
                printf("| len=%u\r\n", outLen);
            }

            /* 收完一包芯片回到 READY，必须重新进入接收 */
          //  wf4463_rx_start(APP_CHANNEL);
        }
        else if (n != WF4463_ERR_TIMEOUT)
        {
            /* CRC 错 / 无包 / FIFO 异常：打印并重启接收 */
            printf("RX err %d\r\n", n);
            wf4463_rx_start(APP_CHANNEL);
        }
        else
        {
            /* 超时：确认芯片仍处于 RX，不在则重新武装 */
            uint8_t st = 0;
            wf4463_device_state(&st);
            if (st != 8u)
            {
                wf4463_rx_start(APP_CHANNEL);
            }
        }


    }
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* CHOOSE_MODE 的 GPIO 已由 CubeMX 的 MX_GPIO_Init() 配成输入+上拉，
 * 这里只做电平判读，不再重复初始化 */

/* 高电平 → ROLE_TX；低电平 → ROLE_RX */
static uint8_t Role_Detect(void)
{
    if (HAL_GPIO_ReadPin(CHOOSE_MODE_GPIO_Port, CHOOSE_MODE_Pin) == GPIO_PIN_SET)
    {
        return ROLE_TX;            /* CHOOSE_MODE 高 → 发送 */
    }
    return ROLE_RX;                /* CHOOSE_MODE 低 → 接收 */
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq();
    while (1) {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
