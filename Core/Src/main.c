#include "stm32f1xx_hal.h"
#include "ina3221.h"
#include <stdio.h>
#include <string.h>

/* Uchwyty HAL */
UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;

/* Piny sterujące silnikami */
#define PIN_GORA GPIO_PIN_9
#define PIN_DOL  GPIO_PIN_10
#define PIN_LEWO GPIO_PIN_11
#define PIN_PRAWO GPIO_PIN_12

#define MOTOR_PORT GPIOA
#define MOTOR_PINS (PIN_GORA | PIN_DOL | PIN_LEWO | PIN_PRAWO)

/* Prototypy */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

static void Motor_Stop(void);
static void Motor_Up(void);
static void Motor_Down(void);
static void Motor_Left(void);
static void Motor_Right(void);
static void Print_Currents(void);

void Error_Handler(void);

/* printf przez USART2 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)&ch,
        1,
        HAL_MAX_DELAY
    );

    return ch;
}

/* Zatrzymanie wszystkich kierunków */
static void Motor_Stop(void)
{
    HAL_GPIO_WritePin(
        MOTOR_PORT,
        MOTOR_PINS,
        GPIO_PIN_RESET
    );
}

/* Tylko jeden kierunek aktywny naraz */
static void Motor_Up(void)
{
    Motor_Stop();

    HAL_GPIO_WritePin(
        MOTOR_PORT,
        PIN_GORA,
        GPIO_PIN_SET
    );

    printf("RUCH: GORA\r\n");
}

static void Motor_Down(void)
{
    Motor_Stop();

    HAL_GPIO_WritePin(
        MOTOR_PORT,
        PIN_DOL,
        GPIO_PIN_SET
    );

    printf("RUCH: DOL\r\n");
}

static void Motor_Left(void)
{
    Motor_Stop();

    HAL_GPIO_WritePin(
        MOTOR_PORT,
        PIN_LEWO,
        GPIO_PIN_SET
    );

    printf("RUCH: LEWO\r\n");
}

static void Motor_Right(void)
{
    Motor_Stop();

    HAL_GPIO_WritePin(
        MOTOR_PORT,
        PIN_PRAWO,
        GPIO_PIN_SET
    );

    printf("RUCH: PRAWO\r\n");
}

/*
 * Ta funkcja wymaga dopasowania do rzeczywistego API INA3221.
 *
 * Przykładowe nazwy:
 * INA3221_ReadCurrent_mA(&hi2c1, 1);
 * INA3221_GetCurrent_mA(&hi2c1, 1);
 * INA3221_ReadChannelCurrent(&hi2c1, 1);
 */



static void Print_Currents(void)
{

    /*
     * INA3221:
     * CH1 - oś lewo/prawo
     * CH2 - oś góra/dół
     * CH3 - niewykorzystany
     */

    float current1_A;
    float current2_A;
    float current3_A;

    current1_A = INA3221_GetCurrentA(1);
    current2_A = INA3221_GetCurrentA(2);
    current3_A = INA3221_GetCurrentA(3);

    printf(
        "INA3221: CH1=%.3f A | CH2=%.3f A | CH3=%.3f A\r\n",
        current1_A,
        current2_A,
        current3_A
    );
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /*
     * PA9  - góra
     * PA10 - dół
     * PA11 - lewo
     * PA12 - prawo
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_9 |
        GPIO_PIN_10 |
        GPIO_PIN_11 |
        GPIO_PIN_12,
        GPIO_PIN_RESET
    );

    GPIO_InitStruct.Pin =
        GPIO_PIN_9 |
        GPIO_PIN_10 |
        GPIO_PIN_11 |
        GPIO_PIN_12;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*
     * LED na PC13
     */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();

        /*
         * PB6 - SCL
         * PB7 - SDA
         */
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * PA2 - TX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /*
         * PA3 - RX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(200);
    }
}

int main(void)
{
    uint8_t rx;
    uint32_t last_current_time = 0;

    HAL_Init();

    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    Motor_Stop();

    printf("\r\n=== TEST RECZNY SILNIKOW ===\r\n");
    printf("W - gora\r\n");
    printf("S - dol\r\n");
    printf("A - lewo\r\n");
    printf("D - prawo\r\n");
    printf("X - stop\r\n");
    printf("I - odczyt pradow\r\n\r\n");


    printf("I2C scan:\r\n");

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1,
                                  addr << 1,
                                  2,
                                  100) == HAL_OK)
        {
            printf("Znaleziono I2C: 0x%02X\r\n", addr);
        }
    }



    INA3221_Init(&hi2c1);

    while (1)
    {
        /*
         * Odbiór blokujący z timeoutem 10 ms.
         * Dzięki temu pętla może także cyklicznie
         * wysyłać prądy.
         */
        if (HAL_UART_Receive(&huart2, &rx, 1, 10) == HAL_OK)
        {
            switch (rx)
            {

            case 'x':
            case 'X':
                Motor_Stop();
                printf("STOP\r\n");
                break;

            case 'w':
            case 'W':
                Motor_Left();
                break;

            case 's':
            case 'S':
                Motor_Down();
                break;

            case 'a':
            case 'A':
                Motor_Up();
                break;

            case 'd':
            case 'D':
                Motor_Right();
                break;

            	case 'i':
                case 'I':
                    Print_Currents();
                    break;

                case '\r':
                case '\n':
                    break;

                default:
                    printf("Nieznany znak: %c\r\n", rx);
                    break;
            }
        }

        /*
         * Automatyczny odczyt prądów co 500 ms.
         */
        if ((HAL_GetTick() - last_current_time) >= 500)
        {
            last_current_time = HAL_GetTick();
            Print_Currents();
        }
    }
}
