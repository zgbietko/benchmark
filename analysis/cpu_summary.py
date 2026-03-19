# analysis/cpu_summary.py
"""
Reads data/cpu/*.csv and prints simple console summaries.

Important:
- Many benchmarks append to the same CSV over time.
- Default mode selects the newest CSV per (benchmark, arch, cpu_model).
- Within a CSV we still keep only the *latest* session per CPU (based on the 'timestamp' column).
"""

from __future__ import annotations

import csv
import argparse
from collections import defaultdict
import os
from pathlib import Path
from statistics import mean, stdev
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "cpu"
RUNS_DIR = ROOT / "data" / "runs"


def _read_csv_rows(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        for row in r:
            row["_source_file"] = path.name
            rows.append(row)
    return rows


def _infer_benchmark_from_filename(name: str) -> str:
    if name.startswith("bandwidth_mt"):
        return "bandwidth_mt"
    if name.startswith("bandwidth"):
        return "bandwidth"
    if name.startswith("pointer_latency"):
        return "pointer_latency"
    if name.startswith("compute_fma_peak"):
        return "compute_fma_peak"
    if name.startswith("compute_fma"):
        return "compute_fma"
    if name.startswith("stream_mt"):
        return "stream_mt"
    if name.startswith("stream"):
        return "stream"
    return "unknown"


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
        if (p / "cpu").exists():
            return p / "cpu"
    if scope == "session":
        if session and session != "latest":
            return RUNS_DIR / session / "cpu"
        latest = _latest_session_dir()
        if latest is not None:
            return latest / "cpu"
    return DATA_DIR


def _select_files(files: list[Path], mode: str) -> list[Path]:
    if mode == "all":
        return files
    latest: dict[tuple[str, str, str], Path] = {}
    latest_mtime: dict[tuple[str, str, str], float] = {}
    for p in files:
        rows = _read_csv_rows(p)
        if rows:
            r0 = rows[0]
            bench = str(r0.get("benchmark", "")).strip() or _infer_benchmark_from_filename(p.name)
            arch = str(r0.get("arch", "")).strip() or "unknown_arch"
            cpu = str(r0.get("cpu_model", "")).strip() or "unknown_cpu"
        else:
            bench = _infer_benchmark_from_filename(p.name)
            arch = "unknown_arch"
            cpu = "unknown_cpu"
        key = (bench, arch, cpu)
        mtime = p.stat().st_mtime
        if key not in latest_mtime or mtime > latest_mtime[key]:
            latest_mtime[key] = mtime
            latest[key] = p
    return sorted(latest.values())


def _to_float(x: Any) -> float | None:
    try:
        if x is None:
            return None
        s = str(x).strip()
        if s == "" or s.lower() == "nan":
            return None
        return float(s)
    except Exception:
        return None


def _to_int(x: Any) -> int | None:
    try:
        if x is None:
            return None
        s = str(x).strip()
        if s == "" or s.lower() == "nan":
            return None
        return int(float(s))
    except Exception:
        return None


def _latest_session_filter(rows: list[dict[str, Any]], key_fields: list[str]) -> list[dict[str, Any]]:
    """
    Keep only rows from the newest timestamp for each key (e.g., per CPU model).
    Assumes ISO-like timestamp strings so lexicographic max works.
    """
    latest_ts: dict[tuple, str] = {}
    for row in rows:
        key = tuple(row.get(k, "") for k in key_fields)
        ts = str(row.get("timestamp", "")).strip()
        if not ts:
            continue
        if key not in latest_ts or ts > latest_ts[key]:
            latest_ts[key] = ts

    out: list[dict[str, Any]] = []
    for row in rows:
        key = tuple(row.get(k, "") for k in key_fields)
        ts = str(row.get("timestamp", "")).strip()
        if key in latest_ts and ts == latest_ts[key]:
            out.append(row)
    return out


def _group_stats(values: list[float]) -> tuple[float, float]:
    if not values:
        return 0.0, 0.0
    if len(values) == 1:
        return values[0], 0.0
    return mean(values), stdev(values)


def _print_energy_line(energy_vals: list[float], power_vals: list[float]) -> None:
    e_mean, e_std = _group_stats(energy_vals)
    p_mean, p_std = _group_stats(power_vals)
    print(f"             energia: {e_mean:7.4f} J ± {e_std:7.4f}, P_avg: {p_mean:6.2f} W ± {p_std:6.2f}")


def _summarize_bandwidth(rows: list[dict[str, Any]], title: str, key_extra: list[str] | None = None) -> None:
    print(f"\n=== {title} ===")
    if key_extra is None:
        key_extra = []

    # Filter newest session per CPU
    rows = _latest_session_filter(rows, key_fields=["arch", "cpu_model"] + key_extra)

    # group: (arch, cpu_model, size_mb, threads?)
    groups: dict[tuple, list[dict[str, Any]]] = defaultdict(list)
    for r in rows:
        arch = r.get("arch", "?")
        cpu = r.get("cpu_model", "?")
        size_mb = _to_int(r.get("size_mb"))
        if size_mb is None:
            continue
        key = (arch, cpu, size_mb)
        for k in key_extra:
            key += (r.get(k, ""),)
        groups[key].append(r)

    # print sorted
    for key in sorted(groups.keys(), key=lambda x: (x[0], x[1], int(x[2]))):
        arch, cpu, size_mb, *rest = key
        vals = []
        e_vals = []
        p_vals = []
        for r in groups[key]:
            gbps = _to_float(r.get("gbps"))
            if gbps is not None:
                vals.append(gbps)
            e = _to_float(r.get("energy_j"))
            p = _to_float(r.get("avg_power_w"))
            if e is not None:
                e_vals.append(e)
            if p is not None:
                p_vals.append(p)
        m, s = _group_stats(vals)
        if rest:
            # MT case: expect first extra is 'threads'
            th = rest[0]
            print(f"{arch:8s} | {cpu:30s} | {size_mb:7d} MB | {str(th):>3s} th | {m:7.2f} GB/s ± {s:5.2f}")
        else:
            print(f"{arch:8s} | {cpu:30s} | {size_mb:7d} MB | {m:7.2f} GB/s ± {s:5.2f}")
        _print_energy_line(e_vals, p_vals)


def _summarize_pointer_latency(rows: list[dict[str, Any]]) -> None:
    print("\n=== Latencja pointer-chasing (pointer_latency_*.csv) ===")
    rows = _latest_session_filter(rows, key_fields=["arch", "cpu_model"])

    groups: dict[tuple, list[dict[str, Any]]] = defaultdict(list)
    for r in rows:
        arch = r.get("arch", "?")
        cpu = r.get("cpu_model", "?")
        ws_kb = _to_int(r.get("working_set_kb"))
        if ws_kb is None:
            continue
        groups[(arch, cpu, ws_kb)].append(r)

    for (arch, cpu, ws_kb) in sorted(groups.keys(), key=lambda x: (x[0], x[1], int(x[2]))):
        vals = []
        e_vals = []
        p_vals = []
        for r in groups[(arch, cpu, ws_kb)]:
            v = _to_float(r.get("latency_ns"))
            if v is not None:
                vals.append(v)
            e = _to_float(r.get("energy_j"))
            p = _to_float(r.get("avg_power_w"))
            if e is not None:
                e_vals.append(e)
            if p is not None:
                p_vals.append(p)
        m, s = _group_stats(vals)
        print(f"{arch:8s} | {cpu:30s} | {ws_kb:7d} KB | {m:7.2f} ns ± {s:6.2f}")
        _print_energy_line(e_vals, p_vals)


def _summarize_compute_fma(rows: list[dict[str, Any]]) -> None:
    print("\n=== FMA compute throughput (compute_fma_*.csv) ===")
    rows = _latest_session_filter(rows, key_fields=["arch", "cpu_model"])
    groups: dict[tuple, list[dict[str, Any]]] = defaultdict(list)
    for r in rows:
        arch = r.get("arch", "?")
        cpu = r.get("cpu_model", "?")
        n = _to_int(r.get("vector_len"))
        iters = _to_int(r.get("iters_inner"))
        if n is None or iters is None:
            continue
        groups[(arch, cpu, n, iters)].append(r)

    for (arch, cpu, n, iters) in sorted(groups.keys(), key=lambda x: (x[0], x[1], int(x[2]), int(x[3]))):
        vals = []
        e_vals = []
        p_vals = []
        for r in groups[(arch, cpu, n, iters)]:
            v = _to_float(r.get("gflops"))
            if v is not None:
                vals.append(v)
            e = _to_float(r.get("energy_j"))
            p = _to_float(r.get("avg_power_w"))
            if e is not None:
                e_vals.append(e)
            if p is not None:
                p_vals.append(p)
        m, s = _group_stats(vals)
        print(f"{arch:8s} | {cpu:30s} | n={n:6d} | iters={iters:8d} | {m:7.2f} GF/s ± {s:5.2f}")
        _print_energy_line(e_vals, p_vals)


def _summarize_compute_fma_peak(rows: list[dict[str, Any]]) -> None:
    print("\n=== Peak FMA throughput (compute_fma_peak_*.csv) ===")
    rows = _latest_session_filter(rows, key_fields=["arch", "cpu_model"])
    # show peak per thread count
    best: dict[tuple, tuple[float, dict[str, Any]]] = {}
    for r in rows:
        arch = r.get("arch", "?")
        cpu = r.get("cpu_model", "?")
        th = _to_int(r.get("threads") or r.get("num_threads"))
        g = _to_float(r.get("gflops"))
        if th is None or g is None:
            continue
        key = (arch, cpu, th)
        if key not in best or g > best[key][0]:
            best[key] = (g, r)

    for (arch, cpu, th) in sorted(best.keys(), key=lambda x: (x[0], x[1], int(x[2]))):
        g, r = best[(arch, cpu, th)]
        iters = _to_int(r.get("iters_inner")) or 0
        n_thr = _to_int(r.get("n_per_thread")) or 0
        e = _to_float(r.get("energy_j"))
        p = _to_float(r.get("avg_power_w"))
        print(f"{arch:8s} | {cpu:30s} | n_thr={n_thr:4d} | th={th:2d} | peak={g:7.2f} GF/s @ iters={iters:8d}")
        _print_energy_line([e] if e is not None else [], [p] if p is not None else [])


def _summarize_stream(rows: list[dict[str, Any]], title: str, mt: bool) -> None:
    print(f"\n=== {title} ===")
    key_fields = ["arch", "cpu_model"]
    if mt:
        key_fields.append("threads")
    rows = _latest_session_filter(rows, key_fields=key_fields)

    groups: dict[tuple, list[dict[str, Any]]] = defaultdict(list)
    for r in rows:
        arch = r.get("arch", "?")
        cpu = r.get("cpu_model", "?")
        kernel = r.get("kernel", "?")
        size_mb = _to_int(r.get("size_mb"))
        if size_mb is None:
            continue
        if mt:
            th = r.get("threads", "?")
            groups[(arch, cpu, kernel, size_mb, th)].append(r)
        else:
            groups[(arch, cpu, kernel, size_mb)].append(r)

    for key in sorted(groups.keys(), key=lambda x: (x[0], x[1], x[2], int(x[3]), str(x[4]) if mt else "")):
        vals = []
        e_vals = []
        p_vals = []
        for r in groups[key]:
            v = _to_float(r.get("gbps"))
            if v is not None:
                vals.append(v)
            e = _to_float(r.get("energy_j"))
            p = _to_float(r.get("avg_power_w"))
            if e is not None:
                e_vals.append(e)
            if p is not None:
                p_vals.append(p)
        m, s = _group_stats(vals)
        if mt:
            arch, cpu, kernel, size_mb, th = key
            print(f"{arch:8s} | {cpu:30s} | {kernel:5s} | {size_mb:7d} MB | {str(th):>3s} th | {m:7.2f} GB/s ± {s:5.2f}")
        else:
            arch, cpu, kernel, size_mb = key
            print(f"{arch:8s} | {cpu:30s} | {kernel:5s} | {size_mb:7d} MB | {m:7.2f} GB/s ± {s:5.2f}")
        _print_energy_line(e_vals, p_vals)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="CPU summary: mean +/- sigma from benchmark CSVs",
    )
    parser.add_argument(
        "--mode",
        choices=["latest", "all"],
        default="latest",
        help="latest: only newest CSV per (benchmark, arch, cpu_model); all: aggregate all history",
    )
    parser.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    parser.add_argument("--session", default="latest")
    args = parser.parse_args()

    scope = args.scope
    if scope == "auto":
        scope = "session" if os.environ.get("BENCH_RUN_DIR", "").strip() else "global"
    data_dir = _data_dir(scope, args.session)
    print(f"[INFO] CPU summary source: {data_dir}")

    if not data_dir.exists():
        print(f"[WARN] Brak katalogu: {data_dir}")
        return

    # gather all files
    files = list(data_dir.glob("*.csv"))
    if not files:
        print(f"[WARN] Brak plików CSV w: {data_dir}")
        return
    files = _select_files(files, args.mode)

    all_rows = []
    for p in files:
        all_rows.extend(_read_csv_rows(p))

    # split by benchmark (field), fallback by filename
    by_bench: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for r in all_rows:
        bench = str(r.get("benchmark", "")).strip()
        if not bench:
            fn = str(r.get("_source_file", ""))
            bench = _infer_benchmark_from_filename(fn)
        by_bench[bench].append(r)

    if "bandwidth" in by_bench:
        _summarize_bandwidth(by_bench["bandwidth"], "Jednowątkowa przepustowość pamięci (bandwidth_*.csv)")
    if "bandwidth_mt" in by_bench:
        _summarize_bandwidth(by_bench["bandwidth_mt"], "Wielowątkowa przepustowość pamięci (bandwidth_mt_*.csv)", key_extra=["threads"])
    if "pointer_latency" in by_bench:
        _summarize_pointer_latency(by_bench["pointer_latency"])
    if "stream" in by_bench:
        _summarize_stream(by_bench["stream"], "STREAM (single-thread)", mt=False)
    if "stream_mt" in by_bench:
        _summarize_stream(by_bench["stream_mt"], "STREAM (multi-thread)", mt=True)
    if "compute_fma" in by_bench:
        _summarize_compute_fma(by_bench["compute_fma"])
    if "compute_fma_peak" in by_bench:
        _summarize_compute_fma_peak(by_bench["compute_fma_peak"])


if __name__ == "__main__":
    main()
