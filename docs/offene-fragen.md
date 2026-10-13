# Offene Fragen

Antworten bitte direkt hier eintragen (oder im Chat) – daraus leiten wir die
Architekturentscheidungen ab.

## A. Fenster & Umgebung
1. Wie viele Fenster sollen überwacht werden, und welche sind erfahrungsgemäß betroffen?
2. Größe der Scheiben, Festverglasung oder Flügel (öffnen/kippen)? Bodentiefe Fenster, Wintergarten, Glasgeländer?
3. Verglasung: Einfach-, 2-fach- oder 3-fach-Isolierglas?
4. Ausrichtung (Himmelsrichtung), Spiegelungen von Bäumen/Himmel, Vogelfutterstelle in der Nähe?
5. Wie häufig passiert Vogelschlag heute ungefähr (pro Monat/Jahr)?
6. Eigentum oder Miete (Montage/Kleben)?

## B. Strom & Funk
7. Steckdosen in Fensternähe vorhanden, oder soll alles batteriebetrieben sein?
8. WLAN-Abdeckung an den Fenstern gut?
9. Gibt es bereits Zigbee / Thread / Matter im Haus?

## C. Home Assistant
10. Welche Installation (HA OS, Container, Supervised) und Version?
11. Gibt es schon einen MQTT-Broker (Mosquitto)? Wird ESPHome genutzt?
12. Wie wird protokolliert – nur Recorder, oder InfluxDB/Grafana o. ä.?
13. Vorhandene Wetterintegration/Wetterstation (für Hagel/Regen/Wind-Filter)?
14. Fensterkontakte vorhanden?
15. Gibt es Kameras, die auf die Fenster bzw. davor blicken?

## D. Anforderungen
16. Was ist wichtiger: sofortige Benachrichtigung (Vogel retten) oder Statistik (Schutzmaßnahmen bewerten)?
17. Wie viele Fehlalarme sind akzeptabel (z. B. 1 pro Woche)? Lieber etwas verpassen oder lieber zu viel melden?
18. Ist ein Kamerabeleg gewünscht?
19. Optik: Wie unauffällig muss der Sensor sein?

## E. Rahmenbedingungen
20. Budget pro Fenster bzw. gesamt?
21. Eigene Fähigkeiten/Werkzeug: Löten, 3D-Drucker, Programmieren (C++, Python, YAML)?
22. Bevorzugte Firmware-Basis: ESPHome (YAML, bequem) oder eigene Firmware (flexibler)?
