# Firefly Autotuner Infrastructure (CPU/GPU Microbench)

## 1. Co już było w projekcie i co zostało wykorzystane

### 1.1 Reużywalne elementy wejścia/wyjścia
- **CLI benchmarków**: istniejące skrypty (`run_*`) mają jawne parametry (`--sizes-mb`, `--iters`, `--runs`, `--device-index`).
- **Backendy GPU**:
  - CUDA: `gpu/cuda/cuda_backend.py`
  - HIP: `gpu/hip/hip_backend.py`
  - Metal: `gpu/metal/metal_backend.py`
  - OpenCL: `gpu/opencl/opencl_backend.py`
- **Wspólny zapis wyników**: CSV dla `data/gpu/*` i `data/cpu/*`.
- **Energia/moc**: `energy_utils.py` z `EnergyLogger(domain="gpu"|"cpu")`.

### 1.2 Reużywalne metryki
- pamięć: `throughput_gbps` / `gbps`, `elapsed_s`
- compute: `gflops`, `elapsed_s`
- energia: `energy_joule` / `energy_j`, `avg_power_watt` / `avg_power_w`
- jakość energii: `energy_supported`, `energy_samples`, `energy_confidence`

### 1.3 Co było brakujące (doprojektowane od zera)
- ogólna reprezentacja **mieszanej przestrzeni decyzji** (continuous/int/categorical)
- ogólny **silnik metaheurystyki** (FA) niezależny od benchmarku
- warstwa **problem = przestrzeń + ewaluacja + ograniczenia**
- warstwa **celu** (single-objective weighted i multi-objective Pareto)
- spójna historia optymalizacji (iteracje + każda ewaluacja + wynik końcowy)

---

## 2. Architektura nowej warstwy FA

Nowe pliki:
- `optimization/search_space.py`
- `optimization/problem.py`
- `optimization/objectives.py`
- `optimization/firefly.py`
- `optimization/problems/gpu_adapters.py`
- `optimization/problems/gpu_memory_problem.py`
- `optimization/problems/gpu_fma_problem.py`
- `optimization/problems/fem_integration_problem.py`
- `optimization/problems/fem_parametric_problem.py`
- `optimization/problems/__init__.py`
- `optimization/__init__.py`
- `run_firefly_optimization.py`
- `run_fem_parametric_preflight.py`
- `run_fem_parametric_matrix.py`
- `run_autotune_gui.py`

### 2.1 Rozdzielenie odpowiedzialności
- `SearchSpace`: kodowanie/dekodowanie pozycji świetlika do konfiguracji benchmarku
- `OptimizationProblem`: interfejs problemu (`evaluate(config)`)
- `BrightnessModel`:
  - `WeightedSumBrightness`
  - `ParetoBrightness`
- `FireflyOptimizer`: populacja, ruch, perturbacja, iteracje, logi
- `GpuMemoryProblem` / `GpuFmaProblem` / `FemIntegrationProblem` / `FemParametricProblem`: implementacje konkretnych problemów

---

## 3. Kodowanie zmiennych i konfiguracji

Wspierane typy:
- `ContinuousVariable`
- `IntegerVariable`
- `CategoricalVariable`

Wewnętrznie pozycja świetlika jest trzymana w hipersześcianie `[0,1]^D`; każdy wymiar ma własne `decode()` do wartości fizycznej.

To daje:
- wspólną geometrię dla FA,
- łatwe dodawanie nowych zmiennych,
- jednolite ograniczanie i naprawę pozycji (`clip_position`).

---

## 4. Ograniczenia i walidacja

W nowych problemach dodano ograniczenia:
- **minimalny czas runu** (`min_elapsed_s`) – filtruje konfiguracje zdominowane przez overhead,
- **stabilność** (`CV <= max_cv`) – filtruje niestabilne pomiary,
- **poprawność metryk** (np. `gbps_mean > 0`, `gflops_mean > 0`).

Każda ewaluacja zwraca:
- `status` (`ok` / `error`),
- `constraints_ok`,
- `violations` (lista powodów odrzucenia),
- słownik metryk.

---

## 5. Funkcje celu (single i multi-objective)

### 5.1 Weighted (single-objective przez scalarizację)
`WeightedSumBrightness`:
- normalizacja metryk per-population,
- obsługa `max` i `min`,
- sumowanie ważone.

Przykład:
- pamięć: `gbps_mean:max:1.0,j_per_gb:min:0.2`
- compute: `gflops_mean:max:1.0,j_per_gflop:min:0.3,edp:min:0.2`

### 5.2 Pareto (multi-objective)
`ParetoBrightness`:
- rankingi Pareto (dominance fronts),
- crowding-distance (utrzymanie różnorodności),
- mapowanie rank+crowding do jasności dla ruchu świetlików.

---

## 6. Logowanie i historia optymalizacji

Każdy run FA zapisuje w `data/optimization/<timestamp>__<problem>__backend-<backend>/`:
- `evaluations.jsonl` – każda ewaluacja świetlika (konfiguracja + metryki + ograniczenia + brightness)
- `iterations.jsonl` – podsumowanie iteracji
- `summary.json` – najlepsza konfiguracja + metryki + przybliżony Pareto front

To jest gotowe pod dalszą analizę w `analysis/` i notebookach.

---

## 7. Problemy zaimplementowane teraz

### 7.1 `GpuMemoryProblem`
Zmienne:
- `transfer_kind` (categorical)
- `size_mb` (continuous)
- `iters_inner` (integer)

Metryki:
- `gbps_mean`, `gbps_sigma`, `cv_gbps`
- `energy_j_mean`, `power_w_mean`, `j_per_gb`

### 7.2 `GpuFmaProblem`
Zmienne:
- `n_elements_m` (continuous, mln elementów)
- `iters_inner` (integer)

Metryki:
- `gflops_mean`, `gflops_sigma`, `cv_gflops`
- `energy_j_mean`, `power_w_mean`, `j_per_gflop`, `edp`
- opcjonalnie roofline: `roofline_attainable_gflops`, `roofline_gap_abs`

### 7.3 `FemIntegrationProblem`
Zmienne:
- `n_elements` (integer)
- `n_qp` (integer)
- `element_type` (categorical: `tet4`/`hex8`)
- `operator` (categorical: `diffusion`, `mass`, `convection`, `diffusion_mass`, `diffusion_convection_mass`)
- `dtype` (categorical: `float32`/`float64`)

Metryki:
- `elapsed_s_mean`
- `gflops_mean`, `gflops_sigma`, `cv_gflops`
- `gbps_mean`, `gbps_sigma`, `cv_gbps`
- `energy_j_mean`, `power_w_mean`, `j_per_gflop`, `j_per_gb`, `edp`

Uwaga:
- problem stroi `real_kernels/fem_integration` dla backendów `cpu`, `cuda`, `metal`, `hip`, `opencl` (oraz aliasów `amd`/`intel`);
- dla backendów bez bezpośredniego jądra integracji używany jest kontrakt `mapped_native` (backendowe prymitywy pamięć+FMA), bez trybu `surrogate` w `fem_parametric`.

### 7.4 `FemParametricProblem` (QSS/SQS/SSQ + mapping wieloplatformowy)
Pełny zestaw parametrów strojenia (zgodny funkcjonalnie z doktoratem):
- `algorithm_variant`: `qss`, `sqs`, `ssq`
- `workgroup_size`
- `use_workspace_for_pde_coeff`
- `use_workspace_for_geo_data`
- `use_workspace_for_shape_fun`
- `use_workspace_for_stiff_mat`
- `padding`
- `compute_all_shape_fun_der`
- `coal_read`
- `coal_write`

Dodatkowe parametry problemu:
- `n_elements`, `n_qp`, `element_type`, `operator`, `dtype`

Tryby wykonania:
- `native`:
  - `cpu`, `cuda`, `metal` (kontrakt natywny `fem_integration`)
- `mapped_native`:
  - `hip`, `opencl`, aliasy `amd`, `intel`, oraz fallback dla backendów bez pełnego kernela
  - parametry bez odpowiednika 1:1 są mapowane na najbliższe kontrolki wykonania.

Dodatkowo:
- `execution_policy=native_only` (domyślne) wymusza tylko ścieżki kontraktowe (`native` lub `mapped_native`);
- legalizacja konfiguracji per urządzenie przycina niedozwolone wartości (np. `workgroup_size`, `dtype`);
- przed uruchomieniem kandydat przechodzi twardy check budżetu pamięci.

---

## 8. Scenariusze eksperymentów (gotowe)

### 8.1 Memory tuning (CUDA)
```bash
python3 run_firefly_optimization.py \
  --problem gpu_memory \
  --backend cuda \
  --device-index 0 \
  --population 20 \
  --iterations 30 \
  --repeats 5 \
  --size-mb-range 4:2048 \
  --iters-range 5:300 \
  --objective-mode weighted \
  --objectives "gbps_mean:max:1.0,j_per_gb:min:0.2"
```

### 8.2 FMA energy-aware (HIP, Pareto)
```bash
python3 run_firefly_optimization.py \
  --problem gpu_fma \
  --backend hip \
  --device-index 0 \
  --population 24 \
  --iterations 40 \
  --repeats 5 \
  --n-elements-m-range 0.25:32.0 \
  --iters-inner-range 200:20000 \
  --objective-mode pareto \
  --objectives "gflops_mean:max:1.0,j_per_gflop:min:1.0,edp:min:0.5"
```

### 8.3 Roofline-aware compute tuning
```bash
python3 run_firefly_optimization.py \
  --problem gpu_fma \
  --backend opencl \
  --device-index 0 \
  --roofline-peak-gflops 9000 \
  --roofline-peak-bw-gbps 220 \
  --arithmetic-intensity 8 \
  --objective-mode weighted \
  --objectives "gflops_mean:max:1.0,roofline_gap_abs:min:0.6,j_per_gflop:min:0.2"
```

### 8.4 FEM integration tuning (CPU/CUDA/Metal/HIP/OpenCL)
```bash
python3 run_firefly_optimization.py \
  --problem fem_integration \
  --backend cpu \
  --population 20 \
  --iterations 30 \
  --repeats 5 \
  --fem-n-elements-range 20000:500000 \
  --fem-n-qp-range 1:8 \
  --fem-element-types tet4,hex8 \
  --fem-operators diffusion,mass,convection,diffusion_mass,diffusion_convection_mass \
  --fem-dtypes float32,float64 \
  --objective-mode weighted \
  --objectives "gflops_mean:max:1.0,cv_gflops:min:0.3,edp:min:0.2"
```

### 8.5 FEM parametric tuning (pełny zestaw opcji)
```bash
python3 run_firefly_optimization.py \
  --problem fem_parametric \
  --backend cuda \
  --population 24 \
  --iterations 40 \
  --repeats 5 \
  --fem-n-elements-range 20000:500000 \
  --fem-n-qp-range 1:8 \
  --fem-element-types tet4,hex8 \
  --fem-operators diffusion,mass,convection,diffusion_mass,diffusion_convection_mass \
  --fem-dtypes float32 \
  --fem-variant-choices qss,sqs,ssq \
  --fem-workgroup-sizes 32,64,128,256 \
  --fem-use-workspace-pde-choices 0,1 \
  --fem-use-workspace-geo-choices 0,1 \
  --fem-use-workspace-shape-choices 0,1 \
  --fem-use-workspace-stiff-choices 0,1 \
  --fem-padding-choices 0,1 \
  --fem-compute-all-shape-der-choices 0,1 \
  --fem-coal-read-choices 0,1 \
  --fem-coal-write-choices 0,1 \
  --objective-mode weighted \
  --objectives "gflops_mean:max:1.0,j_per_gflop:min:0.3,cv_gflops:min:0.2,mapping_score:max:0.05"
```

Tryb oszczędny pamięci/czasu (szczególnie dla `metal`/`opencl`):
```bash
python3 run_firefly_optimization.py \
  --problem fem_parametric \
  --backend metal \
  --fem-execution-policy native_only \
  --population 8 \
  --iterations 12 \
  --repeats 2 \
  --fem-memory-budget-mb 768 \
  --fem-screening-repeats 1 \
  --fem-screening-prune-factor 0.6 \
  --fem-mapped-max-n-fma-light 300000 \
  --fem-mapped-max-buffer-mb-light 32 \
  --fem-mapped-max-mem-iters 64 \
  --fem-mapped-max-inner-iters-light 2048
```

### 8.6 Warianty platformowe (vendor aliases)
AMD:
```bash
python3 run_firefly_optimization.py --problem fem_parametric --backend amd
```
- próba `HIP`, fallback do `OpenCL`.

Intel:
```bash
python3 run_firefly_optimization.py --problem fem_parametric --backend intel
```
- mapowanie do `OpenCL`.

### 8.7 Preflight środowiska
Przed dużą kampanią:
```bash
python3 run_fem_parametric_preflight.py --backend all --execution-policy native_only --strict
```

Wyjście JSON:
```bash
python3 run_fem_parametric_preflight.py --backend cuda,amd,intel --json
```

Skrypt:
- sprawdza dostępność backendów dla `fem_parametric`,
- pokazuje `resolved_backend` i tryb (`native` / `mapped_native`),
- wypisuje komendy naprawcze, gdy backend jest niedostępny.

### 8.8 Macierz walidacyjna vs OpenCL baseline
```bash
python3 run_fem_parametric_matrix.py \
  --backends cpu,cuda,hip,opencl,metal,amd,intel \
  --baseline opencl \
  --n-configs 24 \
  --repeats 2 \
  --execution-policy native_only
```

Skrypt zapisuje:
- `data/validation/<timestamp>__fem_parametric_matrix/summary.json`
- korelację rang (Spearman) i overlap Top-K względem baseline OpenCL.

### 8.9 GUI launcher (wyniki + wykresy)
```bash
python3 run_autotune_gui.py
```

Dla pomiaru energii (powermetrics/NVML) uruchamiaj GUI jako root:
```bash
sudo -E python3 run_autotune_gui.py
```

GUI pozwala:
- uruchamiać cały pipeline:
  - kampanie `run_all_benchmarks.py` / `run_all_backends.py`,
  - benchmarki CPU/GPU i `real_kernels`,
  - autotuning (`run_firefly_optimization.py`),
  - walidacje `preflight` i `matrix`,
  - analizy (`cpu_summary`, `gpu_summary`, `roofline`, `report`, `data_quality`, `generate_plots`),
- oglądać log na żywo,
- śledzić postęp (progress bar + elapsed time + heartbeat „last output”),
- ładować najnowsze artefakty (session, optimization, matrix, report, plot),
- rysować wykresy: konwergencja FA, scatter ewaluacji, porównanie matrix vs baseline.

---

## 9. Co dalej (solidność naukowa + inżynierska)

1. Dodać baseline’y: random search, TPE/Bayes, CMA-ES/PSO i porównać z FA.
2. Dodać replikacje kampanii (`N` seedów) i raport niepewności optimum.
3. Rozszerzyć logi o fingerprint środowiska (driver, kernel, commit SHA, temperatury).
4. Dodać federację wielu hostów:
   - kolejka zadań (np. `jsonl` + lock / Redis),
   - worker per device,
   - centralny aggregator Pareto frontu.
5. Dodać walidację out-of-sample:
   - znalezione optimum uruchamiane w innym oknie czasowym/temperaturowym.
6. Dodać automatyczną publikację wyników do `analysis/` (wykresy convergence, hypervolume, fronty Pareto).

---

## 9. Ocena sensownosci implementacyjnej firefly w v3

### 9.1. Kiedy firefly ma sens
Algorytm firefly ma sens wtedy, gdy:
- przestrzen strojenia jest mieszana i wielowymiarowa,
- pojedyncza ewaluacja konfiguracji jest drozsza niz proste operacje na populacji,
- chcemy utrzymywac kompromis miedzy eksploracja i eksploatacja,
- zalezy nam na czytelnym przebiegu iteracyjnym i artefaktach do dalszej analizy.

W `v3` te warunki sa spelnione szczegolnie dla problemow `fem_parametric`, gdzie pojedyncza konfiguracja laczy wiele kontrolek o charakterze algorytmicznym i pamieciowym.

### 9.2. Co bylo slabe w prostszej wersji
W prostszej implementacji slabe bylyby dwie rzeczy:
- porownywanie score/brightness liczonych tylko na biezacej populacji,
- duplikowanie tych samych konfiguracji pod roznymi pozycjami w hiperszescianie.

Oba te problemy oslabiaja interpretacje, bo utrudniaja porownanie iteracji i zawyzaja pozorna liczbe odwiedzonych punktow.

### 9.3. Co zostalo poprawione w v3
Aktualna implementacja zostala wzmocniona przez:
- **archiwum unikalnych konfiguracji**,
- **ponowne przeliczanie brightness na archiwum**, a nie tylko na chwilowej populacji,
- **elitism** przez `elite_keep`,
- **deduplikacje pozycji** i respawn przy zderzeniach w przestrzeni dyskretnej,
- **stale artefakty diagnostyczne** (`optimization_convergence.png`, `optimization_primary_metric.png`, `optimization_scatter.png`),
- **lepsze summary** z informacja o unikalnych i wykonalnych ewaluacjach.

To sprawia, ze firefly w `v3` jest metodologicznie duzo bardziej sensowny niz wersja oparta tylko na chwilowej populacji.

### 9.4. Ograniczenia firefly, ktore nadal trzeba uczciwie opisac
Mimo poprawek, firefly nadal ma ograniczenia:
- ruch jest realizowany w przestrzeni zakodowanej do `[0,1]^D`, a nie w przestrzeni semantycznej problemu,
- zmienne kategoryczne sa nadal reprezentowane posrednio przez wartosci ciagle,
- dla bardzo malych budzetow ewaluacji random search moze byc prostszy i latwiejszy do interpretacji,
- brightness zalezy od modelu celu, wiec musi byc jawnie opisany w rozprawie.

### 9.5. Jak rekomendowac jego uzycie w rozprawie
Najuczciwiej jest przedstawic firefly jako:
- **narzedzie eksploracyjne i optymalizacyjne**, 
- dobre do badania krajobrazu konfiguracji,
- ale nie jako jedyny dowod metodologiczny.

W logice rozprawy firefly powinien wspierac warstwe poszukiwania konfiguracji, natomiast sila argumentu naukowego nadal powinna opierac sie na polaczeniu:
- mikrobenchmarkow,
- `fem_option_validation`,
- exact/reference,
- replay poprawnosci,
- i korelacji profilerowej.

