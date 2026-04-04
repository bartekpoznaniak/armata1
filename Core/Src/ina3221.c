#include "ina3221.h"
#include <stdio.h>

#define INA3221_ADDR        (0x40 << 1)
#define INA3221_REG_CONFIG  0x00
#define INA3221_REG_SHUNT_1 0x01
#define INA3221_REG_BUS_1   0x02
#define SHUNT_OHMS          0.1f

static I2C_HandleTypeDef *_hi2c = NULL;

static HAL_StatusTypeDef ReadReg(uint8_t reg, int16_t *out)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        _hi2c, INA3221_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, 10);
    if (ret == HAL_OK)
        *out = (int16_t)((buf[0] << 8) | buf[1]);
    return ret;
}

void INA3221_Init(I2C_HandleTypeDef *hi2c)
{
    _hi2c = hi2c;
    uint8_t cfg[3] = { INA3221_REG_CONFIG, 0x71, 0x27 };
    if (HAL_I2C_Master_Transmit(_hi2c, INA3221_ADDR, cfg, 3, 10) != HAL_OK)
        printf("INA3221: BRAK!\r\n");
    else
        printf("INA3221: OK\r\n");
}

float INA3221_GetCurrentA(uint8_t ch)
{
    int16_t raw;
    if (ReadReg(INA3221_REG_SHUNT_1 + (ch-1)*2, &raw) != HAL_OK) return 0.0f;
    return (float)(raw >> 3) * 0.000040f / SHUNT_OHMS;
}

float INA3221_GetBusV(uint8_t ch)
{
    int16_t raw;
    if (ReadReg(INA3221_REG_BUS_1 + (ch-1)*2, &raw) != HAL_OK) return 0.0f;
    return (float)(raw >> 3) * 0.008f;
}
