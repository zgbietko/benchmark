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

from analysis.contract_utils import (
    standardize_campaign_artifacts,
    standardize_optimization_artifacts,
    standardize_session_artifacts,
    standardize_validation_artifacts,
)
from analysis.google_drive_sync import (
    default_google_drive_dir,
    default_google_drive_subdir,
    default_rclone_remote,
    default_sync_mode,
    sync_artifacts_to_google_drive,
)
from profiles.loader import load_profile
from run_session import create_session_dir, manifest_base, write_manifest


PLATFORM_PROFILES_PATH = ROOT / "configs" / "platform_profiles.json"
GPU_BACKENDS = ("metal", "cuda", "hip", "opencl")


def _apply_experiment_profile(args: argparse.Namespace) -> argparse.Namespace:
    profile_name = str(getattr(args, "experiment_profile", "") or "").strip()
    if not profile_name:
        return args
    try:
        profile = load_profile(profile_name)
    except Exception as exc:
        raise SystemExit(f"Nie można wczytać experiment profile '{profile_name}': {exc}")

    args.benchmark_mode = str(profile.get("benchmark_mode", args.benchmark_mode))
    args.repeats = int(profile.get("repetitions", args.repeats))
    args.warmups = int(profile.get("warmups", args.warmups))
    args.real_runs = int(profile.get("real_runs", args.real_runs))
    args.trials = int(profile.get("trials", args.trials))
    args.population = int(profile.get("population", args.population))
    args.iterations = int(profile.get("iterations", args.iterations))
    return args


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


def _generate_thesis_tables(run_dir: Path) -> dict[str, str]:
    path = ROOT / "analysis" / "tables" / "generate_thesis_tables.py"
    if not path.exists():
        return {}
    cmd = [sys.executable, str(path), "--run-dir", str(run_dir)]
    rc = subprocess.run(cmd, cwd=ROOT, check=False).returncode
    if rc != 0:
        return {}
    tables_dir = Path(run_dir).resolve() / "tables"
    return {"thesis_tables_dir": str(tables_dir)} if tables_dir.exists() else {}


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


def _fem_option_validation_dirs() -> set[str]:
    base = ROOT / "data" / "fem_option_validation"
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


def _latest_new_fem_option_validation_dir(before: set[str]) -> Path | None:
    base = ROOT / "data" / "fem_option_validation"
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


def _cpu_thread_env(limit: int) -> dict[str, str]:
    limit = int(limit or 0)
    if limit <= 0:
        return {}
    value = str(limit)
    return {
        "OMP_NUM_THREADS": value,
        "OPENBLAS_NUM_THREADS": value,
        "MKL_NUM_THREADS": value,
        "VECLIB_MAXIMUM_THREADS": value,
        "NUMEXPR_NUM_THREADS": value,
        "BLIS_NUM_THREADS": value,
        "ACCELERATE_NTHREADS": value,
        "V4_CPU_MAX_THREADS": value,
    }


def _cpu_benchmark_args(mode: str, max_threads: int) -> list[str]:
    out = ["--arch-profile", "auto", "--benchmark-mode", str(mode)]
    if int(max_threads or 0) > 0:
        out += ["--max-threads", str(int(max_threads))]
    return out


def _write_session_manifest(session_dir: Path, payload: dict[str, Any]) -> None:
    manifest = manifest_base(str(payload.get("profile", "custom")))
    manifest.update(payload)
    write_manifest(session_dir, manifest)


def _google_drive_sync_payload(args: argparse.Namespace) -> dict[str, str]:
    return {
        "mode": str(getattr(args, "google_drive_sync", "") or default_sync_mode()),
        "google_drive_dir": str(getattr(args, "google_drive_dir", "") or default_google_drive_dir()),
        "rclone_remote": str(getattr(args, "google_drive_rclone_remote", "") or default_rclone_remote()),
        "subdir": str(getattr(args, "google_drive_subdir", "") or default_google_drive_subdir()),
    }


def _maybe_google_drive_sync(source_dir: Path | None, args: argparse.Namespace) -> dict[str, Any]:
    if source_dir is None:
        return {}
    cfg = _google_drive_sync_payload(args)
    if str(cfg["mode"]).strip().lower() in {"", "off", "disabled", "none"}:
        return {}
    sync_info = sync_artifacts_to_google_drive(
        source_dir=Path(source_dir),
        mode=cfg["mode"],
        google_drive_dir=cfg["google_drive_dir"],
        rclone_remote=cfg["rclone_remote"],
        subdir=cfg["subdir"],
        root=ROOT,
    )
    return sync_info


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
                for key in (
                    "figure_set_dir",
                    "figure_set_plots",
                    "figure_appendix_dir",
                    "figure_appendix_plots",
                    "figure_manifest_path",
                ):
                    if key in summary:
                        payload[key] = summary.get(key)
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
    include_ai: bool,
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
    if include_ai:
        results["ai_accel_summary"] = _run_py(
            "analysis/ai_accel_summary.py",
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
        "--benchmark-mode",
        str(args.benchmark_mode),
        "--with-fem-integration",
        "--fem-integration-element-type",
        args.real_fem_element_type,
        "--fem-integration-operator",
        args.real_fem_operator,
        "--fem-integration-n-qp",
        str(args.real_fem_n_qp),
        "--warmups",
        str(int(getattr(args, "warmups", 0) or 0)),
    ]


def _portable_real_kernels_args(args: argparse.Namespace, backend: str) -> list[str]:
    return [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--runs",
        str(args.real_runs),
        "--benchmark-mode",
        str(args.benchmark_mode),
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
        "--warmups",
        str(int(getattr(args, "warmups", 0) or 0)),
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
        "--repeats",
        str(args.repeats),
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
    if bool(getattr(args, "filip_export_canonical_replay_bundles", False)):
        out += ["--export-canonical-replay-bundles"]
    if str(getattr(args, "filip_replay_dump_root", "")).strip():
        out += ["--replay-dump-root", str(args.filip_replay_dump_root).strip()]
    if int(getattr(args, "filip_limit_option_rows", 0) or 0) > 0:
        out += ["--limit-option-rows", str(int(args.filip_limit_option_rows))]
    return out


def _fem_option_validation_args(args: argparse.Namespace, backend: str) -> list[str]:
    out = [
        "--backend",
        backend,
        "--device-index",
        str(args.device_index),
        "--repeats",
        str(args.repeats),
    ]
    if str(getattr(args, "fem_option_validation_operators", "")).strip():
        out += ["--operators", str(args.fem_option_validation_operators).strip()]
    if str(getattr(args, "fem_option_validation_variants", "")).strip():
        out += ["--variants", str(args.fem_option_validation_variants).strip()]
    if int(getattr(args, "fem_option_validation_n_elements", 0)) > 0:
        out += ["--n-elements", str(int(args.fem_option_validation_n_elements))]
    if int(getattr(args, "fem_option_validation_n_qp", 0)) > 0:
        out += ["--n-qp", str(int(args.fem_option_validation_n_qp))]
    if int(getattr(args, "fem_option_validation_workgroup_size", 0)) > 0:
        out += ["--workgroup-size", str(int(args.fem_option_validation_workgroup_size))]
    return out


def _profiler_correlation_args(args: argparse.Namespace) -> list[str]:
    optimization_dir = str(getattr(args, "correlation_optimization_dir", "")).strip()
    fem_option_validation_dir = str(getattr(args, "correlation_fem_option_validation_dir", "")).strip()
    if not optimization_dir:
        raise SystemExit("Profiler correlation requires --correlation-optimization-dir.")
    if not fem_option_validation_dir:
        raise SystemExit("Profiler correlation requires --correlation-fem-option-validation-dir.")
    out = [
        "--optimization-dir",
        optimization_dir,
        "--fem-option-validation-dir",
        fem_option_validation_dir,
    ]
    correlation_out = str(getattr(args, "correlation_out_dir", "")).strip()
    if correlation_out:
        out += ["--out", correlation_out]
    for report in list(getattr(args, "correlation_profiler_report", []) or []):
        report_str = str(report).strip()
        if report_str:
            out += ["--profiler-report", report_str]
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


def _author_assembly_safe_args_firefly(args: argparse.Namespace, backend: str) -> list[str]:
    n_range = "10000:250000" if backend in ("cpu", "cuda") else "10000:120000"
    return [
        "--problem",
        "author_assembly",
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
        "--assembly-n-elements-range",
        n_range,
        "--assembly-n-qp-range",
        "1:8",
        "--assembly-n-dofs-choices",
        "4,6,8",
        "--assembly-variants",
        "qss,sqs,ssq",
        "--assembly-workspace-choices",
        "0,1",
        "--assembly-scatter-choices",
        "0,1",
        "--assembly-padding-choices",
        "0,1",
        "--assembly-dtypes",
        "float32",
    ]


def run_cpu_benchmark(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    env.update(_cpu_thread_env(int(getattr(args, "benchmarks_max_cpu_threads", 0) or 0)))
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
            "benchmark_mode": args.benchmark_mode,
        },
    )
    run_rc = _run_py(
        "run_all_cpu_benchmarks.py",
        _cpu_benchmark_args(str(args.benchmark_mode), int(getattr(args, "benchmarks_max_cpu_threads", 0) or 0)),
        env=env,
    )
    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="cpu",
        backend="cpu",
        include_cpu=True,
        include_gpu=False,
        include_real=False,
        include_ai=False,
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
            "benchmark_mode": args.benchmark_mode,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    contract_info = standardize_session_artifacts(
        session_dir=session_dir,
        workflow=args.workflow,
        resolved_backend="cpu",
        warmups=int(getattr(args, "warmups", 0) or 0),
        command_args=sys.argv[1:],
    )
    contract_info.update(_generate_thesis_tables(session_dir))
    sync_info = _maybe_google_drive_sync(session_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "cpu",
            "benchmark_mode": args.benchmark_mode,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "contracts": contract_info,
            "google_drive_sync": sync_info,
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
            "benchmark_mode": args.benchmark_mode,
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
            "--benchmark-mode",
            str(args.benchmark_mode),
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
        include_ai=False,
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
            "benchmark_mode": args.benchmark_mode,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    contract_info = standardize_session_artifacts(
        session_dir=session_dir,
        workflow=args.workflow,
        resolved_backend=backend,
        warmups=int(getattr(args, "warmups", 0) or 0),
        command_args=sys.argv[1:],
    )
    contract_info.update(_generate_thesis_tables(session_dir))
    sync_info = _maybe_google_drive_sync(session_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "benchmark_mode": args.benchmark_mode,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": exit_code,
        },
    )


def run_cpu_real_kernels(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    env.update(_cpu_thread_env(int(getattr(args, "real_kernels_max_cpu_threads", 0) or 0)))
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "cpu",
            "real_kernels_backend": "cpu",
            "benchmark_mode": args.benchmark_mode,
        },
    )
    run_rc = _run_py(
        "run_all_cpu_benchmarks.py",
        _cpu_benchmark_args(str(args.benchmark_mode), int(getattr(args, "real_kernels_max_cpu_threads", 0) or 0)),
        env=env,
    )
    rk_rc = _run_py("real_kernels/run_all_real_kernels.py", _native_real_kernels_args(args, "cpu"), env=env)
    analysis_rc = _run_session_analysis(
        session_dir=session_dir,
        env=env,
        target="cpu",
        backend="cpu",
        include_cpu=True,
        include_gpu=False,
        include_real=True,
        include_ai=False,
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
            "benchmark_mode": args.benchmark_mode,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    contract_info = standardize_session_artifacts(
        session_dir=session_dir,
        workflow=args.workflow,
        resolved_backend="cpu",
        warmups=int(getattr(args, "warmups", 0) or 0),
        command_args=sys.argv[1:],
    )
    contract_info.update(_generate_thesis_tables(session_dir))
    sync_info = _maybe_google_drive_sync(session_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "cpu",
            "real_kernels_backend": "cpu",
            "benchmark_mode": args.benchmark_mode,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "contracts": contract_info,
            "google_drive_sync": sync_info,
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
            "benchmark_mode": args.benchmark_mode,
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
            "--benchmark-mode",
            str(args.benchmark_mode),
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
        include_ai=False,
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
            "benchmark_mode": args.benchmark_mode,
            "real_kernels_mode": real_mode,
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "exit_code": exit_code,
        },
    )
    contract_info = standardize_session_artifacts(
        session_dir=session_dir,
        workflow=args.workflow,
        resolved_backend=backend,
        warmups=int(getattr(args, "warmups", 0) or 0),
        command_args=sys.argv[1:],
    )
    contract_info.update(_generate_thesis_tables(session_dir))
    sync_info = _maybe_google_drive_sync(session_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "gpu",
            "resolved_backend": backend,
            "benchmark_mode": args.benchmark_mode,
            "real_kernels_mode": real_mode,
            "session_dir": str(session_dir),
            "roofline_dir": str(session_dir / "roofline"),
            "analysis": analysis_rc,
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": exit_code,
        },
    )


def run_ai_accel(args: argparse.Namespace) -> int:
    session_dir = create_session_dir(args.profile)
    env = _session_env(session_dir, args.profile)
    env.update(_cpu_thread_env(int(getattr(args, "real_kernels_max_cpu_threads", 0) or 0)))
    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "ai_accel",
            "backend_request": str(args.backend),
            "benchmark_mode": args.benchmark_mode,
        },
    )

    ai_args = [
        "--backend",
        str(args.backend),
        "--device-index",
        str(args.device_index),
        "--benchmark-mode",
        str(args.benchmark_mode),
        "--runs",
        str(max(int(args.real_runs), 1)),
        "--warmups",
        str(max(int(getattr(args, "warmups", 0) or 0), 0)),
    ]
    if str(getattr(args, "ai_shapes", "")).strip():
        ai_args += ["--shapes", str(args.ai_shapes).strip()]
    if str(getattr(args, "ai_dtypes", "")).strip():
        ai_args += ["--dtypes", str(args.ai_dtypes).strip()]
    if not bool(getattr(args, "ai_include_cpu_baseline", True)):
        ai_args += ["--no-include-cpu-baseline"]
    if not bool(getattr(args, "ai_coreml_ne_probe", True)):
        ai_args += ["--no-coreml-ne-probe"]

    run_rc = _run_py("ai_accel/run_ai_accel_suite.py", ai_args, env=env)
    summary_rc = _run_py(
        "analysis/ai_accel_summary.py",
        ["--scope", "session", "--session", session_dir.name],
        env=env,
    )
    ai_plots_rc = _run_py(
        "analysis/generate_ai_accel_plots.py",
        ["--scope", "session", "--session", session_dir.name],
        env=env,
    )
    path_report_rc = _run_py(
        "analysis/ai_accel_path_report.py",
        ["--scope", "session", "--session", session_dir.name, "--out", str(session_dir / "ai_accel_path_report.json")],
        env=env,
    )
    exit_code = 0 if run_rc == 0 and summary_rc == 0 and ai_plots_rc == 0 and path_report_rc == 0 else 1

    _write_session_manifest(
        session_dir,
        {
            "profile": args.profile,
            "launcher": "run_workflow.py",
            "workflow": args.workflow,
            "target": "ai_accel",
            "backend_request": str(args.backend),
            "benchmark_mode": args.benchmark_mode,
                "analysis": {
                    "ai_accel_summary": int(summary_rc),
                    "ai_accel_plots": int(ai_plots_rc),
                    "ai_accel_path_report": int(path_report_rc),
                },
            "exit_code": exit_code,
        },
    )
    contract_info = standardize_session_artifacts(
        session_dir=session_dir,
        workflow=args.workflow,
        resolved_backend=str(args.backend),
        warmups=int(getattr(args, "warmups", 0) or 0),
        command_args=sys.argv[1:],
    )
    path_report_path = session_dir / "ai_accel_path_report.json"
    if path_report_path.exists():
        contract_info["ai_accel_path_report"] = str(path_report_path)
    contract_info.update(_generate_thesis_tables(session_dir))
    sync_info = _maybe_google_drive_sync(session_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _session_result(
        session_dir,
        {
            "workflow": args.workflow,
            "target": "ai_accel",
            "backend_request": str(args.backend),
            "benchmark_mode": args.benchmark_mode,
            "session_dir": str(session_dir),
            "analysis": {
                "ai_accel_summary": int(summary_rc),
                "ai_accel_plots": int(ai_plots_rc),
                "ai_accel_path_report": int(path_report_rc),
            },
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": exit_code,
        },
    )


def run_filip_autotune(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    env = _cpu_thread_env(int(getattr(args, "filip_max_cpu_threads", 0) or 0))
    run_rc = _run_py("run_filip_autotuning.py", _fem_safe_args_filip(args, backend), env=env)
    out_dir = _latest_new_optimization_dir(before)
    contract_info = (
        standardize_optimization_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=backend,
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir is not None
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "optimizer": "random_search",
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_filip_original(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    env = _cpu_thread_env(int(getattr(args, "filip_max_cpu_threads", 0) or 0))
    if str(args.filip_mode).strip().lower() == "exact_reference":
        backend_token = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
        backend = "metal" if backend_token == "metal" else "opencl"
        run_rc = _run_py("run_filip_reference_exact.py", _filip_exact_args(args), env=env)
    else:
        backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
        run_rc = _run_py("run_filip_original.py", _filip_original_args(args, backend), env=env)
    out_dir = _latest_new_optimization_dir(before)
    contract_info = (
        standardize_optimization_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=backend,
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir is not None
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "filip_case": args.filip_case,
            "filip_mode": args.filip_mode,
            "optimizer": "exhaustive_sweep",
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_fem_option_validation(args: argparse.Namespace) -> int:
    before = _fem_option_validation_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    if backend in ("amd", "intel"):
        backend = "opencl"
    env = _cpu_thread_env(int(getattr(args, "filip_max_cpu_threads", 0) or 0))
    run_rc = _run_py("run_fem_option_validation.py", _fem_option_validation_args(args, backend), env=env)
    out_dir = _latest_new_fem_option_validation_dir(before)
    contract_info = (
        standardize_validation_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=backend,
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir is not None
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "experiment_class": "fem_option_validation",
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_profiler_correlation(args: argparse.Namespace) -> int:
    correlation_out = str(getattr(args, "correlation_out_dir", "")).strip()
    run_rc = _run_py("scripts/run_profiler_correlation.py", _profiler_correlation_args(args))
    out_dir = Path(correlation_out).expanduser().resolve() if correlation_out else (
        Path(str(args.correlation_optimization_dir)).expanduser().resolve() / "profiler_correlation"
    )
    contract_info = (
        standardize_optimization_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=str(getattr(args, "backend", "")),
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir.exists()
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir if out_dir.exists() else None, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "experiment_class": "profiler_correlation",
            "optimization_dir": str(args.correlation_optimization_dir),
            "fem_option_validation_dir": str(args.correlation_fem_option_validation_dir),
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_filip_firefly(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    env = _cpu_thread_env(int(getattr(args, "filip_max_cpu_threads", 0) or 0))
    run_rc = _run_py("run_firefly_optimization.py", _fem_safe_args_firefly(args, backend), env=env)
    out_dir = _latest_new_optimization_dir(before)
    contract_info = (
        standardize_optimization_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=backend,
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir is not None
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "optimizer": "firefly",
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def run_author_assembly_firefly(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    env = _cpu_thread_env(int(getattr(args, "real_kernels_max_cpu_threads", 0) or 0))
    run_rc = _run_py("run_firefly_optimization.py", _author_assembly_safe_args_firefly(args, backend), env=env)
    out_dir = _latest_new_optimization_dir(before)
    contract_info = (
        standardize_optimization_artifacts(
            out_dir=out_dir,
            workflow=args.workflow,
            resolved_backend=backend,
            warmups=int(getattr(args, "warmups", 0) or 0),
            command_args=sys.argv[1:],
        )
        if out_dir is not None
        else {}
    )
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    return _optimization_result(
        out_dir,
        {
            "workflow": args.workflow,
            "resolved_backend": backend,
            "optimizer": "firefly",
            "problem": "author_assembly",
            "contracts": contract_info,
            "google_drive_sync": sync_info,
            "exit_code": 0 if run_rc == 0 else 1,
        },
    )


def _full_thesis_pipeline_args(args: argparse.Namespace) -> list[str]:
    out = [
        "--profile",
        "full",
        "--experiment-profile",
        str(getattr(args, "experiment_profile", "") or ""),
        "--platform-profile",
        str(args.platform_profile),
        "--arch",
        str(args.arch),
        "--backend",
        str(args.backend),
        "--benchmark-mode",
        str(args.benchmark_mode),
        "--benchmarks-max-cpu-threads",
        str(int(getattr(args, "benchmarks_max_cpu_threads", 0) or 0)),
        "--real-kernels-max-cpu-threads",
        str(int(getattr(args, "real_kernels_max_cpu_threads", 0) or 0)),
        "--filip-max-cpu-threads",
        str(int(getattr(args, "filip_max_cpu_threads", 0) or 0)),
        "--device-index",
        str(args.device_index),
        "--roofline-ai",
        str(args.roofline_ai),
        "--roofline-bytes",
        str(args.roofline_bytes),
        "--real-runs",
        str(max(int(args.real_runs), 5)),
        "--repeats",
        str(max(int(args.repeats), 5)),
        "--warmups",
        str(max(int(getattr(args, "warmups", 0) or 0), 0)),
        "--trials",
        str(max(int(args.trials), 256)),
        "--population",
        str(max(int(args.population), 24)),
        "--iterations",
        str(max(int(args.iterations), 40)),
        "--filip-case",
        str(args.filip_case),
        "--fem-option-validation-operators",
        str(args.fem_option_validation_operators),
        "--fem-option-validation-variants",
        str(args.fem_option_validation_variants),
        "--fem-option-validation-n-elements",
        str(max(int(args.fem_option_validation_n_elements), 16384)),
        "--fem-option-validation-n-qp",
        str(max(int(args.fem_option_validation_n_qp), 6)),
        "--fem-option-validation-workgroup-size",
        str(max(int(args.fem_option_validation_workgroup_size), 64)),
        "--google-drive-sync",
        str(getattr(args, "google_drive_sync", "") or default_sync_mode()),
        "--google-drive-dir",
        str(getattr(args, "google_drive_dir", "") or default_google_drive_dir()),
        "--google-drive-rclone-remote",
        str(getattr(args, "google_drive_rclone_remote", "") or default_rclone_remote()),
        "--google-drive-subdir",
        str(getattr(args, "google_drive_subdir", "") or default_google_drive_subdir()),
    ]
    if str(getattr(args, "filip_modfem_dir", "")).strip():
        out += ["--filip-modfem-dir", str(args.filip_modfem_dir).strip()]
    if str(getattr(args, "filip_input_override", "")).strip():
        out += ["--filip-input-override", str(args.filip_input_override).strip()]
    if str(getattr(args, "filip_replay_dump_root", "")).strip():
        out += ["--filip-replay-dump-root", str(args.filip_replay_dump_root).strip()]
    for report in list(getattr(args, "correlation_profiler_report", []) or []):
        report_str = str(report).strip()
        if report_str:
            out += ["--correlation-profiler-report", report_str]
    if bool(getattr(args, "full_pipeline_smoke", False)):
        out += ["--smoke"]
    return out


def run_full_thesis_pipeline(args: argparse.Namespace) -> int:
    before = _optimization_dirs()
    run_rc = _run_py("run_full_thesis_pipeline.py", _full_thesis_pipeline_args(args))
    base = ROOT / "data" / "thesis_full"
    out_dir: Path | None = None
    if base.exists():
        dirs = [p for p in base.iterdir() if p.is_dir()]
        if dirs:
            out_dir = max(dirs, key=lambda p: p.stat().st_mtime)
    contract_info = {}
    if out_dir is not None:
        summary_path = out_dir / "summary.json"
        if summary_path.exists():
            try:
                summary = json.loads(summary_path.read_text(encoding="utf-8"))
                contract_info = standardize_campaign_artifacts(campaign_dir=out_dir, summary=summary)
            except Exception:
                contract_info = {}
    sync_info = _maybe_google_drive_sync(out_dir, args)
    if sync_info:
        contract_info["google_drive_sync"] = sync_info
    payload = {
        "workflow": args.workflow,
        "experiment_class": "thesis_full_campaign",
        "profile": "full",
        "contracts": contract_info,
        "google_drive_sync": sync_info,
        "exit_code": 0 if run_rc == 0 else 1,
    }
    return _optimization_result(out_dir, payload)


def main() -> None:
    ap = argparse.ArgumentParser(description="Unified benchmark/autotuning workflows.")
    ap.add_argument(
        "--workflow",
        choices=[
            "cpu_benchmark",
            "gpu_benchmark",
            "cpu_real_kernels",
            "gpu_real_kernels",
            "ai_accel",
            "filip_original",
            "fem_option_validation",
            "profiler_correlation",
            "filip_autotune",
            "filip_firefly",
            "author_assembly_firefly",
            "full_thesis_pipeline",
        ],
        required=True,
    )
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument("--experiment-profile", default="")
    ap.add_argument("--platform-profile", choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"], default="auto")
    ap.add_argument("--arch", choices=["auto", "apple", "x86", "intel", "amd", "generic"], default="auto")
    ap.add_argument("--backend", choices=["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel"], default="auto")
    ap.add_argument("--benchmark-mode", choices=["standard", "extended"], default="standard")
    ap.add_argument("--benchmarks-max-cpu-threads", type=int, default=0)
    ap.add_argument("--real-kernels-max-cpu-threads", type=int, default=0)
    ap.add_argument("--filip-max-cpu-threads", type=int, default=0)
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
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--filip-case", choices=["portable", "laplace_prism", "test_prism", "prism_pair"], default="prism_pair")
    ap.add_argument("--filip-mode", choices=["portable_sweep", "exact_reference"], default="portable_sweep")
    ap.add_argument("--filip-modfem-dir", default="")
    ap.add_argument("--filip-input-override", default="")
    ap.add_argument("--filip-dump-launch-artifacts", action="store_true")
    ap.add_argument("--filip-export-replay-inputs", action="store_true")
    ap.add_argument("--filip-export-replay-include-expected-output", action="store_true")
    ap.add_argument("--filip-export-canonical-replay-bundles", action="store_true")
    ap.add_argument("--filip-replay-dump-root", default="")
    ap.add_argument("--filip-limit-option-rows", type=int, default=0, help=argparse.SUPPRESS)
    ap.add_argument("--fem-option-validation-operators", default="laplace,test")
    ap.add_argument("--fem-option-validation-variants", default="qss,sqs,ssq")
    ap.add_argument("--fem-option-validation-n-elements", type=int, default=4096)
    ap.add_argument("--fem-option-validation-n-qp", type=int, default=6)
    ap.add_argument("--fem-option-validation-workgroup-size", type=int, default=64)
    ap.add_argument("--correlation-optimization-dir", default="")
    ap.add_argument("--correlation-fem-option-validation-dir", default="")
    ap.add_argument("--correlation-profiler-report", action="append", default=[])
    ap.add_argument("--correlation-out-dir", default="")
    ap.add_argument("--google-drive-sync", choices=["off", "auto", "folder", "rclone"], default=default_sync_mode())
    ap.add_argument("--google-drive-dir", default=default_google_drive_dir())
    ap.add_argument("--google-drive-rclone-remote", default=default_rclone_remote())
    ap.add_argument("--google-drive-subdir", default=default_google_drive_subdir())
    ap.add_argument("--ai-shapes", default="")
    ap.add_argument("--ai-dtypes", default="")
    ap.add_argument("--ai-include-cpu-baseline", action="store_true", default=True)
    ap.add_argument("--no-ai-include-cpu-baseline", dest="ai_include_cpu_baseline", action="store_false")
    ap.add_argument("--ai-coreml-ne-probe", action="store_true", default=True)
    ap.add_argument("--no-ai-coreml-ne-probe", dest="ai_coreml_ne_probe", action="store_false")
    ap.add_argument("--full-pipeline-smoke", action="store_true", help=argparse.SUPPRESS)
    args = ap.parse_args()
    args = _apply_experiment_profile(args)

    if args.workflow == "cpu_benchmark":
        raise SystemExit(run_cpu_benchmark(args))
    if args.workflow == "gpu_benchmark":
        raise SystemExit(run_gpu_benchmark(args))
    if args.workflow == "cpu_real_kernels":
        raise SystemExit(run_cpu_real_kernels(args))
    if args.workflow == "gpu_real_kernels":
        raise SystemExit(run_gpu_real_kernels(args))
    if args.workflow == "ai_accel":
        raise SystemExit(run_ai_accel(args))
    if args.workflow == "filip_original":
        raise SystemExit(run_filip_original(args))
    if args.workflow == "fem_option_validation":
        raise SystemExit(run_fem_option_validation(args))
    if args.workflow == "profiler_correlation":
        raise SystemExit(run_profiler_correlation(args))
    if args.workflow == "filip_autotune":
        raise SystemExit(run_filip_autotune(args))
    if args.workflow == "author_assembly_firefly":
        raise SystemExit(run_author_assembly_firefly(args))
    if args.workflow == "full_thesis_pipeline":
        raise SystemExit(run_full_thesis_pipeline(args))
    raise SystemExit(run_filip_firefly(args))


if __name__ == "__main__":
    main()
