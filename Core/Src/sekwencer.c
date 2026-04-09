// * sekwencer.c * //
#include "flash_config.h"
#include "sekwencer.h"
#include "kalibracja.h"
#include "silniki.h"
#include <stdio.h>

void sekwencer_run(const Pozycja *seq, uint16_t len)
{
    uint16_t krok = 0;

    while (krok < len)
    {


        const Pozycja *p = &seq[krok];
        printf("\r\n[SEQ] Krok %u/%u → OS1=%.1f°  OS2=%.1f°\r\n",
               krok + 1, len, p->kat_os1_deg, p->kat_os2_deg);

        /* Jedź obie osie synchronicznie */
        if (jedz_sync(p->kat_os1_deg, p->kat_os2_deg))
        {


        	printf("[SEQ] Rekalibracja po stall (krok %u)\r\n", krok);
        	wykonaj_kalibracje_pradowa();
        	wykonaj_homing_i_geometrie();

        	KalibracjaFlash kalData = {0};
        	kalData.ms_per_deg_os1 = ms_per_deg_os1;
        	kalData.ms_per_deg_os2 = ms_per_deg_os2;
        	kalData.thresh_ch1_mA  = thresh_ch1_mA;
        	kalData.thresh_ch2_mA  = thresh_ch2_mA;
        	kalData.magic          = FLASH_MAGIC;
        	kalData.crc            = oblicz_crc_pub(&kalData);
        	flash_zapisz_kalibracje(&kalData);
        	printf("[SEQ] Nowa kalibracja zapisana do Flash.\r\n");
        	continue;
        }

        printf("[SEQ] Pozycja OK. Pauza %lu ms\r\n", p->pauza_ms);
        HAL_Delay(p->pauza_ms);

        krok++;
    }
}
