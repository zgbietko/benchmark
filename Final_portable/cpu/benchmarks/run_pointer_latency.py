# cpu/benchmarks/run_pointer_latency.py
import ctypes as ct
import time
import csv
import platform
import subprocess
import argparse
from pathlib import Path
from datetime import datetime
import statistics as stats
import sys
import numpy as np

ROOT = Path(__file__).resolve().parents[2]  # .../apple_microbench
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))


from energy import energy_measurement_supported, read_energy_joules
from cpu_utils import (
    make_cpu_specific_csv_path,
    detect_cpu_cache_hierarchy,
    classify_working_set_residency,
    describe_cpu_cache_hierarchy,
)


def load_library():
    """Ładuje libmicrobench.* z cpu/lib – cross-platform."""
    system = platform.system()
    root = ROOT

    if system == "Darwin":
        lib_name = "libmicrobench.dylib"
    elif system == "Linux":
        lib_name = "libmicrobench.so"
    elif system == "Windows":
        lib_name = "microbench.dll"
    else:
        raise RuntimeError(f"Nieobsługiwany system: {system}")

    lib_path = root / "cpu" / "lib" / lib_name
    if not lib_path.exists():
        raise FileNotFoundError(f"Nie znaleziono biblioteki: {lib_path}")

    return ct.CDLL(str(lib_path)), root


def configure_functions(lib):
    func = lib.pointer_chase_kernel
    func.argtypes = [
        ct.POINTER(ct.c_uint32),
        ct.c_size_t,
        ct.c_size_t,
    ]
    func.restype = None
    return func


def detect_cpu_model() -> str:
    """
    Próbuje wykryć pełny model CPU (Apple M2 Pro, Intel(R) Core..., itd.).
    """
    system = platform.system()

    try:
        if system == "Darwin":
            out = subprocess.check_output(
                ["sysctl", "-n", "machdep.cpu.brand_string"],
                encoding="utf-8",
                stderr=subprocess.DEVNULL,
            ).strip()
            if out:
                return out

        elif system == "Linux":
            cpuinfo = Path("/proc/cpuinfo")
            if cpuinfo.exists():
                for line in cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
                    if "model name" in line:
                        return line.split(":", 1)[1].strip()

        elif system == "Windows":
            return platform.processor() or platform.uname().processor
    except Exception:
        pass

    return platform.processor() or platform.machine()


def bench_pointer_chase(pointer_chase_kernel, working_set_bytes: int, iters_inner: int):
    elem_size = np.dtype(np.uint32).itemsize
    n = working_set_bytes // elem_size
    if n == 0:
        raise ValueError("working_set_bytes za małe")

    indices = np.arange(n, dtype=np.uint32)
    np.random.shuffle(indices)
    idx_arr = np.empty(n, dtype=np.uint32)
    for i in range(n):
        idx_arr[indices[i]] = indices[(i + 1) % n]

    idx_p = idx_arr.ctypes.data_as(ct.POINTER(ct.c_uint32))

    pointer_chase_kernel(idx_p, n, 1)

    energy_j = None
    e_before = None
    if energy_measurement_supported():
        e_before = read_energy_joules()

    t0 = time.perf_counter()
    pointer_chase_kernel(idx_p, n, iters_inner)
    t1 = time.perf_counter()

    if e_before is not None:
        e_after = read_energy_joules()
        if e_after is not None:
            delta = e_after - e_before
            if delta >= 0:
                energy_j = delta

    elapsed = t1 - t0
    total_steps = iters_inner  # dependent steps
    latency_ns = (elapsed / total_steps) * 1e9

    avg_power_w = None
    if energy_j is not None and elapsed > 0:
        avg_power_w = energy_j / elapsed

    return {
        "working_set_bytes": working_set_bytes,
        "working_set_kb": working_set_bytes // 1024,
        "size_bytes": working_set_bytes,
        "n_indices": n,
        "iters_inner": iters_inner,
        "elapsed_s": elapsed,
        "latency_ns": latency_ns,
        "energy_j": energy_j,
        "power_w": avg_power_w,
        "avg_power_w": avg_power_w,
    }


def collect_system_metadata():
    return {
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "system": platform.system(),
        "node": platform.node(),
        "release": platform.release(),
        "version": platform.version(),
        "machine": platform.machine(),
        "arch": platform.machine(),
        "processor": platform.processor(),
        "cpu_model": detect_cpu_model(),
        "python_version": platform.python_version(),
    }


def write_result_to_csv(
    csv_path: Path,
    result: dict,
    meta: dict,
    benchmark_name: str,
    write_header: bool,
):
    row = {
        **meta,
        "benchmark": benchmark_name,
        **result,
    }

    fieldnames = [
        "timestamp",
        "system",
        "node",
        "release",
        "version",
        "machine",
        "arch",
        "processor",
        "cpu_model",
        "python_version",
        "benchmark",
        "run_id",
        "working_set_bytes",
        "working_set_kb",
        "size_bytes",
        "n_indices",
        "estimated_residency",
        "l1d_bytes",
        "l2_bytes",
        "l3_bytes",
        "eff_l1d_bytes",
        "perf_l1d_bytes",
        "eff_l2_bytes",
        "perf_l2_bytes",
        "eff_l1i_bytes",
        "perf_l1i_bytes",
        "cache_model",
        "cache_source",
        "iters_inner",
        "elapsed_s",
        "latency_ns",
        "energy_j",
        "power_w",
        "avg_power_w",
    ]

    with csv_path.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        writer.writerow(row)


def _default_sizes_kb(cache_info: dict[str, object]) -> list[int]:
    sizes: set[int] = set()
    for exponent in range(2, 18):  # 4 KB .. 128 MB
        sizes.add(1 << exponent)

    for field in (
        "l1d_bytes",
        "l2_bytes",
        "l3_bytes",
        "eff_l1d_bytes",
        "perf_l1d_bytes",
        "eff_l2_bytes",
        "perf_l2_bytes",
    ):
        value = cache_info.get(field)
        if not isinstance(value, int) or value <= 0:
            continue
        value_kb = max(1, value // 1024)
        for factor in (0.5, 0.75, 1.0, 1.25, 1.5, 2.0):
            sizes.add(max(1, int(round(value_kb * factor))))

    return sorted(sizes)


def _build_arg_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        description="CPU pointer-chasing latency benchmark with cache-hierarchy annotations."
    )
    ap.add_argument("--iters-inner", type=int, default=200_000)
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument(
        "--sizes-kb",
        default="",
        help="Lista rozmiarów working setu w KB, oddzielona przecinkami. Domyślnie: sweep log2 + okolice progów cache.",
    )
    return ap


def main():
    args = _build_arg_parser().parse_args()
    lib, root = load_library()
    pointer_chase_kernel = configure_functions(lib)

    data_dir = root / "data" / "cpu"
    data_dir.mkdir(parents=True, exist_ok=True)

    csv_path, arch, cpu_model, cpu_slug = make_cpu_specific_csv_path(
        "pointer_latency",
        data_dir,
    )
    if csv_path.exists():
        csv_path.unlink()  # nadpisujemy stare wyniki

    meta = collect_system_metadata()
    meta["arch"] = arch
    meta["cpu_model"] = cpu_model
    cache_info = detect_cpu_cache_hierarchy()

    if args.sizes_kb.strip():
        sizes_kb = sorted(
            {
                max(1, int(part.strip()))
                for part in args.sizes_kb.split(",")
                if part.strip()
            }
        )
    else:
        sizes_kb = _default_sizes_kb(cache_info)
    iters_inner = max(1, int(args.iters_inner))
    runs = max(1, int(args.runs))

    print("=== CPU pointer-chasing latency benchmark ===")
    print(f"iters_inner     : {iters_inner}")
    print(f"runs per size   : {runs}")
    print(f"CPU model       : {meta['cpu_model']}")
    print(f"Cache hierarchy : {describe_cpu_cache_hierarchy(cache_info)}")
    if energy_measurement_supported():
        print("Energy          : best-effort per-run energy_j / avg_power_w")
    else:
        print("Energy          : pomiar niedostępny na tej platformie")

    header_written = False

    for size_kb in sizes_kb:
        working_set_bytes = size_kb * 1024
        residency = classify_working_set_residency(working_set_bytes, cache_info)
        latency_values = []
        energy_values = []

        print(f"\n--- Working set: {size_kb} KB ({residency}) ---")

        for run_id in range(runs):
            result = bench_pointer_chase(pointer_chase_kernel, working_set_bytes, iters_inner)
            latency_values.append(result["latency_ns"])
            if result["energy_j"] is not None:
                energy_values.append(result["energy_j"])

            write_result_to_csv(
                csv_path,
                {
                    **result,
                    "run_id": run_id,
                    "estimated_residency": residency,
                    "l1d_bytes": cache_info.get("l1d_bytes"),
                    "l2_bytes": cache_info.get("l2_bytes"),
                    "l3_bytes": cache_info.get("l3_bytes"),
                    "eff_l1d_bytes": cache_info.get("eff_l1d_bytes"),
                    "perf_l1d_bytes": cache_info.get("perf_l1d_bytes"),
                    "eff_l2_bytes": cache_info.get("eff_l2_bytes"),
                    "perf_l2_bytes": cache_info.get("perf_l2_bytes"),
                    "eff_l1i_bytes": cache_info.get("eff_l1i_bytes"),
                    "perf_l1i_bytes": cache_info.get("perf_l1i_bytes"),
                    "cache_model": cache_info.get("cache_model"),
                    "cache_source": cache_info.get("source"),
                },
                meta,
                "pointer_latency",
                write_header=not header_written,
            )
            header_written = True

            print(
                f"run {run_id:2d}: elapsed = {result['elapsed_s']:.6f} s, "
                f"latency = {result['latency_ns']:.2f} ns, "
                f"energy = {result['energy_j'] if result['energy_j'] is not None else float('nan'):.4f} J"
            )

        mean_lat = stats.mean(latency_values)
        stdev_lat = stats.pstdev(latency_values) if len(latency_values) > 1 else 0.0
        print(f"==> ŚREDNIA: {mean_lat:.2f} ns, σ = {stdev_lat:.2f} ns")

        if energy_values:
            mean_energy = stats.mean(energy_values)
            stdev_energy = stats.pstdev(energy_values) if len(energy_values) > 1 else 0.0
            print(f"    ŚREDNIA energia per run: {mean_energy:.4f} J, σ = {stdev_energy:.4f} J")

    print(f"\nWszystkie runy zapisane do: {csv_path}")


if __name__ == "__main__":
    main()
