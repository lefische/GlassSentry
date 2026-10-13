# PoC-Logger (ESP32-S3)

Erfasst zwei Piezo-Kanäle (20 kHz) und einen LIS3DH-Beschleunigungssensor,
schneidet bei Überschreiten einer Schwelle ein Fenster mit 100 ms Vorlauf und
400 ms Nachlauf aus und sendet es per WLAN/TCP an `tools/gs_receiver.py`.

Aufbau, Schaltung und Messplan: [docs/poc.md](../../docs/poc.md).

## Bauen

1. `main/secrets.example.h` nach `main/secrets.h` kopieren und WLAN-Daten eintragen.
2. ESP-IDF 5.3: `idf.py set-target esp32s3 && idf.py -p <PORT> flash monitor`
   oder PlatformIO: `pio run -t upload && pio device monitor`

Getestet: Build mit ESP-IDF v5.3.1 ohne Warnungen (beide LIS3DH-Modi).

## Module

| Datei | Aufgabe |
|-------|---------|
| `config.h` | Pins, Abtastraten, Fenster, Startschwellen |
| `piezo_adc.c` | ADC1 im DMA-Dauerbetrieb, Ringpuffer, Piezo-Trigger, Rauschstatistik |
| `lis3dh.c` | LIS3DH per SPI, FIFO-Stream, Ringpuffer, Accel-Trigger |
| `capture.c` | Trigger-Zustand, Fensterkopie, Ereignis- und Statusmeldungen |
| `net.c` | WLAN-Station, TCP-Server (Port 5005), Befehlszeilen |
| `main.c` | Start, Befehlsauswertung |

## Protokoll

Jedes Frame ist eine JSON-Zeile, gefolgt von `payload_bytes` Binärdaten:

- `{"type":"status", …}`: alle 10 s mit Ruhepegel, Rauschen, gemessenen Datenraten, RSSI
- `{"type":"event", …}`: Kopf mit Quelle, Raten, Schwellen. Payload: Piezo-Kanäle
  (uint16 × n, nacheinander), danach LIS3DH (int16 × 3 × m, x/y/z verschachtelt), little endian

Befehle an das Gerät (Textzeilen): `thr_piezo <counts>`, `thr_accel <mg>`, `trigger`, `status`.
