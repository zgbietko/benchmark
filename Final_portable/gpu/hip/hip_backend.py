from __future__ import annotations

import ctypes
import platform
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass
class HipDeviceInfo:
    index: int
    name: str
    major: int
    minor: int
    global_mem_bytes: int

    @property
    def arch(self) -> str:
        return f"{self.major}.{self.minor}"

    @property
    def global_mem_gb(self) -> float:
        return self.global_mem_bytes / (1024 ** 3)


@dataclass
class HipContext:
    device_index: int
    lib: ctypes.CDLL
    info: HipDeviceInfo


HipBackend = HipContext  # alias


_LIB_CACHE: Optional[ctypes.CDLL] = None


def _find_library_path() -> Path:
    here = Path(__file__).resolve().parent
    lib_dir = here / "lib"

    system = platform.system()
    candidates = []
    if system == "Darwin":
        candidates.append(lib_dir / "libgpubench_hip.dylib")
    elif system == "Linux":
        candidates.append(lib_dir / "libgpubench_hip.so")
    elif system == "Windows":
        candidates.append(lib_dir / "gpubench_hip.dll")
    else:
        candidates.append(lib_dir / "libgpubench_hip.so")

    for c in candidates:
        if c.is_file():
            return c

    raise FileNotFoundError(
        f"Nie znaleziono biblioteki HIP gpubench w {lib_dir}. "
        f"Upewnij się, że uruchomiłeś gpu/hip/lib/build_hip.sh."
    )


def _load_lib() -> ctypes.CDLL:
    global _LIB_CACHE
    if _LIB_CACHE is not None:
        return _LIB_CACHE

    lib_path = _find_library_path()
    lib = ctypes.cdll.LoadLibrary(str(lib_path))

    lib.gpu_hip_get_device_count.restype = ctypes.c_int

    lib.gpu_hip_get_device_name.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
    lib.gpu_hip_get_device_name.restype = ctypes.c_int

    lib.gpu_hip_get_device_props.argtypes = [
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_size_t),
    ]
    lib.gpu_hip_get_device_props.restype = ctypes.c_int

    lib.gpu_hip_memcpy_bandwidth.argtypes = [
        ctypes.c_int,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
    ]
    lib.gpu_hip_memcpy_bandwidth.restype = ctypes.c_int
    lib.gpu_hip_memcpy_h2d_bandwidth.argtypes = [
        ctypes.c_int,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
    ]
    lib.gpu_hip_memcpy_h2d_bandwidth.restype = ctypes.c_int
    lib.gpu_hip_memcpy_d2h_bandwidth.argtypes = [
        ctypes.c_int,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
    ]
    lib.gpu_hip_memcpy_d2h_bandwidth.restype = ctypes.c_int

    lib.gpu_hip_fma_throughput.argtypes = [
        ctypes.c_int,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
    ]
    lib.gpu_hip_fma_throughput.restype = ctypes.c_int

    if hasattr(lib, "gpu_hip_pointer_chase_latency"):
        lib.gpu_hip_pointer_chase_latency.argtypes = [
            ctypes.c_int,
            ctypes.c_size_t,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_double),
        ]
        lib.gpu_hip_pointer_chase_latency.restype = ctypes.c_int

    _LIB_CACHE = lib
    return lib


def get_device_count() -> int:
    lib = _load_lib()
    rc = lib.gpu_hip_get_device_count()
    if rc < 0:
        raise RuntimeError(f"HIP error in get_device_count(): rc={rc}")
    return int(rc)


def _get_raw_device_info(device_index: int) -> HipDeviceInfo:
    lib = _load_lib()

    buf = ctypes.create_string_buffer(256)
    rc = lib.gpu_hip_get_device_name(device_index, buf, ctypes.sizeof(buf))
    if rc != 0:
        raise RuntimeError(f"HIP error in get_device_name({device_index}): rc={rc}")
    name = buf.value.decode("utf-8", errors="replace")

    major = ctypes.c_int(0)
    minor = ctypes.c_int(0)
    gmem = ctypes.c_size_t(0)
    rc = lib.gpu_hip_get_device_props(device_index, ctypes.byref(major), ctypes.byref(minor), ctypes.byref(gmem))
    if rc != 0:
        raise RuntimeError(f"HIP error in get_device_props({device_index}): rc={rc}")

    return HipDeviceInfo(
        index=device_index,
        name=name,
        major=int(major.value),
        minor=int(minor.value),
        global_mem_bytes=int(gmem.value),
    )


def init_hip(device_index: int = 0) -> HipContext:
    lib = _load_lib()
    info = _get_raw_device_info(device_index)
    return HipContext(device_index=device_index, lib=lib, info=info)


def get_device_info(ctx: HipContext) -> HipDeviceInfo:
    return ctx.info


def get_device_name(device_index: int) -> str:
    return _get_raw_device_info(device_index).name


def hip_memcpy_bandwidth(ctx: HipContext, size_bytes: int, iters: int) -> float:
    elapsed_ms = ctypes.c_double(0.0)
    rc = ctx.lib.gpu_hip_memcpy_bandwidth(ctx.device_index, int(size_bytes), int(iters), ctypes.byref(elapsed_ms))
    if rc != 0:
        raise RuntimeError(f"HIP error in memcpy_bandwidth(): rc={rc}")
    return float(elapsed_ms.value) / 1000.0


def hip_memcpy_h2d_bandwidth(ctx: HipContext, size_bytes: int, iters: int) -> float:
    elapsed_ms = ctypes.c_double(0.0)
    rc = ctx.lib.gpu_hip_memcpy_h2d_bandwidth(ctx.device_index, int(size_bytes), int(iters), ctypes.byref(elapsed_ms))
    if rc != 0:
        raise RuntimeError(f"HIP error in memcpy_h2d_bandwidth(): rc={rc}")
    return float(elapsed_ms.value) / 1000.0


def hip_memcpy_d2h_bandwidth(ctx: HipContext, size_bytes: int, iters: int) -> float:
    elapsed_ms = ctypes.c_double(0.0)
    rc = ctx.lib.gpu_hip_memcpy_d2h_bandwidth(ctx.device_index, int(size_bytes), int(iters), ctypes.byref(elapsed_ms))
    if rc != 0:
        raise RuntimeError(f"HIP error in memcpy_d2h_bandwidth(): rc={rc}")
    return float(elapsed_ms.value) / 1000.0


def hip_fma_throughput(ctx: HipContext, n_elements: int, iters_inner: int) -> float:
    elapsed_ms = ctypes.c_double(0.0)
    rc = ctx.lib.gpu_hip_fma_throughput(ctx.device_index, int(n_elements), int(iters_inner), ctypes.byref(elapsed_ms))
    if rc != 0:
        raise RuntimeError(f"HIP error in fma_throughput(): rc={rc}")
    return float(elapsed_ms.value) / 1000.0


def hip_pointer_chase_latency(ctx: HipContext, n: int, iters: int) -> float:
    if not hasattr(ctx.lib, "gpu_hip_pointer_chase_latency"):
        raise RuntimeError("HIP error: gpu_hip_pointer_chase_latency not found (rebuild HIP library).")
    elapsed_ms = ctypes.c_double(0.0)
    rc = ctx.lib.gpu_hip_pointer_chase_latency(
        ctx.device_index, int(n), int(iters), ctypes.byref(elapsed_ms)
    )
    if rc != 0:
        raise RuntimeError(f"HIP error in pointer_chase_latency(): rc={rc}")
    return float(elapsed_ms.value) / 1000.0
