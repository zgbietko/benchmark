#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from run_session import create_session_dir, manifest_base, write_manifest


PLATFORM_PROFILES_PATH = ROOT / "configs" / "platform_profiles.json"
GPU_BACKENDS = ("metal", "cuda", "hip", "opencl")


def _run_py(rel: str, args: list[str], *, env: dict[str, str] | None = None) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[ERROR] Missing script: {path}")
        return 1
    cmd = [sys.executable, str(path), *args]
    merged_env = dict(os.environ)
    if env:
        merged_env.update(env)
    print(f"\n=== RUN: {rel} {' '.join(args)} ===", flush=True)
    return subprocess.run(cmd, cwd=ROOT, env=merged_env, check=False).returncode


def _load_platform_profiles() -> dict[str, Any]:
    if not PLATFORM_PROFILES_PATH.exists():
        return {}
    try:
        return json.loads(PLATFORM_PROFILES_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _backend_available_cuda() -> bool:
    try:
        from gpu.cuda.cuda_backend import get_device_count  # type: ignore

        return int(get_device_count()) > 0
    except Exception:
        return False


def _backend_available_hip() -> bool:
    try:
        from gpu.hip.hip_backend import get_device_count  # type: ignore

        return int(get_device_count()) > 0
    except Exception:
        return False


def _backend_available_opencl() -> bool:
    try:
        from gpu.opencl.opencl_backend import get_device_count  # type: ignore

        return int(get_device_count()) > 0
    except Exception:
        return False


def _backend_available_metal() -> bool:
    if platform.system() != "Darwin":
        return False
    try:
        from gpu.metal.metal_backend import MetalBackend  # type: ignore

        MetalBackend(device_index=0)
        return True
    except Exception:
        return False


def _backend_available(name: str) -> bool:
    if name == "cuda":
        return _backend_available_cuda()
    if name == "hip":
        return _backend_available_hip()
    if name == "opencl":
        return _backend_available_opencl()
    if name == "metal":
        return _backend_available_metal()
    return False


def _pick_backend_by_order(order: list[str]) -> str | None:
    for backend in order:
        if _backend_available(backend):
            return backend
    return None


def _resolve_gpu_backend(requested: str, platform_profile: str, arch: str) -> str:
    req = requested.strip().lower()
    if req in GPU_BACKENDS:
        return req
    if req == "intel":
        return "opencl"
    if req == "amd":
        return "hip" if _backend_available("hip") else "opencl"

    profiles = _load_platform_profiles()
    selected_profile = profiles.get(platform_profile, {}) if platform_profile != "auto" else {}
    order: list[str] = []
    raw_pref = selected_profile.get("backend_preference", []) if isinstance(selected_profile, dict) else []
    if isinstance(raw_pref, list):
        order = [str(x) for x in raw_pref if str(x) in GPU_BACKENDS]

    if not order:
        if platform_profile == "apple" or arch == "apple" or platform.system() == "Darwin":
            order = ["metal", "opencl"]
        elif platform_profile == "amd" or arch == "amd":
            order = ["hip", "opencl", "cuda"]
        elif platform_profile in ("intel_arc", "intel_igpu"):
            order = ["opencl"]
        else:
            order = ["cuda", "hip", "opencl"]

    selected = _pick_backend_by_order(order)
    if selected is not None:
        return selected
    return order[0]


def _resolve_fem_backend_token(requested: str, platform_profile: str, arch: str) -> str:
    req = requested.strip().lower()
    if req in ("cpu", "cuda", "hip", "metal", "opencl", "amd", "intel"):
        if req == "amd":
            if _backend_available("hip"):
                return "amd"
            if _backend_available("opencl"):
                return "amd"
            return "cpu"
        if req == "intel":
            if _backend_available("opencl"):
                return "intel"
            return "cpu"
        if req == "cpu" or _backend_available(req):
            return req
        return "cpu"

    if platform_profile == "amd":
        if _backend_available("hip") or _backend_available("opencl"):
            return "amd"
        return "cpu"
    if platform_profile in ("intel_arc", "intel_igpu"):
        if _backend_available("opencl"):
            return "intel"
        return "cpu"

    gpu_backend = _resolve_gpu_backend("auto", platform_profile, arch)
    if gpu_backend == "hip" and platform_profile == "amd":
        return "amd"
    if gpu_backend == "opencl" and platform_profile in ("intel_arc", "intel_igpu"):
        return "intel"
    if _backend_available(gpu_backend):
        return gpu_backend
    return "cpu"


def _optimization_dirs() -> set[str]:
    base = ROOT / "data" / "optimization"
    if not base.exists():
        return set()
    return {p.name for p in base.iterdir() if p.is_dir()}


def _latest_new_optimization_dir(before: set[str]) -> Path | None:
    base = ROOT / "data" / "optimization"
    if not base.exists():
        return None
    new_dirs = [p for p in base.iterdir() if p.is_dir() and p.name not in before]
    if not new_dirs:
        return None
    return max(new_dirs, key=lambda p: p.stat().st_mtime)


def _session_env(session_dir: Path, profile: str) -> dict[str, str]:
    return {
        "BENCH_RUN_DIR": str(session_dir),
        "BENCH_PROFILE": profile,
    }


def _write_session_manifest(session_dir: Path, payload: dict[str, Any]) -> None:
    manifest = manifest_base(str(payload.get("profile", "custom")))
    manifest.update(payload)
    write_manifest(session_dir, manifest)


def _session_result(session_dir: Path, payload: dict[str, Any]) -> int:
    payload = dict(payload)
    payload["out_dir"] = str(session_dir)
    print("\n=== WORKFLOW DONE ===")
    print(json.dumps(payload, indent=2, ensure_ascii=True))
    return 0 if int(payload.get("exit_code", 1)) == 0 else 1


def _optimization_result(out_dir: Path | None, payload: dict[str, Any]) -> int:
    payload = dict(payload)
    if out_dir is not None:
        payload["out_dir"] = str(out_dir)
        summary_path = out_dir / "summary.json"
        if summary_path.exists():
            try:
                summary = json.loads(summary_path.read_text(encoding="utf-8"))
                if "article_plots_dir" in summary:
                    payload["article_plots_dir"] = summary.get("article_plots_dir")
                if "article_plots" in summary:
                    payload["article_plots"] = summary.get("article_plots")
            except Exception:
                pass
    print("\n=== WORKFLOW DONE ===")
    print(json.dumps(payload, indent=2, ensure_ascii=True))
    return 0 if int(payload.get("exit_code", 1)) == 0 else 1


def _run_session_analysis(
    *,
    session_dir: Path,
    env: dict[str, str],
    target: str,
    backend: str,
    include_cpu: bool,
    include_gpu: bool,
    include_real: bool,
    roofline_ai: float,
    roofline_bytes: float,
) -> dict[str, int]:
    results: dict[str, int] = {}
    if include_cpu:
        results["cpu_summary"] = _run_py(
            "analysis/cpu_summary.py",
            ["--mode", "latest", "--scope", "session", "--session", session_dir.name],
            env=env,
        )
    if include_gpu:
        results["gpu_summary"] = _run_py(
            "analysis/gpu_summary.py",
            ["--mode", "latest", "--scope", "session", "--session", session_dir.name],
            env=env,
        )
    if include_real:
        results["real_kernels_summary"] = _run_py(
            "analysis/real_kernels_summary.py",
            ["--scope", "session", "--session", session_dir.name],
            env=env,
        )

    results["plots"] = _run_py("analysis/generate_plots.py", [], env=env)

    roof_args = [
        "--target",
        target,
        "--ai",
        str(roofline_ai),
        "--bytes",
        str(roofline_bytes),
        "--scope",
        "session",
        "--session",
        session_dir.name,
        "--export-dir",
        str(session_dir / "roofline"),
    ]
    if target in ("gpu", "both"):
        roof_args += ["--backend", backend]
    results["roofline"] = _run_py("analysis/roofline_model.py", roof_args, env=env)
    return results


def _native_real_kernels_args(args: argparse.Namespace, backend: str) -> list[str]:
    return [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--runs",
        str(args.real_runs),
        "--with-fem-integration",
        "--fem-integration-element-type",
        args.real_fem_element_type,
        "--fem-integration-operator",
        args.real_fem_operator,
        "--fem-integration-n-qp",
        str(args.real_fem_n_qp),
    ]


def _portable_real_kernels_args(args: argparse.Namespace, backend: str) -> list[str]:
    return [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--runs",
        str(args.real_runs),
        "--sizes",
        args.real_fem_sizes,
        "--element-type",
        args.real_fem_element_type,
        "--operator",
        args.real_fem_operator,
        "--n-qp",
        str(args.real_fem_n_qp),
        "--dtype",
        "float32",
    ]


def _fem_common_flag_values() -> list[str]:
    return [
        "--fem-n-qp-range",
        "1:8",
        "--fem-element-types",
        "tet4,hex8",
        "--fem-operators",
        "diffusion,mass,convection,diffusion_mass,diffusion_convection_mass",
        "--fem-dtypes",
        "float32",
        "--fem-variant-choices",
        "qss,sqs,ssq",
        "--fem-workgroup-sizes",
        "32,64,128,256",
        "--fem-use-workspace-pde-choices",
        "0,1",
        "--fem-use-workspace-geo-choices",
        "0,1",
        "--fem-use-workspace-shape-choices",
        "0,1",
        "--fem-use-workspace-stiff-choices",
        "0,1",
        "--fem-padding-choices",
        "0,1",
        "--fem-compute-all-shape-der-choices",
        "0,1",
        "--fem-coal-read-choices",
        "0,1",
        "--fem-coal-write-choices",
        "0,1",
    ]


def _fem_safe_args_filip(args: argparse.Namespace, backend: str) -> list[str]:
    n_range = "20000:500000" if backend in ("cpu", "cuda") else "20000:300000"
    out = [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--trials",
        str(args.trials),
        "--repeats",
        str(args.repeats),
        "--execution-policy",
        "native_only",
        "--fem-n-elements-range",
        n_range,
        "--screening-repeats",
        "1",
        "--screening-prune-factor",
        "0.55",
    ]
    if backend in ("metal", "opencl", "amd", "intel", "hip"):
        out += ["--memory-budget-mb", "768"]
    out += _fem_common_flag_values()
    return out


def _filip_original_args(args: argparse.Namespace, backend: str) -> list[str]:
    out = [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--profile",
        args.profile,
        "--repeats",
        str(args.repeats),
        "--execution-policy",
        "native_only",
        "--benchmark-case",
        args.filip_case,
        "--variants",
        "qss,sqs,ssq",
        "--dtype",
        "float32",
        "--workgroup-size",
        "0",
    ]
    if backend in ("metal", "opencl", "amd", "intel", "hip"):
        out += ["--memory-budget-mb", "768"]
    return out


def _filip_exact_args(args: argparse.Namespace) -> list[str]:
    backend_req = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    if backend_req not in ("metal", "opencl", "intel"):
        raise SystemExit("Filip exact reference mode supports only backend=opencl/intel/metal/auto.")
    out = [
        "--backend",
        "metal" if backend_req == "metal" else ("intel" if backend_req == "intel" else "opencl"),
        "--benchmark-case",
        args.filip_case,
        "--variants",
        "qss,sqs,ssq",
        "--device-index",
        str(args.device_index),
        "--profile",
        str(args.profile),
    ]
    if str(getattr(args, "filip_modfem_dir", "")).strip():
        out += ["--modfem-dir", str(args.filip_modfem_dir).strip()]
    if str(getattr(args, "filip_input_override", "")).strip():
        out += ["--input-override", str(args.filip_input_override).strip()]
    if bool(getattr(args, "filip_dump_launch_artifacts", False)):
        out += ["--dump-launch-artifacts"]
    if bool(getattr(args, "filip_export_replay_inputs", False)):
        out += ["--export-replay-inputs"]
    if bool(getattr(args, "filip_export_replay_include_expected_output", False)):
        out += ["--export-replay-include-expected-output"]
    if str(getattr(args, "filip_replay_dump_root", "")).strip():
        out += ["--replay-dump-root", str(args.filip_replay_dump_root).strip()]
    return out


def _fem_safe_args_firefly(args: argparse.Namespace, backend: str) -> list[str]:
    n_range = "20000:500000" if backend in ("cpu", "cuda") else "20000:300000"
    out = [
        "--problem",
        "fem_parametric",
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--population",
        str(args.population),
        "--iterations",
        str(args.iterations),
        "--repeats",
        str(args.repeats),
        "--fem-execution-policy",
        "native_only",
        "--fem-n-elements-range",
        n_range,
        "--fem-screening-repeats",
        "1",
        "--fem-screening-prune-factor",
        "0.55",
    ]
    if backend in ("metal", "opencl", "amd", "intel", "hip"):
        out += [
            "--fem-memory-budget-mb",
            "768",
            "--fem-mapped-max-n-fma-light",
            "300000",
            "--fem-mapped-max-buffer-mb-light",
            "32",
            "--fem-mapped-max-mem-iters",
            "64",
            "--fem-mapped-max-inner-iters-light",
            "2048",
        ]
    out += _fem_common_flag_values()
    return out


def run_cpu_benchmark(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
        },
    )
    run_rc = _run_py("run_all_cpu_benchmarks.py", ["--arch-profile", "auto"], env=env)
    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="cpu",
        backend="cpu",
        include_cpu=True,
        include_gpu=False,
        include_real=False,
        roofline_ai=args.roofline_ai,
        roofline_bytes=args.roofline_bytes,
    )
    exit_code = 0 if run_rc == 0 and all(v == 0 for v in analysis_rc.values()) else 1
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "cpu",
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )


def run_gpu_benchmark(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    backend = _resolve_gpu_backend(args.backend, args.platform_profile, args.arch)
    if not _backend_available(backend):
        return _session_result(
            session_dir,
            {
                "workflow": args.workflow,
                "target": "gpu",
                "resolved_backend": backend,
                "error": f"GPU backend unavailable: {backend}",
                "exit_code": 1,
            },
        )
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
        },
    )
    run_rc = _run_py(
        "run_all_gpu_benchmarks.py",
        [
            "--platform-profile",
            args.platform_profile,
            "--arch",
            args.arch,
            "--backend",
            backend,
            "--device-index",
            str(args.device_index),
        ],
        env=env,
    )
    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="gpu",
        backend=backend,
        include_cpu=False,
        include_gpu=True,
        include_real=False,
        roofline_ai=args.roofline_ai,
        roofline_bytes=args.roofline_bytes,
    )
    exit_code = 0 if run_rc == 0 and all(v == 0 for v in analysis_rc.values()) else 1
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )


def run_cpu_real_kernels(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
            "real_kernels_backend": "cpu",
        },
    )
    run_rc = _run_py("run_all_cpu_benchmarks.py", ["--arch-profile", "auto"], env=env)
    rk_rc = _run_py("real_kernels/run_all_real_kernels.py", _native_real_kernels_args(args, "cpu"), env=env)
    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="cpu",
        backend="cpu",
        include_cpu=True,
        include_gpu=False,
        include_real=True,
        roofline_ai=args.roofline_ai,
        roofline_bytes=args.roofline_bytes,
    )
    exit_code = 0 if run_rc == 0 and rk_rc == 0 and all(v == 0 for v in analysis_rc.values()) else 1
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
            "real_kernels_backend": "cpu",
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "cpu",
            "real_kernels_backend": "cpu",
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )


def run_gpu_real_kernels(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    backend = _resolve_gpu_backend(args.backend, args.platform_profile, args.arch)
    if not _backend_available(backend):
        return _session_result(
            session_dir,
            {
                "workflow": args.workflow,
                "target": "gpu",
                "resolved_backend": backend,
                "error": f"GPU backend unavailable: {backend}",
                "exit_code": 1,
            },
        )
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
        },
    )
    run_rc = _run_py(
        "run_all_gpu_benchmarks.py",
        [
            "--platform-profile",
            args.platform_profile,
            "--arch",
            args.arch,
            "--backend",
            backend,
            "--device-index",
            str(args.device_index),
        ],
        env=env,
    )

    if backend in ("cuda", "metal"):
        rk_rc = _run_py("real_kernels/run_all_real_kernels.py", _native_real_kernels_args(args, backend), env=env)
        real_mode = "native_suite"
    else:
        print(f"[INFO] Backend {backend} has no full native real_kernels suite. Using portable FEM integration subset.")
        rk_rc = _run_py(
            "real_kernels/benchmarks/run_fem_integration.py",
            _portable_real_kernels_args(args, backend),
            env=env,
        )
        real_mode = "portable_fem_subset"

    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="gpu",
        backend=backend,
        include_cpu=False,
        include_gpu=True,
        include_real=True,
        roofline_ai=args.roofline_ai,
        roofline_bytes=args.roofline_bytes,
    )
    exit_code = 0 if run_rc == 0 and rk_rc == 0 and all(v == 0 for v in analysis_rc.values()) else 1
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "real_kernels_mode": real_mode,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "real_kernels_mode": real_mode,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )


def run_filip_autotune(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    run_rc = _run_py("run_filip_autotuning.py", _fem_safe_args_filip(args, backend))
    out_dir = _latest_new_optimization_dir(before)
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "optimizer": "random_search",
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_filip_original(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    if str(args.filip_mode).strip().lower() == "exact_reference":
        backend_token = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
        backend = "metal" if backend_token == "metal" else "opencl"
        run_rc = _run_py("run_filip_reference_exact.py", _filip_exact_args(args))
    else:
        backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
        run_rc = _run_py("run_filip_original.py", _filip_original_args(args, backend))
    out_dir = _latest_new_optimization_dir(before)
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "filip_case": args.filip_case,
            "filip_mode": args.filip_mode,
            "optimizer": "exhaustive_sweep",
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_filip_firefly(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    run_rc = _run_py("run_firefly_optimization.py", _fem_safe_args_firefly(args, backend))
    out_dir = _latest_new_optimization_dir(before)
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "optimizer": "firefly",
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def main() -> None:
    ap = argparse.ArgumentParser(description="Unified benchmark/autotuning workflows.")
    ap.add_argument(
        "--workflow",
        choices=[
            "cpu_benchmark",
            "gpu_benchmark",
            "cpu_real_kernels",
            "gpu_real_kernels",
            "filip_original",
            "filip_autotune",
            "filip_firefly",
        ],
        required=True,
    )
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument("--platform-profile", choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"], default="auto")
    ap.add_argument("--arch", choices=["auto", "apple", "x86", "intel", "amd", "generic"], default="auto")
    ap.add_argument("--backend", choices=["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel"], default="auto")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--roofline-ai", type=float, default=8.0)
    ap.add_argument("--roofline-bytes", type=float, default=1_000_000_000.0)
    ap.add_argument("--real-runs", type=int, default=3)
    ap.add_argument("--real-fem-sizes", default="20000,100000,500000")
    ap.add_argument("--real-fem-element-type", choices=["tet4", "hex8", "prism6"], default="tet4")
    ap.add_argument(
        "--real-fem-operator",
        choices=["diffusion", "mass", "convection", "diffusion_mass", "diffusion_convection_mass", "laplace", "test"],
        default="diffusion_mass",
    )
    ap.add_argument("--real-fem-n-qp", type=int, default=4)
    ap.add_argument("--trials", type=int, default=96)
    ap.add_argument("--population", type=int, default=12)
    ap.add_argument("--iterations", type=int, default=20)
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--filip-case", choices=["portable", "laplace_prism", "test_prism", "prism_pair"], default="prism_pair")
    ap.add_argument("--filip-mode", choices=["portable_sweep", "exact_reference"], default="portable_sweep")
    ap.add_argument("--filip-modfem-dir", default="")
    ap.add_argument("--filip-input-override", default="")
    ap.add_argument("--filip-dump-launch-artifacts", action="store_true")
    ap.add_argument("--filip-export-replay-inputs", action="store_true")
    ap.add_argument("--filip-export-replay-include-expected-output", action="store_true")
    ap.add_argument("--filip-replay-dump-root", default="")
    args = ap.parse_args()

    if args.workflow == "cpu_benchmark":
        raise SystemExit(run_cpu_benchmark(args))
    if args.workflow == "gpu_benchmark":
        raise SystemExit(run_gpu_benchmark(args))
    if args.workflow == "cpu_real_kernels":
        raise SystemExit(run_cpu_real_kernels(args))
    if args.workflow == "gpu_real_kernels":
        raise SystemExit(run_gpu_real_kernels(args))
    if args.workflow == "filip_original":
        raise SystemExit(run_filip_original(args))
    if args.workflow == "filip_autotune":
        raise SystemExit(run_filip_autotune(args))
    raise SystemExit(run_filip_firefly(args))


if __name__ == "__main__":
    main()
