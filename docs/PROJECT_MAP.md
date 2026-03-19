# apple_microbench – Project Map (variant 2)

To repo jest zorganizowane w 4 główne obszary:

## 1) CPU mikrobenchmarki (C + Python driver)
- `cpu/lib/`
  - `microbench.c` (+ nagłówki)
  - `build_linux.sh`, `build_mac.sh` → `libmicrobench.so` / `libmicrobench.dylib`
- `cpu/benchmarks/`
  - `run_bandwidth.py` – single-thread bandwidth
  - `run_bandwidth_mt.py` – multi-thread bandwidth
  - `run_pointer_latency.py` – pointer-chasing latency
  - `run_compute_fma.py` – single-thread FMA
  - `run_compute_fma_peak.py` – peak FMA vs threads
- Wyniki: `data/cpu/*.csv`
- Orkiestracja: `run_all_cpu_benchmarks.py`

## 2) GPU mikrobenchmarki (Metal / CUDA / HIP / OpenCL)
Wspólny schemat wyników:
`data/gpu/{benchmark}__backend-{backend}__gpu-{gpu_slug}__dev{device}.csv`

### Metal (macOS / Apple GPU)
- `gpu/metal/metal_backend.py` – PyObjC + Metal
- `gpu/metal/kernels.metal`
- `gpu/metal/benchmarks/`
  - `run_metal_bandwidth.py`
  - `run_metal_pointer_latency.py`
  - `run_metal_compute_fma.py`
  - `run_metal_compute_fma_peak.py`

### CUDA (Linux / NVIDIA)
- `gpu/cuda/lib/gpubench.cu` + `build_cuda.sh` → `libgpubench_cuda.so`
- `gpu/cuda/cuda_backend.py` (ctypes)
- `gpu/cuda/benchmarks/`
  - `run_cuda_bandwidth.py`
  - `run_cuda_pointer_latency.py`
  - `run_cuda_compute_fma.py`
  - `run_cuda_compute_fma_peak.py`

### HIP (Linux / AMD ROCm)
- `gpu/hip/lib/gpubench_hip.cu` + `build_hip.sh` → `libgpubench_hip.so`
- `gpu/hip/hip_backend.py` (ctypes)
- `gpu/hip/benchmarks/`
  - `run_hip_bandwidth.py`
  - `run_hip_pointer_latency.py`
  - `run_hip_compute_fma.py`
  - `run_hip_compute_fma_peak.py`

### OpenCL (cross-vendor: NVIDIA/AMD/Intel/Arc/iGPU)
- `gpu/opencl/opencl_backend.py` (PyOpenCL)
- `gpu/opencl/benchmarks/`
  - `run_opencl_bandwidth.py`
  - `run_opencl_pointer_latency.py`
  - `run_opencl_compute_fma.py`
  - `run_opencl_compute_fma_peak.py`

Orkiestracja: `run_all_gpu_benchmarks.py` (backend auto/metal/cuda/hip/opencl/all)  
Dodatkowo: `--platform-profile auto|apple|nvidia|amd|intel_arc|intel_igpu`.

## 3) Energia/moc
- `energy.py` – CPU (RAPL/powermetrics) używane głównie przez benchmarki CPU.
- `energy_utils.py` – uniwersalny logger (CPU i GPU):
  - GPU (Linux):
    - NVML (`pynvml`) dla NVIDIA
    - sysfs/hwmon dla AMD / część iGPU
  - Jeśli brak wsparcia: energia = NaN, `energy_source="unavailable"`

## 4) Analiza wyników
- `analysis/cpu_summary.py`
- `analysis/gpu_summary.py`
- `analysis/data_quality.py` (scope `auto|global|session`, strict gate + per-backend GPU quality)
- `analysis/generate_plots.py` (w tym `gpu_pointer_latency.png`)

## Legacy
- `legacy/` – stare wersje / artefakty przeniesione, żeby repo było „czyste”.
