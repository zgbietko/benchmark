from __future__ import annotations

from dataclasses import dataclass
import time
from typing import Any, List

try:
    import pyopencl as cl  # type: ignore
    import numpy as np  # type: ignore
except Exception as e:  # pragma: no cover - optional dependency
    cl = None  # type: ignore
    np = None  # type: ignore
    _import_error = e
else:
    _import_error = None


@dataclass
class OpenCLContext:
    ctx: "cl.Context"
    queue: "cl.CommandQueue"
    device: "cl.Device"

    # Lazy caches to avoid per-call recompilation / reallocations.
    program_mem_copy: Any = None
    program_fma: Any = None
    program_pointer_chase: Any = None

    mem_copy_a: Any = None
    mem_copy_b: Any = None
    mem_copy_n: int = 0
    mem_copy_warmup_done: bool = False

    fma_a: Any = None
    fma_b: Any = None
    fma_c: Any = None
    fma_n: int = 0
    fma_warmup_done: bool = False

    h2d_host: Any = None
    h2d_buf: Any = None
    h2d_num_elements: int = 0

    d2h_host: Any = None
    d2h_out: Any = None
    d2h_buf: Any = None
    d2h_num_elements: int = 0

    pc_next_buf: Any = None
    pc_out_buf: Any = None
    pc_n: int = 0
    pc_warmup_done: bool = False


@dataclass
class OpenCLDeviceInfo:
    index: int
    platform_name: str
    platform_vendor: str
    device_name: str
    device_vendor: str
    device_type: str


def _require_opencl() -> None:
    if cl is None or np is None:
        raise RuntimeError(
            "PyOpenCL nie jest dostępny. Zainstaluj 'pyopencl' i sterowniki OpenCL."
        )


def _list_devices_flat() -> List["cl.Device"]:
    _require_opencl()
    devices: List[cl.Device] = []
    for platform in cl.get_platforms():
        try:
            devices.extend(platform.get_devices())
        except Exception:
            continue
    return devices


def _device_type_name(device: "cl.Device") -> str:
    _require_opencl()
    dtype = int(getattr(device, "type", 0) or 0)
    if dtype & int(cl.device_type.GPU):
        return "gpu"
    if dtype & int(cl.device_type.CPU):
        return "cpu"
    if dtype & int(cl.device_type.ACCELERATOR):
        return "accelerator"
    return "unknown"


def list_device_infos() -> List[OpenCLDeviceInfo]:
    _require_opencl()
    infos: List[OpenCLDeviceInfo] = []
    index = 0
    for platform in cl.get_platforms():
        try:
            devices = platform.get_devices()
        except Exception:
            continue
        for device in devices:
            infos.append(
                OpenCLDeviceInfo(
                    index=index,
                    platform_name=str(getattr(platform, "name", "") or ""),
                    platform_vendor=str(getattr(platform, "vendor", "") or ""),
                    device_name=str(getattr(device, "name", "") or ""),
                    device_vendor=str(getattr(device, "vendor", "") or ""),
                    device_type=_device_type_name(device),
                )
            )
            index += 1
    return infos


def _normalize_vendor_key(value: str) -> str:
    txt = str(value or "").strip().lower()
    if txt in ("intel", "intel_gpu", "intel_igpu", "intel_arc"):
        return "intel"
    if txt in ("amd", "rocm", "ati"):
        return "amd"
    if txt in ("nvidia", "cuda", "geforce"):
        return "nvidia"
    return txt


def _vendor_match(info: OpenCLDeviceInfo, vendor_key: str) -> bool:
    key = _normalize_vendor_key(vendor_key)
    hay = " ".join(
        [
            info.platform_name,
            info.platform_vendor,
            info.device_name,
            info.device_vendor,
        ]
    ).lower()
    if key == "intel":
        return "intel" in hay
    if key == "amd":
        return "amd" in hay or "advanced micro devices" in hay or "radeon" in hay
    if key == "nvidia":
        return "nvidia" in hay or "geforce" in hay or "quadro" in hay or "tesla" in hay
    return key in hay


def find_preferred_device_index(
    preferred_vendor: str | None = None,
    *,
    prefer_gpu: bool = True,
) -> int | None:
    infos = list_device_infos()
    if not infos:
        return None

    matches = infos
    if preferred_vendor:
        matches = [info for info in infos if _vendor_match(info, preferred_vendor)]
        if not matches:
            return None

    if prefer_gpu:
        gpu_matches = [info for info in matches if info.device_type == "gpu"]
        if gpu_matches:
            matches = gpu_matches

    return matches[0].index if matches else None


def resolve_device_index(
    requested_index: int,
    *,
    preferred_vendor: str | None = None,
    prefer_gpu: bool = True,
) -> tuple[int, str]:
    infos = list_device_infos()
    if not infos:
        return requested_index, "no_opencl_devices"

    if 0 <= int(requested_index) < len(infos):
        chosen = infos[int(requested_index)]
        if preferred_vendor is None or _vendor_match(chosen, preferred_vendor):
            return int(requested_index), "requested_index_kept"

    preferred = find_preferred_device_index(preferred_vendor, prefer_gpu=prefer_gpu)
    if preferred is not None:
        if preferred == int(requested_index):
            return preferred, "requested_index_kept"
        return preferred, "preferred_vendor_auto_selected"

    if 0 <= int(requested_index) < len(infos):
        return int(requested_index), "requested_index_kept_without_vendor_match"
    return 0, "fallback_first_device"


def get_device_count() -> int:
    try:
        return len(_list_devices_flat())
    except Exception:
        return 0


def get_device_name(device_index: int) -> str:
    devices = _list_devices_flat()
    if device_index < 0 or device_index >= len(devices):
        raise IndexError("Nieprawidłowy indeks urządzenia OpenCL")
    return devices[device_index].name


def init_opencl(device_index: int) -> OpenCLContext:
    devices = _list_devices_flat()
    if device_index < 0 or device_index >= len(devices):
        raise IndexError("Nieprawidłowy indeks urządzenia OpenCL")
    device = devices[device_index]
    ctx = cl.Context(devices=[device])
    queue = cl.CommandQueue(
        ctx, properties=cl.command_queue_properties.PROFILING_ENABLE
    )
    return OpenCLContext(ctx=ctx, queue=queue, device=device)


def _program_mem_copy(ocl: OpenCLContext):
    if ocl.program_mem_copy is not None:
        return ocl.program_mem_copy
    ocl.program_mem_copy = cl.Program(
        ocl.ctx,
        """
        __kernel void mem_copy(__global float* a, __global const float* b) {
            size_t i = get_global_id(0);
            a[i] = b[i];
        }
        """,
    ).build()
    return ocl.program_mem_copy


def _program_fma(ocl: OpenCLContext):
    if ocl.program_fma is not None:
        return ocl.program_fma
    ocl.program_fma = cl.Program(
        ocl.ctx,
        """
        __kernel void fma_kernel(__global float* a, __global const float* b, __global const float* c, int iters) {
            size_t i = get_global_id(0);
            float x = a[i];
            float y = b[i];
            float z = c[i];
            for (int k = 0; k < iters; ++k) {
                x = fma(x, y, z);
            }
            a[i] = x;
        }
        """,
    ).build()
    return ocl.program_fma


def _program_pointer_chase(ocl: OpenCLContext):
    if ocl.program_pointer_chase is not None:
        return ocl.program_pointer_chase
    ocl.program_pointer_chase = cl.Program(
        ocl.ctx,
        """
        __kernel void pointer_chase(
            __global const uint* next_idx,
            const uint start_idx,
            const int iters,
            __global uint* out
        ) {
            uint idx = start_idx;
            for (int k = 0; k < iters; ++k) {
                idx = next_idx[idx];
            }
            out[0] = idx;
        }
        """,
    ).build()
    return ocl.program_pointer_chase


def opencl_mem_copy(ocl: OpenCLContext, n: int) -> float:
    _require_opencl()
    n = max(1, int(n))
    nbytes = max(4, n * 4)
    mf = cl.mem_flags
    if ocl.mem_copy_a is None or ocl.mem_copy_b is None or ocl.mem_copy_n != n:
        ocl.mem_copy_a = cl.Buffer(ocl.ctx, mf.READ_WRITE, nbytes)
        ocl.mem_copy_b = cl.Buffer(ocl.ctx, mf.READ_WRITE, nbytes)
        ocl.mem_copy_n = n
        ocl.mem_copy_warmup_done = False
    program = _program_mem_copy(ocl)

    if not ocl.mem_copy_warmup_done:
        program.mem_copy(ocl.queue, (n,), None, ocl.mem_copy_a, ocl.mem_copy_b).wait()
        ocl.mem_copy_warmup_done = True

    event = program.mem_copy(ocl.queue, (n,), None, ocl.mem_copy_a, ocl.mem_copy_b)
    event.wait()
    elapsed_ns = event.profile.end - event.profile.start
    return float(elapsed_ns) * 1e-9


def opencl_memcpy_h2d_bandwidth(ocl: OpenCLContext, bytes_size: int, iters: int) -> float:
    _require_opencl()
    if bytes_size <= 0:
        bytes_size = 4
    if iters <= 0:
        iters = 1

    num_elements = bytes_size // 4
    if num_elements <= 0:
        num_elements = 1

    if ocl.h2d_host is None or ocl.h2d_num_elements != num_elements:
        ocl.h2d_host = np.random.random(num_elements).astype(np.float32)
        mf = cl.mem_flags
        ocl.h2d_buf = cl.Buffer(ocl.ctx, mf.READ_WRITE, ocl.h2d_host.nbytes)
        ocl.h2d_num_elements = num_elements

    # Warm-up
    cl.enqueue_copy(ocl.queue, ocl.h2d_buf, ocl.h2d_host).wait()

    t0 = time.perf_counter()
    for _ in range(iters):
        cl.enqueue_copy(ocl.queue, ocl.h2d_buf, ocl.h2d_host)
    ocl.queue.finish()
    t1 = time.perf_counter()
    return t1 - t0


def opencl_memcpy_d2h_bandwidth(ocl: OpenCLContext, bytes_size: int, iters: int) -> float:
    _require_opencl()
    if bytes_size <= 0:
        bytes_size = 4
    if iters <= 0:
        iters = 1

    num_elements = bytes_size // 4
    if num_elements <= 0:
        num_elements = 1

    if ocl.d2h_host is None or ocl.d2h_num_elements != num_elements:
        ocl.d2h_host = np.random.random(num_elements).astype(np.float32)
        ocl.d2h_out = np.empty_like(ocl.d2h_host)
        mf = cl.mem_flags
        ocl.d2h_buf = cl.Buffer(ocl.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=ocl.d2h_host)
        ocl.d2h_num_elements = num_elements

    # Warm-up
    cl.enqueue_copy(ocl.queue, ocl.d2h_out, ocl.d2h_buf).wait()

    t0 = time.perf_counter()
    for _ in range(iters):
        cl.enqueue_copy(ocl.queue, ocl.d2h_out, ocl.d2h_buf)
    ocl.queue.finish()
    t1 = time.perf_counter()
    return t1 - t0


def opencl_fma_throughput(ocl: OpenCLContext, n: int, iters_inner: int) -> float:
    _require_opencl()
    n = max(1, int(n))
    nbytes = max(4, n * 4)
    mf = cl.mem_flags
    if ocl.fma_a is None or ocl.fma_b is None or ocl.fma_c is None or ocl.fma_n != n:
        ocl.fma_a = cl.Buffer(ocl.ctx, mf.READ_WRITE, nbytes)
        ocl.fma_b = cl.Buffer(ocl.ctx, mf.READ_WRITE, nbytes)
        ocl.fma_c = cl.Buffer(ocl.ctx, mf.READ_WRITE, nbytes)
        ocl.fma_n = n
        ocl.fma_warmup_done = False
    program = _program_fma(ocl)

    if not ocl.fma_warmup_done:
        program.fma_kernel(
            ocl.queue,
            (n,),
            None,
            ocl.fma_a,
            ocl.fma_b,
            ocl.fma_c,
            np.int32(max(1, int(iters_inner))),
        ).wait()
        ocl.fma_warmup_done = True

    event = program.fma_kernel(
        ocl.queue,
        (n,),
        None,
        ocl.fma_a,
        ocl.fma_b,
        ocl.fma_c,
        np.int32(max(1, int(iters_inner))),
    )
    event.wait()
    elapsed_ns = event.profile.end - event.profile.start
    return float(elapsed_ns) * 1e-9


def opencl_fma_peak(ocl: OpenCLContext, n: int, iters_inner: int) -> float:
    # For now, same kernel; allows caller to increase iters_inner for peak.
    return opencl_fma_throughput(ocl, n=n, iters_inner=iters_inner)


def opencl_pointer_chase_latency(ocl: OpenCLContext, n: int, iters: int) -> float:
    _require_opencl()
    if n <= 0:
        n = 1
    if iters <= 0:
        iters = 1

    if ocl.pc_next_buf is None or ocl.pc_out_buf is None or ocl.pc_n != n:
        stride = 17
        next_idx = ((np.arange(n, dtype=np.uint32) + np.uint32(stride)) % np.uint32(n)).astype(np.uint32)
        out = np.zeros(1, dtype=np.uint32)
        mf = cl.mem_flags
        ocl.pc_next_buf = cl.Buffer(ocl.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=next_idx)
        ocl.pc_out_buf = cl.Buffer(ocl.ctx, mf.WRITE_ONLY, out.nbytes)
        ocl.pc_n = n
        ocl.pc_warmup_done = False

    program = _program_pointer_chase(ocl)

    if not ocl.pc_warmup_done:
        program.pointer_chase(
            ocl.queue,
            (1,),
            None,
            ocl.pc_next_buf,
            np.uint32(0),
            np.int32(int(iters)),
            ocl.pc_out_buf,
        ).wait()
        ocl.pc_warmup_done = True

    event = program.pointer_chase(
        ocl.queue,
        (1,),
        None,
        ocl.pc_next_buf,
        np.uint32(0),
        np.int32(int(iters)),
        ocl.pc_out_buf,
    )
    event.wait()
    elapsed_ns = event.profile.end - event.profile.start
    return float(elapsed_ns) * 1e-9
