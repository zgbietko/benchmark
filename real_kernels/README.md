# real_kernels

Moduł `real_kernels` jest oddzielony od mikrobenchmarków (`cpu/`, `gpu/`).

Zakres (MVP):
- `GEMM`
- `reduction`
- `stencil2d`
- `stencil3d`
- `spmv`
- `fem` (prosty kernel integracji elementów)
- `fem_integration` (tet4, quadrature-based, bliżej klasycznego całkowania FEM)

Backendy:
- `cpu` (NumPy)
- `cuda` (CuPy, jeśli dostępne)
- `metal` (MSL + PyObjC Metal)

Uruchamianie:
- `python3 real_kernels/run_all_real_kernels.py --backend all`
- `python3 real_kernels/benchmarks/run_gemm.py --backend cpu`
- `python3 real_kernels/benchmarks/run_reduction.py --backend cpu`
- `python3 real_kernels/benchmarks/run_stencil2d.py --backend cpu`
- `python3 real_kernels/benchmarks/run_stencil3d.py --backend cpu`
- `python3 real_kernels/benchmarks/run_spmv.py --backend cpu`
- `python3 real_kernels/benchmarks/run_fem.py --backend cpu`
- `python3 real_kernels/benchmarks/run_fem_integration.py --backend cpu --element-type tet4 --operator diffusion_convection_mass --sizes 20000,100000 --n-qp 4`
- `python3 real_kernels/benchmarks/run_fem_integration.py --backend cuda --element-type hex8 --operator diffusion --sizes 20000,100000 --n-qp 8`
- `python3 real_kernels/benchmarks/run_fem_integration.py --backend metal --element-type tet4 --operator diffusion_mass --sizes 20000,100000 --n-qp 4`
- `python3 real_kernels/benchmarks/run_fem_integration.py --backend opencl --element-type tet4 --operator diffusion_mass --sizes 20000,100000 --n-qp 4`

Uruchomienie jako benchmark opcjonalny w kampanii:
- `python3 real_kernels/run_all_real_kernels.py --backend all --with-fem-integration`

Uwagi:
- `fem_integration` wspiera backendy `cpu`, `cuda`, `metal`, `hip`, `opencl`.
- aliasy vendorowe dla benchmarku: `amd` (HIP->OpenCL fallback), `intel` (OpenCL).

Wyniki:
- `data/real_kernels/*.csv`

Podsumowanie:
- `python3 analysis/real_kernels_summary.py`
