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
    hip_fma_throughput,
)


def _system_metadata() -> Dict[str, Any]:
    return {
        "backend": "hip",
        "system": platform.system(),
        "arch": platform.machine(),
        "hostname": socket.gethostname(),
        "python_version": platform.python_version(),
    }


def run_fma_bench(
    device_index: int,
    n_elements: int,
    iters_inner: int,
    runs: int,
) -> None:
    ctx = init_hip(device_index)
    info = get_device_info(ctx)
    gpu_name = info.name

    print("=== GPU FMA compute benchmark (HIP) ===")
    print(f"GPU device : {gpu_name} (index {device_index})")
    print(f"n_elements : {n_elements}")
    print(f"iters_inner per run: {iters_inner}")
    print(f"runs      : {runs}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_compute_fma",
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
        "n_elements",
        "iters_inner",
        "run_idx",
        "elapsed_s",
        "gflops",
        "throughput_gflops",
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

    flops_per_iter = 2.0 * float(n_elements)  # 1 FMA = 2 FLOP
    gflops_values: List[float] = []

    with csv_path.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not header_written:
            writer.writeheader()

        for run_idx in range(runs):
            energy_j = float("nan")
            power_w = float("nan")

            if logger is not None:
                logger.start()

            elapsed_s = hip_fma_throughput(ctx, n_elements=n_elements, iters_inner=iters_inner)

            if logger is not None:
                try:
                    energy_j, power_w = logger.stop()
                except RuntimeError:
                    energy_j, power_w = float("nan"), float("nan")
            q = logger.last_quality_metrics if logger is not None else {}

            gflops = (flops_per_iter * iters_inner) / max(elapsed_s, 1e-12) / 1e9
            gflops_values.append(gflops)

            print(
                f"run {run_idx:2d}: elapsed = {elapsed_s:8.5f} s, "
                f"GFLOP/s = {gflops:9.2f}, energy = {energy_j:7.3f} J, "
                f"P_avg = {power_w:7.3f} W, src = {(logger.energy_source if logger is not None else 'unavailable')}"
            )

            row = {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                **_system_metadata(),
                "gpu_model": gpu_name,
                "gpu_index": device_index,
                "n_elements": n_elements,
                "iters_inner": iters_inner,
                "run_idx": run_idx,
                "elapsed_s": elapsed_s,
                "gflops": gflops,
                "throughput_gflops": gflops,
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

    mu = stats.mean(gflops_values) if gflops_values else float("nan")
    sigma = stats.pstdev(gflops_values) if len(gflops_values) > 1 else 0.0
    print()
    print(f"GFLOP/s mean = {mu:9.2f}  (sigma={sigma:7.2f})")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--device-index", type=int, default=0, help="HIP device index")
    ap.add_argument("--n-elements", type=int, default=1_000_000, help="number of float32 elements")
    ap.add_argument("--iters-inner", type=int, default=1000, help="inner iterations")
    ap.add_argument("--runs", type=int, default=10, help="number of runs")
    args = ap.parse_args()

    run_fma_bench(
        device_index=args.device_index,
        n_elements=args.n_elements,
        iters_inner=args.iters_inner,
        runs=args.runs,
    )


if __name__ == "__main__":
    main()
