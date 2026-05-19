from __future__ import annotations

import csv
import os
import platform
import re
import socket
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Dict, Iterable

ROOT = Path(__file__).resolve().parents[1]


def slugify(value: str) -> str:
    out = re.sub(r"[^a-zA-Z0-9]+", "_", (value or "").strip()).strip("_")
    return out.lower() or "unknown"


def make_csv_path(name: str, backend: str, device_name: str, device_index: int) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        data_dir = Path(run_root) / "ai_accel"
    else:
        data_dir = ROOT / "data" / "ai_accel"
    data_dir.mkdir(parents=True, exist_ok=True)
    fname = f"{name}__backend-{backend}__device-{slugify(device_name)}__dev{device_index}.csv"
    return data_dir / fname


def base_meta(backend: str, device_name: str, device_index: int) -> Dict[str, object]:
    return {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "backend": backend,
        "system": platform.system(),
        "arch": platform.machine(),
        "hostname": socket.gethostname(),
        "python_version": platform.python_version(),
        "device_name": device_name,
        "device_index": device_index,
    }


def append_rows(csv_path: Path, rows: Iterable[Dict[str, object]]) -> None:
    items = list(rows)
    if not items:
        return
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = list(items[0].keys())
    write_header = not csv_path.exists()
    with csv_path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        for row in items:
            writer.writerow(row)


def run_warmups(count: int, fn: Callable[[], object]) -> None:
    for _ in range(max(int(count or 0), 0)):
        fn()
