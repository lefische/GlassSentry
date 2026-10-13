# Proof of Concept – Plan, Aufbau und Messprogramm

Stand: 2026-10-13. Grundlage: [anforderungen.md](anforderungen.md).

Der Proof of Concept (PoC) beantwortet die Frage, **ob und wie** sich Vogelschlag durch
3-fach-Isolierglas an einem Holz-Alu-Fenster zuverlässig messen lässt. Es geht
noch nicht um Batterie, Gehäuse oder Home Assistant. Der PoC läuft am Netzteil und
schickt **Rohdaten** an einen PC.

---

## 1. Leitfragen und Erfolgskriterien

| # | Frage | Erfolgskriterium |
|---|-------|------------------|
| F1 | Kommt ein Aufprall außen durch das 3-fach-Glas innen messbar an? | Ein Pendelschlag mit **50 g aus 5 cm** (≈ 0,05 Ns, deutlich schwächer als eine Meise) liefert innen ein **SNR ≥ 20 dB** |
| F2 | Wo muss der Sensor sitzen: auf dem Glas, an der Glasleiste oder am Holzrahmen? | Vergleich des SNR je Position. Ein Rahmenplatz ist „gut genug“, wenn er F1 ebenfalls erfüllt |
| F3 | Piezo oder MEMS-Beschleunigungssensor? Reicht eine niedrige Datenrate (Stromsparen)? | Vergleich der SNR-Werte; Simulation von 400 Hz / 100 Hz (`accel_peak_400hz_mg`, `accel_peak_100hz_mg`) |
| F4 | Hängt das Signal stark davon ab, **wo** auf der Scheibe der Aufprall erfolgt? | Streuung des SNR zwischen Mitte, Rand und Ecke |
| F5 | Lassen sich Pendelschläge von Störquellen trennen (Klopfen, Schiebetür, Screen, Türschlagen)? | In mindestens zwei Merkmalen (z. B. Impulsanzahl, Dauer, Frequenz) sichtbar getrennte Gruppen |
| F6 | Wie oft löst das System im Alltag ohne Vogel aus? | Langzeitmessung: Auslösungen pro Tag bei der gewählten Schwelle |

Aus den Antworten folgen die Entscheidungen für den nächsten Schritt (Kapitel 9).

---

## 2. Aufbau im Überblick

```
   außen                         innen
 ┌───────┐  ┌──────────────────────────────────────┐
 │ Pendel│→ │ 3-fach-Glas                           │
 └───────┘  │  ● Piezo A (Glas, Ecke)               │      ┌───────────────┐
            │  ● LIS3DH (Glas, neben Piezo A)       │──────│ ESP32-S3      │  WLAN   ┌──────────────┐
            │                                       │ Kabel│ + Verstärker  │ ──────▶ │ PC/Laptop    │
            │  ● Piezo B (Glasleiste / Holzrahmen)  │──────│ (Lochraster)  │  TCP    │ gs_receiver  │
            └──────────────────────────────────────┘      └───────┬───────┘          └──────────────┘
                                                                  │ USB-Netzteil
```

- **2 Piezo-Kanäle** mit 20 kHz Abtastrate: Damit lassen sich zwei Positionen
  gleichzeitig vergleichen.
- **1 LIS3DH** mit 1,3 kHz (12 bit) oder 5,4 kHz (8 bit): Er ist ein typischer Kandidat
  für den späteren Batteriesensor.
- Die Firmware hält die letzten ~0,8 s im **Ringpuffer**. Bei einer Auslösung schickt
  sie **100 ms Vorlauf + 400 ms Nachlauf** an den PC.
- Der Laptop kann in einem anderen Raum stehen, weil die Übertragung per WLAN läuft.

---

## 3. Stückliste

Die Preise sind grobe Richtwerte. Bezugsquellen sind z. B. Reichelt, Berrybase,
Eckstein, Mouser oder Amazon.

### Elektronik

| Anz. | Teil | Zweck | ca. € |
|------|------|-------|-------|
| 1 | **ESP32-S3-DevKitC-1** (N8 oder N8R8) | Mikrocontroller, WLAN, ADC | 15 |
| 1 (+1) | **LIS3DH-Breakout** (z. B. Adafruit 2809 oder kompatibel, **mit SPI-Pins**) | MEMS-Beschleunigungssensor | 6 |
| 1 (+1) | **MCP6024-I/P** (4-fach-OpAmp, Rail-to-Rail, DIP-14) + IC-Sockel | Piezo-Verstärker | 3 |
| 4 | Piezo-Scheibe **Ø 20 mm** mit Messing-Träger (z. B. Murata 7BB-20-6L0) | Sensor (2 im Einsatz, 2 Reserve) | 4 |
| 2 | Piezo-Scheibe **Ø 12 mm** (z. B. Murata 7BB-12-9) | Test für kleine Bauform | 3 |
| 4 | Schottky-Diode **BAT85** | Überspannungsschutz am Eingang | 1 |
| – | Widerstände: 4× 10 kΩ, 2× 1 MΩ, 2× 100 kΩ, 2× 1 kΩ (Gain), 2× 1 kΩ (Ausgang), 2× 100 kΩ (Vref-Teiler) | | 1 |
| – | Kondensatoren: 2× 10 µF (Gain-Zweig), 1× 10 µF + 1× 100 nF (Vref), 2× 22 nF (Ausgangsfilter), 1× 100 nF (OpAmp-Versorgung) | | 2 |
| 2 | Jumper + Stiftleisten | Verstärkung 11 oder 101 umschalten | 1 |
| 1 | Lochrasterplatine ca. 7 × 5 cm, Buchsenleisten für das DevKit | | 3 |
| 2 × 2 m | **Geschirmtes** 2-adriges Kabel (Mikrofonkabel) | Piezo-Zuleitungen; ungeschirmt fängt man Netzbrummen ein | 4 |
| 1 × 1 m | dünnes 6-adriges Flachkabel oder Litzen | LIS3DH-Anschluss (SPI) | 2 |
| 1 | USB-Netzteil 5 V / 1 A + USB-C-Kabel (2 m) | Stromversorgung | – |

### Montage am Glas

| Teil | Zweck |
|------|-------|
| **Montagewachs** (Bienenwachs oder Petro-Wachs für Schwingungsaufnehmer) | Standard-Methode für vorübergehende Sensormontage: gute Kopplung bis in den kHz-Bereich, rückstandsfrei |
| Dünnes, festes doppelseitiges Klebeband (z. B. tesa 4965) | Vergleich mit einer möglichen Dauermontage |
| Malerkrepp | Kabel fixieren, Positionen markieren |

### Test-Pendel

| Teil | Zweck |
|------|-------|
| 3 Stoffsäckchen (z. B. Socke, Baumwollbeutel), mit Reis oder Sand gefüllt auf **50 g, 100 g, 200 g** | Weicher Aufprallkörper (Vogel-Ersatz) |
| Küchenwaage | Massen abwiegen |
| Angelschnur oder dünne Schnur, ca. 1,5 m | Pendel |
| Galgen: Stativ, Leiter oder Holzlatte mit Schraubzwinge (alternativ 3D-gedruckte Halterung) | Aufhängepunkt **außen**, ca. 1 m über dem Aufprallpunkt |
| Zollstock oder Maßband, Kreide oder Klebeband zum Markieren | Auslenkung reproduzierbar einstellen |
| Smartphone mit Zeitlupe (optional) | Aufprallgeschwindigkeit kontrollieren |

**Gesamtkosten Elektronik:** ca. 50 € (mit Reserveteilen).

---

## 4. Schaltung

### 4.1 Piezo-Verstärker (pro Kanal, 2× aufbauen)

Eine Piezo-Scheibe liefert bei Erschütterung eine Spannung, ist aber sehr hochohmig.
Der OpAmp puffert und verstärkt das Signal um eine Mittenspannung (Vref ≈ 1,65 V), damit
der ADC positive und negative Ausschläge sieht.

```
                       3V3                                       3V3
                        │                                         │
                      BAT85 (Kathode an 3V3)                    ──┴── MCP6024 Pin 4 (V+), 100 nF nach GND
                        │                                         Pin 11 (V−) an GND
 Piezo (+) ──[10k]──────●───────────────▶ IN+  (Pin 3)
 (Mittelelektrode)      │                         ┌────────┐
                      BAT85 (Anode an GND)        │ U1A    │── OUT (Pin 1) ──[1k]──●──▶ ESP32 GPIO1 (Kanal 0)
                        │                ┌─▶ IN− (Pin 2)   │                        │
                      [1M]               │        └────────┘                      [22n]
                        │                │                                         │
 Piezo (−) ─────────────●── Vref         ├──[100k Rf]── OUT                       GND
 (Messing, Schirm)                       │
                                         └──[Rg]──[10µF]── GND
                                              Rg = 10 kΩ → Verstärkung 11
                                              Rg =  1 kΩ → Verstärkung 101   (Jumper)
```

Kanal 1 ist identisch aufgebaut: U1B (Pins 5/6/7) an **GPIO2**.

**Vref** (Mittenspannung, einmal für beide Kanäle):

```
 3V3 ──[100k]──●──[100k]── GND
               │
             [10µF] ∥ [100n] nach GND
               │
               └──▶ Vref (an die 1-MΩ-Widerstände und die Piezo-(−)-Anschlüsse)
```

| Bauteil | Funktion |
|---------|----------|
| 10 kΩ + 2× BAT85 | begrenzt Spannungsspitzen bei harten Schlägen; Piezos können mehrere Dutzend Volt liefern |
| 1 MΩ nach Vref | Arbeitspunkt; bildet mit der Piezo-Kapazität (~20 nF) einen Hochpass bei ~8 Hz und unterdrückt Wind- und Temperaturdrift |
| Rf / Rg + 10 µF | Wechselspannungs-Verstärkung 11 bzw. 101, Gleichspannungs-Verstärkung 1, kein Übersteuern durch Offset |
| 1 kΩ + 22 nF | Tiefpass ~7 kHz gegen Aliasing (Abtastung 20 kHz) |

**Startwert:** Verstärkung 101. Übersteuert das Signal (Werte nahe 0 oder 4095),
auf 11 umstecken.

### 4.2 Anschlussplan ESP32-S3-DevKitC-1

| Signal | ESP32-S3-Pin | Gegenstelle |
|--------|--------------|-------------|
| Piezo-Kanal 0 | GPIO1 (ADC1_CH0) | Ausgang U1A über 1 kΩ |
| Piezo-Kanal 1 | GPIO2 (ADC1_CH1) | Ausgang U1B über 1 kΩ |
| LIS3DH SCL/SPC | GPIO12 | SPI-Takt |
| LIS3DH SDA/SDI | GPIO11 | MOSI |
| LIS3DH SDO | GPIO13 | MISO |
| LIS3DH CS | GPIO10 | Chip Select |
| LIS3DH VIN / GND | 3V3 / GND | |
| Verstärker V+ | 3V3 | |
| GND | GND | Schirm der Piezo-Kabel nur **auf der Platinenseite** an Vref |

Pins lassen sich in `firmware/poc-logger/main/config.h` ändern.

---

## 5. Sensorpositionen und Montage

```
  Blick von innen auf die Festverglasung (EG)

  ┌───────────────────────────────────────────┐  ← Holzrahmen
  │ ┌───────────────────────────────────────┐ │  ← Glasleiste
  │ │ P1●                                   │ │
  │ │                                       │ │
  │ │                                       │ │
  │ │ P2●               ×A                  │ │     × = Aufprallpunkte außen (Pendel)
  │ │                                       │ │       A = Mitte
  │ │                                       │ │       B = Rand (30 cm von der Kante)
  │ │ ×B                                    │ │       C = Ecke (30 cm von beiden Kanten)
  │ │                                    ×C │ │
  │ └───────────────────────────────────────┘ │
  └───────────────────────────────────────────┘
   P3 = auf der Glasleiste neben P1   P4 = auf dem Holzrahmen neben P1   P5 = Glasmitte (nur Referenz)
```

| Position | Ort | Zweck |
|----------|-----|-------|
| **P1** | Glas, untere Ecke, ca. 3 cm von der Glasleiste | Favorit für die spätere Montage |
| **P2** | Glas, Mitte der Seitenkante, ca. 3 cm von der Glasleiste | Vergleich Ecke ↔ Kante |
| **P3** | Glasleiste (Holz) neben P1 | unsichtbare Montage möglich? |
| **P4** | Blendrahmen (Holz) neben P1 | wie P3, noch weiter vom Glas weg |
| **P5** | Glasmitte | bestmögliches Signal als Referenz |

**Montage mit Wachs:**
1. Glas an der Stelle mit Spiritus reinigen.
2. Eine erbsengroße Menge Wachs mit den Fingern weich kneten.
3. Wachs dünn auf den Sensor auftragen.
4. Sensor fest andrücken und leicht drehen, bis eine dünne, blasenfreie Schicht entsteht.

Die Piezo-Scheibe sitzt mit der **Messingseite** zum Glas. Den LIS3DH direkt neben
Piezo A montieren, mit der Bauteilseite nach außen. Die **Z-Achse** steht dann
senkrecht zur Scheibe.

**Kabel:** Mit Krepp so fixieren, dass sie nicht am Sensor ziehen und nicht frei
schwingen.

---

## 6. Test-Pendel

Das Pendel hängt **außen**. In Ruhe berührt das Säckchen die Scheibe gerade eben
(Schnur senkrecht). Für einen Schlag wird es vom Glas weg ausgelenkt und losgelassen.
Am tiefsten Punkt trifft es mit maximaler Geschwindigkeit senkrecht auf das Glas.

- **Fallhöhe:** h = L − √(L² − d²). Daraus folgt für die Auslenkung d = √(2·L·h − h²).
- **Geschwindigkeit:** v = √(2·g·h). **Impuls:** p = m·v. **Energie:** E = m·g·h.

Werte für eine Schnurlänge **L = 1 m**:

| Masse | Höhe h | Auslenkung d | v | Impuls p | Energie E |
|-------|--------|--------------|---|----------|-----------|
| 50 g | 2 cm | 20 cm | 0,63 m/s | 0,031 Ns | 0,010 J |
| 50 g | 5 cm | 31 cm | 0,99 m/s | 0,050 Ns | 0,025 J |
| 50 g | 10 cm | 44 cm | 1,40 m/s | 0,070 Ns | 0,049 J |
| 50 g | 20 cm | 60 cm | 1,98 m/s | 0,099 Ns | 0,098 J |
| 100 g | 5 cm | 31 cm | 0,99 m/s | 0,099 Ns | 0,049 J |
| 100 g | 10 cm | 44 cm | 1,40 m/s | 0,140 Ns | 0,098 J |
| 100 g | 20 cm | 60 cm | 1,98 m/s | 0,198 Ns | 0,196 J |
| 100 g | 40 cm | 80 cm | 2,80 m/s | 0,280 Ns | 0,392 J |
| 200 g | 20 cm | 60 cm | 1,98 m/s | 0,396 Ns | 0,392 J |
| 200 g | 40 cm | 80 cm | 2,80 m/s | 0,560 Ns | 0,785 J |

**Vergleich mit echten Vögeln:**
- Meise oder Rotkehlchen: 18 g bei 8 m/s ergibt **≈ 0,14 Ns / 0,6 J**.
- Amsel: 90 g bei 10 m/s ergibt **≈ 0,9 Ns / 4,5 J**.

Bei gleichem Impuls ist das Pendel also langsamer und schwerer als ein Vogel. Für die
Glasschwingung zählt vor allem der übertragene Impuls. Ein weiches Säckchen kommt
der Kontaktzeit eines Vogelkörpers (einige Millisekunden) nahe. Die schwächsten Stufen
(50 g / 2–5 cm) liegen bewusst **unter** einem Meisen-Aufprall. Damit sehen wir die
Empfindlichkeitsreserve.

**Sicherheit:** Nur weiche Säckchen verwenden, keine harten Gegenstände. Eine
3-fach-Verglasung hält diese Energien problemlos aus. Bei Arbeiten an der Leiter
eine zweite Person sichern lassen.

---

## 7. Messplan

Jede Messreihe bekommt ein **Label** nach dem Schema
`<Versuch>_<Sensorbelegung>_<Details>`. Beispiel: `M2_A-P1_B-P3_m100_h10_ptA`
bedeutet: Versuch M2, Piezo A auf P1, Piezo B auf P3, 100 g, 10 cm, Aufprallpunkt A.

Im Empfänger lässt sich das Label mit `:label …` umstellen, ohne neu zu starten.
Begleitend wird das [Messprotokoll](messprotokoll.md) geführt.

| Versuch | Inhalt | Umfang | beantwortet |
|---------|--------|--------|-------------|
| **M0 – Inbetriebnahme** | Ruhepegel und Rauschen ablesen (Statuszeilen), Schwellen auf ca. 5× Rauschen setzen (`thr_piezo`, `thr_accel`), Klopftest | 15 min | – |
| **M1 – Grundrauschen** | 10× `trigger` ohne Anregung; dazu läuft die periodische Ruheaufnahme (alle 15 min) | 30 min | Basis für SNR |
| **M2 – Position** | Pendel 100 g / 10 cm auf Punkt A, je **5 Schläge**. Belegungen: A-P1/B-P3, A-P1/B-P4, A-P2/B-P5 | 3 × 5 Schläge | F1, F2 |
| **M3 – Empfindlichkeit** | Sensoren auf P1 und P3 (bzw. bester Rahmenplatz). Alle Masse/Höhe-Stufen der Tabelle, je 5 Schläge, Punkt A | ~50 Schläge | F1, F3 |
| **M4 – Aufprallort** | 100 g / 10 cm auf A, B, C, je 5 Schläge | 15 Schläge | F4 |
| **M5 – Piezo-Größe** | Wie M2, aber Piezo Ø 12 mm statt 20 mm auf P1 | 5 Schläge | Bauform |
| **M6 – Störquellen** | je 5×: Klopfen außen und innen (Knöchel), Haustür zuschlagen, Terrassentür schließen, **Schiebetür** öffnen und schließen, **ZIP-Screen** ab, auf und in die Endlage, Schritte vor dem Fenster, Gegenstand auf Fensterbank abstellen, Scheibe putzen (Abzieher) | ~50 Ereignisse | F5 |
| **M7 – Langzeit** | 1–2 Wochen laufen lassen (Label `M7_langzeit`), Screen-Fahrten und Wetter notieren bzw. aus HA exportieren | Tage | F5, F6 |
| **M8 – Montageart** (optional) | P1 mit Klebeband statt Wachs, sonst wie M2 | 5 Schläge | Dauermontage |

Für **F3** muss nichts extra gemessen werden. Die Auswertung simuliert aus den
LIS3DH-Daten automatisch, was ein Sensor mit 400 Hz bzw. 100 Hz gesehen hätte.

---

## 8. Software

### 8.1 Firmware flashen (`firmware/poc-logger`)

Voraussetzung ist **eine** der beiden Umgebungen:
- **ESP-IDF 5.3**, siehe <https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/get-started/>
- **PlatformIO** (VS Code), Plattform `espressif32` ab 6.9

So geht es weiter:
1. `main/secrets.example.h` nach `main/secrets.h` kopieren und WLAN-Name sowie Passwort
   eintragen.
2. Bauen und flashen:
   - ESP-IDF: `idf.py set-target esp32s3`, dann `idf.py -p /dev/ttyUSB0 flash monitor`
   - PlatformIO: `pio run -t upload`, dann `pio device monitor`
3. Im seriellen Monitor erscheint die IP-Adresse. Das Gerät meldet sich im Router als
   `glasssentry-poc1` an. Bei einer Fritzbox ist es meist unter `glasssentry-poc1.fritz.box`
   erreichbar.

Wichtige Einstellungen stehen in `main/config.h`: Pins, Abtastraten, Vorlauf und
Nachlauf, Startschwellen und LIS3DH-Modus (`GS_LIS_FAST_MODE`).

### 8.2 Empfänger auf dem PC (`tools/`)

```bash
cd tools
python -m venv .venv && source .venv/bin/activate      # Windows: .venv\Scripts\activate
pip install -r requirements.txt

python gs_receiver.py --host glasssentry-poc1 --label M0_test
```

Eingaben im laufenden Empfänger (jeweils mit Enter abschließen):

| Eingabe | Wirkung |
|---------|---------|
| `:label M2_A-P1_B-P3_m100_h10_ptA` | Label für folgende Ereignisse |
| `thr_piezo 40` | Piezo-Schwelle (ADC-Counts) |
| `thr_accel 20` | LIS3DH-Schwelle (mg) |
| `trigger` | Aufnahme ohne Anregung (Rauschprobe) |
| `status` | sofortige Statusmeldung |
| `:quit` | beenden |

Die Daten landen in `data/<Datum>/…npz`. Dazu kommen `data/events.jsonl` (ein Eintrag
pro Ereignis inkl. Merkmalen) und `data/status.jsonl` (Rauschpegel alle 10 s).
Die Messdaten sind nicht im Git-Repository.

**Ohne Hardware testen:** `python fake_device.py`, dann in einem zweiten Terminal
`python gs_receiver.py --host 127.0.0.1`.

### 8.3 Auswertung

```bash
python gs_analyze.py list data/                         # Tabelle aller Ereignisse
python gs_analyze.py list data/ --csv merkmale.csv       # Export für Excel/LibreOffice
python gs_analyze.py summary data/                       # SNR/Spitzen je Label → F1–F4
python gs_analyze.py summary data/ --plot --x piezo0_duration_ms --y piezo0_impulses   # F5
python gs_analyze.py plot data/2026-…/….npz --names "Glas P1,Rahmen P3"
```

Berechnete Merkmale je Kanal:
- Spitzenwert, Rauschen (aus dem Vorlauf), **SNR**
- Energie, Dauer, Anstiegszeit
- dominante Frequenz und spektraler Schwerpunkt
- **Impulsanzahl**
- beim LIS3DH zusätzlich die Spitzenwerte bei simulierten 400 Hz und 100 Hz

---

## 9. Entscheidungen nach dem PoC

| Ergebnis | Folge für den Batteriesensor |
|----------|------------------------------|
| Rahmen (P3/P4) erfüllt F1 | Sensor **unsichtbar** am Rahmen; Ein-Gehäuse-Lösung ohne Kabel zum Glas |
| Nur Glas erfüllt F1 | zweiteiliger Aufbau (Sensorkopf am Glas + Elektronik am Rahmen), siehe anforderungen.md 2.7 |
| Piezo deutlich besser als LIS3DH | Piezo mit stromsparendem Komparator als Weckschaltung, ESP32 im Deep Sleep |
| LIS3DH reicht auch bei 400 Hz | MEMS-Sensor mit Hardware-Wake-up (z. B. LIS2DW12 oder ADXL362), sehr einfache Schaltung |
| Merkmale trennen Störquellen gut | regelbasierte Klassifikation direkt auf dem Sensor |
| Trennung unklar | Merkmale senden und im Gateway bzw. in HA klassifizieren, später mit mehr Daten (ML) |

Danach folgen Phase 2 (Erkennung) und Phase 3 (Gateway mit ESP-NOW und MQTT in HA)
laut [Konzept](konzept.md#8-vorgehen--roadmap-vorschlag).
