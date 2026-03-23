from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Tuple

from fem_catalog import bytes_per_elem_qp, flops_per_elem_qp
try:
    import cupy as cp  # type: ignore
except Exception:
    cp = None  # type: ignore

try:
    import cupyx.scipy.sparse as cpx_sparse  # type: ignore
except Exception:
    cpx_sparse = None  # type: ignore


@dataclass
class CudaRealBackend:
    device_index: int = 0

    def __post_init__(self) -> None:
        if cp is None:
            raise RuntimeError("CuPy not available (install cupy for CUDA real kernels).")
        cp.cuda.Device(self.device_index).use()
        self.device_name = cp.cuda.runtime.getDeviceProperties(self.device_index)["name"].decode("utf-8")

    @staticmethod
    def _prism_qps(n_qp: int) -> list[tuple[float, float, float, float]]:
        if n_qp <= 1:
            return [(1.0 / 3.0, 1.0 / 3.0, 0.0, 1.0)]
        tri = [
            (1.0 / 6.0, 1.0 / 6.0),
            (2.0 / 3.0, 1.0 / 6.0),
            (1.0 / 6.0, 2.0 / 3.0),
        ]
        line = [-1.0 / (3.0 ** 0.5), 1.0 / (3.0 ** 0.5)]
        qps = [(r, s, z, 1.0 / 6.0) for z in line for r, s in tri]
        return qps[: max(1, min(int(n_qp), len(qps)))]

    def gemm(self, m: int, n: int, k: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        a = cp.random.random((m, k), dtype=dt)
        b = cp.random.random((k, n), dtype=dt)

        # warmup
        _ = a @ b
        cp.cuda.Stream.null.synchronize()

        t0 = time.perf_counter()
        c = a @ b
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(m) * float(n) * float(k)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(c[0, 0].get())
        return elapsed, gflops

    def reduction(self, n: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        x = cp.random.random(n, dtype=dt)

        # warmup
        _ = cp.sum(x)
        cp.cuda.Stream.null.synchronize()

        t0 = time.perf_counter()
        s = cp.sum(x)
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_read = float(x.nbytes)
        gbps = bytes_read / max(elapsed, 1e-12) / 1e9
        _ = float(s.get())
        return elapsed, gbps

    def stencil2d(self, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        a = cp.random.random((h, w), dtype=dt)
        out = cp.zeros_like(a)

        # warmup
        out[1:-1, 1:-1] = (
            0.5 * a[1:-1, 1:-1]
            + 0.125 * a[:-2, 1:-1]
            + 0.125 * a[2:, 1:-1]
            + 0.125 * a[1:-1, :-2]
            + 0.125 * a[1:-1, 2:]
        )
        cp.cuda.Stream.null.synchronize()

        t0 = time.perf_counter()
        for _ in range(iters):
            out[1:-1, 1:-1] = (
                0.5 * a[1:-1, 1:-1]
                + 0.125 * a[:-2, 1:-1]
                + 0.125 * a[2:, 1:-1]
                + 0.125 * a[1:-1, :-2]
                + 0.125 * a[1:-1, 2:]
            )
            a, out = out, a
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float((h - 2) * (w - 2) * (5 + 1) * cp.dtype(dt).itemsize * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def spmv(self, n: int, nnz_per_row: int, dtype: str = "float32") -> Tuple[float, float]:
        if cpx_sparse is None:
            raise RuntimeError("cupyx.scipy.sparse is required for CUDA SPMV.")
        dt = cp.float32 if dtype == "float32" else cp.float64
        nnz_per_row = max(1, min(nnz_per_row, n))
        density = float(nnz_per_row) / float(n)
        mat = cpx_sparse.random(n, n, density=density, format="csr", dtype=dt)
        x = cp.random.random(n, dtype=dt)

        # warmup
        _ = mat @ x
        cp.cuda.Stream.null.synchronize()

        t0 = time.perf_counter()
        y = mat @ x
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = 2.0 * float(mat.nnz)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(y[0].get())
        return elapsed, gflops

    def stencil3d(self, d: int, h: int, w: int, iters: int, dtype: str = "float32") -> Tuple[float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        a = cp.random.random((d, h, w), dtype=dt)
        out = cp.zeros_like(a)

        out[1:-1, 1:-1, 1:-1] = (
            0.5 * a[1:-1, 1:-1, 1:-1]
            + (1.0 / 12.0) * a[:-2, 1:-1, 1:-1]
            + (1.0 / 12.0) * a[2:, 1:-1, 1:-1]
            + (1.0 / 12.0) * a[1:-1, :-2, 1:-1]
            + (1.0 / 12.0) * a[1:-1, 2:, 1:-1]
            + (1.0 / 12.0) * a[1:-1, 1:-1, :-2]
            + (1.0 / 12.0) * a[1:-1, 1:-1, 2:]
        )
        cp.cuda.Stream.null.synchronize()

        t0 = time.perf_counter()
        for _ in range(iters):
            out[1:-1, 1:-1, 1:-1] = (
                0.5 * a[1:-1, 1:-1, 1:-1]
                + (1.0 / 12.0) * a[:-2, 1:-1, 1:-1]
                + (1.0 / 12.0) * a[2:, 1:-1, 1:-1]
                + (1.0 / 12.0) * a[1:-1, :-2, 1:-1]
                + (1.0 / 12.0) * a[1:-1, 2:, 1:-1]
                + (1.0 / 12.0) * a[1:-1, 1:-1, :-2]
                + (1.0 / 12.0) * a[1:-1, 1:-1, 2:]
            )
            a, out = out, a
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        bytes_moved = float((d - 2) * (h - 2) * (w - 2) * 8 * cp.dtype(dt).itemsize * iters)
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        return elapsed, gbps

    def fem_element(self, n_elements: int, n_qp: int = 8, dtype: str = "float32") -> Tuple[float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        jac = cp.random.random((n_elements, 9), dtype=dt)
        coeff = cp.random.random((n_qp, 9), dtype=dt)
        out = cp.zeros((n_elements,), dtype=dt)

        cp.cuda.Stream.null.synchronize()
        t0 = time.perf_counter()
        for q in range(n_qp):
            out += cp.sum(jac * coeff[q], axis=1)
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        flops = float(n_elements * n_qp * (9 * 2))
        gflops = flops / max(elapsed, 1e-12) / 1e9
        _ = float(out[0].get())
        return elapsed, gflops

    def fem_integration_tet4(
        self,
        n_elements: int,
        n_qp: int = 4,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        op = operator.lower()
        if op not in (
            "diffusion",
            "mass",
            "convection",
            "diffusion_mass",
            "diffusion_convection_mass",
        ):
            raise ValueError(f"Unsupported operator: {operator}")
        x_e = cp.random.random((n_elements, 4, 3), dtype=dt)
        dN_ref = cp.array(
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
        ke = cp.zeros((n_elements, 4, 4), dtype=dt)
        vel = cp.random.random((n_elements, 3), dtype=dt)

        cp.cuda.Stream.null.synchronize()
        t0 = time.perf_counter()
        for xi, eta, zeta, w in qps:
            grad_phys = None
            j = cp.einsum("eni,nj->eij", x_e, dN_ref, optimize=True)
            det_j = cp.linalg.det(j)
            inv_j = cp.linalg.inv(j)
            scale = (w * cp.abs(det_j)).reshape(-1, 1, 1)

            if op in ("diffusion", "diffusion_mass", "diffusion_convection_mass"):
                grad_phys = cp.einsum("ni,eij->enj", dN_ref, inv_j, optimize=True)
                ke += scale * cp.einsum("eik,ejk->eij", grad_phys, grad_phys, optimize=True)

            if op in ("mass", "diffusion_mass", "diffusion_convection_mass"):
                nvals = cp.array([1.0 - xi - eta - zeta, xi, eta, zeta], dtype=dt)
                nn = cp.outer(nvals, nvals)[None, :, :]
                ke += scale * nn
            if op in ("convection", "diffusion_convection_mass"):
                if grad_phys is None:
                    grad_phys = cp.einsum("ni,eij->enj", dN_ref, inv_j, optimize=True)
                nvals = cp.array([1.0 - xi - eta - zeta, xi, eta, zeta], dtype=dt)
                ug = cp.einsum("ek,ejk->ej", vel, grad_phys, optimize=True)
                conv = nvals[None, :, None] * ug[:, None, :]
                ke += scale * conv
        cp.cuda.Stream.null.synchronize()
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
        bytes_per_elem_qp = float((4 * 3 + 4 * 4) * cp.dtype(dt).itemsize)
        flops = float(n_elements * qn) * flops_per_elem_qp
        bytes_moved = float(n_elements * qn) * bytes_per_elem_qp
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        _ = float(ke[0, 0, 0].get()) if n_elements > 0 else 0.0
        return elapsed, gflops, gbps

    def fem_integration_hex8(
        self,
        n_elements: int,
        n_qp: int = 8,
        operator: str = "diffusion_mass",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        op = operator.lower()
        if op not in (
            "diffusion",
            "mass",
            "convection",
            "diffusion_mass",
            "diffusion_convection_mass",
        ):
            raise ValueError(f"Unsupported operator: {operator}")

        x_e = cp.random.random((n_elements, 8, 3), dtype=dt)
        ke = cp.zeros((n_elements, 8, 8), dtype=dt)
        vel = cp.random.random((n_elements, 3), dtype=dt)
        signs = cp.array(
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
            g = 1.0 / (3.0 ** 0.5)
            base = [-g, g]
            qps = [(xi, eta, zeta, 1.0) for xi in base for eta in base for zeta in base]
            qps = qps[:n_qp]

        cp.cuda.Stream.null.synchronize()
        t0 = time.perf_counter()
        sx = signs[:, 0]
        sy = signs[:, 1]
        sz = signs[:, 2]
        for xi, eta, zeta, w in qps:
            grad_phys = None
            nvals = 0.125 * (1 + sx * xi) * (1 + sy * eta) * (1 + sz * zeta)
            dndxi = cp.stack(
                [
                    0.125 * sx * (1 + sy * eta) * (1 + sz * zeta),
                    0.125 * sy * (1 + sx * xi) * (1 + sz * zeta),
                    0.125 * sz * (1 + sx * xi) * (1 + sy * eta),
                ],
                axis=1,
            ).astype(dt)
            j = cp.einsum("eni,nj->eij", x_e, dndxi, optimize=True)
            det_j = cp.linalg.det(j)
            inv_j = cp.linalg.inv(j)
            scale = (w * cp.abs(det_j)).reshape(-1, 1, 1)
            if op in ("diffusion", "diffusion_mass", "diffusion_convection_mass"):
                grad_phys = cp.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                ke += scale * cp.einsum("eik,ejk->eij", grad_phys, grad_phys, optimize=True)
            if op in ("mass", "diffusion_mass", "diffusion_convection_mass"):
                nn = cp.outer(nvals, nvals)[None, :, :]
                ke += scale * nn
            if op in ("convection", "diffusion_convection_mass"):
                if grad_phys is None:
                    grad_phys = cp.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                ug = cp.einsum("ek,ejk->ej", vel, grad_phys, optimize=True)
                conv = nvals[None, :, None] * ug[:, None, :]
                ke += scale * conv
        cp.cuda.Stream.null.synchronize()
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
        bytes_per_elem_qp = float((8 * 3 + 8 * 8) * cp.dtype(dt).itemsize)
        flops = float(n_elements * qn) * flops_per_elem_qp
        bytes_moved = float(n_elements * qn) * bytes_per_elem_qp
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        _ = float(ke[0, 0, 0].get()) if n_elements > 0 else 0.0
        return elapsed, gflops, gbps

    def fem_integration_prism6(
        self,
        n_elements: int,
        n_qp: int = 6,
        operator: str = "laplace",
        dtype: str = "float32",
    ) -> Tuple[float, float, float]:
        dt = cp.float32 if dtype == "float32" else cp.float64
        op = operator.lower()
        if op not in (
            "diffusion",
            "mass",
            "convection",
            "diffusion_mass",
            "diffusion_convection_mass",
            "laplace",
            "test",
        ):
            raise ValueError(f"Unsupported operator: {operator}")

        x_e = cp.random.random((n_elements, 6, 3), dtype=dt)
        ke = cp.zeros((n_elements, 6, 6), dtype=dt)
        vel = cp.random.random((n_elements, 3), dtype=dt)
        coeff = cp.random.random((n_elements, 20), dtype=dt)
        eye6 = cp.eye(6, dtype=dt)[None, :, :]
        qps = self._prism_qps(n_qp)

        cp.cuda.Stream.null.synchronize()
        t0 = time.perf_counter()
        for r, s, z, w in qps:
            one_minus_z = dt(0.5 * (1.0 - z))
            one_plus_z = dt(0.5 * (1.0 + z))
            tri0 = dt(1.0 - r - s)
            nvals = cp.array(
                [
                    one_minus_z * tri0,
                    one_minus_z * r,
                    one_minus_z * s,
                    one_plus_z * tri0,
                    one_plus_z * r,
                    one_plus_z * s,
                ],
                dtype=dt,
            )
            dndxi = cp.array(
                [
                    [-one_minus_z, -one_minus_z, -0.5 * tri0],
                    [one_minus_z, dt(0.0), -0.5 * r],
                    [dt(0.0), one_minus_z, -0.5 * s],
                    [-one_plus_z, -one_plus_z, 0.5 * tri0],
                    [one_plus_z, dt(0.0), 0.5 * r],
                    [dt(0.0), one_plus_z, 0.5 * s],
                ],
                dtype=dt,
            )

            grad_phys = None
            j = cp.einsum("eni,nj->eij", x_e, dndxi, optimize=True)
            det_j = cp.linalg.det(j)
            inv_j = cp.linalg.inv(j)
            scale = (w * cp.abs(det_j)).reshape(-1, 1, 1)

            do_diff = op in ("diffusion", "diffusion_mass", "diffusion_convection_mass", "laplace", "test")
            do_mass = op in ("mass", "diffusion_mass", "diffusion_convection_mass", "test")
            do_conv = op in ("convection", "diffusion_convection_mass", "test")
            if do_diff:
                grad_phys = cp.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                diff_term = cp.einsum("eik,ejk->eij", grad_phys, grad_phys, optimize=True)
                if op == "test":
                    diff_scale = (1.0 + 0.25 * coeff[:, :6].mean(axis=1)).reshape(-1, 1, 1)
                    ke += scale * diff_scale * diff_term
                else:
                    ke += scale * diff_term
            if do_mass:
                nn = cp.outer(nvals, nvals)[None, :, :]
                if op == "test":
                    mass_scale = (0.55 + 0.10 * coeff[:, 6:12].mean(axis=1)).reshape(-1, 1, 1)
                    ke += scale * mass_scale * nn
                else:
                    ke += scale * nn
            if do_conv:
                if grad_phys is None:
                    grad_phys = cp.einsum("ni,eij->enj", dndxi, inv_j, optimize=True)
                ug = cp.einsum("ek,ejk->ej", vel, grad_phys, optimize=True)
                conv = nvals[None, :, None] * ug[:, None, :]
                if op == "test":
                    conv_scale = (0.60 + 0.10 * coeff[:, 12:18].mean(axis=1)).reshape(-1, 1, 1)
                    ke += scale * conv_scale * conv
                    diag_scale = (0.01 * coeff[:, 18:20].sum(axis=1)).reshape(-1, 1, 1)
                    ke += scale * diag_scale * eye6
                else:
                    ke += scale * conv
        cp.cuda.Stream.null.synchronize()
        t1 = time.perf_counter()

        elapsed = t1 - t0
        qn = len(qps)
        flops = float(n_elements * qn) * flops_per_elem_qp("prism6", op)
        bytes_moved = float(n_elements * qn) * bytes_per_elem_qp("prism6", dtype)
        gflops = flops / max(elapsed, 1e-12) / 1e9
        gbps = bytes_moved / max(elapsed, 1e-12) / 1e9
        _ = float(ke[0, 0, 0].get()) if n_elements > 0 else 0.0
        return elapsed, gflops, gbps
