// Ereigniserfassung: Trigger, Fensterbildung, Versand
#pragma once

#include <stdint.h>

typedef enum {
    GS_SRC_PIEZO0 = 0,
    GS_SRC_PIEZO1 = 1,
    GS_SRC_ACCEL  = 2,
    GS_SRC_MANUAL = 3,
    GS_SRC_NOISE  = 4,
} gs_source_t;

void capture_start(void);

// Werden aus den Erfassungs-Tasks aufgerufen, sobald eine Schwelle überschritten ist
void capture_trigger_piezo(int ch, uint32_t piezo_idx);
void capture_trigger_accel(uint32_t accel_idx);
// Auslösung ohne Schwelle (Befehl "trigger" bzw. periodische Ruheaufnahme)
void capture_trigger_now(gs_source_t src);

void capture_request_status(void);
