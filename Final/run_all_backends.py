#!/usr/bin/env python3
"""run_all_backends.py

Uruchamia pełną kampanię benchmarków:
- CPU (run_all_cpu_benchmarks.py)
- GPU (run_all_gpu_benchmarks.py) dla wszystkich dostępnych backendów

Przykłady:
  python3 run_all_backends.py
  python3 run_all_backends.py --skip-gpu
  python3 run_all_backends.py --device-index 1
  python3 run_all_backends.py --list-devices
"""

from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from pathlib import Path

from cpu_utils import detect_cpu_model
from run_session import create_session_dir, manifest_base, write_manifest


ROOT = Path(__file__).resolve().parent
PROFILES_PATH = ROOT / "configs" / "experiment_profiles.json"
PLATFORM_PROFILES_PATH = ROOT / "configs" / "platform_profiles.json"


def _run_py(rel: str, args: list[str] | None = None, extra_env: dict[str, str] | None = None) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[WARN] Pomijam {rel} (brak pliku).")
        return 0
    cmd = [sys.executable, str(path)]
    if args:
        cmd += args
    print(f"\n=== Uruchamiam: {rel} ===")
    env = None
    if extra_env:
        import os
        env = dict(os.environ)
        env.update(extra_env)
    return subprocess.run(cmd, check=False, env=env).returncode


def _available_architectures() -> list[tuple[str, str]]:
    system = platform.system()
    machine = platform.machine().lower()
    cpu_model = detect_cpu_model().lower()

    if system == "Darwin":
        return [
            ("apple", "macOS / Apple Silicon (CPU generic + GPU Metal)"),
        ]

    if machine in ("x86_64", "amd64"):
        if "intel" in cpu_model:
            return [
                ("intel", "x86 Intel profile"),
                ("x86", "x86 auto profile"),
                ("generic", "generic CPU profile"),
            ]
        if "amd" in cpu_model:
            return [
                ("amd", "x86 AMD profile"),
                ("x86", "x86 auto profile"),
                ("generic", "generic CPU profile"),
            ]
        return [
            ("x86", "x86 auto profile"),
            ("generic", "generic CPU profile"),
        ]

    return [
        ("generic", "generic CPU profile"),
    ]


def _choose_arch_interactive(default_arch: str) -> str:
    options = _available_architectures()
    if not options:
        return default_arch

    print("\n=== Wybór architektury ===")
    for i, (name, desc) in enumerate(options, start=1):
        print(f"  [{i}] {name:8s} - {desc}")

    default_idx = 1
    if default_arch != "auto":
        for i, (name, _desc) in enumerate(options, start=1):
            if name == default_arch:
                default_idx = i
                break

    while True:
        raw = input(f"Wybierz architekturę [1..{len(options)}] (domyślnie {default_idx}): ").strip()
        if raw == "":
            return options[default_idx - 1][0]
        try:
            idx = int(raw)
            if 1 <= idx <= len(options):
                return options[idx - 1][0]
        except ValueError:
            pass
        print(f"[WARN] Niepoprawny wybór: '{raw}'")


def _load_profiles() -> dict:
    if not PROFILES_PATH.exists():
        return {}
    try:
        return json.loads(PROFILES_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _load_platform_profiles() -> dict:
    if not PLATFORM_PROFILES_PATH.exists():
        return {}
    try:
        return json.loads(PLATFORM_PROFILES_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Run all CPU + all GPU backends (full campaign)."
    )
    ap.add_argument("--skip-cpu", action="store_true", help="Pomiń CPU")
    ap.add_argument("--skip-gpu", action="store_true", help="Pomiń GPU")
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument(
        "--arch",
        choices=["auto", "apple", "x86", "intel", "amd", "generic"],
        default="auto",
        help="Profil architektury dla części CPU (wpływa na --arch-profile).",
    )
    ap.add_argument(
        "--platform-profile",
        choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"],
        default="auto",
        help="Profil platformy (mapuje domyślny arch/backend dla GPU).",
    )
    ap.add_argument("--device-index", type=int, default=0, help="Indeks GPU")
    ap.add_argument("--list-devices", action="store_true", help="Wypisz urządzenia dla wszystkich backendów GPU")
    ap.add_argument("--with-real-kernels", action="store_true", help="Uruchom dodatkowo moduł real_kernels.")
    ap.add_argument(
        "--no-real-kernels",
        action="store_true",
        help="Nie uruchamiaj real_kernels (nawet jeśli profil domyślnie je włącza).",
    )
    ap.add_argument(
        "--interactive-arch",
        dest="interactive_arch",
        action="store_true",
        default=True,
        help="Pokaż dostępne architektury i zapytaj o wybór przed uruchomieniem.",
    )
    ap.add_argument(
        "--no-interactive-arch",
        dest="interactive_arch",
        action="store_false",
        help="Wyłącz interaktywny wybór architektury.",
    )
    args = ap.parse_args()

    profiles = _load_profiles()
    platform_profiles = _load_platform_profiles()
    profile_cfg = profiles.get(args.profile, {})
    if args.platform_profile == "auto" and isinstance(profile_cfg, dict):
        prof_platform = str(profile_cfg.get("platform_profile", "auto"))
        if prof_platform in ("auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"):
            args.platform_profile = prof_platform
    platform_cfg = platform_profiles.get(args.platform_profile, {}) if args.platform_profile != "auto" else {}
    if args.arch == "auto" and isinstance(profile_cfg, dict):
        args.arch = profile_cfg.get("arch", args.arch)
    if args.arch == "auto" and isinstance(platform_cfg, dict):
        args.arch = str(platform_cfg.get("arch", args.arch))
    if args.no_real_kernels:
        args.with_real_kernels = False

    if args.interactive_arch and sys.stdin.isatty():
        args.arch = _choose_arch_interactive(args.arch)

    cpu_arch_profile = "auto"
    if args.arch in ("intel", "amd", "generic"):
        cpu_arch_profile = args.arch
    elif args.arch == "apple":
        cpu_arch_profile = "generic"

    session_dir = create_session_dir(args.profile)
    env = {"BENCH_RUN_DIR": str(session_dir), "BENCH_PROFILE": args.profile}
    manifest = manifest_base(args.profile)
    manifest.update(
        {
            "launcher": "run_all_backends.py",
            "arch": args.arch,
            "platform_profile": args.platform_profile,
            "device_index": args.device_index,
            "with_real_kernels": args.with_real_kernels,
            "cpu_arch_profile": cpu_arch_profile,
            "profile_config": profile_cfg,
        }
    )
    write_manifest(session_dir, manifest)
    print(f"[INFO] Session dir: {session_dir}")

    ok = True

    if args.list_devices:
        ok = ok and (
            _run_py(
                "run_all_gpu_benchmarks.py",
                ["--arch", args.arch, "--platform-profile", args.platform_profile, "--backend", "all", "--list-devices"],
                extra_env=env,
            )
            == 0
        )
        return

    if not args.skip_cpu:
        ok = ok and (
            _run_py("run_all_cpu_benchmarks.py", ["--arch-profile", cpu_arch_profile], extra_env=env) == 0
        )

    if not args.skip_gpu:
        ok = ok and (
            _run_py(
                "run_all_gpu_benchmarks.py",
                [
                    "--arch",
                    args.arch,
                    "--platform-profile",
                    args.platform_profile,
                    "--backend",
                    "all",
                    "--device-index",
                    str(args.device_index),
                ],
                extra_env=env,
            )
            == 0
        )

    if args.with_real_kernels:
        rk_backend = profile_cfg.get("real_kernels_backend", "all") if isinstance(profile_cfg, dict) else "all"
        if not isinstance(rk_backend, str):
            rk_backend = "all"
        if rk_backend not in ("cpu", "cuda", "metal", "all"):
            rk_backend = "all"
        rk_runs = str(profile_cfg.get("real_kernels_runs", 3)) if isinstance(profile_cfg, dict) else "3"
        rk_gemm_shapes = str(profile_cfg.get("real_kernels_gemm_shapes", "512x512x512,1024x1024x1024")) if isinstance(profile_cfg, dict) else "512x512x512,1024x1024x1024"
        rk_reduction_sizes = str(profile_cfg.get("real_kernels_reduction_sizes", "1000000,5000000,10000000,50000000")) if isinstance(profile_cfg, dict) else "1000000,5000000,10000000,50000000"
        ok = ok and (
            _run_py(
                "real_kernels/run_all_real_kernels.py",
                [
                    "--backend",
                    rk_backend,
                    "--device-index",
                    str(args.device_index),
                    "--runs",
                    rk_runs,
                    "--gemm-shapes",
                    rk_gemm_shapes,
                    "--reduction-sizes",
                    rk_reduction_sizes,
                ],
                extra_env=env,
            )
            == 0
        )
        _run_py("analysis/real_kernels_summary.py", [], extra_env=env)

    dq_args: list[str] = []
    quality_scope = "auto"
    quality_strict = False
    if isinstance(profile_cfg, dict):
        raw_scope = str(profile_cfg.get("quality_scope", "auto"))
        if raw_scope in ("auto", "global", "session"):
            quality_scope = raw_scope
        quality_strict = bool(profile_cfg.get("quality_strict", False))
    if quality_scope != "auto":
        dq_args += ["--scope", quality_scope]
    if quality_strict:
        dq_args.append("--strict")
    dq_rc = _run_py("analysis/data_quality.py", dq_args, extra_env=env)
    if quality_strict:
        ok = ok and (dq_rc == 0)

    if not ok:
        print("\n[ERROR] Co najmniej jeden etap zakończył się błędem.")
        sys.exit(1)

    print("\n[OK] Pełna kampania benchmarków zakończona pomyślnie.")


if __name__ == "__main__":
    main()
