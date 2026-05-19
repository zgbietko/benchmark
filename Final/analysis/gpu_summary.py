#!/usr/bin/env python3
"""analysis/gpu_summary.py

Podsumowanie benchmarków GPU z `data/gpu/*.csv`.
Obsługuje równolegle stare i nowe nazwy kolumn (compat mode).
"""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
import os
from pathlib import Path
from typing import Any, Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "gpu"
RUNS_DIR = ROOT / "data" / "runs"


def _parse_gpu_filename(fp: Path) -> Tuple[str, str, str, str]:
    stem = fp.stem
    # Legacy format:
    #   gpu_compute_fma_cuda_<gpu_slug>_dev0
    #   gpu_bandwidth_metal_<gpu_slug>
    if "__" not in stem and stem.startswith("gpu_"):
        known = [
            "compute_fma_peak",
            "compute_fma",
            "bandwidth",
        ]
        for k in known:
            pref = f"gpu_{k}_"
            if stem.startswith(pref):
                return k, "unknown", "unknown", "unknown"
        return "unknown", "unknown", "unknown", "unknown"

    parts = stem.split("__")
    benchmark = parts[0].replace("gpu_", "", 1) if parts else "unknown"
    backend = "unknown"
    gpu = "unknown"
    dev = "unknown"
    for part in parts[1:]:
        if part.startswith("backend-"):
            backend = part.split("backend-", 1)[1] or backend
        elif part.startswith("gpu-"):
            gpu = part.split("gpu-", 1)[1] or gpu
        elif part.startswith("dev"):
            dev = part
    return benchmark, backend, gpu, dev


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


def _data_dir(scope: str, session: str) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root)
        if (p / "gpu").exists():
            return p / "gpu"
    if scope == "session":
        if session and session != "latest":
            return RUNS_DIR / session / "gpu"
        latest = _latest_session_dir()
        if latest is not None:
            return latest / "gpu"
    return DATA_DIR


def _is_nan(x: float) -> bool:
    return isinstance(x, float) and math.isnan(x)


def _mean_std(values: Iterable[float]) -> Tuple[float, float]:
    vals = [v for v in values if not _is_nan(v)]
    if not vals:
        return float("nan"), float("nan")
    mu = sum(vals) / len(vals)
    if len(vals) == 1:
        return mu, 0.0
    var = sum((v - mu) ** 2 for v in vals) / len(vals)
    return mu, math.sqrt(var)


def _to_float(row: Dict[str, str], *keys: str) -> float:
    for k in keys:
        raw = row.get(k)
        if raw is None:
            continue
        s = str(raw).strip()
        if s == "":
            continue
        try:
            return float(s)
        except ValueError:
            continue
    return float("nan")


def _to_int(row: Dict[str, str], *keys: str) -> int:
    for k in keys:
        raw = row.get(k)
        if raw is None:
            continue
        s = str(raw).strip()
        if s == "":
            continue
        try:
            return int(float(s))
        except ValueError:
            continue
    return -1


def _select_files(files: List[Path], mode: str) -> List[Path]:
    if mode == "all":
        return files
    latest: Dict[Tuple[str, str, str, str], Path] = {}
    latest_mtime: Dict[Tuple[str, str, str, str], float] = {}
    for fp in files:
        key = _parse_gpu_filename(fp)
        mtime = fp.stat().st_mtime
        if key not in latest_mtime or mtime > latest_mtime[key]:
            latest_mtime[key] = mtime
            latest[key] = fp
    return sorted(latest.values())


def _csv_shape_issues(fp: Path) -> Tuple[int, int]:
    try:
        with fp.open("r", newline="") as f:
            r = csv.reader(f)
            rows = list(r)
    except Exception:
        return 0, 1
    if not rows:
        return 0, 0
    header_len = len(rows[0])
    bad = 0
    for row in rows[1:]:
        if len(row) != header_len:
            bad += 1
    return len(rows) - 1, bad


def _fmt_energy(energy_vals: List[float], power_vals: List[float]) -> str:
    em, es = _mean_std(energy_vals)
    pm, ps = _mean_std(power_vals)
    if _is_nan(em) and _is_nan(pm):
        return "energy: n/a"
    if _is_nan(pm):
        return f"energy: {em:7.4f} J ± {es:7.4f}"
    if _is_nan(em):
        return f"P_avg: {pm:7.2f} W ± {ps:7.2f}"
    return f"energy: {em:7.4f} J ± {es:7.4f}, P_avg: {pm:7.2f} W ± {ps:7.2f}"


def summarize(mode: str, strict: bool = False, *, scope: str = "auto", session: str = "latest") -> int:
    if scope == "auto":
        scope = "session" if os.environ.get("BENCH_RUN_DIR", "").strip() else "global"

    data_dir = _data_dir(scope, session)
    print(f"[INFO] GPU summary source: {data_dir}")

    if not data_dir.exists():
        print(f"[INFO] Brak katalogu {data_dir}")
        return 0

    files = sorted(data_dir.glob("*.csv"))
    if not files:
        print(f"[INFO] Brak plików CSV w {data_dir}")
        return 0
    files = _select_files(files, mode)
    total_rows = 0
    shape_bad_rows = 0
    parse_dropped_rows = 0

    for fp in files:
        rows_n, bad_n = _csv_shape_issues(fp)
        total_rows += rows_n
        shape_bad_rows += bad_n

    bw: Dict[Tuple[str, str, str, str, str, int], List[float]] = defaultdict(list)
    bw_energy: Dict[Tuple[str, str, str, str, str, int], List[float]] = defaultdict(list)
    bw_power: Dict[Tuple[str, str, str, str, str, int], List[float]] = defaultdict(list)
    bw_sources: Dict[Tuple[str, str, str, str, str, int], set[str]] = defaultdict(set)

    fma: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    fma_energy: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    fma_power: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    fma_sources: Dict[Tuple[str, str, int, int], set[str]] = defaultdict(set)

    peak_runs: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    peak_runs_energy: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    peak_runs_power: Dict[Tuple[str, str, int, int], List[float]] = defaultdict(list)
    peak_runs_sources: Dict[Tuple[str, str, int, int], set[str]] = defaultdict(set)
    peak_agg: Dict[Tuple[str, str, int, int], Tuple[float, float, float, float, float]] = {}
    peak_agg_source: Dict[Tuple[str, str, int, int], str] = {}
    lat: Dict[Tuple[str, str, int], List[float]] = defaultdict(list)
    lat_energy: Dict[Tuple[str, str, int], List[float]] = defaultdict(list)
    lat_power: Dict[Tuple[str, str, int], List[float]] = defaultdict(list)
    lat_sources: Dict[Tuple[str, str, int], set[str]] = defaultdict(set)

    for fp in files:
        bench_name, backend_f, gpu_f, dev_f = _parse_gpu_filename(fp)
        with fp.open("r", newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                backend = str(row.get("backend", backend_f)).strip() or backend_f
                gpu = str(row.get("gpu_model", gpu_f)).strip() or gpu_f
                if dev_f != "unknown":
                    gpu = f"{gpu} ({dev_f})"

                energy = _to_float(row, "energy_joule", "energy_j", "energy_joule_mean")
                power = _to_float(row, "avg_power_watt", "avg_power_w", "avg_power_watt_mean")
                source = str(row.get("energy_source", "")).strip() or "unknown"

                size_bytes = _to_int(row, "size_bytes")
                gbps = _to_float(row, "throughput_gbps", "gbps")
                if size_bytes > 0 and not _is_nan(gbps):
                    transfer = str(row.get("transfer_kind", "unknown"))
                    memory_mode = str(row.get("memory_mode", "unknown"))
                    copy_method = str(row.get("copy_method", "unknown"))
                    k_bw = (backend, gpu, transfer, memory_mode, copy_method, size_bytes)
                    bw[k_bw].append(gbps)
                    if not _is_nan(energy):
                        bw_energy[k_bw].append(energy)
                    if not _is_nan(power):
                        bw_power[k_bw].append(power)
                    bw_sources[k_bw].add(source)
                    continue

                lat_ns = _to_float(row, "latency_ns")
                if size_bytes > 0 and not _is_nan(lat_ns):
                    k_lat = (backend, gpu, size_bytes)
                    lat[k_lat].append(lat_ns)
                    if not _is_nan(energy):
                        lat_energy[k_lat].append(energy)
                    if not _is_nan(power):
                        lat_power[k_lat].append(power)
                    lat_sources[k_lat].add(source)
                    continue

                n = _to_int(row, "n_elements", "vector_len")
                iters = _to_int(row, "iters_inner", "inner_iters")
                if n < 0 or iters < 0:
                    parse_dropped_rows += 1
                    continue

                if bench_name == "compute_fma_peak":
                    peak = _to_float(row, "gflops_peak")
                    mean = _to_float(row, "gflops_mean", "gflops", "throughput_gflops")
                    sigma = _to_float(row, "gflops_sigma")
                    if not _is_nan(peak) or not _is_nan(mean):
                        if _is_nan(peak):
                            peak = mean
                        if _is_nan(mean):
                            mean = peak
                        if _is_nan(sigma):
                            sigma = 0.0
                        e_mean = _to_float(row, "energy_joule_mean", "energy_joule", "energy_j")
                        p_mean = _to_float(row, "avg_power_watt_mean", "avg_power_watt", "avg_power_w")
                        peak_agg[(backend, gpu, n, iters)] = (peak, mean, sigma, e_mean, p_mean)
                        peak_agg_source[(backend, gpu, n, iters)] = source
                        continue

                    g = _to_float(row, "gflops", "throughput_gflops")
                    if not _is_nan(g):
                        k_peak = (backend, gpu, n, iters)
                        peak_runs[k_peak].append(g)
                        if not _is_nan(energy):
                            peak_runs_energy[k_peak].append(energy)
                        if not _is_nan(power):
                            peak_runs_power[k_peak].append(power)
                        peak_runs_sources[k_peak].add(source)
                    else:
                        parse_dropped_rows += 1
                    continue

                if bench_name == "compute_fma":
                    g = _to_float(row, "gflops", "throughput_gflops")
                    if _is_nan(g):
                        parse_dropped_rows += 1
                        continue
                    k_fma = (backend, gpu, n, iters)
                    fma[k_fma].append(g)
                    if not _is_nan(energy):
                        fma_energy[k_fma].append(energy)
                    if not _is_nan(power):
                        fma_power[k_fma].append(power)
                    fma_sources[k_fma].add(source)
                else:
                    parse_dropped_rows += 1

    print("\n=== GPU SUMMARY ===")

    if bw:
        print("\n[Bandwidth] mean ± sigma (GB/s)")
        for key in sorted(bw.keys(), key=lambda k: (k[0], k[1], k[2], k[3], k[4], k[5])):
            backend, gpu, transfer, memory_mode, copy_method, size = key
            mu, sd = _mean_std(bw[key])
            print(
                f"- {backend:7s} | {gpu:35s} | {transfer:16s} | {memory_mode:12s} | "
                f"{copy_method:8s} | {size/1024/1024:6.0f} MB : {mu:8.2f} ± {sd:6.2f}"
            )
            print(f"  {_fmt_energy(bw_energy[key], bw_power[key])}")
            print(f"  source: {', '.join(sorted(bw_sources[key]))}")
    else:
        print("\n[Bandwidth] brak danych")

    if fma:
        print("\n[FMA Compute] mean ± sigma (GFLOP/s)")
        for key in sorted(fma.keys(), key=lambda k: (k[0], k[1], k[2], k[3])):
            backend, gpu, n, iters = key
            mu, sd = _mean_std(fma[key])
            print(
                f"- {backend:7s} | {gpu:35s} | n={n:9d} | iters={iters:7d} : {mu:9.2f} ± {sd:7.2f}"
            )
            print(f"  {_fmt_energy(fma_energy[key], fma_power[key])}")
            print(f"  source: {', '.join(sorted(fma_sources[key]))}")
    else:
        print("\n[FMA Compute] brak danych")

    if peak_agg or peak_runs:
        print("\n[FMA Peak] peak / mean ± sigma (GFLOP/s)")
        all_peak_keys = sorted(
            set(peak_agg.keys()) | set(peak_runs.keys()),
            key=lambda k: (k[0], k[1], k[2], k[3]),
        )
        for key in all_peak_keys:
            backend, gpu, n, iters = key
            if key in peak_agg:
                peak, mean, sigma, e_mean, p_mean = peak_agg[key]
                print(
                    f"- {backend:7s} | {gpu:35s} | n={n:9d} | iters={iters:7d} : "
                    f"peak={peak:9.2f}, mean={mean:9.2f} ± {sigma:7.2f}"
                )
                e_vals = [] if _is_nan(e_mean) else [e_mean]
                p_vals = [] if _is_nan(p_mean) else [p_mean]
                print(f"  {_fmt_energy(e_vals, p_vals)}")
                print(f"  source: {peak_agg_source.get(key, 'unknown')}")
                continue

            vals = peak_runs[key]
            peak_val = max(vals) if vals else float("nan")
            mean, sigma = _mean_std(vals)
            print(
                f"- {backend:7s} | {gpu:35s} | n={n:9d} | iters={iters:7d} : "
                f"peak={peak_val:9.2f}, mean={mean:9.2f} ± {sigma:7.2f}"
            )
            print(f"  {_fmt_energy(peak_runs_energy[key], peak_runs_power[key])}")
            print(f"  source: {', '.join(sorted(peak_runs_sources[key]))}")
    else:
        print("\n[FMA Peak] brak danych")

    if lat:
        print("\n[Pointer Latency] mean ± sigma (ns)")
        for key in sorted(lat.keys(), key=lambda k: (k[0], k[1], k[2])):
            backend, gpu, size = key
            mu, sd = _mean_std(lat[key])
            print(
                f"- {backend:7s} | {gpu:35s} | {size/1024:7.0f} KB : "
                f"{mu:9.3f} ± {sd:7.3f}"
            )
            print(f"  {_fmt_energy(lat_energy[key], lat_power[key])}")
            print(f"  source: {', '.join(sorted(lat_sources[key]))}")
    else:
        print("\n[Pointer Latency] brak danych")

    print("\n[Data Quality]")
    print(f"- files: {len(files)}")
    print(f"- rows_total: {total_rows}")
    print(f"- rows_bad_shape: {shape_bad_rows}")
    print(f"- rows_dropped_parse: {parse_dropped_rows}")

    issue_count = shape_bad_rows + parse_dropped_rows
    if strict and issue_count > 0:
        print(f"[STRICT] FAIL: wykryto {issue_count} problematycznych rekordów.")
        return 2
    if strict:
        print("[STRICT] PASS: brak problematycznych rekordów.")
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="GPU summary: mean +/- sigma from benchmark CSVs",
    )
    parser.add_argument(
        "--mode",
        choices=["latest", "all"],
        default="latest",
        help="latest: only newest CSV per (benchmark, backend, gpu, dev); all: aggregate all history",
    )
    parser.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    parser.add_argument("--session", default="latest")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Zwraca błąd (exit code 2), jeśli wykryte są niespójne/odrzucone rekordy CSV.",
    )
    args = parser.parse_args()
    raise SystemExit(summarize(args.mode, strict=args.strict, scope=args.scope, session=args.session))
