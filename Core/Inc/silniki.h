#ifndef SILNIKI_H
#define SILNIKI_H

#include "stm32f1xx_hal.h"

/* Piny kierunków */

//#define PIN_GORA  GPIO_PIN_11
//#define PIN_DOL   GPIO_PIN_10
//#define PIN_CCW   GPIO_PIN_9
//#define PIN_CW    GPIO_PIN_12

#define PIN_GORA  GPIO_PIN_11   /* CH2+ */
#define PIN_DOL   GPIO_PIN_10   /* CH2- */
#define PIN_CCW   GPIO_PIN_12    /* CH1+ */
#define PIN_CW    GPIO_PIN_9   /* CH1- */

/* Timing detekcji stall */
#define INA_INTERVAL_MS      20u
#define STALL_BLANK_MS       200u //było 600
#define STALL_CONFIRM_COUNT  1
#define PAUSE_AFTER_STALL_MS 150u //było 300u

/* Progi stall — ustawiane przez kalibracja.c */
extern float    thresh_ch1_mA;
extern float    thresh_ch2_mA;

/* Timery współdzielone */
extern uint32_t t_ina_last;
extern uint32_t t_blank_until;

void     stop_all(void);
void     start_kierunek(uint16_t pin);
uint32_t jedz_do_zderzaka_blok(uint16_t pin, uint8_t ch);
uint8_t  jedz_przez_czas(uint16_t pin, uint8_t ch, uint32_t czas_ms);
uint8_t jedz_sync(float kat_os1, float kat_os2);
#endif
