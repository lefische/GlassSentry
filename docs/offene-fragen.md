# Offene Fragen

Bereits beantwortete Punkte stehen in [anforderungen.md](anforderungen.md).

## Noch offen

### A. Fenster
1. Ich gehe davon aus, dass die 4 übrigen EG-Elemente neben Schiebe- und Terrassentür Festverglasungen sind. Stimmt das?
2. Welche Fensterart haben die 6 OG-Fenster (Dreh-Kipp, fest)? Wurden dort auch schon Treffer bemerkt?
3. Wie oft kommt es ungefähr zu Vogelschlag (pro Monat oder Jahr), und zu welcher Jahres- und Tageszeit?
4. Welche Rahmen gibt es (Kunststoff, Holz, Alu) und in welcher Farbe? Danach richten sich Gehäusefarbe und Montage.
5. Gibt es außen Rollläden oder Raffstores? Die beeinflussen die Spiegelung und damit die Statistik.
6. Eigentum oder Miete? Davon hängt ab, wie geklebt werden darf.

### B. Home Assistant
7. Läuft auf dem Raspberry Pi **HA OS** (mit Add-ons) oder Docker/Container?
8. Welches Pi-Modell, und läuft es von SD-Karte oder SSD?
9. Gibt es eine Wetterintegration oder Wetterstation, damit sich Hagel und Regen herausfiltern lassen?

### C. Rahmenbedingungen
10. Budget pro Fenster bzw. insgesamt?
11. Welche Fehlerrate ist für die Statistik akzeptabel? Lieber Ereignisse verpassen oder lieber einige Fehlalarme mitzählen?

## Beantwortet
**2026-10-13, Runde 1**
- Verglasung und Anzahl: 3-fach, 6 bodentiefe Fenster im EG, Fenster im OG ca. 1,5 m hoch
- Strom: Sensoren mit Batterie, Gateway an der Steckdose, Proof of Concept auch am Netz
- Funk und HA: Zigbee vorhanden (nur Türkontakte), MQTT und ESPHome nicht eingerichtet, aber möglich
- Ziel: Statistik
- Kenntnisse: Löten, 3D-Druck, Firmware-Basis offen

**2026-10-13, Runde 2**
- OG: 6 Fenster
- EG: darunter eine Schiebetür und eine Terrassentür
- Getroffen werden die festen Fenster und die Schiebetür; Ausrichtung Süden
- Keine Haustiere, Kinder oder Roboter
- Home Assistant läuft auf einem Raspberry Pi
- Sensor soll im finalen Aufbau sehr unauffällig sein
