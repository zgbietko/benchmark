# energy_utils.py
#
# Spójny logger energii/mocy dla benchmarków.
#
# API (backward compatible):
#   from energy_utils import EnergyLogger
#   logger = EnergyLogger()              # domyślnie CPU
#   logger.start()
#   ... run ...
#   energy_j, power_w = logger.stop()
#
# GPU:
#   logger = EnergyLogger(domain="gpu", device_index=0)
#
# Logger próbuje najlepszej dostępnej metody (best-effort).
# Uwaga: dla macOS+GPU fallback zwraca 0 J / 0 W (bez NaN), żeby pipeline
# CSV/analizy był stabilny nawet bez uprawnień do powermetrics.

from __future__ import annotations

import math
import os
import platform
import re
import subprocess
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from shutil import which
from typing import Callable, List, Optional, Tuple


SYSTEM = platform.system()


def _nan_pair() -> Tuple[float, float]:
    return (float("nan"), float("nan"))


# ============================================================
#  CPU backends
# ============================================================

class _CpuDummy:
    source = "unavailable"

    def __init__(self) -> None:
        self._t0: Optional[float] = None

    def start(self) -> None:
        self._t0 = time.perf_counter()

    def stop(self) -> Tuple[float, float]:
        return _nan_pair()


class _CpuRAPL:
    """Linux powercap RAPL (Intel i część platform AMD)."""
    source = "rapl"

    def __init__(self) -> None:
        self._paths = self._find_energy_paths()
        self._e0: Optional[float] = None
        self._t0: Optional[float] = None

    @staticmethod
    def _find_energy_paths() -> List[Path]:
        root = Path("/sys/class/powercap")
        if not root.exists():
            return []
        candidates: List[Path] = []
        for p in root.rglob("energy_uj"):
            # filtr na rapl-like
            s = str(p)
            if "rapl" in s:
                candidates.append(p)
        return sorted(candidates)

    def start(self) -> None:
        self._t0 = time.perf_counter()
        self._e0 = self._read_total_joules()

    def _read_total_joules(self) -> float:
        total_uj = 0
        for p in self._paths:
            try:
                total_uj += int(p.read_text().strip())
            except Exception:
                continue
        return total_uj / 1e6  # uJ -> J

    def stop(self) -> Tuple[float, float]:
        if self._e0 is None or self._t0 is None:
            return _nan_pair()
        e1 = self._read_total_joules()
        t1 = time.perf_counter()
        energy_j = e1 - self._e0
        dt = max(t1 - self._t0, 1e-12)
        power_w = energy_j / dt
        if energy_j < 0:
            # overflow / reset – best-effort: NaN
            return _nan_pair()
        return energy_j, power_w


class _CpuPowermetrics:
    """macOS: powermetrics (wymaga sudo)."""
    source = "powermetrics"

    def __init__(self, sample_interval_s: float = 0.1) -> None:
        self.sample_interval_s = sample_interval_s
        self._proc: Optional[subprocess.Popen] = None
        self._buf: List[str] = []
        self._thread: Optional[threading.Thread] = None
        self._stop_flag = False
        self._t0: Optional[float] = None

    def _reader(self, stream) -> None:
        while not self._stop_flag:
            line = stream.readline()
            if not line:
                break
            try:
                self._buf.append(line.decode("utf-8", errors="replace"))
            except Exception:
                pass

    def start(self) -> None:
        if which("powermetrics") is None:
            raise RuntimeError("powermetrics not found")
        self._t0 = time.perf_counter()
        self._buf = []
        self._stop_flag = False

        # Minimalny tryb: próbkuj CPU power; nie wymuszamy domen.
        cmd = [
            "sudo",
            "powermetrics",
            "--samplers",
            "cpu_power",
            "-i",
            str(int(self.sample_interval_s * 1000)),
        ]

        self._proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        assert self._proc.stdout is not None
        self._thread = threading.Thread(target=self._reader, args=(self._proc.stdout,), daemon=True)
        self._thread.start()

    @staticmethod
    def _parse_avg_cpu_power_w(text: str) -> Optional[float]:
        # powermetrics różni się wersjami; próbujemy kilku wzorców.
        # Szukamy np. "CPU Power: 8.12 W" lub "Average power: 8.12 W"
        patterns = [
            r"CPU Power:\s*([0-9]+\.?[0-9]*)\s*W",
            r"Average power:\s*([0-9]+\.?[0-9]*)\s*W",
        ]
        vals: List[float] = []
        for pat in patterns:
            for m in re.finditer(pat, text):
                try:
                    vals.append(float(m.group(1)))
                except Exception:
                    pass
        if not vals:
            return None
        return sum(vals) / len(vals)

    def stop(self) -> Tuple[float, float]:
        if self._proc is None or self._t0 is None:
            return _nan_pair()

        self._stop_flag = True
        try:
            self._proc.terminate()
        except Exception:
            pass

        try:
            self._proc.wait(timeout=2.0)
        except Exception:
            pass

        if self._thread is not None:
            try:
                self._thread.join(timeout=1.0)
            except Exception:
                pass

        t1 = time.perf_counter()
        dt = max(t1 - self._t0, 1e-12)

        text = "".join(self._buf)
        p = self._parse_avg_cpu_power_w(text)
        if p is None:
            return _nan_pair()
        energy = p * dt
        return energy, p


def _select_cpu_backend(sample_interval_s: float) -> object:
    if SYSTEM == "Linux":
        rapl = _CpuRAPL()
        if rapl._paths:
            return rapl
        return _CpuDummy()
    if SYSTEM == "Darwin":
        try:
            return _CpuPowermetrics(sample_interval_s=sample_interval_s)
        except Exception:
            return _CpuDummy()
    return _CpuDummy()


# ============================================================
#  GPU backends
# ============================================================

@dataclass
class _Samples:
    ts: List[float]
    power_w: List[float]

    def quality(self) -> dict:
        count = len(self.power_w)
        nan_count = sum(1 for p in self.power_w if math.isnan(p))
        duration_s = 0.0
        if len(self.ts) >= 2:
            duration_s = max(self.ts[-1] - self.ts[0], 0.0)
        nan_ratio = (nan_count / count) if count > 0 else 1.0
        confidence = max(0.0, min(1.0, 1.0 - nan_ratio))
        return {
            "sample_count": count,
            "nan_sample_count": nan_count,
            "duration_s": duration_s,
            "nan_ratio": nan_ratio,
            "confidence": confidence,
        }


class _GpuSamplerThread:
    def __init__(self, read_power_w: Callable[[], float], sample_interval_s: float) -> None:
        self.read_power_w = read_power_w
        self.sample_interval_s = sample_interval_s
        self.samples = _Samples(ts=[], power_w=[])
        self._stop = False
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop = True
        self._thread.join(timeout=2.0)

    def _run(self) -> None:
        t0 = time.perf_counter()
        # próbkuj do momentu stop
        while not self._stop:
            t = time.perf_counter() - t0
            try:
                p = float(self.read_power_w())
            except Exception:
                p = float("nan")
            self.samples.ts.append(t)
            self.samples.power_w.append(p)
            time.sleep(self.sample_interval_s)


def _integrate_energy(samples: _Samples) -> Tuple[float, float]:
    # Trapezoidal integration; ignore NaNs.
    ts = samples.ts
    pw = samples.power_w
    if len(ts) < 2:
        return _nan_pair()
    energy = 0.0
    valid = 0
    for i in range(1, len(ts)):
        t0, t1 = ts[i-1], ts[i]
        p0, p1 = pw[i-1], pw[i]
        if math.isnan(p0) or math.isnan(p1):
            continue
        dt = t1 - t0
        if dt <= 0:
            continue
        energy += 0.5 * (p0 + p1) * dt
        valid += 1
    if valid == 0:
        return _nan_pair()
    total_time = ts[-1] - ts[0]
    total_time = max(total_time, 1e-12)
    avg_p = energy / total_time
    return energy, avg_p


class _GpuDummy:
    def __init__(self, source: str = "unavailable") -> None:
        self.source = source
        self.sample_interval_s = float("nan")

    def start(self) -> None:
        pass

    def stop(self) -> Tuple[float, float]:
        return _nan_pair()

    def get_quality_metrics(self) -> dict:
        return {
            "sample_count": 0,
            "nan_sample_count": 0,
            "duration_s": 0.0,
            "nan_ratio": 1.0,
            "confidence": 0.0,
        }


class _GpuZeroFallback:
    """
    Deterministyczny fallback bez NaN.
    Zwraca 0 J / 0 W, gdy pomiar GPU nie może wystartować.
    """

    def __init__(self, source: str = "fallback_zero") -> None:
        self.source = source
        self.sample_interval_s = float("nan")

    def start(self) -> None:
        pass

    def stop(self) -> Tuple[float, float]:
        return 0.0, 0.0

    def get_quality_metrics(self) -> dict:
        return {
            "sample_count": 0,
            "nan_sample_count": 0,
            "duration_s": 0.0,
            "nan_ratio": 0.0,
            "confidence": 0.0,
        }


class _GpuPowermetricsMac:
    """
    macOS GPU power via powermetrics.
    Best-effort parser:
    - prefer direct GPU power lines,
    - fallback to (total - cpu - ane) estimate,
    - final fallback to 0.0 W (never NaN) with low confidence.
    """

    source = "powermetrics_gpu"

    def __init__(self, sample_interval_s: float) -> None:
        self.sample_interval_s = sample_interval_s
        self._proc: Optional[subprocess.Popen] = None
        self._thread: Optional[threading.Thread] = None
        self._stop_flag = False
        self._t0: Optional[float] = None

        self._samples = _Samples(ts=[], power_w=[])
        self._buf: List[str] = []

        # Running estimates for fallback GPU estimate = total - cpu - ane.
        self._last_cpu_w: Optional[float] = None
        self._last_ane_w: Optional[float] = None
        self._last_total_w: Optional[float] = None

    @staticmethod
    def _to_w(value: float, unit: str) -> float:
        u = unit.strip().lower()
        if u == "mw":
            return float(value) / 1000.0
        return float(value)

    @classmethod
    def _extract_named_power_w(cls, line: str, name: str) -> Optional[float]:
        # Examples matched:
        #  "GPU Power: 1.23 W"
        #  "CPU Power: 1534 mW"
        pat = rf"{name}\s*Power[^:]*:\s*([0-9]+(?:\.[0-9]+)?)\s*(m?W)"
        m = re.search(pat, line, flags=re.IGNORECASE)
        if not m:
            return None
        return cls._to_w(float(m.group(1)), m.group(2))

    @classmethod
    def _extract_total_power_w(cls, line: str) -> Optional[float]:
        patterns = [
            r"(?:System|Total|Combined)\s*Power[^:]*:\s*([0-9]+(?:\.[0-9]+)?)\s*(m?W)",
            r"Package\s*Power[^:]*:\s*([0-9]+(?:\.[0-9]+)?)\s*(m?W)",
        ]
        for pat in patterns:
            m = re.search(pat, line, flags=re.IGNORECASE)
            if m:
                return cls._to_w(float(m.group(1)), m.group(2))
        return None

    def _try_start_proc(self) -> None:
        if which("powermetrics") is None:
            raise RuntimeError("powermetrics not found")

        interval_ms = str(max(10, int(self.sample_interval_s * 1000)))
        # Prefer dedicated GPU sampler; fallback to broader samplers.
        # sudo -n: fail-fast bez interaktywnego pytania o hasło
        # (benchmarki zwykle uruchamiane z subprocess bez TTY).
        cmd_candidates = [
            ["sudo", "-n", "powermetrics", "--samplers", "gpu_power", "-i", interval_ms],
            ["sudo", "-n", "powermetrics", "--samplers", "cpu_power,gpu_power", "-i", interval_ms],
            ["sudo", "-n", "powermetrics", "--samplers", "smc", "-i", interval_ms],
            ["sudo", "-n", "powermetrics", "--samplers", "all", "-i", interval_ms],
            ["sudo", "-n", "powermetrics", "-i", interval_ms],
        ]
        for cmd in cmd_candidates:
            try:
                proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                # Jeśli proces kończy się natychmiast (np. zły sampler), próbuj kolejny wariant.
                time.sleep(0.05)
                if proc.poll() is not None:
                    continue
                self._proc = proc
                return
            except Exception:
                continue
        raise RuntimeError("failed to start powermetrics process for GPU")

    def _append_sample(self, power_w: float) -> None:
        if self._t0 is None:
            return
        t = max(0.0, time.perf_counter() - self._t0)
        self._samples.ts.append(t)
        self._samples.power_w.append(power_w)

    def _reader(self, stream) -> None:
        while not self._stop_flag:
            line_b = stream.readline()
            if not line_b:
                break
            try:
                line = line_b.decode("utf-8", errors="replace")
            except Exception:
                continue

            self._buf.append(line)

            gpu_w = self._extract_named_power_w(line, "GPU")
            if gpu_w is not None:
                self._append_sample(gpu_w)
                continue

            cpu_w = self._extract_named_power_w(line, "CPU")
            if cpu_w is not None:
                self._last_cpu_w = cpu_w

            ane_w = self._extract_named_power_w(line, "ANE")
            if ane_w is not None:
                self._last_ane_w = ane_w

            total_w = self._extract_total_power_w(line)
            if total_w is not None:
                self._last_total_w = total_w
                # Fallback estimate when direct GPU line is unavailable.
                cpu = self._last_cpu_w if self._last_cpu_w is not None else 0.0
                ane = self._last_ane_w if self._last_ane_w is not None else 0.0
                est_gpu = max(0.0, total_w - cpu - ane)
                self._append_sample(est_gpu)

    def _parse_fallback_avg_gpu_w(self) -> float:
        vals: List[float] = []
        for line in self._buf:
            v = self._extract_named_power_w(line, "GPU")
            if v is not None:
                vals.append(v)
        if vals:
            return sum(vals) / len(vals)

        # Estimate from total-cpu-ane over full buffer if direct GPU not present.
        cpu_vals: List[float] = []
        ane_vals: List[float] = []
        total_vals: List[float] = []
        for line in self._buf:
            c = self._extract_named_power_w(line, "CPU")
            if c is not None:
                cpu_vals.append(c)
            a = self._extract_named_power_w(line, "ANE")
            if a is not None:
                ane_vals.append(a)
            t = self._extract_total_power_w(line)
            if t is not None:
                total_vals.append(t)
        if total_vals:
            c_mu = (sum(cpu_vals) / len(cpu_vals)) if cpu_vals else 0.0
            a_mu = (sum(ane_vals) / len(ane_vals)) if ane_vals else 0.0
            t_mu = sum(total_vals) / len(total_vals)
            return max(0.0, t_mu - c_mu - a_mu)

        # Never return NaN on macOS GPU path.
        return 0.0

    def start(self) -> None:
        self._samples = _Samples(ts=[], power_w=[])
        self._buf = []
        self._last_cpu_w = None
        self._last_ane_w = None
        self._last_total_w = None
        self._stop_flag = False
        self._t0 = time.perf_counter()

        self._try_start_proc()
        if self._proc is None or self._proc.stdout is None:
            raise RuntimeError("powermetrics process unavailable")

        self._thread = threading.Thread(target=self._reader, args=(self._proc.stdout,), daemon=True)
        self._thread.start()

    def stop(self) -> Tuple[float, float]:
        if self._t0 is None:
            return 0.0, 0.0

        self._stop_flag = True

        if self._proc is not None:
            try:
                self._proc.terminate()
            except Exception:
                pass
            try:
                self._proc.wait(timeout=2.0)
            except Exception:
                pass

        if self._thread is not None:
            try:
                self._thread.join(timeout=1.0)
            except Exception:
                pass

        energy_j, avg_p = _integrate_energy(self._samples)
        if not (math.isnan(energy_j) or math.isnan(avg_p)):
            return energy_j, avg_p

        dt = max(time.perf_counter() - self._t0, 1e-12)
        p = self._parse_fallback_avg_gpu_w()
        return max(0.0, p * dt), max(0.0, p)

    def get_quality_metrics(self) -> dict:
        q = self._samples.quality()
        if q.get("sample_count", 0) == 0:
            # mark explicit low-confidence fallback
            q["confidence"] = 0.0
            q["nan_ratio"] = 0.0
        return q


class _GpuNVML:
    source = "nvml"

    def __init__(self, device_index: int, sample_interval_s: float) -> None:
        self.device_index = device_index
        self.sample_interval_s = sample_interval_s
        self._sampler: Optional[_GpuSamplerThread] = None
        self._t0: Optional[float] = None

        import pynvml  # type: ignore

        self.pynvml = pynvml
        pynvml.nvmlInit()
        self.handle = pynvml.nvmlDeviceGetHandleByIndex(device_index)

    def _read_power_w(self) -> float:
        # mW -> W
        mw = self.pynvml.nvmlDeviceGetPowerUsage(self.handle)
        return float(mw) / 1000.0

    def start(self) -> None:
        self._t0 = time.perf_counter()
        self._sampler = _GpuSamplerThread(self._read_power_w, self.sample_interval_s)
        self._sampler.start()

    def stop(self) -> Tuple[float, float]:
        if self._sampler is None or self._t0 is None:
            return _nan_pair()
        self._sampler.stop()
        energy_j, avg_p = _integrate_energy(self._sampler.samples)
        return energy_j, avg_p

    def get_quality_metrics(self) -> dict:
        if self._sampler is None:
            return _GpuDummy().get_quality_metrics()
        return self._sampler.samples.quality()


class _GpuNvidiaSmi:
    source = "nvidia_smi"

    def __init__(self, device_index: int, sample_interval_s: float) -> None:
        self.device_index = device_index
        self.sample_interval_s = sample_interval_s
        self._sampler: Optional[_GpuSamplerThread] = None

    def _read_power_w(self) -> float:
        cmd = [
            "nvidia-smi",
            "--query-gpu=power.draw",
            "--format=csv,noheader,nounits",
            "-i",
            str(self.device_index),
        ]
        try:
            out = subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL).strip()
        except Exception:
            return float("nan")

        # Zwykle pojedyncza linia, ale defensywnie bierzemy pierwszą niepustą.
        for line in out.splitlines():
            s = line.strip()
            if not s:
                continue
            try:
                return float(s)
            except ValueError:
                continue
        return float("nan")

    def start(self) -> None:
        self._sampler = _GpuSamplerThread(self._read_power_w, self.sample_interval_s)
        self._sampler.start()

    def stop(self) -> Tuple[float, float]:
        if self._sampler is None:
            return _nan_pair()
        self._sampler.stop()
        return _integrate_energy(self._sampler.samples)

    def get_quality_metrics(self) -> dict:
        if self._sampler is None:
            return _GpuDummy().get_quality_metrics()
        return self._sampler.samples.quality()


class _GpuSysfsHwmon:
    source = "sysfs"

    def __init__(self, device_index: int, sample_interval_s: float, drm_card_index: Optional[int] = None) -> None:
        self.device_index = device_index
        self.sample_interval_s = sample_interval_s
        self.drm_card_index = drm_card_index
        self._sampler: Optional[_GpuSamplerThread] = None
        self._power_path = self._select_power_path()

    @staticmethod
    def _candidate_power_paths() -> List[Tuple[int, Path]]:
        # Returns list of (card_index, power_path)
        out: List[Tuple[int, Path]] = []
        drm = Path("/sys/class/drm")
        if not drm.exists():
            return out
        for card in sorted(drm.glob("card[0-9]*")):
            m = re.match(r"card(\d+)$", card.name)
            if not m:
                continue
            idx = int(m.group(1))
            hwmon_root = card / "device" / "hwmon"
            if not hwmon_root.exists():
                continue
            for hwmon in sorted(hwmon_root.glob("hwmon*")):
                # prefer power1_average, then power1_input
                for pat in ("power*_average", "power*_input"):
                    for p in sorted(hwmon.glob(pat)):
                        if p.is_file():
                            out.append((idx, p))
                            break
        return out

    def _select_power_path(self) -> Optional[Path]:
        candidates = self._candidate_power_paths()
        if not candidates:
            return None

        # Jeśli user podał drm_card_index – wybierz dokładnie tę kartę
        if self.drm_card_index is not None:
            for idx, p in candidates:
                if idx == self.drm_card_index:
                    return p

        # Jeśli tylko jedna karta z power – bierzemy ją
        unique_cards = sorted({idx for idx, _ in candidates})
        if len(unique_cards) == 1:
            # pierwsza ścieżka dla tej karty
            for idx, p in candidates:
                if idx == unique_cards[0]:
                    return p

        # Heurystyka: device_index jako indeks karty (jeśli istnieje)
        wanted = self.device_index
        for idx, p in candidates:
            if idx == wanted:
                return p

        # fallback: weź pierwszą
        return candidates[0][1]

    @staticmethod
    def _convert_to_w(raw: float) -> float:
        # sysfs często raportuje uW albo mW; heurystyka skali
        if raw > 1e6:
            return raw / 1e6  # uW
        if raw > 1e3:
            return raw / 1e3  # mW
        return raw  # W

    def _read_power_w(self) -> float:
        if self._power_path is None:
            return float("nan")
        raw = float(self._power_path.read_text().strip())
        return self._convert_to_w(raw)

    def start(self) -> None:
        if self._power_path is None:
            raise RuntimeError("No sysfs GPU power path found")
        self._sampler = _GpuSamplerThread(self._read_power_w, self.sample_interval_s)
        self._sampler.start()

    def stop(self) -> Tuple[float, float]:
        if self._sampler is None:
            return _nan_pair()
        self._sampler.stop()
        return _integrate_energy(self._sampler.samples)

    def get_quality_metrics(self) -> dict:
        if self._sampler is None:
            return _GpuDummy().get_quality_metrics()
        return self._sampler.samples.quality()


def _select_gpu_backend(device_index: int, sample_interval_s: float, drm_card_index: Optional[int]) -> object:
    if SYSTEM == "Darwin":
        try:
            return _GpuPowermetricsMac(sample_interval_s=sample_interval_s)
        except Exception:
            return _GpuZeroFallback(source="unsupported_macos_gpu_energy_zero")
    if SYSTEM != "Linux":
        return _GpuDummy(source=f"unsupported_{SYSTEM.lower()}_gpu_energy")

    # 1) NVML (NVIDIA) - preferowane źródło na Linux + NVIDIA
    try:
        import pynvml  # noqa: F401
        return _GpuNVML(device_index=device_index, sample_interval_s=sample_interval_s)
    except Exception:
        pass

    # 2) nvidia-smi fallback (gdy brak pythonowego NVML)
    if which("nvidia-smi") is not None:
        return _GpuNvidiaSmi(device_index=device_index, sample_interval_s=sample_interval_s)

    # 3) sysfs/hwmon (często AMD i część iGPU)
    try:
        backend = _GpuSysfsHwmon(
            device_index=device_index, sample_interval_s=sample_interval_s, drm_card_index=drm_card_index
        )
        if backend._power_path is not None:
            return backend
    except Exception:
        pass

    return _GpuDummy(source="no_gpu_energy_backend")


# ============================================================
#  Public API
# ============================================================

class EnergyLogger:
    """
    Logger energii/mocy.
    - domain="cpu" lub "gpu"
    - device_index: GPU index (dla NVML i sysfs heurystyk)
    - sample_interval_s: okres próbkowania dla samplerów
    - drm_card_index: (opcjonalnie) jawny index cardX dla sysfs
    """

    def __init__(
        self,
        domain: str = "cpu",
        device_index: int = 0,
        sample_interval_s: float = 0.05,
        drm_card_index: Optional[int] = None,
    ) -> None:
        self.domain = domain
        self.device_index = device_index
        self.sample_interval_s = sample_interval_s
        self.drm_card_index = drm_card_index

        self.energy_source: str = "unavailable"
        self.energy_available: bool = False
        self.last_quality_metrics: dict = {
            "sample_count": 0,
            "nan_sample_count": 0,
            "duration_s": 0.0,
            "nan_ratio": 1.0,
            "confidence": 0.0,
        }
        self._backend = self._init_backend()

    def _init_backend(self):
        if self.domain == "gpu":
            b = _select_gpu_backend(
                device_index=self.device_index,
                sample_interval_s=self.sample_interval_s,
                drm_card_index=self.drm_card_index,
            )
            self.energy_source = getattr(b, "source", "unavailable")
            self.energy_available = self.energy_source not in {
                "unavailable",
                "no_gpu_energy_backend",
            } and not self.energy_source.startswith("unsupported_")
            return b

        b = _select_cpu_backend(sample_interval_s=self.sample_interval_s)
        self.energy_source = getattr(b, "source", "unavailable")
        self.energy_available = self.energy_source != "unavailable"
        return b

    def start(self) -> None:
        try:
            self._backend.start()
        except Exception:
            # Dla macOS+GPU trzymamy twardą gwarancję: brak NaN w energiach.
            if self.domain == "gpu" and SYSTEM == "Darwin":
                self._backend = _GpuZeroFallback(source=f"{self.energy_source}_start_failed_zero")
                self.energy_source = getattr(self._backend, "source", "fallback_zero")
                self.energy_available = False
                self._backend.start()
                return
            raise

    def stop(self) -> Tuple[float, float]:
        # stop może rzucić RuntimeError (np. brak sysfs); mapujemy na NaN
        try:
            energy_j, power_w = self._backend.stop()
            if hasattr(self._backend, "get_quality_metrics"):
                try:
                    self.last_quality_metrics = self._backend.get_quality_metrics()
                except Exception:
                    pass
            else:
                self.last_quality_metrics = {
                    "sample_count": 0,
                    "nan_sample_count": 0,
                    "duration_s": 0.0,
                    "nan_ratio": 1.0,
                    "confidence": 0.0,
                }
            return energy_j, power_w
        except Exception:
            if self.domain == "gpu" and SYSTEM == "Darwin":
                self.energy_source = f"{self.energy_source}_stop_failed_zero"
                self.energy_available = False
                self.last_quality_metrics = {
                    "sample_count": 0,
                    "nan_sample_count": 0,
                    "duration_s": 0.0,
                    "nan_ratio": 0.0,
                    "confidence": 0.0,
                }
                return 0.0, 0.0
            self.energy_source = "unavailable"
            self.energy_available = False
            self.last_quality_metrics = {
                "sample_count": 0,
                "nan_sample_count": 0,
                "duration_s": 0.0,
                "nan_ratio": 1.0,
                "confidence": 0.0,
            }
            return _nan_pair()

    def metadata(self) -> dict:
        return {
            "domain": self.domain,
            "energy_source": self.energy_source,
            "energy_available": self.energy_available,
            "sample_interval_s": self.sample_interval_s,
            "device_index": self.device_index,
            "drm_card_index": self.drm_card_index,
            "last_quality_metrics": self.last_quality_metrics,
        }


def gpu_energy_capabilities(
    device_index: int = 0,
    sample_interval_s: float = 0.05,
    drm_card_index: Optional[int] = None,
) -> dict:
    logger = EnergyLogger(
        domain="gpu",
        device_index=device_index,
        sample_interval_s=sample_interval_s,
        drm_card_index=drm_card_index,
    )
    return logger.metadata()
