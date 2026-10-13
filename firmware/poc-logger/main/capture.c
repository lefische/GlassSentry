// Ereigniserfassung.
//
// Ein Trigger merkt sich die Sample-Indizes beider Datenströme. Sobald der
// Nachlauf vollständig im Ringpuffer liegt, kopiert der Capture-Task das
// Fenster [Trigger - Vorlauf, Trigger + Nachlauf) und sendet es als Frame:
//
//   {"type":"event", ...}\n  +  Payload
//   Payload = Piezo-Kanal 0 (uint16 × n), Piezo-Kanal 1 (uint16 × n),
//             Accel (int16 × 3 × m, x/y/z verschachtelt), little endian
//
// Zusätzlich gehen regelmäßig Statusmeldungen mit Rauschpegeln raus.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"
#include "capture.h"
#include "piezo_adc.h"
#include "lis3dh.h"
#include "net.h"

static const char *TAG = "capture";

typedef enum { STATE_IDLE, STATE_PENDING } state_t;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile state_t s_state = STATE_IDLE;
static uint32_t s_trig_piezo;
static uint32_t s_trig_accel;
static gs_source_t s_src;
static int64_t s_trig_time_us;
static int64_t s_rearm_at_us;
static volatile bool s_status_requested;

static uint32_t s_seq;
static uint32_t s_dropped;      // Ereignisse ohne verbundenen Client
static uint32_t s_truncated;    // Ereignisse, deren Vorlauf schon überschrieben war

static float s_piezo_rate = GS_PIEZO_RATE_HZ;
static float s_accel_rate = GS_ACCEL_RATE_HZ;

static uint16_t s_piezo_buf[GS_PIEZO_CHANNELS][GS_PIEZO_EVT_N];
static int16_t s_accel_buf[GS_ACCEL_EVT_N * 3];
static char s_json[768];

static const char *source_name(gs_source_t src)
{
    switch (src) {
    case GS_SRC_PIEZO0: return "piezo0";
    case GS_SRC_PIEZO1: return "piezo1";
    case GS_SRC_ACCEL:  return "accel";
    case GS_SRC_MANUAL: return "manual";
    case GS_SRC_NOISE:  return "noise";
    }
    return "unknown";
}

static void arm_trigger(gs_source_t src, uint32_t piezo_idx, uint32_t accel_idx)
{
    const int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&s_mux);
    if (s_state == STATE_IDLE && now >= s_rearm_at_us) {
        s_state = STATE_PENDING;
        s_src = src;
        s_trig_piezo = piezo_idx;
        s_trig_accel = accel_idx;
        s_trig_time_us = now;
    }
    portEXIT_CRITICAL(&s_mux);
}

void capture_trigger_piezo(int ch, uint32_t piezo_idx)
{
    if (s_state != STATE_IDLE) {
        return;
    }
    arm_trigger(ch == 0 ? GS_SRC_PIEZO0 : GS_SRC_PIEZO1, piezo_idx, lis3dh_count());
}

void capture_trigger_accel(uint32_t accel_idx)
{
    if (s_state != STATE_IDLE) {
        return;
    }
    arm_trigger(GS_SRC_ACCEL, piezo_count(), accel_idx);
}

void capture_trigger_now(gs_source_t src)
{
    arm_trigger(src, piezo_count(), lis3dh_count());
}

void capture_request_status(void)
{
    s_status_requested = true;
}

static bool window_complete(void)
{
    if (piezo_count() - s_trig_piezo < GS_PIEZO_POST_N) {
        return false;
    }
    if (lis3dh_present() && lis3dh_count() - s_trig_accel < GS_ACCEL_POST_N) {
        return false;
    }
    return true;
}

static void send_event(void)
{
    const uint32_t p_start = s_trig_piezo - GS_PIEZO_PRE_N;
    const uint32_t a_start = s_trig_accel - GS_ACCEL_PRE_N;
    const bool accel = lis3dh_present();

    // Ist der Vorlauf bereits überschrieben? (Task zu spät dran)
    bool truncated = piezo_count() - p_start > GS_PIEZO_RING_LEN;
    if (accel && lis3dh_count() - a_start > GS_ACCEL_RING_LEN) {
        truncated = true;
    }

    for (int ch = 0; ch < GS_PIEZO_CHANNELS; ch++) {
        piezo_copy(ch, p_start, GS_PIEZO_EVT_N, s_piezo_buf[ch]);
    }
    const uint32_t accel_n = accel ? GS_ACCEL_EVT_N : 0;
    if (accel) {
        lis3dh_copy(a_start, accel_n, s_accel_buf);
    }

    const size_t piezo_bytes = sizeof(s_piezo_buf);
    const size_t accel_bytes = accel_n * 3 * sizeof(int16_t);

    char accel_json[160] = "null";
    if (accel) {
        snprintf(accel_json, sizeof(accel_json),
                 "{\"n\":%lu,\"axes\":3,\"rate\":%d,\"rate_meas\":%.1f,\"dtype\":\"int16\",\"mg_per_lsb\":%d}",
                 (unsigned long)accel_n, GS_ACCEL_RATE_HZ, s_accel_rate, GS_ACCEL_MG_PER_LSB);
    }

    const int len = snprintf(s_json, sizeof(s_json),
        "{\"type\":\"event\",\"device\":\"%s\",\"fw\":\"%s\",\"seq\":%lu,"
        "\"uptime_ms\":%lld,\"source\":\"%s\",\"pre_ms\":%d,\"post_ms\":%d,"
        "\"truncated\":%s,\"thr_piezo\":%u,\"thr_accel_mg\":%u,"
        "\"piezo\":{\"channels\":%d,\"n\":%d,\"rate\":%d,\"rate_meas\":%.1f,\"dtype\":\"uint16\"},"
        "\"accel\":%s,\"payload_bytes\":%u}",
        GS_DEVICE_NAME, GS_FW_VERSION, (unsigned long)s_seq,
        (long long)(s_trig_time_us / 1000), source_name(s_src), GS_PRE_MS, GS_POST_MS,
        truncated ? "true" : "false", piezo_get_threshold(), lis3dh_get_threshold_mg(),
        GS_PIEZO_CHANNELS, GS_PIEZO_EVT_N, GS_PIEZO_RATE_HZ, s_piezo_rate,
        accel_json, (unsigned)(piezo_bytes + accel_bytes));

    const bool ok = net_send_frame(s_json, len, s_piezo_buf, piezo_bytes, s_accel_buf, accel_bytes);

    if (truncated) {
        s_truncated++;
    }
    if (!ok) {
        s_dropped++;
    }
    ESP_LOGI(TAG, "Ereignis %lu (%s)%s%s", (unsigned long)s_seq, source_name(s_src),
             truncated ? " [gekürzt]" : "", ok ? "" : " [nicht gesendet]");
    s_seq++;
}

static void send_status(void)
{
    piezo_stats_t ps;
    piezo_take_stats(&ps);
    s_piezo_rate = ps.rate_hz;

    accel_stats_t as = { 0 };
    if (lis3dh_present()) {
        lis3dh_take_stats(&as);
        s_accel_rate = as.rate_hz;
    }

    const int len = snprintf(s_json, sizeof(s_json),
        "{\"type\":\"status\",\"device\":\"%s\",\"fw\":\"%s\",\"uptime_s\":%lld,\"rssi\":%d,"
        "\"thr_piezo\":%u,\"thr_accel_mg\":%u,"
        "\"piezo_rate\":%.1f,\"piezo_base\":[%.1f,%.1f],\"piezo_rms\":[%.2f,%.2f],\"adc_overflows\":%lu,"
        "\"accel\":%s,\"accel_rate\":%.1f,\"accel_rms_mg\":%.2f,\"accel_overruns\":%lu,"
        "\"events\":%lu,\"dropped\":%lu,\"truncated\":%lu,\"payload_bytes\":0}",
        GS_DEVICE_NAME, GS_FW_VERSION, (long long)(esp_timer_get_time() / 1000000), net_rssi(),
        piezo_get_threshold(), lis3dh_get_threshold_mg(),
        ps.rate_hz, ps.base[0], ps.base[1], ps.rms[0], ps.rms[1], (unsigned long)ps.overflows,
        lis3dh_present() ? "true" : "false", as.rate_hz, as.rms_mg, (unsigned long)as.overruns,
        (unsigned long)s_seq, (unsigned long)s_dropped, (unsigned long)s_truncated);

    net_send_frame(s_json, len, NULL, 0, NULL, 0);
}

static void capture_task(void *arg)
{
    int64_t next_status = esp_timer_get_time() + GS_STATUS_INTERVAL_S * 1000000LL;
    int64_t next_noise = GS_NOISE_CAPTURE_INTERVAL_S
        ? esp_timer_get_time() + GS_NOISE_CAPTURE_INTERVAL_S * 1000000LL : INT64_MAX;
    uint32_t last_conn = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5));
        const int64_t now = esp_timer_get_time();

        if (s_state == STATE_PENDING && window_complete()) {
            send_event();
            portENTER_CRITICAL(&s_mux);
            s_rearm_at_us = esp_timer_get_time() + GS_HOLDOFF_MS * 1000LL;
            s_state = STATE_IDLE;
            portEXIT_CRITICAL(&s_mux);
        }

        // Neuer Client: sofort eine Statusmeldung schicken
        const uint32_t conn = net_connection_count();
        if (conn != last_conn) {
            last_conn = conn;
            s_status_requested = true;
        }

        if (now >= next_status || s_status_requested) {
            s_status_requested = false;
            next_status = now + GS_STATUS_INTERVAL_S * 1000000LL;
            send_status();
        }

        if (now >= next_noise) {
            next_noise = now + GS_NOISE_CAPTURE_INTERVAL_S * 1000000LL;
            capture_trigger_now(GS_SRC_NOISE);
        }
    }
}

void capture_start(void)
{
    xTaskCreatePinnedToCore(capture_task, "capture", 6144, NULL, 6, NULL, 0);
}
