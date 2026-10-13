// Piezo-Erfassung über ADC1 im Dauerbetrieb (DMA) mit Ringpuffer
#pragma once

#include <stdint.h>
#include "config.h"

typedef struct {
    float rate_hz;                        // gemessene Abtastrate pro Kanal
    float base[GS_PIEZO_CHANNELS];        // Ruhepegel (ADC-Counts)
    float rms[GS_PIEZO_CHANNELS];         // Rauschen um den Ruhepegel (ADC-Counts)
    uint32_t overflows;                   // verworfene DMA-Frames seit Start
} piezo_stats_t;

void piezo_adc_start(void);

// Index des nächsten zu schreibenden Samples (Kanal 0), läuft modulo 2^32
uint32_t piezo_count(void);

// Kopiert n Samples ab Index start aus dem Ringpuffer von Kanal ch
void piezo_copy(int ch, uint32_t start, uint32_t n, uint16_t *dst);

void piezo_set_threshold(uint16_t counts);
uint16_t piezo_get_threshold(void);

// Liefert Statistik seit dem letzten Aufruf und setzt das Messfenster zurück
void piezo_take_stats(piezo_stats_t *out);
