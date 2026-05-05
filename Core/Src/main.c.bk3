/* ================================================================
 * main.c — STM32F103C8T6 | Węzeł CAN — Armata
 * ================================================================
 * Sterowanie przez CAN:
 *   0x100  FIRE    → wystrzel()
 *   0x120  POS     → jedz_do_kata_os1(elewacja) + jedz_do_kata_os2(obrot)
 *   0x121  SEQ_RUN → sekwencer_run(sekwencja[], N)  (hardkodowana sekwencja)
 *   0x122  RECAL   → rekalibracja prądowa + homing
 *
 * Fizyczne wejście: PC14 (przycisk) nadal działa jako lokalne SEQ_RUN
 * ================================================================ */

#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */
#include "ina3221.h"
#include "silniki.h"
#include "kalibracja.h"
#include "sekwencer.h"
#include "flash_config.h"
#include "wystrzal.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* ─── HAL handles ────────────────────────────────────────── */
UART_HandleTypeDef huart2;
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim1;
CAN_HandleTypeDef  hcan;

/* ─── CAN ID definicje ───────────────────────────────────── */
#define CAN_ID_FIRE     0x100u   /* payload[0]=0xF1 → wystrzel()          */
#define CAN_ID_POS      0x120u   /* payload[1..4] = os1_raw, os2_raw (u16)*/
#define CAN_ID_SEQ_RUN  0x121u   /* uruchamia sekwencję hardkodowaną       */
#define CAN_ID_RECAL    0x122u   /* rekalibracja prądowa + homing          */

/* ─── Konwersja CRSF → stopnie ──────────────────────────── */
/* OS1 elewacja: CRSF 172–1811 → −20°…+90°  (zakres 110°)  */
/* OS2 obrót:    CRSF 172–1811 → −160°…+160° (zakres 320°) */
#define CRSF_MIN  172.0f
#define CRSF_MAX  1811.0f
#define CRSF_RNG  (CRSF_MAX - CRSF_MIN)    /* 1639.0f */

static inline float crsf_to_os1(uint16_t raw) {
    float r = (raw - CRSF_MIN) / CRSF_RNG;          /* 0.0–1.0 */
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    return -20.0f + r * 110.0f;                      /* −20°…+90° */
}

static inline float crsf_to_os2(uint16_t raw) {
    float r = (raw - CRSF_MIN) / CRSF_RNG;
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    return -160.0f + r * KAT_OS2_DEG;               /* −160°…+160° */
}

/* ─── Sekwencja hardkodowana (lokalna) ───────────────────── */
/* USER CODE BEGIN PV */
static const Pozycja sekwencja[] = {
    {  40.0f, 210.0f, 500, 1 },
    {  20.0f, 270.0f, 500, 1 },
    {  70.0f, 280.0f, 500, 1 },  /* WYSTRZAL */
    {  80.0f, 250.0f, 500, 1 },
    {  45.0f, 270.0f, 500, 1 },  /* WYSTRZAL */
    {  60.0f, 250.0f, 500, 1 },
};
#define SEKWENCJA_LEN  (sizeof(sekwencja) / sizeof(sekwencja[0]))

static char uart_buf[80];

/* Flagi ustawiane w callbacku CAN (volatile — callback z IRQ) */
volatile uint8_t  flag_fire    = 0;
volatile uint8_t  flag_seq     = 0;
volatile uint8_t  flag_recal   = 0;
volatile uint8_t  flag_pos     = 0;
volatile uint16_t can_os1_raw  = 992u;   /* domyślnie środek CRSF */
volatile uint16_t can_os2_raw  = 992u;
/* USER CODE END PV */

/* ─── Prototypy ──────────────────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CAN_Init(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */
static void kalibracja_wykonaj_i_zapisz(KalibracjaFlash *kd);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
/* USER CODE END 0 */

/* ================================================================
 * MAIN
 * ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    MX_CAN_Init();

    /* USER CODE BEGIN 2 */
    wystrzal_pwm_init();
    printf("\r\n=== ARMATA CAN NODE START ===\r\n");
    printf("FIRE=0x%03X  POS=0x%03X  SEQ=0x%03X  RECAL=0x%03X\r\n",
           CAN_ID_FIRE, CAN_ID_POS, CAN_ID_SEQ_RUN, CAN_ID_RECAL);

    INA3221_Init(&hi2c1);

    /* ── Wczytaj kalibrację z Flash ── */
    KalibracjaFlash kalData = {0};
    if (flash_wczytaj_kalibracje(&kalData)) {
        printf("Flash OK — pomijam kalibracje.\r\n");
        ms_per_deg_os1 = kalData.ms_per_deg_os1;
        ms_per_deg_os2 = kalData.ms_per_deg_os2;
        thresh_ch1_mA  = kalData.thresh_ch1_mA;
        thresh_ch2_mA  = kalData.thresh_ch2_mA;
        pos_os1        = kalData.pos_os1;
        pos_os2        = kalData.pos_os2;
        printf("OS1=%.1f°  OS2=%.1f°  %.3fms/deg  %.3fms/deg\r\n",
               pos_os1, pos_os2, ms_per_deg_os1, ms_per_deg_os2);
    } else {
        printf("Brak Flash — kalibracja...\r\n");
        kalibracja_wykonaj_i_zapisz(&kalData);
    }

    /* ── Filtr CAN: akceptuje 0x100 i 0x120–0x122 ── */
    /* Bank 0: dokładne dopasowanie 0x100 (FIRE) */
    CAN_FilterTypeDef f = {0};
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh         = (CAN_ID_FIRE << 5);
    f.FilterMaskIdHigh     = (0x7FFu << 5);    /* dokładnie 0x100 */
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &f);

    /* Bank 1: 0x120–0x123 (maska 0x7FC → bity 0,1 wolne) */
    CAN_FilterTypeDef f2 = {0};
    f2.FilterBank           = 1;
    f2.FilterMode           = CAN_FILTERMODE_IDMASK;
    f2.FilterScale          = CAN_FILTERSCALE_32BIT;
    f2.FilterIdHigh         = (0x120u << 5);
    f2.FilterMaskIdHigh     = (0x7FCu << 5);   /* akceptuje 0x120,0x121,0x122,0x123 */
    f2.FilterFIFOAssignment = CAN_RX_FIFO0;
    f2.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &f2);

    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    printf("CAN gotowy. Czekam na komendy...\r\n");
    /* USER CODE END 2 */

    /* ================================================================
     * PĘTLA GŁÓWNA
     * Callbacki CAN ustawiają flagi — pętla je obsługuje.
     * Flagi zamiast bezpośrednich wywołań z IRQ bo jedz_do_kata()
     * i sekwencer_run() używają HAL_Delay() — NIE wolno z IRQ!
     * ================================================================ */
    while (1)
    {
        /* USER CODE BEGIN WHILE */

        /* ── 1. FIRE z CAN ── */
        if (flag_fire) {
            flag_fire = 0;
            printf("[CAN] FIRE\r\n");
            wystrzel();
        }

        /* ── 2. POS z CAN — jedź do podanej pozycji ── */
        if (flag_pos) {
            flag_pos = 0;
            float os1 = crsf_to_os1(can_os1_raw);
            float os2 = crsf_to_os2(can_os2_raw);
            snprintf(uart_buf, sizeof(uart_buf),
                     "[CAN] POS os1=%.1f° os2=%.1f°\r\n", os1, os2);
            printf("%s", uart_buf);
            jedz_do_kata_os1(os1);
            jedz_do_kata_os2(os2);
            /* Zapisz pozycję do Flash */
            kalData.pos_os1 = pos_os1;
            kalData.pos_os2 = pos_os2;
            kalData.magic   = FLASH_MAGIC;
            kalData.crc     = oblicz_crc_pub(&kalData);
            flash_zapisz_kalibracje(&kalData);
        }

        /* ── 3. SEQ_RUN z CAN — sekwencja hardkodowana ── */
        if (flag_seq) {
            flag_seq = 0;
            printf("[CAN] SEQ START\r\n");
            sekwencer_run(sekwencja, SEKWENCJA_LEN);
            printf("[CAN] SEQ OK\r\n");
            kalData.pos_os1 = pos_os1;
            kalData.pos_os2 = pos_os2;
            kalData.magic   = FLASH_MAGIC;
            kalData.crc     = oblicz_crc_pub(&kalData);
            flash_zapisz_kalibracje(&kalData);
        }

        /* ── 4. RECAL z CAN ── */
        if (flag_recal) {
            flag_recal = 0;
            printf("[CAN] RECAL start\r\n");
            kalibracja_wykonaj_i_zapisz(&kalData);
            printf("[CAN] RECAL OK\r\n");
        }

        /* ── 5. Lokalny przycisk PC14 = SEQ_RUN (bez CAN) ── */
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET) {
            HAL_Delay(50);
            while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET)
                HAL_Delay(10);
            HAL_Delay(50);
            printf("[BTN] SEQ START\r\n");
            sekwencer_run(sekwencja, SEKWENCJA_LEN);
            kalData.pos_os1 = pos_os1;
            kalData.pos_os2 = pos_os2;
            kalData.magic   = FLASH_MAGIC;
            kalData.crc     = oblicz_crc_pub(&kalData);
            flash_zapisz_kalibracje(&kalData);
        }

        /* LED heartbeat — mrugnięcie co ~1s gdy idle */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
        /* USER CODE END WHILE */
    }
}

/* ================================================================
 * CALLBACK CAN — tylko ustawia flagi i kopiuje dane
 * NIE wywołuje nic z HAL_Delay() !
 * ================================================================ */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan_h)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    if (HAL_CAN_GetRxMessage(hcan_h, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
        return;

    switch (RxHeader.StdId)
    {
        case CAN_ID_FIRE:
            if (RxData[0] == 0xF1)
                flag_fire = 1;
            break;

        case CAN_ID_POS:
            /* payload: [0]=device_id  [1..2]=os1_raw(u16)  [3..4]=os2_raw(u16) */
            can_os1_raw = ((uint16_t)RxData[1] << 8) | RxData[2];
            can_os2_raw = ((uint16_t)RxData[3] << 8) | RxData[4];
            flag_pos    = 1;
            break;

        case CAN_ID_SEQ_RUN:
            flag_seq = 1;
            break;

        case CAN_ID_RECAL:
            flag_recal = 1;
            break;

        default:
            break;
    }
}

/* ================================================================
 * Pomocnicza: kalibracja + zapis Flash
 * ================================================================ */
static void kalibracja_wykonaj_i_zapisz(KalibracjaFlash *kd)
{
    wykonaj_kalibracje_pradowa();
    wykonaj_homing_i_geometrie();
    kd->ms_per_deg_os1 = ms_per_deg_os1;
    kd->ms_per_deg_os2 = ms_per_deg_os2;
    kd->thresh_ch1_mA  = thresh_ch1_mA;
    kd->thresh_ch2_mA  = thresh_ch2_mA;
    kd->pos_os1        = pos_os1;
    kd->pos_os2        = pos_os2;
    kd->magic          = FLASH_MAGIC;
    kd->crc            = oblicz_crc_pub(kd);
    if (flash_zapisz_kalibracje(kd) == HAL_OK)
        printf("Kalibracja zapisana.\r\n");
    else
        printf("BLAD zapisu Flash!\r\n");
}

/* ================================================================
 * KONFIGURACJE SPRZĘTOWE
 * ================================================================ */
int __io_putchar(int ch);  /* forward decl dla printf */

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef       RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef       RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit     = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* PC13 LED */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PA9/PA10/PA11/PA12 — silniki */
    HAL_GPIO_WritePin(GPIOA, PIN_GORA | PIN_DOL | PIN_CCW | PIN_CW, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = PIN_GORA | PIN_DOL | PIN_CCW | PIN_CW;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB6/PB7 — I2C1 */
    GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PC14 — przycisk START (aktywny LOW) */
    GPIO_InitStruct.Pin   = GPIO_PIN_14;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_CAN_Init(void) {
    hcan.Instance                  = CAN1;
    hcan.Init.Prescaler            = 4;
    hcan.Init.Mode                 = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1             = CAN_BS1_15TQ;
    hcan.Init.TimeSeg2             = CAN_BS2_2TQ;
    hcan.Init.TimeTriggeredMode    = DISABLE;
    hcan.Init.AutoBusOff           = DISABLE;
    hcan.Init.AutoWakeUp           = DISABLE;
    hcan.Init.AutoRetransmission   = ENABLE;
    hcan.Init.ReceiveFifoLocked    = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan) != HAL_OK) Error_Handler();
}

/* USER CODE BEGIN 4 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin   = GPIO_PIN_2;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin  = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}
/* USER CODE END 4 */

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
