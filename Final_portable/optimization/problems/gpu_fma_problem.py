from __future__ import annotations

from dataclasses import dataclass
import math
import statistics as stats
from typing import Any, Dict, List

from energy_utils import EnergyLogger

from optimization.problem import EvaluationResult, OptimizationProblem
from optimization.search_space import ContinuousVariable, IntegerVariable, SearchSpace

from .gpu_adapters import GpuBackendAdapter, init_gpu_adapter


@dataclass
class GpuFmaProblemConfig:
    backend: str
    device_index: int = 0
    repeats: int = 3
    min_elapsed_s: float = 1e-4
    max_cv: float = 0.05
    n_elements_m_min: float = 0.25
    n_elements_m_max: float = 16.0
    iters_inner_min: int = 200
    iters_inner_max: int = 10_000
    roofline_peak_gflops: float | None = None
    roofline_peak_bw_gbps: float | None = None
    arithmetic_intensity: float | None = None


class GpuFmaProblem(OptimizationProblem):
    """
    FA problem for FMA tuning:
    decision vars:
      - n_elements_m (continuous, in millions)
      - iters_inner (integer)
    metrics:
      gflops_mean, cv_gflops, energy_j_mean, j_per_gflop, edp, roofline_gap
    """

    def __init__(self, cfg: GpuFmaProblemConfig) -> None:
        self.cfg = cfg
        self.adapter: GpuBackendAdapter = init_gpu_adapter(cfg.backend, cfg.device_index)
        if self.adapter.run_fma is None:
            raise RuntimeError(f"Backend {cfg.backend} does not support FMA benchmark")

        self._space = SearchSpace(
            [
                ContinuousVariable("n_elements_m", cfg.n_elements_m_min, cfg.n_elements_m_max),
                IntegerVariable("iters_inner", cfg.iters_inner_min, cfg.iters_inner_max, step=1),
            ]
        )

    @property
    def name(self) -> str:
        return f"gpu_fma_{self.adapter.backend}"

    @property
    def search_space(self) -> SearchSpace:
        return self._space

    def _run_once(self, n_elements: int, iters_inner: int) -> tuple[float, float, float, str, float]:
        logger = EnergyLogger(domain="gpu", device_index=self.adapter.device_index)
        logger.start()
        elapsed_s = self.adapter.run_fma(n_elements, iters_inner)
        energy_j, power_w = logger.stop()

        total_flops = 2.0 * float(n_elements) * float(iters_inner)
        gflops = total_flops / max(elapsed_s, 1e-12) / 1e9
        return elapsed_s, gflops, energy_j, logger.energy_source, power_w

    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        n_elements_m = float(config["n_elements_m"])
        iters_inner = int(config["iters_inner"])

        n_elements = int(max(1.0, n_elements_m * 1_000_000.0))
        iters_inner = max(1, iters_inner)

        elapsed_vals: List[float] = []
        gflops_vals: List[float] = []
        energy_vals: List[float] = []
        power_vals: List[float] = []
        sources: List[str] = []

        for _ in range(max(1, self.cfg.repeats)):
            try:
                elapsed_s, gflops, energy_j, src, power_w = self._run_once(n_elements, iters_inner)
            except Exception as e:
                return EvaluationResult(
                    status="error",
                    metrics={
                        "gflops_mean": float("nan"),
                        "cv_gflops": float("nan"),
                    },
                    constraints_ok=False,
                    violations=[f"runtime_error:{e}"],
                )

            elapsed_vals.append(elapsed_s)
            gflops_vals.append(gflops)
            sources.append(src)
            if not math.isnan(energy_j):
                energy_vals.append(energy_j)
            if not math.isnan(power_w):
                power_vals.append(power_w)

        gflops_mean = stats.mean(gflops_vals)
        gflops_sigma = stats.pstdev(gflops_vals) if len(gflops_vals) > 1 else 0.0
        cv = gflops_sigma / max(abs(gflops_mean), 1e-12)
        elapsed_mean = stats.mean(elapsed_vals)

        energy_mean = stats.mean(energy_vals) if energy_vals else float("nan")
        power_mean = stats.mean(power_vals) if power_vals else float("nan")

        j_per_gflop = float("nan")
        if not math.isnan(energy_mean) and gflops_mean > 0 and elapsed_mean > 0:
            total_gflop = gflops_mean * elapsed_mean
            if total_gflop > 0:
                j_per_gflop = energy_mean / total_gflop

        edp = float("nan")
        if not math.isnan(energy_mean):
            edp = energy_mean * elapsed_mean

        roofline_attainable = float("nan")
        roofline_gap_abs = float("nan")
        if (
            self.cfg.roofline_peak_gflops is not None
            and self.cfg.roofline_peak_bw_gbps is not None
            and self.cfg.arithmetic_intensity is not None
        ):
            roofline_attainable = min(
                float(self.cfg.roofline_peak_gflops),
                float(self.cfg.roofline_peak_bw_gbps) * float(self.cfg.arithmetic_intensity),
            )
            roofline_gap_abs = abs(gflops_mean - roofline_attainable)

        violations: List[str] = []
        if elapsed_mean < self.cfg.min_elapsed_s:
            violations.append(f"min_elapsed_s<{self.cfg.min_elapsed_s}")
        if cv > self.cfg.max_cv:
            violations.append(f"cv_gflops>{self.cfg.max_cv}")
        if not math.isfinite(gflops_mean) or gflops_mean <= 0:
            violations.append("invalid_gflops")

        metrics = {
            "elapsed_s_mean": elapsed_mean,
            "gflops_mean": gflops_mean,
            "gflops_sigma": gflops_sigma,
            "cv_gflops": cv,
            "energy_j_mean": energy_mean,
            "power_w_mean": power_mean,
            "j_per_gflop": j_per_gflop,
            "edp": edp,
            "roofline_attainable_gflops": roofline_attainable,
            "roofline_gap_abs": roofline_gap_abs,
            "n_elements": float(n_elements),
            "iters_inner": float(iters_inner),
        }
        artifacts = {
            "energy_sources": sorted(set(sources)),
            "raw_gflops": gflops_vals,
            "raw_elapsed_s": elapsed_vals,
            "raw_energy_j": energy_vals,
            "raw_power_w": power_vals,
            "device": self.adapter.device_name,
            "backend": self.adapter.backend,
        }

        return EvaluationResult(
            status="ok",
            metrics=metrics,
            artifacts=artifacts,
            constraints_ok=(len(violations) == 0),
            violations=violations,
        )
