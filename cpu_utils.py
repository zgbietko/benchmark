# cpu_utils.py
#
# Wspólne funkcje dla benchmarków CPU:
# - wykrywanie modelu CPU (macOS + Linux),
# - generowanie "slug-a" z nazwy CPU do nazw plików,
# - tworzenie ścieżki CSV specyficznej dla danego CPU.

from __future__ import annotations

import platform
import subprocess
import os
from pathlib import Path


def detect_cpu_model() -> str:
    """
    Zwraca nazwę modelu CPU w formie czytelnej (np. 'Apple M2 Pro',
    'Intel(R) Core(TM) i7-12700H').
    Działa na macOS (x86 + ARM) i Linuksie.
    """
    system = platform.system()

    # macOS
    if system == "Darwin":
        # klasyczne x86/ARM
        try:
            out = subprocess.check_output(
                ["sysctl", "-n", "machdep.cpu.brand_string"],
                text=True,
            ).strip()
            if out:
                return out
        except Exception:
            pass

        # system_profiler (Apple Silicon): "Chip: Apple M2 Pro"
        try:
            out = subprocess.check_output(
                ["system_profiler", "SPHardwareDataType"],
                text=True,
            )
            for line in out.splitlines():
                s = line.strip()
                if s.startswith("Chip:"):
                    return s.split("Chip:", 1)[1].strip()
                if s.startswith("Processor Name:"):
                    return s.split("Processor Name:", 1)[1].strip()
        except Exception:
            pass

        # czasem na Apple Silicon można próbować innych kluczy
        try:
            out = subprocess.check_output(
                ["sysctl", "-n", "machdep.cpu.apple_cpu_core"],
                text=True,
            ).strip()
            if out:
                return out
        except Exception:
            pass

        # fallback: model maszyny (np. "Mac14,6")
        try:
            out = subprocess.check_output(
                ["sysctl", "-n", "hw.model"],
                text=True,
            ).strip()
            if out:
                return out
        except Exception:
            pass

        return "Unknown macOS CPU"

    # Linux
    if system == "Linux":
        model = None
        try:
            with open("/proc/cpuinfo", "r", encoding="utf-8") as f:
                for line in f:
                    if "model name" in line:
                        model = line.split(":", 1)[1].strip()
                        break
                    if "Hardware" in line and model is None:
                        model = line.split(":", 1)[1].strip()
            if model:
                return model
        except Exception:
            pass

        return "Unknown Linux CPU"

    # inne systemy – fallback
    return platform.processor() or "Unknown CPU"


def slugify_cpu_model(cpu_model: str) -> str:
    """
    Zamienia nazwę CPU na 'slug' do użycia w nazwach plików, np.:
        'Apple M2 Pro' -> 'apple_m2_pro'
        'Intel(R) Core(TM) i7-12700H' -> 'intel_r_core_tm_i7_12700h'
    """
    s = cpu_model.strip().lower()
    for ch in " -()/":
        s = s.replace(ch, "_")
    allowed = set("abcdefghijklmnopqrstuvwxyz0123456789_")
    s = "".join(ch for ch in s if ch in allowed)
    while "__" in s:
        s = s.replace("__", "_")
    s = s.strip("_")
    return s or "unknown_cpu"


def make_cpu_specific_csv_path(kind: str, data_cpu_dir: Path):
    """
    Tworzy ścieżkę do pliku CSV dla KONKRETNEGO CPU.

    kind: np. 'bandwidth', 'bandwidth_mt', 'pointer_latency',
          'compute_fma', 'compute_fma_peak'

    data_cpu_dir: Path do katalogu 'data/cpu'.

    Zwraca: (csv_path, arch, cpu_model, cpu_slug)
    """
    arch = platform.machine()
    cpu_model = detect_cpu_model()
    cpu_slug = slugify_cpu_model(cpu_model)
    filename = f"{kind}_{cpu_slug}.csv"

    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        cpu_dir = Path(run_root) / "cpu"
        cpu_dir.mkdir(parents=True, exist_ok=True)
        csv_path = cpu_dir / filename
    else:
        csv_path = data_cpu_dir / filename
    return csv_path, arch, cpu_model, cpu_slug
