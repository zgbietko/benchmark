from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Tuple

import numpy as np


@dataclass
class CpuRealBackend:
    device_name: str = "cpu"

    def gemm(self, m: int, n: int, k: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        a = np.random.rand(m, k).astype(dt)
        b = np.random.rand(k, n).astype(dt)

        # warmup
        _ = a @ b

        t0 = time.perf_counter()
        c = a @ b
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(m) * float(n) * float(k)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(c[0, 0])
        return elapsed, gflops

    def reduction(self, n: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        x = np.random.rand(n).astype(dt)

        # warmup
        _ = np.sum(x)

        t0 = time.perf_counter()
        s = np.sum(x)
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_read = float(x.nbytes)
        gbps = bytes_read / max(elapsed, 1e-12) / 1e9
        _ = float(s)
        return elapsed, gbps

    def stencil2d(self, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        a = np.random.rand(h, w).astype(dt)
        out = np.zeros_like(a)

        def _step(src: np.ndarray, dst: np.ndarray) -> None:
            dst[1:-1, 1:-1] = (
                0.5 * src[1:-1, 1:-1]
                + 0.125 * src[:-2, 1:-1]
                + 0.125 * src[2:, 1:-1]
                + 0.125 * src[1:-1, :-2]
                + 0.125 * src[1:-1, 2:]
            )

        _step(a, out)
        a, out = out, a

        t0 = time.perf_counter()
        for _ in range(iters):
            _step(a, out)
            a, out = out, a
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float((h - 2) * (w - 2) * (5 + 1) * np.dtype(dt).itemsize * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def spmv(self, n: int, nnz_per_row: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        nnz_per_row = max(1, min(nnz_per_row, n))
        indptr = np.arange(0, (n + 1) * nnz_per_row, nnz_per_row, dtype=np.int64)
        indices = np.empty(n * nnz_per_row, dtype=np.int64)
        data = np.random.rand(n * nnz_per_row).astype(dt)
        for r in range(n):
            cols = np.random.choice(n, size=nnz_per_row, replace=False)
            indices[r * nnz_per_row : (r + 1) * nnz_per_row] = cols
        x = np.random.rand(n).astype(dt)
        y = np.zeros(n, dtype=dt)

        t0 = time.perf_counter()
        for r in range(n):
            s = dt(0.0)
            start = indptr[r]
            end = indptr[r + 1]
            for j in range(start, end):
                s += data[j] * x[indices[j]]
            y[r] = s
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(n * nnz_per_row)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(y[0])
        return elapsed, gflops

    def stencil3d(self, d: int, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        a = np.random.rand(d, h, w).astype(dt)
        out = np.zeros_like(a)

        def _step(src: np.ndarray, dst: np.ndarray) -> None:
            dst[1:-1, 1:-1, 1:-1] = (
                0.5 * src[1:-1, 1:-1, 1:-1]
                + (1.0 / 12.0) * src[:-2, 1:-1, 1:-1]
                + (1.0 / 12.0) * src[2:, 1:-1, 1:-1]
                + (1.0 / 12.0) * src[1:-1, :-2, 1:-1]
                + (1.0 / 12.0) * src[1:-1, 2:, 1:-1]
                + (1.0 / 12.0) * src[1:-1, 1:-1, :-2]
                + (1.0 / 12.0) * src[1:-1, 1:-1, 2:]
            )

        _step(a, out)
        a, out = out, a
        t0 = time.perf_counter()
        for _ in range(iters):
            _step(a, out)
            a, out = out, a
        t1 = time.perf_counter()
        elapsed = t1 - t0
        bytes_moved = float((d - 2) * (h - 2) * (w - 2) * 8 * np.dtype(dt).itemsize * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def fem_element(self, n_elements: int, n_qp: int = 8, dtype: str = "float32") -> Tuple[float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        jac = np.random.rand(n_elements, 9).astype(dt)
        coeff = np.random.rand(n_qp, 9).astype(dt)
        out = np.zeros((n_elements,), dtype=dt)

        t0 = time.perf_counter()
        for q in range(n_qp):
            out += np.sum(jac * coeff[q], axis=1)
        t1 = time.perf_counter()
        elapsed = t1 - t0
        flops = float(n_elements * n_qp * (9 * 2))
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(out[0])
        return elapsed, gflops

    def fem_integration_tet4(
        self,
        n_elements: int,
        n_qp: int = 4,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        """
        Tet4 element integration:
        K += w * detJ * (gradN_phys gradN_phys^T)    [diffusion]
        M += w * detJ * (N N^T)                      [mass]
        Returns (elapsed_s, gflops, gbps).
        """
        dt = np.float32 if dtype == "float32" else np.float64
        op = operator.lower()
        if op not in (
            "diffusion",
            "mass",
            "convection",
            "diffusion_mass",
            "diffusion_convection_mass",
        ):
            raise ValueError(f"Unsupported operator: {operator}")

        x_e = np.random.rand(n_elements, 4, 3).astype(dt)
        dN_ref = np.array(
            [
                [-1.0, -1.0, -1.0],
                [1.0, 0.0, 0.0],
                [0.0, 1.0, 0.0],
                [0.0, 0.0, 1.0],
            ],
            dtype=dt,
        )
        if n_qp <= 1:
            qps = [(0.25, 0.25, 0.25, 1.0 / 6.0)]
        else:
            a = 0.58541020
            b = 0.13819660
            qps = [
                (b, b, b, 1.0 / 24.0),
                (a, b, b, 1.0 / 24.0),
                (b, a, b, 1.0 / 24.0),
                (b, b, a, 1.0 / 24.0),
            ]
            qps = qps[:n_qp]

        ke = np.zeros((n_elements, 4, 4), dtype=dt)
        vel = np.random.rand(n_elements, 3).astype(dt)

        t0 = time.perf_counter()
        for xi, eta, zeta, w in qps:
            grad_phys = None
            j = np.einsum("eni,nj->eij", x_e, dN_ref, optimize=True)
            det_j = np.linalg.det(j)
            inv_j = np.linalg.inv(j)
            scale = (w * np.abs(det_j)).reshape(-1, 1, 1)

            if op in ("diffusion", "diffusion_mass", "diffusion_convection_mass"):
                grad_phys = np.einsum("ni,eij->enj", dN_ref, inv_j, optimize=True)
                ke += scale * np.einsum("eik,ejk->eij", grad_phys, grad_phys, optimize=True)

            if op in ("mass", "diffusion_mass", "diffusion_convection_mass"):
                nvals = np.array([1.0 - xi - eta - zeta, xi, eta, zeta], dtype=dt)
                nn = np.outer(nvals, nvals)[None, :, :]
                ke += scale * nn
            if op in ("convection", "diffusion_convection_mass"):
                if grad_phys is None:
                    grad_phys = np.einsum("ni,eij->enj", dN_ref, inv_j, optimize=True)
                nvals = np.array([1.0 - xi - eta - zeta, xi, eta, zeta], dtype=dt)
                ug = np.einsum("ek,ejk->ej", vel, grad_phys, optimize=True)  # [e,4]
                conv = nvals[None, :, None] * ug[:, None, :]  # [e,4,4]
                ke += scale * conv
        t1 = time.perf_counter()

        elapsed = t1 - t0
        qn = len(qps)
        if op == "diffusion":
            flops_per_elem_qp = 330.0
        elif op == "mass":
            flops_per_elem_qp = 120.0
        elif op == "convection":
            flops_per_elem_qp = 210.0
        elif op == "diffusion_mass":
            flops_per_elem_qp = 450.0
        else:
            flops_per_elem_qp = 660.0
        bytes_per_elem_qp = float((4 * 3 + 4 * 4) * np.dtype(dt).itemsize)
        flops = float(n_elements * qn) * flops_per_elem_qp
        bytes_moved = float(n_elements * qn) * bytes_per_elem_qp
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        _ = float(ke[0, 0, 0]) if n_elements > 0 else 0.0
        return elapsed, gflops, gbps

    def fem_integration_hex8(
        self,
        n_elements: int,
        n_qp: int = 8,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        dt = np.float32 if dtype == "float32" else np.float64
        op = operator.lower()
        if op not in (
            "diffusion",
            "mass",
            "convection",
            "diffusion_mass",
            "diffusion_convection_mass",
        ):
            raise ValueError(f"Unsupported operator: {operator}")

        x_e = np.random.rand(n_elements, 8, 3).astype(dt)
        ke = np.zeros((n_elements, 8, 8), dtype=dt)
        vel = np.random.rand(n_elements, 3).astype(dt)

        signs = np.array(
            [
                [-1, -1, -1],
                [1, -1, -1],
                [1, 1, -1],
                [-1, 1, -1],
                [-1, -1, 1],
                [1, -1, 1],
                [1, 1, 1],
                [-1, 1, 1],
            ],
            dtype=dt,
        )
        if n_qp <= 1:
            qps = [(0.0, 0.0, 0.0, 8.0)]
        else:
            g = 1.0 / np.sqrt(3.0)
            base = [-g, g]
            qps = [(xi, eta, zeta, 1.0) for xi in base for eta in base for zeta in base]
            qps = qps[:n_qp]

        t0 = time.perf_counter()
        for xi, eta, zeta, w in qps:
            grad_phys = None
            sx = signs[:, 0]
            sy = signs[:, 1]
            sz = signs[:, 2]
            nvals = 0.125 * (1 + sx * xi) * (1 + sy * eta) * (1 + sz * zeta)
            dndxi = np.stack(
                [
                    0.125 * sx * (1 + sy * eta) * (1 + sz * zeta),
                    0.125 * sy * (1 + sx * xi) * (1 + sz * zeta),
                    0.125 * sz * (1 + sx * xi) * (1 + sy * eta),
                ],
                axis=1,
            ).astype(dt)

            j = np.einsum("eni,nj->eij", x_e, dndxi, optimize=True)
            det_j = np.linalg.det(j)
            inv_j = np.linalg.inv(j)
            scale = (w * np.abs(det_j)).reshape(-1, 1, 1)

            if op in ("diffusion", "diffusion_mass", "diffusion_convection_mass"):
                grad_phys = np.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                ke += scale * np.einsum("eik,ejk->eij", grad_phys, grad_phys, optimize=True)
            if op in ("mass", "diffusion_mass", "diffusion_convection_mass"):
                nn = np.outer(nvals, nvals)[None, :, :]
                ke += scale * nn
            if op in ("convection", "diffusion_convection_mass"):
                if grad_phys is None:
                    grad_phys = np.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                ug = np.einsum("ek,ejk->ej", vel, grad_phys, optimize=True)  # [e,8]
                conv = nvals[None, :, None] * ug[:, None, :]  # [e,8,8]
                ke += scale * conv
        t1 = time.perf_counter()

        elapsed = t1 - t0
        qn = len(qps)
        if op == "diffusion":
            flops_per_elem_qp = 1200.0
        elif op == "mass":
            flops_per_elem_qp = 420.0
        elif op == "convection":
            flops_per_elem_qp = 820.0
        elif op == "diffusion_mass":
            flops_per_elem_qp = 1620.0
        else:
            flops_per_elem_qp = 2440.0
        bytes_per_elem_qp = float((8 * 3 + 8 * 8) * np.dtype(dt).itemsize)
        flops = float(n_elements * qn) * flops_per_elem_qp
        bytes_moved = float(n_elements * qn) * bytes_per_elem_qp
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        _ = float(ke[0, 0, 0]) if n_elements > 0 else 0.0
        return elapsed, gflops, gbps
