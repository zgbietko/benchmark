#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
from datetime import datetime
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT = ROOT.parent / "Final_portable"

DIRS_TO_COPY = [
    "ai_accel",
    "analysis",
    "configs",
    "cpu",
    "docs",
    "gpu",
    "monitoring",
    "optimization",
    "profiles",
    "real_kernels",
    "schemas",
    "scripts",
    "tests",
    "validation",
    "web",
]

OPTIONAL_DIRS = [
    "legacy/filip_exact_bundle",
    "Kod Filipa",
]

ROOT_FILES = [
    "README.md",
    "requirements-ubuntu.txt",
    "cpu_utils.py",
    "device_catalog.py",
    "device_resolution.py",
    "energy.py",
    "energy_utils.py",
    "fem_catalog.py",
    "gpu_utils.py",
    "platform_support.py",
    "run_all_backends.py",
    "run_all_benchmarks.py",
    "run_all_cpu_benchmarks.py",
    "run_all_gpu_benchmarks.py",
    "run_autotune_gui.py",
    "run_console_app.py",
    "run_desktop_gui.py",
    "run_device_discovery.py",
    "run_fem_option_validation.py",
    "run_fem_parametric_matrix.py",
    "run_fem_parametric_preflight.py",
    "run_filip_autotuning.py",
    "run_filip_original.py",
    "run_filip_reference_exact.py",
    "run_firefly_optimization.py",
    "run_full_thesis_pipeline.py",
    "run_session.py",
    "run_workflow.py",
]

IGNORE_NAMES = {
    ".DS_Store",
    ".cache",
    ".tmp_fig4",
    "__pycache__",
    "data",
    "wykres.png",
}

PORTABLE_README = """# Final portable Linux bundle

Ta paczka jest dodatkowa wobec glownej wersji `Final`.
Ma sluzyc do przeniesienia projektu na pendrive i uruchamiania testow na obcym Linuxie
z zachowaniem tej samej logiki workflowow co w glownej wersji.

Najwazniejsze ograniczenia:
- bundle przenosi kod, skrypty i lokalne srodowisko Pythona tworzone na hoscie,
- ale nie przenosi sterownikow GPU, CUDA, ROCm ani systemowego OpenCL runtime,
- `exact_reference` na Linux/OpenCL nadal wymaga oneAPI + assets `mod_2022`,
- jesli historyczny `filip_exact_bundle` jest uszkodzony albo nieczytelny, portable exact uruchamiaj z zewnetrznym `--modfem-dir`,
- na Apple exact-style Metal port dziala lokalnie w glownej wersji `Final`, a nie przez ten linuxowy launcher.

Szybki start:
```bash
cd /sciezka/do/Final_portable
bash ./LAUNCH_PORTABLE.sh --package full
```

Jesli host nie ma jeszcze lokalnego env, launcher sam uruchomi bootstrap.

Glowne pakiety:
- `benchmarks`
- `real-kernels`
- `filip`
- `full`

Mozna tez uruchamiac dowolny workflow bezposrednio:
```bash
bash ./LAUNCH_PORTABLE.sh --workflow ai_accel
bash ./LAUNCH_PORTABLE.sh --workflow filip_original --filip-mode exact_reference --modfem-dir /sciezka/do/mod_2022
```

Desktop GUI:
```bash
bash ./LAUNCH_DESKTOP_GUI.sh
```

Dokumentacja:
- `docs/PORTABLE_LINUX_BUNDLE.md`
"""


def _ignore(_dir: str, names: list[str]) -> set[str]:
    ignored = set()
    for name in names:
        if name in IGNORE_NAMES:
            ignored.add(name)
        if name.endswith(".pyc") or name.endswith(".pyo"):
            ignored.add(name)
    return ignored


def _copytree(src: Path, dst: Path) -> None:
    shutil.copytree(src, dst, ignore=_ignore, dirs_exist_ok=True)


def _ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def _copy_paths(paths: Iterable[str], src_root: Path, dst_root: Path) -> None:
    for rel in paths:
        src = src_root / rel
        dst = dst_root / rel
        if not src.exists():
            continue
        if src.is_dir():
            _copytree(src, dst)
        else:
            _ensure_dir(dst.parent)
            shutil.copy2(src, dst)


def _copy_optional_paths(paths: Iterable[str], src_root: Path, dst_root: Path) -> list[str]:
    copied: list[str] = []
    for rel in paths:
        src = src_root / rel
        if not src.exists():
            continue
        dst = dst_root / rel
        if src.is_dir():
            _copytree(src, dst)
        else:
            _ensure_dir(dst.parent)
            shutil.copy2(src, dst)
        copied.append(rel)
    return copied


def _write_launcher(dst_root: Path) -> None:
    content = "#!/usr/bin/env bash\nset -euo pipefail\nROOT=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\nexec bash \"${ROOT}/scripts/run_portable_linux.sh\" \"$@\"\n"
    path = dst_root / "LAUNCH_PORTABLE.sh"
    path.write_text(content, encoding="utf-8")
    path.chmod(0o755)
    desktop_content = "#!/usr/bin/env bash\nset -euo pipefail\nROOT=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\nexec bash \"${ROOT}/scripts/run_desktop_pipeline.sh\" \"$@\"\n"
    desktop_path = dst_root / "LAUNCH_DESKTOP_GUI.sh"
    desktop_path.write_text(desktop_content, encoding="utf-8")
    desktop_path.chmod(0o755)


def _write_manifest(dst_root: Path, copied_optional: list[str]) -> None:
    payload = {
        "source_root": str(ROOT),
        "generated_at": datetime.now().isoformat(),
        "bundle_type": "portable_linux",
        "entrypoint": "LAUNCH_PORTABLE.sh",
        "project_label": "Final_portable",
        "copied_optional_paths": copied_optional,
        "notes": [
            "Portable bundle keeps Final workflow parity wherever the target Linux host provides matching runtime prerequisites.",
            "GPU drivers/runtime must exist on the target Linux host.",
        ],
    }
    path = dst_root / "portable" / "bundle_manifest.json"
    _ensure_dir(path.parent)
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=True), encoding="utf-8")


def _write_readme(dst_root: Path) -> None:
    (dst_root / "README_PORTABLE.md").write_text(PORTABLE_README, encoding="utf-8")


def _create_data_skeleton(dst_root: Path) -> None:
    for rel in [
        "data/runs",
        "data/optimization",
        "data/thesis_full",
        "data/health",
        "data/fem_option_validation",
        "data/ai_accel",
        "portable",
        ".cache/matplotlib",
        "analysis/figures/thesis_core",
        "analysis/figures/manifests",
    ]:
        _ensure_dir(dst_root / rel)


def main() -> None:
    ap = argparse.ArgumentParser(description="Build portable Linux bundle from Final.")
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT))
    ap.add_argument("--force", action="store_true")
    ap.add_argument(
        "--include-exact-assets",
        choices=["auto", "never", "always"],
        default="auto",
        help="Copy legacy/filip_exact_bundle and/or Kod Filipa if present.",
    )
    args = ap.parse_args()

    out_dir = Path(args.out_dir).expanduser().resolve()
    if out_dir.exists():
        if not args.force:
            raise SystemExit(f"[ERROR] Output already exists: {out_dir} (use --force)")
        shutil.rmtree(out_dir)

    out_dir.mkdir(parents=True, exist_ok=True)
    _copy_paths(DIRS_TO_COPY, ROOT, out_dir)
    _copy_paths(ROOT_FILES, ROOT, out_dir)
    copied_optional: list[str] = []
    if args.include_exact_assets != "never":
        copied_optional = _copy_optional_paths(OPTIONAL_DIRS, ROOT, out_dir)
        if args.include_exact_assets == "always":
            missing = [rel for rel in OPTIONAL_DIRS if rel not in copied_optional]
            if missing:
                raise SystemExit(
                    "[ERROR] Requested exact assets are missing: " + ", ".join(missing)
                )
    _create_data_skeleton(out_dir)
    _write_launcher(out_dir)
    _write_manifest(out_dir, copied_optional)
    _write_readme(out_dir)

    print(json.dumps({
        "status": "ok",
        "bundle_dir": str(out_dir),
        "entrypoint": str(out_dir / "LAUNCH_PORTABLE.sh"),
        "copied_optional_paths": copied_optional,
    }, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
