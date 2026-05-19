from __future__ import annotations

import csv
import json
import math
from pathlib import Path
from typing import Any

import numpy as np


def _shape_from_entries(entries_per_element: int) -> int | None:
    if entries_per_element <= 0:
        return None
    disc = 1 + 4 * int(entries_per_element)
    root = math.isqrt(disc)
    if root * root != disc:
        return None
    if (root - 1) % 2 != 0:
        return None
    nshape = (root - 1) // 2
    if nshape <= 0:
        return None
    return nshape if nshape * nshape + nshape == int(entries_per_element) else None


def infer_scalar_matrix_rhs_layout(flat_count: int, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
    meta = dict(metadata or {})
    nr_elems = 0
    for key in ("nr_elems_this_kercall", "nr_elems_per_kernel", "nr_elems", "n_elements_hint"):
        try:
            nr_elems = int(meta.get(key, 0) or 0)
        except Exception:
            nr_elems = 0
        if nr_elems > 0:
            break
    entries_per_element = 0
    if nr_elems > 0 and flat_count % nr_elems == 0:
        entries_per_element = flat_count // nr_elems
    elif flat_count % 42 == 0:
        nr_elems = flat_count // 42
        entries_per_element = 42
    else:
        out_count = int(meta.get("el_data_out_count", 0) or 0)
        if out_count > 0 and flat_count == out_count and nr_elems > 0 and out_count % nr_elems == 0:
            entries_per_element = out_count // nr_elems
    nshape = _shape_from_entries(entries_per_element)
    if nr_elems <= 0 or entries_per_element <= 0 or nshape is None:
        return {}
    return {
        "layout": "matrix_plus_rhs_scalar",
        "n_elements": int(nr_elems),
        "entries_per_element": int(entries_per_element),
        "num_shap": int(nshape),
        "matrix_entries": int(nshape * nshape),
        "rhs_entries": int(nshape),
    }


def decode_scalar_matrix_rhs(arr: np.ndarray, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
    flat = np.ascontiguousarray(arr, dtype=np.float32).reshape(-1)
    layout = infer_scalar_matrix_rhs_layout(int(flat.size), metadata)
    if not layout:
        return {}
    num_shap = int(layout["num_shap"])
    n_elements = int(layout["n_elements"])
    entries_per_element = int(layout["entries_per_element"])
    blocks = flat.reshape(n_elements, entries_per_element)
    matrices = blocks[:, : num_shap * num_shap].reshape(n_elements, num_shap, num_shap)
    rhs = blocks[:, num_shap * num_shap :].reshape(n_elements, num_shap)
    first_matrix = matrices[0]
    first_rhs = rhs[0]
    return {
        **layout,
        "matrix_first_element": first_matrix,
        "rhs_first_element": first_rhs,
        "matrix_trace_first": float(np.trace(first_matrix)),
        "matrix_l2_first": float(np.linalg.norm(first_matrix)),
        "rhs_l2_first": float(np.linalg.norm(first_rhs)),
        "global_l2": float(np.linalg.norm(flat)),
    }


def _write_matrix_csv(path: Path, matrix: np.ndarray) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        width = int(matrix.shape[1]) if matrix.ndim == 2 else int(matrix.size)
        writer.writerow([f"c{idx}" for idx in range(width)])
        if matrix.ndim == 2:
            for row in matrix:
                writer.writerow([f"{float(v):.9g}" for v in row.tolist()])
        else:
            writer.writerow([f"{float(v):.9g}" for v in matrix.reshape(-1).tolist()])


def _write_vector_csv(path: Path, vec: np.ndarray) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "value"])
        for idx, value in enumerate(np.ascontiguousarray(vec, dtype=np.float32).reshape(-1)):
            writer.writerow([idx, f"{float(value):.9g}"])


def write_decoded_output(output_dir: Path, arr: np.ndarray, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
    decoded = decode_scalar_matrix_rhs(arr, metadata)
    if not decoded:
        return {}
    output_dir = Path(output_dir)
    matrix_path = output_dir / "stiffness_matrix__elem000.csv"
    rhs_path = output_dir / "rhs_vector__elem000.csv"
    summary_path = output_dir / "decoded_output_summary.json"
    _write_matrix_csv(matrix_path, decoded["matrix_first_element"])
    _write_vector_csv(rhs_path, decoded["rhs_first_element"])
    serializable = {
        key: value
        for key, value in decoded.items()
        if key not in {"matrix_first_element", "rhs_first_element"}
    }
    serializable["matrix_first_element_preview"] = [
        [float(v) for v in row]
        for row in np.asarray(decoded["matrix_first_element"], dtype=np.float32).tolist()
    ]
    serializable["rhs_first_element_preview"] = [
        float(v) for v in np.asarray(decoded["rhs_first_element"], dtype=np.float32).tolist()
    ]
    summary_path.write_text(json.dumps(serializable, indent=2, ensure_ascii=True), encoding="utf-8")
    return {
        "available": True,
        "summary_path": str(summary_path),
        "matrix_csv": str(matrix_path),
        "rhs_csv": str(rhs_path),
        "layout": str(decoded["layout"]),
        "n_elements": int(decoded["n_elements"]),
        "num_shap": int(decoded["num_shap"]),
    }
