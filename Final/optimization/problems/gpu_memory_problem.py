from __future__ import annotations

from dataclasses import dataclass
import math
import platform
import statistics as stats
from typing import Any, Dict, List

from energy_utils import EnergyLogger

from optimization.problem import EvaluationResult, OptimizationProblem
from optimization.search_space import CategoricalVariable, ContinuousVariable, IntegerVariable, SearchSpace

from .gpu_adapters import GpuBackendAdapter, init_gpu_adapter


@dataclass
class GpuMemoryProblemConfig:
    backend: str
    device_index: int = 0
    repeats: int = 3
    min_elapsed_s: float = 1e-4
    max_cv: float = 0.05
    size_mb_min: float = 4.0
    size_mb_max: float = 1024.0
    iters_min: int = 5
    iters_max: int = 200
    transfer_kinds: List[str] | None = None
    hard_size_mb_cap: float | None = None
    hard_iters_cap: int | None = None
    energy_sample_interval_s: float = 0.01
    energy_min_window_s: float = 0.0
    energy_window_max_batches: int = 256


class GpuMemoryProblem(OptimizationProblem):
    """
    FA problem for memory-path tuning:
    decision vars:
      - transfer_kind (categorical)
      - size_mb (continuous)
      - iters_inner (integer)
    metrics:
      gbps_mean, gbps_sigma, cv_gbps, energy_j_mean, power_w_mean, j_per_gb
    """

    def __init__(self, cfg: GpuMemoryProblemConfig) -> None:
        self.cfg = cfg
        self.adapter: GpuBackendAdapter = init_gpu_adapter(cfg.backend, cfg.device_index)

        # Safety caps to avoid OOM / watchdog kills on Apple Metal.
        if self.adapter.backend == "metal":
            if self.cfg.hard_size_mb_cap is None:
                self.cfg.hard_size_mb_cap = 32.0
            if self.cfg.hard_iters_cap is None:
                self.cfg.hard_iters_cap = 128
            # powermetrics ma próbkowanie rzędu 10+ ms; pojedynczy run bywa zbyt krótki.
            if self.cfg.energy_min_window_s <= 0.0 and platform.system() == "Darwin":
                self.cfg.energy_min_window_s = 0.12

        supported = ["device_to_device"]
        if self.adapter.run_mem_h2d is not None:
            supported.append("host_to_device")
        if self.adapter.run_mem_d2h is not None:
            supported.append("device_to_host")

        if cfg.transfer_kinds:
            transfer_kinds = [k for k in cfg.transfer_kinds if k in supported]
            if not transfer_kinds:
                transfer_kinds = supported
        else:
            transfer_kinds = supported

        self._transfer_kinds = transfer_kinds
        self.range_adjustments: List[str] = []

        size_max_effective = cfg.size_mb_max
        if self.cfg.hard_size_mb_cap is not None:
            size_max_effective = min(size_max_effective, float(self.cfg.hard_size_mb_cap))
        size_min_effective = float(cfg.size_mb_min)
        if size_min_effective >= size_max_effective:
            self.range_adjustments.append(
                f"size_mb_min={cfg.size_mb_min} adjusted to <= size_mb_max={size_max_effective}"
            )
            size_min_effective = max(0.0, size_max_effective - 1e-6)

        iters_max_effective = cfg.iters_max
        if self.cfg.hard_iters_cap is not None:
            iters_max_effective = min(iters_max_effective, int(self.cfg.hard_iters_cap))
        iters_min_effective = int(cfg.iters_min)
        if iters_min_effective > iters_max_effective:
            self.range_adjustments.append(
                f"iters_min={cfg.iters_min} adjusted to iters_max={iters_max_effective}"
            )
            iters_min_effective = int(iters_max_effective)

        self.effective_size_mb_min = float(size_min_effective)
        self.effective_size_mb_max = float(size_max_effective)
        self.effective_iters_min = int(iters_min_effective)
        self.effective_iters_max = int(iters_max_effective)

        self._space = SearchSpace(
            [
                CategoricalVariable("transfer_kind", transfer_kinds),
                ContinuousVariable("size_mb", self.effective_size_mb_min, self.effective_size_mb_max),
                IntegerVariable("iters_inner", self.effective_iters_min, self.effective_iters_max, step=1),
            ]
        )

    @property
    def name(self) -> str:
        return f"gpu_memory_{self.adapter.backend}"

    @property
    def search_space(self) -> SearchSpace:
        return self._space

    def _run_once(self, transfer_kind: str, size_bytes: int, iters_inner: int) -> tuple[float, float, float, str, float]:
        logger = EnergyLogger(
            domain="gpu",
            device_index=self.adapter.device_index,
            sample_interval_s=self.cfg.energy_sample_interval_s,
        )
        logger.start()

        elapsed_total = 0.0
        bytes_total = 0.0
        batches = 0
        min_window = max(0.0, float(self.cfg.energy_min_window_s))
        max_batches = max(1, int(self.cfg.energy_window_max_batches))

        while True:
            if transfer_kind == "device_to_device":
                elapsed_s = self.adapter.run_mem_d2d(size_bytes, iters_inner)
                moved = float(2 * size_bytes * iters_inner)
            elif transfer_kind == "host_to_device":
                if self.adapter.run_mem_h2d is None:
                    raise RuntimeError("host_to_device not supported")
                elapsed_s = self.adapter.run_mem_h2d(size_bytes, iters_inner)
                moved = float(size_bytes * iters_inner)
            elif transfer_kind == "device_to_host":
                if self.adapter.run_mem_d2h is None:
                    raise RuntimeError("device_to_host not supported")
                elapsed_s = self.adapter.run_mem_d2h(size_bytes, iters_inner)
                moved = float(size_bytes * iters_inner)
            else:
                raise ValueError(f"Unknown transfer_kind: {transfer_kind}")

            elapsed_total += float(elapsed_s)
            bytes_total += moved
            batches += 1

            if min_window <= 0.0:
                break
            if elapsed_total >= min_window:
                break
            if batches >= max_batches:
                break

        energy_j, power_w = logger.stop()
        gbps = bytes_total / max(elapsed_total, 1e-12) / 1e9
        return elapsed_total, gbps, energy_j, logger.energy_source, power_w

    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        transfer_kind = str(config["transfer_kind"])
        size_mb = float(config["size_mb"])
        iters_inner = int(config["iters_inner"])

        violations: List[str] = []
        if self.cfg.hard_size_mb_cap is not None and size_mb > float(self.cfg.hard_size_mb_cap):
            violations.append(f"size_mb>{self.cfg.hard_size_mb_cap}")
        if self.cfg.hard_iters_cap is not None and iters_inner > int(self.cfg.hard_iters_cap):
            violations.append(f"iters_inner>{self.cfg.hard_iters_cap}")
        if violations:
            return EvaluationResult(
                status="error",
                metrics={
                    "gbps_mean": float("nan"),
                    "cv_gbps": float("nan"),
                },
                constraints_ok=False,
                violations=violations,
            )

        # Build valid numeric runtime params
        size_bytes = int(max(1.0, size_mb) * 1024.0 * 1024.0)
        size_bytes = max(4, size_bytes)
        iters_inner = max(1, iters_inner)

        elapsed_vals: List[float] = []
        gbps_vals: List[float] = []
        energy_vals: List[float] = []
        power_vals: List[float] = []
        sources: List[str] = []

        for _ in range(max(1, self.cfg.repeats)):
            try:
                elapsed_s, gbps, energy_j, src, power_w = self._run_once(transfer_kind, size_bytes, iters_inner)
            except Exception as e:
                return EvaluationResult(
                    status="error",
                    metrics={
                        "gbps_mean": float("nan"),
                        "cv_gbps": float("nan"),
                    },
                    constraints_ok=False,
                    violations=[f"runtime_error:{e}"],
                )

            elapsed_vals.append(elapsed_s)
            gbps_vals.append(gbps)
            sources.append(src)
            if not math.isnan(energy_j):
                energy_vals.append(energy_j)
            if not math.isnan(power_w):
                power_vals.append(power_w)

        gbps_mean = stats.mean(gbps_vals)
        gbps_sigma = stats.pstdev(gbps_vals) if len(gbps_vals) > 1 else 0.0
        cv = gbps_sigma / max(abs(gbps_mean), 1e-12)
        elapsed_mean = stats.mean(elapsed_vals)

        energy_mean = stats.mean(energy_vals) if energy_vals else float("nan")
        power_mean = stats.mean(power_vals) if power_vals else float("nan")
        j_per_gb = float("nan")
        if not math.isnan(energy_mean) and gbps_mean > 0 and elapsed_mean > 0:
            moved_gb = gbps_mean * elapsed_mean
            if moved_gb > 0:
                j_per_gb = energy_mean / moved_gb

        if elapsed_mean < self.cfg.min_elapsed_s:
            violations.append(f"min_elapsed_s<{self.cfg.min_elapsed_s}")
        if cv > self.cfg.max_cv:
            violations.append(f"cv_gbps>{self.cfg.max_cv}")
        if not math.isfinite(gbps_mean) or gbps_mean <= 0:
            violations.append("invalid_gbps")

        metrics = {
            "elapsed_s_mean": elapsed_mean,
            "gbps_mean": gbps_mean,
            "gbps_sigma": gbps_sigma,
            "cv_gbps": cv,
            "energy_j_mean": energy_mean,
            "power_w_mean": power_mean,
            "j_per_gb": j_per_gb,
            "size_bytes": float(size_bytes),
            "iters_inner": float(iters_inner),
        }
        artifacts = {
            "energy_sources": sorted(set(sources)),
            "raw_gbps": gbps_vals,
            "raw_elapsed_s": elapsed_vals,
            "raw_energy_j": energy_vals,
            "raw_power_w": power_vals,
            "device": self.adapter.device_name,
            "backend": self.adapter.backend,
            "transfer_kind": transfer_kind,
        }

        return EvaluationResult(
            status="ok",
            metrics=metrics,
            artifacts=artifacts,
            constraints_ok=(len(violations) == 0),
            violations=violations,
        )
