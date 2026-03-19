from __future__ import annotations

from contextlib import nullcontext
import time
from pathlib import Path
from typing import List

import Metal
import numpy as np

try:
    import objc  # type: ignore
except Exception:
    objc = None  # type: ignore


class MetalBackend:
    """
    Prosty wrapper na Metal compute API dla mikrobenchmarków.
    - ładuje kernele z gpu/metal/kernels.metal,
    - udostępnia run_mem_copy i run_fma.
    """

    def __init__(self, device_index: int = 0, kernel_file: str | None = None) -> None:
        self.device = self._select_device(device_index)
        self.device_index = device_index
        self.device_name = str(self.device.name())

        self.command_queue = self.device.newCommandQueue()

        if kernel_file is None:
            kernel_path = Path(__file__).resolve().with_name("kernels.metal")
        else:
            kernel_path = Path(kernel_file)

        source = kernel_path.read_text(encoding="utf-8")
        library, err = self.device.newLibraryWithSource_options_error_(source, None, None)
        if err is not None:
            raise RuntimeError(f"Failed to compile Metal library: {err}")

        self.library = library
        self._pipelines = {}
        # Reuse memory-copy buffers to avoid large repeated allocations.
        self._mem_copy_bytes = 0
        self._mem_copy_src = None
        self._mem_copy_dst = None
        # Reuse FMA buffers to avoid repeated large allocations each evaluation.
        self._fma_bytes = 0
        self._fma_a = None
        self._fma_b = None
        self._fma_c = None

    # ------------------------------------------------------------------
    # Device utilities
    # ------------------------------------------------------------------
    @staticmethod
    def _list_devices() -> List["Metal.MTLDevice"]:
        """
        Zwraca listę wszystkich urządzeń Metal.
        Na Apple Silicon zwykle jest jedno zintegrowane GPU.
        """
        devices = []
        if hasattr(Metal, "MTLCopyAllDevices"):
            arr = Metal.MTLCopyAllDevices()
            if arr is not None:
                devices = list(arr)
        else:
            dev = Metal.MTLCreateSystemDefaultDevice()
            if dev is not None:
                devices = [dev]
        return devices

    @classmethod
    def _select_device(cls, index: int):
        devices = cls._list_devices()
        if not devices:
            raise RuntimeError("No Metal-compatible GPU device found.")
        if index < 0 or index >= len(devices):
            # Clamp do poprawnego zakresu
            index = max(0, min(index, len(devices) - 1))
        return devices[index]

    # ------------------------------------------------------------------
    # Pipeline helpers
    # ------------------------------------------------------------------
    def _get_pipeline(self, kernel_name: str):
        if kernel_name in self._pipelines:
            return self._pipelines[kernel_name]

        fn = self.library.newFunctionWithName_(kernel_name)
        if fn is None:
            raise RuntimeError(f"Kernel '{kernel_name}' not found in Metal library.")

        pipeline, err = self.device.newComputePipelineStateWithFunction_error_(fn, None)
        if err is not None:
            raise RuntimeError(f"Failed to create pipeline for '{kernel_name}': {err}")

        self._pipelines[kernel_name] = pipeline
        return pipeline

    # ------------------------------------------------------------------
    # Buffer helpers
    # ------------------------------------------------------------------
    def _make_size(self, x: int, y: int = 1, z: int = 1):
        # PyObjC udostępnia MTLSizeMake; fallback na konstruktor jeśli trzeba
        if hasattr(Metal, "MTLSizeMake"):
            return Metal.MTLSizeMake(x, y, z)
        return Metal.MTLSize(x, y, z)

    def _make_buffer(self, num_bytes: int):
        storage_mode = Metal.MTLResourceStorageModeShared
        opts = Metal.MTLResourceOptions(storage_mode)
        return self.device.newBufferWithLength_options_(num_bytes, opts)

    def _buffer_from_numpy(self, arr: np.ndarray):
        storage_mode = Metal.MTLResourceStorageModeShared
        opts = Metal.MTLResourceOptions(storage_mode)
        # Metal może czytać bezpośrednio z bufora NumPy
        return self.device.newBufferWithBytes_length_options_(arr, arr.nbytes, opts)

    def _autorelease_pool(self):
        if objc is not None and hasattr(objc, "autorelease_pool"):
            return objc.autorelease_pool()
        return nullcontext()

    # ------------------------------------------------------------------
    # Public API for microbenchmarks
    # ------------------------------------------------------------------
    def run_mem_copy(self, num_elements: int) -> float:
        """
        Pojedyncze uruchomienie mem_copy_kernel na num_elements elementach fp32.
        Zwraca czas (sekundy, wall-clock: commit+wait).
        """
        pipeline = self._get_pipeline("mem_copy_kernel")

        with self._autorelease_pool():
            # Buffer wejściowy/wyjściowy.
            # Nie tworzymy za każdym razem dużych tablic NumPy - to powodowało
            # nadmierną presję pamięci i ubijanie procesu na długich kampaniach.
            nbytes = max(4, int(num_elements) * 4)
            if (
                self._mem_copy_src is None
                or self._mem_copy_dst is None
                or self._mem_copy_bytes != nbytes
            ):
                self._mem_copy_src = self._make_buffer(nbytes)
                self._mem_copy_dst = self._make_buffer(nbytes)
                self._mem_copy_bytes = nbytes

            src_buf = self._mem_copy_src
            dst_buf = self._mem_copy_dst

            threads_per_threadgroup = self._make_size(256, 1, 1)
            grid_size = self._make_size(num_elements, 1, 1)

            command_buffer = self.command_queue.commandBuffer()
            encoder = command_buffer.computeCommandEncoder()
            encoder.setComputePipelineState_(pipeline)
            encoder.setBuffer_offset_atIndex_(src_buf, 0, 0)
            encoder.setBuffer_offset_atIndex_(dst_buf, 0, 1)
            encoder.dispatchThreads_threadsPerThreadgroup_(grid_size, threads_per_threadgroup)
            encoder.endEncoding()

            t0 = time.perf_counter()
            command_buffer.commit()
            command_buffer.waitUntilCompleted()
            t1 = time.perf_counter()

            return t1 - t0

    def run_fma(self, num_elements: int, inner_iters: int) -> float:
        """
        Pojedyncze uruchomienie fma_kernel na num_elements elementach.
        inner_iters – liczba operacji FMA na element.
        Zwraca czas (sekundy, wall-clock: commit+wait).
        """
        import struct

        pipeline = self._get_pipeline("fma_kernel")

        with self._autorelease_pool():
            nbytes = max(4, int(num_elements) * 4)
            if (
                self._fma_a is None
                or self._fma_b is None
                or self._fma_c is None
                or self._fma_bytes != nbytes
            ):
                # Initialize once on size change; next calls reuse GPU buffers.
                a = np.empty(int(num_elements), dtype=np.float32)
                b = np.empty(int(num_elements), dtype=np.float32)
                a.fill(1.0)
                b.fill(2.0)
                self._fma_a = self._buffer_from_numpy(a)
                self._fma_b = self._buffer_from_numpy(b)
                self._fma_c = self._make_buffer(nbytes)
                self._fma_bytes = nbytes

            a_buf = self._fma_a
            b_buf = self._fma_b
            c_buf = self._fma_c

            threads_per_threadgroup = self._make_size(256, 1, 1)
            grid_size = self._make_size(num_elements, 1, 1)

            # inner_iters jako 32-bitowy uint
            inner_bytes = struct.pack("I", inner_iters)

            command_buffer = self.command_queue.commandBuffer()
            encoder = command_buffer.computeCommandEncoder()
            encoder.setComputePipelineState_(pipeline)
            encoder.setBuffer_offset_atIndex_(a_buf, 0, 0)
            encoder.setBuffer_offset_atIndex_(b_buf, 0, 1)
            encoder.setBuffer_offset_atIndex_(c_buf, 0, 2)
            encoder.setBytes_length_atIndex_(inner_bytes, len(inner_bytes), 3)
            encoder.dispatchThreads_threadsPerThreadgroup_(grid_size, threads_per_threadgroup)
            encoder.endEncoding()

            t0 = time.perf_counter()
            command_buffer.commit()
            command_buffer.waitUntilCompleted()
            t1 = time.perf_counter()

            return t1 - t0

    def run_pointer_chase(self, num_elements: int, iters_inner: int) -> float:
        import struct

        pipeline = self._get_pipeline("pointer_chase_kernel")

        with self._autorelease_pool():
            n = max(1, int(num_elements))
            stride = 17
            next_idx = np.empty(n, dtype=np.uint32)
            for i in range(n):
                next_idx[i] = (i + stride) % n

            next_buf = self._buffer_from_numpy(next_idx)
            out_buf = self._make_buffer(4)
            iters_bytes = struct.pack("I", int(iters_inner))

            threads_per_threadgroup = self._make_size(1, 1, 1)
            grid_size = self._make_size(1, 1, 1)

            # warmup
            cb = self.command_queue.commandBuffer()
            enc = cb.computeCommandEncoder()
            enc.setComputePipelineState_(pipeline)
            enc.setBuffer_offset_atIndex_(next_buf, 0, 0)
            enc.setBuffer_offset_atIndex_(out_buf, 0, 1)
            enc.setBytes_length_atIndex_(iters_bytes, len(iters_bytes), 2)
            enc.dispatchThreads_threadsPerThreadgroup_(grid_size, threads_per_threadgroup)
            enc.endEncoding()
            cb.commit()
            cb.waitUntilCompleted()

            cb = self.command_queue.commandBuffer()
            enc = cb.computeCommandEncoder()
            enc.setComputePipelineState_(pipeline)
            enc.setBuffer_offset_atIndex_(next_buf, 0, 0)
            enc.setBuffer_offset_atIndex_(out_buf, 0, 1)
            enc.setBytes_length_atIndex_(iters_bytes, len(iters_bytes), 2)
            enc.dispatchThreads_threadsPerThreadgroup_(grid_size, threads_per_threadgroup)
            enc.endEncoding()

            t0 = time.perf_counter()
            cb.commit()
            cb.waitUntilCompleted()
            t1 = time.perf_counter()
            return t1 - t0
