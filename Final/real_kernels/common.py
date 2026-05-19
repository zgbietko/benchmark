from __future__ import annotations

import csv
import platform
import re
import socket
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Dict, Iterable


ROOT = Path(__file__).resolve().parents[1]


def slugify(s: str) -> str:
    out = re.sub(r"[^a-zA-Z0-9]+", "_", (s or "").strip()).strip("_")
    return out.lower() or "unknown"


def make_csv_path(kernel_name: str, backend: str, device_name: str, device_index: int) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        data_dir = Path(run_root) / "real_kernels"
    else:
        data_dir = ROOT / "data" / "real_kernels"
    data_dir.mkdir(parents=True, exist_ok=True)
    fname = f"{kernel_name}__backend-{backend}__device-{slugify(device_name)}__dev{device_index}.csv"
    return data_dir / fname


def _ensure_writable_or_fallback(path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        if os.access(path, os.W_OK):
            return path
    else:
        if os.access(path.parent, os.W_OK):
            return path

    user = slugify(os.environ.get("USER", "") or os.environ.get("LOGNAME", "") or "user")
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    fallback = path.with_name(path.stem + f"__user-{user}__ts-{ts}" + path.suffix)
    print(f"[WARN] No write access to: {path}")
    print(f"[WARN] Using fallback CSV: {fallback}")
    return fallback


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
    rows = list(rows)
    if not rows:
        return
    csv_path = _ensure_writable_or_fallback(csv_path)
    fieldnames = list(rows[0].keys())
    write_header = not csv_path.exists()
    with csv_path.open("a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        if write_header:
            w.writeheader()
        for r in rows:
            w.writerow(r)


def run_warmups(count: int, fn: Callable[[], object]) -> None:
    for _ in range(max(int(count or 0), 0)):
        fn()
