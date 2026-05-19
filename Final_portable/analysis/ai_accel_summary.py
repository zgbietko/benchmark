#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import os
from collections import defaultdict
from pathlib import Path
from statistics import mean, pstdev

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "ai_accel"
RUNS_DIR = ROOT / "data" / "runs"


def _to_float(v: object) -> float | None:
    try:
        if v is None:
            return None
        s = str(v).strip()
        if not s or s.lower() == "nan":
            return None
        return float(s)
    except Exception:
        return None


def _latest_session_dir() -> Path | None:
    latest = RUNS_DIR / "latest"
    if latest.exists():
        try:
            p = latest.resolve()
            if p.exists():
                return p
        except Exception:
            pass
    latest_txt = RUNS_DIR / "latest.txt"
    if latest_txt.exists():
        try:
            name = latest_txt.read_text(encoding="utf-8").strip()
        except Exception:
            name = ""
        if name:
            p = RUNS_DIR / name
            if p.exists():
                return p
    return None


def _source_dir(scope: str, session: str) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root) / "ai_accel"
        if p.exists():
            return p
    if scope == "session":
        if session and session != "latest":
            return RUNS_DIR / session / "ai_accel"
        latest = _latest_session_dir()
        if latest is not None:
            return latest / "ai_accel"
    return DATA_DIR


def main() -> None:
    ap = argparse.ArgumentParser(description="Summary for AI acceleration benchmark CSV files.")
    ap.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    ap.add_argument("--session", default="latest")
    args = ap.parse_args()

    scope = args.scope
    if scope == "auto":
        scope = "session" if os.environ.get("BENCH_RUN_DIR", "").strip() else "global"

    src = _source_dir(scope, args.session)
    print(f"[INFO] AI accel summary source: {src}")
    if not src.exists():
        print("[INFO] no directory")
        return

    files = sorted(src.glob("*.csv"))
    if not files:
        print("[INFO] no files")
        return

    grouped = defaultdict(list)
    for path in files:
        with path.open("r", encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            for row in reader:
                if str(row.get("status", "ok")).strip().lower() != "ok":
                    continue
                backend = str(row.get("backend", "unknown"))
                device = str(row.get("device_name", "unknown"))
                kernel = str(row.get("kernel", ""))
                precision = str(row.get("precision", ""))
                compute_units = str(row.get("compute_units", ""))
                native_path = str(row.get("native_ai_path", ""))
                g = _to_float(row.get("gflops"))
                if g is None:
                    continue
                key = (backend, device, kernel, precision, compute_units, native_path)
                grouped[key].append(g)

    print("=== AI ACCEL SUMMARY ===")
    if not grouped:
        print("[INFO] no successful rows")
        return

    for key in sorted(grouped.keys()):
        vals = grouped[key]
        mu = mean(vals)
        sd = pstdev(vals) if len(vals) > 1 else 0.0
        backend, device, kernel, precision, compute_units, native_path = key
        units = compute_units if compute_units else "-"
        print(
            f"- {backend:8s} | {device:25s} | {kernel:18s} | {precision:7s} | units={units:11s} | "
            f"path={native_path:26s} | {mu:10.2f} +/- {sd:8.2f} GFLOP/s"
        )


if __name__ == "__main__":
    main()
