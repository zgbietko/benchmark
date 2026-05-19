# cpu/benchmarks/run_tlb_latency.py
from __future__ import annotations

import argparse
import ctypes as ct
import csv
import mmap
import platform
import statistics as stats
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from cpu_utils import make_cpu_specific_csv_path
from energy import energy_measurement_supported, read_energy_joules


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
    return ct.CDLL(str(lib_path)), ROOT


def configure_functions(lib):
    func = lib.pointer_chase_kernel
    func.argtypes = [ct.POINTER(ct.c_uint32), ct.c_size_t, ct.c_size_t]
    func.restype = None
    return func


def detect_cpu_model() -> str:
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
    except Exception:
        pass
    return platform.processor() or platform.machine()


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


def _build_page_cycle_indices(n_pages: int, elems_per_page: int) -> np.ndarray:
    page_ids = np.arange(n_pages, dtype=np.uint32)
    np.random.shuffle(page_ids)
    total_elems = n_pages * elems_per_page
    idx_arr = np.zeros(total_elems, dtype=np.uint32)
    for i in range(n_pages):
        current = int(page_ids[i]) * elems_per_page
        nxt = int(page_ids[(i + 1) % n_pages]) * elems_per_page
        idx_arr[current] = nxt
    return idx_arr


def bench_tlb_pointer_chase(pointer_chase_kernel, pages_touched: int, page_size_bytes: int, iters_inner: int):
    elem_size = np.dtype(np.uint32).itemsize
    elems_per_page = max(1, page_size_bytes // elem_size)
    idx_arr = _build_page_cycle_indices(pages_touched, elems_per_page)
    idx_p = idx_arr.ctypes.data_as(ct.POINTER(ct.c_uint32))

    pointer_chase_kernel(idx_p, idx_arr.size, 1)

    energy_j = None
    e_before = None
    if energy_measurement_supported():
        e_before = read_energy_joules()

    t0 = time.perf_counter()
    pointer_chase_kernel(idx_p, idx_arr.size, iters_inner)
    t1 = time.perf_counter()

    if e_before is not None:
        e_after = read_energy_joules()
        if e_after is not None:
            delta = e_after - e_before
            if delta >= 0:
                energy_j = delta

    elapsed = t1 - t0
    latency_ns = (elapsed / iters_inner) * 1e9
    avg_power_w = None
    if energy_j is not None and elapsed > 0:
        avg_power_w = energy_j / elapsed

    return {
        "pages_touched": pages_touched,
        "page_size_bytes": page_size_bytes,
        "stride_bytes": page_size_bytes,
        "working_set_bytes": pages_touched * page_size_bytes,
        "working_set_kb": (pages_touched * page_size_bytes) // 1024,
        "iters_inner": iters_inner,
        "elapsed_s": elapsed,
        "latency_ns": latency_ns,
        "energy_j": energy_j,
        "power_w": avg_power_w,
        "avg_power_w": avg_power_w,
    }


def write_result_to_csv(csv_path: Path, result: dict, meta: dict, benchmark_name: str, write_header: bool):
    row = {**meta, "benchmark": benchmark_name, **result}
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
        "pages_touched",
        "page_size_bytes",
        "stride_bytes",
        "working_set_bytes",
        "working_set_kb",
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


def _build_arg_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="CPU TLB/page-walk pointer-chasing latency benchmark.")
    ap.add_argument("--iters-inner", type=int, default=200_000)
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument(
        "--pages",
        default="16,32,64,128,256,512,1024,2048,4096,8192",
        help="Lista liczby stron dotykanych przez pointer chase.",
    )
    return ap


def main():
    args = _build_arg_parser().parse_args()
    lib, root = load_library()
    pointer_chase_kernel = configure_functions(lib)

    data_dir = root / "data" / "cpu"
    data_dir.mkdir(parents=True, exist_ok=True)
    csv_path, arch, cpu_model, _cpu_slug = make_cpu_specific_csv_path("tlb_latency", data_dir)
    if csv_path.exists():
        csv_path.unlink()

    meta = collect_system_metadata()
    meta["arch"] = arch
    meta["cpu_model"] = cpu_model

    page_size = int(mmap.PAGESIZE)
    pages_list = sorted({max(1, int(part.strip())) for part in args.pages.split(",") if part.strip()})
    iters_inner = max(1, int(args.iters_inner))
    runs = max(1, int(args.runs))

    print("=== CPU TLB / page-walk latency benchmark ===")
    print(f"page_size       : {page_size} B")
    print(f"iters_inner     : {iters_inner}")
    print(f"runs per point  : {runs}")
    print(f"CPU model       : {meta['cpu_model']}")
    if energy_measurement_supported():
        print("Energy          : best-effort per-run energy_j / avg_power_w")
    else:
        print("Energy          : pomiar niedostępny na tej platformie")

    header_written = False
    for pages_touched in pages_list:
        working_set_kb = (pages_touched * page_size) // 1024
        latency_values = []
        energy_values = []
        print(f"\n--- Pages touched: {pages_touched} (working set {working_set_kb} KB) ---")
        for run_id in range(runs):
            result = bench_tlb_pointer_chase(pointer_chase_kernel, pages_touched, page_size, iters_inner)
            latency_values.append(result["latency_ns"])
            if result["energy_j"] is not None:
                energy_values.append(result["energy_j"])
            write_result_to_csv(
                csv_path,
                {**result, "run_id": run_id},
                meta,
                "tlb_latency",
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
