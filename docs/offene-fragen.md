# Offene Fragen

Bereits beantwortete Punkte stehen in [anforderungen.md](anforderungen.md).

## Noch offen
Derzeit keine. Neue Fragen ergeben sich im Proof of Concept.

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

**2026-10-13, Runde 4**
- Sonnenschutz ist ein ZIP-Screen, ca. 5 cm vor der Scheibe
- Die Screen-Position ist in HA nicht bekannt
- Eine Wetterintegration ist vorhanden
- Eigentum

**2026-10-13, Runde 5**
- Der Screen wird über Homematic angesteuert, auch aus HA; manuelle Betätigungen sind in HA auslesbar
- Wetterintegration: Met.no

**2026-10-13, Runde 6**
- Die Screen-Entität hat kein `current_position` → Laufzeit-Cover
