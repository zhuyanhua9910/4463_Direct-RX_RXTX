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

/* 单个按键的运行时状态 */
typedef struct
{
    GPIO_TypeDef *port;        /* 端口 */
    uint16_t      pin;         /* 引脚 */
    const char   *name;        /* 打印用名字 */
    uint8_t       raw;         /* 上一次采样电平：0=按下 1=松开 */
    uint8_t       stable;      /* 消抖确认后的电平 */
    uint32_t      changeTick;  /* 采样电平最后一次变化的时刻 */
    uint32_t      downTick;    /* 本次按下的时刻（用于算按住时长） */
} Key_t;

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

/* ---------- 按键扫描参数 ---------- */
#define KEY_NUM           2     /* 按键个数：S2、S3 */
#define KEY_DEBOUNCE_MS   20    /* 消抖时间（ms），机械抖动一般 < 20ms */
#define KEY_DOWN          0    /* 归一化电平：0 = 按下（硬件低有效） */
#define KEY_UP            1   /* 归一化电平：1 = 松开 */

/* ---------- 工作模式 ---------- */
#define MODE_RAW_RX       0    /* 直收：解调比特流从 GPIO1 输出 */
#define MODE_PACKET       1    /* 双向包收发（原有逻辑） */
#define MODE_SLEEP        2    /* 上电休眠中，还没按过 S2/S3（单向，出不去回不来） */

/* ---------- 上电休眠档（编译期四选一） ----------
 * BOOT_LP_ENABLE 置 0：编译结果与改造前完全一致（上电直接直收、LED 常亮），
 *                      用于 A/B 对比或紧急回退。
 * 档位四选一：
 *   WF4463_LP_STANDBY    40 nA   SLEEP，WUT 与 32k 全关
 *   WF4463_LP_SLEEP_RC   740 nA  SLEEP + WUT + 32k 内部 RC
 *   WF4463_LP_SENSOR     ~1 µA   SLEEP + 32k RC + 低电压检测
 *   WF4463_LP_SHUTDOWN   30 nA   SDN 拉高，寄存器全丢（首次按键要冷启动 ~120 ms）
 * 本工程不用 WUT 唤醒，周期只决定"定时器多久跑一轮"，
 * 到期也留在 SLEEP 只置标志，不会自己跳回 READY(1.8 mA)。
 */
#define BOOT_LP_ENABLE     1
#define BOOT_LP_MODE       WF4463_LP_SHUTDOWN
#define BOOT_LP_WUT_MS     3600000   /* 1 小时 */

/* ---------- LED1（PA15，低电平点亮） ---------- */
#define LED_ON()          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED_OFF()         HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED_FLASH_MS      80   /* 收到包 / 收到 ACK 的短闪时长 */

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
static uint8_t ackBuf[8];                    /*应答帧缓冲区*/
static uint32_t nextTx = 0;                  /* 下一次发送的节拍时刻（ms），切换模式时清 0 */

static uint32_t ledOffTick = 0;              /* LED 定时熄灭时刻（0 = 无定时） */
static uint8_t  ledSteady  = 0;              /* 1 = 常亮（直收模式），忽略定时熄灭 */

/* 按键表：上电默认松开 */
static Key_t s_keys[KEY_NUM] =
{
    { S2_GPIO_Port, S2_Pin, "S2", KEY_UP, KEY_UP, 0, 0 },
    { S3_GPIO_Port, S3_Pin, "S3", KEY_UP, KEY_UP, 0, 0 },
};

/* 当前模式与按键请求的目标模式（上电默认直收） */
static uint8_t g_mode    = MODE_RAW_RX;
static uint8_t g_modeReq = MODE_RAW_RX;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static uint8_t Role_Detect(void);/*双向包主从模式选择*/
static void    Key_Scan(void);/*按键扫描，S2直收模式，S3双向包正常收发模式*/
static void    Mode_Update(void);/*模式切换，直收模式和双向包模式*/
static void    RawRx_Loop(void);
static void    Led_Init(void);
static void    Led_Steady(uint8_t on);/*常亮长灭1/0*/
static void    Led_Flash(uint32_t ms);/*点亮ms后熄灭*/
static void    Led_Poll(void);

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

  /* LED1 在 MX_GPIO_Init() 之后再配：此时 GPIOA 时钟已开 */
  Led_Init();

  /* ---- 角色选择：CHOOSE_MODE 高 = 发送，低 = 接收 ----
   * CHOOSE_MODE 已在 MX_GPIO_Init() 中配置为输入 + 上拉，这里只需读取 */
  gRole = Role_Detect();

  printf("ROLE pin: CHOOSE_MODE=%d -> %s\r\n",
         (int)HAL_GPIO_ReadPin(CHOOSE_MODE_GPIO_Port, CHOOSE_MODE_Pin),
         (gRole == ROLE_TX) ? "TRANSMITTER" : "RECEIVER");

  printf("\r\n===== Wf4463 %s =====\r\n",
         (gRole == ROLE_TX) ? "TRANSMITTER" : "RECEIVER");

  /* 按键自检：松开时读回应为 1（内部上拉），按下为 0 */
  printf("KEY idle: S2(PB6)=%d S3(PB7)=%d  (1=release 0=pressed)\r\n",
         (int)HAL_GPIO_ReadPin(S2_GPIO_Port, S2_Pin),
         (int)HAL_GPIO_ReadPin(S3_GPIO_Port, S3_Pin));

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


  /* ---- 上电：先按宏把芯片配成休眠，等 S2 / S3 ----
   * 一旦按键进入工作模式，之后就不再回到休眠（MODE_SLEEP 是单向出口）。
   * S2 = 直收，S3 = 双向包，两者之间仍可随时互切（在 Mode_Update() 里执行）。 */
#if BOOT_LP_ENABLE
  rc = wf4463_lp_enter(BOOT_LP_MODE, BOOT_LP_WUT_MS);
  Led_Steady(0);                    /* 休眠态 LED 必须是灭的 */
  g_mode    = MODE_SLEEP;
  g_modeReq = MODE_SLEEP;
  printf("[BOOT] LP=%s rc=%d   waiting S2=RAW / S3=PACKET\r\n",
         wf4463_lp_name(BOOT_LP_MODE), rc);
#else
  g_mode    = MODE_RAW_RX;
  g_modeReq = MODE_RAW_RX;
  rc = wf4463_rawrx_start(APP_CHANNEL);
  Led_Steady(1);                    /* 上电默认直收 → LED1 常亮 */
  printf("[MODE] RAW RX (GPIO1=RX_DATA) rc=%d   [S2=RAW  S3=PACKET]\r\n", rc);
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* ---- 轮询按键：TX / RX 两种角色都会扫描 ---- */
    Key_Scan();

    /* 按键请求的模式变更在这里落地（安全点执行，不在中断里做重活） */
    Mode_Update();

    /* LED 定时熄灭（非阻塞，直收常亮时由 ledSteady 挡掉） */
    Led_Poll();

    /* ---- 休眠态：只扫按键，一律不碰芯片 ----
       任何一次 SPI 访问都会把芯片从 SLEEP 拉回 SPI ACTIVE(1.35 mA)，
       所以这里连状态查询都不做。按下 S2/S3 后的唤醒由上面的
       Mode_Update() 负责。 */
    if (g_mode == MODE_SLEEP)
    {
        continue;
    }

    /* ---- 直收模式：芯片自己收，比特流从 GPIO1 出，MCU 只做保活 ---- */
    if (g_mode == MODE_RAW_RX)
    {
        RawRx_Loop();
        continue;
    }

    if (gRole == ROLE_TX)
    {
        /* ================= 发送端 ================= */
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
            		Led_Flash(LED_FLASH_MS);   /* 主机：收到匹配的 ACK，短闪 */
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

        	/* 从机：收到有效包并已回 ACK，短闪；其余情况不亮 */
        	if (ackBuf[0] == ACK_HEAD_OK)
        	{
        		Led_Flash(LED_FLASH_MS);
        	}

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

/* ============================================================
 * 按键扫描（轮询方式，放在 while(1) 里反复调用）
 *
 * 规则：
 *   1. 引脚读回低电平 = 按下，统一归一化成 0=按下 / 1=松开；
 *   2. 采样电平一变就重新开始计时，电平稳定保持 KEY_DEBOUNCE_MS
 *      才认为是真正的状态变化（消抖）；
 *   3. 只在"确认变化"的那一刻打印一次，不会一直刷屏；
 *   4. 全程不阻塞、不用 HAL_Delay，可直接放进任何主循环。
 * ============================================================ */
static void Key_Scan(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < KEY_NUM; i++)
    {
        Key_t *k = &s_keys[i];

        /* 硬件是低有效，这里转成 0=按下 / 1=松开，便于阅读 */
        uint8_t raw = (HAL_GPIO_ReadPin(k->port, k->pin) == GPIO_PIN_RESET) ? KEY_DOWN : KEY_UP;

        if (raw != k->raw)                    /* 电平刚变化：重新开始消抖计时 */
        {
            k->raw        = raw;
            k->changeTick = now;
            continue;
        }

        if (raw == k->stable)                 /* 与已确认的状态相同：没有事件 */
        {
            continue;
        }

        if ((uint32_t)(now - k->changeTick) < KEY_DEBOUNCE_MS)   /* 仍在抖动窗口内 */
        {
            continue;
        }

        /* 消抖通过，确认一次状态变化 */
        k->stable = raw;

        if (k->stable == KEY_DOWN)
        {
            k->downTick = now;
            printf("[KEY] %s DOWN\r\n", k->name);

            /* S2 -> 直收，S3 -> 双向包（真正的切换在 Mode_Update 里做） */
            g_modeReq = (i == 0u) ? MODE_RAW_RX : MODE_PACKET;
        }
        else
        {
            printf("[KEY] %s UP   (held %lu ms)\r\n",
                   k->name, (unsigned long)(now - k->downTick));
        }
    }
}

/* ============================================================
 * 模式切换：把 g_modeReq 落到芯片上
 *
 * 两种模式共用同一份射频配置（radio_config.h），切换时只重配
 * GPIO_PIN_CFG 和接收状态，不重写属性表 —— 所以不存在两套参数
 * 互相残留的问题，来回切多少次结果都一样。
 * ============================================================ */
static void Mode_Update(void)
{
    wf4463_diag_t d;

    if (g_modeReq == g_mode)
    {
        return;
    }

    /* ---- 从休眠态出来：先把芯片叫醒，再进目标模式 ----
     * SLEEP 档：一次 SPI 访问就会让它自动回到 SPI ACTIVE，几微秒；
     * SHUTDOWN 档：寄存器已丢失，必须先冷启动重配（约 120 ms）。 */
    if (g_mode == MODE_SLEEP)
    {
        int8_t wrc = wf4463_lp_wake();

        printf("[WAKE] %s\r\n", (wrc == WF4463_OK) ? "ok" : "FAILED");

        if (wrc != WF4463_OK)
        {
            /* 唤醒失败就不要硬走：留在休眠态，下次按键再试。
               直接把 g_mode 改成工作模式会对着一个没配置的芯片发命令。 */
            g_modeReq = MODE_SLEEP;
            return;
        }
    }

    g_mode = g_modeReq;

    /* 每次切换模式都把双向包的发送序号和发送节拍清 0：
       切回包模式后立刻从 #001 重新开始，不用等上一个 500ms 节拍 */
    txSeq  = 0;
    nextTx = 0;

    if (g_mode == MODE_RAW_RX)
    {
        printf("[MODE] -> RAW RX (GPIO1=RX_DATA)\r\n");
        wf4463_rawrx_start(APP_CHANNEL);
        Led_Steady(1);                  /* 直收模式：LED1 常亮 */
    }
    else
    {
        printf("[MODE] -> PACKET%s  (txSeq reset)\r\n", (gRole == ROLE_TX) ? " (TX)" : " (RX)");
        wf4463_packet_resume(APP_CHANNEL);
        Led_Steady(0);                  /* 包模式：交给收包/收 ACK 短闪 */
    }

    /* 回读校验：XO_TUNE / PA 功率档 / 芯片状态，一眼看出配置有没有写坏 */
    if (wf4463_diag(&d) == WF4463_OK)
    {
        printf("[MODE] chk xo=0x%02X pa=0x%02X state=%u\r\n",
               d.xo_tune, d.pa_pwr, d.state);
    }
}

/* ============================================================
 * 直收模式的主循环
 *
 * 芯片自己把比特流送到 GPIO1，MCU 不参与解码，只做两件事：
 *   1. 什么都不用干（数据在引脚上，用逻辑分析仪/示波器看）
 *   2. 每秒确认一次芯片没掉出 RX，掉了就重新武装
 * ============================================================ */
static void RawRx_Loop(void)
{
    static uint32_t nextCheck = 0;
    uint32_t now = HAL_GetTick();
    uint8_t  st  = 0;

    if ((int32_t)(now - nextCheck) < 0)
    {
        return;
    }
    nextCheck = now + 1000u;

    if ((wf4463_device_state(&st) == WF4463_OK) && (st != 8u))
    {
        printf("[RAW] state=%u, re-arm\r\n", (unsigned)st);
        wf4463_rawrx_start(APP_CHANNEL);
    }
}

/* ============================================================
 * LED1（PA15，低电平点亮）
 *
 * 用法两种，互斥：
 *   Led_Steady(1)     常亮 —— 直收模式
 *   Led_Flash(80)     短闪 —— 收到包 / 收到 ACK
 * 全程非阻塞：Flash 只负责点亮并记下到期时刻，由 Led_Poll()
 * 在主循环里熄灭，不会占用主循环（从机等包只有 50ms、
 * 主机等 ACK 只有 100ms，绝不能在这里 HAL_Delay）。
 * ============================================================ */
static void Led_Init(void)
{
    GPIO_InitTypeDef g = {0};

    LED_OFF();                          /* 先写高电平（灭），避免上电闪一下 */

    g.Pin   = LED1_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED1_GPIO_Port, &g);

    LED_OFF();
}

/* 常亮开关：on=1 常亮并屏蔽定时熄灭；on=0 熄灭并交回定时逻辑 */
static void Led_Steady(uint8_t on)
{
    ledSteady  = on;
    ledOffTick = 0;

    if (on)
    {
        LED_ON();
    }
    else
    {
        LED_OFF();
    }
}

/* 点亮 ms 毫秒，由 Led_Poll() 到期熄灭；常亮期间调用无效 */
static void Led_Flash(uint32_t ms)
{
    if (ledSteady)
    {
        return;
    }

    LED_ON();
    ledOffTick = HAL_GetTick() + ms;
}

/* 主循环调用：到点熄灭 */
static void Led_Poll(void)
{
    if (ledSteady || (ledOffTick == 0u))
    {
        return;
    }

    if ((int32_t)(HAL_GetTick() - ledOffTick) >= 0)
    {
        LED_OFF();
        ledOffTick = 0;
    }
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
