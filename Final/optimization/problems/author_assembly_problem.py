from __future__ import annotations

from dataclasses import dataclass
import math
import platform
import statistics as stats
from types import SimpleNamespace
from typing import Any, Dict, List

from energy_utils import EnergyLogger

from optimization.problem import EvaluationResult, OptimizationProblem
from optimization.search_space import CategoricalVariable, IntegerVariable, SearchSpace


_SUPPORTED_DTYPES = ("float32", "float64")
_SUPPORTED_VARIANTS = ("qss", "sqs", "ssq")


@dataclass
class AuthorAssemblyProblemConfig:
    backend: str
    device_index: int = 0
    repeats: int = 3
    min_elapsed_s: float = 1e-4
    max_cv: float = 0.05

    n_elements_min: int = 10_000
    n_elements_max: int = 250_000
    n_qp_min: int = 1
    n_qp_max: int = 8

    n_dofs_choices: List[int] | None = None
    variant_choices: List[str] | None = None
    workspace_choices: List[int] | None = None
    scatter_choices: List[int] | None = None
    padding_choices: List[int] | None = None
    dtypes: List[str] | None = None

    energy_sample_interval_s: float = 0.01
    energy_min_window_s: float = 0.0
    energy_window_max_batches: int = 128

    mapped_max_n_fma_gpu: int = 4_000_000
    mapped_max_n_fma_light: int = 1_000_000
    mapped_max_buffer_mb_gpu: int = 128
    mapped_max_buffer_mb_light: int = 64
    mapped_max_mem_iters: int = 256
    mapped_max_inner_iters_gpu: int = 10_000
    mapped_max_inner_iters_light: int = 4_096


class AuthorAssemblyProblem(OptimizationProblem):
    """
    Autorski parametryczny workload FEM-like/assembly-like.

    Decision vars:
      - n_elements
      - n_qp
      - n_dofs
      - variant (qss/sqs/ssq)
      - use_workspace
      - scatter_accumulate
      - padding
      - dtype

    Metrics:
      elapsed_s_mean, gflops_mean, gbps_mean, ai_flop_per_byte_mean,
      cv_elapsed, cv_gflops, cv_gbps,
      energy_j_mean, power_w_mean, j_per_gflop, j_per_gb, edp
    """

    def __init__(self, cfg: AuthorAssemblyProblemConfig) -> None:
        self.cfg = cfg
        self.backend = str(cfg.backend).lower().strip()
        self.device_index = int(cfg.device_index)

        self._runner, self._energy_domain, self.resolved_backend = self._init_runner(self.backend)

        if self.cfg.energy_min_window_s <= 0.0 and platform.system() == "Darwin":
            self.cfg.energy_min_window_s = 0.12

        self.device_name = getattr(self._runner, "device_name", self.backend)
        # Compatibility shim: existing reporting code expects problem.mode.
        self.mode = SimpleNamespace(
            resolved_backend=self.resolved_backend,
            execution_mode="native",
            device_name=self.device_name,
        )

        self._dtypes = self._sanitize_str_list(
            cfg.dtypes or ["float32"],
            allowed=_SUPPORTED_DTYPES,
            fallback=["float32"],
        )
        self._variants = self._sanitize_str_list(
            cfg.variant_choices or list(_SUPPORTED_VARIANTS),
            allowed=_SUPPORTED_VARIANTS,
            fallback=list(_SUPPORTED_VARIANTS),
        )
        self._n_dofs_choices = self._sanitize_int_list(
            cfg.n_dofs_choices or [4, 6, 8],
            min_v=2,
            max_v=16,
            fallback=[4, 6, 8],
        )
        self._workspace_choices = self._sanitize_int_list(
            cfg.workspace_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._scatter_choices = self._sanitize_int_list(
            cfg.scatter_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._padding_choices = self._sanitize_int_list(
            cfg.padding_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )

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
                CategoricalVariable("n_dofs", self._n_dofs_choices),
                CategoricalVariable("variant", self._variants),
                CategoricalVariable("use_workspace", self._workspace_choices),
                CategoricalVariable("scatter_accumulate", self._scatter_choices),
                CategoricalVariable("padding", self._padding_choices),
                CategoricalVariable("dtype", self._dtypes),
            ]
        )

    def _mapped_caps(self, backend: str) -> tuple[int, int, int, int]:
        if backend in ("metal", "opencl"):
            return (
                int(self.cfg.mapped_max_n_fma_light),
                int(self.cfg.mapped_max_buffer_mb_light),
                int(self.cfg.mapped_max_mem_iters),
                int(self.cfg.mapped_max_inner_iters_light),
            )
        return (
            int(self.cfg.mapped_max_n_fma_gpu),
            int(self.cfg.mapped_max_buffer_mb_gpu),
            int(self.cfg.mapped_max_mem_iters),
            int(self.cfg.mapped_max_inner_iters_gpu),
        )

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
            f"Backend {requested_backend} not supported for author_assembly "
            "(supported: cpu, cuda, metal, hip, opencl, amd, intel)."
        )

    @staticmethod
    def _sanitize_str_list(values: List[str], allowed: tuple[str, ...], fallback: List[str]) -> List[str]:
        out: List[str] = []
        seen = set()
        for raw in values:
            v = str(raw).strip().lower()
            if v in allowed and v not in seen:
                out.append(v)
                seen.add(v)
        return out or list(fallback)

    @staticmethod
    def _sanitize_int_list(
        values: List[int],
        *,
        min_v: int,
        max_v: int | None = None,
        fallback: List[int],
    ) -> List[int]:
        out: List[int] = []
        seen = set()
        for raw in values:
            try:
                v = int(raw)
            except Exception:
                continue
            if v < min_v:
                continue
            if max_v is not None and v > max_v:
                continue
            if v not in seen:
                out.append(v)
                seen.add(v)
        return out or list(fallback)

    @property
    def name(self) -> str:
        return f"author_assembly_{self.resolved_backend}"

    @property
    def search_space(self) -> SearchSpace:
        return self._space

    def _run_once(
        self,
        *,
        n_elements: int,
        n_qp: int,
        n_dofs: int,
        variant: str,
        use_workspace: int,
        scatter_accumulate: int,
        padding: int,
        dtype: str,
    ) -> tuple[float, float, float, float, float, str, float, int]:
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
        ai_weighted = 0.0
        batches = 0
        min_window = max(0.0, float(self.cfg.energy_min_window_s))
        max_batches = max(1, int(self.cfg.energy_window_max_batches))

        while True:
            elapsed_s, gflops, gbps, ai = self._runner.assembly_like(
                n_elements=n_elements,
                n_qp=n_qp,
                n_dofs=n_dofs,
                variant=variant,
                use_workspace=use_workspace,
                scatter_accumulate=scatter_accumulate,
                padding=padding,
                dtype=dtype,
            )
            elapsed_total += float(elapsed_s)

            if math.isfinite(gflops) and gflops > 0:
                flops_total += float(gflops) * float(elapsed_s) * 1e9
            if math.isfinite(gbps) and gbps > 0:
                bytes_total += float(gbps) * float(elapsed_s) * 1e9
            if math.isfinite(ai):
                ai_weighted += float(ai) * float(elapsed_s)

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
        ai_eff = ai_weighted / max(elapsed_total, 1e-12)

        return elapsed_total, gflops_eff, gbps_eff, ai_eff, energy_j, energy_source, power_w, batches

    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        n_elements = max(1, int(config["n_elements"]))
        n_qp = max(1, int(config["n_qp"]))
        n_dofs = max(2, min(int(config["n_dofs"]), 16))
        variant = str(config["variant"]).lower()
        use_workspace = int(config["use_workspace"])
        scatter_accumulate = int(config["scatter_accumulate"])
        padding = int(config["padding"])
        dtype = str(config["dtype"]).lower()

        if variant not in _SUPPORTED_VARIANTS:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=[f"invalid_variant:{variant}"],
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )

        elapsed_vals: List[float] = []
        gflops_vals: List[float] = []
        gbps_vals: List[float] = []
        ai_vals: List[float] = []
        energy_vals: List[float] = []
        power_vals: List[float] = []
        batches_vals: List[int] = []
        energy_sources: List[str] = []

        for _ in range(max(1, int(self.cfg.repeats))):
            try:
                elapsed, gflops, gbps, ai, energy_j, energy_source, power_w, batches = self._run_once(
                    n_elements=n_elements,
                    n_qp=n_qp,
                    n_dofs=n_dofs,
                    variant=variant,
                    use_workspace=use_workspace,
                    scatter_accumulate=scatter_accumulate,
                    padding=padding,
                    dtype=dtype,
                )
            except Exception as exc:
                return EvaluationResult(
                    status="error",
                    constraints_ok=False,
                    violations=[f"runtime_error:{exc}"],
                    metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
                    artifacts={
                        "resolved_backend": self.resolved_backend,
                        "execution_mode": "native",
                        "device": self.device_name,
                        "error": str(exc),
                    },
                )

            elapsed_vals.append(float(elapsed))
            gflops_vals.append(float(gflops))
            gbps_vals.append(float(gbps))
            ai_vals.append(float(ai))
            energy_vals.append(float(energy_j))
            power_vals.append(float(power_w))
            batches_vals.append(int(batches))
            energy_sources.append(str(energy_source))

        elapsed_mean = stats.mean(elapsed_vals)
        gflops_mean = stats.mean(gflops_vals)
        gbps_mean = stats.mean(gbps_vals)
        ai_mean = stats.mean(ai_vals)
        cv_elapsed = (stats.pstdev(elapsed_vals) / elapsed_mean) if len(elapsed_vals) > 1 and elapsed_mean > 0 else 0.0
        cv_gflops = (stats.pstdev(gflops_vals) / gflops_mean) if len(gflops_vals) > 1 and gflops_mean > 0 else 0.0
        cv_gbps = (stats.pstdev(gbps_vals) / gbps_mean) if len(gbps_vals) > 1 and gbps_mean > 0 else 0.0

        finite_energy = [v for v in energy_vals if math.isfinite(v)]
        finite_power = [v for v in power_vals if math.isfinite(v)]
        energy_mean = stats.mean(finite_energy) if finite_energy else float("nan")
        power_mean = stats.mean(finite_power) if finite_power else float("nan")

        j_per_gflop = (
            energy_mean / max(gflops_mean * elapsed_mean, 1e-12)
            if math.isfinite(energy_mean) and gflops_mean > 0 and elapsed_mean > 0
            else float("nan")
        )
        j_per_gb = (
            energy_mean / max(gbps_mean * elapsed_mean, 1e-12)
            if math.isfinite(energy_mean) and gbps_mean > 0 and elapsed_mean > 0
            else float("nan")
        )
        edp = energy_mean * elapsed_mean if math.isfinite(energy_mean) and elapsed_mean > 0 else float("nan")

        violations: List[str] = []
        if elapsed_mean < float(self.cfg.min_elapsed_s):
            violations.append("short_runtime")
        if cv_gflops > float(self.cfg.max_cv):
            violations.append("high_cv_gflops")
        if cv_gbps > float(self.cfg.max_cv):
            violations.append("high_cv_gbps")

        constraints_ok = len(violations) == 0

        metrics = {
            "elapsed_s_mean": float(elapsed_mean),
            "gflops_mean": float(gflops_mean),
            "gbps_mean": float(gbps_mean),
            "ai_flop_per_byte_mean": float(ai_mean),
            "cv_elapsed": float(cv_elapsed),
            "cv_gflops": float(cv_gflops),
            "cv_gbps": float(cv_gbps),
            "energy_j_mean": float(energy_mean),
            "power_w_mean": float(power_mean),
            "j_per_gflop": float(j_per_gflop),
            "j_per_gb": float(j_per_gb),
            "edp": float(edp),
        }

        artifacts = {
            "resolved_backend": self.resolved_backend,
            "execution_mode": "native",
            "device": self.device_name,
            "n_elements": int(n_elements),
            "n_qp": int(n_qp),
            "n_dofs": int(n_dofs),
            "variant": str(variant),
            "use_workspace": int(use_workspace),
            "scatter_accumulate": int(scatter_accumulate),
            "padding": int(padding),
            "dtype": str(dtype),
            "energy_source": (energy_sources[0] if energy_sources else "unavailable"),
            "batches_mean": float(stats.mean(batches_vals)) if batches_vals else 0.0,
            "backend_details": getattr(self._runner, "last_details", {}),
        }

        return EvaluationResult(
            status="ok",
            metrics=metrics,
            artifacts=artifacts,
            constraints_ok=constraints_ok,
            violations=violations,
        )

    def close(self) -> None:
        closer = getattr(self._runner, "close", None)
        if callable(closer):
            try:
                closer()
            except Exception:
                pass
