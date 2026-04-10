// wystrzal.c
#include "wystrzal.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

/* ─── Konfiguracja sygnału ─────────────────────────────────── */
#define PWM_PRESCALER   71u       // 72 MHz / 72 = 1 MHz → 1 tick = 1 µs
#define PWM_PERIOD      19999u    // 20 000 ticks = 20 ms
#define PWM_IDLE_US     1800u     // spoczynek  1,8 ms
#define PWM_SHOT_US     1300u     // wystrzał   1,3 ms
#define SHOT_PULSES     3u        // liczba impulsów strzału

/* ─── Prywatny handle — widoczny tylko w tym pliku ────────── */
static TIM_HandleTypeDef htim1_pwm;

/* ================================================================
 * wystrzal_pwm_init()
 * Wywołaj JEDEN raz w main(), po MX_GPIO_Init().
 * Uruchamia ciągły sygnał spoczynkowy 1,8 ms / 20 ms na PA8.
 * ================================================================ */
void wystrzal_pwm_init(void)
{
    /* 1. Zegary — przed HAL_TIM_PWM_Init (weak MspInit nic nie robi) */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 2. PA8 → AF Push-Pull */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_8;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 3. Baza timera */
    htim1_pwm.Instance               = TIM1;
    htim1_pwm.Init.Prescaler         = PWM_PRESCALER;
    htim1_pwm.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1_pwm.Init.Period            = PWM_PERIOD;
    htim1_pwm.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1_pwm.Init.RepetitionCounter = 0;
    HAL_TIM_PWM_Init(&htim1_pwm);

    /* 4. Kanał 1 — tryb PWM1, stan spoczynkowy */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = PWM_IDLE_US;          // 1800 ticks = 1,8 ms
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1_pwm, &oc, TIM_CHANNEL_1);

    /* 5. Start — HAL_TIM_PWM_Start ustawia bit MOE dla TIM1 automatycznie */
    HAL_TIM_PWM_Start(&htim1_pwm, TIM_CHANNEL_1);

    printf("[PWM] PA8/TIM1_CH1 aktywny — spoczynek %.1f ms / 20 ms\r\n",
           PWM_IDLE_US / 1000.0f);
}

/* ================================================================
 * wystrzel()
 * Wysyła SHOT_PULSES impulsów 1,3 ms, potem wraca na 1,8 ms.
 * Czas trwania: SHOT_PULSES × 20 ms = 60 ms (blokujące).
 * ================================================================ */
void wystrzel(void)
{
    printf("[STRZAL] *** WYSTRZAL — %u impulsy %.1f ms ***\r\n",
           SHOT_PULSES, PWM_SHOT_US / 1000.0f);

    /* Zmień szerokość impulsu na wartość strzału */
    __HAL_TIM_SET_COMPARE(&htim1_pwm, TIM_CHANNEL_1, PWM_SHOT_US);

    /* Czekaj dokładnie SHOT_PULSES pełnych okresów */
    HAL_Delay(SHOT_PULSES * 20u);

    /* Wróć do spoczynku */
    __HAL_TIM_SET_COMPARE(&htim1_pwm, TIM_CHANNEL_1, PWM_IDLE_US);

    printf("[STRZAL] Powrot idle %.1f ms\r\n", PWM_IDLE_US / 1000.0f);
}

//
//// wystrzal.c
//#include "wystrzal.h"
//#include <stdio.h>
//
//void wystrzel(void)
//{
//    // TODO: modul dzwiekowy, LED blask, pompka dymu — kolejna sesja
//    printf("[STRZAL] *** WYSTRZAL ***\r\n");
//}
