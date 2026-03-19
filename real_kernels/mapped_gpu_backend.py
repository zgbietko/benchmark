from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict

from optimization.problems.gpu_adapters import GpuBackendAdapter, init_gpu_adapter


_SUPPORTED_OPERATORS = (
    "diffusion",
    "mass",
    "convection",
    "diffusion_mass",
    "diffusion_convection_mass",
)


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
        if operator not in _SUPPORTED_OPERATORS:
            raise ValueError(f"Unsupported operator: {operator}")
        if element_type == "tet4":
            table = {
                "diffusion": 330.0,
                "mass": 120.0,
                "convection": 210.0,
                "diffusion_mass": 450.0,
                "diffusion_convection_mass": 660.0,
            }
            return table[operator]
        if element_type == "hex8":
            table = {
                "diffusion": 1200.0,
                "mass": 420.0,
                "convection": 820.0,
                "diffusion_mass": 1620.0,
                "diffusion_convection_mass": 2440.0,
            }
            return table[operator]
        raise ValueError(f"Unsupported element_type: {element_type}")

    @staticmethod
    def _bytes_per_elem_qp(element_type: str, dtype: str) -> float:
        itemsize = 4.0 if dtype == "float32" else 8.0
        if element_type == "tet4":
            return float((4 * 3 + 4 * 4) * itemsize)
        if element_type == "hex8":
            return float((8 * 3 + 8 * 8) * itemsize)
        raise ValueError(f"Unsupported element_type: {element_type}")

    @staticmethod
    def _qp_cap(element_type: str) -> int:
        if element_type == "tet4":
            return 4
        if element_type == "hex8":
            return 8
        return 1

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

        nshape = 4 if element_type == "tet4" else 8
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

        elapsed = max(1e-12, fma_elapsed + mem_elapsed)
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
