//flash_config.c
#include "flash_config.h"
#include <string.h>

uint32_t oblicz_crc_pub(const KalibracjaFlash *d) {
    uint32_t sum = 0;
    sum += d->magic;
    sum += *(uint32_t*)&d->ms_per_deg_os1;
    sum += *(uint32_t*)&d->ms_per_deg_os2;
    sum += *(uint32_t*)&d->thresh_ch1_mA;
    sum += *(uint32_t*)&d->thresh_ch2_mA;
    sum += *(uint32_t*)&d->pos_os1;
    sum += *(uint32_t*)&d->pos_os2;
    return sum;
}

int flash_kalibracja_valid(void) {
    KalibracjaFlash tmp;
    memcpy(&tmp, (void*)FLASH_CONFIG_ADDR, sizeof(tmp));
    return (tmp.magic == FLASH_MAGIC) && (tmp.crc == oblicz_crc_pub(&tmp));
}

int flash_wczytaj_kalibracje(KalibracjaFlash *dane) {
    if (!flash_kalibracja_valid()) return 0;
    memcpy(dane, (void*)FLASH_CONFIG_ADDR, sizeof(*dane));
    return 1;
}

HAL_StatusTypeDef flash_zapisz_kalibracje(const KalibracjaFlash *dane) {
    HAL_StatusTypeDef status;
    uint32_t pageError = 0;
    FLASH_EraseInitTypeDef eraseInit = {
        .TypeErase   = FLASH_TYPEERASE_PAGES,
        .PageAddress = FLASH_CONFIG_ADDR,
        .NbPages     = 1
    };
    HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    if (status != HAL_OK) { HAL_FLASH_Lock(); return status; }

    const uint32_t *ptr  = (const uint32_t *)dane;
    uint32_t        addr = FLASH_CONFIG_ADDR;
    for (uint32_t i = 0; i < sizeof(KalibracjaFlash)/4; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, ptr[i]);
        if (status != HAL_OK) break;
        addr += 4;
    }
    HAL_FLASH_Lock();
    return status;
}
