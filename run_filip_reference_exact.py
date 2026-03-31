#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from datetime import datetime
import json
import math
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
from typing import Any, Iterable
import zipfile

import numpy as np


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

DEFAULT_EXACT_BUNDLE_DIR = ROOT / "legacy" / "filip_exact_bundle" / "mod_2022"
DEFAULT_ORIGINAL_MOD_DIR = ROOT / "Kod Filipa" / "mod_2022"

_mpl_cfg = ROOT / ".cache" / "matplotlib"
_mpl_cfg.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(_mpl_cfg))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # type: ignore

from analysis.filip_article_plots import _estimate_rw_bytes, _estimate_share, _roofline_peaks_for_backend
from device_resolution import resolve_device_index
from optimization.problems import FemParametricProblem, FemParametricProblemConfig


VARIANT_ORDER = ["qss", "sqs", "ssq"]
OPTION_HEADERS = [
    "-D COAL_READ",
    "-D COAL_WRITE",
    "-D COMPUTE_ALL_SHAPE_FUN_DER",
    "-D USE_WORKSPACE_FOR_PDE_COEFF",
    "-D USE_WORKSPACE_FOR_GEO_DATA",
    "-D USE_WORKSPACE_FOR_SHAPE_FUN",
    "-D USE_WORKSPACE_FOR_STIFF_MAT",
    "-D WORKSPACE_PADDING=0",
    "-D WORKSPACE_PADDING=1",
]
RESULT_FIELD_ORDER = [
    *OPTION_HEADERS,
    "el_data_in [MB]",
    "el_data_out [MB]",
    "nr_elems_per_work_group",
    "nr_elems",
    "nr_elems_per_thread",
    "nr_work_groups",
    "work_group_size",
    "nr_threads",
    "sending el_data_in to GPU memory",
    "input_bandwidth_[GB/s]",
    "executing kernel",
    "internal",
    "copying output buffer",
    "output_bandwidth_[GB/s]",
]
PLOT_COLOR = {"qss": "#111111", "sqs": "#b8bcc2", "ssq": "#6b7280"}
PROFILE_SCALE = {"quick": 0.5, "paper": 1.0, "full": 1.5}
BASE_ELEMENTS = {
    "cpu": {"tet4": 12_000, "hex8": 6_000, "prism6": 9_000},
    "native_gpu": {"tet4": 64_000, "hex8": 24_000, "prism6": 48_000},
    "mapped_gpu": {"tet4": 32_000, "hex8": 12_000, "prism6": 24_000},
}


@dataclass(frozen=True)
class ExactCaseSpec:
    case_name: str
    label: str
    operator: str
    element_type: str
    n_qp: int
    arch: str
    work_subdir: str
    binary_name: str = "MFEM_conv_diff_prism_std_krb_ocl"


CASE_SPECS: dict[str, tuple[ExactCaseSpec, ...]] = {
    "laplace_prism": (
        ExactCaseSpec(
            case_name="laplace_prism",
            label="Laplace prism",
            operator="laplace",
            element_type="prism6",
            n_qp=6,
            arch="arc_laplace",
            work_subdir="work/diff_in_box",
        ),
    ),
    "test_prism": (
        ExactCaseSpec(
            case_name="test_prism",
            label="TEST prism",
            operator="test",
            element_type="prism6",
            n_qp=6,
            arch="arc_test",
            work_subdir="work/test_scalar",
        ),
    ),
}
CASE_SPECS["prism_pair"] = (*CASE_SPECS["laplace_prism"], *CASE_SPECS["test_prism"])


def _resolve_exact_backend(requested: str) -> str:
    backend = str(requested).strip().lower()
    if backend == "metal":
        return "metal"
    if backend in {"opencl", "intel"}:
        return "opencl"
    if backend == "auto":
        if platform.system() == "Darwin":
            try:
                import Metal  # type: ignore

                if Metal.MTLCreateSystemDefaultDevice() is not None:
                    return "metal"
            except Exception:
                pass
        return "opencl"
    raise SystemExit("Exact Filip reference mode supports only backend=opencl/intel/metal/auto.")


def _dedupe_paths(values: Iterable[str]) -> list[str]:
    out: list[str] = []
    seen: set[str] = set()
    for value in values:
        item = str(value or "").strip()
        if not item or item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def _prepend_env_paths(env: dict[str, str], key: str, values: Iterable[str]) -> None:
    current = [part for part in str(env.get(key, "")).split(":") if part]
    env[key] = ":".join(_dedupe_paths([*(str(v) for v in values), *current]))


def _candidate_compiler_roots() -> list[Path]:
    base = Path("/opt/intel/oneapi/compiler")
    if not base.exists():
        return []
    roots: list[Path] = []
    latest = base / "latest"
    if latest.exists():
        roots.append(latest.resolve())
    for child in sorted(base.iterdir(), reverse=True):
        if child.is_dir():
            roots.append(child.resolve())
    out: list[Path] = []
    seen: set[str] = set()
    for item in roots:
        key = str(item)
        if key in seen:
            continue
        seen.add(key)
        out.append(item)
    return out


def _candidate_mkl_roots() -> list[Path]:
    base = Path("/opt/intel/oneapi/mkl")
    if not base.exists():
        return []
    roots: list[Path] = []
    latest = base / "latest"
    if latest.exists():
        roots.append(latest.resolve())
    for child in sorted(base.iterdir(), reverse=True):
        if child.is_dir():
            roots.append(child.resolve())
    out: list[Path] = []
    seen: set[str] = set()
    for item in roots:
        key = str(item)
        if key in seen:
            continue
        seen.add(key)
        out.append(item)
    return out


def _detect_compiler_root() -> Path | None:
    for root in _candidate_compiler_roots():
        if (root / "linux" / "bin" / "icx").exists() and (root / "linux" / "compiler" / "lib" / "intel64_lin").exists():
            return root
    return None


def _detect_mkl_root() -> Path | None:
    for root in _candidate_mkl_roots():
        if (root / "include").exists() and (root / "lib" / "intel64").exists():
            return root
    return None


def _augment_exact_env(env: dict[str, str]) -> dict[str, str]:
    merged = dict(env)
    compiler_root = _detect_compiler_root()
    mkl_root = _detect_mkl_root()
    if compiler_root is not None:
        _prepend_env_paths(merged, "PATH", [str(compiler_root / "linux" / "bin")])
        _prepend_env_paths(
            merged,
            "LD_LIBRARY_PATH",
            [str(compiler_root / "linux" / "compiler" / "lib" / "intel64_lin")],
        )
    if mkl_root is not None:
        _prepend_env_paths(merged, "LD_LIBRARY_PATH", [str(mkl_root / "lib" / "intel64")])
    merged.setdefault("OCL_ICD_FILENAMES", str(env.get("OCL_ICD_FILENAMES", "")))
    return merged


def _which_in_env(cmd: str, env: dict[str, str]) -> str | None:
    return shutil.which(cmd, path=str(env.get("PATH", "")))


def _safe_float(value: Any) -> float:
    try:
        if value is None:
            return float("nan")
        return float(value)
    except Exception:
        return float("nan")


def _safe_int(value: Any, default: int = -1) -> int:
    try:
        return int(value)
    except Exception:
        return int(default)


def _slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", str(text).lower()).strip("_") or "plot"


def _parse_csv_list(raw: str) -> list[str]:
    return [item.strip().lower() for item in str(raw).split(",") if item.strip()]


def _make_out_dir(backend: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    out = ROOT / "data" / "optimization" / f"{ts}__filip_original__backend-{backend}__exact"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _default_modfem_dir() -> Path:
    if DEFAULT_EXACT_BUNDLE_DIR.exists():
        return DEFAULT_EXACT_BUNDLE_DIR
    return DEFAULT_ORIGINAL_MOD_DIR


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def _require_path(path: Path, description: str) -> Path:
    if not path.exists():
        raise SystemExit(f"Missing {description}: {path}")
    return path


def _kernel_src(mod_dir: Path, variant: str) -> Path:
    name = f"apr_ocl_num_int_el_reference_prism_{variant.upper()}.cl"
    return _require_path(mod_dir / "src" / "OpenCL_kernels" / name, f"kernel source {name}")


def _case_name_from_record(record: dict[str, Any]) -> str:
    case_key = str(record.get("case_key", "")).strip().lower()
    if case_key == "prism6__laplace":
        return "laplace_prism"
    if case_key == "prism6__test":
        return "test_prism"
    return case_key or "unknown_case"


def _option_output_dir(root: Path, *, case_name: str, variant: str, option_index: int) -> Path:
    return root / case_name / variant / f"opt_{int(option_index):03d}"


def _array_preview(arr: np.ndarray, *, max_values: int = 8) -> dict[str, Any]:
    flat = np.ascontiguousarray(arr, dtype=np.float32).reshape(-1)
    if flat.size == 0:
        return {
            "count": 0,
            "bytes": 0,
            "min": 0.0,
            "max": 0.0,
            "mean": 0.0,
            "std": 0.0,
            "l2_norm": 0.0,
            "nonzero_count": 0,
            "first_values": [],
        }
    return {
        "count": int(flat.size),
        "bytes": int(flat.nbytes),
        "min": float(flat.min()),
        "max": float(flat.max()),
        "mean": float(flat.mean()),
        "std": float(flat.std()),
        "l2_norm": float(np.linalg.norm(flat)),
        "nonzero_count": int(np.count_nonzero(flat)),
        "first_values": [float(v) for v in flat[:max_values].tolist()],
    }


def _write_output_preview(
    *,
    output_dir: Path,
    backend: str,
    case_name: str,
    variant: str,
    option_index: int,
    option_row: list[int],
    arr: np.ndarray,
    output_path: Path,
    validation: dict[str, Any] | None = None,
    source_dump_dir: Path | None = None,
    translated_source_path: str = "",
) -> dict[str, Any]:
    preview = {
        "backend": str(backend),
        "case": str(case_name),
        "variant": str(variant),
        "option_index": int(option_index),
        "option_row": list(option_row),
        "combo_bits": "".join("1" if int(v) else "0" for v in option_row),
        "scalar_type": "float32",
        "output_path": str(output_path),
        **_array_preview(arr),
    }
    if validation is not None:
        preview["validation"] = dict(validation)
    if source_dump_dir is not None:
        preview["source_dump_dir"] = str(source_dump_dir)
    if translated_source_path:
        preview["translated_source_path"] = str(translated_source_path)
    preview_path = output_dir / "output_preview.json"
    preview_path.write_text(json.dumps(preview, indent=2, ensure_ascii=True), encoding="utf-8")
    validation_path = ""
    if validation is not None:
        validation_path = str(output_dir / "validation.json")
        Path(validation_path).write_text(json.dumps(validation, indent=2, ensure_ascii=True), encoding="utf-8")
    return {
        "output_dir": str(output_dir),
        "output_path": str(output_path),
        "preview_path": str(preview_path),
        "validation_path": validation_path,
        "count": int(preview["count"]),
    }


def _save_output_artifacts_from_array(
    *,
    output_dir: Path,
    backend: str,
    case_name: str,
    variant: str,
    option_index: int,
    option_row: list[int],
    arr: np.ndarray,
    validation: dict[str, Any] | None = None,
    source_dump_dir: Path | None = None,
    translated_source_path: str = "",
) -> dict[str, Any]:
    output_dir.mkdir(parents=True, exist_ok=True)
    flat = np.ascontiguousarray(arr, dtype=np.float32).reshape(-1)
    output_path = output_dir / "el_data_out.bin"
    output_path.write_bytes(flat.tobytes())
    return _write_output_preview(
        output_dir=output_dir,
        backend=backend,
        case_name=case_name,
        variant=variant,
        option_index=option_index,
        option_row=option_row,
        arr=flat,
        output_path=output_path,
        validation=validation,
        source_dump_dir=source_dump_dir,
        translated_source_path=translated_source_path,
    )


def _save_output_artifacts_from_file(
    *,
    output_dir: Path,
    backend: str,
    case_name: str,
    variant: str,
    option_index: int,
    option_row: list[int],
    output_path: Path,
) -> dict[str, Any]:
    output_dir.mkdir(parents=True, exist_ok=True)
    arr = np.frombuffer(output_path.read_bytes(), dtype=np.float32).copy()
    return _write_output_preview(
        output_dir=output_dir,
        backend=backend,
        case_name=case_name,
        variant=variant,
        option_index=option_index,
        option_row=option_row,
        arr=arr,
        output_path=output_path,
    )


def _detect_device_label(preferred_vendor: str = "intel") -> str:
    clinfo = shutil.which("clinfo")
    if not clinfo:
        return "OpenCL device (clinfo unavailable)"
    try:
        proc = subprocess.run([clinfo], check=False, text=True, capture_output=True)
    except Exception:
        return "OpenCL device"
    if proc.returncode != 0:
        return "OpenCL device"

    device_names: list[str] = []
    for line in proc.stdout.splitlines():
        if "Device Name" in line:
            try:
                device_names.append(line.split("Device Name", 1)[1].split(":", 1)[1].strip())
            except Exception:
                continue
    if not device_names:
        return "OpenCL device"
    preferred = preferred_vendor.strip().lower()
    for name in device_names:
        if preferred and preferred in name.lower():
            return name
    return device_names[0]


def _filip_option_rows() -> list[list[int]]:
    k = 9
    w = [0] * (k + 1)
    step = [1] * (k + 1)
    rows: list[list[int]] = []
    m = 0
    while True:
        rows.append([w[idx] for idx in range(1, k + 1)])
        m += 1
        idx = 1
        mm = m
        while mm % 2 == 0:
            idx += 1
            mm //= 2
        if idx > k:
            break
        w[idx] += step[idx]
        if w[idx] == 0:
            step[idx] = 1
        if w[idx] == 1:
            step[idx] = -1

    filtered: list[list[int]] = []
    for row in rows:
        if (
            not (row[3] and row[4])
            and not (row[3] and row[5])
            and not (row[3] and row[6])
            and not (row[4] and row[5])
            and not (row[4] and row[6])
            and not (row[5] and row[6])
            and not (row[7] and row[8])
            and not ((row[7] == 0) and (row[8] == 0))
        ):
            filtered.append(row)
    return filtered


def _detect_csh_shell() -> str:
    candidates = [
        "/bin/csh",
        shutil.which("csh") or "",
        "/bin/tcsh",
        shutil.which("tcsh") or "",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    raise SystemExit(
        "\n".join(
            [
                "Exact Filip reference build requires a csh-compatible shell.",
                "The original Makefile_explicit uses 'SHELL = /bin/csh'.",
                "Install one of these on Ubuntu before running exact_reference:",
                "  sudo apt-get install -y tcsh",
                "or",
                "  sudo apt-get install -y csh",
            ]
        )
    )


def _load_option_rows(path: Path) -> list[list[int]]:
    rows: list[list[int]] = []
    for line in _read_text(path).splitlines():
        bits = [token.strip() for token in line.strip().split() if token.strip()]
        if not bits:
            continue
        if len(bits) != 9:
            raise SystemExit(f"Unexpected option row width in {path}: '{line}'")
        rows.append([_safe_int(token, 0) for token in bits])
    return rows


def _require_workspace_files(src: Path, case_name: str) -> None:
    required = [
        "options.txt",
        "input_interactive.txt",
        "problem_conv_diff.dat",
    ]
    missing = [name for name in required if not (src / name).exists()]
    if missing:
        joined = ", ".join(missing)
        raise SystemExit(
            "\n".join(
                [
                    f"Incomplete exact workspace for {case_name}: {src}",
                    f"Missing required files: {joined}",
                    "This usually means the Git bundle 'legacy/filip_exact_bundle/mod_2022' was committed in an incomplete state.",
                    "Regenerate it on the source machine with:",
                    "  ./scripts/export_filip_exact_bundle.sh",
                    "then commit and push the whole bundle again with:",
                    "  git add -A legacy/filip_exact_bundle",
                ]
            )
        )


def _require_exact_toolchain(arch: str, mod_dir: Path, env: dict[str, str]) -> None:
    arch_file = mod_dir / "src" / "platform_files" / f"make.{arch}"
    _require_path(arch_file, f"platform file make.{arch}")

    text = _read_text(arch_file)
    problems: list[str] = []

    if re.search(r"^\s*(CC|CPPC|LD|LDPP)\s*=\s*icx\s*$", text, flags=re.MULTILINE):
        if not _which_in_env("icx", env):
            problems.append("missing compiler 'icx' in PATH")

    required_paths = [
        "/opt/intel/oneapi/mkl/latest/include",
        "/opt/intel/oneapi/mkl/latest/lib/intel64",
        "/opt/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin",
    ]
    for candidate in required_paths:
        if candidate in text and not Path(candidate).exists():
            problems.append(f"missing oneAPI path: {candidate}")

    if "-lOpenCL" in text and not Path("/usr/lib/x86_64-linux-gnu/libOpenCL.so").exists():
        alt = [Path("/usr/lib64/libOpenCL.so"), Path("/usr/lib/libOpenCL.so")]
        if not any(path.exists() for path in alt):
            problems.append("missing OpenCL loader library (libOpenCL.so)")

    if problems:
        detail = "\n".join(f"  - {item}" for item in problems)
        raise SystemExit(
            "\n".join(
                [
                    f"Exact Filip toolchain preflight failed for arch '{arch}'.",
                    f"Platform file: {arch_file}",
                    detail,
                    "",
                    "This exact mode uses Filip's original oneAPI-based build profile.",
                    "Install Intel oneAPI Base Toolkit so that 'icx' and MKL paths exist,",
                    "or provide a compatible custom make.<arch> profile and pass --arch-laplace/--arch-test.",
                ]
            )
        )


def _default_field_std_candidate(mod_dir: Path, case: ExactCaseSpec) -> Path | None:
    preferred: list[Path] = []
    if case.case_name == "laplace_prism":
        preferred.append(mod_dir / "examples" / "pdd_conv_diff" / "diff_in_box" / "prism" / "std" / "arch" / "field_std.dmp")
    if case.case_name == "test_prism":
        preferred.append(mod_dir / "examples" / "pdd_conv_diff" / "test_num_int" / "prism" / "std" / "arch" / "field_std.dmp")
    preferred.append(mod_dir / "examples" / "pdd_conv_diff" / "diff_in_box" / "prism" / "std" / "arch" / "field_std.dmp")
    preferred.append(mod_dir / "examples" / "pdd_conv_diff" / "wave_in_box_dg" / "arch" / "field_std.dmp")
    for candidate in preferred:
        if candidate.exists():
            return candidate
    return None


def _prepare_workspace_runtime_files(dst: Path, *, mod_dir: Path, case: ExactCaseSpec) -> None:
    mesh_zip = dst / "mesh_prism.dmp.zip"
    mesh_plain = dst / "mesh_prism.dmp"
    if not mesh_plain.exists() and mesh_zip.exists():
        with zipfile.ZipFile(mesh_zip, "r") as zf:
            zf.extractall(dst)

    field_plain = dst / "field_std.dmp"
    if not field_plain.exists():
        candidate = _default_field_std_candidate(mod_dir, case)
        if candidate is not None:
            shutil.copy2(candidate, field_plain)


def _copy_case_workspace(*, mod_dir: Path, case: ExactCaseSpec, out_dir: Path, input_override: Path | None, limit_option_rows: int) -> tuple[Path, list[list[int]]]:
    src = mod_dir / case.work_subdir
    if not src.exists():
        raise SystemExit(
            "\n".join(
                [
                    f"Missing workspace for {case.case_name}: {src}",
                    "Exact Filip reference mode needs a real local copy of 'Kod Filipa/mod_2022'.",
                    "Most likely the second machine has only the outer repo clone, without the actual mod_2022 contents.",
                    "Fix one of these:",
                    "  1. copy/export mod_2022 to that machine and pass --modfem-dir /path/to/mod_2022",
                    "  2. prepare a slim bundle on the source machine with scripts/export_filip_exact_bundle.sh",
                ]
            )
        )
    _require_workspace_files(src, case.case_name)
    dst = out_dir / "exact_work" / case.case_name
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    _prepare_workspace_runtime_files(dst, mod_dir=mod_dir, case=case)

    if input_override is not None:
        shutil.copy2(input_override, dst / "input_interactive.txt")

    options_path = dst / "options.txt"
    option_rows = _load_option_rows(options_path)
    if limit_option_rows > 0:
        option_rows = option_rows[:limit_option_rows]
        with options_path.open("w", encoding="utf-8") as f:
            for row in option_rows:
                f.write("\t".join(str(int(v)) for v in row) + "\n")
    return dst, option_rows


def _build_case(*, mod_dir: Path, arch: str, rebuild: bool, log_dir: Path) -> Path:
    env = _augment_exact_env(os.environ.copy())
    env["MOD_FEM_DIR"] = str(mod_dir)
    env["MOD_FEM_ARCH"] = arch
    src_dir = mod_dir / "src"
    build_log = log_dir / f"build__{arch}.log"
    binary = mod_dir / "bin" / arch / "MFEM_conv_diff_prism_std_krb_ocl"
    csh_shell = _detect_csh_shell()
    if not rebuild:
        return _require_path(binary, f"existing OpenCL binary for arch {arch}")
    _require_exact_toolchain(arch, mod_dir, env)
    shutil.rmtree(mod_dir / "obj" / arch, ignore_errors=True)
    shutil.rmtree(mod_dir / "bin" / arch, ignore_errors=True)
    cmds: list[list[str]] = [
        ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit", "config"],
        ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit", "conv_diff_prism_std_krb_ocl"],
    ]
    with build_log.open("w", encoding="utf-8") as log:
        for cmd in cmds:
            log.write(f"$ {' '.join(cmd)}\n")
            log.flush()
            proc = subprocess.run(
                cmd,
                cwd=src_dir,
                env=env,
                text=True,
                capture_output=True,
                check=False,
            )
            if proc.stdout:
                log.write(proc.stdout)
            if proc.stderr:
                log.write(proc.stderr)
            log.write(f"[exit={proc.returncode}]\n\n")
            log.flush()
            if proc.returncode != 0:
                raise SystemExit(
                    f"Build failed for arch '{arch}'. Check log: {build_log}"
                )
    return _require_path(binary, f"built OpenCL binary for arch {arch}")


def _split_csv_row(raw_line: str) -> list[str]:
    parsed = next(csv.reader([raw_line], skipinitialspace=True))
    while parsed and parsed[-1] == "":
        parsed.pop()
    return [cell.strip() for cell in parsed]


def _result_line_to_payload(*, case: ExactCaseSpec, variant: str, option_index: int, option_row: list[int], raw_line: str) -> dict[str, Any]:
    cells = _split_csv_row(raw_line)
    if len(cells) < len(RESULT_FIELD_ORDER):
        raise ValueError(
            f"Unexpected result row width for {case.case_name}/{variant}/{option_index}: {len(cells)}"
        )

    data = {RESULT_FIELD_ORDER[idx]: cells[idx] for idx in range(len(RESULT_FIELD_ORDER))}
    n_elements = max(1.0, _safe_float(data["nr_elems"]))
    kernel_s = _safe_float(data["executing kernel"])
    internal_s = _safe_float(data["internal"])
    input_s = _safe_float(data["sending el_data_in to GPU memory"])
    output_s = _safe_float(data["copying output buffer"])
    workgroup_size = max(1, _safe_int(data["work_group_size"], 64))
    option_cfg = {
        "coal_read": int(option_row[0]),
        "coal_write": int(option_row[1]),
        "compute_all_shape_fun_der": int(option_row[2]),
        "use_workspace_for_pde_coeff": int(option_row[3]),
        "use_workspace_for_geo_data": int(option_row[4]),
        "use_workspace_for_shape_fun": int(option_row[5]),
        "use_workspace_for_stiff_mat": int(option_row[6]),
        "padding": 1 if int(option_row[8]) == 1 else 0,
    }
    ns_per_unit = kernel_s * 1e9 / max(1.0, n_elements * case.n_qp) if math.isfinite(kernel_s) else float("nan")
    internal_ns_per_elem = internal_s * 1e9 / n_elements if math.isfinite(internal_s) else float("nan")
    return {
        "case_key": f"{case.element_type}__{case.operator}",
        "config": {
            "operator": case.operator,
            "element_type": case.element_type,
            "algorithm_variant": variant,
            "n_elements": int(round(n_elements)),
            "n_qp": case.n_qp,
            "workgroup_size": workgroup_size,
            **option_cfg,
        },
        "metrics": {
            "elapsed_s_mean": kernel_s,
            "n_elements": n_elements,
            "n_qp_effective": case.n_qp,
            "gflops_mean": float("nan"),
            "gbps_mean": float("nan"),
            "energy_j_mean": float("nan"),
            "j_per_gflop": float("nan"),
        },
        "raw_phase": {
            "el_data_in_mb": _safe_float(data["el_data_in [MB]"]),
            "el_data_out_mb": _safe_float(data["el_data_out [MB]"]),
            "nr_elems_per_work_group": _safe_int(data["nr_elems_per_work_group"], workgroup_size),
            "nr_elems_per_thread": _safe_int(data["nr_elems_per_thread"], 1),
            "nr_work_groups": _safe_int(data["nr_work_groups"], 0),
            "nr_threads": _safe_int(data["nr_threads"], 0),
            "input_time_s": input_s,
            "input_bw_gbps": _safe_float(data["input_bandwidth_[GB/s]"]),
            "kernel_time_s": kernel_s,
            "internal_time_s": internal_s,
            "output_time_s": output_s,
            "output_bw_gbps": _safe_float(data["output_bandwidth_[GB/s]"]),
            "internal_ns_per_elem": internal_ns_per_elem,
            "kernel_ns_per_unit": ns_per_unit,
        },
        "variant": variant,
        "option_index": option_index,
        "option_row": list(option_row),
        "combo_bits": "".join("1" if int(v) else "0" for v in option_row),
        "status": "ok" if math.isfinite(kernel_s) and math.isfinite(internal_s) else "error",
        "constraints_ok": True,
        "ns_per_unit": ns_per_unit,
    }


def _portable_mode_bucket(problem: FemParametricProblem) -> str:
    if problem.mode.resolved_backend == "cpu":
        return "cpu"
    if problem.mode.execution_mode == "native":
        return "native_gpu"
    return "mapped_gpu"


def _portable_suggest_workgroup(problem: FemParametricProblem, requested: int) -> int:
    supported = list(problem.mode.profile.supported_workgroup_sizes)
    if problem.mode.resolved_backend == "cpu":
        return 1
    if requested > 0:
        if requested in supported:
            return requested
        if supported:
            return min(supported, key=lambda value: abs(value - requested))
        return requested
    for preferred in (64, 32, 128, 256):
        if preferred in supported:
            return preferred
    if supported:
        return supported[0]
    return 64


def _portable_auto_n_elements(
    *,
    problem: FemParametricProblem,
    profile: str,
    element_type: str,
    operator: str,
    dtype: str,
    n_qp: int,
    workgroup_size: int,
) -> int:
    bucket = _portable_mode_bucket(problem)
    scale = PROFILE_SCALE.get(profile, 1.0)
    desired = int(BASE_ELEMENTS[bucket][element_type] * scale)
    if dtype == "float64":
        desired = max(1, desired // 2)

    probe_budget = int(problem.mode.profile.memory_budget_bytes * (0.72 if bucket == "cpu" else 0.58))
    probe_budget = max(64 * 1024 * 1024, probe_budget)

    def estimate(n_elements: int) -> int:
        cfg = {
            "n_elements": int(n_elements),
            "n_qp": int(n_qp),
            "element_type": str(element_type),
            "operator": str(operator),
            "dtype": str(dtype),
            "algorithm_variant": "ssq",
            "workgroup_size": int(workgroup_size),
            "use_workspace_for_pde_coeff": 1,
            "use_workspace_for_geo_data": 1,
            "use_workspace_for_shape_fun": 1,
            "use_workspace_for_stiff_mat": 1,
            "padding": 1,
            "compute_all_shape_fun_der": 1,
            "coal_read": 1,
            "coal_write": 1,
        }
        return int(problem._estimate_candidate_memory_bytes(cfg))

    low = 1
    high = max(1, desired)
    if estimate(high) <= probe_budget:
        return high

    best = low
    while low <= high:
        mid = (low + high) // 2
        if estimate(mid) <= probe_budget:
            best = mid
            low = mid + 1
        else:
            high = mid - 1
    floor = 2_000 if bucket == "cpu" else 8_000
    return max(floor, best)


def _portable_plot_row(record: dict[str, Any], backend: str, device: str) -> dict[str, Any]:
    return {
        "backend": backend,
        "device": device,
        "config": dict(record["config"]),
        "metrics": dict(record["metrics"]),
        "status": str(record["status"]),
        "constraints_ok": bool(record["constraints_ok"]),
        "operator": str(record["config"].get("operator", "")),
        "variant": str(record["variant"]),
    }


def _portable_phase_metrics(record: dict[str, Any], peak_gflops: float, peak_bw: float) -> dict[str, float]:
    elapsed = _safe_float(record["metrics"].get("elapsed_s_mean"))
    row = _portable_plot_row(record, backend=str(record["backend"]), device=str(record["device"]))
    read_share, compute_share, write_share = _estimate_share(row, peak_gflops, peak_bw)
    read_t = elapsed * max(0.0, read_share) / 100.0
    compute_t = elapsed * max(0.0, compute_share) / 100.0
    write_t = elapsed * max(0.0, write_share) / 100.0
    read_bytes, write_bytes = _estimate_rw_bytes(row)
    read_bw = read_bytes / max(read_t, 1e-12) / 1e9 if read_t > 0.0 else float("inf")
    write_bw = write_bytes / max(write_t, 1e-12) / 1e9 if write_t > 0.0 else float("inf")
    return {
        "el_data_in_mb": read_bytes / 1e6,
        "el_data_out_mb": write_bytes / 1e6,
        "input_time_s": read_t,
        "input_bw_gbps": read_bw,
        "kernel_time_s": elapsed,
        "internal_time_s": compute_t,
        "output_time_s": write_t,
        "output_bw_gbps": write_bw,
    }


def _portable_record_to_payload(
    *,
    case: ExactCaseSpec,
    backend: str,
    device_label: str,
    record: dict[str, Any],
    peak_gflops: float,
    peak_bw: float,
) -> dict[str, Any]:
    cfg = dict(record["config"])
    metrics = dict(record["metrics"])
    phase = _portable_phase_metrics(record, peak_gflops, peak_bw)
    n_elements = max(1.0, _safe_float(metrics.get("n_elements")))
    n_qp = max(1.0, _safe_float(metrics.get("n_qp_effective")) or float(case.n_qp))
    wg = max(1, _safe_int(cfg.get("workgroup_size"), 64))
    n_work_groups = max(1, int(math.ceil(n_elements / max(float(wg), 1.0))))
    n_threads = n_work_groups * wg
    ns_per_unit = _safe_float(record["ns_per_unit"])
    internal_time_s = _safe_float(phase["internal_time_s"])
    internal_ns_per_elem = internal_time_s * 1e9 / n_elements if math.isfinite(internal_time_s) else float("nan")
    return {
        "case_key": record["case_key"],
        "config": {
            "operator": cfg["operator"],
            "element_type": cfg["element_type"],
            "algorithm_variant": record["variant"],
            "n_elements": int(round(n_elements)),
            "n_qp": int(round(n_qp)),
            "workgroup_size": wg,
            "coal_read": int(cfg["coal_read"]),
            "coal_write": int(cfg["coal_write"]),
            "compute_all_shape_fun_der": int(cfg["compute_all_shape_fun_der"]),
            "use_workspace_for_pde_coeff": int(cfg["use_workspace_for_pde_coeff"]),
            "use_workspace_for_geo_data": int(cfg["use_workspace_for_geo_data"]),
            "use_workspace_for_shape_fun": int(cfg["use_workspace_for_shape_fun"]),
            "use_workspace_for_stiff_mat": int(cfg["use_workspace_for_stiff_mat"]),
            "padding": int(cfg["padding"]),
        },
        "metrics": metrics,
        "raw_phase": {
            "el_data_in_mb": _safe_float(phase["el_data_in_mb"]),
            "el_data_out_mb": _safe_float(phase["el_data_out_mb"]),
            "nr_elems_per_work_group": wg,
            "nr_elems": int(round(n_elements)),
            "nr_elems_per_thread": 1,
            "nr_work_groups": n_work_groups,
            "work_group_size": wg,
            "nr_threads": n_threads,
            "input_time_s": _safe_float(phase["input_time_s"]),
            "input_bw_gbps": _safe_float(phase["input_bw_gbps"]),
            "kernel_time_s": _safe_float(phase["kernel_time_s"]),
            "internal_time_s": internal_time_s,
            "output_time_s": _safe_float(phase["output_time_s"]),
            "output_bw_gbps": _safe_float(phase["output_bw_gbps"]),
            "internal_ns_per_elem": internal_ns_per_elem,
            "kernel_ns_per_unit": ns_per_unit,
        },
        "variant": record["variant"],
        "option_index": int(record["option_index"]),
        "option_row": list(record["option_row"]),
        "combo_bits": "".join("1" if int(v) else "0" for v in record["option_row"]),
        "status": str(record["status"]),
        "constraints_ok": bool(record["constraints_ok"]),
        "ns_per_unit": ns_per_unit,
        "backend": backend,
        "device": device_label,
    }


def _append_jsonl(path: Path, payload: dict[str, Any]) -> None:
    with path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(payload, ensure_ascii=True) + "\n")


def _write_current_combined_csv(path: Path, records: list[dict[str, Any]], *, backend: str, device_label: str) -> None:
    header = [
        "backend",
        "device",
        "case_key",
        "variant",
        "option_index",
        *OPTION_HEADERS,
        "el_data_in [MB]",
        "el_data_out [MB]",
        "nr_elems_per_work_group",
        "nr_elems",
        "nr_elems_per_thread",
        "nr_work_groups",
        "work_group_size",
        "nr_threads",
        "sending el_data_in to GPU memory",
        "input_bandwidth_gbps",
        "executing kernel",
        "internal",
        "copying output buffer",
        "output_bandwidth_gbps",
        "status",
        "constraints_ok",
        "kernel_ns_per_unit",
        "internal_ns_per_elem",
    ]
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        for record in records:
            cfg = record["config"]
            phase = record["raw_phase"]
            writer.writerow(
                [
                    backend,
                    device_label,
                    record["case_key"],
                    record["variant"],
                    record["option_index"],
                    *record["option_row"],
                    phase["el_data_in_mb"],
                    phase["el_data_out_mb"],
                    phase["nr_elems_per_work_group"],
                    cfg["n_elements"],
                    phase["nr_elems_per_thread"],
                    phase["nr_work_groups"],
                    cfg["workgroup_size"],
                    phase["nr_threads"],
                    phase["input_time_s"],
                    phase["input_bw_gbps"],
                    phase["kernel_time_s"],
                    phase["internal_time_s"],
                    phase["output_time_s"],
                    phase["output_bw_gbps"],
                    record["status"],
                    int(bool(record["constraints_ok"])),
                    record["ns_per_unit"],
                    phase["internal_ns_per_elem"],
                ]
            )


def _write_exact_case_csvs(out_dir: Path, records: list[dict[str, Any]], *, backend: str) -> list[str]:
    csv_dir = out_dir / "csv"
    csv_dir.mkdir(parents=True, exist_ok=True)
    generated: list[str] = []
    grouped: dict[tuple[str, str], list[dict[str, Any]]] = {}
    for record in records:
        case_name = "laplace_prism" if record["case_key"] == "prism6__laplace" else "test_prism"
        key = (case_name, str(record["variant"]))
        grouped.setdefault(key, []).append(record)

    for (case_name, variant), group in grouped.items():
        ordered = sorted(group, key=lambda row: int(row["option_index"]))
        path = csv_dir / f"result__{case_name}__{variant.upper()}__{backend}.csv"
        with path.open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(RESULT_FIELD_ORDER)
            for record in ordered:
                phase = record["raw_phase"]
                writer.writerow(
                    [
                        *record["option_row"],
                        phase["el_data_in_mb"],
                        phase["el_data_out_mb"],
                        phase["nr_elems_per_work_group"],
                        phase["nr_elems"],
                        phase["nr_elems_per_thread"],
                        phase["nr_work_groups"],
                        phase["work_group_size"],
                        phase["nr_threads"],
                        phase["input_time_s"],
                        phase["input_bw_gbps"],
                        phase["kernel_time_s"],
                        phase["internal_time_s"],
                        phase["output_time_s"],
                        phase["output_bw_gbps"],
                    ]
                )
        generated.append(str(path))
    return generated


def _plot_setup() -> None:
    plt.style.use("default")
    plt.rcParams.update(
        {
            "figure.facecolor": "white",
            "axes.facecolor": "white",
            "axes.grid": True,
            "grid.alpha": 0.25,
            "font.family": "serif",
            "axes.titlesize": 11,
            "axes.labelsize": 10,
            "legend.fontsize": 8,
        }
    )


def _plot_axis_style(ax: Any) -> None:
    ax.set_facecolor("#ffffff")
    ax.grid(True, axis="y", color="#d4d4d4", linewidth=0.5, alpha=0.9)
    ax.grid(True, axis="x", color="#ececec", linewidth=0.35, alpha=0.8)
    for spine in ax.spines.values():
        spine.set_color("#8f8f8f")
        spine.set_linewidth(0.8)
    ax.tick_params(axis="y", labelsize=8, colors="#222222")
    ax.tick_params(axis="x", length=0, pad=3, colors="#333333")


def _stacked_combo_label(bits: str) -> str:
    return "\n".join(list(bits))


def _combo_fontsize(combos: list[str]) -> float:
    n_points = len(combos)
    if n_points >= 72:
        return 3.8
    if n_points >= 48:
        return 4.2
    if n_points >= 24:
        return 4.8
    return 5.4


def _set_combo_ticklabels(ax: Any, combos: list[str]) -> None:
    xs = list(range(len(combos)))
    ax.set_xticks(xs, [_stacked_combo_label(combo) for combo in combos], fontsize=_combo_fontsize(combos))
    for lbl in ax.get_xticklabels():
        lbl.set_fontfamily("monospace")
        lbl.set_color("#333333")


def _article_ylim(values: Iterable[float]) -> tuple[float, float] | None:
    finite_vals = sorted(float(v) for v in values if math.isfinite(float(v)))
    if not finite_vals:
        return None
    lo = finite_vals[0]
    hi = finite_vals[-1]
    span = max(hi - lo, max(abs(lo), abs(hi), 1.0) * 0.08, 1e-6)
    pad = span * 0.08
    lower = max(0.0, lo - pad)
    upper = hi + pad
    if lower >= upper:
        upper = lower + 1.0
    return lower, upper


def _plot_exact_results(records: list[dict[str, Any]], out_dir: Path) -> list[str]:
    plots_dir = out_dir / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)
    _plot_setup()

    case_order: list[str] = []
    seen_cases: set[str] = set()
    for record in records:
        case_key = str(record["case_key"])
        if case_key not in seen_cases:
            case_order.append(case_key)
            seen_cases.add(case_key)

    panels = [(case_key, variant) for case_key in case_order for variant in VARIANT_ORDER]
    panels = [panel for panel in panels if any(r["case_key"] == panel[0] and r["variant"] == panel[1] for r in records)]
    if not panels:
        return []

    all_vals = [float(r["raw_phase"]["internal_ns_per_elem"]) for r in records if math.isfinite(float(r["raw_phase"]["internal_ns_per_elem"]))]
    y_bounds = _article_ylim(all_vals)

    fig, axes = plt.subplots(len(panels), 1, figsize=(24, 4.6 * len(panels) + 1.0), squeeze=False)
    for idx, (case_key, variant) in enumerate(panels):
        subset = [r for r in records if r["case_key"] == case_key and r["variant"] == variant]
        subset.sort(key=lambda row: int(row["option_index"]))
        combos = [str(r["combo_bits"]) for r in subset]
        ys = [float(r["raw_phase"]["internal_ns_per_elem"]) for r in subset]
        xs = list(range(len(subset)))
        ax = axes[idx][0]
        ax.plot(xs, ys, linewidth=1.6, color=PLOT_COLOR.get(variant, "#111111"))
        _plot_axis_style(ax)
        _set_combo_ticklabels(ax, combos)
        ax.set_xlim(-0.5, len(subset) - 0.5)
        if y_bounds is not None:
            ax.set_ylim(*y_bounds)
        ax.set_title(f"{variant.upper()} | {case_key.replace('__', ' | ')}", fontsize=10, pad=3)
        ax.set_ylabel("Time [ns]", fontsize=9)
        ax.set_xlabel("Options")
    fig.subplots_adjust(left=0.05, right=0.995, top=0.97, bottom=0.04, hspace=0.42)
    main_plot = plots_dir / "article_paper_option_times.png"
    fig.savefig(main_plot, dpi=220)
    plt.close(fig)
    return [str(main_plot)]


def _score(record: dict[str, Any]) -> float:
    internal = _safe_float(record["raw_phase"].get("internal_ns_per_elem"))
    if math.isfinite(internal):
        return internal
    return _safe_float(record.get("ns_per_unit"))


def _run_binary_once(*, binary: Path, work_dir: Path, env: dict[str, str], log_path: Path) -> None:
    proc = subprocess.run(
        [str(binary), "."],
        cwd=work_dir,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    with log_path.open("a", encoding="utf-8") as log:
        if proc.stdout:
            log.write(proc.stdout)
        if proc.stderr:
            log.write(proc.stderr)
        log.write(f"[exit={proc.returncode}]\n")
    if proc.returncode != 0:
        raise SystemExit(f"Exact reference run failed. Check log: {log_path}")


def _read_last_result_line(result_csv: Path) -> str:
    lines = [line.strip() for line in _read_text(result_csv).splitlines() if line.strip()]
    if not lines:
        raise SystemExit(f"No result rows in {result_csv}")
    return lines[-1]


def _write_raw_csv(*, header_path: Path, result_path: Path, out_path: Path) -> None:
    header = _read_text(header_path).strip()
    result = _read_text(result_path).strip()
    lines = [line for line in [header, result] if line]
    out_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def _best_overall_payload(records: list[dict[str, Any]]) -> dict[str, Any] | None:
    scored = [record for record in records if math.isfinite(_score(record))]
    if not scored:
        return None
    best = min(scored, key=_score)
    return {
        "case_key": best["case_key"],
        "variant": best["variant"],
        "option_index": int(best["option_index"]),
        "option_row": list(best["option_row"]),
        "combo_bits": str(best["combo_bits"]),
        "score_internal_ns_per_elem": _safe_float(best["raw_phase"].get("internal_ns_per_elem")),
        "score_kernel_ns_per_unit": _safe_float(best.get("ns_per_unit")),
        "config": dict(best["config"]),
        "metrics": dict(best["metrics"]),
    }


def _validation_summary(records: list[dict[str, Any]]) -> dict[str, Any]:
    validations = [
        dict(record.get("raw_phase", {}).get("validation", {}))
        for record in records
        if isinstance(record.get("raw_phase", {}).get("validation"), dict)
    ]
    if not validations:
        return {}
    checked = [v for v in validations if v.get("within_tolerance") is not None]
    max_abs_vals = [float(v["max_abs_diff"]) for v in checked if v.get("max_abs_diff") is not None]
    rms_vals = [float(v["rms_diff"]) for v in checked if v.get("rms_diff") is not None]
    return {
        "records_with_validation": len(validations),
        "records_checked": len(checked),
        "records_with_expected_output": sum(1 for v in validations if bool(v.get("expected_output_present"))),
        "records_within_tolerance": sum(1 for v in checked if bool(v.get("within_tolerance"))),
        "records_out_of_tolerance": sum(1 for v in checked if v.get("within_tolerance") is False),
        "worst_max_abs_diff": max(max_abs_vals) if max_abs_vals else None,
        "worst_rms_diff": max(rms_vals) if rms_vals else None,
    }


def _exact_numerical_output_summary(
    *,
    records: list[dict[str, Any]],
    root: Path | None,
    best_overall: dict[str, Any] | None,
    note: str,
) -> dict[str, Any]:
    payload: dict[str, Any] = {
        "available": bool(root is not None),
        "note": str(note),
        "root": str(root) if root is not None else "",
        "records_with_output": 0,
    }
    output_rows = [record for record in records if isinstance(record.get("numerical_output"), dict)]
    payload["records_with_output"] = len(output_rows)
    if root is None or not output_rows:
        return payload
    payload["example_output_dir"] = str(output_rows[0]["numerical_output"].get("output_dir", ""))
    payload["example_output_preview"] = str(output_rows[0]["numerical_output"].get("preview_path", ""))
    if best_overall is None:
        return payload
    best_case_key = str(best_overall.get("case_key", ""))
    best_variant = str(best_overall.get("variant", ""))
    best_option_index = int(best_overall.get("option_index", -1))
    for record in output_rows:
        if (
            str(record.get("case_key", "")) == best_case_key
            and str(record.get("variant", "")) == best_variant
            and int(record.get("option_index", -1)) == best_option_index
        ):
            payload["best_output_dir"] = str(record["numerical_output"].get("output_dir", ""))
            payload["best_output_path"] = str(record["numerical_output"].get("output_path", ""))
            payload["best_output_preview"] = str(record["numerical_output"].get("preview_path", ""))
            payload["best_output_validation_path"] = str(record["numerical_output"].get("validation_path", ""))
            break
    return payload


def _run_case(
    *,
    mod_dir: Path,
    case: ExactCaseSpec,
    work_dir: Path,
    option_rows: list[list[int]],
    variants: list[str],
    binary: Path,
    out_dir: Path,
    eval_path: Path,
    iter_path: Path,
    global_eval_start: int,
    total_evals: int,
    device_label: str,
    numerical_outputs_root: Path,
    dump_launch_root: Path | None,
) -> tuple[list[dict[str, Any]], int]:
    env = _augment_exact_env(os.environ.copy())
    env["MOD_FEM_DIR"] = str(mod_dir)
    env["MOD_FEM_ARCH"] = case.arch

    raw_dir = out_dir / "raw" / case.case_name
    raw_dir.mkdir(parents=True, exist_ok=True)
    csv_dir = out_dir / "csv"
    csv_dir.mkdir(parents=True, exist_ok=True)

    case_records: list[dict[str, Any]] = []
    best_so_far = float("inf")
    eval_counter = global_eval_start

    for variant in variants:
        kernel_path = _kernel_src(mod_dir, variant)
        shutil.copy2(kernel_path, work_dir / "tmr_ocl_num_int_el.cl")

        header_path = work_dir / "header.csv"
        result_path = work_dir / "result.csv"
        if header_path.exists():
            header_path.unlink()
        if result_path.exists():
            result_path.unlink()

        variant_log = raw_dir / f"{case.case_name}__{variant}.log"
        variant_log.write_text(
            f"# Exact Filip reference run\n# case={case.case_name}\n# variant={variant}\n# binary={binary}\n# work_dir={work_dir}\n",
            encoding="utf-8",
        )

        for option_index, option_row in enumerate(option_rows):
            eval_counter += 1
            print(
                f"[exact] {eval_counter}/{total_evals} case={case.case_name} variant={variant} option={option_index + 1}/{len(option_rows)}",
                flush=True,
            )
            run_env = dict(env)
            dump_dir: Path | None = None
            result_dir = _option_output_dir(
                dump_launch_root if dump_launch_root is not None else numerical_outputs_root,
                case_name=case.case_name,
                variant=variant,
                option_index=option_index,
            )
            if result_dir.exists() and dump_launch_root is None:
                shutil.rmtree(result_dir)
            result_dir.mkdir(parents=True, exist_ok=True)
            run_env["FILIP_EXACT_RESULT_DIR"] = str(result_dir)
            run_env["FILIP_EXACT_CASE"] = case.case_name
            run_env["FILIP_EXACT_VARIANT"] = variant
            run_env["FILIP_EXACT_OPTION_INDEX"] = str(option_index)
            if dump_launch_root is not None:
                dump_dir = dump_launch_root / case.case_name / variant / f"opt_{option_index:03d}"
                if dump_dir.exists():
                    shutil.rmtree(dump_dir)
                dump_dir.mkdir(parents=True, exist_ok=True)
                run_env["FILIP_EXACT_DUMP_DIR"] = str(dump_dir)
                (dump_dir / "requested_option.json").write_text(
                    json.dumps(
                        {
                            "case": case.case_name,
                            "variant": variant,
                            "option_index": option_index,
                            "option_row": list(option_row),
                            "combo_bits": "".join("1" if int(v) else "0" for v in option_row),
                        },
                        indent=2,
                        ensure_ascii=True,
                    ),
                    encoding="utf-8",
                )
            _run_binary_once(binary=binary, work_dir=work_dir, env=run_env, log_path=variant_log)
            result_output_path = result_dir / "el_data_out.bin"
            if not result_output_path.exists():
                raise SystemExit(
                    "\n".join(
                        [
                            f"Exact reference run finished but numerical output is missing for {case.case_name}/{variant}/opt_{option_index:03d}.",
                            f"Expected output file: {result_output_path}",
                            "Rebuild mod_2022 from the updated sources and rerun.",
                        ]
                    )
                )
            if dump_dir is not None:
                required_dump_files = [
                    dump_dir / "launch_meta.json",
                    dump_dir / "execution_parameters.bin",
                    dump_dir / "gauss_dat.bin",
                    dump_dir / "shape_fun_ref.bin",
                    dump_dir / "el_data_in.bin",
                    dump_dir / "el_data_out.bin",
                ]
                missing_dump_files = [str(path.name) for path in required_dump_files if not path.exists()]
                if missing_dump_files:
                    raise SystemExit(
                        "\n".join(
                            [
                                f"OpenCL exact run finished but launch dump is incomplete for {case.case_name}/{variant}/opt_{option_index:03d}.",
                                f"Dump dir: {dump_dir}",
                                f"Missing files: {', '.join(missing_dump_files)}",
                                "Rebuild mod_2022 from the updated sources and rerun with --dump-launch-artifacts.",
                            ]
                        )
                    )
            if not header_path.exists() or not result_path.exists():
                raise SystemExit(f"Expected files missing after run: {header_path} / {result_path}")
            record = _result_line_to_payload(
                case=case,
                variant=variant,
                option_index=option_index,
                option_row=option_row,
                raw_line=_read_last_result_line(result_path),
            )
            record["numerical_output"] = _save_output_artifacts_from_file(
                output_dir=result_dir,
                backend="opencl",
                case_name=case.case_name,
                variant=variant,
                option_index=option_index,
                option_row=option_row,
                output_path=result_output_path,
            )
            case_records.append(record)
            _append_jsonl(eval_path, record)

            score = _score(record)
            if math.isfinite(score) and score < best_so_far:
                best_so_far = score
            _append_jsonl(
                iter_path,
                {
                    "iteration": eval_counter,
                    "workflow": "filip_original_exact",
                    "case_key": record["case_key"],
                    "variant": record["variant"],
                    "option_index": record["option_index"],
                    "best_score_internal_ns_per_elem": best_so_far if math.isfinite(best_so_far) else None,
                },
            )

        raw_csv_path = csv_dir / f"result__{case.case_name}__{variant.upper()}__opencl.csv"
        _write_raw_csv(header_path=header_path, result_path=result_path, out_path=raw_csv_path)

    return case_records, eval_counter


def _run_metal_exact_replay(
    *,
    mod_dir: Path,
    benchmark_case: str,
    case_specs: list[ExactCaseSpec],
    variants: list[str],
    out_dir: Path,
    eval_path: Path,
    iter_path: Path,
    replay_dump_root: Path,
    numerical_outputs_root: Path,
    device_index: int,
    requested_device_index: int,
    repeats: int,
    limit_option_rows: int,
    verify_output_tol: float,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    from gpu.metal.filip_exact_replay import FilipMetalReplayRunner, load_replay_dump, option_dump_dir, resolve_dump_root

    _require_path(mod_dir, "Filip mod_2022 directory with OpenCL kernel sources")
    dump_root = resolve_dump_root(replay_dump_root)
    _require_path(dump_root, "OpenCL replay dump root")

    runner = FilipMetalReplayRunner(device_index=device_index)
    device_label = runner.device_name
    option_rows = _filip_option_rows()
    if int(limit_option_rows) > 0:
        option_rows = option_rows[: int(limit_option_rows)]
    total_evals = len(case_specs) * len(variants) * len(option_rows)

    print("=== FILIP ORIGINAL EXACT REPLAY (METAL) ===")
    print(f"benchmark case     : {benchmark_case}")
    print("backend            : metal")
    print(f"device label       : {device_label}")
    print(f"replay dump root   : {dump_root}")
    print(f"repeats            : {max(1, int(repeats))}")
    print(f"total evaluations  : {total_evals}")
    print(f"out dir            : {out_dir}")

    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    translated_dir = out_dir / "translated_metal"
    translated_dir.mkdir(parents=True, exist_ok=True)

    records: list[dict[str, Any]] = []
    best_so_far = float("inf")
    complete = 0

    for case in case_specs:
        case_log_dir = raw_dir / case.case_name
        case_log_dir.mkdir(parents=True, exist_ok=True)
        for variant in variants:
            variant_log = case_log_dir / f"{case.case_name}__{variant}.log"
            variant_log.write_text(
                "\n".join(
                    [
                        "# Filip exact replay Metal run",
                        f"# case={case.case_name}",
                        f"# variant={variant}",
                        f"# backend=metal",
                        f"# device={device_label}",
                        f"# replay_dump_root={dump_root}",
                        "",
                    ]
                ),
                encoding="utf-8",
            )
            kernel_path = _kernel_src(mod_dir, variant)
            for option_index, option_row in enumerate(option_rows):
                complete += 1
                print(
                    f"[metal replay] {complete}/{total_evals} case={case.case_name} variant={variant} option={option_index + 1}/{len(option_rows)}",
                    flush=True,
                )
                dump_dir = option_dump_dir(dump_root, case.case_name, variant, option_index)
                if not dump_dir.exists():
                    raise SystemExit(
                        "\n".join(
                            [
                                f"Missing OpenCL replay dump for {case.case_name}/{variant}/opt_{option_index:03d}.",
                                f"Expected dump dir: {dump_dir}",
                                "Generate dumps first on Linux/OpenCL with:",
                                "  python run_filip_reference_exact.py --backend intel --benchmark-case ... --dump-launch-artifacts",
                            ]
                        )
                    )
                dump = load_replay_dump(dump_dir)
                replay = runner.replay(
                    dump=dump,
                    kernel_path=kernel_path,
                    variant=variant,
                    option_row=option_row,
                    repeats=max(1, int(repeats)),
                    debug_source_path=translated_dir / case.case_name / variant / f"opt_{option_index:03d}.metal",
                    verify_tol=float(verify_output_tol),
                )

                validation = dict(replay["validation"])
                if validation.get("within_tolerance") is False:
                    with variant_log.open("a", encoding="utf-8") as log:
                        log.write(
                            json.dumps(
                                {
                                    "case": case.case_name,
                                    "variant": variant,
                                    "option_index": option_index,
                                    "dump_dir": str(dump_dir),
                                    "validation": validation,
                                },
                                ensure_ascii=True,
                                indent=2,
                            )
                            + "\n"
                        )
                    raise SystemExit(
                        "\n".join(
                            [
                                f"Metal replay output mismatch for {case.case_name}/{variant}/opt_{option_index:03d}.",
                                f"Dump dir: {dump_dir}",
                                f"Max abs diff: {validation.get('max_abs_diff')}",
                                f"See log: {variant_log}",
                            ]
                        )
                    )

                meta = dict(dump.metadata)
                work_group_size = int(meta["work_group_size"])
                nr_work_groups = int(meta["nr_work_groups"])
                nr_threads = nr_work_groups * work_group_size
                nr_elems_per_thread = int(dump.execution_parameters[0]) if dump.execution_parameters.size >= 1 else 1
                nr_elems = int(dump.execution_parameters[1]) if dump.execution_parameters.size >= 2 else int(meta.get("nr_elems_this_kercall", 0))
                nr_elems_per_work_group = nr_elems_per_thread * work_group_size
                kernel_s = float(replay["kernel_time_s"])
                internal_s = float(replay["internal_time_s"])
                input_s = float(replay["input_time_s"])
                output_s = float(replay["output_time_s"])
                ns_per_unit = kernel_s * 1e9 / max(1.0, float(nr_elems) * float(case.n_qp))
                internal_ns_per_elem = internal_s * 1e9 / max(1.0, float(nr_elems))
                record = {
                    "case_key": f"{case.element_type}__{case.operator}",
                    "config": {
                        "operator": case.operator,
                        "element_type": case.element_type,
                        "algorithm_variant": variant,
                        "n_elements": int(nr_elems),
                        "n_qp": int(case.n_qp),
                        "workgroup_size": int(work_group_size),
                        "coal_read": int(option_row[0]),
                        "coal_write": int(option_row[1]),
                        "compute_all_shape_fun_der": int(option_row[2]),
                        "use_workspace_for_pde_coeff": int(option_row[3]),
                        "use_workspace_for_geo_data": int(option_row[4]),
                        "use_workspace_for_shape_fun": int(option_row[5]),
                        "use_workspace_for_stiff_mat": int(option_row[6]),
                        "padding": 1 if int(option_row[8]) else 0,
                    },
                    "metrics": {
                        "elapsed_s_mean": kernel_s,
                        "n_elements": float(nr_elems),
                        "n_qp_effective": float(case.n_qp),
                        "gflops_mean": float("nan"),
                        "gbps_mean": float("nan"),
                        "energy_j_mean": float("nan"),
                        "j_per_gflop": float("nan"),
                    },
                    "raw_phase": {
                        "el_data_in_mb": float(meta.get("el_data_in_bytes", int(dump.el_data_in.nbytes))) / 1e6,
                        "el_data_out_mb": float(meta.get("el_data_out_bytes", 0)) / 1e6,
                        "nr_elems_per_work_group": int(nr_elems_per_work_group),
                        "nr_elems": int(nr_elems),
                        "nr_elems_per_thread": int(nr_elems_per_thread),
                        "nr_work_groups": int(nr_work_groups),
                        "work_group_size": int(work_group_size),
                        "nr_threads": int(nr_threads),
                        "input_time_s": input_s,
                        "input_bw_gbps": float(replay["input_bw_gbps"]),
                        "kernel_time_s": kernel_s,
                        "internal_time_s": internal_s,
                        "output_time_s": output_s,
                        "output_bw_gbps": float(replay["output_bw_gbps"]),
                        "internal_ns_per_elem": internal_ns_per_elem,
                        "kernel_ns_per_unit": ns_per_unit,
                        "validation": validation,
                    },
                    "variant": variant,
                    "option_index": int(option_index),
                    "option_row": list(option_row),
                    "combo_bits": "".join("1" if int(v) else "0" for v in option_row),
                    "status": "ok",
                    "constraints_ok": True,
                    "ns_per_unit": ns_per_unit,
                }
                record["numerical_output"] = _save_output_artifacts_from_array(
                    output_dir=_option_output_dir(
                        numerical_outputs_root,
                        case_name=case.case_name,
                        variant=variant,
                        option_index=option_index,
                    ),
                    backend="metal",
                    case_name=case.case_name,
                    variant=variant,
                    option_index=option_index,
                    option_row=option_row,
                    arr=np.asarray(replay["output"], dtype=np.float32),
                    validation=validation,
                    source_dump_dir=dump_dir,
                    translated_source_path=str(replay["translated_source_path"]),
                )
                records.append(record)
                _append_jsonl(eval_path, record)
                with variant_log.open("a", encoding="utf-8") as log:
                    log.write(
                        json.dumps(
                            {
                                "case": case.case_name,
                                "variant": variant,
                                "option_index": option_index,
                                "dump_dir": str(dump_dir),
                                "translated_source_path": str(replay["translated_source_path"]),
                                "timings": {
                                    "input_time_s": input_s,
                                    "kernel_time_s": kernel_s,
                                    "internal_time_s": internal_s,
                                    "output_time_s": output_s,
                                },
                                "validation": validation,
                            },
                            ensure_ascii=True,
                        )
                        + "\n"
                    )

                score = _score(record)
                if math.isfinite(score) and score < best_so_far:
                    best_so_far = score
                _append_jsonl(
                    iter_path,
                    {
                        "iteration": complete,
                        "workflow": "filip_original_exact_metal_replay",
                        "case_key": record["case_key"],
                        "variant": record["variant"],
                        "option_index": record["option_index"],
                        "best_score_internal_ns_per_elem": best_so_far if math.isfinite(best_so_far) else None,
                    },
                )

    meta = {
        "device": device_label,
        "resolved_backend": "metal",
        "execution_mode": "exact_reference_metal_replay",
        "requested_device_index": int(requested_device_index),
        "device_index_used": int(runner.device_index),
        "repeats": max(1, int(repeats)),
        "comparison_ready_metric": "internal_ns_per_elem",
        "comparison_note": (
            "True kernel port: Metal replays the exact packed launch buffers dumped from Filip's original OpenCL path "
            "and runs translated QSS/SQS/SSQ kernels on Apple GPU."
        ),
        "replay_dump_root": str(dump_root),
        "translated_sources_root": str(translated_dir),
    }
    return records, meta


def _run_metal_exact_port(
    *,
    benchmark_case: str,
    case_specs: list[ExactCaseSpec],
    variants: list[str],
    out_dir: Path,
    eval_path: Path,
    iter_path: Path,
    device_index: int,
    requested_device_index: int,
    profile: str,
    repeats: int,
    memory_budget_mb: int,
    memory_budget_fraction: float,
    requested_n_elements: int,
    requested_workgroup_size: int,
    limit_option_rows: int,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    problem = FemParametricProblem(
        FemParametricProblemConfig(
            backend="metal",
            device_index=device_index,
            repeats=max(1, int(repeats)),
            execution_policy="native_only",
            n_elements_min=1,
            n_elements_max=1,
            n_qp_min=1,
            n_qp_max=1,
            element_types=sorted({case.element_type for case in case_specs}),
            operators=sorted({case.operator for case in case_specs}),
            dtypes=["float32"],
            algorithm_variants=list(variants),
            workgroup_sizes=[1, 32, 64, 128, 256, 512],
            use_workspace_for_pde_coeff_choices=[0, 1],
            use_workspace_for_geo_data_choices=[0, 1],
            use_workspace_for_shape_fun_choices=[0, 1],
            use_workspace_for_stiff_mat_choices=[0, 1],
            padding_choices=[0, 1],
            compute_all_shape_fun_der_choices=[0, 1],
            coal_read_choices=[0, 1],
            coal_write_choices=[0, 1],
            memory_budget_mb=max(0, int(memory_budget_mb)),
            memory_budget_fraction=float(memory_budget_fraction),
            record_raw_artifacts=False,
        )
    )
    device_label = problem.mode.device_name
    workgroup_size = _portable_suggest_workgroup(problem, int(requested_workgroup_size))
    option_rows = _filip_option_rows()
    if int(limit_option_rows) > 0:
        option_rows = option_rows[: int(limit_option_rows)]
    total_evals = len(case_specs) * len(variants) * len(option_rows)

    print("=== FILIP ORIGINAL EXACT PORT (METAL) ===")
    print(f"benchmark case     : {benchmark_case}")
    print("backend            : metal")
    print(f"device label       : {device_label}")
    print(f"profile            : {profile}")
    print(f"repeats            : {repeats}")
    print(f"workgroup size     : {workgroup_size}")
    print(f"total evaluations  : {total_evals}")
    print(f"out dir            : {out_dir}")

    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    records_portable: list[dict[str, Any]] = []
    best_so_far = float("inf")
    complete = 0

    for case in case_specs:
        case_n_elements = int(requested_n_elements)
        if case_n_elements <= 0:
            case_n_elements = _portable_auto_n_elements(
                problem=problem,
                profile=profile,
                element_type=case.element_type,
                operator=case.operator,
                dtype="float32",
                n_qp=case.n_qp,
                workgroup_size=workgroup_size,
            )

        case_log_dir = raw_dir / case.case_name
        case_log_dir.mkdir(parents=True, exist_ok=True)
        for variant in variants:
            variant_log = case_log_dir / f"{case.case_name}__{variant}.log"
            with variant_log.open("w", encoding="utf-8") as log:
                log.write(
                    "\n".join(
                        [
                            "# Filip exact-style Metal port",
                            f"# case={case.case_name}",
                            f"# variant={variant}",
                            f"# backend=metal",
                            f"# device={device_label}",
                            f"# workgroup_size={workgroup_size}",
                            f"# n_elements={case_n_elements}",
                            "",
                        ]
                    )
                )
            for option_index, option_row in enumerate(option_rows):
                complete += 1
                print(
                    f"[metal exact] {complete}/{total_evals} case={case.case_name} variant={variant} option={option_index + 1}/{len(option_rows)}",
                    flush=True,
                )
                cfg = {
                    "n_elements": int(case_n_elements),
                    "n_qp": int(case.n_qp),
                    "element_type": case.element_type,
                    "operator": case.operator,
                    "dtype": "float32",
                    "algorithm_variant": variant,
                    "workgroup_size": int(workgroup_size),
                    "coal_read": int(option_row[0]),
                    "coal_write": int(option_row[1]),
                    "compute_all_shape_fun_der": int(option_row[2]),
                    "use_workspace_for_pde_coeff": int(option_row[3]),
                    "use_workspace_for_geo_data": int(option_row[4]),
                    "use_workspace_for_shape_fun": int(option_row[5]),
                    "use_workspace_for_stiff_mat": int(option_row[6]),
                    "padding": 1 if int(option_row[8]) == 1 else 0,
                }
                res = problem.evaluate(cfg)
                metrics = dict(res.metrics)
                cfg_eff = (
                    dict(res.artifacts.get("config_effective"))
                    if isinstance(res.artifacts.get("config_effective"), dict)
                    else dict(cfg)
                )
                ns_per_unit = (
                    _safe_float(metrics.get("elapsed_s_mean")) * 1e9
                    / max(1.0, _safe_float(metrics.get("n_elements")) * max(1.0, _safe_float(metrics.get("n_qp_effective"))))
                )
                record = {
                    "backend": "metal",
                    "device": device_label,
                    "case_key": f"{case.element_type}__{case.operator}",
                    "variant": variant,
                    "option_index": option_index,
                    "option_row": list(option_row),
                    "config": cfg_eff,
                    "metrics": metrics,
                    "status": str(res.status),
                    "constraints_ok": bool(res.constraints_ok),
                    "violations": list(res.violations),
                    "ns_per_unit": ns_per_unit,
                }
                records_portable.append(record)
                with variant_log.open("a", encoding="utf-8") as log:
                    log.write(json.dumps(record, ensure_ascii=True) + "\n")
                if math.isfinite(ns_per_unit) and res.status == "ok" and res.constraints_ok and ns_per_unit < best_so_far:
                    best_so_far = ns_per_unit
                _append_jsonl(
                    iter_path,
                    {
                        "iteration": complete,
                        "workflow": "filip_original_exact_metal_port",
                        "case_key": record["case_key"],
                        "variant": variant,
                        "option_index": option_index,
                        "best_score_ns_per_unit": best_so_far if math.isfinite(best_so_far) else None,
                    },
                )

    plot_rows = [_portable_plot_row(record, backend="metal", device=device_label) for record in records_portable if record["status"] == "ok"]
    peak_gflops, peak_bw = _roofline_peaks_for_backend("metal", plot_rows)
    records = [
        _portable_record_to_payload(
            case=next(spec for spec in case_specs if f"{spec.element_type}__{spec.operator}" == record["case_key"]),
            backend="metal",
            device_label=device_label,
            record=record,
            peak_gflops=peak_gflops,
            peak_bw=peak_bw,
        )
        for record in records_portable
    ]
    for record in records:
        _append_jsonl(eval_path, record)

    meta = {
        "device": device_label,
        "resolved_backend": "metal",
        "execution_mode": problem.mode.execution_mode,
        "requested_device_index": int(requested_device_index),
        "device_index_used": int(device_index),
        "profile": str(profile),
        "repeats": int(repeats),
        "workgroup_size": int(workgroup_size),
        "memory_budget_bytes": int(problem.mode.profile.memory_budget_bytes),
        "comparison_ready_metric": "internal_ns_per_elem",
        "comparison_note": (
            "Metal exact-style port uses the native project Metal FEM backend and the same 80-option Filip campaign. "
            "Phase timings are reconstructed from the portable performance model; this is not the legacy OpenCL/internal event timeline."
        ),
    }
    return records, meta


def main() -> None:
    ap = argparse.ArgumentParser(description="Run Filip's exact validation workflow on legacy OpenCL or the native Metal exact-style port.")
    ap.add_argument("--backend", choices=["opencl", "intel", "metal", "auto"], default="auto")
    ap.add_argument("--benchmark-case", choices=sorted(CASE_SPECS.keys()), default="prism_pair")
    ap.add_argument("--variants", default="qss,sqs,ssq")
    ap.add_argument("--modfem-dir", default=str(_default_modfem_dir()))
    ap.add_argument("--arch-laplace", default="auto")
    ap.add_argument("--arch-test", default="auto")
    ap.add_argument("--input-override", default="", help="Optional replacement input_interactive.txt for exact runs.")
    ap.add_argument("--device-label", default="", help="Optional label written to summary/CSV. Does not change actual OpenCL selection.")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--memory-budget-mb", type=int, default=768)
    ap.add_argument("--memory-budget-fraction", type=float, default=0.35)
    ap.add_argument("--n-elements", type=int, default=0, help="Metal exact-style port only. 0 = automatic conservative size.")
    ap.add_argument("--workgroup-size", type=int, default=0, help="Metal exact-style port only. 0 = prefer 64 or nearest supported.")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--dump-launch-artifacts", action="store_true", help="OpenCL exact only: dump packed launch buffers for later Metal replay.")
    ap.add_argument("--replay-dump-root", default="", help="Metal exact only: path to OpenCL launch_dumps root or an exact output dir that contains it.")
    ap.add_argument("--verify-output-tol", type=float, default=1e-5, help="Metal replay only: max abs diff tolerance against dumped OpenCL outputs.")
    ap.add_argument("--limit-option-rows", type=int, default=0, help=argparse.SUPPRESS)
    args = ap.parse_args()

    resolved_backend = _resolve_exact_backend(args.backend)

    variants = _parse_csv_list(args.variants)
    if not variants:
        variants = list(VARIANT_ORDER)
    unsupported_variants = [variant for variant in variants if variant not in VARIANT_ORDER]
    if unsupported_variants:
        raise SystemExit(f"Unsupported variants for exact reference mode: {', '.join(unsupported_variants)}")

    out_dir = _make_out_dir(resolved_backend)
    eval_path = out_dir / "evaluations.jsonl"
    iter_path = out_dir / "iterations.jsonl"
    logs_dir = out_dir / "logs"
    logs_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "plots").mkdir(parents=True, exist_ok=True)

    case_specs = list(CASE_SPECS[args.benchmark_case])
    normalized_case_specs: list[ExactCaseSpec] = []
    for case in case_specs:
        arch = case.arch
        if case.case_name == "laplace_prism" and str(args.arch_laplace).strip().lower() not in {"", "auto"}:
            arch = str(args.arch_laplace).strip()
        if case.case_name == "test_prism" and str(args.arch_test).strip().lower() not in {"", "auto"}:
            arch = str(args.arch_test).strip()
        normalized_case_specs.append(
            ExactCaseSpec(
                case_name=case.case_name,
                label=case.label,
                operator=case.operator,
                element_type=case.element_type,
                n_qp=case.n_qp,
                arch=arch,
                work_subdir=case.work_subdir,
                binary_name=case.binary_name,
            )
        )

    if resolved_backend == "metal":
        resolved_device_index, _ = resolve_device_index("metal", int(args.device_index))
        replay_dump_root = Path(args.replay_dump_root).expanduser().resolve() if str(args.replay_dump_root).strip() else None
        numerical_outputs_root = out_dir / "numerical_outputs"
        if replay_dump_root is not None:
            mod_dir = Path(args.modfem_dir).expanduser().resolve()
            records, metal_meta = _run_metal_exact_replay(
                mod_dir=mod_dir,
                benchmark_case=args.benchmark_case,
                case_specs=normalized_case_specs,
                variants=variants,
                out_dir=out_dir,
                eval_path=eval_path,
                iter_path=iter_path,
                replay_dump_root=replay_dump_root,
                numerical_outputs_root=numerical_outputs_root,
                device_index=resolved_device_index,
                requested_device_index=int(args.device_index),
                repeats=int(args.repeats),
                limit_option_rows=int(args.limit_option_rows),
                verify_output_tol=float(args.verify_output_tol),
            )
        else:
            records, metal_meta = _run_metal_exact_port(
                benchmark_case=args.benchmark_case,
                case_specs=normalized_case_specs,
                variants=variants,
                out_dir=out_dir,
                eval_path=eval_path,
                iter_path=iter_path,
                device_index=resolved_device_index,
                requested_device_index=int(args.device_index),
                profile=str(args.profile),
                repeats=int(args.repeats),
                memory_budget_mb=int(args.memory_budget_mb),
                memory_budget_fraction=float(args.memory_budget_fraction),
                requested_n_elements=int(args.n_elements),
                requested_workgroup_size=int(args.workgroup_size),
                limit_option_rows=int(args.limit_option_rows),
            )
        device_label = str(args.device_label).strip() or str(metal_meta["device"])
        case_csvs = _write_exact_case_csvs(out_dir, records, backend="metal")
        combined_csv = out_dir / "csv" / "result_filip_original__metal.csv"
        _write_current_combined_csv(combined_csv, records, backend="metal", device_label=device_label)
        plots = _plot_exact_results(records, out_dir)
        best_overall = _best_overall_payload(records)
        summary = {
            "created_at": datetime.now().astimezone().isoformat(),
            "backend": "metal",
            "resolved_backend": "metal",
            "execution_mode": str(metal_meta["execution_mode"]),
            "workflow": "filip_original",
            "filip_mode": "exact_reference",
            "benchmark_case": args.benchmark_case,
            "operators": sorted({case.operator for case in normalized_case_specs}),
            "element_types": sorted({case.element_type for case in normalized_case_specs}),
            "variants": variants,
            "device": device_label,
            "cases": [
                {
                    "case_name": case.case_name,
                    "label": case.label,
                    "operator": case.operator,
                    "element_type": case.element_type,
                    "n_qp": case.n_qp,
                    "arch": "metal_port",
                }
                for case in normalized_case_specs
            ],
            "total_evaluations": len(records),
            "evaluations_path": str(eval_path),
            "iterations_path": str(iter_path),
            "combined_csv": str(combined_csv),
            "case_csvs": case_csvs,
            "plots": plots,
            "best_overall": best_overall,
            "comparison_ready_metric": str(metal_meta["comparison_ready_metric"]),
            "comparison_note": str(metal_meta["comparison_note"]),
            "out_dir": str(out_dir),
            "profile": metal_meta.get("profile"),
            "repeats": metal_meta["repeats"],
            "workgroup_size": metal_meta.get("workgroup_size"),
            "memory_budget_bytes": metal_meta.get("memory_budget_bytes"),
            "device_index_requested": metal_meta["requested_device_index"],
            "device_index_used": metal_meta["device_index_used"],
            "replay_dump_root": metal_meta.get("replay_dump_root"),
            "translated_sources_root": metal_meta.get("translated_sources_root"),
            "numerical_outputs": _exact_numerical_output_summary(
                records=records,
                root=numerical_outputs_root if metal_meta["execution_mode"] == "exact_reference_metal_replay" else None,
                best_overall=best_overall,
                note=(
                    "Saved per-option Metal output buffers and JSON previews for replay validation."
                    if metal_meta["execution_mode"] == "exact_reference_metal_replay"
                    else "The exact-style Metal fallback does not currently expose raw output buffers."
                ),
            ),
            "validation_summary": _validation_summary(records),
        }
    else:
        mod_dir = Path(args.modfem_dir).expanduser().resolve()
        _require_path(mod_dir, "Filip mod_2022 directory")
        input_override = Path(args.input_override).expanduser().resolve() if str(args.input_override).strip() else None
        if input_override is not None:
            _require_path(input_override, "input_interactive override")

        staged_cases: list[tuple[ExactCaseSpec, Path, list[list[int]]]] = []
        for case in normalized_case_specs:
            work_dir, option_rows = _copy_case_workspace(
                mod_dir=mod_dir,
                case=case,
                out_dir=out_dir,
                input_override=input_override,
                limit_option_rows=int(args.limit_option_rows),
            )
            staged_cases.append((case, work_dir, option_rows))

        total_evals = sum(len(option_rows) * len(variants) for _, _, option_rows in staged_cases)
        device_label = str(args.device_label).strip() or _detect_device_label("intel")

        print("=== FILIP ORIGINAL EXACT REFERENCE ===")
        print(f"benchmark case     : {args.benchmark_case}")
        print("backend            : opencl")
        print(f"device label       : {device_label}")
        print(f"mod_fem_dir        : {mod_dir}")
        print(f"variants           : {','.join(variants)}")
        print(f"total evaluations  : {total_evals}")
        print(f"out dir            : {out_dir}")

        built_arches: dict[str, Path] = {}
        dump_launch_root = (out_dir / "launch_dumps") if bool(args.dump_launch_artifacts) else None
        numerical_outputs_root = dump_launch_root if dump_launch_root is not None else (out_dir / "numerical_outputs")
        records = []
        eval_counter = 0
        for case, work_dir, option_rows in staged_cases:
            if case.arch not in built_arches:
                built_arches[case.arch] = _build_case(
                    mod_dir=mod_dir,
                    arch=case.arch,
                    rebuild=not bool(args.skip_build),
                    log_dir=logs_dir,
                )
            binary = built_arches[case.arch]
            case_records, eval_counter = _run_case(
                mod_dir=mod_dir,
                case=case,
                work_dir=work_dir,
                option_rows=option_rows,
                variants=variants,
                binary=binary,
                out_dir=out_dir,
                eval_path=eval_path,
                iter_path=iter_path,
                global_eval_start=eval_counter,
                total_evals=total_evals,
                device_label=device_label,
                numerical_outputs_root=numerical_outputs_root,
                dump_launch_root=dump_launch_root,
            )
            records.extend(case_records)

        case_csvs = _write_exact_case_csvs(out_dir, records, backend="opencl")
        combined_csv = out_dir / "csv" / "result_filip_original__opencl.csv"
        _write_current_combined_csv(combined_csv, records, backend="opencl", device_label=device_label)
        plots = _plot_exact_results(records, out_dir)
        best_overall = _best_overall_payload(records)
        summary = {
            "created_at": datetime.now().astimezone().isoformat(),
            "backend": "opencl",
            "resolved_backend": "opencl",
            "execution_mode": "exact_reference",
            "workflow": "filip_original",
            "filip_mode": "exact_reference",
            "benchmark_case": args.benchmark_case,
            "operators": sorted({case.operator for case in normalized_case_specs}),
            "element_types": sorted({case.element_type for case in normalized_case_specs}),
            "variants": variants,
            "device": device_label,
            "modfem_dir": str(mod_dir),
            "built_arches": {arch: str(path) for arch, path in built_arches.items()},
            "cases": [
                {
                    "case_name": case.case_name,
                    "label": case.label,
                    "operator": case.operator,
                    "element_type": case.element_type,
                    "n_qp": case.n_qp,
                    "arch": case.arch,
                    "work_dir": str(work_dir),
                    "option_rows": len(option_rows),
                }
                for case, work_dir, option_rows in staged_cases
            ],
            "total_evaluations": len(records),
            "evaluations_path": str(eval_path),
            "iterations_path": str(iter_path),
            "combined_csv": str(combined_csv),
            "case_csvs": case_csvs,
            "plots": plots,
            "best_overall": best_overall,
            "comparison_ready_metric": "internal_ns_per_elem",
            "comparison_note": "This mode executes Filip's original OpenCL path and records native 'internal' timings from the original CSV output.",
            "out_dir": str(out_dir),
            "launch_dumps_root": str(dump_launch_root) if dump_launch_root is not None else "",
            "numerical_outputs": _exact_numerical_output_summary(
                records=records,
                root=numerical_outputs_root,
                best_overall=best_overall,
                note=(
                    "Saved per-option OpenCL output buffers and JSON previews."
                    if dump_launch_root is None
                    else "Per-option OpenCL outputs are available inside launch_dumps together with full launch artifacts."
                ),
            ),
        }

    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
