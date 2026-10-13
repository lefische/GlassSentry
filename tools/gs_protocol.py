"""Datenformat zwischen PoC-Logger und PC.

Jedes Frame besteht aus einer JSON-Kopfzeile (abgeschlossen mit '\\n') und
``payload_bytes`` Binärdaten. Ereignis-Payload (little endian):

    Piezo-Kanal 0 … C-1 : uint16 × n (hintereinander)
    Accel               : int16 × 3 × m (x, y, z verschachtelt), falls vorhanden
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import BinaryIO

import numpy as np


@dataclass
class Event:
    meta: dict
    piezo: np.ndarray                      # Form (Kanäle, n), uint16, ADC-Counts
    accel: np.ndarray | None = None        # Form (m, 3), int16, LSB (siehe mg_per_lsb)
    extra: dict = field(default_factory=dict)

    @property
    def piezo_rate(self) -> float:
        p = self.meta["piezo"]
        return float(p.get("rate_meas") or p["rate"])

    @property
    def accel_rate(self) -> float | None:
        a = self.meta.get("accel")
        if not a:
            return None
        return float(a.get("rate_meas") or a["rate"])

    @property
    def accel_mg(self) -> np.ndarray | None:
        if self.accel is None:
            return None
        return self.accel.astype(float) * self.meta["accel"]["mg_per_lsb"]

    @property
    def pre_ms(self) -> float:
        return float(self.meta["pre_ms"])


def read_frame(stream: BinaryIO) -> tuple[dict, bytes] | None:
    """Liest ein Frame; None bei Verbindungsende."""
    line = stream.readline()
    if not line:
        return None
    header = json.loads(line)
    size = int(header.get("payload_bytes", 0))
    payload = stream.read(size) if size else b""
    if len(payload) != size:
        return None
    return header, payload


def encode_frame(header: dict, payload: bytes = b"") -> bytes:
    header = dict(header, payload_bytes=len(payload))
    return json.dumps(header, separators=(",", ":")).encode() + b"\n" + payload


def decode_event(header: dict, payload: bytes) -> Event:
    p = header["piezo"]
    channels, n = int(p["channels"]), int(p["n"])
    piezo_bytes = channels * n * 2
    piezo = np.frombuffer(payload[:piezo_bytes], dtype="<u2").reshape(channels, n).copy()

    accel = None
    a = header.get("accel")
    if a:
        m = int(a["n"])
        raw = payload[piezo_bytes:piezo_bytes + m * 6]
        accel = np.frombuffer(raw, dtype="<i2").reshape(m, 3).copy()
    return Event(meta=header, piezo=piezo, accel=accel)


def encode_event_payload(piezo: np.ndarray, accel: np.ndarray | None) -> bytes:
    data = piezo.astype("<u2").tobytes()
    if accel is not None:
        data += accel.astype("<i2").tobytes()
    return data


def save_event(path: Path, event: Event) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    meta = dict(event.meta, **event.extra)
    np.savez_compressed(
        path,
        piezo=event.piezo,
        accel=event.accel if event.accel is not None else np.zeros((0, 3), dtype=np.int16),
        meta=np.array(json.dumps(meta)),
    )


def load_event(path: Path) -> Event:
    with np.load(path, allow_pickle=False) as f:
        meta = json.loads(str(f["meta"]))
        accel = f["accel"]
        return Event(meta=meta, piezo=f["piezo"], accel=accel if accel.size else None)


def find_events(paths: list[str | Path]) -> list[Path]:
    """Sammelt .npz-Dateien aus Dateien und Verzeichnissen (rekursiv), sortiert."""
    found: list[Path] = []
    for p in map(Path, paths):
        if p.is_dir():
            found.extend(p.rglob("*.npz"))
        elif p.suffix == ".npz":
            found.append(p)
    return sorted(found)
