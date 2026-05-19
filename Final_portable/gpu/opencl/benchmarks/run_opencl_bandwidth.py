from __future__ import annotations

import argparse
import csv
import statistics as stats
from datetime import datetime, timezone
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import gpu_utils  # type: ignore

try:
    from energy_utils import EnergyLogger  # type: ignore
except Exception:
    EnergyLogger = None  # type: ignore

from gpu.opencl.opencl_backend import (
    init_opencl,
    opencl_mem_copy,
    opencl_memcpy_h2d_bandwidth,
    opencl_memcpy_d2h_bandwidth,
    get_device_name,
)


def run_bandwidth_bench(device_index: int, runs_per_size: int, sizes_mb: list[int], iters_per_run: int) -> None:
    ocl = init_opencl(device_index)
    gpu_name = get_device_name(device_index)

    transfer_kinds = [
        ("device_to_device", "device", "kernel", None),
        ("host_to_device", "host", "memcpy", opencl_memcpy_h2d_bandwidth),
        ("device_to_host", "host", "memcpy", opencl_memcpy_d2h_bandwidth),
    ]
    print("=== GPU memory bandwidth benchmark (OpenCL) ===")
    print(f"GPU device   : {gpu_name} (index {device_index})")
    print(f"runs per size: {runs_per_size}")
    print(f"iters per run: {iters_per_run}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_bandwidth",
        data_dir=data_dir,
        gpu_backend="opencl",
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

    for size_mb in sizes_mb:
        size_bytes = size_mb * 1024 * 1024
        num_elements = size_bytes // 4

        for transfer_kind, memory_mode, copy_method, fn in transfer_kinds:
            print(f"--- Transfer: {transfer_kind} ---")
            print(f"--- Size: {size_mb:5d} MB ({size_bytes} bytes, {num_elements} elements) ---")

            times = []
            gbps_values = []
            energies = []
            powers = []

            for run_idx in range(runs_per_size):
                energy_j = float("nan")
                avg_power_w = float("nan")

                if logger is not None:
                    logger.start()

                if copy_method == "kernel":
                    elapsed = opencl_mem_copy(ocl, num_elements)
                    iters_inner = 1
                else:
                    elapsed = fn(ocl, size_bytes, iters_per_run)
                    iters_inner = iters_per_run

                if logger is not None:
                    try:
                        energy_j, avg_power_w = logger.stop()
                    except Exception:
                        energy_j, avg_power_w = float("nan"), float("nan")
                q = logger.last_quality_metrics if logger is not None else {}

                bytes_total = (2.0 * size_bytes) if copy_method == "kernel" else (size_bytes * iters_per_run)
                gbps = bytes_total / elapsed / 1e9

                times.append(elapsed)
                gbps_values.append(gbps)
                energies.append(energy_j)
                powers.append(avg_power_w)

                print(
                    f"run {run_idx:2d}: elapsed = {elapsed:8.4f} s, "
                    f"GB/s = {gbps:7.2f}, energy = {energy_j:7.4f} J, "
                    f"P_avg = {avg_power_w:7.2f} W"
                )

                row = gpu_utils.common_system_metadata("opencl")
                row.update(
                    {
                        "timestamp": datetime.now(timezone.utc).isoformat(),
                        "gpu_model": gpu_name,
                        "gpu_index": device_index,
                        "transfer_kind": transfer_kind,
                        "memory_mode": memory_mode,
                        "copy_method": copy_method,
                        "size_bytes": str(size_bytes),
                        "num_elements": str(num_elements),
                        "iters_inner": str(iters_inner),
                        "run_idx": str(run_idx),
                        "elapsed_s": f"{elapsed:.6f}",
                        "throughput_gbps": f"{gbps:.4f}",
                        "gbps": f"{gbps:.4f}",
                        "energy_joule": (f"{energy_j:.6f}" if energy_j == energy_j else ""),
                        "energy_j": (f"{energy_j:.6f}" if energy_j == energy_j else ""),
                        "avg_power_watt": (f"{avg_power_w:.6f}" if avg_power_w == avg_power_w else ""),
                        "avg_power_w": (f"{avg_power_w:.6f}" if avg_power_w == avg_power_w else ""),
                        "energy_source": (logger.energy_source if logger is not None else "unavailable"),
                        "sample_interval_s": (logger.sample_interval_s if logger is not None else float("nan")),
                        "energy_supported": (1 if (logger is not None and logger.energy_available) else 0),
                        "energy_samples": int(q.get("sample_count", 0) or 0),
                        "energy_nan_samples": int(q.get("nan_sample_count", 0) or 0),
                        "energy_confidence": float(q.get("confidence", 0.0) or 0.0),
                    }
                )

                with csv_path.open("a", newline="") as f:
                    writer = csv.DictWriter(f, fieldnames=fieldnames)
                    if not header_written:
                        writer.writeheader()
                        header_written = True
                    writer.writerow(row)

            mean_gbps = stats.mean(gbps_values) if gbps_values else float("nan")
            std_gbps = stats.pstdev(gbps_values) if len(gbps_values) > 1 else 0.0

            print(
                f"==> MEAN: {mean_gbps:7.2f} GB/s, "
                f"sigma = {std_gbps:7.2f} GB/s"
            )
            if energies and any(e == e for e in energies):
                valid_energies = [e for e in energies if e == e]
                valid_powers = [p for p in powers if p == p]
                e_mean = stats.mean(valid_energies)
                e_std = stats.pstdev(valid_energies) if len(valid_energies) > 1 else 0.0
                p_mean = stats.mean(valid_powers)
                p_std = stats.pstdev(valid_powers) if len(valid_powers) > 1 else 0.0
                print(
                    f"    energy: {e_mean:7.4f} J ± {e_std:7.4f} J, "
                    f"P_avg: {p_mean:7.2f} W ± {p_std:7.2f} W"
                )

            print()

    print(f"Wszystkie runy zapisane do: {csv_path}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="GPU memory bandwidth benchmark (OpenCL)."
    )
    parser.add_argument("--device-index", type=int, default=0)
    parser.add_argument("--runs", type=int, default=7)
    parser.add_argument("--iters", type=int, default=50)
    parser.add_argument(
        "--sizes-mb",
        type=int,
        nargs="*",
        default=[4, 16, 64, 256, 1024],
    )

    args = parser.parse_args()
    run_bandwidth_bench(
        device_index=args.device_index,
        runs_per_size=args.runs,
        sizes_mb=args.sizes_mb,
        iters_per_run=args.iters,
    )


if __name__ == "__main__":
    main()
