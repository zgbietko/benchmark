from __future__ import annotations

import argparse
import csv
import statistics as stats
from datetime import datetime, timezone
from pathlib import Path
import sys

# ROOT projektu: .../apple_microbench
ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import gpu_utils  # z apple_microbench/gpu_utils.py

try:
    from energy_utils import EnergyLogger  # type: ignore
except Exception:
    EnergyLogger = None  # type: ignore

from gpu.metal.metal_backend import MetalBackend  # type: ignore


def run_fma_bench(
    device_index: int,
    vector_len: int,
    runs_per_config: int,
    inner_iters_list: list[int],
) -> None:
    backend = MetalBackend(device_index=device_index)
    gpu_name = backend.device_name

    print("=== GPU FMA compute benchmark (Metal, fma_kernel) ===")
    print(f"GPU device      : {gpu_name} (index {device_index})")
    print(f"vector_len      : {vector_len}")
    print(f"runs per config : {runs_per_config}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_compute_fma",
        data_dir=data_dir,
        gpu_backend="metal",
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
        "vector_len",
        "inner_iters",
        "run_idx",
        "elapsed_s",
        "throughput_gflops",
        "gflops",
        "n_elements",
        "iters_inner",
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
    if logger is not None:
        print(
            f"Energy          : source={logger.energy_source}, "
            f"available={'yes' if logger.energy_available else 'no'}"
        )
    else:
        print("Energy          : unavailable (energy_utils import failed)")
    print()

    for inner_iters in inner_iters_list:
        print(f"--- inner_iters = {inner_iters} ---")

        times: list[float] = []
        energies: list[float] = []
        powers: list[float] = []

        for run_idx in range(runs_per_config):
            energy_j = 0.0
            avg_power_w = 0.0

            if logger is not None:
                logger.start()

            elapsed = backend.run_fma(vector_len, inner_iters)

            if logger is not None:
                try:
                    energy_j, avg_power_w = logger.stop()
                except Exception:
                    energy_j, avg_power_w = 0.0, 0.0
            q = logger.last_quality_metrics if logger is not None else {}

            # Każda operacja fma to 2 FLOP (mnożenie + dodawanie)
            flops = 2.0 * float(vector_len) * float(inner_iters)
            gflops = flops / elapsed / 1e9

            times.append(elapsed)
            energies.append(energy_j)
            powers.append(avg_power_w)

            print(
                f"run {run_idx:2d}: elapsed = {elapsed:8.4f} s, "
                f"GFLOP/s = {gflops:7.2f}, energy = {energy_j:7.4f} J, "
                f"P_avg = {avg_power_w:7.2f} W"
            )

            row = gpu_utils.common_gpu_metadata("metal", gpu_name, device_index)
            row.update(
                {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "vector_len": str(vector_len),
                    "inner_iters": str(inner_iters),
                    "run_idx": str(run_idx),
                    "elapsed_s": f"{elapsed:.6f}",
                    "throughput_gflops": f"{gflops:.4f}",
                    "gflops": f"{gflops:.4f}",
                    "n_elements": str(vector_len),
                    "iters_inner": str(inner_iters),
                    "energy_joule": (
                        f"{energy_j:.6f}" if energy_j == energy_j else ""
                    ),
                    "energy_j": (
                        f"{energy_j:.6f}" if energy_j == energy_j else ""
                    ),
                    "avg_power_watt": (
                        f"{avg_power_w:.6f}" if avg_power_w == avg_power_w else ""
                    ),
                    "avg_power_w": (
                        f"{avg_power_w:.6f}" if avg_power_w == avg_power_w else ""
                    ),
                    "energy_source": (logger.energy_source if logger is not None else "unavailable"),
                    "sample_interval_s": (
                        f"{logger.sample_interval_s:.6f}" if logger is not None else ""
                    ),
                    "energy_supported": (
                        "1" if (logger is not None and logger.energy_available) else "0"
                    ),
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

        mean_gflops = stats.mean(
            (2.0 * float(vector_len) * float(inner_iters)) / t / 1e9 for t in times
        )
        std_gflops = stats.pstdev(
            (2.0 * float(vector_len) * float(inner_iters)) / t / 1e9 for t in times
        )

        print(
            f"==> MEAN: {mean_gflops:7.2f} GFLOP/s, "
            f"sigma = {std_gflops:7.2f} GFLOP/s"
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
        description="GPU FMA compute benchmark (Metal)."
    )
    parser.add_argument(
        "--device-index",
        type=int,
        default=0,
        help="Indeks urządzenia Metal (jeśli jest ich więcej niż jedno).",
    )
    parser.add_argument(
        "--vector-len",
        type=int,
        default=1 << 20,  # 1M elementów
        help="Długość wektora (liczba elementów float32).",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=5,
        help="Liczba powtórzeń dla każdej konfiguracji.",
    )
    parser.add_argument(
        "--inner-iters",
        type=int,
        nargs="*",
        default=[1000, 5000, 10000],
        help="Lista wartości inner_iters (domyślnie: 1000 5000 10000).",
    )

    args = parser.parse_args()
    run_fma_bench(
        device_index=args.device_index,
        vector_len=args.vector_len,
        runs_per_config=args.runs,
        inner_iters_list=args.inner_iters,
    )


if __name__ == "__main__":
    main()
