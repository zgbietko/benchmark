from __future__ import annotations

from dataclasses import dataclass
import math
import platform
import statistics as stats
from typing import Any, Dict, List

from energy_utils import EnergyLogger

from optimization.problem import EvaluationResult, OptimizationProblem
from optimization.search_space import CategoricalVariable, IntegerVariable, SearchSpace


_SUPPORTED_ELEMENT_TYPES = ("tet4", "hex8")
_SUPPORTED_OPERATORS = (
    "diffusion",
    "mass",
    "convection",
    "diffusion_mass",
    "diffusion_convection_mass",
)
_SUPPORTED_DTYPES = ("float32", "float64")


@dataclass
class FemIntegrationProblemConfig:
    backend: str
    device_index: int = 0
    repeats: int = 3
    min_elapsed_s: float = 1e-4
    max_cv: float = 0.05

    n_elements_min: int = 20_000
    n_elements_max: int = 1_000_000
    n_qp_min: int = 1
    n_qp_max: int = 8

    element_types: List[str] | None = None
    operators: List[str] | None = None
    dtypes: List[str] | None = None

    energy_sample_interval_s: float = 0.01
    energy_min_window_s: float = 0.0
    energy_window_max_batches: int = 256


class FemIntegrationProblem(OptimizationProblem):
    """
    FA problem for real-kernel FEM integration tuning.

    Decision vars:
      - n_elements (integer)
      - n_qp (integer)
      - element_type (categorical)
      - operator (categorical)
      - dtype (categorical)

    Metrics:
      elapsed_s_mean, gflops_mean, gbps_mean, cv_gflops, cv_gbps,
      energy_j_mean, power_w_mean, j_per_gflop, j_per_gb, edp
    """

    def __init__(self, cfg: FemIntegrationProblemConfig) -> None:
        self.cfg = cfg
        self.backend = cfg.backend.lower().strip()
        self.device_index = int(cfg.device_index)

        self._runner, self._energy_domain, self.resolved_backend = self._init_runner(self.backend)

        # powermetrics/NVML sampling may require a slightly longer measurement window.
        if self.cfg.energy_min_window_s <= 0.0 and platform.system() == "Darwin":
            self.cfg.energy_min_window_s = 0.12

        self.device_name = getattr(self._runner, "device_name", self.backend)

        element_types = self._sanitize_list(
            cfg.element_types or list(_SUPPORTED_ELEMENT_TYPES),
            allowed=_SUPPORTED_ELEMENT_TYPES,
            fallback=list(_SUPPORTED_ELEMENT_TYPES),
        )
        operators = self._sanitize_list(
            cfg.operators or list(_SUPPORTED_OPERATORS),
            allowed=_SUPPORTED_OPERATORS,
            fallback=list(_SUPPORTED_OPERATORS),
        )
        dtypes = self._sanitize_list(
            cfg.dtypes or ["float32"],
            allowed=_SUPPORTED_DTYPES,
            fallback=["float32"],
        )

        self._element_types = element_types
        self._operators = operators
        self._dtypes = dtypes

        n_elements_min = max(1, int(cfg.n_elements_min))
        n_elements_max = max(n_elements_min, int(cfg.n_elements_max))
        n_qp_min = max(1, int(cfg.n_qp_min))
        n_qp_max = max(n_qp_min, int(cfg.n_qp_max))

        self.effective_n_elements_min = n_elements_min
        self.effective_n_elements_max = n_elements_max
        self.effective_n_qp_min = n_qp_min
        self.effective_n_qp_max = n_qp_max

        self._space = SearchSpace(
            [
                IntegerVariable("n_elements", n_elements_min, n_elements_max, step=1),
                IntegerVariable("n_qp", n_qp_min, n_qp_max, step=1),
                CategoricalVariable("element_type", element_types),
                CategoricalVariable("operator", operators),
                CategoricalVariable("dtype", dtypes),
            ]
        )

    def _mapped_caps(self, backend: str) -> tuple[int, int, int, int]:
        if backend in ("metal", "opencl"):
            return (1_000_000, 64, 128, 4096)
        return (4_000_000, 128, 256, 10_000)

    def _init_runner(self, requested_backend: str):
        b = requested_backend

        if b == "cpu":
            from real_kernels.cpu_backend import CpuRealBackend

            return CpuRealBackend(), "cpu", "cpu"

        if b == "cuda":
            from real_kernels.cuda_backend import CudaRealBackend

            return CudaRealBackend(device_index=self.device_index), "gpu", "cuda"

        if b == "metal":
            from real_kernels.metal_backend import MetalRealBackend

            return MetalRealBackend(device_index=self.device_index), "gpu", "metal"

        if b == "hip":
            from real_kernels.hip_backend import HipRealBackend

            n_fma, buf_mb, mem_iters, inner_iters = self._mapped_caps("hip")
            return (
                HipRealBackend(
                    device_index=self.device_index,
                    max_n_fma=n_fma,
                    max_buffer_mb=buf_mb,
                    max_mem_iters=mem_iters,
                    max_inner_iters=inner_iters,
                ),
                "gpu",
                "hip",
            )

        if b == "opencl":
            from real_kernels.opencl_backend import OpenCLRealBackend

            n_fma, buf_mb, mem_iters, inner_iters = self._mapped_caps("opencl")
            return (
                OpenCLRealBackend(
                    device_index=self.device_index,
                    max_n_fma=n_fma,
                    max_buffer_mb=buf_mb,
                    max_mem_iters=mem_iters,
                    max_inner_iters=inner_iters,
                ),
                "gpu",
                "opencl",
            )

        if b == "amd":
            try:
                return self._init_runner("hip")
            except Exception:
                return self._init_runner("opencl")

        if b == "intel":
            return self._init_runner("opencl")

        raise RuntimeError(
            f"Backend {requested_backend} not supported for fem_integration "
            "(supported: cpu, cuda, metal, hip, opencl, amd, intel)."
        )

    @staticmethod
    def _sanitize_list(values: List[str], allowed: tuple[str, ...], fallback: List[str]) -> List[str]:
        out: List[str] = []
        seen = set()
        for raw in values:
            v = str(raw).strip().lower()
            if v in allowed and v not in seen:
                out.append(v)
                seen.add(v)
        return out or list(fallback)

    @staticmethod
    def _qp_cap(element_type: str) -> int:
        if element_type == "tet4":
            return 4
        if element_type == "hex8":
            return 8
        return 1

    @property
    def name(self) -> str:
        return f"fem_integration_{self.resolved_backend}"

    @property
    def search_space(self) -> SearchSpace:
        return self._space

    def _run_kernel(
        self,
        *,
        n_elements: int,
        n_qp: int,
        element_type: str,
        operator: str,
        dtype: str,
    ) -> tuple[float, float, float]:
        if element_type == "tet4":
            return self._runner.fem_integration_tet4(
                n_elements=n_elements,
                n_qp=n_qp,
                operator=operator,
                dtype=dtype,
            )
        if element_type == "hex8":
            return self._runner.fem_integration_hex8(
                n_elements=n_elements,
                n_qp=n_qp,
                operator=operator,
                dtype=dtype,
            )
        raise ValueError(f"Unsupported element_type: {element_type}")

    def _run_once(
        self,
        *,
        n_elements: int,
        n_qp_requested: int,
        element_type: str,
        operator: str,
        dtype: str,
    ) -> tuple[float, float, float, float, str, float, int, int]:
        n_qp_cap = self._qp_cap(element_type)
        n_qp = max(1, min(int(n_qp_requested), int(n_qp_cap)))

        logger: EnergyLogger | None = None
        energy_source = "unavailable"
        try:
            logger = EnergyLogger(
                domain=self._energy_domain,
                device_index=self.device_index,
                sample_interval_s=self.cfg.energy_sample_interval_s,
            )
            logger.start()
            energy_source = logger.energy_source
        except Exception:
            logger = None

        elapsed_total = 0.0
        flops_total = 0.0
        bytes_total = 0.0
        batches = 0
        min_window = max(0.0, float(self.cfg.energy_min_window_s))
        max_batches = max(1, int(self.cfg.energy_window_max_batches))

        while True:
            elapsed_s, gflops, gbps = self._run_kernel(
                n_elements=n_elements,
                n_qp=n_qp,
                element_type=element_type,
                operator=operator,
                dtype=dtype,
            )
            elapsed_total += float(elapsed_s)

            if math.isfinite(gflops) and gflops > 0:
                flops_total += float(gflops) * float(elapsed_s) * 1e9
            if math.isfinite(gbps) and gbps > 0:
                bytes_total += float(gbps) * float(elapsed_s) * 1e9

            batches += 1
            if min_window <= 0.0:
                break
            if elapsed_total >= min_window:
                break
            if batches >= max_batches:
                break

        if logger is not None:
            try:
                energy_j, power_w = logger.stop()
                energy_source = logger.energy_source
            except Exception:
                energy_j, power_w = float("nan"), float("nan")
                energy_source = "unavailable"
        else:
            energy_j, power_w = float("nan"), float("nan")
        gflops_eff = flops_total / max(elapsed_total, 1e-12) / 1e9
        gbps_eff = bytes_total / max(elapsed_total, 1e-12) / 1e9

        return elapsed_total, gflops_eff, gbps_eff, energy_j, energy_source, power_w, batches, n_qp

    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        n_elements = max(1, int(config["n_elements"]))
        n_qp_requested = max(1, int(config["n_qp"]))
        element_type = str(config["element_type"]).lower()
        operator = str(config["operator"]).lower()
        dtype = str(config["dtype"]).lower()

        if element_type not in _SUPPORTED_ELEMENT_TYPES:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=[f"invalid_element_type:{element_type}"],
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )
        if operator not in _SUPPORTED_OPERATORS:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=[f"invalid_operator:{operator}"],
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )
        if dtype not in _SUPPORTED_DTYPES:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=[f"invalid_dtype:{dtype}"],
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )

        elapsed_vals: List[float] = []
        gflops_vals: List[float] = []
        gbps_vals: List[float] = []
        energy_vals: List[float] = []
        power_vals: List[float] = []
        sources: List[str] = []
        batch_counts: List[int] = []
        n_qp_effective_vals: List[int] = []

        for _ in range(max(1, int(self.cfg.repeats))):
            try:
                elapsed_s, gflops, gbps, energy_j, src, power_w, n_batches, n_qp_effective = self._run_once(
                    n_elements=n_elements,
                    n_qp_requested=n_qp_requested,
                    element_type=element_type,
                    operator=operator,
                    dtype=dtype,
                )
            except Exception as e:
                return EvaluationResult(
                    status="error",
                    constraints_ok=False,
                    violations=[f"runtime_error:{e}"],
                    metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
                )

            elapsed_vals.append(elapsed_s)
            gflops_vals.append(gflops)
            gbps_vals.append(gbps)
            sources.append(src)
            batch_counts.append(n_batches)
            n_qp_effective_vals.append(n_qp_effective)
            if not math.isnan(energy_j):
                energy_vals.append(energy_j)
            if not math.isnan(power_w):
                power_vals.append(power_w)

        elapsed_mean = stats.mean(elapsed_vals)
        gflops_mean = stats.mean(gflops_vals)
        gflops_sigma = stats.pstdev(gflops_vals) if len(gflops_vals) > 1 else 0.0
        cv_gflops = gflops_sigma / max(abs(gflops_mean), 1e-12)

        gbps_mean = stats.mean(gbps_vals)
        gbps_sigma = stats.pstdev(gbps_vals) if len(gbps_vals) > 1 else 0.0
        cv_gbps = gbps_sigma / max(abs(gbps_mean), 1e-12)

        energy_mean = stats.mean(energy_vals) if energy_vals else float("nan")
        power_mean = stats.mean(power_vals) if power_vals else float("nan")

        j_per_gflop = float("nan")
        if not math.isnan(energy_mean) and gflops_mean > 0 and elapsed_mean > 0:
            total_gflop = gflops_mean * elapsed_mean
            if total_gflop > 0:
                j_per_gflop = energy_mean / total_gflop

        j_per_gb = float("nan")
        if not math.isnan(energy_mean) and gbps_mean > 0 and elapsed_mean > 0:
            total_gb = gbps_mean * elapsed_mean
            if total_gb > 0:
                j_per_gb = energy_mean / total_gb

        edp = float("nan")
        if not math.isnan(energy_mean):
            edp = energy_mean * elapsed_mean

        violations: List[str] = []
        if elapsed_mean < float(self.cfg.min_elapsed_s):
            violations.append(f"min_elapsed_s<{self.cfg.min_elapsed_s}")
        if cv_gflops > float(self.cfg.max_cv):
            violations.append(f"cv_gflops>{self.cfg.max_cv}")
        if not math.isfinite(gflops_mean) or gflops_mean <= 0:
            violations.append("invalid_gflops")
        if not math.isfinite(gbps_mean) or gbps_mean <= 0:
            violations.append("invalid_gbps")

        n_qp_effective = max(n_qp_effective_vals) if n_qp_effective_vals else n_qp_requested
        metrics = {
            "elapsed_s_mean": elapsed_mean,
            "gflops_mean": gflops_mean,
            "gflops_sigma": gflops_sigma,
            "cv_gflops": cv_gflops,
            "gbps_mean": gbps_mean,
            "gbps_sigma": gbps_sigma,
            "cv_gbps": cv_gbps,
            "energy_j_mean": energy_mean,
            "power_w_mean": power_mean,
            "j_per_gflop": j_per_gflop,
            "j_per_gb": j_per_gb,
            "edp": edp,
            "n_elements": float(n_elements),
            "n_qp_requested": float(n_qp_requested),
            "n_qp_effective": float(n_qp_effective),
        }
        artifacts = {
            "energy_sources": sorted(set(sources)),
            "raw_elapsed_s": elapsed_vals,
            "raw_gflops": gflops_vals,
            "raw_gbps": gbps_vals,
            "raw_energy_j": energy_vals,
            "raw_power_w": power_vals,
            "raw_batches": batch_counts,
            "device": self.device_name,
            "backend": self.backend,
            "element_type": element_type,
            "operator": operator,
            "dtype": dtype,
            "n_qp_requested": n_qp_requested,
            "n_qp_effective": n_qp_effective,
            "n_qp_clipped": int(n_qp_requested != n_qp_effective),
        }

        return EvaluationResult(
            status="ok",
            metrics=metrics,
            artifacts=artifacts,
            constraints_ok=(len(violations) == 0),
            violations=violations,
        )
