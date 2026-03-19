from __future__ import annotations

import argparse
import csv
import math
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


def run_peak_fma(
    device_index: int,
    n_elements_list: List[int],
    iters_list: List[int],
    runs_per_point: int,
) -> None:
    ctx = init_hip(device_index)
    info = get_device_info(ctx)
    gpu_name = info.name

    print("=== GPU Peak FMA benchmark (HIP) ===")
    print(f"GPU device   : {gpu_name} (index {device_index})")
    print(f"n_elements   : {n_elements_list}")
    print(f"iters_inner  : {iters_list}")
    print(f"runs/point   : {runs_per_point}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_compute_fma_peak",
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
        "runs_per_config",
        "gflops_peak",
        "gflops_mean",
        "gflops_sigma",
        "gflops",
        "throughput_gflops",
        "energy_joule_mean",
        "energy_joule_sigma",
        "energy_joule",
        "energy_j",
        "avg_power_watt_mean",
        "avg_power_watt_sigma",
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

        for n_elems in n_elements_list:
            for iters_inner in iters_list:
                gvals: List[float] = []
                evals: List[float] = []
                pvals: List[float] = []
                q_samples: List[int] = []
                q_nan_samples: List[int] = []
                q_conf: List[float] = []

                print(f"--- n_elements={n_elems}, iters_inner={iters_inner} ---")
                flops_per_iter = 2.0 * float(n_elems)

                for run_idx in range(runs_per_point):
                    energy_j = float("nan")
                    power_w = float("nan")

                    if logger is not None:
                        logger.start()

                    elapsed_s = hip_fma_throughput(ctx, n_elements=n_elems, iters_inner=iters_inner)

                    if logger is not None:
                        try:
                            energy_j, power_w = logger.stop()
                        except RuntimeError:
                            energy_j, power_w = float("nan"), float("nan")
                    q = logger.last_quality_metrics if logger is not None else {}

                    gflops = (flops_per_iter * iters_inner) / max(elapsed_s, 1e-12) / 1e9
                    gvals.append(gflops)
                    if not math.isnan(energy_j):
                        evals.append(energy_j)
                    if not math.isnan(power_w):
                        pvals.append(power_w)
                    q_samples.append(int(q.get("sample_count", 0) or 0))
                    q_nan_samples.append(int(q.get("nan_sample_count", 0) or 0))
                    q_conf.append(float(q.get("confidence", 0.0) or 0.0))

                    print(
                        f"run {run_idx:2d}: elapsed = {elapsed_s:8.5f} s, "
                        f"GFLOP/s = {gflops:9.2f}, energy = {energy_j:7.3f} J, "
                        f"P_avg = {power_w:7.3f} W"
                    )

                g_peak = max(gvals) if gvals else float("nan")
                g_mean = stats.mean(gvals) if gvals else float("nan")
                g_sigma = stats.pstdev(gvals) if len(gvals) > 1 else 0.0

                e_mean = stats.mean(evals) if evals else float("nan")
                e_sigma = stats.pstdev(evals) if len(evals) > 1 else 0.0
                p_mean = stats.mean(pvals) if pvals else float("nan")
                p_sigma = stats.pstdev(pvals) if len(pvals) > 1 else 0.0

                print(
                    f"  peak = {g_peak:9.2f} GFLOP/s  "
                    f"mean = {g_mean:9.2f} GFLOP/s  sigma={g_sigma:7.2f}"
                )
                print()

                row = {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    **_system_metadata(),
                    "gpu_model": gpu_name,
                    "gpu_index": device_index,
                    "n_elements": n_elems,
                    "iters_inner": iters_inner,
                    "runs_per_config": runs_per_point,
                    "gflops_peak": g_peak,
                    "gflops_mean": g_mean,
                    "gflops_sigma": g_sigma,
                    "gflops": g_mean,
                    "throughput_gflops": g_mean,
                    "energy_joule_mean": e_mean,
                    "energy_joule_sigma": e_sigma,
                    "energy_joule": e_mean,
                    "energy_j": e_mean,
                    "avg_power_watt_mean": p_mean,
                    "avg_power_watt_sigma": p_sigma,
                    "avg_power_watt": p_mean,
                    "avg_power_w": p_mean,
                    "energy_source": (logger.energy_source if logger is not None else "unavailable"),
                    "sample_interval_s": (logger.sample_interval_s if logger is not None else float("nan")),
                    "energy_supported": (1 if (logger is not None and logger.energy_available) else 0),
                    "energy_samples": int(stats.mean(q_samples)) if q_samples else 0,
                    "energy_nan_samples": int(stats.mean(q_nan_samples)) if q_nan_samples else 0,
                    "energy_confidence": float(stats.mean(q_conf)) if q_conf else 0.0,
                }
                writer.writerow(row)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--device-index", type=int, default=0, help="HIP device index")
    ap.add_argument("--runs-per-point", type=int, default=7, help="runs per (n_elements, iters_inner)")
    ap.add_argument(
        "--n-elements",
        type=int,
        nargs="+",
        default=[250_000, 1_000_000, 4_000_000],
        help="list of vector sizes",
    )
    ap.add_argument(
        "--iters-inner",
        type=int,
        nargs="+",
        default=[200, 500, 1000, 2000],
        help="list of inner iters",
    )
    args = ap.parse_args()

    run_peak_fma(
        device_index=args.device_index,
        n_elements_list=args.n_elements,
        iters_list=args.iters_inner,
        runs_per_point=args.runs_per_point,
    )


if __name__ == "__main__":
    main()
