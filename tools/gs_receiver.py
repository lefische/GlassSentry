#!/usr/bin/env python3
"""Empfänger für den GlassSentry PoC-Logger.

Verbindet sich per TCP mit dem ESP32, speichert jedes Ereignis als .npz und
führt ein Protokoll (events.jsonl, status.jsonl) im Ausgabeverzeichnis.

Eingaben während des Betriebs (Enter drücken):
    :label <text>      Label für alle folgenden Ereignisse setzen
    :label             Label anzeigen
    thr_piezo <n>      Befehl an das Gerät (ebenso thr_accel, trigger, status)
    :quit              beenden

Beispiel:
    python gs_receiver.py --host glasssentry-poc1 --label M2_P1_m100_h10
"""

from __future__ import annotations

import argparse
import json
import socket
import sys
import threading
from datetime import datetime
from pathlib import Path

from gs_features import event_features
from gs_protocol import decode_event, read_frame, save_event


class Receiver:
    def __init__(self, host: str, port: int, out: Path, label: str, verbose_status: bool):
        self.host, self.port, self.out = host, port, out
        self.label = label
        self.verbose_status = verbose_status
        self.sock: socket.socket | None = None
        self.lock = threading.Lock()
        self.stop = threading.Event()
        self.events_received = 0

    # -- Ausgabe ------------------------------------------------------------
    def log(self, msg: str) -> None:
        print(f"{datetime.now():%H:%M:%S} {msg}", flush=True)

    def append_jsonl(self, name: str, record: dict) -> None:
        self.out.mkdir(parents=True, exist_ok=True)
        with open(self.out / name, "a", encoding="utf-8") as f:
            f.write(json.dumps(record, ensure_ascii=False) + "\n")

    # -- Verbindung ---------------------------------------------------------
    def send_command(self, line: str) -> None:
        with self.lock:
            if self.sock is None:
                self.log("nicht verbunden – Befehl verworfen")
                return
            self.sock.sendall(line.encode() + b"\n")

    def handle_status(self, header: dict) -> None:
        header["received_at"] = datetime.now().isoformat(timespec="milliseconds")
        self.append_jsonl("status.jsonl", header)
        if self.verbose_status:
            rms = ", ".join(f"{v:.1f}" for v in header.get("piezo_rms", []))
            self.log(
                f"Status: Piezo-Rauschen [{rms}] Counts, Accel {header.get('accel_rms_mg', 0):.1f} mg, "
                f"Schwellen {header.get('thr_piezo')}/{header.get('thr_accel_mg')} mg, "
                f"RSSI {header.get('rssi')} dBm, Ereignisse {header.get('events')}"
            )

    def handle_event(self, header: dict, payload: bytes, max_events: int | None) -> None:
        ev = decode_event(header, payload)
        now = datetime.now()
        ev.extra = {"label": self.label, "received_at": now.isoformat(timespec="milliseconds")}
        name = f"{now:%H%M%S_%f}"[:-3] + f"_{header['seq']:05d}_{self.label or 'ohne'}.npz"
        path = self.out / f"{now:%Y-%m-%d}" / name
        save_event(path, ev)

        ev.meta.update(ev.extra)
        feats = event_features(ev)
        record = {**{k: v for k, v in header.items() if k != "payload_bytes"}, **ev.extra,
                  "file": str(path.relative_to(self.out))}
        record["features"] = feats
        self.append_jsonl("events.jsonl", record)

        parts = []
        for ch in range(ev.piezo.shape[0]):
            parts.append(f"P{ch} {feats.get(f'piezo{ch}_peak', 0):6.0f} ({feats.get(f'piezo{ch}_snr_db', 0):4.1f} dB)")
        if "accel_peak_axis_mg" in feats:
            parts.append(f"A {feats['accel_peak_axis_mg']:6.0f} mg ({feats.get('accel_snr_db', 0):4.1f} dB)")
        flag = " [gekürzt]" if header.get("truncated") else ""
        self.log(f"Ereignis #{header['seq']} {header['source']:<6} [{self.label}] " + " | ".join(parts) + flag)

        self.events_received += 1
        if max_events and self.events_received >= max_events:
            self.stop.set()

    def run(self, max_events: int | None = None) -> None:
        while not self.stop.is_set():
            try:
                sock = socket.create_connection((self.host, self.port), timeout=10)
            except OSError as e:
                self.log(f"Verbindung zu {self.host}:{self.port} fehlgeschlagen ({e}), neuer Versuch in 5 s")
                self.stop.wait(5)
                continue
            sock.settimeout(None)
            with self.lock:
                self.sock = sock
            self.log(f"verbunden mit {self.host}:{self.port}, Ausgabe nach {self.out}")
            try:
                stream = sock.makefile("rb")
                while not self.stop.is_set():
                    frame = read_frame(stream)
                    if frame is None:
                        break
                    header, payload = frame
                    if header.get("type") == "event":
                        self.handle_event(header, payload, max_events)
                    elif header.get("type") == "status":
                        self.handle_status(header)
            except (OSError, ValueError) as e:
                self.log(f"Verbindungsfehler: {e}")
            finally:
                with self.lock:
                    self.sock = None
                sock.close()
            if not self.stop.is_set():
                self.log("Verbindung getrennt, neuer Versuch in 2 s")
                self.stop.wait(2)

    def console(self) -> None:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            if line == ":quit":
                self.stop.set()
                with self.lock:
                    if self.sock:
                        self.sock.shutdown(socket.SHUT_RDWR)
                return
            if line.startswith(":label"):
                new = line[len(":label"):].strip()
                if new:
                    self.label = new
                self.log(f"Label: {self.label or '(keins)'}")
                continue
            try:
                self.send_command(line)
            except OSError as e:
                self.log(f"Senden fehlgeschlagen: {e}")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="glasssentry-poc1", help="Hostname oder IP des ESP32")
    ap.add_argument("--port", type=int, default=5005)
    ap.add_argument("--out", type=Path, default=Path(__file__).resolve().parent.parent / "data")
    ap.add_argument("--label", default="", help="Label für die Messreihe, z. B. M2_P1_m100_h10")
    ap.add_argument("--quiet-status", action="store_true", help="Statusmeldungen nicht anzeigen")
    ap.add_argument("--max-events", type=int, default=None, help="nach n Ereignissen beenden")
    ap.add_argument("--no-console", action="store_true", help="keine Eingaben von stdin lesen")
    args = ap.parse_args(argv)

    rx = Receiver(args.host, args.port, args.out, args.label, not args.quiet_status)
    if not args.no_console:
        threading.Thread(target=rx.console, daemon=True).start()
    try:
        rx.run(args.max_events)
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
