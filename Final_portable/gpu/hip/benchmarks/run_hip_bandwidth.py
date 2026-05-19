from __future__ import annotations

import argparse
import csv
import platform
import socket
import statistics as stats
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import gpu_utils  # type: ignore

try:
    from energy_utils import EnergyLogger  # type: ignore
except Exception:
    EnergyLogger = None  # type: ignore

from gpu.hip.hip_backend import (
    init_hip,
    get_device_info,
    hip_memcpy_bandwidth,
    hip_memcpy_h2d_bandwidth,
    hip_memcpy_d2h_bandwidth,
)


def _system_metadata() -> Dict[str, Any]:
    return {
        "backend": "hip",
        "system": platform.system(),
        "arch": platform.machine(),
        "hostname": socket.gethostname(),
        "python_version": platform.python_version(),
    }


def run_bandwidth_bench(
    device_index: int,
    sizes_mb: List[int],
    iters_per_run: int,
    runs_per_size: int,
) -> None:
    ctx = init_hip(device_index)
    info = get_device_info(ctx)
    gpu_name = info.name

    transfer_kinds = [
        ("device_to_device", "device", "kernel", hip_memcpy_bandwidth),
        ("host_to_device", "pinned_host", "memcpy", hip_memcpy_h2d_bandwidth),
        ("device_to_host", "pinned_host", "memcpy", hip_memcpy_d2h_bandwidth),
    ]
    print("=== GPU memory bandwidth benchmark (HIP) ===")
    print(f"GPU device   : {gpu_name} (index {device_index})")
    print(f"runs per size: {runs_per_size}")
    print(f"iters per run: {iters_per_run}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_bandwidth",
        data_dir=data_dir,
        gpu_backend="hip",
        gpu_name=gpu_name,
        device_id=device_index,
    )
    header_written = csv_path.exists() and csv_path.stat().st_size > 0

    fieldnames = [
        "timestamp",
        "backend",
        "system",
        "arch",
        "hostname",
        "python_version",
        "gpu_model",
        "gpu_index",
        "transfer_kind",
        "memory_mode",
        "copy_method",
        "size_bytes",
        "num_elements",
        "iters_inner",
        "run_idx",
        "elapsed_s",
        "throughput_gbps",
        "gbps",
        "energy_joule",
        "energy_j",
        "avg_power_watt",
        "avg_power_w",
        "energy_source",
        "sample_interval_s",
        "energy_supported",
        "energy_samples",
        "energy_nan_samples",
        "energy_confidence",
    ]

    logger = EnergyLogger(domain="gpu", device_index=device_index) if EnergyLogger is not None else None

    with csv_path.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not header_written:
            writer.writeheader()

        for size_mb in sizes_mb:
            size_bytes = size_mb * 1024 * 1024
            n_elems = size_bytes // 4  # float32

            for transfer_kind, memory_mode, copy_method, fn in transfer_kinds:
                print(f"--- Transfer: {transfer_kind} ---")
                print(f"--- Size: {size_mb:6d} MB ({size_bytes} bytes, {n_elems} elements) ---")
                gbps_values: List[float] = []

                for run_idx in range(runs_per_size):
                    energy_j = float("nan")
                    power_w = float("nan")

                    if logger is not None:
                        logger.start()

                    elapsed_s = fn(ctx, size_bytes=size_bytes, iters=iters_per_run)

                    if logger is not None:
                        try:
                            energy_j, power_w = logger.stop()
                        except RuntimeError:
                            energy_j, power_w = float("nan"), float("nan")
                    q = logger.last_quality_metrics if logger is not None else {}

                    gbps = (size_bytes / 1e9) * iters_per_run / max(elapsed_s, 1e-12)
                    gbps_values.append(gbps)

                    print(
                        f"run {run_idx:2d}: elapsed = {elapsed_s:8.4f} s, "
                        f"GB/s = {gbps:7.2f}, energy = {energy_j:7.3f} J, "
                        f"P_avg = {power_w:7.3f} W, src = {(logger.energy_source if logger is not None else 'unavailable')}"
                    )

                    row = {
                        "timestamp": datetime.now(timezone.utc).isoformat(),
                        **_system_metadata(),
                        "gpu_model": gpu_name,
                        "gpu_index": device_index,
                        "transfer_kind": transfer_kind,
                        "memory_mode": memory_mode,
                        "copy_method": copy_method,
                        "size_bytes": size_bytes,
                        "num_elements": int(n_elems),
                        "iters_inner": iters_per_run,
                        "run_idx": run_idx,
                        "elapsed_s": elapsed_s,
                        "throughput_gbps": gbps,
                        "gbps": gbps,
                        "energy_joule": energy_j,
                        "energy_j": energy_j,
                        "avg_power_watt": power_w,
                        "avg_power_w": power_w,
                        "energy_source": (logger.energy_source if logger is not None else "unavailable"),
                        "sample_interval_s": (logger.sample_interval_s if logger is not None else float("nan")),
                        "energy_supported": (1 if (logger is not None and logger.energy_available) else 0),
                        "energy_samples": int(q.get("sample_count", 0) or 0),
                        "energy_nan_samples": int(q.get("nan_sample_count", 0) or 0),
                        "energy_confidence": float(q.get("confidence", 0.0) or 0.0),
                    }
                    writer.writerow(row)

                mu = stats.mean(gbps_values) if gbps_values else float("nan")
                sigma = stats.pstdev(gbps_values) if len(gbps_values) > 1 else 0.0
                print(f"  mean GB/s = {mu:7.2f}  (sigma={sigma:6.2f})")
                print()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--device-index", type=int, default=0, help="HIP device index")
    ap.add_argument("--runs", type=int, default=7, help="runs per size")
    ap.add_argument("--iters", type=int, default=50, help="iters per run")
    ap.add_argument(
        "--sizes-mb",
        type=int,
        nargs="+",
        default=[4, 16, 64, 256, 1024],
        help="buffer sizes in MB",
    )
    args = ap.parse_args()

    run_bandwidth_bench(
        device_index=args.device_index,
        sizes_mb=args.sizes_mb,
        iters_per_run=args.iters,
        runs_per_size=args.runs,
    )


if __name__ == "__main__":
    main()
