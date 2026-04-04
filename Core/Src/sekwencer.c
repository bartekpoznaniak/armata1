// * sekwencer.c * //
#include "sekwencer.h"
#include "kalibracja.h"
#include "silniki.h"
#include <stdio.h>

void sekwencer_run(const Pozycja *seq, uint16_t len)
{
    uint16_t krok = 0;

    while (krok < len)
    {
        /* Sekwencja zakończona — zacznij od nowa */
        if (krok >= len)
        {
            printf("[SEQ] Cykl zakończony. Restart.\r\n\r\n");
            krok = 0;
        }

        const Pozycja *p = &seq[krok];
        printf("\r\n[SEQ] Krok %u/%u → OS1=%.1f°  OS2=%.1f°\r\n",
               krok + 1, len, p->kat_os1_deg, p->kat_os2_deg);

        /* Jedź obie osie synchronicznie */
        if (jedz_sync(p->kat_os1_deg, p->kat_os2_deg))
        {
            printf("[SEQ] Rekalibracja po stall (krok %u)\r\n", krok);
            wykonaj_kalibracje_pradowa();
            wykonaj_homing_i_geometrie();
            continue;
        }

        printf("[SEQ] Pozycja OK. Pauza %lu ms\r\n", p->pauza_ms);
        HAL_Delay(p->pauza_ms);

        krok++;
    }
}
