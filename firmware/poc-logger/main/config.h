// GlassSentry PoC-Logger – Konfiguration
#pragma once

#include "hal/adc_types.h"

#define GS_FW_VERSION         "0.1.0"
#define GS_DEVICE_NAME        "poc1"
#define GS_HOSTNAME           "glasssentry-poc1"
#define GS_TCP_PORT           5005

// ---------------------------------------------------------------------------
// Piezo-Kanäle (ADC1, Dauerbetrieb per DMA)
// ESP32-S3: ADC1_CH0 = GPIO1, ADC1_CH1 = GPIO2
// ---------------------------------------------------------------------------
#define GS_PIEZO_CHANNELS     2
#define GS_PIEZO_ADC_CH0      ADC_CHANNEL_0
#define GS_PIEZO_ADC_CH1      ADC_CHANNEL_1
#define GS_PIEZO_RATE_HZ      20000          // pro Kanal
#define GS_PIEZO_RING_LEN     16384          // Zweierpotenz, ~0,82 s bei 20 kHz

// ---------------------------------------------------------------------------
// LIS3DH-Beschleunigungssensor an SPI2 (FSPI-Standardpins des S3)
// ---------------------------------------------------------------------------
#define GS_LIS_ENABLED        1
#define GS_LIS_PIN_SCLK       12
#define GS_LIS_PIN_MOSI       11              // SDA/SDI am Breakout
#define GS_LIS_PIN_MISO       13              // SDO am Breakout
#define GS_LIS_PIN_CS         10
#define GS_LIS_FAST_MODE      0               // 0 = 1344 Hz / 12 bit, 1 = 5376 Hz / 8 bit

// ---------------------------------------------------------------------------
// Erfassungsfenster je Ereignis
// ---------------------------------------------------------------------------
#define GS_PRE_MS             100             // Vorlauf vor dem Auslösezeitpunkt
#define GS_POST_MS            400             // Nachlauf
#define GS_HOLDOFF_MS         500             // Sperrzeit nach einem Ereignis

// ---------------------------------------------------------------------------
// Auslöseschwellen (Startwerte, zur Laufzeit per Befehl änderbar)
// ---------------------------------------------------------------------------
#define GS_THR_PIEZO_DEFAULT  80              // ADC-Counts Abweichung vom Ruhepegel
#define GS_THR_ACCEL_DEFAULT  40              // mg Abweichung vom Ruhewert

#define GS_STATUS_INTERVAL_S          10      // Statusmeldung (Rauschpegel usw.)
#define GS_NOISE_CAPTURE_INTERVAL_S   900     // periodische Ruheaufnahme, 0 = aus

// ---------------------------------------------------------------------------
// Abgeleitete Größen
// ---------------------------------------------------------------------------
#if GS_LIS_FAST_MODE
#define GS_ACCEL_RATE_HZ      5376
#define GS_ACCEL_MG_PER_LSB   16
#define GS_ACCEL_RING_LEN     4096            // Zweierpotenz, ~0,76 s
#else
#define GS_ACCEL_RATE_HZ      1344
#define GS_ACCEL_MG_PER_LSB   1
#define GS_ACCEL_RING_LEN     1024            // Zweierpotenz, ~0,76 s
#endif

#define GS_PIEZO_PRE_N   (GS_PIEZO_RATE_HZ * GS_PRE_MS / 1000)
#define GS_PIEZO_POST_N  (GS_PIEZO_RATE_HZ * GS_POST_MS / 1000)
#define GS_PIEZO_EVT_N   (GS_PIEZO_PRE_N + GS_PIEZO_POST_N)
#define GS_ACCEL_PRE_N   (GS_ACCEL_RATE_HZ * GS_PRE_MS / 1000)
#define GS_ACCEL_POST_N  (GS_ACCEL_RATE_HZ * GS_POST_MS / 1000)
#define GS_ACCEL_EVT_N   (GS_ACCEL_PRE_N + GS_ACCEL_POST_N)

_Static_assert((GS_PIEZO_RING_LEN & (GS_PIEZO_RING_LEN - 1)) == 0, "Ringpuffer muss Zweierpotenz sein");
_Static_assert((GS_ACCEL_RING_LEN & (GS_ACCEL_RING_LEN - 1)) == 0, "Ringpuffer muss Zweierpotenz sein");
// Reserve für Verzögerung zwischen Nachlaufende und Kopieren (min. ~250 ms)
_Static_assert(GS_PIEZO_RING_LEN >= GS_PIEZO_EVT_N + GS_PIEZO_RATE_HZ / 4, "Piezo-Ringpuffer zu klein");
_Static_assert(GS_ACCEL_RING_LEN >= GS_ACCEL_EVT_N + GS_ACCEL_RATE_HZ / 4, "Accel-Ringpuffer zu klein");
