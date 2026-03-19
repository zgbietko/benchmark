## CSV schema (v2 – canonical)

Ten dokument opisuje **kanoniczny** schemat kolumn w plikach CSV generowanych przez
`apple_microbench_variant2_streamfix/v2`. Starsze pola (legacy) mogą dalej istnieć
dla kompatybilności, ale **analiza i publikacje powinny używać tylko pól kanonicznych**.

### 1. Pola wspólne (CPU / GPU / real_kernels)

- **timestamp**: ISO 8601, `YYYY-MM-DDTHH:MM:SS`, lokalny czas hosta.
- **system**: wartość z `platform.system()` (np. `Linux`, `Darwin`, `Windows`).
- **node**: hostname (`platform.node()`).
- **release**: `platform.release()`.
- **version**: `platform.version()`.
- **machine** / **arch**: architektura (`x86_64`, `arm64`, ...).
- **processor** / **cpu_model**: opis modelu CPU.
- **python_version**: wersja Pythona.
- **benchmark**: identyfikator benchmarku (np. `bandwidth`, `stream`, `compute_fma`,
  `compute_fma_peak`, `gpu_bandwidth`, `gpu_compute_fma`, `gemm`, `stencil2d`, `spmv`, ...).
- **run_id**: numer powtórzenia w obrębie danego zestawu parametrów (0..N-1).
- **elapsed_s**: czas trwania mierzonej części runu [sekundy].

### 2. CPU – mikrobenchmarki

#### 2.1 Bandwidth / STREAM

- **size_mb**: rozmiar danych per-array [MiB] (logiczny, niekoniecznie fizycznie
  zmapowany w RAM).
- **n_elements** (opcjonalne): liczba elementów typu `float32`.
- **bytes_per_iter**: liczba bajtów przetwarzanych w jednej iteracji pętli.
- **iters**: liczba iteracji pętli wewnętrznej.
- **kernel** (STREAM): `copy` | `scale` | `add` | `triad`.
- **threads** (dla wariantów *_mt): liczba wątków pracera.
- **throughput_gbps**: przepustowość pamięci [GB/s], zawsze liczona względem `1e9`.

Legacy:
- `gbps` – alias na `throughput_gbps` (w v2 utrzymywany, ale kanoniczne pole to `throughput_gbps`).

#### 2.2 Pointer latency

- **working_set_kb**: rozmiar working setu [KiB].
- **latency_ns**: średnia latencja jednego kroku pointer-chasing [ns].

#### 2.3 Compute FMA

- **vector_len** / **n**: długość wektora.
- **iters_inner**: liczba iteracji wewnętrznych.
- **gflops**: osiągnięta wydajność [GFLOP/s].

#### 2.4 Peak FMA

Dodatkowe pola:

- **threads** / **num_threads**: liczba wątków.
- **n_per_thread**: liczba elementów na wątek.
- **gflops** lub:
  - **gflops_peak**
  - **gflops_mean**
  - **gflops_sigma**

### 3. GPU – mikrobenchmarki

Obowiązkowe pola identyfikacji:

- **backend**: `metal` | `cuda` | `hip` | `opencl` | inne.
- **gpu_model**: opis GPU (np. `apple_m2_pro`, `NVIDIA GeForce RTX 3070 Ti Laptop GPU`).
- **size_bytes**: liczba bajtów przetwarzanych w jednym „bloku” danych.
- **iters_inner** / **inner_iters** (jeśli dotyczy).
- **transfer_kind** (bandwidth): `device_to_device` | `host_to_device` | `device_to_host`.
- **memory_mode** (opcjonalne): np. `pinned`, `pageable`, `managed`.
- **copy_method** (opcjonalne): np. `memcpy`, `kernel`.

Metryki:

- **throughput_gbps**: przepustowość [GB/s], liczona względem `1e9`.
- **latency_ns**: pointer-chasing latency [ns] (jeśli dotyczy).
- **gflops** / **throughput_gflops**: GFLOP/s.
- **gflops_peak**, **gflops_mean**, **gflops_sigma** – dla peak FMA.

Legacy:
- `gbps` – alias na `throughput_gbps`.

### 4. real_kernels

Minimalny wspólny zestaw:

- **benchmark**: `gemm` | `reduction` | `stencil2d` | `stencil3d` | `spmv` | `fem` | `fem_integration`.
- **backend**: `cpu` | `cuda` | `metal` (lub inne).
- **problem_size**: liczbowy opis wielkości problemu (np. liczba elementów, NNZ, itp.).
- **shape** (opcjonalne): zapis rozmiarów, np. `MxNxK`.
- **elapsed_s**: czas całkowity kerneLa / pętli benchmarku.
- **throughput_gbps** lub **gflops** – w zależności od natury problemu.
- **status**: `ok` | `not_implemented` | `error:<...>`.

### 5. Energia i moc

Pola standardowe (jeśli energia jest mierzona):

- **energy_j**: energia pobrana w trakcie runu [J].
- **avg_power_w**: średnia moc [W] (energia / czas).
- **energy_source**: źródło pomiaru, np.:
  - `rapl`
  - `powermetrics`
  - `nvml`
  - `nvidia_smi`
  - `sysfs`
  - `powermetrics_gpu`
  - `fallback_zero` / `unsupported_*` / `no_gpu_energy_backend`
- **energy_supported**: `0` / `1` – czy pomiar energii był realnie dostępny.
- **energy_samples**: liczba próbek mocy użytych do integracji (0 jeśli tylko fallback).
- **energy_confidence**: heurystyczny poziom zaufania 0.0–1.0.

Pochodne (opcjonalne, ale rekomendowane):

- **j_per_gb**: energia na 1 GB przetworzonych danych [J/GB].
- **j_per_gflop**: energia na 1 GFLOP [J/GFLOP].
- **edp**: Energy-Delay Product (energia * czas) [J·s].

### 6. Uwagi dot. zgodności wstecznej (v1 → v2)

- Skrypty analityczne w v2 powinny **czytać zarówno pola legacy, jak i kanoniczne**, ale
  wszystkie *nowe* benchmarki powinny już zapisywać tylko schemat kanoniczny.
- Dla istniejących benchmarków, rozszerzanie o nowe kolumny (np. `energy_source`,
  `energy_supported`, `energy_confidence`) jest preferowane zamiast zmiany nazw
  istniejących kolumn.

