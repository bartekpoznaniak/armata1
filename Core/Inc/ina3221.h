#ifndef INA3221_H
#define INA3221_H

#include "stm32f1xx_hal.h"

void  INA3221_Init(I2C_HandleTypeDef *hi2c);
float INA3221_GetCurrentA(uint8_t ch);
float INA3221_GetBusV(uint8_t ch);

#endif
