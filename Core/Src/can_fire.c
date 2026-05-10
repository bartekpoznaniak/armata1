/* can_fire.c */
#include "can_fire.h"
#include "sekwencer.h"
#include "main.h"
#include <stdio.h>


CAN_HandleTypeDef hcan1;
CAN_RxHeaderTypeDef rx_header;
uint8_t rx_data[8];

volatile uint8_t can_fire_requested = 0;

void can_fire_init(void)
{
    hcan1.Instance                  = CAN1;
    hcan1.Init.Prescaler            = 9;
    hcan1.Init.Mode                 = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1             = CAN_BS1_6TQ;
    hcan1.Init.TimeSeg2             = CAN_BS2_1TQ;
    hcan1.Init.TimeTriggeredMode    = DISABLE;
    hcan1.Init.AutoBusOff           = DISABLE;
    hcan1.Init.AutoWakeUp           = DISABLE;
    hcan1.Init.AutoRetransmission   = DISABLE;
    hcan1.Init.ReceiveFifoLocked    = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
    	printf("CAN Init FAIL\r\n");
    	return; }

    CAN_FilterTypeDef filter;
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDLIST;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = (0x100 << 5);
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK) {
    	printf("HAL_CAN_ConfigFilter FAIL\r\n");
    	return; }


    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        printf("CAN Start FAIL\r\n");
    	return; }
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) { return; }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        	printf("CAN RX: ID=0x%03lX [%02X %02X %02X]\r\n",
        	                   rx_header.StdId, rx_data[0], rx_data[1], rx_data[2]);
            if (rx_header.StdId == 0x100 &&
                rx_data[0] == 0xF1 &&
                rx_data[1] == 0x12 &&
                rx_data[2] == 0xE0) {
                can_fire_requested = 1;
                printf("FIRE!\r\n");
            }
        }
    }
}


void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hcan->Instance == CAN1) {
        __HAL_RCC_CAN1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_AFIO_CLK_ENABLE();   // ← OBOWIĄZKOWO dla remap

        /* Remap CAN na PB8/PB9 */

        /* Remap CAN na PB8/PB9 — bezpośrednio przez rejestr AFIO */
        uint32_t tmpreg = AFIO->MAPR;
        tmpreg &= ~AFIO_MAPR_CAN_REMAP;           // wyczyść bity CAN remap
        tmpreg |= AFIO_MAPR_CAN_REMAP_REMAP2;     // ustaw REMAP2 = PB8/PB9
        AFIO->MAPR = tmpreg;

        //__HAL_AFIO_REMAP_CAN1_2();     // ← TO jest klucz!

        /* PB8 — CAN RX — input floating */
        GPIO_InitStruct.Pin  = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* PB9 — CAN TX — AF push-pull */
        GPIO_InitStruct.Pin   = GPIO_PIN_9;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

