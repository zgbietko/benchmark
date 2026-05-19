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

from gpu.cuda.cuda_backend import init_cuda, get_device_info, cuda_pointer_chase_latency


def _system_metadata() -> Dict[str, Any]:
    return {
        "backend": "cuda",
        "system": platform.system(),
        "arch": platform.machine(),
        "hostname": socket.gethostname(),
        "python_version": platform.python_version(),
    }


def run_pointer_latency(
    device_index: int,
    sizes_kb: List[int],
    runs_per_size: int,
    iters_inner: int,
) -> None:
    ctx = init_cuda(device_index)
    if not hasattr(ctx.lib, "gpu_cuda_pointer_chase_latency"):
        print("[WARN] gpu_cuda_pointer_chase_latency missing in CUDA library; rebuild gpu/cuda/lib and rerun.")
        return
    info = get_device_info(ctx)
    gpu_name = info.name

    print("=== GPU pointer-chasing latency benchmark (CUDA) ===")
    print(f"GPU device   : {gpu_name} (index {device_index})")
    print(f"runs per size: {runs_per_size}")
    print(f"iters inner  : {iters_inner}")
    print()

    data_dir = ROOT / "data" / "gpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path = gpu_utils.make_gpu_specific_csv_path(
        benchmark_name="gpu_pointer_latency",
        data_dir=data_dir,
        gpu_backend="cuda",
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
        "size_bytes",
        "n_elements",
        "iters_inner",
        "run_idx",
        "elapsed_s",
        "latency_ns",
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

        for kb in sizes_kb:
            size_bytes = max(4, kb * 1024)
            n = max(1, size_bytes // 4)
            latencies_ns: List[float] = []
            print(f"--- size: {kb:6d} KB ({n} elements) ---")
            for run_idx in range(runs_per_size):
                energy_j = float("nan")
                power_w = float("nan")
                if logger is not None:
                    logger.start()

                elapsed_s = cuda_pointer_chase_latency(ctx, n=n, iters=iters_inner)

                if logger is not None:
                    try:
                        energy_j, power_w = logger.stop()
                    except RuntimeError:
                        energy_j, power_w = float("nan"), float("nan")
                q = logger.last_quality_metrics if logger is not None else {}

                lat_ns = (elapsed_s * 1e9) / max(iters_inner, 1)
                latencies_ns.append(lat_ns)
                print(
                    f"run {run_idx:2d}: elapsed={elapsed_s:8.6f}s, "
                    f"lat={lat_ns:8.3f} ns, energy={energy_j:7.3f} J"
                )

                row = {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    **_system_metadata(),
                    "gpu_model": gpu_name,
                    "gpu_index": device_index,
                    "size_bytes": int(size_bytes),
                    "n_elements": int(n),
                    "iters_inner": int(iters_inner),
                    "run_idx": run_idx,
                    "elapsed_s": elapsed_s,
                    "latency_ns": lat_ns,
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

            mu = stats.mean(latencies_ns)
            sd = stats.pstdev(latencies_ns) if len(latencies_ns) > 1 else 0.0
            print(f"==> mean latency: {mu:8.3f} ± {sd:8.3f} ns\n")

    print(f"Wszystkie runy zapisane do: {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description="GPU pointer-chasing latency benchmark (CUDA).")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--sizes-kb", type=str, default="4,16,64,256,1024,4096,16384,65536")
    ap.add_argument("--runs", type=int, default=7)
    ap.add_argument("--iters-inner", type=int, default=2_000_000)
    args = ap.parse_args()

    run_pointer_latency(
        device_index=args.device_index,
        sizes_kb=[int(x) for x in args.sizes_kb.split(",") if x.strip()],
        runs_per_size=args.runs,
        iters_inner=args.iters_inner,
    )


if __name__ == "__main__":
    main()
