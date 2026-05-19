#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Tuple


ROOT = Path(__file__).resolve().parents[1]


@dataclass
class Stats:
    files: int = 0
    rows: int = 0
    bad_shape: int = 0
    missing_key_fields: int = 0
    missing_metric: int = 0
    energy_supported_rows: int = 0
    energy_low_conf_rows: int = 0
    energy_zero_samples_rows: int = 0


def _to_float(value: object) -> float | None:
    if value is None:
        return None
    s = str(value).strip()
    if s == "":
        return None
    try:
        return float(s)
    except ValueError:
        return None


def _to_int(value: object) -> int | None:
    x = _to_float(value)
    if x is None:
        return None
    try:
        return int(x)
    except Exception:
        return None


def _latest_session_dir() -> Path | None:
    runs_root = ROOT / "data" / "runs"
    latest_link = runs_root / "latest"
    if latest_link.exists() or latest_link.is_symlink():
        try:
            p = latest_link.resolve()
            if p.exists():
                return p
        except Exception:
            pass
    latest_txt = runs_root / "latest.txt"
    if latest_txt.exists():
        try:
            name = latest_txt.read_text(encoding="utf-8").strip()
            if name:
                p = runs_root / name
                if p.exists():
                    return p
        except Exception:
            pass
    return None


def _resolve_data_dirs(scope: str, session: Path | None) -> Tuple[Path, Path, Path, str]:
    if scope == "global":
        return (ROOT / "data" / "cpu", ROOT / "data" / "gpu", ROOT / "data" / "real_kernels", "global")

    if scope == "session":
        base = session
        if base is None:
            env_run_dir = os.environ.get("BENCH_RUN_DIR", "").strip()
            base = Path(env_run_dir) if env_run_dir else _latest_session_dir()
        if base is None:
            return (ROOT / "data" / "cpu", ROOT / "data" / "gpu", ROOT / "data" / "real_kernels", "global-fallback")
        return (base / "cpu", base / "gpu", base / "real_kernels", f"session:{base}")

    # auto
    env_run_dir = os.environ.get("BENCH_RUN_DIR", "").strip()
    if env_run_dir:
        base = Path(env_run_dir)
        return (base / "cpu", base / "gpu", base / "real_kernels", f"session:{base}")
    latest = _latest_session_dir()
    if latest is not None:
        return (latest / "cpu", latest / "gpu", latest / "real_kernels", f"session:{latest}")
    return (ROOT / "data" / "cpu", ROOT / "data" / "gpu", ROOT / "data" / "real_kernels", "global")


def _backend_from_filename(path: Path) -> str:
    m = re.search(r"__backend-([a-zA-Z0-9_]+?)(?:__|$)", path.stem)
    if m:
        return m.group(1).lower()
    return "unknown"


def _count_shape_issues(path: Path) -> Tuple[int, int]:
    try:
        with path.open("r", encoding="utf-8", newline="") as f:
            rows = list(csv.reader(f))
    except Exception:
        return 0, 1
    if not rows:
        return 0, 0
    hdr_len = len(rows[0])
    bad = 0
    for row in rows[1:]:
        if len(row) != hdr_len:
            bad += 1
    return len(rows) - 1, bad


def _check_dir(
    data_dir: Path,
    required_any: Iterable[str],
    metric_any: Iterable[str],
    min_energy_confidence: float,
    per_backend: bool = False,
) -> Tuple[Stats, Dict[str, Stats]]:
    required_any = tuple(required_any)
    metric_any = tuple(metric_any)
    total = Stats()
    backend_stats: Dict[str, Stats] = {}

    if not data_dir.exists():
        return total, backend_stats

    for p in sorted(data_dir.glob("*.csv")):
        total.files += 1
        rows_n, bad_shape = _count_shape_issues(p)
        total.rows += rows_n
        total.bad_shape += bad_shape
        file_backend = _backend_from_filename(p)
        if per_backend:
            backend_stats.setdefault(file_backend, Stats())
            backend_stats[file_backend].files += 1
            backend_stats[file_backend].bad_shape += bad_shape

        try:
            with p.open("r", encoding="utf-8", newline="") as f:
                dr = csv.DictReader(f)
                for row in dr:
                    b = str(row.get("backend", "")).strip().lower() or _backend_from_filename(p)
                    if per_backend:
                        backend_stats.setdefault(b, Stats())
                        backend_stats[b].rows += 1

                    req_ok = any(str(row.get(k, "")).strip() for k in required_any)
                    if not req_ok:
                        total.missing_key_fields += 1
                        if per_backend:
                            backend_stats[b].missing_key_fields += 1

                    metric_ok = any(str(row.get(k, "")).strip() for k in metric_any)
                    if not metric_ok:
                        total.missing_metric += 1
                        if per_backend:
                            backend_stats[b].missing_metric += 1

                    energy_supported = _to_int(row.get("energy_supported")) or 0
                    if energy_supported > 0:
                        total.energy_supported_rows += 1
                        if per_backend:
                            backend_stats[b].energy_supported_rows += 1

                        samples = _to_int(row.get("energy_samples"))
                        if samples is None or samples <= 0:
                            total.energy_zero_samples_rows += 1
                            if per_backend:
                                backend_stats[b].energy_zero_samples_rows += 1

                        conf = _to_float(row.get("energy_confidence"))
                        if conf is None or conf < min_energy_confidence:
                            total.energy_low_conf_rows += 1
                            if per_backend:
                                backend_stats[b].energy_low_conf_rows += 1
        except Exception:
            total.bad_shape += 1

    return total, backend_stats


def _print_stats(label: str, st: Stats) -> None:
    print(
        f"[{label}] files={st.files}, rows={st.rows}, bad_shape={st.bad_shape}, "
        f"missing_key={st.missing_key_fields}, missing_metric={st.missing_metric}, "
        f"energy_supported_rows={st.energy_supported_rows}, "
        f"energy_low_conf={st.energy_low_conf_rows}, energy_zero_samples={st.energy_zero_samples_rows}"
    )


def main() -> None:
    ap = argparse.ArgumentParser(description="Data quality checks for CPU/GPU/real_kernels CSV.")
    ap.add_argument("--strict", action="store_true", help="exit 2 if any issue found")
    ap.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    ap.add_argument("--session", type=Path, default=None, help="session dir path (used when --scope session)")
    ap.add_argument("--min-energy-confidence", type=float, default=0.20)
    args = ap.parse_args()

    cpu_dir, gpu_dir, real_dir, source = _resolve_data_dirs(args.scope, args.session)

    cpu, _ = _check_dir(
        cpu_dir,
        required_any=("elapsed_s", "gbps", "gflops", "latency_ns"),
        metric_any=("elapsed_s", "gbps", "gflops", "latency_ns"),
        min_energy_confidence=args.min_energy_confidence,
        per_backend=False,
    )
    gpu, gpu_backends = _check_dir(
        gpu_dir,
        required_any=("elapsed_s", "throughput_gbps", "gflops", "latency_ns", "gflops_peak"),
        metric_any=("throughput_gbps", "gbps", "gflops", "latency_ns", "gflops_peak", "gflops_mean"),
        min_energy_confidence=args.min_energy_confidence,
        per_backend=True,
    )
    real, _ = _check_dir(
        real_dir,
        required_any=("elapsed_s", "throughput_gbps", "gflops", "latency_ns", "status"),
        metric_any=("throughput_gbps", "gflops", "latency_ns", "elapsed_s"),
        min_energy_confidence=args.min_energy_confidence,
        per_backend=False,
    )

    print("=== DATA QUALITY ===")
    print(f"[SOURCE] {source}")
    _print_stats("CPU", cpu)
    _print_stats("GPU", gpu)
    _print_stats("REAL", real)

    if gpu_backends:
        print("\n[GPU per-backend]")
        for b in sorted(gpu_backends.keys()):
            _print_stats(f"GPU/{b}", gpu_backends[b])

    issues = (
        cpu.bad_shape + cpu.missing_key_fields + cpu.missing_metric +
        gpu.bad_shape + gpu.missing_key_fields + gpu.missing_metric + gpu.energy_low_conf_rows + gpu.energy_zero_samples_rows +
        real.bad_shape + real.missing_key_fields + real.missing_metric
    )
    print(f"\n[TOTAL] issues={issues}")

    if args.strict and issues > 0:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
