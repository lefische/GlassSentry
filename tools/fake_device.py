#!/usr/bin/env python3
"""Simulierter PoC-Logger zum Testen von Empfänger und Auswertung ohne Hardware.

Erzeugt synthetische Ereignisse (abklingende Scheibenschwingung, Doppel-Klopfen,
Ruhe) im selben Format wie die Firmware.

    python fake_device.py --port 5005 --interval 3
    python gs_receiver.py --host 127.0.0.1 --label test
"""

from __future__ import annotations

import argparse
import socket
import sys
import threading
import time

import numpy as np

from gs_protocol import encode_event_payload, encode_frame

PIEZO_RATE = 20000
ACCEL_RATE = 1344
PRE_MS, POST_MS = 100, 400


def ring(t: np.ndarray, amp: float, freqs=(380.0, 1150.0), tau=0.04) -> np.ndarray:
    out = np.zeros_like(t)
    on = t >= 0
    for i, f in enumerate(freqs):
        out[on] += amp / (i + 1) * np.sin(2 * np.pi * f * t[on]) * np.exp(-t[on] / tau)
    return out


def synth(kind: str, rng: np.random.Generator) -> tuple[np.ndarray, np.ndarray]:
    n = PIEZO_RATE * (PRE_MS + POST_MS) // 1000
    m = ACCEL_RATE * (PRE_MS + POST_MS) // 1000
    t = np.arange(n) / PIEZO_RATE - PRE_MS / 1000
    ta = np.arange(m) / ACCEL_RATE - PRE_MS / 1000

    amp = {"bird": 600.0, "knock": 300.0, "noise": 0.0}[kind]
    sig = ring(t, amp)
    acc = ring(ta, amp / 10, freqs=(380.0,))
    if kind == "knock":
        sig += ring(t - 0.12, amp)
        acc += ring(ta - 0.12, amp / 10, freqs=(380.0,))

    piezo = np.vstack([
        2048 + sig + rng.normal(0, 4, n),
        2040 + 0.3 * sig + rng.normal(0, 4, n),
    ]).clip(0, 4095).astype(np.uint16)
    accel = np.column_stack([
        rng.normal(0, 3, m),
        rng.normal(0, 3, m),
        1000 + acc + rng.normal(0, 3, m),
    ]).astype(np.int16)
    return piezo, accel


def serve(port: int, interval: float, count: int | None) -> None:
    rng = np.random.default_rng(1)
    srv = socket.create_server(("127.0.0.1", port), reuse_port=False)
    conn, _ = srv.accept()
    thr = {"piezo": 80, "accel": 40}
    manual = threading.Event()

    def reader():
        for line in conn.makefile("r"):
            parts = line.split()
            if not parts:
                continue
            if parts[0] == "thr_piezo":
                thr["piezo"] = int(parts[1])
            elif parts[0] == "thr_accel":
                thr["accel"] = int(parts[1])
            elif parts[0] == "trigger":
                manual.set()

    threading.Thread(target=reader, daemon=True).start()
    kinds = ["bird", "knock", "noise"]
    seq, t0 = 0, time.time()
    try:
        while count is None or seq < count:
            status = {"type": "status", "device": "fake", "fw": "sim", "uptime_s": int(time.time() - t0),
                      "rssi": -55, "thr_piezo": thr["piezo"], "thr_accel_mg": thr["accel"],
                      "piezo_rate": PIEZO_RATE, "piezo_base": [2048, 2040], "piezo_rms": [4.0, 4.0],
                      "adc_overflows": 0, "accel": True, "accel_rate": ACCEL_RATE, "accel_rms_mg": 3.0,
                      "accel_overruns": 0, "events": seq, "dropped": 0, "truncated": 0}
            conn.sendall(encode_frame(status))
            manual.wait(interval)
            kind = "noise" if manual.is_set() else kinds[seq % len(kinds)]
            manual.clear()
            piezo, accel = synth(kind, rng)
            header = {"type": "event", "device": "fake", "fw": "sim", "seq": seq,
                      "uptime_ms": int((time.time() - t0) * 1000),
                      "source": {"bird": "piezo0", "knock": "piezo0", "noise": "manual"}[kind],
                      "pre_ms": PRE_MS, "post_ms": POST_MS, "truncated": False,
                      "thr_piezo": thr["piezo"], "thr_accel_mg": thr["accel"],
                      "piezo": {"channels": 2, "n": piezo.shape[1], "rate": PIEZO_RATE,
                                "rate_meas": PIEZO_RATE, "dtype": "uint16"},
                      "accel": {"n": len(accel), "axes": 3, "rate": ACCEL_RATE, "rate_meas": ACCEL_RATE,
                                "dtype": "int16", "mg_per_lsb": 1},
                      "sim_kind": kind}
            conn.sendall(encode_frame(header, encode_event_payload(piezo, accel)))
            seq += 1
    except (BrokenPipeError, ConnectionResetError):
        pass
    finally:
        conn.close()
        srv.close()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=5005)
    ap.add_argument("--interval", type=float, default=3.0)
    ap.add_argument("--count", type=int, default=None)
    args = ap.parse_args()
    serve(args.port, args.interval, args.count)
    return 0


if __name__ == "__main__":
    sys.exit(main())
