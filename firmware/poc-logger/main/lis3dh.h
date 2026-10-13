// LIS3DH-Beschleunigungssensor (SPI) mit FIFO-Auslesung und Ringpuffer
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float rate_hz;         // gemessene Datenrate
    float rms_mg;          // Rauschen (Betrag der Abweichung vom Ruhewert)
    uint32_t overruns;     // FIFO-Überläufe seit Start
} accel_stats_t;

bool lis3dh_start(void);   // false, wenn kein Sensor gefunden
bool lis3dh_present(void);

uint32_t lis3dh_count(void);
// Kopiert n Samples (x,y,z interleaved, Einheit: LSB, siehe GS_ACCEL_MG_PER_LSB)
void lis3dh_copy(uint32_t start, uint32_t n, int16_t *dst_xyz);

void lis3dh_set_threshold_mg(uint16_t mg);
uint16_t lis3dh_get_threshold_mg(void);

void lis3dh_take_stats(accel_stats_t *out);
