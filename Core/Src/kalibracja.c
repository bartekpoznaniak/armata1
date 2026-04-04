// * kalibracja.c * //
#include "kalibracja.h"
#include "silniki.h"
#include "ina3221.h"
#include <math.h>
#include <stdio.h>

float ms_per_deg_os1 = 0.0f;
float ms_per_deg_os2 = 0.0f;

/* ── Kalibracja progów prądowych ── */
void wykonaj_kalibracje_pradowa(void)
{
    printf("\r\n=== KALIBRACJA PRADOWA ===\r\n");

    /* Używamy zmierzonych wartości — zmień jeśli silniki inne */
    thresh_ch1_mA = DEFAULT_THRESH_CH1_MA;
    thresh_ch2_mA = DEFAULT_THRESH_CH2_MA;

    printf("[CALIB] CH1 (CW/CCW)   prog stall: %.1f mA\r\n", thresh_ch1_mA);
    printf("[CALIB] CH2 (GORA/DOL) prog stall: %.1f mA\r\n", thresh_ch2_mA);
    printf("=== OK ===\r\n\r\n");
}

/* ── Homing + pomiar zakresu obu osi ── */
void wykonaj_homing_i_geometrie(void)
{
    printf("\r\n=== HOMING + KALIBRACJA GEOMETRII ===\r\n");

    /* OS1: zderzak A (GORA) */
    printf("[H] OS1 → zderzak A (GORA)...\r\n");
    jedz_do_zderzaka_blok(PIN_GORA, 2);

    /* OS1: pomiar zakresu do zderzaka B (DOL) */
    printf("[H] OS1 → mierzę zakres (DOL)...\r\n");
    uint32_t t1 = jedz_do_zderzaka_blok(PIN_DOL, 2);
    ms_per_deg_os1 = (float)t1 / KAT_OS1_DEG;
    printf("[H] OS1: %lu ms / %.0f° = %.3f ms/°\r\n\r\n",
           t1, KAT_OS1_DEG, ms_per_deg_os1);

    /* OS1: wróć na zderzak A (pozycja 0°) */
    printf("[H] OS1 → powrót na zderzak A...\r\n");
    jedz_do_zderzaka_blok(PIN_GORA, 2);

    /* OS2: zderzak A (CCW) */
    printf("[H] OS2 → zderzak A (CCW)...\r\n");
    jedz_do_zderzaka_blok(PIN_CCW, 1);

    /* OS2: pomiar zakresu do zderzaka B (CW) */
    printf("[H] OS2 → mierzę zakres (CW)...\r\n");
    uint32_t t2 = jedz_do_zderzaka_blok(PIN_CW, 1);
    ms_per_deg_os2 = (float)t2 / KAT_OS2_DEG;
    printf("[H] OS2: %lu ms / %.0f° = %.3f ms/°\r\n\r\n",
           t2, KAT_OS2_DEG, ms_per_deg_os2);

    /* OS2: wróć na zderzak A */
    printf("[H] OS2 → powrót na zderzak A (CCW)...\r\n");
    jedz_do_zderzaka_blok(PIN_CCW, 1);
    pos_os1 = 0.0f;
    pos_os2 = 0.0f;
    printf("=== HOMING ZAKOŃCZONY ===\r\n");
    printf("    OS1: %.3f ms/°  |  OS2: %.3f ms/°\r\n\r\n",
           ms_per_deg_os1, ms_per_deg_os2);
}

/* ── Pozycjonowanie ── */
uint8_t jedz_do_kata_os1(float kat_deg)
{
    printf("[POS] OS1 → %.1f°\r\n", kat_deg);

    /* Homing na zderzak A */
    jedz_do_zderzaka_blok(PIN_GORA, 2);

    /* Jedź do kąta docelowego */
    uint32_t czas_ms = (uint32_t)(kat_deg * ms_per_deg_os1);
    printf("[POS] OS1: %lu ms ruchu\r\n", czas_ms);

    uint8_t ret = jedz_przez_czas(PIN_DOL, 2, czas_ms);
    if (ret == 0)
        printf("[POS] OS1 na %.1f°\r\n", kat_deg);
    return ret;
}

uint8_t jedz_do_kata_os2(float kat_deg)
{
    printf("[POS] OS2 → %.1f°\r\n", kat_deg);

    /* Homing na zderzak A */
    jedz_do_zderzaka_blok(PIN_CCW, 1);

    /* Jedź do kąta docelowego */
    uint32_t czas_ms = (uint32_t)(kat_deg * ms_per_deg_os2);
    printf("[POS] OS2: %lu ms ruchu\r\n", czas_ms);

    uint8_t ret = jedz_przez_czas(PIN_CW, 1, czas_ms);
    if (ret == 0)
        printf("[POS] OS2 na %.1f°\r\n", kat_deg);
    return ret;
}
