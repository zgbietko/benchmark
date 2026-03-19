#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def _run_py(rel: str, args: list[str] | None = None) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[ERROR] Missing script: {path}")
        return 1
    cmd = [sys.executable, str(path)]
    if args:
        cmd.extend(args)
    print(f"\n=== RUN: {rel} {' '.join(args or [])} ===\n")
    return subprocess.run(cmd, cwd=ROOT, check=False).returncode


def _ask(prompt: str, default: str = "") -> str:
    s = input(f"{prompt}{f' [{default}]' if default else ''}: ").strip()
    if s == "" and default != "":
        return default
    return s


def _ask_int(prompt: str, default: int) -> int:
    while True:
        raw = _ask(prompt, str(default))
        try:
            return int(raw)
        except ValueError:
            print(f"[WARN] Not an integer: {raw}")


def _ask_menu(prompt: str, options: list[tuple[str, str]], default_idx: int = 1) -> str:
    while True:
        print(f"\n{prompt}")
        for i, (_value, label) in enumerate(options, start=1):
            print(f"  {i}. {label}")
        raw = _ask(f"Select [1-{len(options)}]", str(default_idx))
        try:
            idx = int(raw)
            if 1 <= idx <= len(options):
                return options[idx - 1][0]
        except ValueError:
            pass
        print(f"[WARN] Invalid option: {raw}")


def _ask_yes_no(prompt: str, default_yes: bool) -> bool:
    default_idx = 1 if default_yes else 2
    return _ask_menu(prompt, [("y", "yes"), ("n", "no")], default_idx) == "y"


def _pause() -> None:
    input("\nPress Enter to continue...")


def _clear() -> None:
    if os.name == "nt":
        os.system("cls")
    else:
        os.system("clear")


def _prompt_profile() -> str:
    return _ask_menu("Experiment profile", [("quick", "quick"), ("paper", "paper"), ("full", "full")], 2)


def _prompt_platform_profile() -> str:
    return _ask_menu(
        "Platform profile",
        [
            ("auto", "auto"),
            ("apple", "apple"),
            ("nvidia", "nvidia"),
            ("amd", "amd"),
            ("intel_arc", "intel arc"),
            ("intel_igpu", "intel iGPU"),
        ],
        1,
    )


def _prompt_arch() -> str:
    return _ask_menu(
        "Architecture",
        [("auto", "auto"), ("apple", "apple"), ("x86", "x86"), ("intel", "intel"), ("amd", "amd"), ("generic", "generic")],
        1,
    )


def _prompt_backend(include_all: bool = True) -> str:
    opts = [("auto", "auto"), ("metal", "metal"), ("cuda", "cuda"), ("hip", "hip"), ("opencl", "opencl")]
    if include_all:
        opts.append(("all", "all"))
    return _ask_menu("GPU backend", opts, 1)


def _recommended_gpu_backend(platform_profile: str) -> str:
    if platform_profile == "apple":
        return "metal"
    if platform_profile == "nvidia":
        return "cuda"
    if platform_profile == "amd":
        return "hip"
    if platform_profile in ("intel_arc", "intel_igpu"):
        return "opencl"
    return "cuda"


def action_guided_thesis_workflow() -> None:
    print("\nGuided workflow: Microbench -> Practical validation (real_kernels) -> Analysis")

    profile = _prompt_profile()
    platform_profile = _prompt_platform_profile()
    device_index = _ask_int("GPU device index", 0)

    include_real = _ask_yes_no("Phase 2: run real_kernels validation", True)
    include_fem = _ask_yes_no("Include FEM integration in real_kernels", False) if include_real else False
    run_plots = _ask_yes_no("Generate plots at the end", True)
    run_report = _ask_yes_no("Generate markdown report at the end", True)

    # Phase 1: microbench only
    print("\n[PHASE 1/3] Microbench campaign (CPU+GPU, without real_kernels)")
    args = [
        "--profile",
        profile,
        "--platform-profile",
        platform_profile,
        "--arch",
        "auto",
        "--backend",
        "auto",
        "--device-index",
        str(device_index),
        "--no-real-kernels",
        "--no-interactive-arch",
    ]
    rc = _run_py("run_all_benchmarks.py", args)
    if rc != 0:
        print(f"\n[ERROR] Phase 1 failed (exit={rc}).")
        return

    # Phase 2: practical kernels validation
    if include_real:
        print("\n[PHASE 2/3] Practical validation with real_kernels")
        rec_backend = _recommended_gpu_backend(platform_profile)
        rk_backend = _ask_menu(
            "real_kernels backend",
            [("cpu", "cpu"), (rec_backend, f"{rec_backend} (recommended)"), ("all", "all")],
            2,
        )
        rk_runs = _ask_int("Runs per kernel", 3)

        rk_args = [
            "--backend",
            rk_backend,
            "--device-index",
            str(device_index),
            "--runs",
            str(rk_runs),
        ]

        if include_fem:
            elem = _ask_menu("FEM element type", [("tet4", "tet4"), ("hex8", "hex8")], 1)
            op = _ask_menu(
                "FEM operator",
                [
                    ("diffusion", "diffusion"),
                    ("mass", "mass"),
                    ("convection", "convection"),
                    ("diffusion_mass", "diffusion + mass"),
                    ("diffusion_convection_mass", "diffusion + convection + mass"),
                ],
                5,
            )
            nqp = _ask_menu("FEM quadrature points", [("1", "1"), ("4", "4"), ("8", "8")], 2)
            rk_args += [
                "--with-fem-integration",
                "--fem-integration-element-type",
                elem,
                "--fem-integration-operator",
                op,
                "--fem-integration-n-qp",
                nqp,
            ]

        rc = _run_py("real_kernels/run_all_real_kernels.py", rk_args)
        if rc != 0:
            print(f"\n[ERROR] Phase 2 failed (exit={rc}).")
            return

    # Phase 3: analysis
    print("\n[PHASE 3/3] Summaries + roofline + data quality")
    _run_py("analysis/cpu_summary.py", ["--mode", "latest"])
    _run_py("analysis/gpu_summary.py", ["--mode", "latest"])
    _run_py("analysis/real_kernels_summary.py", [])

    roof_backend = _recommended_gpu_backend(platform_profile)
    if roof_backend not in ("cuda", "metal", "hip", "opencl"):
        roof_backend = "cuda"
    _run_py(
        "analysis/roofline_model.py",
        ["--target", "both", "--backend", roof_backend, "--ai", "8", "--bytes", "1000000000", "--scope", "session"],
    )
    _run_py("analysis/data_quality.py", ["--scope", "session", "--strict"])

    if run_plots:
        _run_py("analysis/generate_plots.py", [])
    if run_report:
        _run_py(
            "analysis/report.py",
            [
                "--mode",
                "latest",
                "--roofline-target",
                "both",
                "--roofline-backend",
                roof_backend,
                "--roofline-ai",
                "8",
                "--roofline-bytes",
                "1000000000",
                "--with-plots",
            ],
        )


def action_microbench_campaign() -> None:
    profile = _prompt_profile()
    platform_profile = _prompt_platform_profile()
    arch = _prompt_arch()
    backend = _prompt_backend(include_all=True)
    device_index = _ask_int("GPU device index", 0)
    include_real = _ask_yes_no("Include real_kernels", False)

    args = [
        "--profile",
        profile,
        "--platform-profile",
        platform_profile,
        "--arch",
        arch,
        "--backend",
        backend,
        "--device-index",
        str(device_index),
        "--no-interactive-arch",
    ]
    if include_real:
        args.append("--with-real-kernels")
    else:
        args.append("--no-real-kernels")

    rc = _run_py("run_all_benchmarks.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_microbench_all_backends() -> None:
    profile = _prompt_profile()
    platform_profile = _prompt_platform_profile()
    arch = _prompt_arch()
    device_index = _ask_int("GPU device index", 0)
    include_real = _ask_yes_no("Include real_kernels", False)

    args = [
        "--profile",
        profile,
        "--platform-profile",
        platform_profile,
        "--arch",
        arch,
        "--device-index",
        str(device_index),
        "--no-interactive-arch",
    ]
    if include_real:
        args.append("--with-real-kernels")
    else:
        args.append("--no-real-kernels")

    rc = _run_py("run_all_backends.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_cpu_only() -> None:
    profile = _ask_menu("CPU profile", [("auto", "auto"), ("generic", "generic"), ("intel", "intel"), ("amd", "amd")], 1)
    rc = _run_py("run_all_cpu_benchmarks.py", ["--arch-profile", profile])
    print(f"\n[INFO] Exit code: {rc}")


def action_gpu_only() -> None:
    platform_profile = _prompt_platform_profile()
    arch = _ask_menu("Architecture", [("auto", "auto"), ("apple", "apple"), ("x86", "x86"), ("intel", "intel"), ("amd", "amd")], 1)
    backend = _prompt_backend(include_all=True)
    device_index = _ask_int("GPU device index", 0)
    list_devices = _ask_yes_no("List devices only", False)
    interactive_device = _ask_yes_no("Interactive device picker", False)

    args = ["--platform-profile", platform_profile, "--arch", arch, "--backend", backend, "--device-index", str(device_index)]
    if list_devices:
        args.append("--list-devices")
    if interactive_device:
        args.append("--interactive-device")

    rc = _run_py("run_all_gpu_benchmarks.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_real_kernels() -> None:
    backend = _ask_menu("real_kernels backend", [("cpu", "cpu"), ("cuda", "cuda"), ("metal", "metal"), ("all", "all")], 4)
    device_index = _ask_int("GPU device index", 0)
    runs = _ask_int("Runs per kernel", 3)
    with_fem_int = _ask_yes_no("Include FEM integration", False)

    args = ["--backend", backend, "--device-index", str(device_index), "--runs", str(runs)]
    if with_fem_int:
        elem = _ask_menu("FEM element type", [("tet4", "tet4"), ("hex8", "hex8")], 1)
        op = _ask_menu(
            "FEM operator",
            [
                ("diffusion", "diffusion"),
                ("mass", "mass"),
                ("convection", "convection"),
                ("diffusion_mass", "diffusion + mass"),
                ("diffusion_convection_mass", "diffusion + convection + mass"),
            ],
            5,
        )
        nqp = _ask_menu("FEM quadrature points", [("1", "1"), ("4", "4"), ("8", "8")], 2)
        args += [
            "--with-fem-integration",
            "--fem-integration-element-type",
            elem,
            "--fem-integration-operator",
            op,
            "--fem-integration-n-qp",
            nqp,
        ]

    rc = _run_py("real_kernels/run_all_real_kernels.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_firefly_optimizer() -> None:
    problem = _ask_menu("Optimization problem", [("gpu_memory", "gpu_memory"), ("gpu_fma", "gpu_fma")], 1)
    backend = _ask_menu("GPU backend", [("cuda", "cuda"), ("hip", "hip"), ("metal", "metal"), ("opencl", "opencl")], 1)
    device_index = _ask_int("GPU device index", 0)
    objective_mode = _ask_menu("Objective mode", [("weighted", "weighted"), ("pareto", "pareto")], 1)
    population = _ask_int("Population size", 16)
    iterations = _ask_int("Iterations", 25)
    repeats = _ask_int("Repeats per config", 3)

    args = [
        "--problem",
        problem,
        "--backend",
        backend,
        "--device-index",
        str(device_index),
        "--objective-mode",
        objective_mode,
        "--population",
        str(population),
        "--iterations",
        str(iterations),
        "--repeats",
        str(repeats),
    ]

    if problem == "gpu_memory":
        size_profile = _ask_menu("Size range", [("4:16", "tiny (4..16 MB)"), ("4:64", "small (4..64 MB)"), ("4:256", "medium (4..256 MB)")], 2)
        iters_profile = _ask_menu("Iters range", [("1:8", "tiny (1..8)"), ("1:16", "small (1..16)"), ("5:100", "medium (5..100)")], 2)
        objective = _ask_menu(
            "Memory objective",
            [
                ("gbps_mean:max:1.0", "Max GB/s"),
                ("gbps_mean:max:1.0,j_per_gb:min:0.2", "GB/s + energy"),
                ("gbps_mean:max:1.0,cv_gbps:min:0.2", "GB/s + stability"),
            ],
            1,
        )
        args += ["--size-mb-range", size_profile, "--iters-range", iters_profile, "--objectives", objective]
    else:
        n_profile = _ask_menu("n_elements range", [("0.25:8.0", "small"), ("0.25:16.0", "medium"), ("0.25:32.0", "large")], 2)
        iters_profile = _ask_menu("iters_inner range", [("200:5000", "short"), ("200:10000", "medium"), ("500:20000", "long")], 2)
        objective = _ask_menu(
            "FMA objective",
            [
                ("gflops_mean:max:1.0", "Max GFLOP/s"),
                ("gflops_mean:max:1.0,j_per_gflop:min:0.3", "GFLOP/s + energy"),
                ("gflops_mean:max:1.0,cv_gflops:min:0.2", "GFLOP/s + stability"),
            ],
            1,
        )
        args += ["--n-elements-m-range", n_profile, "--iters-inner-range", iters_profile, "--objectives", objective]

    rc = _run_py("run_firefly_optimization.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_summaries() -> None:
    mode = _ask_menu("Summary mode", [("latest", "latest"), ("all", "all")], 1)
    rc1 = _run_py("analysis/cpu_summary.py", ["--mode", mode])
    rc2 = _run_py("analysis/gpu_summary.py", ["--mode", mode])
    rc3 = _run_py("analysis/real_kernels_summary.py", [])
    print(f"\n[INFO] Exit codes: cpu={rc1}, gpu={rc2}, real={rc3}")


def action_roofline() -> None:
    target = _ask_menu("Target", [("cpu", "cpu"), ("gpu", "gpu"), ("both", "both")], 2)
    backend = ""
    if target in ("gpu", "both"):
        backend = _ask_menu("GPU backend", [("cuda", "cuda"), ("metal", "metal"), ("hip", "hip"), ("opencl", "opencl")], 1)
    ai = _ask_menu("Arithmetic intensity (FLOP/byte)", [("1", "1"), ("4", "4"), ("8", "8"), ("16", "16"), ("32", "32")], 3)
    bytes_total = _ask_menu("Bytes moved", [("100000000", "1e8"), ("1000000000", "1e9"), ("10000000000", "1e10")], 2)
    scope = _ask_menu("Data scope", [("global", "global"), ("session", "session")], 2)

    args = ["--target", target, "--ai", ai, "--bytes", bytes_total, "--scope", scope]
    if backend:
        args.extend(["--backend", backend])
    rc = _run_py("analysis/roofline_model.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_plots() -> None:
    rc = _run_py("analysis/generate_plots.py", [])
    print(f"\n[INFO] Exit code: {rc}")


def action_report() -> None:
    mode = _ask_menu("Summary mode", [("latest", "latest"), ("all", "all")], 1)
    target = _ask_menu("Roofline target", [("cpu", "cpu"), ("gpu", "gpu"), ("both", "both")], 2)
    backend = "cuda"
    if target in ("gpu", "both"):
        backend = _ask_menu("Roofline backend", [("cuda", "cuda"), ("metal", "metal"), ("hip", "hip"), ("opencl", "opencl")], 1)
    ai = _ask_menu("Roofline AI", [("1", "1"), ("4", "4"), ("8", "8"), ("16", "16")], 3)
    bytes_total = _ask_menu("Roofline bytes", [("100000000", "1e8"), ("1000000000", "1e9"), ("10000000000", "1e10")], 2)
    with_plots = _ask_yes_no("Include plots", True)

    args = ["--mode", mode, "--roofline-target", target, "--roofline-ai", ai, "--roofline-bytes", bytes_total]
    if target in ("gpu", "both"):
        args += ["--roofline-backend", backend]
    if with_plots:
        args += ["--with-plots"]

    rc = _run_py("analysis/report.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_data_quality() -> None:
    strict = _ask_yes_no("Strict mode", False)
    scope = _ask_menu("Data scope", [("auto", "auto"), ("session", "session"), ("global", "global")], 1)
    min_conf = _ask_menu("Min energy confidence", [("0.0", "0.0"), ("0.2", "0.2"), ("0.5", "0.5"), ("0.8", "0.8")], 2)

    args = ["--scope", scope, "--min-energy-confidence", min_conf]
    if strict:
        args.append("--strict")

    rc = _run_py("analysis/data_quality.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def action_normalize_gpu_csv() -> None:
    mode = _ask_menu("Normalize mode", [("all", "all"), ("latest", "latest")], 1)
    dry_run = _ask_yes_no("Dry run", False)
    backup = _ask_yes_no("Create backup", True)

    args = ["--mode", mode]
    if dry_run:
        args.append("--dry-run")
    if not backup:
        args.append("--no-backup")

    rc = _run_py("analysis/normalize_gpu_csv.py", args)
    print(f"\n[INFO] Exit code: {rc}")


def main() -> None:
    actions = {
        "1": ("Guided Thesis Workflow (microbench -> practical validation -> analysis)", action_guided_thesis_workflow),
        "2": ("Microbench Campaign (CPU + GPU)", action_microbench_campaign),
        "3": ("Microbench Campaign (All GPU Backends)", action_microbench_all_backends),
        "4": ("CPU Microbench Only", action_cpu_only),
        "5": ("GPU Microbench Only", action_gpu_only),
        "6": ("Real Kernels Validation", action_real_kernels),
        "7": ("Firefly Optimizer (Advanced)", action_firefly_optimizer),
        "8": ("Summaries (CPU/GPU/real_kernels)", action_summaries),
        "9": ("Roofline Model", action_roofline),
        "10": ("Generate Plots", action_plots),
        "11": ("Generate Markdown Report", action_report),
        "12": ("Data Quality Checks", action_data_quality),
        "13": ("Normalize GPU CSV (legacy fix)", action_normalize_gpu_csv),
        "14": ("Exit", None),
    }

    while True:
        _clear()
        print("=== Microbench Console App ===")
        print(f"Workspace: {ROOT}")
        print("\nGoal: characterize architectures with microbenchmarks and validate with practical kernels.\n")

        for k in sorted(actions.keys(), key=int):
            print(f"{k}. {actions[k][0]}")

        choice = input("\nSelect option (1-14): ").strip()
        if choice not in actions:
            print(f"[WARN] Invalid option: {choice}")
            _pause()
            continue

        if choice == "14":
            print("Bye.")
            return

        fn = actions[choice][1]
        try:
            assert fn is not None
            fn()
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted by user.")
        except Exception as e:
            print(f"\n[ERROR] {e}")
        _pause()


if __name__ == "__main__":
    main()
