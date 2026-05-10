/* can_fire.h */
#ifndef CAN_FIRE_H
#define CAN_FIRE_H

#include "main.h"  // dla HAL_CAN

extern volatile uint8_t can_fire_requested;

void can_fire_init(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

#endif
