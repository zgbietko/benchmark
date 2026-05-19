# GPU OpenCL (cross-vendor)

Backend OpenCL jest wspólnym mianownikiem do porównań między architekturami GPU (NVIDIA, AMD, Intel, iGPU).

Wymagania:
- Zainstalowane sterowniki OpenCL
- Python: `pyopencl`

Benchmarki:
- `gpu/opencl/benchmarks/run_opencl_bandwidth.py`
- `gpu/opencl/benchmarks/run_opencl_pointer_latency.py`
- `gpu/opencl/benchmarks/run_opencl_compute_fma.py`
- `gpu/opencl/benchmarks/run_opencl_compute_fma_peak.py`

Typowe uruchomienie:
- `python3 run_all_gpu_benchmarks.py --platform-profile intel_arc --backend auto --device-index 0`
- `python3 run_all_gpu_benchmarks.py --platform-profile intel_igpu --backend auto --device-index 0`
- `python3 run_all_gpu_benchmarks.py --platform-profile amd --backend opencl --device-index 0`
