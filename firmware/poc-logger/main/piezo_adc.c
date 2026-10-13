// Piezo-Erfassung über ADC1 im Dauerbetrieb (DMA) mit Ringpuffer.
//
// Jedes Sample wird in einen Ringpuffer geschrieben und mit einem gleitenden
// Ruhepegel verglichen. Überschreitet die Abweichung die Schwelle, wird ein
// Ereignis ausgelöst; das Fenster wird später aus dem Ringpuffer kopiert.

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "piezo_adc.h"
#include "capture.h"

#define FRAME_BYTES   256                       // Vielfaches von 4 Byte × Kanalzahl
#define RING_MASK     (GS_PIEZO_RING_LEN - 1)
#define BASE_SHIFT    10                        // Zeitkonstante Ruhepegel: 1024 Samples (~50 ms)
#define WARMUP_N      GS_PIEZO_RATE_HZ          // 1 s keine Auslösung nach Start

static const char *TAG = "piezo";

static const adc_channel_t s_channels[GS_PIEZO_CHANNELS] = { GS_PIEZO_ADC_CH0, GS_PIEZO_ADC_CH1 };

static adc_continuous_handle_t s_handle;
static uint16_t s_ring[GS_PIEZO_CHANNELS][GS_PIEZO_RING_LEN];
static volatile uint32_t s_count[GS_PIEZO_CHANNELS];
static volatile uint16_t s_thr = GS_THR_PIEZO_DEFAULT;
static volatile uint32_t s_overflows;

// Ruhepegel in Q8-Festkomma; nur vom ADC-Task geschrieben
static int32_t s_base_q8[GS_PIEZO_CHANNELS];
static bool s_base_init[GS_PIEZO_CHANNELS];

// Statistik, geschützt durch s_mux
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint64_t s_sumsq[GS_PIEZO_CHANNELS];
static uint32_t s_nstat[GS_PIEZO_CHANNELS];
static int64_t s_stat_t0_us;
static uint32_t s_stat_count0;

static bool on_pool_ovf(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user)
{
    s_overflows++;
    return false;
}

static int channel_index(uint32_t adc_ch)
{
    for (int i = 0; i < GS_PIEZO_CHANNELS; i++) {
        if ((uint32_t)s_channels[i] == adc_ch) {
            return i;
        }
    }
    return -1;
}

static void adc_task(void *arg)
{
    static uint8_t buf[FRAME_BYTES];
    uint64_t sumsq[GS_PIEZO_CHANNELS];
    uint32_t nstat[GS_PIEZO_CHANNELS];

    while (true) {
        uint32_t len = 0;
        esp_err_t err = adc_continuous_read(s_handle, buf, sizeof(buf), &len, portMAX_DELAY);
        if (err != ESP_OK) {
            continue;
        }

        memset(sumsq, 0, sizeof(sumsq));
        memset(nstat, 0, sizeof(nstat));
        const int32_t thr = s_thr;

        for (uint32_t i = 0; i + SOC_ADC_DIGI_RESULT_BYTES <= len; i += SOC_ADC_DIGI_RESULT_BYTES) {
            const adc_digi_output_data_t *p = (const adc_digi_output_data_t *)&buf[i];
            if (p->type2.unit != 0) {
                continue;
            }
            const int ch = channel_index(p->type2.channel);
            if (ch < 0) {
                continue;
            }
            const uint16_t v = p->type2.data;
            const uint32_t idx = s_count[ch];
            s_ring[ch][idx & RING_MASK] = v;

            const int32_t x_q8 = (int32_t)v << 8;
            if (!s_base_init[ch]) {
                s_base_q8[ch] = x_q8;
                s_base_init[ch] = true;
            }
            s_base_q8[ch] += (x_q8 - s_base_q8[ch]) >> BASE_SHIFT;
            const int32_t dev = (x_q8 - s_base_q8[ch]) >> 8;

            sumsq[ch] += (uint64_t)((int64_t)dev * dev);
            nstat[ch]++;

            if ((dev > thr || dev < -thr) && idx > WARMUP_N) {
                capture_trigger_piezo(ch, idx);
            }
            s_count[ch] = idx + 1;
        }

        portENTER_CRITICAL(&s_mux);
        for (int ch = 0; ch < GS_PIEZO_CHANNELS; ch++) {
            s_sumsq[ch] += sumsq[ch];
            s_nstat[ch] += nstat[ch];
        }
        portEXIT_CRITICAL(&s_mux);
    }
}

void piezo_adc_start(void)
{
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = 16 * FRAME_BYTES,     // ~25 ms Puffer bei 40 kHz gesamt
        .conv_frame_size = FRAME_BYTES,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &s_handle));

    adc_digi_pattern_config_t pattern[GS_PIEZO_CHANNELS] = { 0 };
    for (int i = 0; i < GS_PIEZO_CHANNELS; i++) {
        pattern[i].atten = ADC_ATTEN_DB_12;
        pattern[i].channel = s_channels[i];
        pattern[i].unit = ADC_UNIT_1;
        pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }
    adc_continuous_config_t cfg = {
        .pattern_num = GS_PIEZO_CHANNELS,
        .adc_pattern = pattern,
        .sample_freq_hz = GS_PIEZO_RATE_HZ * GS_PIEZO_CHANNELS,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ESP_ERROR_CHECK(adc_continuous_config(s_handle, &cfg));

    adc_continuous_evt_cbs_t cbs = { .on_pool_ovf = on_pool_ovf };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(s_handle, &cbs, NULL));

    s_stat_t0_us = esp_timer_get_time();
    s_stat_count0 = 0;
    ESP_ERROR_CHECK(adc_continuous_start(s_handle));
    xTaskCreatePinnedToCore(adc_task, "piezo_adc", 4096, NULL, 10, NULL, 1);
    ESP_LOGI(TAG, "%d Kanäle à %d Hz gestartet", GS_PIEZO_CHANNELS, GS_PIEZO_RATE_HZ);
}

uint32_t piezo_count(void)
{
    return s_count[0];
}

void piezo_copy(int ch, uint32_t start, uint32_t n, uint16_t *dst)
{
    for (uint32_t k = 0; k < n; k++) {
        dst[k] = s_ring[ch][(start + k) & RING_MASK];
    }
}

void piezo_set_threshold(uint16_t counts)
{
    s_thr = counts;
}

uint16_t piezo_get_threshold(void)
{
    return s_thr;
}

void piezo_take_stats(piezo_stats_t *out)
{
    uint64_t sumsq[GS_PIEZO_CHANNELS];
    uint32_t nstat[GS_PIEZO_CHANNELS];

    portENTER_CRITICAL(&s_mux);
    for (int ch = 0; ch < GS_PIEZO_CHANNELS; ch++) {
        sumsq[ch] = s_sumsq[ch];
        nstat[ch] = s_nstat[ch];
        s_sumsq[ch] = 0;
        s_nstat[ch] = 0;
    }
    portEXIT_CRITICAL(&s_mux);

    const int64_t now = esp_timer_get_time();
    const uint32_t count = s_count[0];
    const float dt = (now - s_stat_t0_us) / 1e6f;
    out->rate_hz = dt > 0 ? (float)(count - s_stat_count0) / dt : 0;
    s_stat_t0_us = now;
    s_stat_count0 = count;

    for (int ch = 0; ch < GS_PIEZO_CHANNELS; ch++) {
        out->base[ch] = s_base_q8[ch] / 256.0f;
        out->rms[ch] = nstat[ch] ? sqrtf((float)((double)sumsq[ch] / nstat[ch])) : 0;
    }
    out->overflows = s_overflows;
}
