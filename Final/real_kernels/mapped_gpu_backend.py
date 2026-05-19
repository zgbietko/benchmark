from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict

from fem_catalog import (
    SUPPORTED_ASSEMBLY_VARIANTS as _SUPPORTED_ASSEMBLY_VARIANTS,
    assembly_like_bytes_per_elem_qp,
    assembly_like_flops_per_elem_qp,
    assembly_variant_multiplier,
    SUPPORTED_OPERATORS as _SUPPORTED_OPERATORS,
    bytes_per_elem_qp,
    flops_per_elem_qp,
    nshape as fem_nshape,
    operator_elapsed_multiplier,
    qp_cap,
)
from optimization.problems.gpu_adapters import GpuBackendAdapter, init_gpu_adapter


@dataclass
class MappedGpuFemBackend:
    """
    Backend contract for platforms without direct real_kernels FEM integration.
    It runs backend-native primitives (FMA + memory path) and maps them to
    equivalent FEM integration metrics.
    """

    backend: str
    device_index: int = 0
    max_n_fma: int = 4_000_000
    max_buffer_mb: int = 128
    max_mem_iters: int = 256
    max_inner_iters: int = 10_000

    adapter: GpuBackendAdapter = field(init=False)
    device_name: str = field(init=False)
    global_mem_bytes: int | None = field(init=False, default=None)
    max_workgroup_size: int | None = field(init=False, default=None)
    supports_fp64: bool = field(init=False, default=False)
    preferred_workgroup_sizes: tuple[int, ...] = field(init=False, default=(32, 64, 128, 256))
    last_details: Dict[str, Any] = field(init=False, default_factory=dict)

    def __post_init__(self) -> None:
        self.adapter = init_gpu_adapter(self.backend, self.device_index)
        self.device_name = self.adapter.device_name
        self.global_mem_bytes = self.adapter.global_mem_bytes
        self.max_workgroup_size = self.adapter.max_workgroup_size
        self.supports_fp64 = self.adapter.supports_fp64
        self.preferred_workgroup_sizes = self.adapter.preferred_workgroup_sizes

    @staticmethod
    def _flops_per_elem_qp(element_type: str, operator: str) -> float:
        return flops_per_elem_qp(element_type, operator)

    @staticmethod
    def _bytes_per_elem_qp(element_type: str, dtype: str) -> float:
        return bytes_per_elem_qp(element_type, dtype)

    @staticmethod
    def _qp_cap(element_type: str) -> int:
        return qp_cap(element_type)

    def _run(
        self,
        *,
        n_elements: int,
        n_qp: int,
        element_type: str,
        operator: str,
        dtype: str,
    ) -> tuple[float, float, float]:
        if dtype not in ("float32", "float64"):
            raise ValueError(f"Unsupported dtype: {dtype}")
        if dtype == "float64" and not self.supports_fp64:
            dtype = "float32"

        n_elements = max(1, int(n_elements))
        n_qp = max(1, min(int(n_qp), self._qp_cap(element_type)))

        flops_per_elem_qp = self._flops_per_elem_qp(element_type, operator)
        bytes_per_elem_qp = self._bytes_per_elem_qp(element_type, dtype)
        base_flops = float(n_elements * n_qp) * flops_per_elem_qp
        base_bytes = float(n_elements * n_qp) * bytes_per_elem_qp

        nshape = fem_nshape(element_type)
        n_fma = int(max(2048, min(int(self.max_n_fma), n_elements * nshape)))
        iters_inner = int(max(32, min(int(self.max_inner_iters), (n_qp + 1) * 128)))
        size_bytes = int(
            max(
                4 * 1024 * 1024,
                min(int(self.max_buffer_mb) * 1024 * 1024, int(base_bytes / 4.0)),
            )
        )
        mem_iters = int(max(1, min(int(self.max_mem_iters), n_qp + 1)))

        if self.adapter.run_fma is None:
            raise RuntimeError(f"Backend {self.adapter.backend} has no FMA primitive")
        fma_elapsed = float(self.adapter.run_fma(n_fma, iters_inner))

        transfer_used = "device_to_device"
        mem_elapsed = 0.0
        moved_bytes = 0.0
        if self.adapter.run_mem_d2d is not None:
            mem_elapsed = float(self.adapter.run_mem_d2d(size_bytes, mem_iters))
            moved_bytes = float(2 * size_bytes * mem_iters)
        elif self.adapter.run_mem_h2d is not None:
            transfer_used = "host_to_device"
            mem_elapsed = float(self.adapter.run_mem_h2d(size_bytes, mem_iters))
            moved_bytes = float(size_bytes * mem_iters)
        elif self.adapter.run_mem_d2h is not None:
            transfer_used = "device_to_host"
            mem_elapsed = float(self.adapter.run_mem_d2h(size_bytes, mem_iters))
            moved_bytes = float(size_bytes * mem_iters)
        else:
            raise RuntimeError(f"Backend {self.adapter.backend} has no memory primitive")

        elapsed = max(1e-12, (fma_elapsed + mem_elapsed) * operator_elapsed_multiplier(element_type, operator))
        gflops = base_flops / elapsed / 1e9
        gbps = base_bytes / elapsed / 1e9

        self.last_details = {
            "n_fma": int(n_fma),
            "iters_inner": int(iters_inner),
            "mem_size_bytes": int(size_bytes),
            "mem_iters": int(mem_iters),
            "transfer_used": transfer_used,
            "primitive_fma_elapsed_s": float(fma_elapsed),
            "primitive_mem_elapsed_s": float(mem_elapsed),
            "primitive_mem_moved_bytes": float(moved_bytes),
            "caps": {
                "max_n_fma": int(self.max_n_fma),
                "max_buffer_mb": int(self.max_buffer_mb),
                "max_mem_iters": int(self.max_mem_iters),
                "max_inner_iters": int(self.max_inner_iters),
            },
        }
        return elapsed, gflops, gbps

    def fem_integration_tet4(
        self,
        n_elements: int,
        n_qp: int = 4,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> tuple[float, float, float]:
        return self._run(
            n_elements=n_elements,
            n_qp=n_qp,
            element_type="tet4",
            operator=operator.lower(),
            dtype=dtype.lower(),
        )

    def fem_integration_hex8(
        self,
        n_elements: int,
        n_qp: int = 8,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> tuple[float, float, float]:
        return self._run(
            n_elements=n_elements,
            n_qp=n_qp,
            element_type="hex8",
            operator=operator.lower(),
            dtype=dtype.lower(),
        )

    def fem_integration_prism6(
        self,
        n_elements: int,
        n_qp: int = 6,
        operator: str = "laplace",
        dtype: str = "float32",
    ) -> tuple[float, float, float]:
        return self._run(
            n_elements=n_elements,
            n_qp=n_qp,
            element_type="prism6",
            operator=operator.lower(),
            dtype=dtype.lower(),
        )

    def assembly_like(
        self,
        n_elements: int,
        n_qp: int = 4,
        n_dofs: int = 6,
        variant: str = "qss",
        use_workspace: int = 1,
        scatter_accumulate: int = 1,
        padding: int = 0,
        dtype: str = "float32",
    ) -> tuple[float, float, float, float]:
        mode = str(variant).strip().lower()
        if mode not in _SUPPORTED_ASSEMBLY_VARIANTS:
            raise ValueError(f"Unsupported assembly variant: {variant}")

        nd = max(2, min(int(n_dofs), 16))
        nq = max(1, min(int(n_qp), 16))
        ne = max(1, int(n_elements))
        ws = int(bool(use_workspace))
        scatter = int(bool(scatter_accumulate))
        pad = int(bool(padding))
        dtype_eff = "float64" if dtype == "float64" and self.supports_fp64 else "float32"

        flops = float(ne * nq) * float(
            assembly_like_flops_per_elem_qp(
                nd,
                mode,
                use_workspace=bool(ws),
                scatter=bool(scatter),
            )
        )
        bytes_moved = float(ne * nq) * float(
            assembly_like_bytes_per_elem_qp(
                nd,
                dtype_eff,
                use_workspace=bool(ws),
                scatter=bool(scatter),
                padding=bool(pad),
            )
        )

        n_fma = int(max(2048, min(int(self.max_n_fma), ne * nd)))
        iters_inner = int(max(64, min(int(self.max_inner_iters), nq * nd * 8)))
        size_bytes = int(
            max(
                2 * 1024 * 1024,
                min(int(self.max_buffer_mb) * 1024 * 1024, int(max(bytes_moved / 4.0, 1.0))),
            )
        )
        mem_iters = int(max(1, min(int(self.max_mem_iters), nq + ws + scatter + 1)))

        if self.adapter.run_fma is None:
            raise RuntimeError(f"Backend {self.adapter.backend} has no FMA primitive")
        fma_elapsed = float(self.adapter.run_fma(n_fma, iters_inner))

        transfer_used = "device_to_device"
        mem_elapsed = 0.0
        moved_bytes = 0.0
        if self.adapter.run_mem_d2d is not None:
            mem_elapsed = float(self.adapter.run_mem_d2d(size_bytes, mem_iters))
            moved_bytes = float(2 * size_bytes * mem_iters)
        elif self.adapter.run_mem_h2d is not None:
            transfer_used = "host_to_device"
            mem_elapsed = float(self.adapter.run_mem_h2d(size_bytes, mem_iters))
            moved_bytes = float(size_bytes * mem_iters)
        elif self.adapter.run_mem_d2h is not None:
            transfer_used = "device_to_host"
            mem_elapsed = float(self.adapter.run_mem_d2h(size_bytes, mem_iters))
            moved_bytes = float(size_bytes * mem_iters)
        else:
            raise RuntimeError(f"Backend {self.adapter.backend} has no memory primitive")

        elapsed = max(1e-12, (fma_elapsed + mem_elapsed))
        elapsed *= assembly_variant_multiplier(mode)
        if ws:
            elapsed *= 1.10
        if scatter:
            elapsed *= 1.07
        if pad:
            elapsed *= 1.03

        gflops = flops / elapsed / 1e9
        gbps = bytes_moved / elapsed / 1e9
        ai = flops / max(bytes_moved, 1.0)
        self.last_details = {
            "n_fma": int(n_fma),
            "iters_inner": int(iters_inner),
            "mem_size_bytes": int(size_bytes),
            "mem_iters": int(mem_iters),
            "transfer_used": transfer_used,
            "primitive_fma_elapsed_s": float(fma_elapsed),
            "primitive_mem_elapsed_s": float(mem_elapsed),
            "primitive_mem_moved_bytes": float(moved_bytes),
            "requested_n_dofs": int(nd),
            "requested_n_qp": int(nq),
            "variant": mode,
            "caps": {
                "max_n_fma": int(self.max_n_fma),
                "max_buffer_mb": int(self.max_buffer_mb),
                "max_mem_iters": int(self.max_mem_iters),
                "max_inner_iters": int(self.max_inner_iters),
            },
        }
        return elapsed, gflops, gbps, ai
