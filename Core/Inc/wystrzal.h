// wystrzal.h
#ifndef WYSTRZAL_H
#define WYSTRZAL_H

void wystrzal_pwm_init(void);   // wywolaj raz w main() po MX_GPIO_Init()
void generator_wl(void);        // wlacz generator dymu (poczatek sekwencji)
void generator_wyl(void);       // wylacz generator dymu (koniec sekwencji)
void wystrzel(void);            // PWM + LED + pompka

#endif
