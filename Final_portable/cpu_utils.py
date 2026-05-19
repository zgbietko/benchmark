# cpu_utils.py
#
# Wspólne funkcje dla benchmarków CPU:
# - wykrywanie modelu CPU (macOS + Linux),
# - generowanie "slug-a" z nazwy CPU do nazw plików,
# - tworzenie ścieżki CSV specyficznej dla danego CPU.

from __future__ import annotations

import os
import platform
import subprocess
from pathlib import Path
from typing import Any


STANDARD_MEMCOPY_SIZES_MB = [4, 16, 64, 256, 1024]
EXTENDED_MEMCOPY_SIZES_MB = [4, 8, 16, 32, 64, 128, 256, 512, 1024]
STANDARD_STREAM_SIZES_MB = [4, 16, 64, 256, 1024]
EXTENDED_STREAM_SIZES_MB = [4, 8, 16, 32, 64, 128, 256, 512, 1024]


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


def detect_cpu_topology() -> dict[str, Any]:
    """
    Best-effort wykrywanie topologii CPU potrzebnej do porównań między architekturami.

    Zwraca słownik z polami:
    - logical_cpus
    - physical_cpus
    - perf_logical_cpus
    - eff_logical_cpus
    - perf_physical_cpus
    - eff_physical_cpus
    - model
    - source
    """
    info: dict[str, Any] = {
        "logical_cpus": os.cpu_count() or 1,
        "physical_cpus": None,
        "perf_logical_cpus": None,
        "eff_logical_cpus": None,
        "perf_physical_cpus": None,
        "eff_physical_cpus": None,
        "model": "uniform",
        "source": "python",
    }

    system = platform.system()

    if system == "Darwin":
        mapping = {
            "logical_cpus": "hw.logicalcpu",
            "physical_cpus": "hw.physicalcpu",
            "perf_logical_cpus": "hw.perflevel0.logicalcpu",
            "eff_logical_cpus": "hw.perflevel1.logicalcpu",
            "perf_physical_cpus": "hw.perflevel0.physicalcpu",
            "eff_physical_cpus": "hw.perflevel1.physicalcpu",
        }
        parsed = False
        for field, sysctl_key in mapping.items():
            try:
                out = subprocess.check_output(
                    ["sysctl", "-n", sysctl_key],
                    text=True,
                    stderr=subprocess.DEVNULL,
                ).strip()
                if out:
                    info[field] = int(out)
                    parsed = True
            except Exception:
                continue
        if parsed:
            info["source"] = "sysctl"
        if info.get("perf_logical_cpus") and info.get("eff_logical_cpus"):
            info["model"] = "heterogeneous"
        return info

    if system == "Linux":
        try:
            out = subprocess.check_output(
                ["lscpu", "-p=CPU,CORE"],
                text=True,
                stderr=subprocess.DEVNULL,
            )
            logical_ids = set()
            core_ids = set()
            for line in out.splitlines():
                s = line.strip()
                if not s or s.startswith("#"):
                    continue
                parts = s.split(",")
                if len(parts) < 2:
                    continue
                logical_ids.add(parts[0])
                core_ids.add(parts[1])
            if logical_ids:
                info["logical_cpus"] = len(logical_ids)
                info["source"] = "lscpu"
            if core_ids:
                info["physical_cpus"] = len(core_ids)
        except Exception:
            pass
        return info

    return info


def recommended_thread_sweep(topology: dict[str, Any] | None = None) -> list[int]:
    """
    Generuje sensowny sweep liczby wątków dla porównań między architekturami.

    Zasady:
    - zachowujemy punkty 1/2/4/8/... dla czytelnego skalowania,
    - dodajemy naturalne punkty architektury:
      P-core plateau, physical CPU count, all-core logical count,
    - zwracamy posortowaną listę unikalnych dodatnich wartości.
    """
    topo = topology or detect_cpu_topology()
    logical = int(topo.get("logical_cpus") or 1)
    physical = int(topo.get("physical_cpus") or logical or 1)
    perf = topo.get("perf_logical_cpus")
    eff = topo.get("eff_logical_cpus")

    candidates: set[int] = {1}
    x = 1
    while x < logical:
        candidates.add(x)
        x *= 2
    candidates.add(logical)
    candidates.add(physical)

    if isinstance(perf, int) and perf > 0:
        candidates.add(perf)
    if isinstance(eff, int) and eff > 0:
        candidates.add(eff)
        if isinstance(perf, int) and perf > 0:
            candidates.add(perf + eff)

    return sorted(v for v in candidates if isinstance(v, int) and 1 <= v <= logical)


def extended_thread_sweep(topology: dict[str, Any] | None = None) -> list[int]:
    """
    Gestszy sweep watkow dla trybu extended.

    Cel:
    - zachowac porownywalny tryb standard jako podstawowy,
    - ale w trybie extended mocniej zbadac charakterystyke konkretnej maszyny,
      w tym heterogenicznosc Apple Silicon i punkty nasycenia na x86.

    Zasady:
    - dla mniejszych maszyn badamy wszystkie punkty 1..N,
    - dla wiekszych maszyn bierzemy gesty przod 1..16 oraz naturalne plateau
      (physical, perf-core, all-core),
    - nie dopuszczamy duplikatow ani wartosci > logical.
    """
    topo = topology or detect_cpu_topology()
    logical = max(1, int(topo.get("logical_cpus") or 1))
    physical = int(topo.get("physical_cpus") or logical)
    perf = topo.get("perf_logical_cpus")
    eff = topo.get("eff_logical_cpus")

    candidates: set[int] = set()

    if logical <= 16:
        candidates.update(range(1, logical + 1))
    elif logical <= 32:
        candidates.update(range(1, min(16, logical) + 1))
        for extra in (18, 20, 24, 28, 32):
            if extra <= logical:
                candidates.add(extra)
    else:
        candidates.update(range(1, min(12, logical) + 1))
        power = 16
        while power < logical:
            candidates.add(power)
            power *= 2

    natural_points = [
        physical,
        logical,
        int(perf) if isinstance(perf, int) and perf > 0 else None,
        int(eff) if isinstance(eff, int) and eff > 0 else None,
        (int(perf) + int(eff))
        if isinstance(perf, int) and perf > 0 and isinstance(eff, int) and eff > 0
        else None,
    ]
    for value in natural_points:
        if isinstance(value, int) and 1 <= value <= logical:
            candidates.add(value)

    return sorted(v for v in candidates if 1 <= int(v) <= logical)


def classify_thread_point(num_threads: int, topology: dict[str, Any] | None = None) -> str:
    topo = topology or detect_cpu_topology()
    logical = int(topo.get("logical_cpus") or num_threads or 1)
    physical = int(topo.get("physical_cpus") or logical)
    perf = topo.get("perf_logical_cpus")
    eff = topo.get("eff_logical_cpus")

    if num_threads <= 1:
        return "single-thread"
    if isinstance(perf, int) and perf > 0 and num_threads == perf:
        return "p-core-plateau"
    if isinstance(perf, int) and isinstance(eff, int) and perf > 0 and eff > 0 and num_threads == (perf + eff):
        return "all-core"
    if num_threads == logical:
        return "all-core"
    if num_threads == physical:
        return "physical-core-plateau"
    return "scaling"


def throughput_gbps_decimal(total_bytes: float, elapsed_s: float) -> float:
    if elapsed_s <= 0:
        return 0.0
    return (total_bytes / elapsed_s) / 1e9


def memcopy_size_profile(mode: str = "standard") -> list[int]:
    """
    Zwraca profil rozmiarów dla benchmarków mem_copy.

    standard:
        Wspólna, porównywalna ścieżka 1T/MT do wykresów między platformami.
    extended:
        Gęstszy sweep, żeby lepiej pokazać przejścia i momenty nasycenia.
    """
    normalized = str(mode or "standard").strip().lower()
    if normalized == "extended":
        return list(EXTENDED_MEMCOPY_SIZES_MB)
    return list(STANDARD_MEMCOPY_SIZES_MB)


def stream_size_profile(mode: str = "standard") -> list[int]:
    """
    Zwraca profil rozmiarów dla benchmarków STREAM.

    Utrzymujemy zgodność 1T/MT oraz spójność z mem_copy, żeby wykresy
    pamięci CPU były interpretowalne bez ukrytych różnic w siatce punktów.
    """
    normalized = str(mode or "standard").strip().lower()
    if normalized == "extended":
        return list(EXTENDED_STREAM_SIZES_MB)
    return list(STANDARD_STREAM_SIZES_MB)


def _parse_cache_size_token(token: str) -> int | None:
    s = token.strip().upper().replace("IB", "I").replace("B", "")
    if not s:
        return None
    mult = 1
    if s.endswith("K"):
        mult = 1024
        s = s[:-1]
    elif s.endswith("M"):
        mult = 1024 * 1024
        s = s[:-1]
    elif s.endswith("G"):
        mult = 1024 * 1024 * 1024
        s = s[:-1]
    try:
        return int(float(s) * mult)
    except Exception:
        return None


def detect_cpu_cache_hierarchy() -> dict[str, Any]:
    """
    Best-effort wykrywanie rozmiarów cache CPU.

    Zwraca słownik z polami:
    - l1d_bytes
    - l2_bytes
    - l3_bytes
    - source

    Wartości mogą być None, jeśli system ich nie udostępnia.
    """
    result: dict[str, Any] = {
        "l1d_bytes": None,
        "l2_bytes": None,
        "l3_bytes": None,
        "perf_l1d_bytes": None,
        "perf_l2_bytes": None,
        "eff_l1d_bytes": None,
        "eff_l2_bytes": None,
        "perf_l1i_bytes": None,
        "eff_l1i_bytes": None,
        "cache_model": "uniform",
        "source": "unknown",
    }
    system = platform.system()

    if system == "Darwin":
        perflevels: list[dict[str, Any]] = []
        try:
            out = subprocess.check_output(
                ["sysctl", "-n", "hw.nperflevels"],
                text=True,
                stderr=subprocess.DEVNULL,
            ).strip()
            nperf = int(out)
        except Exception:
            nperf = 0

        for idx in range(max(0, nperf)):
            prefix = f"hw.perflevel{idx}"
            level: dict[str, Any] = {"id": idx}
            for field, key in (
                ("name", "name"),
                ("l1d_bytes", "l1dcachesize"),
                ("l1i_bytes", "l1icachesize"),
                ("l2_bytes", "l2cachesize"),
                ("cpus_per_l2", "cpusperl2"),
            ):
                try:
                    out = subprocess.check_output(
                        ["sysctl", "-n", f"{prefix}.{key}"],
                        text=True,
                        stderr=subprocess.DEVNULL,
                    ).strip()
                    if field == "name":
                        level[field] = out
                    else:
                        level[field] = int(out)
                except Exception:
                    continue
            if any(k in level for k in ("l1d_bytes", "l2_bytes")):
                perflevels.append(level)

        if perflevels:
            perf_sorted = sorted(
                perflevels,
                key=lambda item: (
                    int(item.get("l1d_bytes") or 0),
                    int(item.get("l2_bytes") or 0),
                ),
            )
            eff = perf_sorted[0]
            perf = perf_sorted[-1]
            result["eff_l1d_bytes"] = eff.get("l1d_bytes")
            result["eff_l2_bytes"] = eff.get("l2_bytes")
            result["eff_l1i_bytes"] = eff.get("l1i_bytes")
            result["perf_l1d_bytes"] = perf.get("l1d_bytes")
            result["perf_l2_bytes"] = perf.get("l2_bytes")
            result["perf_l1i_bytes"] = perf.get("l1i_bytes")
            if len(perflevels) > 1:
                result["cache_model"] = "heterogeneous"

        keys = {
            "l1d_bytes": "hw.l1dcachesize",
            "l2_bytes": "hw.l2cachesize",
            "l3_bytes": "hw.l3cachesize",
        }
        found = False
        for field, sysctl_key in keys.items():
            try:
                out = subprocess.check_output(
                    ["sysctl", "-n", sysctl_key],
                    text=True,
                    stderr=subprocess.DEVNULL,
                ).strip()
                if out:
                    result[field] = int(out)
                    found = True
            except Exception:
                continue
        if found:
            result["source"] = "sysctl"
            return result

    if system == "Linux":
        cache_root = Path("/sys/devices/system/cpu/cpu0/cache")
        found = False
        if cache_root.exists():
            for idx_dir in sorted(cache_root.glob("index*")):
                try:
                    level = (idx_dir / "level").read_text(encoding="utf-8").strip()
                    cache_type = (idx_dir / "type").read_text(encoding="utf-8").strip().lower()
                    size_token = (idx_dir / "size").read_text(encoding="utf-8").strip()
                except Exception:
                    continue
                size_bytes = _parse_cache_size_token(size_token)
                if size_bytes is None:
                    continue
                if level == "1" and cache_type == "data":
                    result["l1d_bytes"] = size_bytes
                    found = True
                elif level == "2":
                    prev = result["l2_bytes"]
                    result["l2_bytes"] = size_bytes if prev is None else max(prev, size_bytes)
                    found = True
                elif level == "3":
                    prev = result["l3_bytes"]
                    result["l3_bytes"] = size_bytes if prev is None else max(prev, size_bytes)
                    found = True
        if found:
            result["source"] = "sysfs"
            return result

        try:
            out = subprocess.check_output(
                ["lscpu"],
                text=True,
                stderr=subprocess.DEVNULL,
            )
            mapping = {
                "l1d cache:": "l1d_bytes",
                "l2 cache:": "l2_bytes",
                "l3 cache:": "l3_bytes",
            }
            parsed = False
            for line in out.splitlines():
                lower = line.strip().lower()
                for prefix, field in mapping.items():
                    if lower.startswith(prefix):
                        value = line.split(":", 1)[1].strip()
                        size_bytes = _parse_cache_size_token(value)
                        if size_bytes is not None:
                            result[field] = size_bytes
                            parsed = True
            if parsed:
                result["source"] = "lscpu"
                return result
        except Exception:
            pass

    return result


def classify_working_set_residency(working_set_bytes: int, cache_info: dict[str, Any]) -> str:
    """
    Przybliżona klasyfikacja dominującego poziomu pamięci dla pointer-chasing.

    To nie jest gwarancja ścisłej lokalizacji danych, tylko sensowny opis
    eksperymentalny użyteczny do analizy wykresów i pracy doktorskiej.
    """
    if working_set_bytes <= 0:
        return "unknown"
    perf_l1 = cache_info.get("perf_l1d_bytes")
    perf_l2 = cache_info.get("perf_l2_bytes")
    eff_l1 = cache_info.get("eff_l1d_bytes")
    eff_l2 = cache_info.get("eff_l2_bytes")
    l3 = cache_info.get("l3_bytes")

    if cache_info.get("cache_model") == "heterogeneous":
        if isinstance(eff_l1, int) and eff_l1 > 0 and working_set_bytes <= eff_l1:
            return "L1(all cores)"
        if (
            isinstance(eff_l1, int)
            and isinstance(perf_l1, int)
            and eff_l1 < working_set_bytes <= perf_l1
        ):
            return "P-L1 / E-L2"
        if isinstance(eff_l2, int) and eff_l2 > 0 and working_set_bytes <= eff_l2:
            return "L2(all cores)"
        if (
            isinstance(eff_l2, int)
            and isinstance(perf_l2, int)
            and eff_l2 < working_set_bytes <= perf_l2
        ):
            return "P-L2 / E-DRAM"
        if isinstance(l3, int) and l3 > 0 and working_set_bytes <= l3:
            return "L3"
        if any(isinstance(v, int) and v > 0 for v in (eff_l1, perf_l1, eff_l2, perf_l2, l3)):
            return "DRAM"

    l1 = cache_info.get("l1d_bytes")
    l2 = cache_info.get("l2_bytes")
    if isinstance(l1, int) and l1 > 0 and working_set_bytes <= l1:
        return "L1"
    if isinstance(l2, int) and l2 > 0 and working_set_bytes <= l2:
        return "L2"
    if isinstance(l3, int) and l3 > 0 and working_set_bytes <= l3:
        return "L3"
    if any(isinstance(v, int) and v > 0 for v in (l1, l2, l3)):
        return "DRAM"
    return "unknown"


def describe_cpu_cache_hierarchy(cache_info: dict[str, Any]) -> str:
    if cache_info.get("cache_model") == "heterogeneous":
        return (
            "Apple heterogeneous cache model: "
            f"E-L1D={cache_info.get('eff_l1d_bytes') or 'n/a'} B, "
            f"P-L1D={cache_info.get('perf_l1d_bytes') or 'n/a'} B, "
            f"E-L2={cache_info.get('eff_l2_bytes') or 'n/a'} B, "
            f"P-L2={cache_info.get('perf_l2_bytes') or 'n/a'} B, "
            f"L3={cache_info.get('l3_bytes') or 'n/a'} B "
            f"(source={cache_info.get('source', 'unknown')})"
        )
    return (
        f"L1D={cache_info.get('l1d_bytes') or 'n/a'} B, "
        f"L2={cache_info.get('l2_bytes') or 'n/a'} B, "
        f"L3={cache_info.get('l3_bytes') or 'n/a'} B "
        f"(source={cache_info.get('source', 'unknown')})"
    )
