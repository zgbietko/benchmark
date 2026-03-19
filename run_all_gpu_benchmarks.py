#!/usr/bin/env python3
"""run_all_gpu_benchmarks.py

Uruchamia zestaw benchmarków GPU dla:
- macOS: Metal (Apple GPU) lub OpenCL
- Linux: CUDA (NVIDIA), HIP (AMD/ROCm) lub OpenCL (cross-vendor)

CLI:
  --backend auto|metal|cuda|hip|opencl|all
  --arch auto|apple|x86|intel|amd
  --platform-profile auto|apple|nvidia|amd|intel_arc|intel_igpu
  --device-index N
  --interactive-device
  --list-devices
  --build-libs   (spróbuj zbudować brakujące biblioteki)
"""

from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional


ROOT = Path(__file__).resolve().parent
PLATFORM_PROFILES_PATH = ROOT / "configs" / "platform_profiles.json"

from device_resolution import list_opencl_devices, resolve_device_index


def _run_py(rel: str, args: Optional[List[str]] = None) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[WARN] Pomijam {rel} (brak pliku).")
        return 0
    cmd = [sys.executable, str(path)]
    if args:
        cmd += args
    print(f"\n=== Uruchamiam: {rel} ===")
    return subprocess.run(cmd, check=False).returncode


def _try_build(rel_script: str) -> bool:
    path = ROOT / rel_script
    if not path.exists():
        return False
    print(f"[INFO] Buduję: {rel_script}")
    rc = subprocess.run([str(path)], check=False, cwd=str(path.parent)).returncode
    return rc == 0


def _backend_available_cuda() -> bool:
    try:
        from gpu.cuda.cuda_backend import get_device_count  # type: ignore
        return get_device_count() > 0
    except Exception:
        return False


def _backend_available_hip() -> bool:
    try:
        from gpu.hip.hip_backend import get_device_count  # type: ignore
        return get_device_count() > 0
    except Exception:
        return False


def _backend_available_opencl() -> bool:
    try:
        from gpu.opencl.opencl_backend import get_device_count  # type: ignore
        return get_device_count() > 0
    except Exception:
        return False


def _backend_available_metal() -> bool:
    if platform.system() != "Darwin":
        return False
    try:
        from gpu.metal.metal_backend import MetalBackend  # type: ignore
        _ = MetalBackend(device_index=0)
        return True
    except Exception:
        return False


def _load_platform_profiles() -> dict:
    if not PLATFORM_PROFILES_PATH.exists():
        return {}
    try:
        return json.loads(PLATFORM_PROFILES_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}


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
    for b in order:
        if _backend_available(b):
            return b
    return None


def _list_devices(backend: str) -> None:
    if backend == "cuda":
        from gpu.cuda.cuda_backend import get_device_count, get_device_name  # type: ignore
        n = get_device_count()
        print(f"CUDA devices: {n}")
        for i in range(n):
            print(f"  [{i}] {get_device_name(i)}")
        return

    if backend == "hip":
        from gpu.hip.hip_backend import get_device_count, get_device_name  # type: ignore
        n = get_device_count()
        print(f"HIP devices: {n}")
        for i in range(n):
            print(f"  [{i}] {get_device_name(i)}")
        return

    if backend == "metal":
        try:
            from gpu.metal.metal_backend import MetalBackend  # type: ignore
            # Metal: best-effort list (0..3)
            print("Metal devices (best-effort):")
            for i in range(4):
                try:
                    b = MetalBackend(device_index=i)
                    print(f"  [{i}] {b.device_name}")
                except Exception:
                    break
        except Exception as e:
            print(f"[WARN] Nie mogę wypisać urządzeń Metal: {e}")
        return
    if backend == "opencl":
        try:
            infos = list_opencl_devices()
            n = len(infos)
            print(f"OpenCL devices: {n}")
            for info in infos:
                print(
                    f"  [{info['index']}] {info['device_name']} | "
                    f"vendor={info['device_vendor']} | type={info['device_type']} | "
                    f"platform={info['platform_name']}"
                )
        except Exception as e:
            print(f"[WARN] Nie mogę wypisać urządzeń OpenCL: {e}")
        return


def _get_device_count(backend: str) -> int:
    if backend == "cuda":
        try:
            from gpu.cuda.cuda_backend import get_device_count  # type: ignore
            return int(get_device_count())
        except Exception:
            return 0
    if backend == "hip":
        try:
            from gpu.hip.hip_backend import get_device_count  # type: ignore
            return int(get_device_count())
        except Exception:
            return 0
    if backend == "opencl":
        try:
            from gpu.opencl.opencl_backend import get_device_count  # type: ignore
            return int(get_device_count())
        except Exception:
            return 0
    if backend == "metal":
        try:
            from gpu.metal.metal_backend import MetalBackend  # type: ignore
            count = 0
            for i in range(16):
                try:
                    MetalBackend(device_index=i)
                    count += 1
                except Exception:
                    break
            return count
        except Exception:
            return 0
    return 0


def _select_device_index(backend: str, requested: int, interactive: bool) -> int:
    n = _get_device_count(backend)
    if n <= 0:
        return requested

    if 0 <= requested < n:
        return requested

    if n == 1:
        return 0

    if interactive and sys.stdin.isatty():
        print(f"[INFO] Wykryto {n} urządzeń dla backendu {backend}.")
        _list_devices(backend)
        while True:
            raw = input(f"Wybierz device-index [0..{n-1}] (domyślnie 0): ").strip()
            if raw == "":
                return 0
            try:
                idx = int(raw)
                if 0 <= idx < n:
                    return idx
            except ValueError:
                pass
            print(f"[WARN] Niepoprawny indeks: '{raw}'.")

    print(f"[WARN] device-index={requested} poza zakresem [0..{n-1}] dla backendu {backend}; używam 0.")
    return 0


def _probe_gpu_energy(device_index: int) -> None:
    try:
        from energy_utils import EnergyLogger, gpu_energy_capabilities  # type: ignore
    except Exception as e:
        print(f"[INFO] Energy probe: energy_utils niedostępny ({e})")
        return

    caps = gpu_energy_capabilities(device_index=device_index)
    src = str(caps.get("energy_source", "unavailable"))
    avail = bool(caps.get("energy_available", False))
    print(f"[INFO] Energy probe: source={src}, available={'yes' if avail else 'no'}")
    if not avail:
        return

    logger = EnergyLogger(domain="gpu", device_index=device_index)
    try:
        logger.start()
        time.sleep(0.15)
        e_j, p_w = logger.stop()
        print(f"[INFO] Energy probe: source={src}, sample_energy={e_j:.6f} J, sample_power={p_w:.3f} W")
    except Exception as e:
        print(f"[INFO] Energy probe: source={src}, probe failed ({e})")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--arch", choices=["auto", "apple", "x86", "intel", "amd"], default="auto")
    ap.add_argument(
        "--platform-profile",
        choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"],
        default="auto",
        help="Profil platformy (mapuje domyślny arch/backend).",
    )
    ap.add_argument("--backend", choices=["auto", "metal", "cuda", "hip", "opencl", "all"], default="auto")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--interactive-device", action="store_true")
    ap.add_argument("--list-devices", action="store_true")
    ap.add_argument("--build-libs", action="store_true", help="spróbuj zbudować brakujące biblioteki (CUDA/HIP)")
    args = ap.parse_args()

    system = platform.system()
    profiles = _load_platform_profiles()
    selected_profile = profiles.get(args.platform_profile, {}) if args.platform_profile != "auto" else {}

    if args.arch == "auto" and isinstance(selected_profile, dict):
        prof_arch = selected_profile.get("arch")
        if isinstance(prof_arch, str) and prof_arch in ("apple", "x86", "intel", "amd"):
            args.arch = prof_arch

    print("=== Uruchamianie benchmarków GPU ===")
    print(f"[INFO] Wykryty system: {system}")
    print(f"[INFO] Profil platformy: {args.platform_profile}")

    backend = args.backend
    if backend == "auto":
        pref_order: list[str] = []
        if isinstance(selected_profile, dict):
            raw_pref = selected_profile.get("backend_preference", [])
            if isinstance(raw_pref, list):
                pref_order = [str(x) for x in raw_pref if str(x) in ("metal", "cuda", "hip", "opencl")]

        if not pref_order:
            if args.arch == "apple":
                pref_order = ["metal", "opencl"]
            elif args.arch == "amd":
                pref_order = ["hip", "opencl", "cuda"]
            elif args.arch in ("x86", "intel"):
                pref_order = ["cuda", "hip", "opencl"]
            elif system == "Darwin":
                pref_order = ["metal", "opencl"]
            else:
                pref_order = ["cuda", "hip", "opencl"]

        selected = _pick_backend_by_order(pref_order)
        backend = selected if selected is not None else pref_order[0]

    if system == "Darwin" and backend not in ("metal", "opencl", "all"):
        print("[ERROR] Na macOS wspierany jest tylko backend metal lub opencl.")
        sys.exit(2)

    if args.build_libs and backend in ("cuda", "hip"):
        if backend == "cuda":
            # build if lib missing
            lib_dir = ROOT / "gpu" / "cuda" / "lib"
            if not any((lib_dir / n).exists() for n in ("libgpubench_cuda.so", "libgpubench_cuda.dylib", "gpubench_cuda.dll")):
                _try_build("gpu/cuda/lib/build_cuda.sh")
        if backend == "hip":
            lib_dir = ROOT / "gpu" / "hip" / "lib"
            if not any((lib_dir / n).exists() for n in ("libgpubench_hip.so", "libgpubench_hip.dylib", "gpubench_hip.dll")):
                _try_build("gpu/hip/lib/build_hip.sh")

    if args.list_devices:
        profile_all_backends: list[str] = []
        if isinstance(selected_profile, dict):
            raw_all = selected_profile.get("all_backends", [])
            if isinstance(raw_all, list):
                profile_all_backends = [str(x) for x in raw_all if str(x) in ("metal", "cuda", "hip", "opencl")]
        if backend == "all":
            if profile_all_backends:
                backends = profile_all_backends
            else:
                backends = ["metal", "opencl"] if system == "Darwin" else ["cuda", "hip", "opencl"]
            for b in backends:
                # skip unavailable
                if b == "cuda" and not _backend_available_cuda():
                    continue
                if b == "hip" and not _backend_available_hip():
                    continue
                if b == "opencl" and not _backend_available_opencl():
                    continue
                print(f"\n=== {b.upper()} ===")
                _list_devices(b)
        else:
            _list_devices(backend)
        return

    selected_device_index = args.device_index
    if backend in ("metal", "cuda", "hip", "opencl"):
        selected_device_index = _select_device_index(
            backend=backend,
            requested=args.device_index,
            interactive=args.interactive_device,
        )
        selected_device_index, resolution_reason = resolve_device_index(
            backend,
            selected_device_index,
            platform_profile=args.platform_profile,
        )
        if backend == "opencl":
            print(
                f"[INFO] OpenCL device resolution: requested={args.device_index}, "
                f"used={selected_device_index}, reason={resolution_reason}"
            )

    if backend in ("cuda", "hip", "opencl", "metal"):
        _probe_gpu_energy(selected_device_index)

    dev_arg = ["--device-index", str(selected_device_index)]

    def _run_backend(b: str) -> bool:
        ok_local = True
        if b == "metal":
            ok_local = ok_local and (_run_py("gpu/metal/benchmarks/run_metal_bandwidth.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/metal/benchmarks/run_metal_pointer_latency.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/metal/benchmarks/run_metal_compute_fma.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/metal/benchmarks/run_metal_compute_fma_peak.py", dev_arg) == 0)
        elif b == "cuda":
            ok_local = ok_local and (_run_py("gpu/cuda/benchmarks/run_cuda_bandwidth.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/cuda/benchmarks/run_cuda_pointer_latency.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/cuda/benchmarks/run_cuda_compute_fma.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/cuda/benchmarks/run_cuda_compute_fma_peak.py", dev_arg) == 0)
        elif b == "hip":
            ok_local = ok_local and (_run_py("gpu/hip/benchmarks/run_hip_bandwidth.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/hip/benchmarks/run_hip_pointer_latency.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/hip/benchmarks/run_hip_compute_fma.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/hip/benchmarks/run_hip_compute_fma_peak.py", dev_arg) == 0)
        elif b == "opencl":
            ok_local = ok_local and (_run_py("gpu/opencl/benchmarks/run_opencl_bandwidth.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/opencl/benchmarks/run_opencl_pointer_latency.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/opencl/benchmarks/run_opencl_compute_fma.py", dev_arg) == 0)
            ok_local = ok_local and (_run_py("gpu/opencl/benchmarks/run_opencl_compute_fma_peak.py", dev_arg) == 0)
        return ok_local

    ok = True
    profile_all_backends: list[str] = []
    if isinstance(selected_profile, dict):
        raw_all = selected_profile.get("all_backends", [])
        if isinstance(raw_all, list):
            profile_all_backends = [str(x) for x in raw_all if str(x) in ("metal", "cuda", "hip", "opencl")]

    if backend == "all":
        if profile_all_backends:
            backends = profile_all_backends
        else:
            backends = ["metal", "opencl"] if system == "Darwin" else ["cuda", "hip", "opencl"]
        for b in backends:
            if b == "cuda" and not _backend_available_cuda():
                continue
            if b == "hip" and not _backend_available_hip():
                continue
            if b == "opencl" and not _backend_available_opencl():
                continue
            ok = ok and _run_backend(b)
    else:
        ok = _run_backend(backend)

    if not ok:
        print("\n[ERROR] Co najmniej jeden benchmark GPU zakończył się błędem.")
        sys.exit(1)

    print("\n[OK] Wszystkie benchmarki GPU zakończone pomyślnie.")


if __name__ == "__main__":
    main()
