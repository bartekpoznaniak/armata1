//flash_config.h
#pragma once
#include "stm32f1xx_hal.h"

#define FLASH_CONFIG_ADDR    0x0800FC00UL
#define FLASH_MAGIC          0xCAFE1234UL


typedef struct {
    uint32_t  magic;
    float     ms_per_deg_os1;
    float     ms_per_deg_os2;
    float     thresh_ch1_mA;
    float     thresh_ch2_mA;
    float     pos_os1;
    float     pos_os2;
    uint32_t  crc;
} KalibracjaFlash;

uint32_t          oblicz_crc_pub(const KalibracjaFlash *d);
HAL_StatusTypeDef flash_zapisz_kalibracje(const KalibracjaFlash *dane);
int               flash_wczytaj_kalibracje(KalibracjaFlash *dane);
int               flash_kalibracja_valid(void);
