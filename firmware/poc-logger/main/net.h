// WLAN-Station und TCP-Server (ein Client) für den PoC-Logger
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*net_cmd_cb_t)(const char *line);

void net_start(net_cmd_cb_t on_command);

// Sendet eine JSON-Kopfzeile (ohne '\n') und danach bis zu zwei Binärblöcke
// als ein zusammenhängendes Frame. Liefert false, wenn kein Client verbunden
// ist oder das Senden scheitert.
bool net_send_frame(const char *json, size_t json_len,
                    const void *part1, size_t len1, const void *part2, size_t len2);

bool net_client_connected(void);
// Zählt bei jeder neuen Client-Verbindung hoch
uint32_t net_connection_count(void);
int net_rssi(void);
