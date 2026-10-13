// GlassSentry PoC-Logger
//
// Erfasst zwei Piezo-Kanäle (20 kHz) und einen LIS3DH (1,3 bzw. 5,4 kHz),
// schneidet bei Überschreiten einer Schwelle ein Fenster mit Vorlauf aus und
// schickt es per TCP an den PC-Empfänger (tools/gs_receiver.py).
//
// Befehle vom Empfänger (eine Zeile je Befehl):
//   thr_piezo <counts>   Piezo-Schwelle setzen
//   thr_accel <mg>       Accel-Schwelle setzen
//   trigger              sofortige Aufnahme ohne Schwelle
//   status               sofortige Statusmeldung

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "config.h"
#include "capture.h"
#include "lis3dh.h"
#include "net.h"
#include "piezo_adc.h"

static const char *TAG = "main";

static void on_command(const char *line)
{
    if (strncmp(line, "thr_piezo ", 10) == 0) {
        const int v = atoi(line + 10);
        if (v > 0 && v < 4096) {
            piezo_set_threshold(v);
        }
    } else if (strncmp(line, "thr_accel ", 10) == 0) {
        const int v = atoi(line + 10);
        if (v > 0 && v < 4000) {
            lis3dh_set_threshold_mg(v);
        }
    } else if (strcmp(line, "trigger") == 0) {
        capture_trigger_now(GS_SRC_MANUAL);
        return;
    } else if (strcmp(line, "status") != 0) {
        ESP_LOGW(TAG, "unbekannter Befehl: %s", line);
        return;
    }
    capture_request_status();
}

void app_main(void)
{
    ESP_LOGI(TAG, "GlassSentry PoC-Logger %s (%s)", GS_FW_VERSION, GS_DEVICE_NAME);

    net_start(on_command);
#if GS_LIS_ENABLED
    lis3dh_start();
#endif
    piezo_adc_start();
    capture_start();
}
