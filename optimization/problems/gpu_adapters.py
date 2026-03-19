from __future__ import annotations

from dataclasses import dataclass
from typing import Callable


@dataclass
class GpuBackendAdapter:
    backend: str
    device_name: str
    device_index: int
    run_mem_d2d: Callable[[int, int], float] | None
    run_mem_h2d: Callable[[int, int], float] | None
    run_mem_d2h: Callable[[int, int], float] | None
    run_fma: Callable[[int, int], float] | None
    global_mem_bytes: int | None = None
    max_workgroup_size: int | None = None
    supports_fp64: bool = False
    preferred_workgroup_sizes: tuple[int, ...] = (32, 64, 128, 256)


def init_gpu_adapter(backend: str, device_index: int) -> GpuBackendAdapter:
    b = backend.lower().strip()

    if b == "cuda":
        from gpu.cuda.cuda_backend import (
            init_cuda,
            get_device_info,
            cuda_memcpy_bandwidth,
            cuda_memcpy_h2d_bandwidth,
            cuda_memcpy_d2h_bandwidth,
            cuda_fma_throughput,
        )

        ctx = init_cuda(device_index)
        info = get_device_info(ctx)

        return GpuBackendAdapter(
            backend="cuda",
            device_name=info.name,
            device_index=device_index,
            run_mem_d2d=lambda size_bytes, iters: cuda_memcpy_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_mem_h2d=lambda size_bytes, iters: cuda_memcpy_h2d_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_mem_d2h=lambda size_bytes, iters: cuda_memcpy_d2h_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_fma=lambda n_elements, iters_inner: cuda_fma_throughput(ctx, n=n_elements, iters_inner=iters_inner),
            global_mem_bytes=int(info.global_mem_bytes),
            max_workgroup_size=1024,
            supports_fp64=True,
            preferred_workgroup_sizes=(32, 64, 128, 256, 512),
        )

    if b == "hip":
        from gpu.hip.hip_backend import (
            init_hip,
            get_device_info,
            hip_memcpy_bandwidth,
            hip_memcpy_h2d_bandwidth,
            hip_memcpy_d2h_bandwidth,
            hip_fma_throughput,
        )

        ctx = init_hip(device_index)
        info = get_device_info(ctx)

        return GpuBackendAdapter(
            backend="hip",
            device_name=info.name,
            device_index=device_index,
            run_mem_d2d=lambda size_bytes, iters: hip_memcpy_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_mem_h2d=lambda size_bytes, iters: hip_memcpy_h2d_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_mem_d2h=lambda size_bytes, iters: hip_memcpy_d2h_bandwidth(ctx, size_bytes=size_bytes, iters=iters),
            run_fma=lambda n_elements, iters_inner: hip_fma_throughput(ctx, n_elements=n_elements, iters_inner=iters_inner),
            global_mem_bytes=int(info.global_mem_bytes),
            max_workgroup_size=1024,
            supports_fp64=True,
            preferred_workgroup_sizes=(64, 128, 256, 512),
        )

    if b == "opencl":
        from gpu.opencl.opencl_backend import (
            init_opencl,
            get_device_name,
            opencl_mem_copy,
            opencl_memcpy_h2d_bandwidth,
            opencl_memcpy_d2h_bandwidth,
            opencl_fma_throughput,
        )

        ctx = init_opencl(device_index)
        dev_name = get_device_name(device_index)
        dev = ctx.device
        max_wg = int(getattr(dev, "max_work_group_size", 256))
        glb = int(getattr(dev, "global_mem_size", 0) or 0)
        exts = str(getattr(dev, "extensions", "")).lower()
        supports_fp64 = ("cl_khr_fp64" in exts) or ("cl_amd_fp64" in exts)

        def _d2d(size_bytes: int, iters: int) -> float:
            n = max(1, size_bytes // 4)
            # opencl_mem_copy is one kernel launch; emulate iters via repeated calls.
            total = 0.0
            for _ in range(max(1, iters)):
                total += opencl_mem_copy(ctx, n)
            return total

        return GpuBackendAdapter(
            backend="opencl",
            device_name=dev_name,
            device_index=device_index,
            run_mem_d2d=_d2d,
            run_mem_h2d=lambda size_bytes, iters: opencl_memcpy_h2d_bandwidth(ctx, bytes_size=size_bytes, iters=iters),
            run_mem_d2h=lambda size_bytes, iters: opencl_memcpy_d2h_bandwidth(ctx, bytes_size=size_bytes, iters=iters),
            run_fma=lambda n_elements, iters_inner: opencl_fma_throughput(ctx, n=n_elements, iters_inner=iters_inner),
            global_mem_bytes=glb if glb > 0 else None,
            max_workgroup_size=max(1, max_wg),
            supports_fp64=bool(supports_fp64),
            preferred_workgroup_sizes=(32, 64, 128, 256),
        )

    if b == "metal":
        from gpu.metal.metal_backend import MetalBackend

        ctx = MetalBackend(device_index=device_index)
        glb = None
        try:
            rws = getattr(ctx.device, "recommendedMaxWorkingSetSize", None)
            if callable(rws):
                glb = int(rws())
            elif rws is not None:
                glb = int(rws)
        except Exception:
            glb = None

        def _d2d(size_bytes: int, iters: int) -> float:
            n = max(1, size_bytes // 4)
            total = 0.0
            for _ in range(max(1, iters)):
                total += ctx.run_mem_copy(n)
            return total

        return GpuBackendAdapter(
            backend="metal",
            device_name=ctx.device_name,
            device_index=device_index,
            run_mem_d2d=_d2d,
            run_mem_h2d=None,
            run_mem_d2h=None,
            run_fma=lambda n_elements, iters_inner: ctx.run_fma(n_elements, iters_inner),
            global_mem_bytes=glb,
            max_workgroup_size=1024,
            supports_fp64=False,
            preferred_workgroup_sizes=(32, 64, 128, 256),
        )

    raise ValueError(f"Unsupported backend: {backend}")
