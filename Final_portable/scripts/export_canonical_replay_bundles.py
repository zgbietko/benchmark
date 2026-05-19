#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.provenance import collect_runtime_provenance, sha256_json
from scripts.export_filip_replay_inputs import export_replay_input_bundle, resolve_dump_root

CANONICAL_PRESETS: dict[str, dict[str, object]] = {
    "smoke_test_prism_qss_opt000": {
        "cases": ["test_prism"],
        "variants": ["qss"],
        "option_indices": [0],
    },
    "smoke_test_prism_sqs_opt017": {
        "cases": ["test_prism"],
        "variants": ["sqs"],
        "option_indices": [17],
    },
    "smoke_laplace_prism_ssq_opt062": {
        "cases": ["laplace_prism"],
        "variants": ["ssq"],
        "option_indices": [62],
    },
}


def export_canonical_replay_bundles(*, src: Path, out_root: Path, include_expected_output: bool = True) -> dict[str, object]:
    src_root = resolve_dump_root(Path(src))
    out_root = Path(out_root).expanduser().resolve()
    if out_root.exists():
        raise SystemExit(f"Output path already exists: {out_root}")
    out_root.mkdir(parents=True, exist_ok=False)

    bundles: dict[str, object] = {}
    for name, preset in CANONICAL_PRESETS.items():
        bundle_dir = out_root / name
        manifest = export_replay_input_bundle(
            src=src_root,
            out=bundle_dir,
            cases=list(preset["cases"]),
            variants=list(preset["variants"]),
            option_indices=list(preset["option_indices"]),
            include_expected_output=bool(include_expected_output),
        )
        bundles[name] = manifest

    top_manifest = {
        "source_root": str(src_root),
        "bundle_root": str(out_root),
        "include_expected_output": bool(include_expected_output),
        "presets": bundles,
        "provenance": collect_runtime_provenance(ROOT),
    }
    top_manifest["bundle_hash"] = sha256_json(top_manifest)
    manifest_path = out_root / "canonical_bundles_manifest.json"
    manifest_path.write_text(json.dumps(top_manifest, indent=2, ensure_ascii=True), encoding="utf-8")
    top_manifest["manifest_path"] = str(manifest_path)
    return top_manifest


def main() -> None:
    ap = argparse.ArgumentParser(description="Export canonical small replay bundles from Filip OpenCL launch dumps.")
    ap.add_argument("--src", required=True, help="Path to exact output dir or launch_dumps root.")
    ap.add_argument("--out", required=True, help="Output directory for the canonical replay bundles.")
    ap.add_argument("--without-expected-output", action="store_true", help="Do not include el_data_out.bin in the bundles.")
    args = ap.parse_args()

    manifest = export_canonical_replay_bundles(
        src=Path(args.src),
        out_root=Path(args.out),
        include_expected_output=not bool(args.without_expected_output),
    )
    print("=== CANONICAL REPLAY BUNDLES ===")
    print(f"source root : {manifest['source_root']}")
    print(f"bundle root : {manifest['bundle_root']}")
    print(f"manifest    : {manifest['manifest_path']}")
    print(f"presets     : {', '.join(sorted(CANONICAL_PRESETS.keys()))}")


if __name__ == "__main__":
    main()
