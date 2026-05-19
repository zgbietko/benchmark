#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
from datetime import datetime
from pathlib import Path
from typing import Dict, List


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GPU_DIR = ROOT / "data" / "gpu"


BANDWIDTH_V24 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "transfer_kind", "memory_mode", "copy_method",
    "size_bytes", "num_elements", "iters_inner", "run_idx", "elapsed_s",
    "throughput_gbps", "gbps", "energy_joule", "energy_j", "avg_power_watt",
    "avg_power_w", "energy_source", "sample_interval_s",
]
BANDWIDTH_V17 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "size_bytes", "num_elements", "run_idx",
    "elapsed_s", "throughput_gbps", "energy_joule", "avg_power_watt",
    "energy_source", "sample_interval_s",
]
BANDWIDTH_V18_MIX = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "transfer_kind", "size_bytes", "num_elements",
    "run_idx", "elapsed_s", "throughput_gbps", "energy_joule",
    "avg_power_watt", "energy_source", "sample_interval_s",
]
BANDWIDTH_V16 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "size_bytes", "num_elements", "iters_inner",
    "run_idx", "elapsed_s", "throughput_gbps", "energy_joule", "avg_power_watt",
]
BANDWIDTH_V15 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "size_bytes", "num_elements", "run_idx",
    "elapsed_s", "throughput_gbps", "energy_joule", "avg_power_watt",
]

COMPUTE_V22 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "vector_len", "inner_iters", "run_idx",
    "elapsed_s", "throughput_gflops", "gflops", "n_elements", "iters_inner",
    "energy_joule", "energy_j", "avg_power_watt", "avg_power_w",
    "energy_source", "sample_interval_s",
]
COMPUTE_V20 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "n_elements", "iters_inner", "run_idx",
    "elapsed_s", "gflops", "throughput_gflops", "energy_joule", "energy_j",
    "avg_power_watt", "avg_power_w", "energy_source", "sample_interval_s",
]
COMPUTE_V17 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "vector_len", "inner_iters", "run_idx",
    "elapsed_s", "throughput_gflops", "energy_joule", "avg_power_watt",
    "energy_source", "sample_interval_s",
]
COMPUTE_V15 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "vector_len", "inner_iters", "run_idx",
    "elapsed_s", "throughput_gflops", "energy_joule", "avg_power_watt",
]

PEAK_V26 = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "n_elements", "iters_inner", "runs_per_config",
    "gflops_peak", "gflops_mean", "gflops_sigma", "gflops",
    "throughput_gflops", "energy_joule_mean", "energy_joule_sigma",
    "energy_joule", "energy_j", "avg_power_watt_mean", "avg_power_watt_sigma",
    "avg_power_watt", "avg_power_w", "energy_source", "sample_interval_s",
]

BANDWIDTH_CANON = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "transfer_kind", "memory_mode", "copy_method",
    "size_bytes", "num_elements", "iters_inner", "run_idx", "elapsed_s",
    "throughput_gbps", "gbps", "energy_joule", "energy_j", "avg_power_watt",
    "avg_power_w", "energy_source", "sample_interval_s", "energy_supported",
]
COMPUTE_CANON = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "vector_len", "inner_iters", "n_elements",
    "iters_inner", "run_idx", "elapsed_s", "throughput_gflops", "gflops",
    "energy_joule", "energy_j", "avg_power_watt", "avg_power_w",
    "energy_source", "sample_interval_s", "energy_supported",
]
PEAK_CANON = [
    "timestamp", "backend", "system", "arch", "hostname", "python_version",
    "gpu_model", "gpu_index", "vector_len", "inner_iters", "n_elements",
    "iters_inner", "runs_per_config", "run_idx", "elapsed_s", "gflops_peak",
    "gflops_mean", "gflops_sigma", "throughput_gflops", "gflops",
    "energy_joule_mean", "energy_joule_sigma", "energy_joule", "energy_j",
    "avg_power_watt_mean", "avg_power_watt_sigma", "avg_power_watt",
    "avg_power_w", "energy_source", "sample_interval_s", "energy_supported",
]


def _to_float(v: str | None) -> float | None:
    if v is None:
        return None
    s = str(v).strip()
    if not s:
        return None
    try:
        return float(s)
    except ValueError:
        return None


def _to_int(v: str | None) -> int | None:
    x = _to_float(v)
    if x is None:
        return None
    try:
        return int(x)
    except Exception:
        return None


def _map_values(values: List[str], layout: List[str]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for i, k in enumerate(layout):
        out[k] = values[i].strip() if i < len(values) else ""
    return out


def _read_raw_rows(path: Path) -> tuple[list[str], list[list[str]]]:
    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.reader(f)
        rows = list(reader)
    if not rows:
        return [], []
    return rows[0], rows[1:]


def _detect_kind(path: Path) -> str:
    stem = path.stem
    if "bandwidth" in stem:
        return "bandwidth"
    if "compute_fma_peak" in stem:
        return "peak"
    if "compute_fma" in stem:
        return "compute"
    return "unknown"


def _choose_layout(kind: str, row_len: int) -> List[str]:
    if kind == "bandwidth":
        return {
            24: BANDWIDTH_V24,
            18: BANDWIDTH_V18_MIX,
            17: BANDWIDTH_V17,
            16: BANDWIDTH_V16,
            15: BANDWIDTH_V15,
        }.get(row_len, BANDWIDTH_V24 if row_len > 20 else BANDWIDTH_V17)
    if kind == "compute":
        return {
            22: COMPUTE_V22,
            20: COMPUTE_V20,
            17: COMPUTE_V17,
            15: COMPUTE_V15,
        }.get(row_len, COMPUTE_V22 if row_len > 20 else COMPUTE_V17)
    if kind == "peak":
        return {
            26: PEAK_V26,
            22: COMPUTE_V22,
            20: COMPUTE_V20,
            17: COMPUTE_V17,
            15: COMPUTE_V15,
        }.get(row_len, PEAK_V26 if row_len > 24 else COMPUTE_V22)
    return []


def _energy_supported(source: str, e_j: str, p_w: str) -> str:
    src = (source or "").strip().lower()
    if src and src not in {
        "unavailable",
        "no_gpu_energy_backend",
        "unsupported_macos_gpu_energy",
    } and not src.startswith("unsupported_"):
        return "1"
    e = _to_float(e_j)
    p = _to_float(p_w)
    if e is not None and not math.isnan(e):
        return "1"
    if p is not None and not math.isnan(p):
        return "1"
    return "0"


def _normalize_bandwidth(row: Dict[str, str]) -> Dict[str, str]:
    out = {k: "" for k in BANDWIDTH_CANON}
    for k in BANDWIDTH_CANON:
        if k in row:
            out[k] = row.get(k, "")

    backend = (out.get("backend", "") or row.get("backend", "")).strip().lower()
    if not out["transfer_kind"]:
        out["transfer_kind"] = "device_to_device"
    if not out["memory_mode"]:
        out["memory_mode"] = "shared" if backend == "metal" else "device"
    if not out["copy_method"]:
        out["copy_method"] = "kernel" if backend == "metal" else "memcpy"
    if not out["gbps"]:
        out["gbps"] = out.get("throughput_gbps", "")
    if not out["throughput_gbps"]:
        out["throughput_gbps"] = out.get("gbps", "")
    if not out["energy_j"]:
        out["energy_j"] = out.get("energy_joule", "")
    if not out["energy_joule"]:
        out["energy_joule"] = out.get("energy_j", "")
    if not out["avg_power_w"]:
        out["avg_power_w"] = out.get("avg_power_watt", "")
    if not out["avg_power_watt"]:
        out["avg_power_watt"] = out.get("avg_power_w", "")
    if not out["energy_source"]:
        out["energy_source"] = "unavailable"
    if not out["iters_inner"]:
        out["iters_inner"] = "1"
    out["energy_supported"] = _energy_supported(
        out.get("energy_source", ""),
        out.get("energy_joule", ""),
        out.get("avg_power_watt", ""),
    )
    return out


def _normalize_compute(row: Dict[str, str]) -> Dict[str, str]:
    out = {k: "" for k in COMPUTE_CANON}
    for k in COMPUTE_CANON:
        if k in row:
            out[k] = row.get(k, "")

    if not out["n_elements"]:
        out["n_elements"] = out.get("vector_len", "")
    if not out["vector_len"]:
        out["vector_len"] = out.get("n_elements", "")
    if not out["iters_inner"]:
        out["iters_inner"] = out.get("inner_iters", "")
    if not out["inner_iters"]:
        out["inner_iters"] = out.get("iters_inner", "")
    if not out["gflops"]:
        out["gflops"] = out.get("throughput_gflops", "")
    if not out["throughput_gflops"]:
        out["throughput_gflops"] = out.get("gflops", "")
    if not out["energy_j"]:
        out["energy_j"] = out.get("energy_joule", "")
    if not out["energy_joule"]:
        out["energy_joule"] = out.get("energy_j", "")
    if not out["avg_power_w"]:
        out["avg_power_w"] = out.get("avg_power_watt", "")
    if not out["avg_power_watt"]:
        out["avg_power_watt"] = out.get("avg_power_w", "")
    if not out["energy_source"]:
        out["energy_source"] = "unavailable"
    out["energy_supported"] = _energy_supported(
        out.get("energy_source", ""),
        out.get("energy_joule", ""),
        out.get("avg_power_watt", ""),
    )
    return out


def _normalize_peak(row: Dict[str, str]) -> Dict[str, str]:
    out = {k: "" for k in PEAK_CANON}
    for k in PEAK_CANON:
        if k in row:
            out[k] = row.get(k, "")

    if not out["n_elements"]:
        out["n_elements"] = row.get("vector_len", "")
    if not out["vector_len"]:
        out["vector_len"] = row.get("n_elements", "")
    if not out["iters_inner"]:
        out["iters_inner"] = row.get("inner_iters", "")
    if not out["inner_iters"]:
        out["inner_iters"] = row.get("iters_inner", "")
    if not out["gflops"]:
        out["gflops"] = row.get("gflops_mean", "") or row.get("throughput_gflops", "")
    if not out["throughput_gflops"]:
        out["throughput_gflops"] = row.get("gflops", "")
    if not out["gflops_mean"] and out["gflops"]:
        out["gflops_mean"] = out["gflops"]
    if not out["gflops_peak"] and out["gflops"]:
        out["gflops_peak"] = out["gflops"]
    if not out["energy_j"]:
        out["energy_j"] = row.get("energy_joule", "") or row.get("energy_joule_mean", "")
    if not out["energy_joule"]:
        out["energy_joule"] = out["energy_j"]
    if not out["energy_joule_mean"] and out["energy_joule"]:
        out["energy_joule_mean"] = out["energy_joule"]
    if not out["avg_power_w"]:
        out["avg_power_w"] = row.get("avg_power_watt", "") or row.get("avg_power_watt_mean", "")
    if not out["avg_power_watt"]:
        out["avg_power_watt"] = out["avg_power_w"]
    if not out["avg_power_watt_mean"] and out["avg_power_watt"]:
        out["avg_power_watt_mean"] = out["avg_power_watt"]
    if not out["energy_source"]:
        out["energy_source"] = "unavailable"
    out["energy_supported"] = _energy_supported(
        out.get("energy_source", ""),
        out.get("energy_joule", ""),
        out.get("avg_power_watt", ""),
    )
    return out


def normalize_file(path: Path, dry_run: bool, make_backup: bool) -> tuple[int, int]:
    kind = _detect_kind(path)
    if kind == "unknown":
        return (0, 0)

    header, raw_rows = _read_raw_rows(path)
    if not header:
        return (0, 0)

    normalized_rows: List[Dict[str, str]] = []
    fixed = 0
    skipped = 0
    for vals in raw_rows:
        if not vals:
            continue
        layout = _choose_layout(kind, len(vals))
        if not layout:
            skipped += 1
            continue

        mapped = _map_values(vals, layout)
        if kind == "bandwidth":
            nr = _normalize_bandwidth(mapped)
            # skip rows where size_bytes is missing/non-numeric after normalization
            size = _to_int(nr.get("size_bytes"))
            if size is None or size <= 0:
                skipped += 1
                continue
        elif kind == "compute":
            nr = _normalize_compute(mapped)
        else:
            nr = _normalize_peak(mapped)
        normalized_rows.append(nr)
        if len(vals) != len(layout) or header != layout:
            fixed += 1

    if not dry_run:
        if make_backup:
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            backup = path.with_suffix(path.suffix + f".bak_{ts}")
            backup.write_bytes(path.read_bytes())

        canon = BANDWIDTH_CANON if kind == "bandwidth" else (COMPUTE_CANON if kind == "compute" else PEAK_CANON)
        try:
            with path.open("w", encoding="utf-8", newline="") as f:
                w = csv.DictWriter(f, fieldnames=canon)
                w.writeheader()
                for row in normalized_rows:
                    w.writerow(row)
        except PermissionError:
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            fallback = path.with_name(path.stem + f"__normalized_{ts}" + path.suffix)
            with fallback.open("w", encoding="utf-8", newline="") as f:
                w = csv.DictWriter(f, fieldnames=canon)
                w.writeheader()
                for row in normalized_rows:
                    w.writerow(row)
            print(f"[WARN] Brak zapisu do {path.name}; zapisano fallback: {fallback.name}")

    return fixed, skipped


def main() -> None:
    ap = argparse.ArgumentParser(description="Normalizuje historyczne CSV GPU do spójnego schematu.")
    ap.add_argument("--data-dir", type=Path, default=DEFAULT_GPU_DIR)
    ap.add_argument("--mode", choices=["latest", "all"], default="all")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-backup", action="store_true")
    args = ap.parse_args()

    data_dir = args.data_dir
    if not data_dir.exists():
        print(f"[INFO] Brak katalogu: {data_dir}")
        return

    files = sorted(data_dir.glob("*.csv"))
    if args.mode == "latest":
        by_stem: Dict[str, Path] = {}
        for p in files:
            key = p.stem.split("__user-", 1)[0].split("__ts-", 1)[0]
            prev = by_stem.get(key)
            if prev is None or p.stat().st_mtime > prev.stat().st_mtime:
                by_stem[key] = p
        files = sorted(by_stem.values())

    total_fixed = 0
    total_skipped = 0
    touched = 0
    for p in files:
        fixed, skipped = normalize_file(
            path=p,
            dry_run=args.dry_run,
            make_backup=not args.no_backup,
        )
        if fixed > 0 or skipped > 0:
            touched += 1
        total_fixed += fixed
        total_skipped += skipped
        print(f"[OK] {p.name}: fixed={fixed}, skipped={skipped}")

    print()
    print(
        f"[DONE] files={len(files)}, touched={touched}, "
        f"fixed_rows={total_fixed}, skipped_rows={total_skipped}, dry_run={args.dry_run}"
    )


if __name__ == "__main__":
    main()
