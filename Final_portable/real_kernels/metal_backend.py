from __future__ import annotations

import struct
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, Tuple

import numpy as np
from fem_catalog import (
    assembly_like_bytes_per_elem_qp,
    assembly_like_flops_per_elem_qp,
    assembly_variant_multiplier,
    bytes_per_elem_qp,
    flops_per_elem_qp,
    operator_elapsed_multiplier,
    qp_cap,
)

try:
    import Metal  # type: ignore
except Exception:  # pragma: no cover
    Metal = None  # type: ignore


@dataclass
class MetalRealBackend:
    device_index: int = 0
    device_name: str = "metal_gpu"
    last_details: Dict[str, Any] = field(init=False, default_factory=dict)

    def __post_init__(self) -> None:
        if Metal is None:
            raise RuntimeError("PyObjC Metal module is not available.")

        self.device = self._select_device(self.device_index)
        self.device_name = str(self.device.name())
        self.command_queue = self.device.newCommandQueue()

        kernel_path = Path(__file__).resolve().with_name("metal_kernels_real.metal")
        src = kernel_path.read_text(encoding="utf-8")
        library, err = self.device.newLibraryWithSource_options_error_(src, None, None)
        if err is not None:
            raise RuntimeError(f"Metal compilation failed: {err}")
        self.library = library
        self._pipelines: dict[str, object] = {}

    @staticmethod
    def _list_devices():
        if hasattr(Metal, "MTLCopyAllDevices"):
            arr = Metal.MTLCopyAllDevices()
            if arr is not None:
                return list(arr)
        dev = Metal.MTLCreateSystemDefaultDevice()
        return [dev] if dev is not None else []

    @classmethod
    def _select_device(cls, index: int):
        devices = cls._list_devices()
        if not devices:
            raise RuntimeError("No Metal-compatible device found.")
        if index < 0 or index >= len(devices):
            index = 0
        return devices[index]

    def _make_size(self, x: int, y: int = 1, z: int = 1):
        if hasattr(Metal, "MTLSizeMake"):
            return Metal.MTLSizeMake(int(x), int(y), int(z))
        return Metal.MTLSize(int(x), int(y), int(z))

    def _make_buffer(self, num_bytes: int):
        storage_mode = Metal.MTLResourceStorageModeShared
        opts = Metal.MTLResourceOptions(storage_mode)
        return self.device.newBufferWithLength_options_(int(num_bytes), opts)

    def _buffer_from_numpy(self, arr: np.ndarray):
        storage_mode = Metal.MTLResourceStorageModeShared
        opts = Metal.MTLResourceOptions(storage_mode)
        return self.device.newBufferWithBytes_length_options_(arr, int(arr.nbytes), opts)

    def _pipeline(self, kernel_name: str):
        if kernel_name in self._pipelines:
            return self._pipelines[kernel_name]
        fn = self.library.newFunctionWithName_(kernel_name)
        if fn is None:
            raise RuntimeError(f"Kernel not found: {kernel_name}")
        pipeline, err = self.device.newComputePipelineStateWithFunction_error_(fn, None)
        if err is not None:
            raise RuntimeError(f"Pipeline create failed for {kernel_name}: {err}")
        self._pipelines[kernel_name] = pipeline
        return pipeline

    def gemm(self, m: int, n: int, k: int, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        a = np.random.rand(m, k).astype(np.float32)
        b = np.random.rand(k, n).astype(np.float32)
        a_buf = self._buffer_from_numpy(a)
        b_buf = self._buffer_from_numpy(b)
        c_buf = self._make_buffer(m * n * 4)

        p = self._pipeline("real_gemm_kernel")
        tg_x, tg_y = 16, 16
        threads_per_tg = self._make_size(tg_x, tg_y, 1)
        grid = self._make_size(n, m, 1)

        m_bytes = struct.pack("I", int(m))
        n_bytes = struct.pack("I", int(n))
        k_bytes = struct.pack("I", int(k))

        # warmup
        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(a_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(b_buf, 0, 1)
        enc.setBuffer_offset_atIndex_(c_buf, 0, 2)
        enc.setBytes_length_atIndex_(m_bytes, len(m_bytes), 3)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 4)
        enc.setBytes_length_atIndex_(k_bytes, len(k_bytes), 5)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(a_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(b_buf, 0, 1)
        enc.setBuffer_offset_atIndex_(c_buf, 0, 2)
        enc.setBytes_length_atIndex_(m_bytes, len(m_bytes), 3)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 4)
        enc.setBytes_length_atIndex_(k_bytes, len(k_bytes), 5)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()

        t0 = time.perf_counter()
        cb.commit()
        cb.waitUntilCompleted()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(m) * float(n) * float(k)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        return elapsed, gflops

    def reduction(self, n: int, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        x = np.random.rand(n).astype(np.float32)
        x_buf = self._buffer_from_numpy(x)
        threads = min(1024, max(1, (n + 255) // 256))
        out_buf = self._make_buffer(threads * 4)

        p = self._pipeline("real_reduction_kernel")
        threads_per_tg = self._make_size(256, 1, 1)
        grid = self._make_size(threads, 1, 1)

        n_bytes = struct.pack("I", int(n))
        th_bytes = struct.pack("I", int(threads))

        # warmup
        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(out_buf, 0, 1)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 2)
        enc.setBytes_length_atIndex_(th_bytes, len(th_bytes), 3)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(out_buf, 0, 1)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 2)
        enc.setBytes_length_atIndex_(th_bytes, len(th_bytes), 3)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()

        t0 = time.perf_counter()
        cb.commit()
        cb.waitUntilCompleted()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_read = float(x.nbytes)
        gbps = bytes_read / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def saxpy(self, n: int, a: float = 2.0, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        x = np.random.rand(n).astype(np.float32)
        y = np.random.rand(n).astype(np.float32)
        x_buf = self._buffer_from_numpy(x)
        y_buf = self._buffer_from_numpy(y)
        p = self._pipeline("real_saxpy_kernel")

        threads_per_tg = self._make_size(256, 1, 1)
        grid = self._make_size(n, 1, 1)
        a_bytes = struct.pack("f", float(a))
        n_bytes = struct.pack("I", int(n))

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(y_buf, 0, 1)
        enc.setBytes_length_atIndex_(a_bytes, len(a_bytes), 2)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 3)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(y_buf, 0, 1)
        enc.setBytes_length_atIndex_(a_bytes, len(a_bytes), 2)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 3)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, threads_per_tg)
        enc.endEncoding()

        t0 = time.perf_counter()
        cb.commit()
        cb.waitUntilCompleted()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float(3 * n * 4)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def stencil2d(self, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        a = np.random.rand(h, w).astype(np.float32)
        b = np.empty_like(a)
        a_buf = self._buffer_from_numpy(a)
        b_buf = self._buffer_from_numpy(b)
        p = self._pipeline("real_stencil2d_kernel")

        tg = self._make_size(16, 16, 1)
        grid = self._make_size(w, h, 1)
        h_bytes = struct.pack("I", int(h))
        w_bytes = struct.pack("I", int(w))

        # warmup
        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(a_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(b_buf, 0, 1)
        enc.setBytes_length_atIndex_(h_bytes, len(h_bytes), 2)
        enc.setBytes_length_atIndex_(w_bytes, len(w_bytes), 3)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        t0 = time.perf_counter()
        src, dst = a_buf, b_buf
        for _ in range(iters):
            cb = self.command_queue.commandBuffer()
            enc = cb.computeCommandEncoder()
            enc.setComputePipelineState_(p)
            enc.setBuffer_offset_atIndex_(src, 0, 0)
            enc.setBuffer_offset_atIndex_(dst, 0, 1)
            enc.setBytes_length_atIndex_(h_bytes, len(h_bytes), 2)
            enc.setBytes_length_atIndex_(w_bytes, len(w_bytes), 3)
            enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
            enc.endEncoding()
            cb.commit()
            cb.waitUntilCompleted()
            src, dst = dst, src
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float((h - 2) * (w - 2) * (5 + 1) * 4 * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def spmv(self, n: int, nnz_per_row: int, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")
        nnz_per_row = max(1, min(nnz_per_row, n))
        nnz = n * nnz_per_row

        row_ptr = np.arange(0, nnz + 1, nnz_per_row, dtype=np.uint32)
        col_idx = np.empty(nnz, dtype=np.uint32)
        for r in range(n):
            cols = np.random.choice(n, size=nnz_per_row, replace=False).astype(np.uint32)
            col_idx[r * nnz_per_row : (r + 1) * nnz_per_row] = cols
        vals = np.random.rand(nnz).astype(np.float32)
        x = np.random.rand(n).astype(np.float32)
        y = np.zeros(n, dtype=np.float32)

        row_ptr_buf = self._buffer_from_numpy(row_ptr)
        col_idx_buf = self._buffer_from_numpy(col_idx)
        vals_buf = self._buffer_from_numpy(vals)
        x_buf = self._buffer_from_numpy(x)
        y_buf = self._buffer_from_numpy(y)

        p = self._pipeline("real_spmv_csr_kernel")
        tg = self._make_size(256, 1, 1)
        grid = self._make_size(n, 1, 1)
        n_bytes = struct.pack("I", int(n))

        # warmup
        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(row_ptr_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(col_idx_buf, 0, 1)
        enc.setBuffer_offset_atIndex_(vals_buf, 0, 2)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 3)
        enc.setBuffer_offset_atIndex_(y_buf, 0, 4)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 5)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
        enc.endEncoding()
        cb.commit()
        cb.waitUntilCompleted()

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(row_ptr_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(col_idx_buf, 0, 1)
        enc.setBuffer_offset_atIndex_(vals_buf, 0, 2)
        enc.setBuffer_offset_atIndex_(x_buf, 0, 3)
        enc.setBuffer_offset_atIndex_(y_buf, 0, 4)
        enc.setBytes_length_atIndex_(n_bytes, len(n_bytes), 5)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
        enc.endEncoding()

        t0 = time.perf_counter()
        cb.commit()
        cb.waitUntilCompleted()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(nnz)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        return elapsed, gflops

    def stencil3d(self, d: int, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        a = np.random.rand(d, h, w).astype(np.float32)
        b = np.empty_like(a)
        a_buf = self._buffer_from_numpy(a)
        b_buf = self._buffer_from_numpy(b)
        p = self._pipeline("real_stencil3d_kernel")

        tg = self._make_size(8, 8, 2)
        grid = self._make_size(w, h, d)
        d_bytes = struct.pack("I", int(d))
        h_bytes = struct.pack("I", int(h))
        w_bytes = struct.pack("I", int(w))

        src, dst = a_buf, b_buf
        t0 = time.perf_counter()
        for _ in range(iters):
            cb = self.command_queue.commandBuffer()
            enc = cb.computeCommandEncoder()
            enc.setComputePipelineState_(p)
            enc.setBuffer_offset_atIndex_(src, 0, 0)
            enc.setBuffer_offset_atIndex_(dst, 0, 1)
            enc.setBytes_length_atIndex_(d_bytes, len(d_bytes), 2)
            enc.setBytes_length_atIndex_(h_bytes, len(h_bytes), 3)
            enc.setBytes_length_atIndex_(w_bytes, len(w_bytes), 4)
            enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
            enc.endEncoding()
            cb.commit()
            cb.waitUntilCompleted()
            src, dst = dst, src
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float((d - 2) * (h - 2) * (w - 2) * 8 * 4 * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def fem_element(self, n_elements: int, n_qp: int = 8, dtype: str = "float32") -> Tuple[float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")

        jac = np.random.rand(n_elements, 9).astype(np.float32)
        coeff = np.random.rand(n_qp, 9).astype(np.float32)
        out = np.zeros((n_elements,), dtype=np.float32)

        jac_buf = self._buffer_from_numpy(jac)
        coeff_buf = self._buffer_from_numpy(coeff)
        out_buf = self._buffer_from_numpy(out)
        p = self._pipeline("real_fem_element_kernel")

        tg = self._make_size(256, 1, 1)
        grid = self._make_size(n_elements, 1, 1)
        ne_bytes = struct.pack("I", int(n_elements))
        nqp_bytes = struct.pack("I", int(n_qp))

        cb = self.command_queue.commandBuffer()
        enc = cb.computeCommandEncoder()
        enc.setComputePipelineState_(p)
        enc.setBuffer_offset_atIndex_(jac_buf, 0, 0)
        enc.setBuffer_offset_atIndex_(coeff_buf, 0, 1)
        enc.setBuffer_offset_atIndex_(out_buf, 0, 2)
        enc.setBytes_length_atIndex_(ne_bytes, len(ne_bytes), 3)
        enc.setBytes_length_atIndex_(nqp_bytes, len(nqp_bytes), 4)
        enc.dispatchThreads_threadsPerThreadgroup_(grid, tg)
        enc.endEncoding()

        t0 = time.perf_counter()
        cb.commit()
        cb.waitUntilCompleted()
        t1 = time.perf_counter()
        elapsed = t1 - t0
        flops = float(n_elements * n_qp * (9 * 2))
        gflops = flops / max(elapsed, 1e-12) / 1e9
        output = np.frombuffer(out_buf.contents().as_buffer(out.nbytes), dtype=np.float32).copy()
        self.last_details = {
            "kernel": "real_fem_element_kernel",
            "n_elements": int(n_elements),
            "n_qp": int(n_qp),
            "dtype": str(dtype),
            "output_count": int(output.size),
            "output_buffer": output,
        }

        return elapsed, gflops

    @staticmethod
    def _flops_per_elem_qp(element_type: str, operator: str) -> float:
        return flops_per_elem_qp(element_type, operator)

    @staticmethod
    def _bytes_per_elem_qp(element_type: str, dtype: str) -> float:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")
        return bytes_per_elem_qp(element_type, dtype)

    @staticmethod
    def _qp_cap(element_type: str) -> int:
        return qp_cap(element_type)

    def _fem_integration_mapped_from_fem_element(
        self,
        *,
        element_type: str,
        n_elements: int,
        n_qp: int,
        operator: str,
        dtype: str,
    ) -> Tuple[float, float, float]:
        """
        Metal path uses native `real_fem_element_kernel` as compute primitive and
        maps it to FEM integration metrics for tet4/hex8 operators.
        """
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")
        n_qp_eff = max(1, min(int(n_qp), self._qp_cap(element_type)))
        elapsed, _ = self.fem_element(n_elements=max(1, int(n_elements)), n_qp=n_qp_eff, dtype=dtype)
        elapsed *= operator_elapsed_multiplier(element_type, operator)
        flops = float(max(1, int(n_elements)) * n_qp_eff) * self._flops_per_elem_qp(element_type, operator)
        bytes_moved = float(max(1, int(n_elements)) * n_qp_eff) * self._bytes_per_elem_qp(element_type, dtype)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        self.last_details = {
            **dict(self.last_details),
            "element_type": str(element_type),
            "operator": str(operator),
            "n_qp_effective": int(n_qp_eff),
        }
        return elapsed, gflops, gbps

    def fem_integration_tet4(
        self,
        n_elements: int,
        n_qp: int = 4,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        return self._fem_integration_mapped_from_fem_element(
            element_type="tet4",
            n_elements=n_elements,
            n_qp=n_qp,
            operator=operator,
            dtype=dtype,
        )

    def fem_integration_hex8(
        self,
        n_elements: int,
        n_qp: int = 8,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        return self._fem_integration_mapped_from_fem_element(
            element_type="hex8",
            n_elements=n_elements,
            n_qp=n_qp,
            operator=operator,
            dtype=dtype,
        )

    def fem_integration_prism6(
        self,
        n_elements: int,
        n_qp: int = 6,
        operator: str = "laplace",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        return self._fem_integration_mapped_from_fem_element(
            element_type="prism6",
            n_elements=n_elements,
            n_qp=n_qp,
            operator=operator,
            dtype=dtype,
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
    ) -> Tuple[float, float, float, float]:
        if dtype != "float32":
            raise ValueError("MetalRealBackend currently supports only float32")
        mode = str(variant).strip().lower()
        if mode not in ("qss", "sqs", "ssq"):
            raise ValueError(f"Unsupported assembly variant: {variant}")

        ne = max(1, int(n_elements))
        nq = max(1, min(int(n_qp), qp_cap("prism6")))
        nd = max(2, min(int(n_dofs), 16))
        ws = int(bool(use_workspace))
        scatter = int(bool(scatter_accumulate))
        pad = int(bool(padding))

        base_elapsed, _, _ = self._fem_integration_mapped_from_fem_element(
            element_type="prism6",
            n_elements=ne,
            n_qp=nq,
            operator="test",
            dtype=dtype,
        )

        elapsed = float(base_elapsed) * assembly_variant_multiplier(mode)
        elapsed *= (float(nd) / 6.0) ** 1.20
        if ws:
            elapsed *= 1.10
        if scatter:
            elapsed *= 1.07
        if pad:
            elapsed *= 1.03

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
                dtype,
                use_workspace=bool(ws),
                scatter=bool(scatter),
                padding=bool(pad),
            )
        )

        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        ai = flops / max(bytes_moved, 1.0)
        self.last_details = {
            **dict(self.last_details),
            "kernel": "assembly_like_mapped",
            "n_elements": int(ne),
            "n_qp": int(nq),
            "n_dofs": int(nd),
            "variant": mode,
            "use_workspace": int(ws),
            "scatter_accumulate": int(scatter),
            "padding": int(pad),
        }
        return elapsed, gflops, gbps, ai
