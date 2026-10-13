// LIS3DH-Beschleunigungssensor über SPI.
//
// Der Sensor läuft im FIFO-Stream-Modus; ein Task liest den FIFO alle 2 ms aus,
// schreibt die Samples in einen Ringpuffer und prüft die Auslöseschwelle
// (Abweichung vom gleitenden Ruhewert, also ohne Schwerkraftanteil).

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"
#include "lis3dh.h"
#include "capture.h"

#define REG_WHO_AM_I     0x0F
#define REG_CTRL_REG1    0x20
#define REG_CTRL_REG2    0x21
#define REG_CTRL_REG3    0x22
#define REG_CTRL_REG4    0x23
#define REG_CTRL_REG5    0x24
#define REG_OUT_X_L      0x28
#define REG_FIFO_CTRL    0x2E
#define REG_FIFO_SRC     0x2F
#define WHO_AM_I_VALUE   0x33

#define SPI_READ         0x80
#define SPI_AUTOINC      0x40

#define RING_MASK        (GS_ACCEL_RING_LEN - 1)
#define BASE_SHIFT       8                    // Zeitkonstante Ruhewert: 256 Samples
#define WARMUP_N         GS_ACCEL_RATE_HZ     // 1 s keine Auslösung nach Start

#if GS_LIS_FAST_MODE
#define CTRL1_VALUE      0x9F                 // ODR 5,376 kHz, Low-Power, XYZ an
#define CTRL4_VALUE      0x80                 // BDU, ±2 g
#define DATA_SHIFT       8                    // 8 bit linksbündig
#else
#define CTRL1_VALUE      0x97                 // ODR 1,344 kHz, XYZ an
#define CTRL4_VALUE      0x88                 // BDU, ±2 g, High-Resolution
#define DATA_SHIFT       4                    // 12 bit linksbündig
#endif

static const char *TAG = "lis3dh";

static spi_device_handle_t s_dev;
static bool s_present;
static int16_t s_ring[GS_ACCEL_RING_LEN][3];
static volatile uint32_t s_count;
static volatile uint16_t s_thr_mg = GS_THR_ACCEL_DEFAULT;
static volatile uint32_t s_overruns;

static int32_t s_base_q8[3];
static bool s_base_init;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint64_t s_sumsq;
static uint32_t s_nstat;
static int64_t s_stat_t0_us;
static uint32_t s_stat_count0;

static esp_err_t reg_read(uint8_t reg, uint8_t *data, size_t n)
{
    uint8_t tx[8] = { 0 };
    uint8_t rx[8] = { 0 };
    if (n == 0 || n > sizeof(tx) - 1) {
        return ESP_ERR_INVALID_SIZE;
    }
    tx[0] = reg | SPI_READ | (n > 1 ? SPI_AUTOINC : 0);
    spi_transaction_t t = {
        .length = 8 * (n + 1),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_polling_transmit(s_dev, &t);
    memcpy(data, rx + 1, n);
    return err;
}

static esp_err_t reg_write(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { reg & 0x3F, value };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
    };
    return spi_device_polling_transmit(s_dev, &t);
}

static void process_sample(const uint8_t raw[6], uint64_t *sumsq, uint32_t *nstat)
{
    const uint32_t idx = s_count;
    int32_t dev_max = 0;
    int64_t dev_sq = 0;

    for (int a = 0; a < 3; a++) {
        const int16_t v = (int16_t)((uint16_t)raw[2 * a] | ((uint16_t)raw[2 * a + 1] << 8)) >> DATA_SHIFT;
        s_ring[idx & RING_MASK][a] = v;

        const int32_t x_q8 = (int32_t)v << 8;
        if (!s_base_init) {
            s_base_q8[a] = x_q8;
        }
        s_base_q8[a] += (x_q8 - s_base_q8[a]) >> BASE_SHIFT;
        const int32_t dev = (x_q8 - s_base_q8[a]) >> 8;
        dev_sq += (int64_t)dev * dev;
        if (abs(dev) > dev_max) {
            dev_max = abs(dev);
        }
    }
    s_base_init = true;

    *sumsq += (uint64_t)dev_sq;
    (*nstat)++;

    if (dev_max * GS_ACCEL_MG_PER_LSB > s_thr_mg && idx > WARMUP_N) {
        capture_trigger_accel(idx);
    }
    s_count = idx + 1;
}

static void lis_task(void *arg)
{
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(2));

        uint8_t src = 0;
        if (reg_read(REG_FIFO_SRC, &src, 1) != ESP_OK) {
            continue;
        }
        uint32_t n = src & 0x1F;
        if (src & 0x40) {              // OVRN: FIFO voll, ältere Samples verloren
            s_overruns++;
            n = 32;
        }

        uint64_t sumsq = 0;
        uint32_t nstat = 0;
        for (uint32_t i = 0; i < n; i++) {
            uint8_t raw[6];
            if (reg_read(REG_OUT_X_L, raw, sizeof(raw)) != ESP_OK) {
                break;
            }
            process_sample(raw, &sumsq, &nstat);
        }

        portENTER_CRITICAL(&s_mux);
        s_sumsq += sumsq;
        s_nstat += nstat;
        portEXIT_CRITICAL(&s_mux);
    }
}

bool lis3dh_start(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = GS_LIS_PIN_MOSI,
        .miso_io_num = GS_LIS_PIN_MISO,
        .sclk_io_num = GS_LIS_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 8 * 1000 * 1000,
        .mode = 3,
        .spics_io_num = GS_LIS_PIN_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &s_dev));

    uint8_t who = 0;
    reg_read(REG_WHO_AM_I, &who, 1);
    if (who != WHO_AM_I_VALUE) {
        ESP_LOGW(TAG, "kein LIS3DH gefunden (WHO_AM_I = 0x%02x)", who);
        return false;
    }

    reg_write(REG_CTRL_REG1, CTRL1_VALUE);
    reg_write(REG_CTRL_REG2, 0x00);          // kein interner Hochpass
    reg_write(REG_CTRL_REG3, 0x00);
    reg_write(REG_CTRL_REG4, CTRL4_VALUE);
    reg_write(REG_CTRL_REG5, 0x40);          // FIFO an
    reg_write(REG_FIFO_CTRL, 0x80);          // Stream-Modus

    s_present = true;
    s_stat_t0_us = esp_timer_get_time();
    xTaskCreatePinnedToCore(lis_task, "lis3dh", 4096, NULL, 9, NULL, 1);
    ESP_LOGI(TAG, "LIS3DH gestartet, %d Hz, %d mg/LSB", GS_ACCEL_RATE_HZ, GS_ACCEL_MG_PER_LSB);
    return true;
}

bool lis3dh_present(void)
{
    return s_present;
}

uint32_t lis3dh_count(void)
{
    return s_count;
}

void lis3dh_copy(uint32_t start, uint32_t n, int16_t *dst_xyz)
{
    for (uint32_t k = 0; k < n; k++) {
        const int16_t *s = s_ring[(start + k) & RING_MASK];
        dst_xyz[3 * k + 0] = s[0];
        dst_xyz[3 * k + 1] = s[1];
        dst_xyz[3 * k + 2] = s[2];
    }
}

void lis3dh_set_threshold_mg(uint16_t mg)
{
    s_thr_mg = mg;
}

uint16_t lis3dh_get_threshold_mg(void)
{
    return s_thr_mg;
}

void lis3dh_take_stats(accel_stats_t *out)
{
    portENTER_CRITICAL(&s_mux);
    const uint64_t sumsq = s_sumsq;
    const uint32_t nstat = s_nstat;
    s_sumsq = 0;
    s_nstat = 0;
    portEXIT_CRITICAL(&s_mux);

    const int64_t now = esp_timer_get_time();
    const uint32_t count = s_count;
    const float dt = (now - s_stat_t0_us) / 1e6f;
    out->rate_hz = dt > 0 ? (float)(count - s_stat_count0) / dt : 0;
    s_stat_t0_us = now;
    s_stat_count0 = count;

    out->rms_mg = nstat ? sqrtf((float)((double)sumsq / nstat)) * GS_ACCEL_MG_PER_LSB : 0;
    out->overruns = s_overruns;
}
