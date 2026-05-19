# GPU HIP (AMD ROCm)

Backend HIP jest ścieżką natywną dla kart AMD (Linux + ROCm).

Wymagania:
- Zainstalowane ROCm (`hipcc` w `PATH`)
- Zbudowana biblioteka: `gpu/hip/lib/build_hip.sh`

Benchmarki:
- `gpu/hip/benchmarks/run_hip_bandwidth.py`
- `gpu/hip/benchmarks/run_hip_pointer_latency.py`
- `gpu/hip/benchmarks/run_hip_compute_fma.py`
- `gpu/hip/benchmarks/run_hip_compute_fma_peak.py`

Typowe uruchomienie:
- `python3 run_all_gpu_benchmarks.py --platform-profile amd --backend auto --device-index 0`
