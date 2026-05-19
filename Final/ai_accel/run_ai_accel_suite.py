#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
import platform
import time
from pathlib import Path
import sys
from typing import Any, Callable

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from ai_accel.common import append_rows, base_meta, make_csv_path, run_warmups
from optimization.problems.gpu_adapters import GpuBackendAdapter, init_gpu_adapter


STANDARD_SHAPES = "512x512x512,1024x1024x1024"
EXTENDED_SHAPES = "512x512x512,1024x1024x1024,1536x1536x1536,2048x2048x2048"
STANDARD_DTYPES = "float16,float32"
EXTENDED_DTYPES = "float16,float32,float64,int8"


def _to_float(v: object) -> float | None:
    try:
        if v is None:
            return None
        s = str(v).strip()
        if not s:
            return None
        return float(s)
    except Exception:
        return None


def _parse_shapes(raw: str) -> list[tuple[int, int, int]]:
    out: list[tuple[int, int, int]] = []
    for part in str(raw).split(","):
        p = part.strip().lower()
        if not p:
            continue
        vals = p.split("x")
        if len(vals) != 3:
            continue
        m, n, k = int(vals[0]), int(vals[1]), int(vals[2])
        if m > 0 and n > 0 and k > 0:
            out.append((m, n, k))
    return out


def _parse_csv(raw: str) -> list[str]:
    return [str(x).strip().lower() for x in str(raw).split(",") if str(x).strip()]


def _dtype_numpy(dtype: str) -> Any:
    d = str(dtype).strip().lower()
    if d == "float16":
        return np.float16
    if d == "float32":
        return np.float32
    if d == "float64":
        return np.float64
    if d == "int8":
        return np.int8
    raise ValueError(f"Unsupported dtype: {dtype}")


def _dtype_itemsize(dtype: str) -> int:
    d = str(dtype).strip().lower()
    if d == "float16":
        return 2
    if d == "float32":
        return 4
    if d == "float64":
        return 8
    if d == "int8":
        return 1
    raise ValueError(f"Unsupported dtype: {dtype}")


def _matmul_ops(m: int, n: int, k: int) -> float:
    return 2.0 * float(m) * float(n) * float(k)


def _matmul_bytes(m: int, n: int, k: int, dtype: str) -> float:
    d = str(dtype).strip().lower()
    in_size = float(_dtype_itemsize(d))
    out_size = 4.0 if d == "int8" else in_size
    return (float(m) * float(k) + float(k) * float(n)) * in_size + (float(m) * float(n)) * out_size


def _backend_available_cuda() -> bool:
    try:
        import cupy as cp  # type: ignore

        return int(cp.cuda.runtime.getDeviceCount()) > 0
    except Exception:
        return False


def _backend_available_metal() -> bool:
    if platform.system() != "Darwin":
        return False
    try:
        from real_kernels.metal_backend import MetalRealBackend

        MetalRealBackend(device_index=0)
        return True
    except Exception:
        return False


def _backend_available_mapped(name: str) -> bool:
    try:
        _ = init_gpu_adapter(name, 0)
        return True
    except Exception:
        return False


def _backend_available(name: str) -> bool:
    key = str(name).strip().lower()
    if key == "cpu":
        return True
    if key == "cuda":
        return _backend_available_cuda()
    if key == "metal":
        return _backend_available_metal()
    if key in ("hip", "opencl"):
        return _backend_available_mapped(key)
    return False


def _resolve_auto_backend() -> str:
    if platform.system() == "Darwin" and _backend_available("metal"):
        return "metal"
    if _backend_available("cuda"):
        return "cuda"
    if _backend_available("hip"):
        return "hip"
    if _backend_available("opencl"):
        return "opencl"
    return "cpu"


def _resolve_requested_backend(name: str) -> str:
    key = str(name).strip().lower()
    if key == "auto":
        return _resolve_auto_backend()
    if key == "amd":
        if _backend_available("hip"):
            return "hip"
        if _backend_available("opencl"):
            return "opencl"
        return "cpu"
    if key == "intel":
        if _backend_available("opencl"):
            return "opencl"
        return "cpu"
    return key


def _native_path_label(backend: str, dtype: str, kernel: str) -> str:
    b = str(backend).strip().lower()
    d = str(dtype).strip().lower()
    if kernel == "coreml_mlp_predict":
        return "apple_coreml_ne_path"
    if b == "cuda" and d in ("float16", "int8"):
        return "cuda_tensor_core_candidate"
    if b in ("hip", "opencl"):
        return "mapped_native_proxy"
    return "generic_compute_path"


def _implementation_level(backend: str, kernel: str) -> str:
    b = str(backend).strip().lower()
    if kernel == "coreml_mlp_predict":
        return "native_runtime"
    if b == "cuda":
        return "native_vendor_gemm"
    if b == "metal":
        return "native_runtime"
    if b in ("hip", "opencl"):
        return "portable_proxy"
    if b == "cpu":
        return "cpu_fallback"
    return "unsupported"


def _execution_device(backend: str, kernel: str) -> str:
    b = str(backend).strip().lower()
    if kernel == "coreml_mlp_predict":
        return "neural_engine_or_automatic"
    if b == "cpu":
        return "cpu"
    return "gpu"


def _vendor_ai_unit_used(backend: str, dtype: str, kernel: str, status: str, compute_units: str = "") -> str:
    st = str(status).strip().lower()
    if st != "ok":
        return "unknown"
    if kernel == "coreml_mlp_predict":
        cu = str(compute_units).strip().lower()
        return "probable" if cu in ("cpu_and_ne", "all") else "false"
    b = str(backend).strip().lower()
    d = str(dtype).strip().lower()
    if b == "cuda" and d in ("float16", "int8"):
        return "probable"
    return "false"


def _native_ai_available(backend: str, dtype: str, kernel: str, status: str, compute_units: str = "") -> int:
    used = _vendor_ai_unit_used(
        backend=backend,
        dtype=dtype,
        kernel=kernel,
        status=status,
        compute_units=compute_units,
    )
    return 1 if used in ("probable", "confirmed") else 0


def _acceleration_class(
    *,
    backend: str,
    kernel: str,
    dtype: str,
    status: str,
    implementation_level: str,
    ops_count: float,
) -> str:
    st = str(status).strip().lower()
    if st != "ok":
        return "unsupported"
    if implementation_level == "portable_proxy":
        return "fallback-bound"
    if kernel == "coreml_mlp_predict":
        return "runtime-overhead-bound" if ops_count < 2.0e6 else "dense-compute-bound"
    if ops_count < 1.0e8:
        return "runtime-overhead-bound"
    if str(dtype).strip().lower() in ("float16", "int8") and str(backend).strip().lower() in ("cuda", "metal"):
        return "precision-limited"
    return "dense-compute-bound"


def _validation_thresholds(dtype: str) -> tuple[float, float]:
    d = str(dtype).strip().lower()
    if d == "float16":
        return 5e-1, 2e-1
    if d == "float32":
        return 1e-2, 5e-3
    if d == "float64":
        return 1e-8, 1e-8
    if d == "int8":
        return 0.0, 0.0
    return 1e-3, 1e-4


def _validation_default(reference_dtype: str = "", status: str = "unsupported", reason: str = "") -> dict[str, object]:
    return {
        "max_abs_error": float("nan"),
        "mean_abs_error": float("nan"),
        "max_rel_error": float("nan"),
        "mean_rel_error": float("nan"),
        "reference_dtype": reference_dtype,
        "validation_status": status,
        "validation_reason": reason,
    }


def _probe_inputs(dtype: str, m: int = 128, n: int = 128, k: int = 128, seed: int = 12345) -> tuple[np.ndarray, np.ndarray, np.ndarray, str]:
    rng = np.random.default_rng(seed)
    d = str(dtype).strip().lower()
    if d == "int8":
        a = rng.integers(-8, 8, size=(m, k), dtype=np.int8)
        b = rng.integers(-8, 8, size=(k, n), dtype=np.int8)
        ref = a.astype(np.int32) @ b.astype(np.int32)
        return a, b, ref.astype(np.float64), "int32"
    dt = _dtype_numpy(d)
    a = rng.standard_normal(size=(m, k)).astype(dt)
    b = rng.standard_normal(size=(k, n)).astype(dt)
    ref = a.astype(np.float64) @ b.astype(np.float64)
    return a, b, ref, "float64"


def _error_metrics(observed: np.ndarray, reference: np.ndarray, dtype: str) -> dict[str, object]:
    obs = observed.astype(np.float64, copy=False)
    ref = reference.astype(np.float64, copy=False)
    diff = np.abs(obs - ref)
    denom = np.maximum(np.abs(ref), 1e-12)
    rel = diff / denom
    max_abs = float(np.max(diff))
    mean_abs = float(np.mean(diff))
    max_rel = float(np.max(rel))
    mean_rel = float(np.mean(rel))
    abs_thr, rel_thr = _validation_thresholds(dtype)
    validation_status = "pass" if max_abs <= abs_thr and max_rel <= rel_thr else "warning"
    return {
        "max_abs_error": max_abs,
        "mean_abs_error": mean_abs,
        "max_rel_error": max_rel,
        "mean_rel_error": mean_rel,
        "validation_status": validation_status,
    }


def _cpu_validation(dtype: str) -> dict[str, object]:
    try:
        a, b, ref, ref_dtype = _probe_inputs(dtype)
        d = str(dtype).strip().lower()
        if d == "int8":
            out = a.astype(np.int32) @ b.astype(np.int32)
        else:
            out = a @ b
        metrics = _error_metrics(out, ref, dtype)
        metrics["reference_dtype"] = ref_dtype
        metrics["validation_reason"] = "cpu_reference_probe"
        return metrics
    except Exception as exc:
        return _validation_default(reference_dtype="float64", status="unsupported", reason=str(exc))


def _cuda_validation(dtype: str, device_index: int) -> dict[str, object]:
    try:
        import cupy as cp  # type: ignore
    except Exception as exc:
        return _validation_default(reference_dtype="float64", status="unsupported", reason=f"cupy unavailable: {exc}")
    try:
        cp.cuda.Device(int(device_index)).use()
        a, b, ref, ref_dtype = _probe_inputs(dtype)
        d = str(dtype).strip().lower()
        if d == "int8":
            a_gpu = cp.asarray(a, dtype=cp.int8).astype(cp.int32)
            b_gpu = cp.asarray(b, dtype=cp.int8).astype(cp.int32)
            out = a_gpu @ b_gpu
        else:
            dt = cp.float16 if d == "float16" else (cp.float32 if d == "float32" else cp.float64)
            out = cp.asarray(a, dtype=dt) @ cp.asarray(b, dtype=dt)
        cp.cuda.Stream.null.synchronize()
        out_np = cp.asnumpy(out)
        metrics = _error_metrics(out_np, ref, dtype)
        metrics["reference_dtype"] = ref_dtype
        metrics["validation_reason"] = "cuda_probe_vs_cpu_reference"
        return metrics
    except Exception as exc:
        return _validation_default(reference_dtype="float64", status="unsupported", reason=str(exc))


def _validation_for_backend(*, backend: str, dtype: str, kernel: str, device_index: int) -> dict[str, object]:
    b = str(backend).strip().lower()
    if kernel == "coreml_mlp_predict":
        return _validation_default(reference_dtype="float64", status="unsupported", reason="runtime probe only")
    if b == "cpu":
        return _cpu_validation(dtype)
    if b == "cuda":
        return _cuda_validation(dtype, device_index)
    return _validation_default(reference_dtype="float64", status="unsupported", reason="validation not implemented for backend")


def _cpu_matmul(m: int, n: int, k: int, dtype: str) -> float:
    dt = _dtype_numpy(dtype)
    d = str(dtype).strip().lower()
    if d == "int8":
        a = np.random.randint(-8, 8, size=(m, k), dtype=np.int8)
        b = np.random.randint(-8, 8, size=(k, n), dtype=np.int8)
        t0 = time.perf_counter()
        out = a.astype(np.int32) @ b.astype(np.int32)
        t1 = time.perf_counter()
        _ = float(out[0, 0])
        return max(t1 - t0, 1e-12)

    a = np.random.rand(m, k).astype(dt)
    b = np.random.rand(k, n).astype(dt)
    t0 = time.perf_counter()
    out = a @ b
    t1 = time.perf_counter()
    _ = float(out[0, 0])
    return max(t1 - t0, 1e-12)


def _cuda_matmul(m: int, n: int, k: int, dtype: str) -> float:
    import cupy as cp  # type: ignore

    d = str(dtype).strip().lower()
    if d == "float16":
        dt = cp.float16
    elif d == "float32":
        dt = cp.float32
    elif d == "float64":
        dt = cp.float64
    elif d == "int8":
        dt = cp.int8
    else:
        raise ValueError(f"Unsupported CUDA dtype: {dtype}")

    if d == "int8":
        a = cp.random.randint(-8, 8, size=(m, k), dtype=cp.int8)
        b = cp.random.randint(-8, 8, size=(k, n), dtype=cp.int8)
    else:
        a = cp.random.random((m, k), dtype=dt)
        b = cp.random.random((k, n), dtype=dt)

    _ = a @ b
    cp.cuda.Stream.null.synchronize()

    t0 = time.perf_counter()
    out = a @ b
    cp.cuda.Stream.null.synchronize()
    t1 = time.perf_counter()
    _ = float(out.reshape(-1)[0].get())
    return max(t1 - t0, 1e-12)


def _metal_matmul(device_index: int, m: int, n: int, k: int, dtype: str) -> float:
    if str(dtype).strip().lower() != "float32":
        raise ValueError("Metal AI matmul supports only float32 in this pipeline.")
    from real_kernels.metal_backend import MetalRealBackend

    be = MetalRealBackend(device_index=device_index)
    elapsed, _gflops = be.gemm(m=m, n=n, k=k, dtype="float32")
    return max(float(elapsed), 1e-12)


def _mapped_proxy_matmul(adapter: GpuBackendAdapter, m: int, n: int, k: int, dtype: str) -> tuple[float, dict[str, Any]]:
    if str(dtype).strip().lower() == "float64" and not bool(adapter.supports_fp64):
        raise ValueError("Backend proxy path does not support float64.")
    if adapter.run_fma is None:
        raise RuntimeError("Mapped backend has no FMA primitive.")

    ops_target = _matmul_ops(m, n, k)
    n_fma = int(max(4096, min(6_000_000, m * n)))
    iters_inner = int(max(128, min(8192, k)))
    fma_elapsed = float(adapter.run_fma(n_fma, iters_inner))
    primitive_ops = max(2.0 * float(n_fma) * float(iters_inner), 1.0)
    compute_rate = primitive_ops / max(fma_elapsed, 1e-12)
    compute_elapsed = ops_target / max(compute_rate, 1e-12)

    mem_elapsed = 0.0
    transfer_used = "none"
    size_bytes = int(max(1_048_576, min(256 * 1024 * 1024, _matmul_bytes(m, n, k, dtype))))
    if adapter.run_mem_d2d is not None:
        mem_elapsed = float(adapter.run_mem_d2d(size_bytes, 1))
        transfer_used = "device_to_device"
    elif adapter.run_mem_h2d is not None:
        mem_elapsed = float(adapter.run_mem_h2d(size_bytes, 1))
        transfer_used = "host_to_device"
    elif adapter.run_mem_d2h is not None:
        mem_elapsed = float(adapter.run_mem_d2h(size_bytes, 1))
        transfer_used = "device_to_host"

    total = max(compute_elapsed + mem_elapsed, 1e-12)
    details = {
        "proxy_transfer": transfer_used,
        "proxy_n_fma": int(n_fma),
        "proxy_iters_inner": int(iters_inner),
        "proxy_fma_elapsed_s": float(fma_elapsed),
        "proxy_mem_elapsed_s": float(mem_elapsed),
        "proxy_mem_size_bytes": int(size_bytes),
    }
    return total, details


def _run_backend_matmul(
    *,
    backend: str,
    device_index: int,
    shapes: list[tuple[int, int, int]],
    dtypes: list[str],
    runs: int,
    warmups: int,
) -> tuple[str, list[dict[str, object]]]:
    key = str(backend).strip().lower()
    resolved = _resolve_requested_backend(key)
    rows: list[dict[str, object]] = []

    if resolved == "cpu":
        device_name = platform.processor() or "cpu"
        run_fn: Callable[[int, int, int, str], tuple[float, dict[str, Any]]] = (
            lambda m, n, k, dtype: (_cpu_matmul(m, n, k, dtype), {})
        )
    elif resolved == "cuda":
        try:
            import cupy as cp  # type: ignore

            dev_props = cp.cuda.runtime.getDeviceProperties(int(device_index))
            device_name = str(dev_props["name"].decode("utf-8"))
            cp.cuda.Device(int(device_index)).use()
        except Exception as exc:
            return resolved, [
                {
                    **base_meta(resolved, "cuda_unavailable", int(device_index)),
                    "kernel_group": "ai_accel",
                    "benchmark": "ai_accel_suite",
                    "kernel": "matmul",
                    "workload_name": "ai_matmul",
                    "precision": "float32",
                    "m": 0,
                    "n": 0,
                    "k": 0,
                    "ops_count": 0.0,
                    "bytes_moved": 0.0,
                    "elapsed_s": float("nan"),
                    "gflops": float("nan"),
                    "throughput_gbps": float("nan"),
                    "ai_flop_per_byte": float("nan"),
                    "native_ai_path": "cuda_tensor_core_candidate",
                    "native_ai_available": 0,
                    "execution_device": "gpu",
                    "acceleration_class": "unsupported",
                    "implementation_level": "native_vendor_gemm",
                    "vendor_ai_unit_used": "unknown",
                    "notes": "CuPy unavailable",
                    "run_idx": 0,
                    "status": "error",
                    "error": str(exc),
                    **_validation_default(reference_dtype="float64", status="unsupported", reason=str(exc)),
                }
            ]

        run_fn = lambda m, n, k, dtype: (_cuda_matmul(m, n, k, dtype), {})
    elif resolved == "metal":
        try:
            from real_kernels.metal_backend import MetalRealBackend

            be = MetalRealBackend(device_index=int(device_index))
            device_name = str(be.device_name)
        except Exception as exc:
            return resolved, [
                {
                    **base_meta(resolved, "metal_unavailable", int(device_index)),
                    "kernel_group": "ai_accel",
                    "benchmark": "ai_accel_suite",
                    "kernel": "matmul",
                    "workload_name": "ai_matmul",
                    "precision": "float32",
                    "m": 0,
                    "n": 0,
                    "k": 0,
                    "ops_count": 0.0,
                    "bytes_moved": 0.0,
                    "elapsed_s": float("nan"),
                    "gflops": float("nan"),
                    "throughput_gbps": float("nan"),
                    "ai_flop_per_byte": float("nan"),
                    "native_ai_path": "generic_compute_path",
                    "native_ai_available": 0,
                    "execution_device": "gpu",
                    "acceleration_class": "unsupported",
                    "implementation_level": "native_runtime",
                    "vendor_ai_unit_used": "unknown",
                    "notes": "Metal backend unavailable",
                    "run_idx": 0,
                    "status": "error",
                    "error": str(exc),
                    **_validation_default(reference_dtype="float64", status="unsupported", reason=str(exc)),
                }
            ]

        run_fn = lambda m, n, k, dtype: (_metal_matmul(int(device_index), m, n, k, dtype), {})
    elif resolved in ("hip", "opencl"):
        adapter = init_gpu_adapter(resolved, int(device_index))
        device_name = str(adapter.device_name)
        run_fn = lambda m, n, k, dtype: _mapped_proxy_matmul(adapter, m, n, k, dtype)
    else:
        return resolved, []

    print(f"=== AI ACCEL MATMUL ({resolved}) ===")
    print(f"device: {device_name} (index {device_index})")
    print(f"runs: {runs}, warmups: {warmups}")
    validation_cache: dict[str, dict[str, object]] = {}

    for m, n, k in shapes:
        for dtype in dtypes:
            print(f"\n--- shape {m}x{n}x{k} | dtype={dtype} ---")
            val_key = f"{resolved}:{dtype}:matmul"
            if val_key not in validation_cache:
                validation_cache[val_key] = _validation_for_backend(
                    backend=resolved,
                    dtype=dtype,
                    kernel="matmul",
                    device_index=int(device_index),
                )
            validation = dict(validation_cache[val_key])

            def _warm() -> None:
                _elapsed, _details = run_fn(m, n, k, dtype)

            warmup_status = "ok"
            warmup_error = ""
            try:
                run_warmups(warmups, _warm)
            except Exception as exc:
                warmup_status = "error"
                warmup_error = str(exc)

            for run_idx in range(max(int(runs), 1)):
                status = warmup_status
                error = warmup_error
                details: dict[str, Any] = {}
                elapsed = float("nan")
                gflops = float("nan")
                gbps = float("nan")
                ai = float("nan")
                ops = _matmul_ops(m, n, k)
                moved = _matmul_bytes(m, n, k, dtype)
                if status == "ok":
                    try:
                        elapsed, details = run_fn(m, n, k, dtype)
                        gflops = ops / max(elapsed, 1e-12) / 1e9
                        gbps = moved / max(elapsed, 1e-12) / 1e9
                        ai = ops / max(moved, 1.0)
                    except Exception as exc:
                        status = "unsupported" if "support" in str(exc).lower() else "error"
                        error = str(exc)
                print(
                    f"run {run_idx:2d}: status={status:11s} elapsed={elapsed:.6f}s "
                    f"gflops={gflops:.2f} gbps={gbps:.2f}"
                )
                native_path = _native_path_label(resolved, dtype, "matmul")
                implementation_level = _implementation_level(resolved, "matmul")
                execution_device = _execution_device(resolved, "matmul")
                acceleration_class = _acceleration_class(
                    backend=resolved,
                    kernel="matmul",
                    dtype=dtype,
                    status=status,
                    implementation_level=implementation_level,
                    ops_count=ops,
                )
                vendor_ai_unit = _vendor_ai_unit_used(
                    backend=resolved,
                    dtype=dtype,
                    kernel="matmul",
                    status=status,
                )
                row_validation = (
                    validation
                    if status == "ok"
                    else _validation_default(
                        reference_dtype=str(validation.get("reference_dtype", "")),
                        status="unsupported",
                        reason=f"execution_status={status}",
                    )
                )
                rows.append(
                    {
                        **base_meta(resolved, device_name, int(device_index)),
                        "kernel_group": "ai_accel",
                        "benchmark": "ai_accel_suite",
                        "kernel": "matmul",
                        "workload_name": "ai_matmul",
                        "precision": dtype,
                        "m": int(m),
                        "n": int(n),
                        "k": int(k),
                        "ops_count": float(ops),
                        "bytes_moved": float(moved),
                        "elapsed_s": float(elapsed),
                        "gflops": float(gflops),
                        "throughput_gbps": float(gbps),
                        "ai_flop_per_byte": float(ai),
                        "native_ai_path": native_path,
                        "native_ai_available": _native_ai_available(
                            backend=resolved,
                            dtype=dtype,
                            kernel="matmul",
                            status=status,
                        ),
                        "execution_device": execution_device,
                        "acceleration_class": acceleration_class,
                        "implementation_level": implementation_level,
                        "vendor_ai_unit_used": vendor_ai_unit,
                        "notes": str(details) if details else "",
                        "run_idx": int(run_idx),
                        "status": status,
                        "error": error,
                        "reference_dtype": str(row_validation.get("reference_dtype", "")),
                        "max_abs_error": row_validation.get("max_abs_error", float("nan")),
                        "mean_abs_error": row_validation.get("mean_abs_error", float("nan")),
                        "max_rel_error": row_validation.get("max_rel_error", float("nan")),
                        "mean_rel_error": row_validation.get("mean_rel_error", float("nan")),
                        "validation_status": row_validation.get("validation_status", "unsupported"),
                        "validation_reason": row_validation.get("validation_reason", ""),
                    }
                )

    return resolved, rows


def _coreml_probe_rows(*, runs: int, warmups: int, vector_size: int = 512) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    if platform.system() != "Darwin":
        return rows

    try:
        import coremltools as ct  # type: ignore
        from coremltools.models import datatypes  # type: ignore
        from coremltools.models.neural_network import NeuralNetworkBuilder  # type: ignore
        from coremltools.models.utils import save_spec  # type: ignore
    except Exception as exc:
        rows.append(
            {
                **base_meta("coreml", "apple_coreml", 0),
                "kernel_group": "ai_accel",
                "benchmark": "ai_accel_suite",
                "kernel": "coreml_mlp_predict",
                "workload_name": "apple_neural_engine_probe",
                "precision": "float32",
                "m": int(vector_size),
                "n": int(vector_size),
                "k": int(vector_size),
                "ops_count": float(2.0 * vector_size * vector_size),
                "bytes_moved": float(0.0),
                "elapsed_s": float("nan"),
                "gflops": float("nan"),
                "throughput_gbps": float("nan"),
                "ai_flop_per_byte": float("nan"),
                "native_ai_path": "apple_coreml_ne_path",
                "native_ai_available": 0,
                "execution_device": "neural_engine_or_automatic",
                "acceleration_class": "unsupported",
                "implementation_level": "native_runtime",
                "vendor_ai_unit_used": "unknown",
                "compute_units": "n/a",
                "notes": "coremltools unavailable",
                "run_idx": 0,
                "status": "unsupported",
                "error": str(exc),
                **_validation_default(reference_dtype="float64", status="unsupported", reason=str(exc)),
            }
        )
        return rows

    import tempfile

    with tempfile.TemporaryDirectory(prefix="ai_accel_coreml_") as tmp:
        model_path = Path(tmp) / "ai_probe.mlmodel"

        input_features = [("x", datatypes.Array(vector_size))]
        output_features = [("y", datatypes.Array(vector_size))]
        builder = NeuralNetworkBuilder(input_features, output_features, use_float_arraytype=True)

        w = np.random.rand(vector_size, vector_size).astype(np.float32)
        b = np.random.rand(vector_size).astype(np.float32)
        builder.add_inner_product(
            name="dense",
            W=w,
            b=b,
            input_channels=vector_size,
            output_channels=vector_size,
            has_bias=True,
            input_name="x",
            output_name="y",
        )
        save_spec(builder.spec, str(model_path))

        unit_specs = [
            ("CPU_ONLY", "cpu_only"),
            ("CPU_AND_GPU", "cpu_and_gpu"),
            ("CPU_AND_NE", "cpu_and_ne"),
            ("ALL", "all"),
        ]
        ops = 2.0 * float(vector_size) * float(vector_size)
        moved = float((2 * vector_size + vector_size) * 4)
        x = np.random.rand(vector_size).astype(np.float32)

        for enum_name, label in unit_specs:
            compute_unit = getattr(ct.ComputeUnit, enum_name, None)
            if compute_unit is None:
                continue

            status = "ok"
            err = ""
            elapsed = float("nan")
            gflops = float("nan")
            gbps = float("nan")
            try:
                model = ct.models.MLModel(str(model_path), compute_units=compute_unit)
                for _ in range(max(int(warmups), 0)):
                    _ = model.predict({"x": x})
                t0 = time.perf_counter()
                for _ in range(max(int(runs), 1)):
                    _ = model.predict({"x": x})
                t1 = time.perf_counter()
                elapsed = max((t1 - t0) / max(int(runs), 1), 1e-12)
                gflops = ops / elapsed / 1e9
                gbps = moved / elapsed / 1e9
            except Exception as exc:
                status = "unsupported"
                err = str(exc)

            rows.append(
                {
                    **base_meta("coreml", "apple_coreml", 0),
                    "kernel_group": "ai_accel",
                    "benchmark": "ai_accel_suite",
                    "kernel": "coreml_mlp_predict",
                    "workload_name": "apple_neural_engine_probe",
                    "precision": "float32",
                    "m": int(vector_size),
                    "n": int(vector_size),
                    "k": int(vector_size),
                    "ops_count": float(ops),
                    "bytes_moved": float(moved),
                    "elapsed_s": float(elapsed),
                    "gflops": float(gflops),
                    "throughput_gbps": float(gbps),
                    "ai_flop_per_byte": float(ops / max(moved, 1.0)),
                    "native_ai_path": "apple_coreml_ne_path",
                    "native_ai_available": _native_ai_available(
                        backend="coreml",
                        dtype="float32",
                        kernel="coreml_mlp_predict",
                        status=status,
                        compute_units=label,
                    ),
                    "execution_device": _execution_device("coreml", "coreml_mlp_predict"),
                    "acceleration_class": _acceleration_class(
                        backend="coreml",
                        kernel="coreml_mlp_predict",
                        dtype="float32",
                        status=status,
                        implementation_level=_implementation_level("coreml", "coreml_mlp_predict"),
                        ops_count=ops,
                    ),
                    "implementation_level": _implementation_level("coreml", "coreml_mlp_predict"),
                    "vendor_ai_unit_used": _vendor_ai_unit_used(
                        backend="coreml",
                        dtype="float32",
                        kernel="coreml_mlp_predict",
                        status=status,
                        compute_units=label,
                    ),
                    "compute_units": label,
                    "notes": "coremltools predict path",
                    "run_idx": 0,
                    "status": status,
                    "error": err,
                    **_validation_default(reference_dtype="float64", status="unsupported", reason="runtime probe only"),
                }
            )

    return rows


def main() -> None:
    ap = argparse.ArgumentParser(description="AI acceleration suite (portable + vendor-native probes).")
    ap.add_argument(
        "--backend",
        choices=["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel", "all"],
        default="auto",
    )
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--benchmark-mode", choices=["standard", "extended"], default="standard")
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--shapes", type=str, default="")
    ap.add_argument("--dtypes", type=str, default="")
    ap.add_argument("--include-cpu-baseline", action="store_true", default=True)
    ap.add_argument("--no-include-cpu-baseline", dest="include_cpu_baseline", action="store_false")
    ap.add_argument("--with-coreml-ne-probe", action="store_true", default=True)
    ap.add_argument("--no-coreml-ne-probe", dest="with_coreml_ne_probe", action="store_false")
    args = ap.parse_args()

    shapes_raw = args.shapes or (EXTENDED_SHAPES if args.benchmark_mode == "extended" else STANDARD_SHAPES)
    dtypes_raw = args.dtypes or (EXTENDED_DTYPES if args.benchmark_mode == "extended" else STANDARD_DTYPES)
    shapes = _parse_shapes(shapes_raw)
    dtypes = _parse_csv(dtypes_raw)

    if not shapes:
        raise SystemExit("No valid matrix shapes.")
    if not dtypes:
        raise SystemExit("No valid dtypes.")

    requested = str(args.backend).strip().lower()
    if requested == "all":
        gpu_candidates = [b for b in ("metal", "cuda", "hip", "opencl") if _backend_available(b)]
        backends = ["cpu", *gpu_candidates]
    else:
        resolved = _resolve_requested_backend(requested)
        backends = [resolved]
        if args.include_cpu_baseline and resolved != "cpu":
            backends = ["cpu", resolved]

    rows_all: list[dict[str, object]] = []
    print("=== AI ACCEL SUITE ===")
    print(f"requested backend: {requested}")
    print(f"resolved backends: {backends}")
    print(f"benchmark mode   : {args.benchmark_mode}")

    for backend in backends:
        if not _backend_available(backend) and backend != "cpu":
            rows_all.append(
                {
                    **base_meta(backend, f"{backend}_unavailable", int(args.device_index)),
                    "kernel_group": "ai_accel",
                    "benchmark": "ai_accel_suite",
                    "kernel": "matmul",
                    "workload_name": "ai_matmul",
                    "precision": "float32",
                    "m": 0,
                    "n": 0,
                    "k": 0,
                    "ops_count": 0.0,
                    "bytes_moved": 0.0,
                    "elapsed_s": float("nan"),
                    "gflops": float("nan"),
                    "throughput_gbps": float("nan"),
                    "ai_flop_per_byte": float("nan"),
                    "native_ai_path": _native_path_label(backend, "float32", "matmul"),
                    "native_ai_available": 0,
                    "execution_device": _execution_device(backend, "matmul"),
                    "acceleration_class": "unsupported",
                    "implementation_level": _implementation_level(backend, "matmul"),
                    "vendor_ai_unit_used": "unknown",
                    "notes": "backend unavailable",
                    "run_idx": 0,
                    "status": "unsupported",
                    "error": "backend unavailable",
                    **_validation_default(reference_dtype="float64", status="unsupported", reason="backend unavailable"),
                }
            )
            continue

        resolved_backend, rows = _run_backend_matmul(
            backend=backend,
            device_index=int(args.device_index),
            shapes=shapes,
            dtypes=dtypes,
            runs=max(int(args.runs), 1),
            warmups=max(int(args.warmups), 0),
        )
        rows_all.extend(rows)

        ok_rows = [row for row in rows if str(row.get("status", "")).lower() == "ok"]
        if ok_rows:
            device_name = str(ok_rows[0].get("device_name", "unknown"))
        else:
            device_name = f"{resolved_backend}_unknown"
        csv_path = make_csv_path("ai_accel", resolved_backend, device_name, int(args.device_index))
        append_rows(csv_path, rows)
        print(f"[INFO] CSV written: {csv_path}")

    if args.with_coreml_ne_probe and platform.system() == "Darwin":
        coreml_rows = _coreml_probe_rows(runs=max(int(args.runs), 1), warmups=max(int(args.warmups), 0))
        if coreml_rows:
            rows_all.extend(coreml_rows)
            csv_path = make_csv_path("ai_coreml_probe", "coreml", "apple_coreml", 0)
            append_rows(csv_path, coreml_rows)
            print(f"[INFO] CSV written: {csv_path}")

    ok = sum(1 for row in rows_all if str(row.get("status", "")).lower() == "ok")
    unsupported = sum(1 for row in rows_all if str(row.get("status", "")).lower() == "unsupported")
    failed = sum(1 for row in rows_all if str(row.get("status", "")).lower() == "error")
    print("\n=== AI ACCEL DONE ===")
    print(f"rows: {len(rows_all)} | ok: {ok} | unsupported: {unsupported} | error: {failed}")


if __name__ == "__main__":
    main()
