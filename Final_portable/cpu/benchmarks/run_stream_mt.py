# cpu/benchmarks/run_stream_mt.py
import argparse
import ctypes as ct
import csv
import os
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

from energy import energy_measurement_supported, energy_measurement_label, read_energy_joules
from cpu_utils import (
    classify_thread_point,
    detect_cpu_model,
    detect_cpu_topology,
    make_cpu_specific_csv_path,
    recommended_thread_sweep,
    stream_size_profile,
    throughput_gbps_decimal,
)


DEFAULT_RUNS_PER_CONFIG = 5
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
    f_copy = lib.stream_copy_kernel_mt
    f_copy.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_size_t, ct.c_int]
    f_copy.restype = None

    f_scale = lib.stream_scale_kernel_mt
    f_scale.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_float, ct.c_size_t, ct.c_int]
    f_scale.restype = None

    f_add = lib.stream_add_kernel_mt
    f_add.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_size_t, ct.c_int]
    f_add.restype = None

    f_triad = lib.stream_triad_kernel_mt
    f_triad.argtypes = [ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.c_float, ct.c_size_t, ct.c_int]
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
    if kernel in ("copy", "scale"):
        return 2 * n_bytes
    if kernel in ("add", "triad"):
        return 3 * n_bytes
    raise ValueError(kernel)


def bench_one(fn, args_tuple, iters: int):
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
    ap = argparse.ArgumentParser()
    ap.add_argument("--device-threads", type=str, default="", help="Lista wątków, np. 1,2,4,8,12 (domyślnie: automatyczny sweep topologii CPU)")
    ap.add_argument(
        "--sizes-mb",
        type=str,
        default="",
        help="Lista rozmiarów w MB, np. 4,16,64,256,1024 (domyślnie: wspólny profil standard)",
    )
    args = ap.parse_args()

    cpu_model = detect_cpu_model()
    meta = collect_system_metadata(cpu_model)

    if args.sizes_mb.strip():
        sizes_mb = [int(x) for x in args.sizes_mb.split(",") if x.strip()]
    else:
        sizes_mb = stream_size_profile("standard")
    runs_per_cfg = DEFAULT_RUNS_PER_CONFIG
    iters_per_run = DEFAULT_ITERS_PER_RUN

    topology = detect_cpu_topology()
    if args.device_threads.strip():
        threads_list = [int(x) for x in args.device_threads.split(",") if x.strip()]
    else:
        threads_list = recommended_thread_sweep(topology)

    print("=== CPU STREAM benchmark (multi-thread) ===")
    print(f"runs per config : {runs_per_cfg}")
    print(f"iters per run   : {iters_per_run}")
    print(f"CPU model       : {cpu_model}")
    print(f"sizes [MB]      : {sizes_mb}")
    print(f"thread sweep    : {threads_list}")
    print(f"Energy          : {energy_measurement_label()}")

    lib = load_library()
    f_copy, f_scale, f_add, f_triad = configure_functions(lib)

    data_dir = ROOT / "data" / "cpu"
    csv_path, arch, cpu_model, _cpu_slug = make_cpu_specific_csv_path("stream_mt", data_dir)
    meta["arch"] = arch
    meta["cpu_model"] = cpu_model
    rows_out = []

    for mb in sizes_mb:
        n_bytes = mb * 1024 * 1024
        n = n_bytes // 4

        # arrays
        a = np.empty(n, dtype=np.float32)
        b = np.empty(n, dtype=np.float32)
        c = np.empty(n, dtype=np.float32)
        a.fill(0.0)
        b.fill(1.0)
        c.fill(2.0)

        aptr = a.ctypes.data_as(ct.POINTER(ct.c_float))
        bptr = b.ctypes.data_as(ct.POINTER(ct.c_float))
        cptr = c.ctypes.data_as(ct.POINTER(ct.c_float))

        # warm-up
        f_copy(aptr, bptr, n, 1)
        f_scale(aptr, bptr, ct.c_float(SCALAR), n, 1)
        f_add(aptr, bptr, cptr, n, 1)
        f_triad(aptr, bptr, cptr, ct.c_float(SCALAR), n, 1)

        kernels = [
            ("copy", f_copy, lambda th: (aptr, bptr, n, th)),
            ("scale", f_scale, lambda th: (aptr, bptr, ct.c_float(SCALAR), n, th)),
            ("add", f_add, lambda th: (aptr, bptr, cptr, n, th)),
            ("triad", f_triad, lambda th: (aptr, bptr, cptr, ct.c_float(SCALAR), n, th)),
        ]

        for th in threads_list:
            print(f"\n--- Size per array: {mb} MB, threads: {th} ---")
            for kname, fn, args_fn in kernels:
                bytes_iter = _bytes_per_iter(kname, n_bytes)
                for run in range(runs_per_cfg):
                    elapsed, energy_j, p_w = bench_one(fn, args_fn(th), iters_per_run)
                    gbps = throughput_gbps_decimal(bytes_iter * iters_per_run, elapsed)
                    print(f"{kname:5s} run {run:2d}: elapsed = {elapsed:.4f} s, GB/s = {gbps:7.2f}, energy = {(energy_j if energy_j is not None else float('nan')):.4f} J")

                    row = {
                        **meta,
                        "benchmark": "stream_mt",
                        "kernel": kname,
                        "size_mb": mb,
                        "n_elements": n,
                        "threads": th,
                        "thread_role": classify_thread_point(th, topology),
                        "iters": iters_per_run,
                        "bytes_per_iter": bytes_iter,
                        "elapsed_s": elapsed,
                        "gbps": gbps,
                        "gbps_per_thread": gbps / max(1, th),
                        "throughput_unit": "GB/s",
                        "energy_j": "" if energy_j is None else energy_j,
                        "avg_power_w": "" if p_w is None else p_w,
                    }
                    rows_out.append(row)

    write_rows(csv_path, rows_out)
    print(f"\nWszystkie runy zapisane do: {csv_path}")


if __name__ == "__main__":
    main()
