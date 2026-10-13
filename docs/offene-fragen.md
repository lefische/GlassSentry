# Offene Fragen

Bereits beantwortete Punkte stehen in [anforderungen.md](anforderungen.md).

## Noch offen

### A. Fenster & Umgebung
1. Wie viele Fenster gibt es im **Obergeschoss**, und sollen alle überwacht werden?
2. Sind die bodentiefen Fenster im EG Fest-, Dreh-/Kipp- oder Schiebeelemente (z. B. Terrassentür, Hebe-Schiebe-Tür)?
3. Welche Fenster sind erfahrungsgemäß betroffen, und wie oft passiert es ungefähr?
4. In welche Himmelsrichtungen zeigen die Fenster? Gibt es Spiegelungen von Bäumen oder Himmel, oder eine Futterstelle in der Nähe?
5. Gibt es Haustiere, Kinder oder Mäh-/Saugroboter in Fensternähe? Das wären Störquellen.
6. Eigentum oder Miete? Davon hängt ab, wie geklebt werden darf.

### B. Home Assistant
7. Welche Installation (HA OS, Container, Supervised) und welche Version? Davon hängt ab, wie der MQTT-Broker eingerichtet wird (Mosquitto-Add-on oder separat).
8. Wird außer dem Standard-Recorder schon etwas zur Protokollierung genutzt (InfluxDB, Grafana)? Wenn nicht: Soll das für die Statistik eingeführt werden?
9. Gibt es eine Wetterintegration oder Wetterstation, damit sich Hagel, Regen und Wind herausfiltern lassen?

### C. Rahmenbedingungen
10. Budget pro Fenster bzw. insgesamt?
11. Wie unauffällig muss der Sensor sein, gerade an den großen bodentiefen Scheiben?
12. Welche Fehlerrate ist für die Statistik akzeptabel? Lieber Ereignisse verpassen oder lieber einige Fehlalarme mitzählen?

## Beantwortet (2026-10-13)
- Verglasung und Anzahl: 3-fach, 6 bodentiefe Fenster im EG, Fenster im OG ca. 1,5 m hoch
- Strom: Sensoren mit Batterie, Gateway an der Steckdose, Proof of Concept auch am Netz
- Funk und HA: Zigbee vorhanden (nur Türkontakte), MQTT und ESPHome nicht eingerichtet, aber möglich
- Ziel: Statistik
- Kenntnisse: Löten, 3D-Druck, Firmware-Basis offen
