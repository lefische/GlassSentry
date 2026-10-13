"""Ende-zu-Ende-Test: simuliertes Gerät → Empfänger → Dateien → Merkmale."""

import socket
import sys
import threading
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import fake_device  # noqa: E402
import gs_analyze  # noqa: E402
from gs_features import event_features  # noqa: E402
from gs_protocol import find_events, load_event  # noqa: E402
from gs_receiver import Receiver  # noqa: E402


def free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def test_receive_and_analyze(tmp_path: Path) -> None:
    port = free_port()
    dev = threading.Thread(target=fake_device.serve, args=(port, 0.05, 3), daemon=True)
    dev.start()

    rx = Receiver("127.0.0.1", port, tmp_path, "test", verbose_status=False)
    rx.run(max_events=3)

    files = find_events([tmp_path])
    assert len(files) == 3
    assert (tmp_path / "events.jsonl").read_text().count("\n") == 3
    assert (tmp_path / "status.jsonl").exists()

    feats = {load_event(f).meta["sim_kind"]: event_features(load_event(f)) for f in files}

    bird, knock, noise = feats["bird"], feats["knock"], feats["noise"]
    assert bird["label"] == "test"
    assert bird["piezo0_snr_db"] > 30
    assert noise["piezo0_snr_db"] < 20
    assert bird["piezo0_impulses"] == 1
    assert knock["piezo0_impulses"] == 2
    assert 300 <= bird["piezo0_dominant_hz"] <= 450
    assert bird["accel_peak_axis_mg"] > 40
    # Blockmittelung auf 100 Hz dämpft die 380-Hz-Schwingung deutlich
    assert bird["accel_peak_100hz_mg"] < 0.5 * bird["accel_peak_axis_mg"]

    assert gs_analyze.main(["list", str(tmp_path)]) == 0
    assert gs_analyze.main(["summary", str(tmp_path), "--plot", "--save", str(tmp_path / "s.png")]) == 0
    assert gs_analyze.main(["plot", str(files[0]), "--save", str(tmp_path / "e.png")]) == 0
    assert (tmp_path / "e.png").stat().st_size > 0
