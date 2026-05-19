## Standard metryk (v2)

Ten dokument definiuje metryki używane w `apple_microbench_variant2_streamfix/v2`
tak, aby:

- były **jednoznaczne** (brak wątpliwości co do jednostek),
- dało się je wykorzystać w **publikacjach naukowych** (doktorat),
- były spójne między CPU / GPU / real_kernels.

### 1. Metryki czasu

- **elapsed_s**  
  - Definicja: czas trwania mierzonej sekcji runu (kernel loop, pętla benchmarku).  
  - Jednostka: sekundy \[s].  
  - Uwagi:
    - Dla części benchmarków jest to czas pojedynczego runu (zawiera `iters`),
      nie pojedynczego wywołania kernela.
    - W opisach publikacyjnych należy jasno zaznaczyć, czego dotyczy „elapsed”.

### 2. Przepustowość pamięci

- **throughput_gbps**  
  - Definicja:  
    \[
    \text{throughput\_gbps} = \frac{\text{bytes\_total}}{\text{elapsed\_s} \cdot 10^{9}}
    \]
  - Jednostka: gigabajty na sekundę \[GB/s], **zawsze** \(10^9\) bajtów.  
  - `bytes_total`:
    - CPU STREAM: suma bajtów czytanych i zapisywanych zgodnie z regułami STREAM,
      zliczana przez `bytes_per_iter * iters`.
    - CPU bandwidth: zwykle `2 * size_bytes * iters` (read + write).
    - GPU bandwidth: analogicznie, zależnie od `transfer_kind`.
  - Uwagi:
    - Dla porównań CPU vs GPU zawsze należy upewnić się, że liczymy
      bytes_total w ten sam sposób (czy liczymy tylko payload, czy również
      dodatkowe kopie / buforowanie).

Legacy:
- **gbps** – alias na `throughput_gbps`; w nowym kodzie preferujemy nazwę `throughput_gbps`.

### 3. Wydajność obliczeniowa

- **gflops** / **throughput_gflops**  
  - Definicja:
    \[
    \text{GFLOP/s} = \frac{\text{flops\_total}}{\text{elapsed\_s} \cdot 10^{9}}
    \]
  - Jednostka: giga operacji zmiennoprzecinkowych na sekundę \[GFLOP/s].  
  - `flops_total`:
    - dla FMA: \(2 \cdot n\_elements \cdot iters\_inner\),
    - dla GEMM: klasyczne \(2MNK\) dla mnożenia macierzy \(M \times K\) i \(K \times N\).

- **gflops_peak**, **gflops_mean**, **gflops_sigma**  
  - Używane w benchmarkach typu „peak”, gdzie:
    - `gflops_peak` – maksymalna wartość z wielu runów,
    - `gflops_mean` – średnia arytmetyczna,
    - `gflops_sigma` – odchylenie standardowe (populacyjne).

### 4. Latencja

- **latency_ns**  
  - Definicja: średni czas pojedynczej operacji / kroku (np. pointer-chasing)
    liczony jako:
    \[
    \text{latency\_ns} = \frac{\text{elapsed\_s}}{\text{liczba\_operacji}} \cdot 10^{9}
    \]
  - Jednostka: nanosekundy \[ns].  
  - Używana głównie w:
    - CPU pointer-chasing (`run_pointer_latency.py`),
    - CPU TLB / page-walk (`run_tlb_latency.py`),
    - GPU pointer-chasing (Metal/CUDA/HIP/OpenCL).

- **estimated_residency**
  - Definicja: przybliżona klasyfikacja, który poziom hierarchii pamięci
    najprawdopodobniej dominuje dla danego working setu.
  - Wartości:
    - platformy jednorodne: `L1`, `L2`, `L3`, `DRAM`, `unknown`,
    - platformy heterogeniczne: dodatkowo strefy graniczne, np.
      `P-L1 / E-L2`, `P-L2 / E-DRAM`.
  - Sposób wyznaczania:
    - benchmark wykrywa rozmiary cache CPU,
    - working set jest porównywany z progami L1/L2/L3,
    - na platformach heterogenicznych porównanie wykonywane jest osobno dla
      progów rdzeni `Performance` i `Efficiency`,
    - klasyfikacja jest zapisywana do CSV i używana do wykresów.
  - Uwaga metodologiczna:
    - nie jest to sprzętowy licznik producenta,
    - to eksperymentalna, interpretacyjna etykieta wspierająca analizę doktoratu.
    - na Apple Silicon nie wolno mylić `L1 instruction cache` z `L1 data cache`;
      benchmark pointer-chasing odnosi się do `L1D`, nie do `L1I`.

### 5. Energia i moc

- **energy_j**  
  - Definicja: energia pobrana w trakcie runu, obliczona jako różnica licznika
    energii przed i po runie lub jako całka z mocy po czasie.  
  - Jednostka: dżule \[J].

- **avg_power_w**  
  - Definicja:
    \[
    \text{avg\_power\_w} = \frac{\text{energy\_j}}{\text{elapsed\_s}}
    \]
    jeśli obie wartości są dodatnie i skończone.  
  - Jednostka: waty \[W].

- **energy_source**  
  - Źródło pomiaru energii/mocy (patrz `CSV_SCHEMA.md`).  
  - Krytyczne przy interpretacji wyników – np. `rapl` vs `powermetrics` vs `nvml`.

- **energy_supported**, **energy_samples**, **energy_confidence**  
  - `energy_supported`: czy backend realnie mierzył energię (`1`) czy użył fallbacku (`0`).  
  - `energy_samples`: liczba próbek mocy użytych do integracji (0 → tylko fallback).  
  - `energy_confidence`: heurystyczna jakość pomiaru (0–1), pełne znaczenie
    opisane w dokumentacji modułu `energy_utils.py`.

### 6. Metryki efektywności energetycznej

- **j_per_gb**  
  - Definicja:
    \[
    \text{j\_per\_gb} = \frac{\text{energy\_j}}{\text{bytes\_total} / 10^{9}}
    \]
  - Jednostka: J/GB (energia na gigabajt przetworzonych danych).
  - Interpretacja:
    - Niższa wartość → lepsza efektywność energetyczna transferu pamięci.

- **j_per_gflop**  
  - Definicja:
    \[
    \text{j\_per\_gflop} = \frac{\text{energy\_j}}{\text{flops\_total} / 10^{9}}
    \]
  - Jednostka: J/GFLOP.  
  - Interpretacja:
    - Niższa wartość → lepsza efektywność energetyczna obliczeń.

- **edp** (Energy-Delay Product)  
  - Definicja:
    \[
    \text{edp} = \text{energy\_j} \cdot \text{elapsed\_s}
    \]
  - Jednostka: J·s.  
  - Interpretacja:
    - Niższa wartość oznacza lepszy kompromis między energią a czasem wykonania.

### 7. Roofline

- **ai_flop_per_byte**  
  - Intensywność obliczeniowa (Arithmetic Intensity):
    \[
    \text{AI} = \frac{\text{FLOP}}{\text{bytes}}
    \]
  - Jednostka: FLOP/byte.

- **peak_bw_gbps**, **peak_gflops**  
  - Wartości „sufitów” wyznaczane na podstawie pików z mikrobenchmarków
    (zob. `analysis/roofline_model.py`).

- **bw_limited_gflops**, **attainable_gflops**, **regime**
  - `bw_limited_gflops = AI * peak_bw_gbps`.  
  - `attainable_gflops = min(peak_gflops, bw_limited_gflops)`.  
  - `regime = "memory-bound"` jeśli `bw_limited_gflops < peak_gflops`,
    w przeciwnym razie `"compute-bound"`.

### 8. Wskazówki do publikacji

- Zawsze podawaj:
  - jednostki (GB/s, GFLOP/s, ns, J, W),
  - źródło energii (`energy_source`),
  - liczbę powtórzeń (runs) oraz sposób agregacji (mean±sigma, CV).
- Przy porównaniach CPU vs GPU:
  - upewnij się, że bytes_total i flops_total są liczone w ten sam sposób,
  - komentuj, czy benchmark odzwierciedla realne workloady (tu pomaga moduł `real_kernels`).
