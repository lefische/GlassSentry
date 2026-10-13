// WLAN-Station und TCP-Server für den PoC-Logger.
//
// Das Gerät lauscht auf GS_TCP_PORT; genau ein Client (der PC-Empfänger) wird
// bedient. Empfangene Zeilen werden als Befehle an den Callback gegeben.

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "config.h"
#include "net.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "main/secrets.h fehlt: secrets.example.h kopieren und WLAN-Daten eintragen"
#endif

static const char *TAG = "net";

static SemaphoreHandle_t s_lock;
static volatile int s_client = -1;
static volatile uint32_t s_connections;
static net_cmd_cb_t s_on_command;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WLAN getrennt, verbinde neu");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *e = (const ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP " IPSTR ", Port %d, Hostname %s", IP2STR(&e->ip_info.ip), GS_TCP_PORT, GS_HOSTNAME);
    }
}

static void wifi_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif, GS_HOSTNAME);

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t cfg = { 0 };
    strncpy((char *)cfg.sta.ssid, GS_WIFI_SSID, sizeof(cfg.sta.ssid));
    strncpy((char *)cfg.sta.password, GS_WIFI_PASSWORD, sizeof(cfg.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    // Netzbetrieb im PoC: kein Modem-Sleep, damit Ereignisse ohne Verzögerung rausgehen
    esp_wifi_set_ps(WIFI_PS_NONE);
}

static bool send_all(int fd, const void *data, size_t len)
{
    const uint8_t *p = data;
    while (len > 0) {
        const int n = send(fd, p, len, 0);
        if (n < 0) {
            return false;
        }
        p += n;
        len -= n;
    }
    return true;
}

bool net_send_frame(const char *json, size_t json_len,
                    const void *part1, size_t len1, const void *part2, size_t len2)
{
    bool ok = false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    const int fd = s_client;
    if (fd >= 0) {
        ok = send_all(fd, json, json_len) && send_all(fd, "\n", 1);
        if (ok && len1) {
            ok = send_all(fd, part1, len1);
        }
        if (ok && len2) {
            ok = send_all(fd, part2, len2);
        }
        if (!ok) {
            // Empfangsschleife bemerkt das und räumt die Verbindung ab
            shutdown(fd, SHUT_RDWR);
        }
    }
    xSemaphoreGive(s_lock);
    return ok;
}

static void handle_client(int fd)
{
    char line[128];
    size_t len = 0;
    char buf[64];

    while (true) {
        const int n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        for (int i = 0; i < n; i++) {
            const char c = buf[i];
            if (c == '\n') {
                line[len] = '\0';
                if (len > 0 && s_on_command) {
                    s_on_command(line);
                }
                len = 0;
            } else if (c != '\r' && len < sizeof(line) - 1) {
                line[len++] = c;
            }
        }
    }
}

static void server_task(void *arg)
{
    const int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    const int one = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(GS_TCP_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(listen_fd, 1) != 0) {
        ESP_LOGE(TAG, "TCP-Server konnte nicht starten");
        vTaskDelete(NULL);
        return;
    }

    while (true) {
        struct sockaddr_storage peer;
        socklen_t peer_len = sizeof(peer);
        const int fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (fd < 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        const struct timeval tv = { .tv_sec = 20 };
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_client = fd;
        s_connections++;
        xSemaphoreGive(s_lock);
        ESP_LOGI(TAG, "Client verbunden");

        handle_client(fd);

        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_client = -1;
        close(fd);
        xSemaphoreGive(s_lock);
        ESP_LOGI(TAG, "Client getrennt");
    }
}

void net_start(net_cmd_cb_t on_command)
{
    s_on_command = on_command;
    s_lock = xSemaphoreCreateMutex();
    wifi_init();
    xTaskCreatePinnedToCore(server_task, "tcp_server", 4096, NULL, 5, NULL, 0);
}

bool net_client_connected(void)
{
    return s_client >= 0;
}

uint32_t net_connection_count(void)
{
    return s_connections;
}

int net_rssi(void)
{
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        return ap.rssi;
    }
    return 0;
}
