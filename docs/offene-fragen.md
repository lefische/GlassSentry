# Offene Fragen

Bereits beantwortete Punkte stehen in [anforderungen.md](anforderungen.md).

## Noch offen
1. Welche Art von Sonnenblende ist es: Textil-Screen (ZIP), Raffstore mit Lamellen oder Markise?
   Wie weit ist sie von der Scheibe entfernt? Davon hängt ab, ob die Blende an die Scheibe schlagen kann
   und wie viel Spiegelung sie verhindert.
2. Über welche Integration ist die Blende in HA eingebunden, und meldet sie ihre Position
   (`cover` mit `current_position`)?
3. Gibt es eine Wetterintegration oder Wetterstation mit Wind, Regen und Hagel als Kontext?
4. Eigentum oder Miete? Davon hängt ab, wie geklebt werden darf.

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

**2026-10-13, Runde 3**
- EG: 4 fest; OG: überwiegend fest, 3 Kippfenster
- Häufigkeit unbekannt (soll gemessen werden), vermutlich Sommer und Nachmittag
- Rahmen innen Holz, außen Alu; elektrische Sonnenblende, über HA steuerbar
- HA OS auf Raspberry Pi 5 mit SD-Karte
- Statistik soll alle Auslösungen enthalten, Filterung später
- Budget flexibel, Fokus auf dem Proof of Concept
