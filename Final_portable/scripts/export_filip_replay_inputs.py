#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path


INPUT_FILES = [
    "launch_meta.json",
    "execution_parameters.bin",
    "gauss_dat.bin",
    "shape_fun_ref.bin",
    "el_data_in.bin",
]
OPTIONAL_FILES = [
    "requested_option.json",
]
EXPECTED_OUTPUT_FILES = [
    "el_data_out.bin",
]


def resolve_dump_root(path: Path) -> Path:
    candidate = path.expanduser().resolve()
    launch_dumps = candidate / "launch_dumps"
    return launch_dumps if launch_dumps.exists() else candidate


def _is_variant_dir(path: Path) -> bool:
    if not path.is_dir():
        return False
    try:
        return any(child.is_dir() and child.name.startswith("opt_") for child in path.iterdir())
    except OSError:
        return False


def discover_case_dirs(root: Path) -> dict[str, Path]:
    if _is_variant_dir(root):
        return {root.parent.name: root.parent}
    case_dirs: dict[str, Path] = {}
    for child in sorted(root.iterdir()):
        if not child.is_dir():
            continue
        if any(_is_variant_dir(grandchild) for grandchild in child.iterdir() if grandchild.is_dir()):
            case_dirs[child.name] = child
    if case_dirs:
        return case_dirs
    if any(_is_variant_dir(child) for child in root.iterdir() if child.is_dir()):
        return {root.name: root}
    raise SystemExit(f"Could not find replay case directories under: {root}")


def parse_csv_list(raw: str) -> list[str]:
    items = [part.strip() for part in str(raw).split(",") if part.strip()]
    return items


def parse_int_list(raw: str) -> list[int]:
    values: list[int] = []
    for item in parse_csv_list(raw):
        try:
            values.append(int(item))
        except ValueError as exc:
            raise SystemExit(f"Invalid integer in list: {item}") from exc
    return values


def format_bytes(num_bytes: int) -> str:
    size = float(max(0, int(num_bytes)))
    for unit in ["B", "KB", "MB", "GB", "TB"]:
        if size < 1024.0 or unit == "TB":
            return f"{size:.1f} {unit}"
        size /= 1024.0
    return f"{size:.1f} TB"


def export_replay_input_bundle(
    *,
    src: Path,
    out: Path,
    cases: list[str] | None = None,
    variants: list[str] | None = None,
    option_indices: list[int] | None = None,
    limit_option_rows: int = 0,
    include_expected_output: bool = False,
) -> dict[str, object]:
    src_root = resolve_dump_root(Path(src))
    out_root = Path(out).expanduser().resolve()
    launch_out_root = out_root / "launch_dumps"

    if out_root.exists():
        raise SystemExit(f"Output path already exists: {out_root}")

    case_dirs = discover_case_dirs(src_root)
    selected_cases = list(cases) if cases else list(case_dirs.keys())
    missing_cases = [case for case in selected_cases if case not in case_dirs]
    if missing_cases:
        raise SystemExit(f"Requested case(s) not found under {src_root}: {', '.join(missing_cases)}")

    selected_variants = set(variants or [])
    selected_option_indices = set(int(v) for v in (option_indices or []))
    required_files = list(INPUT_FILES) + (list(EXPECTED_OUTPUT_FILES) if bool(include_expected_output) else [])

    export_manifest: dict[str, object] = {
        "source_root": str(src_root),
        "bundle_root": str(out_root),
        "include_expected_output": bool(include_expected_output),
        "required_files": required_files,
        "cases": {},
        "total_options_exported": 0,
        "total_bytes_copied": 0,
    }

    out_root.mkdir(parents=True, exist_ok=False)
    launch_out_root.mkdir(parents=True, exist_ok=True)

    for case_name in selected_cases:
        case_dir = case_dirs[case_name]
        variant_dirs = {
            child.name: child
            for child in sorted(case_dir.iterdir())
            if child.is_dir() and _is_variant_dir(child)
        }
        case_variants = sorted(selected_variants or set(variant_dirs.keys()))
        missing_variants = [variant for variant in case_variants if variant not in variant_dirs]
        if missing_variants:
            raise SystemExit(
                f"Requested variant(s) not found for case {case_name}: {', '.join(missing_variants)}"
            )

        case_manifest: dict[str, object] = {"variants": {}}

        for variant in case_variants:
            variant_dir = variant_dirs[variant]
            option_dirs = sorted(
                (path for path in variant_dir.iterdir() if path.is_dir() and path.name.startswith("opt_")),
                key=lambda path: int(path.name.split("_", 1)[1]),
            )
            if selected_option_indices:
                option_dirs = [
                    path for path in option_dirs if int(path.name.split("_", 1)[1]) in selected_option_indices
                ]
            if int(limit_option_rows) > 0:
                option_dirs = option_dirs[: int(limit_option_rows)]
            if not option_dirs:
                raise SystemExit(f"No option directories selected for {case_name}/{variant}")

            variant_manifest: dict[str, object] = {
                "option_count": len(option_dirs),
                "options": [],
            }
            dest_variant_dir = launch_out_root / case_name / variant
            dest_variant_dir.mkdir(parents=True, exist_ok=True)

            for option_dir in option_dirs:
                option_index = int(option_dir.name.split("_", 1)[1])
                missing = [name for name in required_files if not (option_dir / name).exists()]
                if missing:
                    raise SystemExit(
                        f"Replay dump is incomplete for {case_name}/{variant}/{option_dir.name}: missing {', '.join(missing)}"
                    )
                dest_option_dir = dest_variant_dir / option_dir.name
                dest_option_dir.mkdir(parents=True, exist_ok=True)

                copied_files: list[dict[str, object]] = []
                for name in list(INPUT_FILES) + list(OPTIONAL_FILES) + (list(EXPECTED_OUTPUT_FILES) if bool(include_expected_output) else []):
                    src_file = option_dir / name
                    if not src_file.exists():
                        continue
                    dest_file = dest_option_dir / name
                    shutil.copy2(src_file, dest_file)
                    file_size = int(dest_file.stat().st_size)
                    copied_files.append({"name": name, "bytes": file_size})
                    export_manifest["total_bytes_copied"] = int(export_manifest["total_bytes_copied"]) + file_size

                variant_manifest["options"].append(
                    {
                        "option_index": option_index,
                        "source_dir": str(option_dir),
                        "dest_dir": str(dest_option_dir),
                        "files": copied_files,
                    }
                )
                export_manifest["total_options_exported"] = int(export_manifest["total_options_exported"]) + 1

            case_manifest["variants"][variant] = variant_manifest

        export_manifest["cases"][case_name] = case_manifest

    readme_path = out_root / "README_replay_bundle.md"
    readme_path.write_text(
        "\n".join(
            [
                "# Filip replay input bundle",
                "",
                "This directory is a compact export of OpenCL `launch_dumps` for deterministic replay.",
                "",
                "It preserves the directory layout expected by:",
                "",
                "- `python run_filip_reference_exact.py --backend metal --replay-dump-root <this_dir>`",
                "",
                "What it contains:",
                "",
                "- `launch_meta.json`",
                "- `execution_parameters.bin`",
                "- `gauss_dat.bin`",
                "- `shape_fun_ref.bin`",
                "- `el_data_in.bin`",
                "- optional `el_data_out.bin` if exported with `--include-expected-output`",
                "",
                "Without `el_data_out.bin`, replay still runs on the same frozen inputs, but it cannot validate against OpenCL output.",
                "",
            ]
        ),
        encoding="utf-8",
    )

    manifest_path = out_root / "replay_bundle_manifest.json"
    manifest_path.write_text(json.dumps(export_manifest, indent=2, ensure_ascii=True), encoding="utf-8")
    export_manifest["manifest_path"] = str(manifest_path)
    export_manifest["bundle_root"] = str(out_root)
    return export_manifest


def main() -> None:
    ap = argparse.ArgumentParser(
        description=(
            "Export a compact, deterministic Filip replay bundle from OpenCL launch_dumps. "
            "The result keeps only the input-side files needed to replay the same problem on Metal."
        )
    )
    ap.add_argument("--src", required=True, help="Path to an exact output dir, launch_dumps dir, or case-specific dump dir.")
    ap.add_argument("--out", required=True, help="Output directory for the compact replay bundle.")
    ap.add_argument("--cases", default="", help="Comma-separated case names to export, e.g. test_prism,laplace_prism. Default: all discovered.")
    ap.add_argument("--variants", default="", help="Comma-separated variants to export, e.g. qss,sqs,ssq. Default: all discovered.")
    ap.add_argument("--option-indices", default="", help="Comma-separated option indices to export, e.g. 0,7,22. Default: all discovered.")
    ap.add_argument("--limit-option-rows", type=int, default=0, help="Export only the first N option rows per variant. Default: all.")
    ap.add_argument(
        "--include-expected-output",
        action="store_true",
        help="Also copy el_data_out.bin for OpenCL-vs-Metal validation. Without this, the bundle only freezes inputs.",
    )
    args = ap.parse_args()

    export_manifest = export_replay_input_bundle(
        src=Path(args.src),
        out=Path(args.out),
        cases=parse_csv_list(args.cases) if args.cases else None,
        variants=parse_csv_list(args.variants) if args.variants else None,
        option_indices=parse_int_list(args.option_indices) if args.option_indices else None,
        limit_option_rows=int(args.limit_option_rows),
        include_expected_output=bool(args.include_expected_output),
    )

    print("=== FILIP REPLAY INPUT BUNDLE ===")
    print(f"source root         : {export_manifest['source_root']}")
    print(f"bundle root         : {export_manifest['bundle_root']}")
    print(f"include exp. output : {bool(export_manifest['include_expected_output'])}")
    print(f"total options       : {export_manifest['total_options_exported']}")
    print(f"total bytes copied  : {export_manifest['total_bytes_copied']} ({format_bytes(int(export_manifest['total_bytes_copied']))})")
    print(f"manifest            : {export_manifest['manifest_path']}")
    print()
    print("Replay on Metal with:")
    print(
        "  python run_filip_reference_exact.py "
        f"--backend metal --benchmark-case <case> --replay-dump-root {export_manifest['bundle_root']}"
    )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
