# cpu/benchmarks/run_stream.py
import ctypes as ct
import csv
import platform
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from energy import energy_measurement_supported, read_energy_joules
from cpu_utils import make_cpu_specific_csv_path, detect_cpu_model


DEFAULT_SIZES_MB = [4, 16, 64, 256, 1024]  # per-array size
DEFAULT_RUNS_PER_SIZE = 7
DEFAULT_ITERS_PER_RUN = 10
SCALAR = 3.0


def load_library():
    system = platform.system()
    if system == "Darwin":
        lib_name = "libmicrobench.dylib"
    elif system == "Linux":
        lib_name = "libmicrobench.so"
    elif system == "Windows":
        lib_name = "microbench.dll"
    else:
        raise RuntimeError(f"Nieobsługiwany system: {system}")

    lib_path = ROOT / "cpu" / "lib" / lib_name
    if not lib_path.exists():
        raise FileNotFoundError(f"Nie znaleziono biblioteki: {lib_path}")

    return ct.CDLL(str(lib_path))


def configure_functions(lib):
    f_copy = lib.stream_copy_kernel
    f_copy.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_size_t]
    f_copy.restype = None

    f_scale = lib.stream_scale_kernel
    f_scale.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_float, ct.c_size_t]
    f_scale.restype = None

    f_add = lib.stream_add_kernel
    f_add.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_size_t]
    f_add.restype = None

    f_triad = lib.stream_triad_kernel
    f_triad.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_float, ct.c_size_t]
    f_triad.restype = None

    return f_copy, f_scale, f_add, f_triad


def collect_system_metadata(cpu_model: str) -> dict:
    return {
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "arch": platform.machine(),
        "os": platform.system(),
        "os_release": platform.release(),
        "cpu_model": cpu_model,
        "dtype": "float32",
    }


def _bytes_per_iter(kernel: str, n_bytes: int) -> int:
    # STREAM rules (per element in FP32):
    # copy  : read 1 + write 1 = 2 arrays
    # scale : read 1 + write 1 = 2 arrays
    # add   : read 2 + write 1 = 3 arrays
    # triad : read 2 + write 1 = 3 arrays
    if kernel in ("copy", "scale"):
        return 2 * n_bytes
    if kernel in ("add", "triad"):
        return 3 * n_bytes
    raise ValueError(kernel)


def bench_one(kernel_name: str, fn, args_tuple, iters: int):
    energy_j = None
    e_before = None
    if energy_measurement_supported():
        e_before = read_energy_joules()

    t0 = time.perf_counter()
    for _ in range(iters):
        fn(*args_tuple)
    t1 = time.perf_counter()

    if e_before is not None:
        e_after = read_energy_joules()
        if e_after is not None:
            delta = e_after - e_before
            if delta >= 0:
                energy_j = delta

    elapsed = t1 - t0
    avg_power_w = energy_j / elapsed if (energy_j is not None and elapsed > 0) else None
    return elapsed, energy_j, avg_power_w


def write_rows(csv_path: Path, rows: list[dict]) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = list(rows[0].keys())
    write_header = not csv_path.exists()
    with csv_path.open("a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        if write_header:
            w.writeheader()
        for r in rows:
            w.writerow(r)


def main():
    print("=== CPU STREAM benchmark (single-thread) ===")
    cpu_model = detect_cpu_model()
    meta = collect_system_metadata(cpu_model)

    sizes_mb = DEFAULT_SIZES_MB
    runs_per_size = DEFAULT_RUNS_PER_SIZE
    iters_per_run = DEFAULT_ITERS_PER_RUN

    print(f"runs per size : {runs_per_size}")
    print(f"iters per run : {iters_per_run}")
    print(f"CPU model     : {cpu_model}")
    if energy_measurement_supported():
        if platform.system() == "Darwin":
            print("Energy        : macOS powermetrics (CPU power), per-kernel energy_j / avg_power_w")
        else:
            print("Energy        : Linux RAPL (CPU), per-kernel energy_j / avg_power_w")
    else:
        print("Energy        : not available (energy_j = NaN)")

    lib = load_library()
    f_copy, f_scale, f_add, f_triad = configure_functions(lib)

    data_dir = ROOT / "data" / "cpu"
    csv_path, arch, cpu_model, _cpu_slug = make_cpu_specific_csv_path("stream", data_dir)
    meta["arch"] = arch
    meta["cpu_model"] = cpu_model
    rows_out = []

    for mb in sizes_mb:
        n_bytes = mb * 1024 * 1024
        n = n_bytes // 4  # float32

        print(f"\n--- Size per array: {mb} MB (n={n}) ---")

        # arrays
        a = np.empty(n, dtype=np.float32)
        b = np.empty(n, dtype=np.float32)
        c = np.empty(n, dtype=np.float32)

        a.fill(0.0)
        b.fill(1.0)
        c.fill(2.0)

        ap = a.ctypes.data_as(ct.POINTER(ct.c_float))
        bp = b.ctypes.data_as(ct.POINTER(ct.c_float))
        cp = c.ctypes.data_as(ct.POINTER(ct.c_float))

        # warm-up
        f_copy(ap, bp, n)
        f_scale(ap, bp, ct.c_float(SCALAR), n)
        f_add(ap, bp, cp, n)
        f_triad(ap, bp, cp, ct.c_float(SCALAR), n)

        kernels = [
            ("copy", f_copy, (ap, bp, n)),
            ("scale", f_scale, (ap, bp, ct.c_float(SCALAR), n)),
            ("add", f_add, (ap, bp, cp, n)),
            ("triad", f_triad, (ap, bp, cp, ct.c_float(SCALAR), n)),
        ]

        for (kname, fn, args) in kernels:
            bytes_iter = _bytes_per_iter(kname, n_bytes)
            for run in range(runs_per_size):
                elapsed, energy_j, p_w = bench_one(kname, fn, args, iters_per_run)
                gbps = (bytes_iter * iters_per_run) / elapsed / 1e9 if elapsed > 0 else 0.0
                print(f"{kname:5s} run {run:2d}: elapsed = {elapsed:.4f} s, GB/s = {gbps:7.2f}, energy = {0.0 if energy_j is None else energy_j:.4f} J")

                row = {
                    **meta,
                    "benchmark": "stream",
                    "kernel": kname,
                    "size_mb": mb,
                    "n_elements": n,
                    "iters": iters_per_run,
                    "bytes_per_iter": bytes_iter,
                    "elapsed_s": elapsed,
                    "gbps": gbps,
                    "energy_j": "" if energy_j is None else energy_j,
                    "avg_power_w": "" if p_w is None else p_w,
                }
                rows_out.append(row)

    write_rows(csv_path, rows_out)
    print(f"\nWszystkie runy zapisane do: {csv_path}")


if __name__ == "__main__":
    main()
