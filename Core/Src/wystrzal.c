// wystrzal.c
#include "wystrzal.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

/* ================================================================
 * PARAMETRY WYSTRZALU — edytuj wg potrzeb
 * ================================================================ */

/* ── Sygnał PWM ── */
#define PWM_PRESCALER       71u     // 72MHz/72 = 1MHz → 1 tick = 1µs
#define PWM_PERIOD          19999u  // 20 000 ticks = 20 ms
#define PWM_IDLE_US         1800u   // spoczynek  1,8 ms
#define PWM_SHOT_US         1300u   // wystrzal   1,3 ms  ← REGULUJ (1100-1400)
#define SHOT_PULSES         3u      // liczba impulsow                ← REGULUJ

/* ── Timigi efektow ── */
#define LED_CZAS_MS          380u    // czas blasku LED  [ms]          ← REGULUJ
#define POMPKA_CZAS_MS      500u    // czas pracy pompki dymu [ms]    ← REGULUJ

/* ── GPIO efektow ── */
#define PIN_LED_FLASH       GPIO_PIN_5   // PA5  — LED blask
#define PIN_POMPKA          GPIO_PIN_7   // PA7  — pompka dymu
#define PIN_GENERATOR       GPIO_PIN_15  // PB15 — generator dymu (GN)

/* ── Prywatny handle TIM1 ── */
static TIM_HandleTypeDef htim1_pwm;

/* ================================================================
 * wystrzal_pwm_init()
 * Wywolaj JEDEN raz w main(), po MX_GPIO_Init().
 * Konfiguruje: TIM1/PA8 (PWM), PA5 (LED), PA7 (pompka), PB15 (gen).
 * ================================================================ */
void wystrzal_pwm_init(void)
{
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* PA8 → TIM1_CH1 AF Push-Pull (PWM) */
    gpio.Pin   = GPIO_PIN_8;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PA5 (LED blask) + PA7 (pompka) → wyjscia, domyslnie LOW */
    HAL_GPIO_WritePin(GPIOA, PIN_LED_FLASH | PIN_POMPKA, GPIO_PIN_RESET);
    gpio.Pin   = PIN_LED_FLASH | PIN_POMPKA;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PB15 (generator dymu) → wyjscie, domyslnie LOW */
    HAL_GPIO_WritePin(GPIOB, PIN_GENERATOR, GPIO_PIN_RESET);
    gpio.Pin   = PIN_GENERATOR;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* TIM1 — baza */
    htim1_pwm.Instance               = TIM1;
    htim1_pwm.Init.Prescaler         = PWM_PRESCALER;
    htim1_pwm.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1_pwm.Init.Period            = PWM_PERIOD;
    htim1_pwm.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1_pwm.Init.RepetitionCounter = 0;
    HAL_TIM_PWM_Init(&htim1_pwm);

    /* TIM1 — kanal 1, stan spoczynkowy 1,8 ms */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = PWM_IDLE_US;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1_pwm, &oc, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1_pwm, TIM_CHANNEL_1);

    printf("[PWM] PA8/TIM1_CH1 aktywny — spoczynek 1.8ms/20ms\r\n");
    printf("[PWM] PA5=LED  PA7=pompka  PB15=generator — gotowe\r\n");
}

/* ================================================================
 * generator_wl() / generator_wyl()
 * Wywoluj z sekwencer.c na poczatku/koncu sekwencji.
 * Czas rozgrzewania (GENERATOR_ROZGRZEW_MS) ustaw w sekwencer.c.
 * ================================================================ */
void generator_wl(void)
{
    HAL_GPIO_WritePin(GPIOB, PIN_GENERATOR, GPIO_PIN_SET);
    printf("[DYM] Generator ON\r\n");
}

void generator_wyl(void)
{
    HAL_GPIO_WritePin(GPIOB, PIN_GENERATOR, GPIO_PIN_RESET);
    printf("[DYM] Generator OFF\r\n");
}

/* ================================================================
 * wystrzel()
 * Oś czasu (domyslne wartosci):
 *   t=0   ms : LED on, pompka on, PWM→1,3ms
 *   t=60  ms : PWM→1,8ms (po 3 impulsach)
 *   t=80  ms : LED off     (LED_CZAS_MS)
 *   t=200 ms : pompka off  (POMPKA_CZAS_MS)
 * ================================================================ */
void wystrzel(void)
{
    uint32_t t_start = HAL_GetTick();

    printf("[STRZAL] *** WYSTRZAL — %u x %.1fms ***\r\n",
           SHOT_PULSES, PWM_SHOT_US / 1000.0f);

    /* t=0: wszystko wlacz jednoczesnie */
    HAL_GPIO_WritePin(GPIOA, PIN_LED_FLASH | PIN_POMPKA, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim1_pwm, TIM_CHANNEL_1, PWM_SHOT_US);

    /* Czekaj na SHOT_PULSES pelnych okresow PWM */
    HAL_Delay(SHOT_PULSES * 20u);
    __HAL_TIM_SET_COMPARE(&htim1_pwm, TIM_CHANNEL_1, PWM_IDLE_US);

    /* Gasnie LED po LED_CZAS_MS od startu */
    uint32_t elapsed = HAL_GetTick() - t_start;
    if (elapsed < LED_CZAS_MS)
        HAL_Delay(LED_CZAS_MS - elapsed);
    HAL_GPIO_WritePin(GPIOA, PIN_LED_FLASH, GPIO_PIN_RESET);

    /* Gasnie pompka po POMPKA_CZAS_MS od startu */
    elapsed = HAL_GetTick() - t_start;
    if (elapsed < POMPKA_CZAS_MS)
        HAL_Delay(POMPKA_CZAS_MS - elapsed);
    HAL_GPIO_WritePin(GPIOA, PIN_POMPKA, GPIO_PIN_RESET);

    printf("[STRZAL] Koniec — t_total=%lu ms\r\n",
           HAL_GetTick() - t_start);
}
