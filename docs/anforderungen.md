# Anforderungen & Rahmenbedingungen

Stand: 2026-10-13 – aus der ersten Abstimmung zu den [offenen Fragen](offene-fragen.md).

## 1. Ausgangslage

### Fenster
- **Erdgeschoss:** 6 bodentiefe Fenster.
- **Obergeschoss:** Fenster mit ca. 1,5 m Höhe (Anzahl noch offen).
- **Verglasung:** durchgehend 3-fach-Isolierglas.

### Energieversorgung
- **Sensoren:** müssen mit Batterie laufen, weil an den Fenstern keine Steckdosen sind.
- **Empfänger/Gateway:** darf am Netz hängen, Steckdosen sind vorhanden.
- **Proof of Concept:** darf mit Netzteil betrieben werden.

### Vorhandene Infrastruktur
- Home Assistant läuft schon.
- Ein Zigbee-Netz läuft schon, mit Türkontakten. An den Fenstern gibt es noch keine Kontakte.
- MQTT ist noch nicht eingerichtet, kann aber eingeführt werden.
- ESPHome wird noch nicht genutzt, kann aber eingeführt werden.

### Zielsetzung
- **Hauptziel ist die Statistik:** Es soll erfasst werden, wie oft, wo und wann Vögel gegen die Fenster fliegen.
- Eine Sofort-Benachrichtigung ist nicht erforderlich.

### Kenntnisse & Werkzeug
- Löten und 3D-Druck sind vorhanden, Gehäuse und Platinen können selbst gebaut werden.
- Bei der Firmware gibt es keine Festlegung: ESPHome (YAML) und eigene Firmware sind beide möglich.

---

## 2. Was daraus folgt

### 2.1 3-fach-Verglasung ist das zentrale technische Risiko
Der Vogel trifft die äußere Scheibe. Der Sensor sitzt innen, also zwei Scheiben
und zwei Gaszwischenräume entfernt. Der Proof of Concept muss deshalb **zuerst** klären,
ob das Signal innen noch sicher messbar ist. Dazu vergleichen wir:

- einen MEMS-Beschleunigungssensor mit einer Piezo-Scheibe,
- mehrere Positionen: Scheibenecke, Mitte der Kante, Rahmen bzw. Flügelprofil,
- ein Test-Pendel (20 / 50 / 100 g) mit Klopfen und Türschlagen.

Falls innen nicht genug ankommt, gibt es zwei Ausweichlösungen: einen Sensor außen auf der Scheibe
(wetterfest) oder am Flügelrahmen.

### 2.2 Bodentiefe Fenster bringen zusätzliche Störquellen
Große, tief reichende Scheiben spiegeln stärker und werden daher häufiger getroffen.
In Bodenhöhe kommen aber neue Fehlauslöser hinzu:

- Haustiere, Kinder, Staubsauger- und Mähroboter,
- Gartenmöbel und Gartengeräte,
- Fensterputzen.

Dafür braucht es einen Putzmodus bzw. einen Schalter zum Scharfstellen in Home Assistant.
Außerdem müssen diese Fälle in den Störtests vorkommen.

Große Scheiben schwingen tiefer und schwächer. Die Schwellwerte müssen deshalb für EG und OG
getrennt kalibriert werden.

### 2.3 Batteriesensoren mit Gateway am Netz
Die Topologie ist damit gesetzt:

```
[Sensor-Knoten pro Scheibe]  --Funk-->  [Gateway (Steckdose)]  -->  Home Assistant
 Batterie, schläft, wacht                 dauerhaft empfangsbereit
 nur bei Erschütterung auf                leitet Ereignisse weiter
```

Für die Funkstrecke vom Sensor zum Gateway gibt es drei Kandidaten:

| Option | Energie pro Ereignis | Datenmenge | Aufwand | Bewertung |
|--------|----------------------|------------|---------|-----------|
| **ESP-NOW** (ESP32 → ESP32-Gateway) | sehr gering (kein WLAN-Login, ~50–100 ms aktiv) | bis 250 Byte pro Paket, mehrere Pakete möglich | gering bis mittel | **Favorit** |
| Zigbee (ESP32-C6/H2 ins bestehende Netz) | gering | klein, Rohdaten kaum möglich | hoch (eigener Z2M/ZHA-Konverter) | Alternative für später |
| WLAN direkt vom Sensor | hoch (Verbindungsaufbau 1–3 s) | beliebig | gering | nur im Proof of Concept |

Das **Gateway** ist ein ESP32 am USB-Netzteil. Es reicht die Ereignisse per **MQTT**
oder **ESPHome** an Home Assistant weiter. Im Proof of Concept darf der Sensor auch direkt per WLAN
senden und am Netzteil hängen.

### 2.4 Statistik als Ziel – Folgen für Erkennung und Datenhaltung
- **Kurze Latenz ist unwichtig.** Ereignisse dürfen gepuffert und gesammelt gesendet werden.
  Das spart Energie.
- **Die Klassifikation muss nicht endgültig sein.** Jeder Kandidat wird mit seinen
  Merkmalen (Stärke, Dauer, Frequenz, Impulsanzahl usw.) gespeichert, nicht nur als
  „Vogel ja/nein“. So lässt sich die Statistik später mit verbesserten Regeln
  **rückwirkend neu berechnen**.
- **Die Daten müssen dauerhaft gespeichert werden.** Der HA-Recorder löscht Verlaufsdaten
  standardmäßig nach 10 Tagen. Wir brauchen deshalb zwei Dinge:
  - Zähler pro Fenster mit `state_class: total_increasing`. Die landen in den
    Long-Term-Statistics und bleiben dauerhaft erhalten.
  - Ein vollständiges Ereignisprotokoll mit Merkmalen, z. B. in InfluxDB oder als
    CSV/SQLite-Export. Die Entscheidung dazu ist noch offen.
- **Wir brauchen echte Bestätigungen (Ground Truth):** Ein „Bestätigen / Fehlalarm“-Button in HA
  macht das möglich, z. B. nach einem Blick auf Federstaub-Abdrücke. Erst damit lässt sich
  die Erkennungsgüte messen.
- Für spätere Auswertungen kommen Kontextdaten hinzu: Sonnenstand, Wetter und Jahreszeit.
  Daraus lässt sich ablesen, wann und warum welches Fenster betroffen ist.

### 2.5 Firmware
- **Sensor-Knoten: eigene Firmware** mit ESP-IDF oder Arduino. Die nötigen Funktionen
  gehen über das hinaus, was ESPHome vorgesehen hat:
  - schnelle Abtastung,
  - Daten vor dem Auslösezeitpunkt per FIFO,
  - Merkmalsberechnung,
  - ESP-NOW,
  - Deep Sleep.
- **Gateway:** ESPHome oder eigene Firmware mit MQTT-Auto-Discovery. Das entscheiden wir im
  Proof of Concept. Ich tendiere zu MQTT, weil es flexible JSON-Daten erlaubt.
- **Zusatznutzen:** Der Beschleunigungssensor erkennt über die Lage zur Schwerkraft,
  ob ein Fenster **gekippt** ist. Damit lassen sich Störungen beim Öffnen und Kippen
  ausblenden, ganz ohne zusätzliche Fensterkontakte. Ein Drehflügel, der geöffnet wird,
  ist so allerdings nicht erkennbar.

---

## 3. Angepasste Ziel-Prioritäten

| # | Ziel | Priorität |
|---|------|-----------|
| Z1 | Vogelschlag erkennen (Zeitpunkt, Fenster) | Muss |
| Z2 | Ereignisse an HA melden und **dauerhaft** protokollieren | Muss |
| Z5 | Statistik nach Fenster, Uhrzeit, Jahreszeit, Sonnenstand | **Muss** |
| Z4 | Stärke des Aufpralls schätzen | Soll |
| Z6 | Wirksamkeit von Schutzmaßnahmen bewerten | **Soll** |
| Z3 | Sofort-Benachrichtigung | **Kann** |
| Z7 | Kamerabeleg / Artbestimmung | Kann |
