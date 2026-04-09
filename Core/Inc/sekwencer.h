// sekwencer.h
#ifndef SEKWENCER_H
#define SEKWENCER_H

#include <stdint.h>

typedef struct {
    float    kat_os1_deg;
    float    kat_os2_deg;
    uint32_t pauza_ms;
} Pozycja;

void sekwencer_run(const Pozycja *seq, uint16_t len);

#endif
