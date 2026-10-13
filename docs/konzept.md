# GlassSentry – Konzept & Lösungsraum

Dieses Dokument sammelt alle Aspekte, die für ein System zur Erkennung von
Vogelschlag an Fenstern relevant sind. Es ist bewusst breit angelegt und wird
nach der Klärung der [offenen Fragen](offene-fragen.md) zu einer konkreten
Architektur verdichtet.

> **Stand 2026-10-13:** Die ersten Rahmenbedingungen sind geklärt: 3-fach-Glas,
> Batteriesensoren mit Gateway am Netz, Ziel ist die Statistik. Daraus abgeleitete
> Festlegungen und angepasste Prioritäten stehen in [anforderungen.md](anforderungen.md).
> Diese haben Vorrang vor den allgemeinen Optionen unten.

---

## 1. Ziele

| # | Ziel | Priorität |
|---|------|-----------------------|
| Z1 | Vogelschlag zuverlässig erkennen (Zeitpunkt, welches Fenster) | Muss |
| Z2 | Ereignis an Home Assistant melden und dort protokollieren | Muss |
| Z3 | Sofort-Benachrichtigung, damit man nach einem benommenen Vogel sehen kann | Kann |
| Z4 | Stärke des Aufpralls schätzen (leichter Streifer vs. harter Aufprall) | Soll |
| Z5 | Statistik: Häufung nach Fenster, Uhrzeit, Jahreszeit, Sonnenstand | Muss |
| Z6 | Wirksamkeit von Schutzmaßnahmen (Markierungen, Folien) vorher/nachher bewerten | Soll |
| Z7 | Fotobeleg / Artbestimmung per Kamera | Kann |

**Nicht-Ziele (vorerst):** Vögel aktiv vertreiben, Erkennung von Einbruch/Glasbruch
(dafür gibt es zertifizierte Melder).

---

## 2. Physik eines Vogelschlags

Was wir messen wollen, und wie es sich von Störquellen unterscheidet.

| Größe | Typischer Wert |
|-------|----------------|
| Masse | Meise/Rotkehlchen 10–20 g · Amsel/Star 80–100 g · Taube 300–500 g |
| Geschwindigkeit | 5–12 m/s (≈ 20–40 km/h) |
| Kinetische Energie | 20 g @ 8 m/s ≈ 0,6 J · 100 g @ 10 m/s ≈ 5 J |
| Kontaktdauer | wenige Millisekunden (weicher Körper, Federn) |
| Signalbild | **ein einzelner** kurzer Impuls, danach Ausklingen der Scheibe (Eigenschwingung, einige 100 Hz bis wenige kHz) |
| Akustik | dumpfes „Plopp/Klonk“, oft gut im Raum hörbar |
| Spuren | häufig Abdruck aus Federstaub auf der Scheibe |

### Störquellen (False Positives)

| Quelle | Unterscheidungsmerkmal |
|--------|------------------------|
| Klopfen mit der Hand | meist mehrere Impulse in kurzer Folge; oft von innen |
| Tür zuschlagen / Schritte | betrifft **alle** Fenster gleichzeitig (Gleichtakt), tieffrequent, über den Rahmen |
| Haustiere, Roboter, Gartenmöbel (bodentiefe Scheiben) | Einzelimpuls möglich → Putz-/Scharf-Schalter, Labeling, ggf. Kamera |
| Wind / Böen | langsamer Druckverlauf (< 20 Hz), kein scharfer Impuls |
| Regen | sehr viele, sehr schwache Impulse (Tropfen ≈ 1 mJ) |
| **Hagel** | Impulse ähnlich stark wie kleine Vögel (2-cm-Korn ≈ 0,8 J), aber in Serie → über Wetterdaten aus HA ausschließbar |
| Insekten, Blätter | deutlich zu schwach |
| Ball, Steinchen, Kinder | härter/kürzer als ein Vogel, kaum sicher unterscheidbar → Kamera hilft |
| Fenster öffnen/schließen | Fensterkontakt-Sensor in HA → Erkennung kurz sperren |
| Verkehr, LKW, Bauarbeiten | tieffrequent, länger andauernd |

---

## 3. Sensorprinzipien

### 3.1 Körperschall / Vibration an der Scheibe (Favorit als Primärsensor)

Ein Sensor wird innen in eine Ecke der Scheibe geklebt und misst die
Erschütterung.

| Variante | Vorteile | Nachteile |
|----------|----------|-----------|
| **MEMS-Beschleunigungssensor** (z. B. ADXL362, LIS2DW12, LIS3DH, ADXL345) | digital, kalibriert, Hardware-Schwellwert-Interrupt, FIFO für Vorlauf-Daten, sehr stromsparend | Abtastrate begrenzt (ca. 400 Hz – 3,2 kHz je nach Typ) |
| **Piezo-Scheibe** (Kontaktmikrofon) | extrem empfindlich, hohe Bandbreite, praktisch kein Stromverbrauch, wenige Cent | analog, braucht Verstärker/Komparator, nicht kalibriert |

- **Pro:** Direkte Messung, ein Ereignis ist eindeutig einer Scheibe zugeordnet,
  günstig, batteriefähig, keine Datenschutzprobleme.
- **Contra:** Ein Sensor pro Scheibe, sichtbar (klein, ~2×2 cm), Kabel/Gehäuse am Rahmen.
- **Wichtig – Mehrfachverglasung:** Der Vogel trifft die äußere Scheibe, der Sensor
  klebt auf der inneren. Der Impuls wird über Randverbund und Gasfüllung
  gedämpft übertragen. Bei 3-fach-Verglasung muss die Empfindlichkeit erst
  gemessen werden → **frühes Experiment im Prototyp**.
- Befestigung: steif koppeln (Cyanacrylat, Epoxid, dünnes hartes Klebepad).
  Weiches Schaum-Klebeband (VHB) dämpft hohe Frequenzen.

### 3.2 Mikrofon im Raum

- I²S-MEMS-Mikrofon (z. B. INMP441, SPH0645) plus Klassifikation des Geräuschs.
- **Pro:** Ein Sensor kann mehrere Fenster eines Raums abdecken.
- **Contra:** Viele Störgeräusche (Musik, Gespräche, Haushalt), Zuordnung zum
  Fenster schwierig, Datenschutz (Mikrofon im Wohnraum), höherer Stromverbrauch,
  ML-Modell nötig.
- Sinnvoll eher als **Zusatzmerkmal** zur Bestätigung.

### 3.3 Kamera

- Außen- oder Innenkamera mit Blick auf/entlang der Scheibe, Ringpuffer der
  letzten Sekunden; Aufnahme wird **durch den Vibrationssensor ausgelöst**.
- Optional Artbestimmung (z. B. Frigate + eigenes Modell, BirdNET-ähnliche
  Bildmodelle).
- **Pro:** Beleg, Artbestimmung, Ausschluss von Fehlalarmen (Ball, Mensch).
- **Contra:** Aufwand, Strom, Nacht, Spiegelung, **DSGVO** wenn öffentlicher
  Raum oder Nachbargrundstück erfasst wird.
- Bereits vorhandene Kameras (in HA) können evtl. einfach mitgenutzt werden.

### 3.4 Weitere (eher verworfen)

| Prinzip | Warum eher nicht |
|---------|------------------|
| PIR-Bewegungsmelder | funktioniert nicht durch Glas, zu träge |
| Lichtschranke vor der Scheibe | Montage außen, Fehlauslösungen durch Insekten/Regen |
| mmWave-Radar nach außen | technisch spannend (Anflug erkennen), aber aufwändig und unsicher |
| Fertige Glasbruchmelder | auf Bruch optimiert, kein Rohsignal, schlecht anpassbar |

### Empfehlung (Vorschlag zur Diskussion)

**Mehrstufig:**
1. **Pflicht:** Vibrationssensor pro relevanter Scheibe → erkennt Ereignis.
2. **Plausibilisierung:** Gleichtakt-Unterdrückung über mehrere Sensoren,
   Kontext aus HA (Wetter/Hagel, Fensterkontakt, Anwesenheit).
3. **Optional:** Kamera-Snapshot / Clip als Beleg.

---

## 4. Signalverarbeitung & Erkennung

### 4.1 Erfassung

- Sensor im Schwellwert-Modus (Hardware-Interrupt) → System schläft bis zum Ereignis.
- FIFO/Ringpuffer liefert **Pre-Trigger-Daten** (z. B. 50 ms vor, 300 ms nach dem Trigger).
- Abtastrate: ≥ 1 kHz, besser 3–5 kHz, um die Scheibenschwingung aufzulösen.

### 4.2 Merkmale pro Ereignis

| Merkmal | Bedeutung |
|---------|-----------|
| Spitzenwert (g bzw. ADC) | Stärke |
| Signalenergie (Σ x²) | robustere Stärke |
| Anstiegszeit | Vogel: weich; Stein/Klopfen: hart |
| Ausklingzeit | Eigenschwingung der Scheibe |
| Dominante Frequenz / spektraler Schwerpunkt | Art der Anregung |
| Anzahl Impulse in 1 s | Vogel = 1, Klopfen ≥ 2, Regen/Hagel ≫ |
| Gleichzeitigkeit mit anderen Sensoren | Gleichtakt = Haus-Erschütterung |

### 4.3 Klassifikation – Ausbaustufen

1. **Regelbasiert:** Schwellwerte + Einzelimpuls-Regel + Gleichtakt-Ausschluss.
2. **Statistisch:** Schwellwerte aus gesammelten, gelabelten Daten ableiten.
3. **TinyML** (optional): kleines Modell auf dem Mikrocontroller oder in HA/Server,
   trainiert mit eigenen Daten (z. B. Edge Impulse, TensorFlow Lite Micro).

### 4.4 Kalibrierung & Tests – ohne Vögel zu gefährden

- **Pendel-Prüfstand:** Weiches Säckchen (Stoff mit Reis/Sand, 20 g / 50 g / 100 g)
  an einer Schnur, definierte Auslenkung → reproduzierbare Energie.
  E = m · g · h, z. B. 20 g aus 1 m Höhe ≈ 0,2 J; für 0,6 J ≈ 3 m oder größere Masse.
- Gezielte Störtests: Klopfen, Tür knallen, Fenster schließen, Gießkanne als „Regen“.
- Alle Rohdaten speichern und **labeln** → Grundlage für Schwellwerte und Modell.
- Ein „Labeling“-Button in HA („War ein Vogel“ / „Fehlalarm“) erzeugt Trainingsdaten
  aus dem Echtbetrieb.

---

## 5. Hardware

### 5.1 Mikrocontroller & Funk

| Option | Funk | Pro | Contra |
|--------|------|-----|--------|
| ESP32-S3 / ESP32-C3 | WLAN | ESPHome/MQTT, viel Rechenleistung, günstig | WLAN-Verbindungsaufbau kostet Strom |
| ESP32-C6 / ESP32-H2 | Zigbee / Thread (+WLAN beim C6) | stromsparend, Mesh, Matter möglich | Firmware aufwändiger |
| nRF52840 | BLE / Thread / Zigbee | sehr stromsparend | eigene Firmware nötig |
| ESP-NOW (ESP32 ↔ ESP32) | proprietär, 2,4 GHz | kein WLAN-Login, sehr kurze Aktivzeit, Gateway leitet weiter | braucht ein Gateway |

→ Abhängig von der vorhandenen Infrastruktur (siehe offene Fragen).

### 5.2 Topologie

- **Variante A – ein Knoten pro Fenster:** Sensor + MCU + Batterie in einem
  kleinen Gehäuse am Rahmen/an der Scheibe. Unabhängig, kabellos.
- **Variante B – ein Knoten pro Raum:** Mehrere Piezos/Sensoren per dünnem Kabel
  an einem MCU mit USB-Stromversorgung. Günstiger, Gleichtakt-Erkennung lokal,
  aber Kabel über bewegliche Fensterflügel.

### 5.3 Energie

- Batteriebetrieb ist realistisch, weil Ereignisse selten sind:
  - ADXL362 im Wake-up-Modus: < 1 µA; ESP32 im Deep Sleep: ~10 µA.
  - Pro Ereignis: Aufwachen, Daten auslesen, Funk senden (~1–3 s aktiv).
  - Herzschlag (Batteriestand, „lebe noch“) z. B. alle 6–12 h.
  - Erwartung: Monate bis > 1 Jahr mit Li-Ion/LiFePO₄ oder 2×AA (zu verifizieren).
- **Achtung:** Bei Regen/Hagel häufige Aufwachvorgänge → Energiesparlogik nötig.
- Alternative: USB-Netzteil, falls Steckdose in Fensternähe.

### 5.4 Mechanik

- Kleines Gehäuse (3D-Druck), Sensor steif auf Glas gekoppelt.
- Wahrscheinlich innen montiert (wetterfrei); außen nur bei schlechter
  Übertragung durch Mehrfachverglasung.
- Fenster öffnen/kippen darf nicht stören (Kabelführung, Gewicht).

---

## 6. Home-Assistant-Anbindung

### 6.1 Transportweg (Optionen)

| Weg | Bemerkung |
|-----|-----------|
| **MQTT mit Auto-Discovery** | flexibel, Rohdaten/JSON-Payloads möglich, eigene Firmware |
| **ESPHome** | sehr bequem in HA, aber hochfrequente Signalverarbeitung braucht eine Custom Component |
| Zigbee (Z2M/ZHA) | nur mit eigenem Gerät + Konverter, wenig Payload |
| Eigene HA-Integration (custom_component) | später, falls sinnvoll |

### 6.2 Entitäten pro Fenster (Entwurf)

| Entität | Typ | Inhalt |
|---------|-----|--------|
| `event.glasssentry_<fenster>_aufprall` | event | Ereignis mit Attributen (Stärke, Klasse, Konfidenz) |
| `sensor.glasssentry_<fenster>_letzte_staerke` | sensor | Stärke des letzten Aufpralls |
| `sensor.glasssentry_<fenster>_letzter_aufprall` | sensor (timestamp) | Zeitpunkt |
| `sensor.glasssentry_<fenster>_batterie` | sensor | % / V |
| `sensor.glasssentry_<fenster>_signal` | sensor | RSSI / LQI |
| `number.glasssentry_<fenster>_schwelle` | number | Empfindlichkeit aus HA einstellbar |
| `switch.glasssentry_<fenster>_scharf` | switch | Erkennung an/aus (z. B. beim Fensterputzen) |
| `button.glasssentry_<fenster>_testmodus` | button | Rohdatenaufnahme für Kalibrierung |

### 6.3 Datenmodell eines Ereignisses (Entwurf, JSON)

```json
{
  "device": "wohnzimmer_sued",
  "ts": "2026-10-13T08:42:17.312Z",
  "peak_g": 1.84,
  "energy": 0.0213,
  "duration_ms": 62,
  "dominant_hz": 410,
  "impulses": 1,
  "class": "bird",
  "confidence": 0.86,
  "fw": "0.1.0",
  "raw_ref": "2026-10-13T084217_wohnzimmer_sued.bin"
}
```

Rohdaten (einige KB) werden nicht in den HA-Recorder geschrieben, sondern
separat abgelegt (z. B. Datei-Upload, MQTT-Topic mit Archivierung, InfluxDB).

### 6.4 Protokollierung & Auswertung

- HA-Recorder / Logbuch für Ereignisse.
- Langzeit: Long-Term-Statistics, optional InfluxDB + Grafana.
- Auswertungen: Aufpralle pro Fenster/Monat, Tageszeit, Korrelation mit
  Sonnenstand (Spiegelung!), Zugzeiten (März–Mai, Aug.–Okt.).

### 6.5 Automationen (Ideen)

- Push-Nachricht mit Kamerabild: „Vogelschlag am Fenster Wohnzimmer Süd – bitte nachsehen“.
- Kontext-Unterdrückung: Hagel/Starkregen laut Wetterintegration, Fenster geöffnet,
  Fensterputz-Modus.
- Erinnerung mit Erste-Hilfe-Hinweis: benommenen Vogel in einen dunklen,
  belüfteten Karton an einen ruhigen Ort setzen, nach 1–2 h draußen freilassen,
  kein Futter/Wasser einflößen; bei Verletzung Wildvogelstation kontaktieren.
- Hinweis bei wiederholten Treffern an einem Fenster → Schutzmaßnahme empfehlen.

---

## 7. Datenschutz & Rechtliches

- Vibrationssensoren: unkritisch.
- Mikrofon: nur lokale Auswertung, keine Speicherung von Audio aus dem Wohnraum.
- Kamera: Erfassung öffentlicher Flächen/Nachbargrundstücke vermeiden (DSGVO, § 4 BDSG).
- Mietwohnung: Klebemontage rückstandsfrei lösbar wählen.

---

## 8. Vorgehen / Roadmap (Vorschlag)

| Phase | Inhalt | Ergebnis |
|-------|--------|----------|
| 0 | Anforderungen klären (offene Fragen) | Entscheidungen dokumentiert |
| 1 | **Machbarkeits-Prototyp:** ESP32 + Beschleunigungssensor + Piezo an *einem* Fenster, Rohdaten per WLAN streamen | Messkurven: Pendel vs. Klopfen vs. Tür, Dämpfung durch Verglasung |
| 2 | Erkennungsalgorithmus (regelbasiert) aus Messdaten | Schwellwerte, Fehlalarmrate |
| 3 | HA-Integration (MQTT-Discovery), Dashboard, Benachrichtigung | Ende-zu-Ende-Pfad |
| 4 | Energieoptimierung, Gehäuse, Ausrollen auf weitere Fenster | Serienstand |
| 5 | Optional: Kamera-Verknüpfung, ML-Klassifikator, Statistik-Dashboard | Ausbau |

---

## 9. Risiken

| Risiko | Gegenmaßnahme |
|--------|---------------|
| Signal durch 3-fach-Verglasung zu schwach | früh messen (Phase 1); ggf. Sensor außen oder Piezo |
| Zu wenige echte Ereignisse zum Validieren | Pendel-Prüfstand; Labeling im Echtbetrieb über lange Zeit |
| Fehlalarme nerven → System wird ignoriert | Kontextfilter, Benachrichtigung erst ab Konfidenz X |
| Batterielaufzeit bei Unwetter | Ereignis-Rate begrenzen, Sammelmeldung |
| Optik/Akzeptanz (sichtbarer Sensor) | kleine Bauform, Ecke der Scheibe |
