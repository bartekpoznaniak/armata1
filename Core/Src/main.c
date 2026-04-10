/* USER CODE BEGIN Header */
/* ================================================================
 * main.c — STM32F103C8T6 | Sekwencer kątowy
 * ================================================================ */
/* USER CODE END Header */

#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */
#include "ina3221.h"
#include "silniki.h"
#include "kalibracja.h"
#include "sekwencer.h"
#include "flash_config.h"
#include "wystrzal.h"
#include <stdio.h>
/* USER CODE END Includes */

/* ─── HAL handles — CubeMX regeneruje te deklaracje ─────── */
UART_HandleTypeDef huart2;
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim1;   /* dodane przez CubeMX po regeneracji */

/* USER CODE BEGIN PV */
static const Pozycja sekwencja[] = {
    {  40.0f, 100.0f,  500, 1 },
    {  20.0f,  50.0f,  500, 1 },
    {  70.0f,  80.0f,  500, 1 },  /* WYSTRZAL       */
    {  80.0f, 120.0f,  500, 1 },
    {  45.0f, 170.0f,  500, 1 },  /* WYSTRZAL       */
    {  60.0f, 150.0f,  500, 1 },
};
#define SEKWENCJA_LEN  (sizeof(sekwencja) / sizeof(sekwencja[0]))
/* USER CODE END PV */

/* ─── Prototypy funkcji sprzętowych ─────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
//static void MX_TIM1_Init(void);   /* CubeMX doda tę funkcję — NIE wywołuj z main() */
static void MX_USART2_UART_Init(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */
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
    /* MX_TIM1_Init(); */   /* ← ZAKOMENTOWANE — wystrzal_pwm_init() robi to samo */
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    wystrzal_pwm_init();
    printf("\r\n=== SYSTEM START ===\r\n\r\n");

    INA3221_Init(&hi2c1);

    KalibracjaFlash kalData = {0};

    if (flash_wczytaj_kalibracje(&kalData)) {
        printf("Flash OK — pomijam kalibracje.\r\n");
        ms_per_deg_os1 = kalData.ms_per_deg_os1;
        ms_per_deg_os2 = kalData.ms_per_deg_os2;
        thresh_ch1_mA  = kalData.thresh_ch1_mA;
        thresh_ch2_mA  = kalData.thresh_ch2_mA;
        pos_os1        = kalData.pos_os1;
        pos_os2        = kalData.pos_os2;
        printf("Pozycja przywrocona: OS1=%.1f  OS2=%.1f\r\n", pos_os1, pos_os2);
        printf("OS1: %.3f ms/deg  OS2: %.3f ms/deg\r\n",
               ms_per_deg_os1, ms_per_deg_os2);
    } else {
        printf("Brak danych Flash — wykonuje kalibracje...\r\n");
        wykonaj_kalibracje_pradowa();
        wykonaj_homing_i_geometrie();
        kalData.ms_per_deg_os1 = ms_per_deg_os1;
        kalData.ms_per_deg_os2 = ms_per_deg_os2;
        kalData.thresh_ch1_mA  = thresh_ch1_mA;
        kalData.thresh_ch2_mA  = thresh_ch2_mA;
        kalData.magic          = FLASH_MAGIC;
        kalData.crc            = oblicz_crc_pub(&kalData);
        if (flash_zapisz_kalibracje(&kalData) == HAL_OK)
            printf("Kalibracja zapisana do Flash.\r\n");
        else
            printf("BLAD zapisu Flash!\r\n");
    }

    uint32_t nr = 0;
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN WHILE */
        printf("\r\n[%lu] Czekam na przycisk (PC14)...\r\n", ++nr);
        while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_SET) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(300);
        }
        HAL_Delay(50);
        while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET) HAL_Delay(10);
        HAL_Delay(50);

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        printf("Start sekwencji #%lu\r\n", nr);

        sekwencer_run(sekwencja, SEKWENCJA_LEN);

        if (0) {   /* TODO: flaga rekalibracji */
            printf("Nieoczekiwany zderzak — rekalibracja!\r\n");
            //generator_wyl();
            wykonaj_kalibracje_pradowa();
            wykonaj_homing_i_geometrie();
            kalData.ms_per_deg_os1 = ms_per_deg_os1;
            kalData.ms_per_deg_os2 = ms_per_deg_os2;
            kalData.thresh_ch1_mA  = thresh_ch1_mA;
            kalData.thresh_ch2_mA  = thresh_ch2_mA;
            kalData.magic          = FLASH_MAGIC;
            kalData.crc            = oblicz_crc_pub(&kalData);
            flash_zapisz_kalibracje(&kalData);
            //generator_wl();
            printf("Rekalibracja zapisana.\r\n");
        } else {
            printf("Sekwencja #%lu OK. Zapisuje pozycje do Flash.\r\n", nr);
            kalData.ms_per_deg_os1 = ms_per_deg_os1;
            kalData.ms_per_deg_os2 = ms_per_deg_os2;
            kalData.thresh_ch1_mA  = thresh_ch1_mA;
            kalData.thresh_ch2_mA  = thresh_ch2_mA;
            kalData.pos_os1        = pos_os1;
            kalData.pos_os2        = pos_os2;
            kalData.magic          = FLASH_MAGIC;
            kalData.crc            = oblicz_crc_pub(&kalData);
            flash_zapisz_kalibracje(&kalData);
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        /* USER CODE END 3 */
    }
}

/* ================================================================
 * KONFIGURACJE SPRZĘTOWE
 * (CubeMX regeneruje te funkcje — Twój kod NIE jest tutaj)
 * ================================================================ */
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

    /* PC14 — przycisk START (aktywny LOW, wewnętrzny pull-up) */
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

//static void MX_TIM1_Init(void) {
//    /* Pusta — nigdy nie wywolywana.
//     * CubeMX wygeneruje tu wlasna wersje po regeneracji.
//     * Timer jest inicjowany przez wystrzal_pwm_init() w wystrzal.c */
//}

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
