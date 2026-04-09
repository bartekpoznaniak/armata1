// * kalibracja.h * //
#include <stdint.h>
#ifndef KALIBRACJA_H
#define KALIBRACJA_H

/* Kąty całkowite osi — zmierzone fizycznie */
#define KAT_OS1_DEG   100.0f   /* GORA/DOL */
#define KAT_OS2_DEG   320.0f   /* CW/CCW   */

/* Progi domyślne z pomiarów (baseline * 1.8) */
#define DEFAULT_THRESH_CH1_MA  60.0f
#define DEFAULT_THRESH_CH2_MA  45.0f

extern float ms_per_deg_os1;
extern float ms_per_deg_os2;

extern float pos_os1;
extern float pos_os2;

void    wykonaj_kalibracje_pradowa(void);
void    wykonaj_homing_i_geometrie(void);
uint8_t jedz_do_kata_os1(float kat_deg);
uint8_t jedz_do_kata_os2(float kat_deg);

extern float ms_per_deg_os1;
extern float ms_per_deg_os2;
extern float pos_os1;
extern float pos_os2;

#endif
