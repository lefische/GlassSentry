# GlassSentry

Erkennung von Vogelschlag an Fensterscheiben mit Anbindung an Home Assistant.

**Status:** Proof of Concept – Messaufbau und Logger-Firmware stehen bereit, Messungen am Fenster folgen.

- [Konzept & Lösungsraum](docs/konzept.md)
- [Anforderungen & Festlegungen](docs/anforderungen.md)
- [Offene Fragen](docs/offene-fragen.md)
- [Proof of Concept: Aufbau & Messplan](docs/poc.md)
- [Messprotokoll-Vorlage](docs/messprotokoll.md)

## Geplante Struktur

```
docs/           Konzept, Entscheidungen, Messprotokolle
hardware/       Schaltpläne, Stückliste, Gehäuse (3D-Druck)
firmware/       Firmware (poc-logger: Rohdaten-Logger für den PoC)
homeassistant/  MQTT-Discovery, Automationen, Dashboards
tools/          PC-Empfänger, Auswertung, Gerätesimulation
```
