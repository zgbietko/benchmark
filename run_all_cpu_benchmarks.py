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

from cpu_utils import detect_cpu_model

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


def run_benchmark(relative_path: str) -> bool:
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
            [sys.executable, str(script_path)],
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
        "cpu/benchmarks/run_stream.py",
        "cpu/benchmarks/run_stream_mt.py",
        "cpu/benchmarks/run_compute_fma.py",
        "cpu/benchmarks/run_compute_fma_peak.py",
    ]

    if profile == "generic":
        return generic

    vendor_prefix = f"cpu/{profile}/benchmarks"
    vendor_list = [
        f"{vendor_prefix}/run_bandwidth.py",
        f"{vendor_prefix}/run_bandwidth_mt.py",
        f"{vendor_prefix}/run_pointer_latency.py",
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


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Run all CPU benchmarks with selectable architecture profile."
    )
    ap.add_argument(
        "--arch-profile",
        choices=["auto", "generic", "intel", "amd"],
        default="auto",
        help="Profil benchmarków CPU: auto wykrywa Intel/AMD na x86, inaczej generic.",
    )
    args = ap.parse_args()

    profile = _resolve_arch_profile(args.arch_profile)
    print("=== CPU microbenchmarks: start ===")
    print(f"[INFO] CPU arch profile: {profile}")

    # 1. Upewnij się, że biblioteka CPU jest zbudowana
    ensure_cpu_lib_built()

    # 2. Lista benchmarków do uruchomienia (w ustalonej kolejności)
    benchmarks = _benchmark_list_for_profile(profile)

    # 3. Uruchamiaj po kolei; w razie błędu przerwij
    for rel in benchmarks:
        ok = run_benchmark(rel)
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
