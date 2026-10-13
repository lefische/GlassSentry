# Anforderungen & Rahmenbedingungen

Stand: 2026-10-13 – aus vier Abstimmungsrunden zu den [offenen Fragen](offene-fragen.md).

## 1. Ausgangslage

### Fenster
- **Erdgeschoss:** 6 bodentiefe Elemente: 4 Festverglasungen, 1 Schiebetür, 1 Terrassentür.
- **Obergeschoss:** 6 Fenster mit ca. 1,5 m Höhe: 3 Festverglasungen, 3 Kippfenster.
- **Gesamt:** 12 Elemente, davon 7 fest, 3 kippbar, 1 Schiebetür, 1 Terrassentür.
- **Rahmen:** Holz-Alu (innen Holz, außen Alu).
- **Verglasung:** durchgehend 3-fach-Isolierglas.
- **Ausrichtung:** Süden.
- **Betroffen sind bisher:** die festen Fenster und die Schiebetür.
- **Sonnenschutz:** elektrischer **ZIP-Screen**, ca. 5 cm vor der Scheibe, über
  Home Assistant steuerbar. Die **Position** meldet er in HA **nicht**.
- **Eigentum:** Kleben und notfalls Schrauben am Rahmen sind möglich.
- **Häufigkeit:** unbekannt, das soll ja gemessen werden. Vermutung: vor allem
  **im Sommer und am Nachmittag**.

### Umgebung
- Keine Haustiere, keine Kinder, keine Saug- oder Mähroboter.

### Energieversorgung
- **Sensoren:** müssen mit Batterie laufen, weil an den Fenstern keine Steckdosen sind.
- **Empfänger/Gateway:** darf am Netz hängen, Steckdosen sind vorhanden.
- **Proof of Concept:** darf mit Netzteil betrieben werden.

### Vorhandene Infrastruktur
- Home Assistant OS läuft auf einem **Raspberry Pi 5** mit **SD-Karte**.
- Ein Zigbee-Netz läuft schon, mit Türkontakten. An den Fenstern gibt es noch keine Kontakte.
- MQTT ist noch nicht eingerichtet, kann aber eingeführt werden.
- ESPHome wird noch nicht genutzt, kann aber eingeführt werden.
- Eine Wetterintegration ist vorhanden.

### Zielsetzung
- **Hauptziel ist die Statistik:** Es soll erfasst werden, wie oft, wo und wann Vögel gegen die Fenster fliegen.
- Eine Sofort-Benachrichtigung ist nicht erforderlich.
- **Lieber alles erfassen:** Die Statistik soll alle Auslösungen enthalten. Fehlalarme
  werden bei Bedarf später herausgefiltert.

### Budget
- Flexibel, zunächst geht es nur um den Proof of Concept.

### Optik
- Im finalen Aufbau muss der Sensor **sehr unauffällig** sein.

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

### 2.2 Fenstertypen: fest, schiebend, drehend
Ohne Haustiere, Kinder und Roboter bleiben als Störquellen vor allem drei Dinge:
Bedienung der Türen, Fensterputzen und Wetter.

| Typ | Besonderheit | Umgang |
|-----|--------------|--------|
| **Festverglasung** | bewegt sich nie, ruhigster Messort | **idealer Startpunkt** für den Proof of Concept; Elektronik kann fest in der Laibung oder am Rahmen sitzen |
| **Schiebetür** | Öffnen und Schließen erzeugt Rollgeräusche und einen Anschlag | Sensor erkennt die Bewegung selbst (seitliche Beschleunigung, lange Signaldauer) und unterdrückt sie; Kabel zum Flügel nicht möglich → Sensor und Elektronik sitzen komplett am Flügel |
| **Terrassentür (Dreh/Kipp)** | Schließen und Zuschlagen | Kippstellung über die Lage zur Schwerkraft erkennbar; Zuschlagen über Gleichtakt mit den Nachbarscheiben und lange Signaldauer unterdrücken. Bisher nicht betroffen → **niedrige Priorität** |

Große Scheiben schwingen tiefer und schwächer. Die Schwellwerte müssen deshalb für EG und OG
getrennt kalibriert werden. Für das Fensterputzen gibt es einen Putzmodus
bzw. einen Scharf-Schalter in Home Assistant.

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
  - Ein vollständiges Ereignisprotokoll mit Merkmalen (Umsetzung siehe 2.8).
- **Empfindlich auslösen, später filtern:** Der Schwellwert wird bewusst niedrig gesetzt.
  Jede Auslösung landet im Protokoll. Die Klassifikation („Vogel“, „Bedienung“,
  „Wetter“ usw.) ist nur ein Attribut und wird nie zum Verwerfen genutzt.
  Pro Fenster gibt es deshalb zwei Zähler: **alle Auslösungen** und **wahrscheinlich Vogel**.
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

### 2.6 Südausrichtung
- **Spiegelung:** Am Nachmittag steht die Sonne im Südwesten. Dann spiegeln die Scheiben
  Himmel und Garten oft besonders stark. Das passt zur Vermutung „Sommer, Nachmittag“.
  Für die Statistik nehmen wir deshalb Sonnenstand (`sun.sun`: Azimut, Elevation)
  und Bewölkung zu jedem Ereignis mit auf. So lässt sich die Vermutung **prüfen**.
- **Hitze:** Ein Gehäuse direkt an der besonnten Scheibe kann 50–70 °C erreichen.
  - **Kein PLA** verwenden, sondern **PETG oder ASA**.
  - Die Batterie muss für die Temperatur ausgelegt sein, z. B. Lithium-Primärzellen
    (Li-SOCl₂ oder Li-MnO₂) bis über 60 °C. LiPo-Akkus hinter Glas in der Sonne sind ungeeignet.
- **Thermische Spannungen:** Sonne und Wolkenwechsel können ein Knacken in Glas und Rahmen
  auslösen. Diese Fälle müssen in die Störtests, z. B. an einem sonnigen Tag mit wechselnder Bewölkung.

### 2.6a ZIP-Screen als Sonnenschutz
Ein ZIP-Screen ist ein Textilbehang, der seitlich in Schienen geführt wird. Er sitzt
ca. 5 cm vor der Scheibe und ist für das Projekt **sehr wertvoll**, und zwar in vier Rollen:

1. **Einflussgröße:** Ist der Screen unten, spiegelt die Scheibe nicht mehr, und Vögel
   kommen nicht an das Glas heran. Die Position wird deshalb zu jedem Ereignis gespeichert.
2. **Richtige Bezugsgröße für die Statistik:** Statt „Treffer pro Monat“ zählt
   „Treffer pro Stunde **offener** Scheibe“. Dafür erfasst HA die Stunden mit
   hochgefahrenem Screen pro Fenster, z. B. über `history_stats`.
3. **Gratis-Referenz für Fehlalarme:** Jede Auslösung bei geschlossenem Screen kann
   **kein Vogel am Glas** sein. Diese Ereignisse sind automatisch als „kein Vogel“ markiert.
   Damit lernen wir die Störquellen kennen, ohne dass jemand von Hand bestätigen muss.
4. **Experiment:** Später lässt sich gezielt testen, ob ein Herunterfahren an
   kritischen Nachmittagen Treffer verhindert (Ziel Z6). HA kann das sogar automatisch.

**Störquelle:** Der Screen selbst kann Fehlalarme auslösen, nämlich durch Motorlauf
und Anschlag in der Endlage. Bei starkem Wind kann sich das Tuch bewegen und
eventuell den Rahmen berühren. Ein ZIP-Screen flattert kaum, weil er in den Schienen geführt ist.
- Bei 5 cm Abstand und großen Scheiben ist eine Berührung des Glases unwahrscheinlich, aber
  nicht ausgeschlossen. Das muss in den Störtests bei Wind geprüft werden.
- Während der Fahrt und kurz danach (z. B. 10 s) werden Ereignisse als „Screen aktiv“ markiert.

**Position ist in HA nicht bekannt.** Dafür gibt es drei Lösungswege, die sich kombinieren lassen:

| Weg | Prinzip | Bewertung |
|-----|---------|-----------|
| **A – Laufzeit-Cover** | HA rechnet die Position aus dem letzten Befehl und der Fahrzeit (z. B. die HACS-Integration *Cover Time Based*) | gut, solange **nur über HA** gefahren wird; Handsender oder Taster bringen es aus dem Takt |
| **B – Helligkeitssensor im Sensorknoten** | kleiner Lichtsensor (z. B. VEML7700, ca. 2×2 mm), der durch die Scheibe misst: Screen unten → Helligkeit bricht stark ein | unabhängig von der Bedienart, liefert **zusätzlich die Sonneneinstrahlung je Fenster**. Das ist eine gute Größe für die Spiegelung. Kostet etwas Strom und Platz |
| **C – Motorlauf erkennen** | Der Vibrationssensor spürt den Screen-Motor im Rahmen | nur für Fahrbeginn und Fahrende, nicht für die Position; trotzdem nützlich zum Markieren |

**Vorschlag:** **A** sofort in HA umsetzen. **B** im Proof of Concept mit erproben.
**C** nutzen wir ohnehin automatisch.

### 2.6c Wetter
- Die vorhandene Wetterintegration liefert zu jedem Ereignis den Kontext: Wind, Niederschlag
  und Bewölkung.
- Hagel ist in den meisten Integrationen nur indirekt erkennbar, über Wetterzustand oder
  Warnungen (z. B. DWD-Integration). Für die Statistik reicht das, weil wir ohnehin nur markieren
  und nichts verwerfen.

### 2.6b Holz-Alu-Rahmen
- **Innen Holz:** Holz dämpft Schwingungen. Ein Sensor am Rahmen bekommt deshalb
  voraussichtlich weniger Signal als einer direkt auf dem Glas. Das muss im
  Proof of Concept gemessen werden.
- **Montage:** Das Haus ist Eigentum. Kleben ist also unproblematisch, und kleine Schrauben am
  Holzrahmen sind möglich. Auf dem Glas soll trotzdem lösbar geklebt werden, damit sich der
  Sensor umsetzen lässt.
- **Optik:** Das Gehäuse wird im Holzton gedruckt (Holz-Filament oder lackiertes PETG/ASA)
  oder sitzt in der Laibung.

### 2.7 Unauffälligkeit im finalen Aufbau
Daraus folgt ein **zweiteiliger Aufbau**:

```
  Scheibe (innen, Ecke)                 Rahmen / Laibung / Glasleiste
 ┌──────────────────────┐   dünnes     ┌────────────────────────────┐
 │ Sensorkopf           │   Flachkabel │ Elektronik + Batterie      │
 │ Piezo Ø 10–15 mm oder├──────────────┤ ESP32, Funk, Batterie      │
 │ MEMS-Sensor 2×2 mm   │   (~1 mm)    │ farblich an Rahmen angepasst│
 └──────────────────────┘              └────────────────────────────┘
```

- **Sensorkopf:** möglichst klein, in der Scheibenecke direkt an der Glasleiste,
  in Rahmenfarbe abgedeckt (3D-Druck).
- **Elektronikbox:** flach am Rahmen, an der Glasleiste oder in der Laibung, wo sie weniger auffällt.
- **Beste Option:** Reicht das Signal **am Rahmen bzw. an der Glasleiste** aus, verschwindet
  der Sensor ganz von der Scheibe. Diese Messung ist im Proof of Concept deshalb ein Pflichtpunkt.

### 2.8 Home Assistant OS auf dem Raspberry Pi 5 (SD-Karte)
- **MQTT:** Der Broker kommt als **Mosquitto-Add-on** dazu, so wie es bei HA OS üblich ist.
- **Datenmenge:** Sie ist klein: einige Ereignisse pro Woche zu je ~1 KB Merkmalen.
  Selbst bei Unwetter mit vielen Auslösungen bleibt sie überschaubar. Das Gateway
  begrenzt die Rate und fasst Ereignisse zusammen.
- **Ereignisprotokoll (Vorschlag):**
  - Das Gateway schickt jedes Ereignis als JSON per MQTT.
  - HA hängt es als **JSON-Zeile** an eine Datei unter `/share/glasssentry/` an,
    z. B. über die File-Integration oder ein kleines Add-on.
  - Diese Datei sichert das normale HA-Backup mit. Sie lässt sich später in Python
    oder Excel auswerten.
  - InfluxDB/Grafana kann später dazukommen, wenn die Speicherfrage geklärt ist.
- **SD-Karte:** Sie verschleißt durch häufiges Schreiben. Unsere Daten fallen dabei kaum
  ins Gewicht. Der HA-Recorder selbst ist der größere Verursacher. **Empfehlung unabhängig
  vom Projekt:** Beim Pi 5 auf eine **NVMe-SSD** (M.2-HAT) umsteigen. Für InfluxDB wäre das
  Voraussetzung.
- **Rohdaten:** Wellenformen speichern wir nur im Proof of Concept und dann auf einem anderen
  Rechner, nicht auf dem Pi.

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

---

## 4. Ausbau-Reihenfolge (Vorschlag)

| Stufe | Fenster | Begründung |
|-------|---------|------------|
| Proof of Concept | **1 Festverglasung EG** (die am häufigsten getroffene) | ruhig, betroffen, Netzteil möglich |
| Ausbau 1 | übrige Festverglasungen EG + **Schiebetür** | alle bekannten Trefferstellen |
| Ausbau 2 | 6 Fenster OG (3 fest, 3 kippbar) | Statistik vervollständigen; Kippstellung über die Lage zur Schwerkraft erkennen |
| Ausbau 3 | Terrassentür | bisher nicht betroffen |
