#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
import platform
import shutil
import sys
from typing import Any, Dict, List


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from device_resolution import list_opencl_devices, resolve_device_index
from optimization.problems import FemParametricProblem, FemParametricProblemConfig


ALL_BACKENDS = ["cpu", "cuda", "hip", "opencl", "metal", "amd", "intel"]


def _import_check(module_name: str) -> tuple[bool, str]:
    try:
        __import__(module_name)
        return True, ""
    except Exception as e:
        return False, str(e)


def _first_existing(paths: List[Path]) -> Path | None:
    for p in paths:
        if p.exists():
            return p
    return None


def _detect_env() -> Dict[str, Any]:
    cuda_lib = _first_existing(
        [
            ROOT / "gpu" / "cuda" / "lib" / "libgpubench_cuda.so",
            ROOT / "gpu" / "cuda" / "lib" / "libgpubench_cuda.dylib",
            ROOT / "gpu" / "cuda" / "lib" / "gpubench_cuda.dll",
        ]
    )
    hip_lib = _first_existing(
        [
            ROOT / "gpu" / "hip" / "lib" / "libgpubench_hip.so",
            ROOT / "gpu" / "hip" / "lib" / "libgpubench_hip.dylib",
            ROOT / "gpu" / "hip" / "lib" / "gpubench_hip.dll",
        ]
    )

    pyopencl_ok, pyopencl_err = _import_check("pyopencl")
    cupy_ok, cupy_err = _import_check("cupy")
    metal_ok, metal_err = _import_check("Metal")

    return {
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "python": sys.version.split()[0],
        "command_presence": {
            "nvcc": shutil.which("nvcc") is not None,
            "hipcc": shutil.which("hipcc") is not None,
            "clang": shutil.which("clang") is not None,
            "nvidia-smi": shutil.which("nvidia-smi") is not None,
            "clinfo": shutil.which("clinfo") is not None,
        },
        "python_modules": {
            "pyopencl": {"ok": pyopencl_ok, "error": pyopencl_err},
            "cupy": {"ok": cupy_ok, "error": cupy_err},
            "metal_pyobjc": {"ok": metal_ok, "error": metal_err},
        },
        "project_libraries": {
            "cuda_gpubench": str(cuda_lib) if cuda_lib is not None else None,
            "hip_gpubench": str(hip_lib) if hip_lib is not None else None,
        },
        "opencl_devices": list_opencl_devices(),
    }


def _recommendations(backend: str, reason: str) -> List[str]:
    b = backend.lower()
    r = reason.lower()
    rec: List[str] = []

    if b == "cpu":
        rec.append("python3 -c \"import numpy; print(numpy.__version__)\"")
        return rec

    if b == "cuda":
        rec.append("bash gpu/cuda/lib/build_cuda.sh")
        rec.append("python3 -c \"from gpu.cuda.cuda_backend import get_device_count; print(get_device_count())\"")
        if "cupy" in r:
            rec.append("pip install cupy-cuda12x")
        return rec

    if b == "hip":
        rec.append("bash gpu/hip/lib/build_hip.sh")
        rec.append("python3 -c \"from gpu.hip.hip_backend import get_device_count; print(get_device_count())\"")
        if "rocm" in r or "hip" in r:
            rec.append("Sprawdź instalację ROCm i dostępność `hipcc`.")
        return rec

    if b == "opencl":
        rec.append("pip install pyopencl")
        rec.append("python3 -c \"import pyopencl as cl; print([(p.name, len(p.get_devices())) for p in cl.get_platforms()])\"")
        rec.append("Linux Intel: zainstaluj OpenCL ICD (np. intel-opencl-icd).")
        rec.append("Linux AMD: zainstaluj OpenCL runtime (ROCm/OpenCL ICD).")
        return rec

    if b == "metal":
        rec.append("pip install pyobjc pyobjc-framework-Metal")
        rec.append("python3 -c \"import Metal; d=Metal.MTLCreateSystemDefaultDevice(); print(d.name() if d else 'NO_DEVICE')\"")
        rec.append("Metal wymaga macOS oraz GPU zgodnego z Metal.")
        return rec

    if b == "amd":
        rec.append("Preferowane: HIP -> `bash gpu/hip/lib/build_hip.sh` + ROCm.")
        rec.append("Fallback: OpenCL -> `pip install pyopencl` + OpenCL ICD dla AMD.")
        return rec

    if b == "intel":
        rec.append("Intel backend używa OpenCL: `pip install pyopencl`.")
        rec.append("Zainstaluj OpenCL runtime/ICD dla Intel GPU/CPU.")
        return rec

    return rec


def _check_backend(backend: str, device_index: int, execution_policy: str, platform_profile: str) -> Dict[str, Any]:
    resolved_device_index, device_resolution_reason = resolve_device_index(
        backend,
        device_index,
        platform_profile=platform_profile,
    )
    result: Dict[str, Any] = {
        "backend": backend,
        "available": False,
        "resolved_backend": None,
        "execution_mode": None,
        "device_name": None,
        "device_index_requested": int(device_index),
        "device_index_used": int(resolved_device_index),
        "device_resolution_reason": str(device_resolution_reason),
        "reason": "",
        "recommendations": [],
    }
    try:
        problem = FemParametricProblem(
            FemParametricProblemConfig(
                backend=backend,
                device_index=resolved_device_index,
                execution_policy=execution_policy,
                repeats=1,
                n_elements_min=32,
                n_elements_max=32,
                n_qp_min=1,
                n_qp_max=1,
                element_types=["tet4"],
                operators=["diffusion"],
                dtypes=["float32"],
                algorithm_variants=["qss"],
                workgroup_sizes=[64],
                use_workspace_for_pde_coeff_choices=[0, 1],
                use_workspace_for_geo_data_choices=[0, 1],
                use_workspace_for_shape_fun_choices=[0, 1],
                use_workspace_for_stiff_mat_choices=[0, 1],
                padding_choices=[0, 1],
                compute_all_shape_fun_der_choices=[0, 1],
                coal_read_choices=[0, 1],
                coal_write_choices=[0, 1],
            )
        )
        result["available"] = True
        result["resolved_backend"] = problem.mode.resolved_backend
        result["execution_mode"] = problem.mode.execution_mode
        result["mapping_score"] = float(problem.mode.mapping_score)
        result["device_name"] = problem.mode.device_name
        result["memory_budget_bytes"] = int(problem.mode.profile.memory_budget_bytes)
        result["reason"] = "ok"
    except Exception as e:
        reason = str(e).strip() or "unknown_error"
        result["reason"] = reason
        result["recommendations"] = _recommendations(backend, reason)
    return result


def _parse_backends(raw: str) -> List[str]:
    txt = raw.strip().lower()
    if txt == "all":
        return list(ALL_BACKENDS)
    out: List[str] = []
    for chunk in txt.split(","):
        b = chunk.strip()
        if not b:
            continue
        if b not in ALL_BACKENDS:
            raise ValueError(f"Unsupported backend: {b}. Allowed: {', '.join(ALL_BACKENDS)}")
        if b not in out:
            out.append(b)
    if not out:
        raise ValueError("No backends selected.")
    return out


def _print_human(env: Dict[str, Any], checks: List[Dict[str, Any]]) -> None:
    print("=== FEM PARAMETRIC PREFLIGHT ===")
    print(
        f"platform={env['system']} {env['release']} {env['machine']} | "
        f"python={env['python']}"
    )
    print(
        "commands: "
        f"nvcc={int(env['command_presence']['nvcc'])}, "
        f"hipcc={int(env['command_presence']['hipcc'])}, "
        f"clang={int(env['command_presence']['clang'])}, "
        f"nvidia-smi={int(env['command_presence']['nvidia-smi'])}, "
        f"clinfo={int(env['command_presence']['clinfo'])}"
    )
    print(
        "python modules: "
        f"pyopencl={int(env['python_modules']['pyopencl']['ok'])}, "
        f"cupy={int(env['python_modules']['cupy']['ok'])}, "
        f"Metal={int(env['python_modules']['metal_pyobjc']['ok'])}"
    )
    print(
        "project libs: "
        f"cuda={env['project_libraries']['cuda_gpubench'] or 'missing'}, "
        f"hip={env['project_libraries']['hip_gpubench'] or 'missing'}"
    )
    if env.get("opencl_devices"):
        print("opencl devices:")
        for info in env["opencl_devices"]:
            print(
                f"  [{info['index']}] {info['device_name']} | "
                f"vendor={info['device_vendor']} | type={info['device_type']} | "
                f"platform={info['platform_name']}"
            )
    print()

    for item in checks:
        status = "OK" if item["available"] else "FAIL"
        print(f"[{status}] backend={item['backend']}")
        if item["available"]:
            print(
                f"  resolved={item['resolved_backend']}, "
                f"mode={item['execution_mode']}, "
                f"device={item['device_name']}, "
                f"device_index={item.get('device_index_used')}, "
                f"device_resolution={item.get('device_resolution_reason')}, "
                f"mapping_score={item.get('mapping_score')}, "
                f"memory_budget_bytes={item.get('memory_budget_bytes')}"
            )
        else:
            print(f"  reason={item['reason']}")
            for rec in item["recommendations"]:
                print(f"  fix: {rec}")
        print()


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Preflight checker for fem_parametric autotuning backends."
    )
    ap.add_argument(
        "--backend",
        default="all",
        help="all or CSV from: cpu,cuda,hip,opencl,metal,amd,intel",
    )
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument(
        "--platform-profile",
        choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"],
        default="auto",
        help="Used to resolve OpenCL device vendor on mixed systems.",
    )
    ap.add_argument(
        "--execution-policy",
        choices=["native_only", "allow_fallback"],
        default="native_only",
        help="Execution policy used when initializing fem_parametric.",
    )
    ap.add_argument("--json", action="store_true", help="Print JSON summary.")
    ap.add_argument(
        "--strict",
        action="store_true",
        help="Exit with code 2 if at least one selected backend is unavailable.",
    )
    args = ap.parse_args()

    backends = _parse_backends(args.backend)
    env = _detect_env()
    checks = [_check_backend(b, args.device_index, args.execution_policy, args.platform_profile) for b in backends]

    payload = {
        "env": env,
        "checks": checks,
        "requested_backends": backends,
        "all_ok": all(c["available"] for c in checks),
    }

    if args.json:
        print(json.dumps(payload, indent=2, ensure_ascii=True))
    else:
        _print_human(env, checks)

    if args.strict and not payload["all_ok"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
