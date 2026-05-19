# Raport handoff do zewnętrznego AI

Data raportu: 2026-05-09  
Wersja robocza projektu: `v4`  
Główny katalog: `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4`

## 1. Cel projektu
Projekt jest uporządkowaną platformą badawczą do doktoratu. Łączy:
- mikrobenchmarki CPU i GPU,
- real kernels,
- pipeline analizy roofline,
- kampanie autotuningu i pełnego sweepu dla kodu Filipa,
- walidację poprawności,
- finalne figury publication-grade,
- GUI / panel WWW,
- bundle portable dla Linuksa.

Główny cel metodologiczny:
- przejść od charakterystyki platformy,
- przez model architektury i roofline,
- do interpretacji zachowania realistycznego kodu FEM (kod Filipa),
- i wygenerować materiał nadający się bezpośrednio do rozprawy doktorskiej i artykułów.

## 2. Która wersja jest aktualna
Aktualna wersja robocza to **`v4`**.

Starsze katalogi (`v2`, `v3`) są historyczne i nie powinny być traktowane jako główna baza do dalszego rozwoju.

Dodatkowo istnieje osobny bundle portable:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4_portable_linux`

## 3. Aktualna struktura metodologiczna
W projekcie przyjęto uporządkowany pipeline badawczy:

1. **Charakterystyka platformy**
   - CPU microbenchmarks
   - GPU microbenchmarks
   - cache / TLB / page-walk
   - roofline ceilings

2. **Real kernels**
   - CPU real kernels
   - GPU real kernels
   - walidacja modelu roofline na realistyczniejszych jądrach

3. **Most interpretacyjny FEM**
   - `fem_option_validation`

4. **Kampania aplikacyjna: kod Filipa**
   - `filip_original_portable`
   - `filip_autotune`
   - `filip_firefly`

5. **Walidacja poprawności obliczeń**
   - `filip_exact_reference`
   - replay / frozen inputs

6. **Synteza i interpretacja**
   - `profiler_correlation`

Ważna zasada metodologiczna:
- `L1/L2/L3/TLB/page-walk` **nie są osobnym etapem**, tylko należą do etapu mikrobenchmarków.

## 4. Co zostało już zrobione

### 4.1. Mikrobenchmarki CPU
Zaimplementowane i spięte:
- `memcpy` 1T / MT
- `STREAM` 1T / MT
- `FMA` / `peak FMA`
- `pointer latency`
- `TLB / page-walk latency`

Najważniejsze usprawnienia, które już zostały wprowadzone:
- dynamiczny sweep liczby wątków,
- lepsze wykrywanie topologii CPU,
- obsługa heterogeniczności Apple Silicon,
- ujednolicenie zakresów rozmiarów dla `memcpy` i `STREAM`,
- poprawa wiarygodności benchmarków compute na Apple Silicon,
- dodane wykresy skalowania oraz bardziej sensowny model porównania architektur.

### 4.2. Mikrobenchmarki GPU
Zaimplementowane backendy:
- `metal`
- `cuda`
- `hip`
- `opencl`

Projekt zakłada:
- macOS: `metal` / `opencl`
- Linux: `cuda` / `hip` / `opencl`

### 4.3. Tryby benchmarków
Dwa tryby benchmarków:
- `standard` = ścieżka porównawcza między architekturami,
- `extended` = bardziej architektur-specyficzna diagnostyka.

`extended` jest wdrożony przede wszystkim dla benchmark suites CPU/GPU.  
Nie jest jeszcze w pełni rozwinięty jako osobny, bogaty zestaw dla `real kernels`.

### 4.4. Real kernels
Sekcja `real kernels` została uporządkowana pod jedną historię badawczą:
- mikrobenchmarki opisują platformę,
- roofline daje model ograniczeń,
- real kernels służą do walidacji tego modelu.

Najważniejszy kontrast interpretacyjny:
- kod Filipa jest traktowany jako bardziej **compute-leaning FEM**,
- przeciwnym przypadkiem został przyjęty **SpMV** jako kontrast bardziej memory-bound.

### 4.5. Kod Filipa
Dla `filip_original` poprawiono generator figur tak, aby:
- warianty `QSS`, `SQS`, `SSQ` miały osobne wykresy,
- każdy wykres obejmował **pełne 80 kombinacji czasu** dla wariantu,
- generator brał wszystkie `status=ok`, a nie tylko kombinacje `constraints_ok`.

To była ważna poprawka, bo wcześniej na wykresach pojawiało się tylko kilka punktów zamiast pełnego sweepu.

### 4.6. Firefly
Implementacja Firefly została przejrzana i dopracowana.
Najważniejsze poprawki:
- lepsza semantyka brightness,
- archiwum unikalnych konfiguracji,
- poprawione summary,
- lepsza integracja z GUI i artefaktami.

### 4.7. GUI / panel WWW
Panel został uporządkowany i uproszczony:
- usunięto główny widok bloków jako dominujący interfejs,
- zostawiono 3 główne pakiety operacyjne:
  - benchmarki,
  - real kernels,
  - test Filipa,
- dodano paski postępu etapów,
- dodano zwijane sekcje,
- dodano podgląd figur,
- uproszczono ustawienia.

### 4.8. Portable bundle
Dodano osobną wersję przenośną dla Linuksa:
- `v4_portable_linux`

Bundle:
- tworzy lokalne środowisko,
- potrafi uruchamiać benchmarki na obcym hoście Linux,
- zapisuje raport zgodności hosta,
- buduje figury,
- potrafi uruchomić pakiety:
  - `benchmarks`
  - `real-kernels`
  - `filip`
  - `full`

## 5. Aktualny stan zdrowia systemu
Wynik z `python3 scripts/check_v4_health.py` na dzień 2026-05-09:

- ostatnia stabilna pełna kampania:
  - `20260508_102227__full_thesis__profile-full__backend-metal`
- `critical_success = true`
- `exit_code = 0`
- `running = false`

Status kroków w ostatniej stabilnej pełnej kampanii:
- `cpu_benchmark` -> `ok`
- `gpu_benchmark` -> `ok`
- `cpu_real_kernels` -> `ok`
- `gpu_real_kernels` -> `ok`
- `fem_option_validation` -> `ok`
- `filip_original_portable` -> `ok`
- `filip_autotune` -> `ok`
- `filip_firefly` -> `ok`
- `filip_exact_reference` -> `skipped`
- `profiler_correlation` -> `ok`

Raport zdrowia:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/health/v4_health_report.md`

## 6. Finalny zestaw figur publication-grade
System został przebudowany tak, żeby generować ograniczony, publikacyjny zestaw figur, a nie duży dump techniczny.

### 6.1. Globalny zestaw `thesis_core`
Katalog:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/analysis/figures/thesis_core`

Manifest:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/analysis/figures/manifests/thesis_core_manifest.json`

Aktualnie generowane figury globalne (`8`):
- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `cpu_peak_compute_scaling.png`
- `cpu_memory_latency_hierarchy.png`
- `gpu_microbenchmark_suite.png`
- `platform_roofline_measured.png`
- `real_kernels_model_validation.png`
- `real_kernels_filip_contrast_map.png`

### 6.2. Zestaw Filipa `thesis_core`
Katalog przykładowego runu:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/optimization/20260508_103330_478372__filip_original__backend-metal/figures/thesis_core`

Manifest przykładowego runu:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/optimization/20260508_103330_478372__filip_original__backend-metal/figures/manifests/filip_figures_manifest.json`

Aktualnie generowane figury Filipa (`6` + `1 appendix`):
- `filip_variant_qss.png`
- `filip_variant_sqs.png`
- `filip_variant_ssq.png`
- `filip_autotuning_trace.png`
- `filip_best_summary.png`
- `filip_memory_compute_breakdown.png`
- appendix: `filip_best_configuration_card.png`

### 6.3. Pakiety ZIP
ZIP z figurami jest generowany automatycznie.

Aktualny ZIP dla ostatniej stabilnej kampanii:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/thesis_full/20260508_102227__full_thesis__profile-full__backend-metal/artifacts/plots_bundle__20260508_102227__full_thesis__profile-full__backend-metal__20260509_140954.zip`

## 7. Język i podpisy figur
Zostało uporządkowane:
- figury finalne są spolonizowane,
- podpis każdej figury zawiera testowaną platformę,
- podpis zawiera też jawnie architekturę, np.:
  - `system: Darwin`
  - `architektura: arm64`

Ujednolicony styl podpisu:
- `Platforma testowa: CPU: ... | system: ... | architektura: ...`
- `Platforma testowa: GPU: ... | backend: ... | system: ... | architektura: ...`
- dla Filipa dodatkowo może pojawić się `przypadek: ...`

## 8. Zgodność platformowa

### 8.1. Co działa najlepiej
Najbardziej dojrzałe środowiska uruchomieniowe:
- macOS + Metal
- Linux + CUDA / HIP / OpenCL

### 8.2. Windows
Windows **nie jest jeszcze domkniętym first-class targetem**.

Co działa sensownie na Windowsie:
- analiza danych,
- generowanie figur,
- ZIP-y,
- panel WWW,
- część warstwy Pythonowej.

Czego nie należy dziś uznawać za domknięte:
- pełny natywny `build + run` CPU na Windows,
- pełna, utrzymywana ścieżka portable native Windows,
- bezproblemowa wielobackendowa ścieżka GPU natywnie na Windows.

Najrozsądniejsza ścieżka dla Windows:
- **Windows + WSL2 + Linuxowy bundle portable**

Szczególnie:
- NVIDIA + WSL2 = mocna ścieżka,
- AMD + WSL2 = możliwe, ale zależne od wspieranego SKU i sterowników,
- Intel = bardziej ostrożnie.

## 9. Co jest jeszcze nie w pełni domknięte / co warto sprawdzić dalej
Poniżej lista rzeczy, które nie są dziś krytycznym blockerem, ale mogą nadal wymagać rozszerzenia, domknięcia albo audytu:

1. **Native Windows build/run**
- projekt sam wskazuje, że Windows nie ma jeszcze stabilnej, utrzymywanej ścieżki build/run przynajmniej dla CPU,
- warto ocenić, czy to w ogóle ma sens rozwijać natywnie, czy lepiej postawić na WSL2.

2. **Exact/reference na macOS**
- w ostatniej stabilnej kampanii `filip_exact_reference` było `skipped`,
- to jest oczekiwane bez odpowiedniego replay dump root,
- ale warto ocenić, czy do finalnych eksperymentów należy mocniej zautomatyzować tę ścieżkę.

3. **Extended mode dla real kernels**
- `extended` jest wdrożony głównie dla benchmark suites CPU/GPU,
- warto sprawdzić, czy jest sens rozszerzyć go także na `real kernels`.

4. **Energia / moc**
- historycznie dokumentacja wskazywała niespójności między CPU i GPU w logice pomiaru energii,
- warto sprawdzić, czy po ostatnich zmianach nadal jest tu coś do uporządkowania.

5. **CSV / summary contract**
- warto sprawdzić, czy wszystkie workflowy zapisują już wystarczająco spójny kontrakt danych,
- szczególnie jeśli projekt ma iść dalej w stronę automatycznych meta-analiz lub cross-platform comparison.

6. **Legacy artefakty i nazewnictwo**
- historyczne katalogi i stare wykresy nadal istnieją w repo jako tło rozwojowe,
- główny pipeline jest już oparty o `thesis_core`, ale warto ocenić, czy nie należy jeszcze mocniej odciąć warstwy legacy.

## 10. Najważniejsze osiągnięcia projektu na dziś
Najkrótsze podsumowanie tego, co już realnie jest gotowe:

- istnieje uporządkowana wersja `v4`, która działa jako główna baza projektu,
- istnieje 6-etapowy, spójny pipeline metodologiczny,
- istnieje pełna kampania `full_thesis_pipeline`, która przechodzi z sukcesem,
- istnieje publication-grade figure pipeline,
- liczba figur została radykalnie zredukowana i uporządkowana,
- figury Filipa pokazują pełne 80 kombinacji czasu dla wariantów,
- figury są po polsku i podpisane architekturą/platformą,
- istnieje bundle portable dla Linuksa,
- istnieje uproszczony panel WWW,
- istnieje ZIP z końcowymi figurami.

## 11. Pytania do zewnętrznego AI / audytu
Proszę przeanalizować ten stan i odpowiedzieć w szczególności na pytania:

1. Czy obecna struktura metodologiczna jest wystarczająco mocna do doktoratu?
2. Czy w zestawie `thesis_core` nadal są figury redundantne albo zbyt słabe naukowo?
3. Czy czegoś brakuje w warstwie walidacji poprawności i interpretacji roofline?
4. Czy `extended mode` powinien zostać rozbudowany w `real kernels` lub w kodzie Filipa?
5. Czy jest sens rozwijać natywną ścieżkę Windows, czy lepiej oficjalnie przyjąć strategię `Windows host + WSL2`?
6. Czy warto dodać jeszcze jakieś brakujące benchmarki kontrastowe lub dodatkowe miary normalizacji między architekturami?
7. Czy są jakieś luki w spójności danych, kontraktów wynikowych albo reprodukowalności, które jeszcze nie zostały domknięte?

## 12. Najważniejsze ścieżki i pliki

### Główna baza
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4`

### Raport zdrowia
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/health/v4_health_report.md`

### Finalne figury globalne
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/analysis/figures/thesis_core`

### Manifest figur globalnych
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/analysis/figures/manifests/thesis_core_manifest.json`

### Finalne figury Filipa (przykładowy ostatni stabilny run)
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/optimization/20260508_103330_478372__filip_original__backend-metal/figures/thesis_core`

### Manifest figur Filipa
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/optimization/20260508_103330_478372__filip_original__backend-metal/figures/manifests/filip_figures_manifest.json`

### ZIP z figurami dla ostatniej stabilnej kampanii
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4/data/thesis_full/20260508_102227__full_thesis__profile-full__backend-metal/artifacts/plots_bundle__20260508_102227__full_thesis__profile-full__backend-metal__20260509_140954.zip`

### Portable Linux bundle
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4_portable_linux`
