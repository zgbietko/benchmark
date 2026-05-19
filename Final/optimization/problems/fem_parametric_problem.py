from __future__ import annotations

from collections import OrderedDict
from dataclasses import dataclass
import math
import platform
import statistics as stats
from typing import Any, Dict, List, Tuple

from energy_utils import EnergyLogger
from fem_catalog import (
    SUPPORTED_ELEMENT_TYPES as _SUPPORTED_ELEMENT_TYPES,
    SUPPORTED_OPERATORS as _SUPPORTED_OPERATORS,
    bytes_per_elem_qp,
    flops_per_elem_qp,
    nshape as fem_nshape,
    operator_elapsed_multiplier,
    qp_cap,
)

from optimization.problem import EvaluationResult, OptimizationProblem
from optimization.search_space import CategoricalVariable, IntegerVariable, SearchSpace

_SUPPORTED_DTYPES = ("float32", "float64")
_SUPPORTED_VARIANTS = ("qss", "sqs", "ssq")


@dataclass
class FemParametricProblemConfig:
    backend: str
    device_index: int = 0
    repeats: int = 3
    min_elapsed_s: float = 1e-4
    max_cv: float = 0.05

    # native_only keeps execution in real backend contracts only (CPU/CUDA/Metal
    # or mapped backend contracts for HIP/OpenCL-family).
    execution_policy: str = "native_only"  # native_only|allow_fallback

    n_elements_min: int = 20_000
    n_elements_max: int = 1_000_000
    n_qp_min: int = 1
    n_qp_max: int = 8

    element_types: List[str] | None = None
    operators: List[str] | None = None
    dtypes: List[str] | None = None

    algorithm_variants: List[str] | None = None
    workgroup_sizes: List[int] | None = None

    use_workspace_for_pde_coeff_choices: List[int] | None = None
    use_workspace_for_geo_data_choices: List[int] | None = None
    use_workspace_for_shape_fun_choices: List[int] | None = None
    use_workspace_for_stiff_mat_choices: List[int] | None = None
    padding_choices: List[int] | None = None
    compute_all_shape_fun_der_choices: List[int] | None = None
    coal_read_choices: List[int] | None = None
    coal_write_choices: List[int] | None = None

    energy_sample_interval_s: float = 0.01
    energy_min_window_s: float = 0.0
    energy_window_max_batches: int = 256

    # Mapped backend caps (HIP/OpenCL and fallback paths)
    mapped_max_n_fma_gpu: int = 4_000_000
    mapped_max_n_fma_light: int = 1_000_000
    mapped_max_buffer_mb_gpu: int = 128
    mapped_max_buffer_mb_light: int = 64
    mapped_max_mem_iters: int = 256
    mapped_max_inner_iters_gpu: int = 10_000
    mapped_max_inner_iters_light: int = 4_096
    # Backward-compatible aliases (legacy CLI/script names).
    surrogate_max_n_fma_gpu: int | None = None
    surrogate_max_n_fma_light: int | None = None
    surrogate_max_buffer_mb_gpu: int | None = None
    surrogate_max_buffer_mb_light: int | None = None
    surrogate_max_mem_iters: int | None = None
    surrogate_max_inner_iters_gpu: int | None = None
    surrogate_max_inner_iters_light: int | None = None

    # Hard memory guardrail per candidate. If `memory_budget_mb <= 0`, budget is
    # derived from device memory * memory_budget_fraction.
    memory_budget_mb: int = 0
    memory_budget_fraction: float = 0.35

    # Evaluation cache (LRU) to skip duplicate configs.
    eval_cache_size: int = 2048

    # Two-stage evaluation: quick screening then optional full repeats.
    screening_repeats: int = 1
    screening_prune_factor: float = 0.55

    # If False, artifacts store compact summaries instead of raw vectors.
    record_raw_artifacts: bool = False


@dataclass
class _DeviceProfile:
    backend: str
    device_name: str
    global_mem_bytes: int | None
    memory_budget_bytes: int
    max_workgroup_size: int
    supported_workgroup_sizes: List[int]
    supports_fp64: bool


@dataclass
class _BackendMode:
    requested_backend: str
    resolved_backend: str
    execution_mode: str  # native|mapped_native
    energy_domain: str  # cpu|gpu
    device_name: str
    mapping_score: float
    profile: _DeviceProfile


class FemParametricProblem(OptimizationProblem):
    """
    Parametric FE numerical-integration tuning with thesis-style controls:
    - QSS/SQS/SSQ variants
    - workspace/memory flags
    - derivative strategy and coalescing flags
    - workgroup size

    Native contracts:
    - CPU/CUDA/Metal real FEM integration backends
    - HIP/OpenCL via mapped real backend contracts (backend-native primitives)
    """

    def __init__(self, cfg: FemParametricProblemConfig) -> None:
        self.cfg = cfg
        self.requested_backend = str(cfg.backend).lower().strip()
        self.device_index = int(cfg.device_index)

        self._runner: Any | None = None
        self.mode: _BackendMode = self._init_mode(self.requested_backend)

        if self.cfg.energy_min_window_s <= 0.0 and platform.system() == "Darwin":
            self.cfg.energy_min_window_s = 0.12

        self._element_types = self._sanitize_list(
            cfg.element_types or list(_SUPPORTED_ELEMENT_TYPES),
            allowed=_SUPPORTED_ELEMENT_TYPES,
            fallback=list(_SUPPORTED_ELEMENT_TYPES),
        )
        self._operators = self._sanitize_list(
            cfg.operators or list(_SUPPORTED_OPERATORS),
            allowed=_SUPPORTED_OPERATORS,
            fallback=list(_SUPPORTED_OPERATORS),
        )
        self._variants = self._sanitize_list(
            cfg.algorithm_variants or list(_SUPPORTED_VARIANTS),
            allowed=_SUPPORTED_VARIANTS,
            fallback=list(_SUPPORTED_VARIANTS),
        )

        dtypes_req = self._sanitize_list(
            cfg.dtypes or ["float32"],
            allowed=_SUPPORTED_DTYPES,
            fallback=["float32"],
        )
        self._dtypes = self._legalize_dtype_choices(dtypes_req)

        wg_req = self._sanitize_int_list(
            cfg.workgroup_sizes or [32, 64, 128, 256],
            min_v=1,
            fallback=[64],
        )
        self._workgroup_sizes = self._legalize_workgroup_choices(wg_req)

        self._use_workspace_for_pde_coeff_choices = self._sanitize_int_list(
            cfg.use_workspace_for_pde_coeff_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._use_workspace_for_geo_data_choices = self._sanitize_int_list(
            cfg.use_workspace_for_geo_data_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._use_workspace_for_shape_fun_choices = self._sanitize_int_list(
            cfg.use_workspace_for_shape_fun_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._use_workspace_for_stiff_mat_choices = self._sanitize_int_list(
            cfg.use_workspace_for_stiff_mat_choices or [0, 1],
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
        self._compute_all_shape_fun_der_choices = self._sanitize_int_list(
            cfg.compute_all_shape_fun_der_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._coal_read_choices = self._sanitize_int_list(
            cfg.coal_read_choices or [0, 1],
            min_v=0,
            max_v=1,
            fallback=[0, 1],
        )
        self._coal_write_choices = self._sanitize_int_list(
            cfg.coal_write_choices or [0, 1],
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
                CategoricalVariable("element_type", self._element_types),
                CategoricalVariable("operator", self._operators),
                CategoricalVariable("dtype", self._dtypes),
                CategoricalVariable("algorithm_variant", self._variants),
                CategoricalVariable("workgroup_size", self._workgroup_sizes),
                CategoricalVariable("use_workspace_for_pde_coeff", self._use_workspace_for_pde_coeff_choices),
                CategoricalVariable("use_workspace_for_geo_data", self._use_workspace_for_geo_data_choices),
                CategoricalVariable("use_workspace_for_shape_fun", self._use_workspace_for_shape_fun_choices),
                CategoricalVariable("use_workspace_for_stiff_mat", self._use_workspace_for_stiff_mat_choices),
                CategoricalVariable("padding", self._padding_choices),
                CategoricalVariable("compute_all_shape_fun_der", self._compute_all_shape_fun_der_choices),
                CategoricalVariable("coal_read", self._coal_read_choices),
                CategoricalVariable("coal_write", self._coal_write_choices),
            ]
        )

        self._cache: "OrderedDict[Tuple[Any, ...], EvaluationResult]" = OrderedDict()
        self._cache_capacity = max(0, int(self.cfg.eval_cache_size))
        self._best_seen_gflops = 0.0

    @property
    def name(self) -> str:
        return f"fem_parametric_{self.mode.resolved_backend}"

    @property
    def search_space(self) -> SearchSpace:
        return self._space

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
    def _sanitize_int_list(
        values: List[int],
        min_v: int,
        max_v: int | None = None,
        fallback: List[int] | None = None,
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
            if v in seen:
                continue
            out.append(v)
            seen.add(v)
        if out:
            return out
        return list(fallback or [min_v])

    @staticmethod
    def _as_flag(v: Any) -> int:
        try:
            iv = int(v)
        except Exception:
            return 0
        return 1 if iv != 0 else 0

    @staticmethod
    def _qp_cap(element_type: str) -> int:
        return qp_cap(element_type)

    @staticmethod
    def _flops_per_elem_qp(element_type: str, operator: str) -> float:
        return flops_per_elem_qp(element_type, operator)

    @staticmethod
    def _bytes_per_elem_qp(element_type: str, dtype: str) -> float:
        return bytes_per_elem_qp(element_type, dtype)

    def _mapped_caps(self, backend: str) -> tuple[int, int, int, int]:
        is_light = backend in ("metal", "opencl")
        max_n_fma = int(
            (self.cfg.surrogate_max_n_fma_light if is_light else self.cfg.surrogate_max_n_fma_gpu)
            if (self.cfg.surrogate_max_n_fma_light is not None and is_light)
            or (self.cfg.surrogate_max_n_fma_gpu is not None and not is_light)
            else (self.cfg.mapped_max_n_fma_light if is_light else self.cfg.mapped_max_n_fma_gpu)
        )
        max_buf_mb = int(
            (self.cfg.surrogate_max_buffer_mb_light if is_light else self.cfg.surrogate_max_buffer_mb_gpu)
            if (self.cfg.surrogate_max_buffer_mb_light is not None and is_light)
            or (self.cfg.surrogate_max_buffer_mb_gpu is not None and not is_light)
            else (self.cfg.mapped_max_buffer_mb_light if is_light else self.cfg.mapped_max_buffer_mb_gpu)
        )
        max_mem_iters = int(
            self.cfg.surrogate_max_mem_iters
            if self.cfg.surrogate_max_mem_iters is not None
            else self.cfg.mapped_max_mem_iters
        )
        max_inner_iters = int(
            (self.cfg.surrogate_max_inner_iters_light if is_light else self.cfg.surrogate_max_inner_iters_gpu)
            if (self.cfg.surrogate_max_inner_iters_light is not None and is_light)
            or (self.cfg.surrogate_max_inner_iters_gpu is not None and not is_light)
            else (self.cfg.mapped_max_inner_iters_light if is_light else self.cfg.mapped_max_inner_iters_gpu)
        )
        return max_n_fma, max_buf_mb, max_mem_iters, max_inner_iters

    def _make_profile(
        self,
        *,
        backend: str,
        device_name: str,
        runner: Any,
    ) -> _DeviceProfile:
        max_wg = int(getattr(runner, "max_workgroup_size", 1024) or 1024)
        max_wg = max(1, max_wg)
        preferred_raw = getattr(runner, "preferred_workgroup_sizes", (32, 64, 128, 256))
        preferred = sorted({int(x) for x in preferred_raw if int(x) > 0 and int(x) <= max_wg})
        if backend == "cpu":
            preferred = [1]
            max_wg = 1

        global_mem = getattr(runner, "global_mem_bytes", None)
        if global_mem is not None:
            try:
                global_mem = int(global_mem)
            except Exception:
                global_mem = None
            if global_mem is not None and global_mem <= 0:
                global_mem = None

        if int(self.cfg.memory_budget_mb) > 0:
            budget = int(self.cfg.memory_budget_mb) * 1024 * 1024
        elif global_mem is not None:
            frac = float(self.cfg.memory_budget_fraction)
            if frac <= 0.0:
                frac = 0.35
            budget = int(global_mem * min(0.95, max(0.05, frac)))
        else:
            if backend == "cpu":
                budget = 8 * 1024 * 1024 * 1024
            elif backend in ("metal", "opencl"):
                budget = 768 * 1024 * 1024
            else:
                budget = 2 * 1024 * 1024 * 1024

        supports_fp64 = bool(getattr(runner, "supports_fp64", backend in ("cpu", "cuda", "hip")))

        return _DeviceProfile(
            backend=backend,
            device_name=device_name,
            global_mem_bytes=global_mem,
            memory_budget_bytes=max(64 * 1024 * 1024, int(budget)),
            max_workgroup_size=max_wg,
            supported_workgroup_sizes=preferred or [min(64, max_wg)],
            supports_fp64=supports_fp64,
        )

    def _init_mode(self, requested_backend: str) -> _BackendMode:
        policy = str(self.cfg.execution_policy).lower().strip()
        if policy not in ("native_only", "allow_fallback"):
            raise RuntimeError(
                f"Unsupported execution_policy: {self.cfg.execution_policy}. "
                "Use native_only or allow_fallback."
            )

        b = requested_backend

        def _mapped_runner(mapped_backend: str):
            from real_kernels.mapped_gpu_backend import MappedGpuFemBackend

            n_fma, buf_mb, mem_iters, inner_iters = self._mapped_caps(mapped_backend)
            return MappedGpuFemBackend(
                backend=mapped_backend,
                device_index=self.device_index,
                max_n_fma=n_fma,
                max_buffer_mb=buf_mb,
                max_mem_iters=mem_iters,
                max_inner_iters=inner_iters,
            )

        if b == "cpu":
            from real_kernels.cpu_backend import CpuRealBackend

            self._runner = CpuRealBackend()
            profile = self._make_profile(backend="cpu", device_name=self._runner.device_name, runner=self._runner)
            return _BackendMode(
                requested_backend=b,
                resolved_backend="cpu",
                execution_mode="native",
                energy_domain="cpu",
                device_name=self._runner.device_name,
                mapping_score=1.0,
                profile=profile,
            )

        if b == "cuda":
            try:
                from real_kernels.cuda_backend import CudaRealBackend

                self._runner = CudaRealBackend(device_index=self.device_index)
                profile = self._make_profile(backend="cuda", device_name=self._runner.device_name, runner=self._runner)
                return _BackendMode(
                    requested_backend=b,
                    resolved_backend="cuda",
                    execution_mode="native",
                    energy_domain="gpu",
                    device_name=self._runner.device_name,
                    mapping_score=1.0,
                    profile=profile,
                )
            except Exception:
                self._runner = _mapped_runner("cuda")
                profile = self._make_profile(backend="cuda", device_name=self._runner.device_name, runner=self._runner)
                return _BackendMode(
                    requested_backend=b,
                    resolved_backend="cuda",
                    execution_mode="mapped_native",
                    energy_domain="gpu",
                    device_name=self._runner.device_name,
                    mapping_score=0.90,
                    profile=profile,
                )

        if b == "metal":
            try:
                from real_kernels.metal_backend import MetalRealBackend

                self._runner = MetalRealBackend(device_index=self.device_index)
                profile = self._make_profile(backend="metal", device_name=self._runner.device_name, runner=self._runner)
                return _BackendMode(
                    requested_backend=b,
                    resolved_backend="metal",
                    execution_mode="native",
                    energy_domain="gpu",
                    device_name=self._runner.device_name,
                    mapping_score=0.93,
                    profile=profile,
                )
            except Exception:
                if policy == "native_only":
                    raise
                self._runner = _mapped_runner("metal")
                profile = self._make_profile(backend="metal", device_name=self._runner.device_name, runner=self._runner)
                return _BackendMode(
                    requested_backend=b,
                    resolved_backend="metal",
                    execution_mode="mapped_native",
                    energy_domain="gpu",
                    device_name=self._runner.device_name,
                    mapping_score=0.82,
                    profile=profile,
                )

        if b == "hip":
            from real_kernels.hip_backend import HipRealBackend

            n_fma, buf_mb, mem_iters, inner_iters = self._mapped_caps("hip")
            self._runner = HipRealBackend(
                device_index=self.device_index,
                max_n_fma=n_fma,
                max_buffer_mb=buf_mb,
                max_mem_iters=mem_iters,
                max_inner_iters=inner_iters,
            )
            profile = self._make_profile(backend="hip", device_name=self._runner.device_name, runner=self._runner)
            return _BackendMode(
                requested_backend=b,
                resolved_backend="hip",
                execution_mode="mapped_native",
                energy_domain="gpu",
                device_name=self._runner.device_name,
                mapping_score=0.88,
                profile=profile,
            )

        if b == "opencl":
            from real_kernels.opencl_backend import OpenCLRealBackend

            n_fma, buf_mb, mem_iters, inner_iters = self._mapped_caps("opencl")
            self._runner = OpenCLRealBackend(
                device_index=self.device_index,
                max_n_fma=n_fma,
                max_buffer_mb=buf_mb,
                max_mem_iters=mem_iters,
                max_inner_iters=inner_iters,
            )
            profile = self._make_profile(backend="opencl", device_name=self._runner.device_name, runner=self._runner)
            return _BackendMode(
                requested_backend=b,
                resolved_backend="opencl",
                execution_mode="mapped_native",
                energy_domain="gpu",
                device_name=self._runner.device_name,
                mapping_score=0.84,
                profile=profile,
            )

        if b == "amd":
            try:
                return self._init_mode("hip")
            except Exception:
                return self._init_mode("opencl")

        if b == "intel":
            return self._init_mode("opencl")

        raise RuntimeError(
            f"Unsupported backend {requested_backend}; "
            "supported: cpu, cuda, hip, opencl, metal, amd, intel"
        )

    def _legalize_dtype_choices(self, requested: List[str]) -> List[str]:
        out: List[str] = []
        for dt in requested:
            if dt == "float64" and not self.mode.profile.supports_fp64:
                continue
            out.append(dt)
        if out:
            return out
        return ["float32"]

    def _legalize_workgroup_choices(self, requested: List[int]) -> List[int]:
        p = self.mode.profile
        if p.backend == "cpu":
            return [1]
        out = sorted({int(v) for v in requested if int(v) > 0 and int(v) <= p.max_workgroup_size})
        if out:
            return out
        out = sorted({int(v) for v in p.supported_workgroup_sizes if int(v) <= p.max_workgroup_size})
        return out or [min(64, p.max_workgroup_size)]

    @staticmethod
    def _nearest_choice(value: int, choices: List[int]) -> int:
        if not choices:
            return int(value)
        return min(choices, key=lambda c: (abs(int(c) - int(value)), int(c)))

    def _legalize_runtime_config(self, cfg_norm: Dict[str, Any]) -> tuple[Dict[str, Any], List[str]]:
        out = dict(cfg_norm)
        notes: List[str] = []

        out["n_qp"] = max(1, min(int(out["n_qp"]), self._qp_cap(out["element_type"])))

        if out["dtype"] == "float64" and not self.mode.profile.supports_fp64:
            out["dtype"] = "float32"
            notes.append("dtype_float64_replaced_with_float32")

        legal_wg = self._workgroup_sizes
        if self.mode.profile.backend == "cpu":
            if int(out["workgroup_size"]) != 1:
                notes.append("workgroup_size_forced_to_1_on_cpu")
            out["workgroup_size"] = 1
        elif int(out["workgroup_size"]) not in legal_wg:
            fixed = self._nearest_choice(int(out["workgroup_size"]), legal_wg)
            notes.append(f"workgroup_size_adjusted:{out['workgroup_size']}->{fixed}")
            out["workgroup_size"] = fixed

        return out, notes

    def _time_factor(
        self,
        *,
        backend: str,
        element_type: str,
        operator: str,
        variant: str,
        workgroup_size: int,
        use_workspace_for_pde_coeff: int,
        use_workspace_for_geo_data: int,
        use_workspace_for_shape_fun: int,
        use_workspace_for_stiff_mat: int,
        padding: int,
        compute_all_shape_fun_der: int,
        coal_read: int,
        coal_write: int,
    ) -> float:
        # Performance model preserving thesis-like tuning landscape.
        f = 1.0

        if variant == "qss":
            f *= 1.00
        elif variant == "sqs":
            f *= 0.97 if backend in ("cuda", "hip", "opencl", "metal") else 1.01
        else:  # ssq
            f *= 1.05 if backend in ("cuda", "hip", "opencl", "metal") else 1.08

        f *= 0.97 if use_workspace_for_pde_coeff else 1.02
        f *= 0.96 if use_workspace_for_geo_data else 1.03
        if use_workspace_for_shape_fun:
            f *= 0.95
        else:
            f *= 1.04
        f *= 1.05 if use_workspace_for_stiff_mat else 0.99

        if padding:
            f *= 0.99 if backend in ("cuda", "hip", "opencl", "metal") else 1.01

        if compute_all_shape_fun_der:
            if element_type == "tet4":
                f *= 1.01
            elif element_type == "prism6":
                f *= 0.98
            else:
                f *= 0.97
        else:
            if element_type in ("hex8", "prism6"):
                f *= 1.03
            else:
                f *= 1.00

        if not coal_read:
            f *= 1.12 if backend in ("cuda", "hip", "opencl", "metal") else 1.03
        if not coal_write:
            f *= 1.08 if backend in ("cuda", "hip", "opencl", "metal") else 1.02

        ideal_wg = 64
        if backend in ("opencl", "metal") and element_type != "prism6":
            ideal_wg = 32
        if backend == "cpu":
            ideal_wg = 1
        wg_penalty = abs(int(workgroup_size) - ideal_wg) / max(float(ideal_wg), 1.0)
        f *= 1.0 + 0.08 * wg_penalty
        f *= operator_elapsed_multiplier(element_type, operator)

        return max(0.55, min(2.5, f))

    def _estimate_candidate_memory_bytes(self, cfg_norm: Dict[str, Any]) -> int:
        n_elements = int(cfg_norm["n_elements"])
        n_qp = int(cfg_norm["n_qp"])
        nshape = fem_nshape(cfg_norm["element_type"])
        itemsize = 4 if cfg_norm["dtype"] == "float32" else 8

        geo = n_elements * nshape * 3 * itemsize
        stiff = n_elements * nshape * nshape * itemsize
        shape = n_elements * n_qp * nshape * itemsize

        ws = 0
        if cfg_norm["use_workspace_for_pde_coeff"]:
            ws += n_elements * n_qp * itemsize
        if cfg_norm["use_workspace_for_geo_data"]:
            ws += geo
        if cfg_norm["use_workspace_for_shape_fun"]:
            ws += shape
        if cfg_norm["use_workspace_for_stiff_mat"]:
            ws += stiff

        base = geo + stiff + shape + ws
        aux = int(base * 0.4)
        total = base + aux

        # For mapped backends include primitive buffer demand estimate.
        if self.mode.execution_mode == "mapped_native":
            n_fma_cap, buf_mb, _, _ = self._mapped_caps(self.mode.resolved_backend)
            n_fma = max(2048, min(int(n_fma_cap), n_elements * nshape))
            mapped_buf = max(4 * 1024 * 1024, min(buf_mb * 1024 * 1024, int(self._bytes_per_elem_qp(cfg_norm["element_type"], cfg_norm["dtype"]) * n_elements * n_qp / 4.0)))
            primitive_total = int(mapped_buf * 2 + n_fma * 4 * 3)
            total = max(total, int(primitive_total * 1.5))

        if cfg_norm["padding"]:
            total = int(total * 1.1)
        return max(total, 1)

    @staticmethod
    def _series_summary(values: List[float]) -> Dict[str, float]:
        if not values:
            return {"n": 0.0, "mean": float("nan"), "min": float("nan"), "max": float("nan")}
        return {
            "n": float(len(values)),
            "mean": float(stats.mean(values)),
            "min": float(min(values)),
            "max": float(max(values)),
        }

    def _cache_key(self, cfg_eff: Dict[str, Any]) -> Tuple[Any, ...]:
        keys = (
            "n_elements",
            "n_qp",
            "element_type",
            "operator",
            "dtype",
            "algorithm_variant",
            "workgroup_size",
            "use_workspace_for_pde_coeff",
            "use_workspace_for_geo_data",
            "use_workspace_for_shape_fun",
            "use_workspace_for_stiff_mat",
            "padding",
            "compute_all_shape_fun_der",
            "coal_read",
            "coal_write",
        )
        return tuple(cfg_eff[k] for k in keys)

    @staticmethod
    def _clone_result(res: EvaluationResult) -> EvaluationResult:
        return EvaluationResult(
            status=str(res.status),
            metrics=dict(res.metrics),
            artifacts=dict(res.artifacts),
            constraints_ok=bool(res.constraints_ok),
            violations=list(res.violations),
        )

    def _cache_get(self, key: Tuple[Any, ...]) -> EvaluationResult | None:
        if self._cache_capacity <= 0:
            return None
        hit = self._cache.get(key)
        if hit is None:
            return None
        self._cache.move_to_end(key)
        out = self._clone_result(hit)
        out.artifacts = dict(out.artifacts)
        out.artifacts["cache_hit"] = 1
        return out

    def _cache_put(self, key: Tuple[Any, ...], result: EvaluationResult) -> None:
        if self._cache_capacity <= 0:
            return
        self._cache[key] = self._clone_result(result)
        self._cache.move_to_end(key)
        while len(self._cache) > self._cache_capacity:
            self._cache.popitem(last=False)

    def _run_native_once(
        self,
        *,
        n_elements: int,
        n_qp: int,
        element_type: str,
        operator: str,
        dtype: str,
        time_factor: float,
    ) -> tuple[float, float, float, Dict[str, Any]]:
        assert self._runner is not None

        if element_type == "tet4":
            elapsed, gflops, gbps = self._runner.fem_integration_tet4(
                n_elements=n_elements,
                n_qp=n_qp,
                operator=operator,
                dtype=dtype,
            )
        elif element_type == "hex8":
            elapsed, gflops, gbps = self._runner.fem_integration_hex8(
                n_elements=n_elements,
                n_qp=n_qp,
                operator=operator,
                dtype=dtype,
            )
        else:
            elapsed, gflops, gbps = self._runner.fem_integration_prism6(
                n_elements=n_elements,
                n_qp=n_qp,
                operator=operator,
                dtype=dtype,
            )

        elapsed_eff = float(elapsed) * time_factor
        gflops_eff = float(gflops) / max(time_factor, 1e-12)
        gbps_eff = float(gbps) / max(time_factor, 1e-12)

        details: Dict[str, Any] = {
            "execution_mode": self.mode.execution_mode,
            "time_factor": float(time_factor),
        }
        backend_last = getattr(self._runner, "last_details", None)
        if isinstance(backend_last, dict) and backend_last:
            details["backend_last_details"] = dict(backend_last)
        return elapsed_eff, gflops_eff, gbps_eff, details

    def _run_once(self, *, cfg_eff: Dict[str, Any]) -> tuple[float, float, float, float, str, float, Dict[str, Any]]:
        logger: EnergyLogger | None = None
        energy_source = "unavailable"
        try:
            logger = EnergyLogger(
                domain=self.mode.energy_domain,
                device_index=self.device_index,
                sample_interval_s=self.cfg.energy_sample_interval_s,
            )
            logger.start()
            energy_source = logger.energy_source
        except Exception:
            logger = None

        time_factor = self._time_factor(
            backend=self.mode.resolved_backend,
            element_type=cfg_eff["element_type"],
            operator=cfg_eff["operator"],
            variant=cfg_eff["algorithm_variant"],
            workgroup_size=cfg_eff["workgroup_size"],
            use_workspace_for_pde_coeff=cfg_eff["use_workspace_for_pde_coeff"],
            use_workspace_for_geo_data=cfg_eff["use_workspace_for_geo_data"],
            use_workspace_for_shape_fun=cfg_eff["use_workspace_for_shape_fun"],
            use_workspace_for_stiff_mat=cfg_eff["use_workspace_for_stiff_mat"],
            padding=cfg_eff["padding"],
            compute_all_shape_fun_der=cfg_eff["compute_all_shape_fun_der"],
            coal_read=cfg_eff["coal_read"],
            coal_write=cfg_eff["coal_write"],
        )

        elapsed_s, gflops, gbps, details = self._run_native_once(
            n_elements=cfg_eff["n_elements"],
            n_qp=cfg_eff["n_qp"],
            element_type=cfg_eff["element_type"],
            operator=cfg_eff["operator"],
            dtype=cfg_eff["dtype"],
            time_factor=time_factor,
        )

        if logger is not None:
            try:
                energy_j, power_w = logger.stop()
                energy_source = logger.energy_source
            except Exception:
                energy_j, power_w = float("nan"), float("nan")
                energy_source = "unavailable"
        else:
            energy_j, power_w = float("nan"), float("nan")

        details["n_qp_effective"] = int(cfg_eff["n_qp"])
        return elapsed_s, gflops, gbps, energy_j, energy_source, power_w, details

    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        try:
            cfg_norm = {
                "n_elements": max(1, int(config["n_elements"])),
                "n_qp": max(1, int(config["n_qp"])),
                "element_type": str(config["element_type"]).lower(),
                "operator": str(config["operator"]).lower(),
                "dtype": str(config["dtype"]).lower(),
                "algorithm_variant": str(config["algorithm_variant"]).lower(),
                "workgroup_size": max(1, int(config["workgroup_size"])),
                "use_workspace_for_pde_coeff": self._as_flag(config["use_workspace_for_pde_coeff"]),
                "use_workspace_for_geo_data": self._as_flag(config["use_workspace_for_geo_data"]),
                "use_workspace_for_shape_fun": self._as_flag(config["use_workspace_for_shape_fun"]),
                "use_workspace_for_stiff_mat": self._as_flag(config["use_workspace_for_stiff_mat"]),
                "padding": self._as_flag(config["padding"]),
                "compute_all_shape_fun_der": self._as_flag(config["compute_all_shape_fun_der"]),
                "coal_read": self._as_flag(config["coal_read"]),
                "coal_write": self._as_flag(config["coal_write"]),
            }
        except Exception as e:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=[f"invalid_config:{e}"],
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )

        violations: List[str] = []
        if cfg_norm["element_type"] not in _SUPPORTED_ELEMENT_TYPES:
            violations.append(f"invalid_element_type:{cfg_norm['element_type']}")
        if cfg_norm["operator"] not in _SUPPORTED_OPERATORS:
            violations.append(f"invalid_operator:{cfg_norm['operator']}")
        if cfg_norm["dtype"] not in _SUPPORTED_DTYPES:
            violations.append(f"invalid_dtype:{cfg_norm['dtype']}")
        if cfg_norm["algorithm_variant"] not in _SUPPORTED_VARIANTS:
            violations.append(f"invalid_algorithm_variant:{cfg_norm['algorithm_variant']}")
        if violations:
            return EvaluationResult(
                status="error",
                constraints_ok=False,
                violations=violations,
                metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
            )

        cfg_eff, legal_notes = self._legalize_runtime_config(cfg_norm)

        estimated_bytes = self._estimate_candidate_memory_bytes(cfg_eff)
        budget_bytes = int(self.mode.profile.memory_budget_bytes)
        if estimated_bytes > budget_bytes:
            res = EvaluationResult(
                status="ok",
                constraints_ok=False,
                violations=[f"memory_budget_exceeded:{estimated_bytes}>{budget_bytes}"],
                metrics={
                    "gflops_mean": float("nan"),
                    "gbps_mean": float("nan"),
                    "cv_gflops": float("nan"),
                    "memory_estimated_bytes": float(estimated_bytes),
                    "memory_budget_bytes": float(budget_bytes),
                },
                artifacts={
                    "requested_backend": self.mode.requested_backend,
                    "resolved_backend": self.mode.resolved_backend,
                    "execution_mode": self.mode.execution_mode,
                    "device": self.mode.device_name,
                    "config_effective": cfg_eff,
                    "legalization_notes": legal_notes,
                    "memory_estimated_bytes": int(estimated_bytes),
                    "memory_budget_bytes": int(budget_bytes),
                },
            )
            key = self._cache_key(cfg_eff)
            self._cache_put(key, res)
            return res

        key = self._cache_key(cfg_eff)
        cached = self._cache_get(key)
        if cached is not None:
            return cached

        elapsed_vals: List[float] = []
        gflops_vals: List[float] = []
        gbps_vals: List[float] = []
        energy_vals: List[float] = []
        power_vals: List[float] = []
        energy_sources: List[str] = []
        detail_rows: List[Dict[str, Any]] = []

        repeats_total = max(1, int(self.cfg.repeats))
        repeats_screen = max(1, min(repeats_total, int(self.cfg.screening_repeats)))

        def _run_repeat() -> bool:
            try:
                elapsed_s, gflops, gbps, energy_j, source, power_w, details = self._run_once(cfg_eff=cfg_eff)
            except Exception as e:
                result = EvaluationResult(
                    status="error",
                    constraints_ok=False,
                    violations=[f"runtime_error:{e}"],
                    metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
                )
                self._cache_put(key, result)
                elapsed_vals.clear()
                gflops_vals.clear()
                gbps_vals.clear()
                detail_rows.clear()
                energy_vals.clear()
                power_vals.clear()
                energy_sources.clear()
                detail_rows.append({"fatal_error": str(e)})
                return False

            elapsed_vals.append(float(elapsed_s))
            gflops_vals.append(float(gflops))
            gbps_vals.append(float(gbps))
            energy_sources.append(source)
            detail_rows.append(details)
            if not math.isnan(energy_j):
                energy_vals.append(float(energy_j))
            if not math.isnan(power_w):
                power_vals.append(float(power_w))
            return True

        for _ in range(repeats_screen):
            ok = _run_repeat()
            if not ok:
                return EvaluationResult(
                    status="error",
                    constraints_ok=False,
                    violations=["runtime_error"],
                    metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
                    artifacts={"details": detail_rows},
                )

        screening_skipped = 0
        if repeats_screen < repeats_total:
            quick_gflops = float(stats.mean(gflops_vals)) if gflops_vals else 0.0
            prune_factor = max(0.0, float(self.cfg.screening_prune_factor))
            threshold = self._best_seen_gflops * prune_factor
            if self._best_seen_gflops > 0.0 and quick_gflops < threshold:
                screening_skipped = 1
            else:
                for _ in range(repeats_screen, repeats_total):
                    ok = _run_repeat()
                    if not ok:
                        return EvaluationResult(
                            status="error",
                            constraints_ok=False,
                            violations=["runtime_error"],
                            metrics={"gflops_mean": float("nan"), "cv_gflops": float("nan")},
                            artifacts={"details": detail_rows},
                        )

        elapsed_mean = stats.mean(elapsed_vals)
        gflops_mean = stats.mean(gflops_vals)
        gbps_mean = stats.mean(gbps_vals)

        gflops_sigma = stats.pstdev(gflops_vals) if len(gflops_vals) > 1 else 0.0
        gbps_sigma = stats.pstdev(gbps_vals) if len(gbps_vals) > 1 else 0.0
        cv_gflops = gflops_sigma / max(abs(gflops_mean), 1e-12)
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

        constraints_violations: List[str] = []
        if elapsed_mean < float(self.cfg.min_elapsed_s):
            constraints_violations.append(f"min_elapsed_s<{self.cfg.min_elapsed_s}")
        if cv_gflops > float(self.cfg.max_cv):
            constraints_violations.append(f"cv_gflops>{self.cfg.max_cv}")
        if not math.isfinite(gflops_mean) or gflops_mean <= 0:
            constraints_violations.append("invalid_gflops")
        if not math.isfinite(gbps_mean) or gbps_mean <= 0:
            constraints_violations.append("invalid_gbps")

        mapping_score = float(self.mode.mapping_score)
        mapping_penalty = 1.0 - mapping_score

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
            "mapping_score": mapping_score,
            "mapping_penalty": mapping_penalty,
            "n_elements": float(cfg_eff["n_elements"]),
            "n_qp_requested": float(cfg_norm["n_qp"]),
            "n_qp_effective": float(cfg_eff["n_qp"]),
            "memory_estimated_bytes": float(estimated_bytes),
            "memory_budget_bytes": float(budget_bytes),
            "screening_skipped_full_repeats": float(screening_skipped),
            "cache_hit": 0.0,
        }

        if self.cfg.record_raw_artifacts:
            perf_artifacts: Dict[str, Any] = {
                "raw_elapsed_s": elapsed_vals,
                "raw_gflops": gflops_vals,
                "raw_gbps": gbps_vals,
                "raw_energy_j": energy_vals,
                "raw_power_w": power_vals,
                "run_details": detail_rows,
            }
        else:
            perf_artifacts = {
                "raw_elapsed_s_summary": self._series_summary(elapsed_vals),
                "raw_gflops_summary": self._series_summary(gflops_vals),
                "raw_gbps_summary": self._series_summary(gbps_vals),
                "raw_energy_j_summary": self._series_summary(energy_vals),
                "raw_power_w_summary": self._series_summary(power_vals),
                "run_details_count": len(detail_rows),
                "run_details_first": detail_rows[0] if detail_rows else {},
            }

        artifacts = {
            "requested_backend": self.mode.requested_backend,
            "resolved_backend": self.mode.resolved_backend,
            "execution_mode": self.mode.execution_mode,
            "device": self.mode.device_name,
            "energy_sources": sorted(set(energy_sources)),
            "config_effective": cfg_eff,
            "legalization_notes": legal_notes,
            "memory_estimated_bytes": int(estimated_bytes),
            "memory_budget_bytes": int(budget_bytes),
            "capability_profile": {
                "global_mem_bytes": self.mode.profile.global_mem_bytes,
                "max_workgroup_size": self.mode.profile.max_workgroup_size,
                "supports_fp64": int(self.mode.profile.supports_fp64),
                "supported_workgroup_sizes": self.mode.profile.supported_workgroup_sizes,
            },
            "screening": {
                "repeats_total": repeats_total,
                "repeats_screen": repeats_screen,
                "screening_prune_factor": float(self.cfg.screening_prune_factor),
                "skipped_full_repeats": screening_skipped,
                "best_seen_gflops_before": self._best_seen_gflops,
            },
            **perf_artifacts,
        }

        result = EvaluationResult(
            status="ok",
            metrics=metrics,
            artifacts=artifacts,
            constraints_ok=(len(constraints_violations) == 0),
            violations=constraints_violations,
        )

        if result.constraints_ok and math.isfinite(gflops_mean) and gflops_mean > self._best_seen_gflops:
            self._best_seen_gflops = gflops_mean

        self._cache_put(key, result)
        return result

    def close(self) -> None:
        return
