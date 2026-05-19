#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import os
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from statistics import median
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
RUNS_DIR = ROOT / "data" / "runs"
GLOBAL_DIR = ROOT / "data" / "ai_accel"


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
    return GLOBAL_DIR


def _read_rows(src: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for path in sorted(src.glob("*.csv")):
        with path.open("r", encoding="utf-8", newline="") as handle:
            rows.extend(csv.DictReader(handle))
    return rows


def _limitations(row: dict[str, str]) -> list[str]:
    limits: list[str] = []
    impl = str(row.get("implementation_level", "")).strip().lower()
    backend = str(row.get("backend", "")).strip().lower()
    kernel = str(row.get("kernel", "")).strip().lower()
    if impl == "portable_proxy":
        limits.append("portable proxy path, not vendor-optimized GEMM")
    if backend == "coreml" or kernel == "coreml_mlp_predict":
        limits.append("Core ML may not expose exact execution unit (CPU/GPU/NE)")
    if impl == "cpu_fallback":
        limits.append("CPU fallback path")
    return limits


def build_report(rows: list[dict[str, str]], source_dir: Path) -> dict[str, Any]:
    grouped: dict[tuple[str, str, str, str, str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        key = (
            str(row.get("backend", "")).strip().lower() or "unknown",
            str(row.get("device_name", "")).strip() or "unknown",
            str(row.get("kernel", "")).strip().lower() or "unknown",
            str(row.get("precision", "")).strip().lower() or "unknown",
            str(row.get("compute_units", "")).strip().lower() or "",
            str(row.get("implementation_level", "")).strip().lower() or "unknown",
        )
        grouped[key].append(row)

    entries: list[dict[str, Any]] = []
    for key in sorted(grouped.keys()):
        backend, device_name, kernel, precision, compute_units, impl = key
        bucket = grouped[key]
        status_counts = Counter(str(r.get("status", "unknown")).strip().lower() or "unknown" for r in bucket)
        gvals = [_to_float(r.get("gflops")) for r in bucket]
        tvals = [_to_float(r.get("elapsed_s")) for r in bucket]
        g_ok = [x for x in gvals if x is not None]
        t_ok = [x for x in tvals if x is not None]
        example = bucket[0]
        native_label = str(example.get("native_ai_path", "")).strip()
        vendor_used = str(example.get("vendor_ai_unit_used", "")).strip().lower() or "unknown"
        entry = {
            "backend": backend,
            "execution_device": str(example.get("execution_device", "")).strip() or "unknown",
            "acceleration_class": str(example.get("acceleration_class", "")).strip() or "unknown",
            "native_ai_path": bool(int(float(str(example.get("native_ai_available", "0") or "0")))) if str(example.get("native_ai_available", "")).strip() else False,
            "native_ai_path_label": native_label,
            "vendor_ai_unit_used": vendor_used,
            "implementation_level": impl,
            "kernel": kernel,
            "precision": precision,
            "compute_units": compute_units,
            "device_name": device_name,
            "status_counts": dict(status_counts),
            "samples": len(bucket),
            "median_gflops": None if not g_ok else float(median(g_ok)),
            "median_elapsed_s": None if not t_ok else float(median(t_ok)),
            "limitations": _limitations(example),
        }
        entries.append(entry)

    return {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "source_dir": str(source_dir),
        "rows": len(rows),
        "entries": entries,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description="Build AI acceleration path report (native/proxy/fallback visibility).")
    ap.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    ap.add_argument("--session", default="latest")
    ap.add_argument("--out", default="")
    args = ap.parse_args()

    scope = args.scope
    if scope == "auto":
        scope = "session" if os.environ.get("BENCH_RUN_DIR", "").strip() else "global"
    src = _source_dir(scope, args.session)
    if not src.exists():
        raise SystemExit(f"AI accel source not found: {src}")
    rows = _read_rows(src)
    report = build_report(rows, src)

    if args.out:
        out_path = Path(args.out).expanduser().resolve()
    else:
        run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
        if run_root:
            out_path = Path(run_root) / "ai_accel_path_report.json"
        elif scope == "session":
            if args.session and args.session != "latest":
                out_path = RUNS_DIR / args.session / "ai_accel_path_report.json"
            else:
                latest = _latest_session_dir()
                out_path = (latest / "ai_accel_path_report.json") if latest is not None else (ROOT / "data" / "ai_accel_path_report.json")
        else:
            out_path = ROOT / "data" / "ai_accel_path_report.json"

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"[OK] AI path report: {out_path}")


if __name__ == "__main__":
    main()
