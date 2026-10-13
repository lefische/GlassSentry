"""Merkmale eines Ereignisses für Auswertung und spätere Klassifikation.

Alle Merkmale werden je Kanal aus dem Nachlauf berechnet; Ruhepegel und
Rauschen stammen aus dem Vorlauf (vor dem Trigger).
"""

from __future__ import annotations

import numpy as np

from gs_protocol import Event

# Simulierte Datenraten für sparsame Sensoren (Frage F3 im PoC-Plan)
SIMULATED_ACCEL_RATES = (400, 100)


def _impulse_count(env: np.ndarray, rate: float, level: float, min_gap_ms: float = 30.0) -> int:
    """Zählt getrennte Impulse: Anstiege der Hüllkurve über ``level``."""
    above = env > level
    rising = np.flatnonzero(above[1:] & ~above[:-1]) + 1
    if above[0]:
        rising = np.r_[0, rising]
    count, last = 0, -np.inf
    min_gap = min_gap_ms / 1000 * rate
    for idx in rising:
        if idx - last >= min_gap:
            count += 1
        last = idx
    return count


def _envelope(x: np.ndarray, rate: float, window_ms: float = 2.0) -> np.ndarray:
    """Hüllkurve als gleitendes Maximum des Betrags."""
    w = max(1, int(rate * window_ms / 1000))
    a = np.abs(x)
    if w == 1 or len(a) < w:
        return a
    padded = np.pad(a, (w // 2, w - 1 - w // 2), mode="edge")
    return np.lib.stride_tricks.sliding_window_view(padded, w).max(axis=1)


def signal_features(x: np.ndarray, rate: float, pre_n: int) -> dict:
    """Merkmale eines einzelnen Kanals (x als float, beliebige Einheit)."""
    pre = x[:pre_n] if pre_n > 0 else x[:1]
    base = float(np.median(pre))
    noise = float(np.std(pre)) or 1e-9
    y = x - base
    post = y[pre_n:]
    if post.size == 0:
        return {}

    a = np.abs(post)
    peak_i = int(np.argmax(a))
    peak = float(a[peak_i])
    env = _envelope(post, rate)

    # Dauer: Hüllkurve über max(5·Rauschen, 10 % Spitze)
    level = max(5 * noise, 0.1 * peak)
    idx = np.flatnonzero(env > level)
    duration_ms = float((idx[-1] - idx[0] + 1) / rate * 1000) if idx.size else 0.0

    # Anstiegszeit: erstes Überschreiten von 10 % bis zur Spitze
    first = np.flatnonzero(a > 0.1 * peak)
    rise_ms = float((peak_i - first[0]) / rate * 1000) if first.size and peak > 0 else 0.0

    # Dominante Frequenz in den ersten 100 ms ab Trigger
    seg = post[: max(16, int(0.1 * rate))]
    spec = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
    freqs = np.fft.rfftfreq(len(seg), 1 / rate)
    valid = freqs >= 20
    dominant_hz = float(freqs[valid][np.argmax(spec[valid])]) if valid.any() else 0.0
    centroid_hz = float((freqs[valid] * spec[valid]).sum() / spec[valid].sum()) if spec[valid].sum() else 0.0

    return {
        "base": base,
        "noise_rms": noise,
        "peak": peak,
        "snr_db": float(20 * np.log10(peak / noise)) if peak > 0 else 0.0,
        "energy": float((post ** 2).sum() / rate),
        "duration_ms": duration_ms,
        "rise_ms": rise_ms,
        "dominant_hz": dominant_hz,
        "centroid_hz": centroid_hz,
        "impulses": _impulse_count(env, rate, max(5 * noise, 0.3 * peak)),
    }


def _decimate_peak(dev: np.ndarray, rate: float, target: float, pre_n: int) -> float:
    """Spitzenwert nach Blockmittelung auf ``target`` Hz (grobes Modell des
    internen Tiefpasses eines sparsamen Sensors)."""
    k = int(round(rate / target))
    if k <= 1:
        return float(np.abs(dev[pre_n:]).max())
    post = dev[pre_n:]
    n = len(post) // k * k
    if n == 0:
        return 0.0
    blocks = post[:n].reshape(-1, k, *post.shape[1:]).mean(axis=1)
    return float(np.abs(blocks).max())


def event_features(ev: Event) -> dict:
    """Flaches Dict mit Merkmalen aller Kanäle, z. B. ``piezo0_peak``."""
    out: dict = {
        "seq": ev.meta.get("seq"),
        "source": ev.meta.get("source"),
        "label": ev.meta.get("label", ""),
        "received_at": ev.meta.get("received_at", ""),
    }
    rate = ev.piezo_rate
    pre_n = int(round(ev.pre_ms / 1000 * rate))
    for ch in range(ev.piezo.shape[0]):
        f = signal_features(ev.piezo[ch].astype(float), rate, pre_n)
        out.update({f"piezo{ch}_{k}": v for k, v in f.items()})

    acc = ev.accel_mg
    if acc is not None and len(acc):
        arate = ev.accel_rate
        apre = int(round(ev.pre_ms / 1000 * arate))
        dev = acc - np.median(acc[:apre], axis=0)
        mag = np.linalg.norm(dev, axis=1)
        # Betrag ist immer ≥ 0: Rauschen und Ruhepegel aus dem Vorlauf
        f = signal_features(mag, arate, apre)
        out.update({f"accel_{k}": v for k, v in f.items()})
        out["accel_peak_axis_mg"] = float(np.abs(dev[apre:]).max())
        for r in SIMULATED_ACCEL_RATES:
            out[f"accel_peak_{r}hz_mg"] = _decimate_peak(dev, arate, r, apre)
    return out
