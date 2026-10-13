#!/usr/bin/env python3
"""Auswertung der PoC-Messdaten.

    python gs_analyze.py list    data/                 Merkmalstabelle (optional --csv)
    python gs_analyze.py plot    data/…/datei.npz      Zeitverlauf und Spektrum eines Ereignisses
    python gs_analyze.py summary data/ [--plot]        Kennzahlen je Label (z. B. SNR je Sensorposition)
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np

from gs_features import event_features
from gs_protocol import find_events, load_event

LIST_COLUMNS = [
    "label", "source", "seq",
    "piezo0_peak", "piezo0_snr_db", "piezo0_duration_ms", "piezo0_dominant_hz", "piezo0_impulses",
    "piezo1_peak", "piezo1_snr_db", "piezo1_duration_ms", "piezo1_dominant_hz", "piezo1_impulses",
    "accel_peak_axis_mg", "accel_snr_db", "accel_peak_400hz_mg", "accel_peak_100hz_mg",
]

SUMMARY_METRICS = ["piezo0_snr_db", "piezo1_snr_db", "accel_snr_db", "piezo0_peak", "piezo1_peak", "accel_peak_axis_mg"]


def collect(paths: list[str]) -> list[dict]:
    rows = []
    for p in find_events(paths):
        f = event_features(load_event(p))
        f["file"] = str(p)
        rows.append(f)
    return rows


def fmt(v) -> str:
    if isinstance(v, float):
        return f"{v:.1f}"
    return "" if v is None else str(v)


def cmd_list(args) -> int:
    rows = collect(args.paths)
    if not rows:
        print("keine Ereignisse gefunden")
        return 1
    if args.csv:
        keys = sorted({k for r in rows for k in r})
        with open(args.csv, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=keys, delimiter=";")
            w.writeheader()
            w.writerows(rows)
        print(f"{len(rows)} Ereignisse nach {args.csv} geschrieben")
        return 0
    cols = [c for c in LIST_COLUMNS if any(c in r for r in rows)]
    table = [[fmt(r.get(c)) for c in cols] for r in rows]
    widths = [max(len(c), *(len(t[i]) for t in table)) for i, c in enumerate(cols)]
    print("  ".join(c.ljust(w) for c, w in zip(cols, widths)))
    for t in table:
        print("  ".join(v.ljust(w) for v, w in zip(t, widths)))
    return 0


def cmd_summary(args) -> int:
    rows = collect(args.paths)
    if not rows:
        print("keine Ereignisse gefunden")
        return 1
    groups: dict[str, list[dict]] = defaultdict(list)
    for r in rows:
        groups[r.get(args.by) or "(ohne)"].append(r)

    metrics = [m for m in SUMMARY_METRICS if any(m in r for r in rows)]
    header = [args.by, "n"] + [f"{m} (min/median)" for m in metrics]
    print(" | ".join(header))
    for key in sorted(groups):
        g = groups[key]
        cells = [key, str(len(g))]
        for m in metrics:
            vals = np.array([r[m] for r in g if m in r], dtype=float)
            cells.append(f"{vals.min():.1f} / {np.median(vals):.1f}" if vals.size else "-")
        print(" | ".join(cells))

    if args.plot:
        import matplotlib.pyplot as plt

        x_key, y_key = args.x, args.y
        fig, ax = plt.subplots(figsize=(8, 6))
        for key in sorted(groups):
            pts = [(r[x_key], r[y_key]) for r in groups[key] if x_key in r and y_key in r]
            if pts:
                xs, ys = zip(*pts)
                ax.scatter(xs, ys, label=key, alpha=0.75)
        ax.set_xlabel(x_key)
        ax.set_ylabel(y_key)
        ax.grid(alpha=0.3)
        ax.legend(fontsize=8)
        fig.tight_layout()
        if args.save:
            fig.savefig(args.save, dpi=130)
            print(f"Diagramm gespeichert: {args.save}")
        else:
            plt.show()
    return 0


def cmd_plot(args) -> int:
    import matplotlib.pyplot as plt

    ev = load_event(Path(args.file))
    rate = ev.piezo_rate
    channels = ev.piezo.shape[0]
    acc = ev.accel_mg
    rows = channels + (1 if acc is not None else 0)
    fig, axes = plt.subplots(rows, 2, figsize=(12, 2.6 * rows), squeeze=False,
                             gridspec_kw={"width_ratios": [3, 2]})

    names = args.names.split(",") if args.names else [f"Piezo {c}" for c in range(channels)]
    pre_n = int(round(ev.pre_ms / 1000 * rate))
    t = (np.arange(ev.piezo.shape[1]) - pre_n) / rate * 1000
    for ch in range(channels):
        x = ev.piezo[ch].astype(float)
        x -= np.median(x[:pre_n])
        axes[ch, 0].plot(t, x, lw=0.7)
        axes[ch, 0].set_ylabel(f"{names[ch] if ch < len(names) else ch}\n[Counts]")
        post = x[pre_n:pre_n + int(0.1 * rate)]
        spec = np.abs(np.fft.rfft(post * np.hanning(len(post))))
        f = np.fft.rfftfreq(len(post), 1 / rate)
        axes[ch, 1].semilogy(f, spec + 1e-9, lw=0.7)
        axes[ch, 1].set_xlim(0, min(rate / 2, args.fmax))

    if acc is not None:
        arate = ev.accel_rate
        apre = int(round(ev.pre_ms / 1000 * arate))
        ta = (np.arange(len(acc)) - apre) / arate * 1000
        dev = acc - np.median(acc[:apre], axis=0)
        r = rows - 1
        for i, axis in enumerate("xyz"):
            axes[r, 0].plot(ta, dev[:, i], lw=0.7, label=axis)
        axes[r, 0].set_ylabel("Accel\n[mg]")
        axes[r, 0].legend(fontsize=8, loc="upper right")
        post = dev[apre:apre + int(0.1 * arate)]
        f = np.fft.rfftfreq(len(post), 1 / arate)
        for i in range(3):
            spec = np.abs(np.fft.rfft(post[:, i] * np.hanning(len(post))))
            axes[r, 1].semilogy(f, spec + 1e-9, lw=0.7)

    for a in axes[:, 0]:
        a.axvline(0, color="grey", lw=0.5, ls="--")
        a.grid(alpha=0.3)
    for a in axes[:, 1]:
        a.grid(alpha=0.3)
    axes[-1, 0].set_xlabel("Zeit relativ zum Trigger [ms]")
    axes[-1, 1].set_xlabel("Frequenz [Hz] (erste 100 ms)")
    m = ev.meta
    fig.suptitle(f"#{m.get('seq')} {m.get('source')} – {m.get('label', '')} – {m.get('received_at', '')}")
    fig.tight_layout()
    if args.save:
        fig.savefig(args.save, dpi=130)
        print(f"Diagramm gespeichert: {args.save}")
    else:
        plt.show()
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("list", help="Merkmalstabelle")
    p.add_argument("paths", nargs="+")
    p.add_argument("--csv", help="als CSV (Semikolon) exportieren")
    p.set_defaults(func=cmd_list)

    p = sub.add_parser("summary", help="Kennzahlen je Gruppe")
    p.add_argument("paths", nargs="+")
    p.add_argument("--by", default="label", help="Gruppierung (Standard: label)")
    p.add_argument("--plot", action="store_true", help="Streudiagramm anzeigen")
    p.add_argument("--x", default="piezo0_duration_ms")
    p.add_argument("--y", default="piezo0_peak")
    p.add_argument("--save", help="Diagramm als PNG speichern statt anzeigen")
    p.set_defaults(func=cmd_summary)

    p = sub.add_parser("plot", help="Ereignis darstellen")
    p.add_argument("file")
    p.add_argument("--names", help="Kanalnamen, z. B. 'Glas Ecke,Holzrahmen'")
    p.add_argument("--fmax", type=float, default=5000)
    p.add_argument("--save", help="als PNG speichern statt anzeigen")
    p.set_defaults(func=cmd_plot)

    args = ap.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
