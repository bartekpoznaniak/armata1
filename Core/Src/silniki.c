
// * silniki.c * //
#include "silniki.h"
#include "ina3221.h"
#include <math.h>
#include <stdio.h>



extern float ms_per_deg_os1;
extern float ms_per_deg_os2;
#define BLANK_MS  200u

/* Progi — wartości domyślne z pomiarów (nadpisywane przez kalibrację) */

float pos_os1 = 0.0f;   // eksportuj przez silniki.h: extern float pos_os1;
float pos_os2 = 0.0f;

float    thresh_ch1_mA  = 48.0f; //bylo 60
float    thresh_ch2_mA  = 36.0f; //bylo 45

/* Timery */
uint32_t t_ina_last    = 0;
uint32_t t_blank_until = 0;

/* ── Zatrzymaj wszystkie silniki ── */
void stop_all(void)
{
    HAL_GPIO_WritePin(GPIOA,
        PIN_GORA | PIN_DOL | PIN_CCW | PIN_CW,
        GPIO_PIN_RESET);
}

/* ── Uruchom silnik w kierunku pin ── */
void start_kierunek(uint16_t pin)
{
    stop_all();
    HAL_Delay(30);
    HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET);
    t_ina_last    = HAL_GetTick();
    t_blank_until = HAL_GetTick() + STALL_BLANK_MS;
}

/* ── Jedź do zderzaka (blokujące) — zwraca czas ruchu [ms] ── */
uint32_t jedz_do_zderzaka_blok(uint16_t pin, uint8_t ch)
{
    float   thresh = (ch == 2) ? thresh_ch2_mA : thresh_ch1_mA;
    uint8_t cnt    = 0;
    float   I_prev = 0.0f;

    start_kierunek(pin);
    uint32_t t_start = HAL_GetTick();

    while (1)
    {
        uint32_t now = HAL_GetTick();
        if (now - t_ina_last >= INA_INTERVAL_MS)
        {
            t_ina_last = now;
            float I_mA = fabsf(INA3221_GetCurrentA(ch) * 1000.0f);
            float dI   = I_mA - I_prev;
            I_prev = I_mA;

            if (now >= t_blank_until)
            {
                uint8_t hit = (I_mA > thresh) || (dI > 9.4f && I_mA > thresh * 0.5f);
                //uint8_t hit = (I_mA > thresh) || (dI > 10.0f && I_mA > thresh * 0.5f);
                if (hit)
                {
                    cnt++;
                    printf("  STALL? CH%u I=%.1fmA dI=%.1f (%u/%u)\r\n",
                           ch, I_mA, dI, cnt, STALL_CONFIRM_COUNT);
                    if (cnt >= STALL_CONFIRM_COUNT)
                    {
                        stop_all();
                        uint32_t t = now - t_start;
                        printf(">>> ZDERZAK CH%u t=%lu ms\r\n", ch, t);
                        HAL_Delay(PAUSE_AFTER_STALL_MS);
                        return t;
                    }
                }
                else { cnt = 0; }
            }
        }
    }
}

/* ── Jedź przez czas_ms — zwraca 0=OK, 1=nieoczekiwany stall ── */
uint8_t jedz_przez_czas(uint16_t pin, uint8_t ch, uint32_t czas_ms)
{
    if (czas_ms == 0) return 0;

    float   thresh = (ch == 2) ? thresh_ch2_mA : thresh_ch1_mA;
    uint8_t cnt    = 0;

    start_kierunek(pin);
    uint32_t t0 = HAL_GetTick();

    while ((HAL_GetTick() - t0) < czas_ms)
    {
        uint32_t now = HAL_GetTick();
        if (now - t_ina_last >= INA_INTERVAL_MS)
        {
            t_ina_last = now;
            float I_mA = fabsf(INA3221_GetCurrentA(ch) * 1000.0f);

            if (now >= t_blank_until)
            {
                if (I_mA > thresh)
                {
                    cnt++;
                    if (cnt >= STALL_CONFIRM_COUNT)
                    {
                        stop_all();
                        printf("!!! STALL CH%u podczas ruchu I=%.1fmA\r\n",
                               ch, I_mA);
                        return 1;
                    }
                }
                else { cnt = 0; }
            }
        }
    }
    stop_all();
    return 0;
}
uint8_t jedz_sync(float kat_os1, float kat_os2)
{
    float delta1 = kat_os1 - pos_os1;
    float delta2 = kat_os2 - pos_os2;

    printf("[SYNC] OS1→%.1f° (Δ%+.1f°)  OS2→%.1f° (Δ%+.1f°)\r\n",
           kat_os1, delta1, kat_os2, delta2);

    uint16_t pin_os1 = (delta1 >= 0.0f) ? PIN_DOL  : PIN_GORA;
    uint16_t pin_os2 = (delta2 >= 0.0f) ? PIN_CW   : PIN_CCW;

    uint32_t t_os1 = (uint32_t)(fabsf(delta1) * ms_per_deg_os1);
    uint32_t t_os2 = (uint32_t)(fabsf(delta2) * ms_per_deg_os2);

    stop_all();
    HAL_Delay(30);
    if (t_os1 > 0) HAL_GPIO_WritePin(GPIOA, pin_os1, GPIO_PIN_SET);
    if (t_os2 > 0) HAL_GPIO_WritePin(GPIOA, pin_os2, GPIO_PIN_SET);

    uint32_t start   = HAL_GetTick();
    uint32_t t_blank = start + BLANK_MS;
    uint8_t  done1   = (t_os1 == 0);
    uint8_t  done2   = (t_os2 == 0);
    uint8_t  cnt1    = 0, cnt2 = 0;

    while (!done1 || !done2)
    {
        uint32_t now     = HAL_GetTick();
        uint32_t elapsed = now - start;

        if (!done1 && elapsed >= t_os1) {
            HAL_GPIO_WritePin(GPIOA, pin_os1, GPIO_PIN_RESET);
            done1 = 1;
            printf("[SYNC] OS1 na %.1f°\r\n", kat_os1);
        }
        if (!done2 && elapsed >= t_os2) {
            HAL_GPIO_WritePin(GPIOA, pin_os2, GPIO_PIN_RESET);
            done2 = 1;
            printf("[SYNC] OS2 na %.1f°\r\n", kat_os2);
        }

        if (now - t_ina_last >= INA_INTERVAL_MS) {
            t_ina_last = now;
            if (now >= t_blank) {
                if (!done1) {
                    float I1 = fabsf(INA3221_GetCurrentA(2) * 1000.0f);
                    if (I1 > thresh_ch2_mA) { cnt1++; } else { cnt1 = 0; }
                    if (cnt1 >= STALL_CONFIRM_COUNT) {
                        stop_all();
                        printf("!!! STALL OS1 I=%.1fmA\r\n", I1);
                        return 1;
                    }
                }
                if (!done2) {
                    float I2 = fabsf(INA3221_GetCurrentA(1) * 1000.0f);
                    if (I2 > thresh_ch1_mA) { cnt2++; } else { cnt2 = 0; }
                    if (cnt2 >= STALL_CONFIRM_COUNT) {
                        stop_all();
                        printf("!!! STALL OS2 I=%.1fmA\r\n", I2);
                        return 1;
                    }
                }
            }
        }
        HAL_Delay(5);
    }

    pos_os1 = kat_os1;
    pos_os2 = kat_os2;

    return 0;
}



