# GlassSentry

Erkennung von Vogelschlag an Fensterscheiben mit Anbindung an Home Assistant.

**Status:** Konzeptphase – die Anforderungen und Lösungsansätze werden gerade gemeinsam erarbeitet.

- [Konzept & Lösungsraum](docs/konzept.md)
- [Offene Fragen](docs/offene-fragen.md)

## Geplante Struktur

```
docs/           Konzept, Entscheidungen, Messprotokolle
hardware/       Schaltpläne, Stückliste, Gehäuse (3D-Druck)
firmware/       Sensor-Firmware (ESP32)
homeassistant/  MQTT-Discovery, Automationen, Dashboards
tools/          Auswertung von Rohdaten, Test-Pendel, Klassifikator-Training
```
