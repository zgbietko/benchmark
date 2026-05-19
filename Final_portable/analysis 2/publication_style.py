from __future__ import annotations

from pathlib import Path
from typing import Iterable, Any
import math

ROOT = Path(__file__).resolve().parents[1]
FIGURES_DIR = ROOT / "analysis" / "figures"
THESIS_CORE_DIR = FIGURES_DIR / "thesis_core"
APPENDIX_DIR = FIGURES_DIR / "appendix"
MANIFEST_DIR = FIGURES_DIR / "manifests"


def ensure_figure_dirs() -> dict[str, Path]:
    for path in (FIGURES_DIR, THESIS_CORE_DIR, APPENDIX_DIR, MANIFEST_DIR):
        path.mkdir(parents=True, exist_ok=True)
    return {
        "figures": FIGURES_DIR,
        "thesis_core": THESIS_CORE_DIR,
        "appendix": APPENDIX_DIR,
        "manifests": MANIFEST_DIR,
    }


def clear_pngs(directory: Path) -> None:
    if not directory.exists():
        return
    for path in directory.glob("*.png"):
        try:
            path.unlink()
        except Exception:
            continue


def setup_publication_theme(plt: Any) -> None:
    plt.style.use("default")
    plt.rcParams.update(
        {
            "figure.facecolor": "white",
            "axes.facecolor": "white",
            "savefig.facecolor": "white",
            "font.family": "serif",
            "axes.titlesize": 12,
            "axes.labelsize": 10,
            "xtick.labelsize": 8,
            "ytick.labelsize": 8,
            "legend.fontsize": 8,
            "axes.grid": False,
            "grid.alpha": 0.18,
            "axes.spines.top": False,
            "axes.spines.right": False,
        }
    )


def backend_color(backend: str) -> str:
    key = str(backend or "").strip().lower()
    palette = {
        "cpu": "#1d4ed8",
        "metal": "#0f766e",
        "cuda": "#166534",
        "hip": "#b45309",
        "opencl": "#7c3aed",
        "unknown": "#475569",
    }
    return palette.get(key, "#475569")


def algorithm_color(name: str) -> str:
    key = str(name or "").strip().lower()
    palette = {
        "gemm": "#7c3aed",
        "spmv": "#dc2626",
        "fem": "#16a34a",
        "fem_integration": "#2563eb",
        "reduction": "#0891b2",
        "stencil2d": "#b45309",
        "stencil3d": "#ea580c",
        "matmul": "#7c3aed",
        "coreml_mlp_predict": "#0f766e",
        "mem_copy": "#1d4ed8",
        "stream": "#0f766e",
        "compute": "#7c3aed",
    }
    return palette.get(key, "#334155")


def operator_style(name: str) -> dict[str, Any]:
    key = str(name or "").strip().lower()
    if key == "laplace":
        return {"color": "#111111", "linestyle": (0, (1.2, 1.2)), "linewidth": 2.0}
    if key == "test":
        return {"color": "#b8bcc2", "linestyle": "-", "linewidth": 2.0}
    if key == "diffusion":
        return {"color": "#334155", "linestyle": "-", "linewidth": 1.9}
    if key == "diffusion_convection_mass":
        return {"color": "#0f766e", "linestyle": "-", "linewidth": 1.9}
    return {"color": "#334155", "linestyle": "-", "linewidth": 1.8}


def apply_axis_style(ax: Any, *, grid_axis: str = "y") -> None:
    ax.set_facecolor("#ffffff")
    if grid_axis in {"x", "both"}:
        ax.grid(True, axis="x", color="#d4d4d8", linewidth=0.55, alpha=0.7)
    if grid_axis in {"y", "both"}:
        ax.grid(True, axis="y", color="#d4d4d8", linewidth=0.55, alpha=0.7)
    for side in ("left", "bottom"):
        ax.spines[side].set_color("#71717a")
        ax.spines[side].set_linewidth(0.8)
    ax.tick_params(axis="both", colors="#27272a")


def add_platform_badge(fig: Any, label: str) -> None:
    text = str(label or "").strip()
    if not text:
        return
    fig.text(
        0.995,
        0.995,
        text,
        ha="right",
        va="top",
        fontsize=8.2,
        color="#475569",
        bbox={
            "boxstyle": "round,pad=0.24",
            "facecolor": "white",
            "edgecolor": "#cbd5e1",
            "alpha": 0.92,
        },
    )


def save_figure(fig: Any, out: Path, *, dpi: int = 220, platform_label: str = "") -> None:
    add_platform_badge(fig, platform_label)
    out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out, dpi=dpi, bbox_inches="tight")


def figure_entry(*, figure_id: str, section: str, path: Path, question: str, summary: str) -> dict[str, str]:
    return {
        "figure_id": figure_id,
        "section": section,
        "path": str(path),
        "filename": path.name,
        "question": question,
        "summary": summary,
    }


def finite(values: Iterable[float]) -> list[float]:
    out: list[float] = []
    for value in values:
        try:
            x = float(value)
        except Exception:
            continue
        if math.isfinite(x):
            out.append(x)
    return out


def padded_ylim(values: Iterable[float], *, lower_floor_zero: bool = False, pad_fraction: float = 0.08) -> tuple[float, float] | None:
    vals = finite(values)
    if not vals:
        return None
    lo = min(vals)
    hi = max(vals)
    span = max(hi - lo, max(abs(lo), abs(hi), 1.0) * pad_fraction, 1e-6)
    pad = span * pad_fraction
    lower = lo - pad
    upper = hi + pad
    if lower_floor_zero and lo >= 0.0:
        lower = max(0.0, lower)
    if lower >= upper:
        upper = lower + 1.0
    return lower, upper
