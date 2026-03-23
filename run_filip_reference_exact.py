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
import re
import shutil
import subprocess
import sys
from typing import Any, Iterable


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


def _make_out_dir() -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    out = ROOT / "data" / "optimization" / f"{ts}__filip_original__backend-opencl__exact"
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
    env = os.environ.copy()
    env["MOD_FEM_DIR"] = str(mod_dir)
    env["MOD_FEM_ARCH"] = arch
    src_dir = mod_dir / "src"
    build_log = log_dir / f"build__{arch}.log"
    binary = mod_dir / "bin" / arch / "MFEM_conv_diff_prism_std_krb_ocl"
    csh_shell = _detect_csh_shell()
    if not rebuild:
        return _require_path(binary, f"existing OpenCL binary for arch {arch}")
    cmds: list[list[str]] = []
    if rebuild:
        cmds.extend(
            [
                ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit", "deep_clean"],
                ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit", "clean"],
            ]
        )
    cmds.extend(
        [
            ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit"],
            ["make", f"SHELL={csh_shell}", "-f", "Makefile_explicit", "conv_diff_prism_std_krb_ocl"],
        ]
    )
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


def _append_jsonl(path: Path, payload: dict[str, Any]) -> None:
    with path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(payload, ensure_ascii=True) + "\n")


def _write_current_combined_csv(path: Path, records: list[dict[str, Any]], *, device_label: str) -> None:
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
                    "opencl",
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
) -> tuple[list[dict[str, Any]], int]:
    env = os.environ.copy()
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
            _run_binary_once(binary=binary, work_dir=work_dir, env=env, log_path=variant_log)
            if not header_path.exists() or not result_path.exists():
                raise SystemExit(f"Expected files missing after run: {header_path} / {result_path}")
            record = _result_line_to_payload(
                case=case,
                variant=variant,
                option_index=option_index,
                option_row=option_row,
                raw_line=_read_last_result_line(result_path),
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


def main() -> None:
    ap = argparse.ArgumentParser(description="Run Filip's original OpenCL reference benchmark as an exact validation workflow.")
    ap.add_argument("--backend", choices=["opencl", "intel", "auto"], default="intel")
    ap.add_argument("--benchmark-case", choices=sorted(CASE_SPECS.keys()), default="prism_pair")
    ap.add_argument("--variants", default="qss,sqs,ssq")
    ap.add_argument("--modfem-dir", default=str(_default_modfem_dir()))
    ap.add_argument("--arch-laplace", default="auto")
    ap.add_argument("--arch-test", default="auto")
    ap.add_argument("--input-override", default="", help="Optional replacement input_interactive.txt for exact runs.")
    ap.add_argument("--device-label", default="", help="Optional label written to summary/CSV. Does not change actual OpenCL selection.")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--limit-option-rows", type=int, default=0, help=argparse.SUPPRESS)
    args = ap.parse_args()

    backend = str(args.backend).strip().lower()
    if backend not in {"opencl", "intel", "auto"}:
        raise SystemExit("Exact Filip reference mode supports only OpenCL/Intel backends.")

    mod_dir = Path(args.modfem_dir).expanduser().resolve()
    _require_path(mod_dir, "Filip mod_2022 directory")
    input_override = Path(args.input_override).expanduser().resolve() if str(args.input_override).strip() else None
    if input_override is not None:
        _require_path(input_override, "input_interactive override")

    variants = _parse_csv_list(args.variants)
    if not variants:
        variants = list(VARIANT_ORDER)
    unsupported_variants = [variant for variant in variants if variant not in VARIANT_ORDER]
    if unsupported_variants:
        raise SystemExit(f"Unsupported variants for exact reference mode: {', '.join(unsupported_variants)}")

    out_dir = _make_out_dir()
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
    print(f"backend            : opencl")
    print(f"device label       : {device_label}")
    print(f"mod_fem_dir        : {mod_dir}")
    print(f"variants           : {','.join(variants)}")
    print(f"total evaluations  : {total_evals}")
    print(f"out dir            : {out_dir}")

    built_arches: dict[str, Path] = {}
    records: list[dict[str, Any]] = []
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
        )
        records.extend(case_records)

    combined_csv = out_dir / "csv" / "result_filip_original__opencl.csv"
    _write_current_combined_csv(combined_csv, records, device_label=device_label)
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
        "plots": plots,
        "best_overall": best_overall,
        "comparison_ready_metric": "internal_ns_per_elem",
        "comparison_note": "This mode executes Filip's original OpenCL path and records native 'internal' timings from the original CSV output.",
        "out_dir": str(out_dir),
    }
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    print(json.dumps(summary, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
