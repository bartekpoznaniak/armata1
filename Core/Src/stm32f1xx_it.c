/* ================================================================
 * stm32f1xx_it.c — handlery przerwań | Armata CAN node
 * ================================================================ */

#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef  hcan;
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef  hi2c1;

/* SysTick — wymagany przez HAL_Delay() */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* CAN1 RX FIFO0 — wywołuje callback w main.c */
//void CAN1_RX0_IRQHandler(void)
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan);
}

/* USART2 — opcjonalnie jeśli użyjesz UART RX w przyszłości */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
