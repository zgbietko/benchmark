#!/usr/bin/env python3
"""
run_all_cpu_benchmarks.py

Orkiestruje uruchamianie wszystkich benchmarków CPU:

1. Sprawdza / buduje bibliotekę CPU:
   - macOS: cpu/lib/libmicrobench.dylib (build_mac.sh)
   - Linux: cpu/lib/libmicrobench.so    (build_linux.sh)

2. Uruchamia kolejno:
   - cpu/benchmarks/run_bandwidth.py
   - cpu/benchmarks/run_bandwidth_mt.py
   - cpu/benchmarks/run_pointer_latency.py
   - cpu/benchmarks/run_tlb_latency.py
   - cpu/benchmarks/run_compute_fma.py
   - cpu/benchmarks/run_compute_fma_peak.py

3. Jeśli wszystkie zakończą się sukcesem, uruchamia:
   - analysis/cpu_summary.py

Wszystkie skrypty benchmarków zapisują dane do data/cpu/*.csv.
"""

from __future__ import annotations

import argparse
import platform
import subprocess
import sys
from pathlib import Path

from cpu_utils import (
    detect_cpu_model,
    detect_cpu_topology,
    extended_thread_sweep,
    memcopy_size_profile,
    recommended_thread_sweep,
    stream_size_profile,
)

ROOT = Path(__file__).resolve().parent

def _sources_newer_than_lib(lib_path: Path, sources: list[Path]) -> bool:
    try:
        lib_mtime = lib_path.stat().st_mtime
    except FileNotFoundError:
        return True
    for s in sources:
        try:
            if s.stat().st_mtime > lib_mtime:
                return True
        except FileNotFoundError:
            continue
    return False


def ensure_cpu_lib_built() -> None:
    """
    Upewnia się, że biblioteka CPU została zbudowana dla aktualnego systemu.

    macOS (Darwin):
        cpu/lib/libmicrobench.dylib  <- build_mac.sh

    Linux:
        cpu/lib/libmicrobench.so     <- build_linux.sh

    Jeśli biblioteka nie istnieje, wywołuje odpowiedni skrypt build_*.
    """
    lib_dir = ROOT / "cpu" / "lib"
    system = platform.system()

    if system == "Darwin":
        lib_path = lib_dir / "libmicrobench.dylib"
        build_script = "build_mac.sh"
    elif system == "Linux":
        lib_path = lib_dir / "libmicrobench.so"
        build_script = "build_linux.sh"
    else:
        raise RuntimeError(f"Nieobsługiwany system dla benchmarków CPU: {system}")

    sources = [lib_dir / "microbench.c", lib_dir / "microbench.h"]
    if lib_path.exists() and not _sources_newer_than_lib(lib_path, sources):
        print(f"[INFO] Biblioteka CPU już zbudowana: {lib_path}")
        return

    if lib_path.exists():
        print(f"[INFO] Biblioteka CPU jest starsza niż źródła – przebudowuję: {lib_path}")

    print(f"[INFO] Biblioteka CPU nie istnieje ({lib_path}), uruchamiam {build_script}...")
    try:
        subprocess.run(
            ["bash", build_script],
            cwd=lib_dir,
            check=True,
        )
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Nie udało się zbudować biblioteki CPU ({build_script}): {e}") from e

    if not lib_path.exists():
        raise RuntimeError(
            f"Po wykonaniu {build_script} nadal brak biblioteki: {lib_path}"
        )

    print(f"[INFO] OK, zbudowano bibliotekę: {lib_path}")


def run_benchmark(relative_path: str, extra_args: list[str] | None = None) -> bool:
    """
    Uruchamia pojedynczy skrypt benchmarku (relative_path względem ROOT).

    Zwraca:
        True  - jeśli skrypt zakończył się kodem 0,
        False - jeśli nastąpił błąd (kod != 0 lub inny wyjątek).
    """
    script_path = ROOT / relative_path
    if not script_path.exists():
        print(f"[ERROR] Nie znaleziono skryptu benchmarku: {script_path}")
        return False

    print(f"\n=== Uruchamiam: {relative_path} ===")
    try:
        result = subprocess.run(
            [sys.executable, str(script_path), *(extra_args or [])],
            cwd=ROOT,
            check=False,
        )
    except Exception as e:
        print(f"[ERROR] Wyjątek przy uruchamianiu {relative_path}: {e}")
        return False

    if result.returncode != 0:
        print(f"[ERROR] Skrypt {relative_path} zakończył się kodem {result.returncode}")
        return False

    print(f"[INFO] Zakończono pomyślnie: {relative_path}")
    return True


def run_cpu_summary() -> None:
    """
    Uruchamia analysis/cpu_summary.py, jeśli istnieje.
    """
    summary_path = ROOT / "analysis" / "cpu_summary.py"
    if not summary_path.exists():
        print("[WARN] Brak pliku analysis/cpu_summary.py – pomijam podsumowanie.")
        return

    print("\n=== Uruchamiam podsumowanie: analysis/cpu_summary.py ===")
    try:
        subprocess.run(
            [sys.executable, str(summary_path)],
            cwd=ROOT,
            check=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Podsumowanie CPU nie powiodło się: {e}")


def _resolve_arch_profile(user_profile: str) -> str:
    if user_profile != "auto":
        return user_profile

    arch = platform.machine().lower()
    cpu_model = detect_cpu_model().lower()
    if platform.system() == "Darwin" and ("apple" in cpu_model or arch in ("arm64", "aarch64")):
        return "apple"
    if arch in ("x86_64", "amd64"):
        if "intel" in cpu_model:
            return "intel"
        if "amd" in cpu_model:
            return "amd"
    return "generic"


def _benchmark_list_for_profile(profile: str) -> list[str]:
    generic = [
        "cpu/benchmarks/run_bandwidth.py",
        "cpu/benchmarks/run_bandwidth_mt.py",
        "cpu/benchmarks/run_pointer_latency.py",
        "cpu/benchmarks/run_tlb_latency.py",
        "cpu/benchmarks/run_stream.py",
        "cpu/benchmarks/run_stream_mt.py",
        "cpu/benchmarks/run_compute_fma.py",
        "cpu/benchmarks/run_compute_fma_peak.py",
    ]

    if profile in {"generic", "apple"}:
        return generic

    vendor_prefix = f"cpu/{profile}/benchmarks"
    vendor_list = [
        f"{vendor_prefix}/run_bandwidth.py",
        f"{vendor_prefix}/run_bandwidth_mt.py",
        f"{vendor_prefix}/run_pointer_latency.py",
        f"{vendor_prefix}/run_tlb_latency.py",
        f"{vendor_prefix}/run_stream.py",
        f"{vendor_prefix}/run_stream_mt.py",
        f"{vendor_prefix}/run_compute_fma.py",
        f"{vendor_prefix}/run_compute_fma_peak.py",
    ]

    missing = [p for p in vendor_list if not (ROOT / p).exists()]
    if missing:
        print(f"[WARN] Brak części benchmarków dla profilu '{profile}', fallback do 'generic'.")
        for p in missing:
            print(f"       - missing: {p}")
        return generic

    return vendor_list


def _cap_thread_sweep(threads: list[int], max_threads: int) -> list[int]:
    if max_threads <= 0:
        return sorted({int(v) for v in threads if int(v) > 0})
    capped = [int(v) for v in threads if 0 < int(v) <= int(max_threads)]
    if not capped:
        capped = [int(max_threads)]
    return sorted(set(capped))


def _thread_args_for_mode(*, mode: str, max_threads: int) -> list[str]:
    topology = detect_cpu_topology()
    if mode == "extended":
        sweep = extended_thread_sweep(topology)
    else:
        sweep = recommended_thread_sweep(topology)
    sweep = _cap_thread_sweep(sweep, max_threads)
    if not sweep:
        return []
    return ["--device-threads", ",".join(str(v) for v in sweep)]


def _memcopy_size_args(mode: str) -> list[str]:
    sizes = memcopy_size_profile(mode)
    if not sizes:
        return []
    return ["--sizes-mb", ",".join(str(v) for v in sizes)]


def _stream_size_args(mode: str) -> list[str]:
    sizes = stream_size_profile(mode)
    if not sizes:
        return []
    return ["--sizes-mb", ",".join(str(v) for v in sizes)]


def _extended_benchmark_args(relative_path: str, *, max_threads: int) -> list[str]:
    """
    Dodatkowe argumenty dla trybu extended.

    Extended ma sluzyc do diagnostyki architektury, nie do zastapienia trybu
    porownawczego. Dlatego:
    - dla MT robimy gestszy sweep watkow,
    - dla mem_copy ujednolicamy zakres 1T/MT i zageszczamy rozmiary,
    - dla opoznien pamieci zwiekszamy stabilnosc pomiaru i zakres TLB.
    """
    name = Path(relative_path).name
    if name == "run_bandwidth.py":
        return _memcopy_size_args("extended")
    if name == "run_stream.py":
        return _stream_size_args("extended")
    if name in {"run_bandwidth_mt.py", "run_stream_mt.py", "run_compute_fma_peak.py"}:
        extra = _thread_args_for_mode(mode="extended", max_threads=max_threads)
        if name == "run_bandwidth_mt.py":
            extra.extend(_memcopy_size_args("extended"))
        if name == "run_stream_mt.py":
            extra.extend(_stream_size_args("extended"))
        return extra
    if name == "run_pointer_latency.py":
        return [
            "--runs",
            "7",
            "--iters-inner",
            "400000",
        ]
    if name == "run_tlb_latency.py":
        return [
            "--runs",
            "7",
            "--iters-inner",
            "400000",
            "--pages",
            "8,16,32,64,128,256,512,1024,2048,4096,8192,16384",
        ]
    return []


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Run all CPU benchmarks with selectable architecture profile."
    )
    ap.add_argument(
        "--arch-profile",
        choices=["auto", "generic", "apple", "intel", "amd"],
        default="auto",
        help="Profil benchmarków CPU: auto wykrywa Apple/Intel/AMD; profile vendorowe mogą nadpisywać benchmarki generic.",
    )
    ap.add_argument(
        "--benchmark-mode",
        choices=["standard", "extended"],
        default="standard",
        help="standard = wspolna sciezka porownawcza; extended = gestsze i bardziej diagnostyczne pomiary architektury.",
    )
    ap.add_argument(
        "--max-threads",
        type=int,
        default=0,
        help="Maksymalna liczba watkow CPU dla benchmarkow MT. 0 = pelna topologia wykryta automatycznie.",
    )
    args = ap.parse_args()

    profile = _resolve_arch_profile(args.arch_profile)
    print("=== CPU microbenchmarks: start ===")
    print(f"[INFO] CPU arch profile: {profile}")
    print(f"[INFO] CPU benchmark mode: {args.benchmark_mode}")

    # 1. Upewnij się, że biblioteka CPU jest zbudowana
    ensure_cpu_lib_built()

    # 2. Lista benchmarków do uruchomienia (w ustalonej kolejności)
    benchmarks = _benchmark_list_for_profile(profile)

    # 3. Uruchamiaj po kolei; w razie błędu przerwij
    for rel in benchmarks:
        extra_args: list[str] = []
        name = Path(rel).name
        if args.benchmark_mode == "extended":
            extra_args = _extended_benchmark_args(rel, max_threads=int(args.max_threads))
        elif args.max_threads > 0 and name in {"run_bandwidth_mt.py", "run_stream_mt.py", "run_compute_fma_peak.py"}:
            extra_args = _thread_args_for_mode(mode="standard", max_threads=int(args.max_threads))
        if extra_args:
            print(f"[INFO] Extra args for {Path(rel).name}: {' '.join(extra_args)}")
        ok = run_benchmark(rel, extra_args=extra_args)
        if not ok:
            print(
                "\n[ERROR] Przerywam, ponieważ jeden z benchmarków zakończył się błędem."
            )
            print("[INFO] Podsumowanie CPU zostało pominięte z powodu błędów.")
            return

    # 4. Wszystkie benchmarki zakończone sukcesem – uruchom podsumowanie
    run_cpu_summary()

    print("\n=== CPU microbenchmarks: koniec ===")


if __name__ == "__main__":
    main()
