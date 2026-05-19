# Audyt publikacyjny figur i pipeline'u wykresów (`v4`)

## Cel
Celem refaktoru nie było wygenerowanie większej liczby wykresów, tylko zbudowanie zwartego, publication-grade pipeline'u figur do:
- rozprawy doktorskiej,
- przyszłych artykułów,
- slajdów,
- paczek archiwalnych z artefaktami.

Kluczowa decyzja projektowa:
**pipeline ma generować mały rdzeń figur o wysokiej wartości argumentacyjnej, a nie dump wszystkich możliwych projekcji danych.**

---

## Wniosek główny
Stary pipeline był zbyt silnie "data-driven":
- prawie każdy dostępny CSV dostawał osobny wykres,
- powstawały rodziny typu `*_best`, `*_per_thread`, `*_speedup`, `*_family`, `*_overview`, `*_publication`,
- część figur była debugowa albo eksploracyjna,
- narracja między benchmarkami, roofline i real kernels była niespójna,
- wykresy Filipa mieszały widoki referencyjne, exploratory i appendixowe.

Nowy pipeline jest **argument-driven**. Każda figura ma odpowiadać na konkretne pytanie badawcze.

---

## Docelowy rdzeń figur (`Compact thesis set`)

### Globalny zestaw thesis-core
Generowany do:
- `analysis/figures/thesis_core/`

1. `cpu_memcpy_bandwidth_scaling.png`
2. `cpu_stream_triad_scaling.png`
3. `cpu_peak_compute_scaling.png`
4. `cpu_memory_latency_hierarchy.png`
5. `gpu_microbenchmark_suite.png`
6. `platform_roofline_measured.png`
7. `real_kernels_model_validation.png`
8. `real_kernels_filip_contrast_map.png`

### Zestaw Filipa thesis-core
Generowany do:
- `<optimization_run>/figures/thesis_core/`

9. `filip_variant_qss.png`
10. `filip_variant_sqs.png`
11. `filip_variant_ssq.png`
12. `filip_autotuning_trace.png`
13. `filip_best_summary.png`
14. `filip_memory_compute_breakdown.png`

### Appendix / supporting
Generowany do:
- `<optimization_run>/figures/appendix/`

- `filip_best_configuration_card.png`

To daje **14 głównych figur**, czyli dokładnie zakres odpowiedni dla zwartego rozdziału doktorskiego.

---

## Narracja badawcza po uporządkowaniu

### 1. Mikrobenchmarki budują model platformy
- `cpu_memcpy_bandwidth_scaling`
- `cpu_stream_triad_scaling`
- `cpu_peak_compute_scaling`
- `cpu_memory_latency_hierarchy`
- `gpu_microbenchmark_suite`

Pytanie badawcze:
**jakie są zmierzone limity platformy: bandwidth, compute, latency, scaling?**

### 2. Roofline zamienia benchmarki w model interpretacyjny
- `platform_roofline_measured`

Pytanie badawcze:
**jakie są rzeczywiste ceilings używane dalej do interpretacji real kernels i kodu Filipa?**

### 3. Real kernels walidują model
- `real_kernels_model_validation`
- `real_kernels_filip_contrast_map`

Pytanie badawcze:
**czy uproszczone, ale realistyczne kernelle zachowują się zgodnie z limitem przewidzianym przez mikrobenchmarki i roofline?**

### 4. Kod Filipa pokazuje krajobraz konfiguracji i praktyczne strojenie
- `filip_variant_qss`
- `filip_variant_sqs`
- `filip_variant_ssq`
- `filip_autotuning_trace`
- `filip_best_summary`
- `filip_memory_compute_breakdown`

Pytanie badawcze:
**jak zmienia się czas wykonania wraz z konfiguracją autotuningu oraz jaki jest charakter obciążenia aplikacyjnego?**

---

## Najważniejsze decyzje metodologiczne

### 1. Usunięto równoległe wersje `publication` vs zwykłe
W starym pipeline równolegle żyły np.:
- `cpu_cache_hierarchy_latency`
- `cpu_cache_hierarchy_latency_publication`
- `cpu_tlb_page_walk_latency`
- `cpu_tlb_page_walk_latency_publication`

Nowa zasada:
**jest tylko jedna finalna figura publikacyjna.**

### 2. Usunięto syntetyczny punkt z roofline
Stary roofline wykorzystywał dane z `roofline_result.json`, które zawierały punkt estymowany dla zadanego `AI`, a nie tylko czyste zmierzone ceilings.

Nowa zasada:
**centralny roofline używa tylko zmierzonych peaków z mikrobenchmarków.**

### 3. Naprawiono clipping osi Y w wykresach Filipa
Stare wykresy wariantowe stosowały robust clipping, przez co wolniejsze konfiguracje mogły znikać z osi Y.

Nowa zasada:
**w core figurach Filipa nie przycinamy outlierów w osi Y.**

### 4. Odrzucono punkty `out-of-model` z centralnych figur real kernels
W praktyce okazało się, że np. `gemm/cpu` korzysta z `numpy @` / vendor BLAS i może osiągać przepustowości lub compute wykraczające poza generic CPU peak microbenchmark.

Nowa zasada:
**punkty przekraczające 110% zmierzonego sufitu są pomijane w centralnych figurach modelowych i jawnie oznaczane jako poza zakresem modelu.**

To nie usuwa danych z projektu. To tylko rozdziela:
- **rdzeń porównawczy**, od
- **wyników vendor/library-specific**.

### 5. Ujednolicono semantykę sekcji CPU scaling
Wcześniej były osobno:
- `*_best`
- `*_per_thread`
- `*_speedup`

Nowa zasada:
**jedna figura = throughput + efficiency.**

`speedup` nie jest już osobnym produktem domyślnym, bo najczęściej nie wnosi niezależnej treści wobec throughput i efficiency.

---

## Audyt figur: stan starych wykresów

### A. Globalne figury benchmarków i roofline

| Figura | Decyzja | Uzasadnienie | Następca |
|---|---|---|---|
| `cpu_cache_hierarchy_latency.png` | MERGE | Duplikat semantyczny wobec wersji publication. | `cpu_memory_latency_hierarchy.png` |
| `cpu_cache_hierarchy_latency_publication.png` | MERGE | Zamiast dwóch wersji istnieje jedna figura cache+TLB. | `cpu_memory_latency_hierarchy.png` |
| `cpu_compute_family.png` | REMOVE | Zbyt surowy dump rodzin benchmarku. | `cpu_peak_compute_scaling.png` |
| `cpu_compute_scaling_best.png` | MERGE | Część jednej historii o scalingu. | `cpu_peak_compute_scaling.png` |
| `cpu_compute_scaling_per_thread.png` | MERGE | Część tej samej historii. | `cpu_peak_compute_scaling.png` |
| `cpu_compute_scaling_speedup.png` | REMOVE | Niska dodatkowa wartość względem throughput + efficiency. | brak osobnego następcy |
| `cpu_mem_copy_family.png` | REMOVE | Eksploracyjny dump rodzin zamiast figury argumentacyjnej. | `cpu_memcpy_bandwidth_scaling.png` |
| `cpu_mem_copy_multi_thread.png` | MERGE | MT powinno być subplotem, nie osobnym plikiem. | `cpu_memcpy_bandwidth_scaling.png` |
| `cpu_mem_copy_scaling_best.png` | MERGE | Jedna część historii skalowania. | `cpu_memcpy_bandwidth_scaling.png` |
| `cpu_mem_copy_scaling_per_thread.png` | MERGE | Nadmiar wobec throughput + efficiency. | `cpu_memcpy_bandwidth_scaling.png` |
| `cpu_mem_copy_scaling_speedup.png` | REMOVE | Redundancja. | brak osobnego następcy |
| `cpu_mem_copy_single_thread.png` | MERGE | 1T powinno być subplotem. | `cpu_memcpy_bandwidth_scaling.png` |
| `cpu_memory_latency_suite.png` | KEEP | Dobra intuicja sekcyjna, ale zastąpiona nowszą wersją thesis-core. | `cpu_memory_latency_hierarchy.png` |
| `cpu_microbench_overview.png` | REMOVE | Wykres zbiorczy był zbyt gęsty i mieszał kilka historii naraz. | rozbite na trzy core figury CPU |
| `cpu_stream_family.png` | REMOVE | Za dużo linii, niska czytelność legendy. | `cpu_stream_triad_scaling.png` |
| `cpu_stream_scaling_best.png` | MERGE | Część jednej figury skalowania. | `cpu_stream_triad_scaling.png` |
| `cpu_stream_scaling_per_thread.png` | MERGE | Nadmiar wobec core figury. | `cpu_stream_triad_scaling.png` |
| `cpu_stream_scaling_speedup.png` | REMOVE | Redundancja. | brak osobnego następcy |
| `cpu_stream_triad_vs_size.png` | MERGE | Sensowny kierunek, ale teraz jako subplot finalnej figury. | `cpu_stream_triad_scaling.png` |
| `cpu_tlb_page_walk_latency.png` | MERGE | Duplikat semantyczny wersji publication. | `cpu_memory_latency_hierarchy.png` |
| `cpu_tlb_page_walk_latency_publication.png` | MERGE | Wchodzi do wspólnej figury cache+TLB. | `cpu_memory_latency_hierarchy.png` |
| `gpu_bandwidth_detailed.png` | MERGE | Dublował `gpu_bandwidth_vs_size`. | `gpu_microbenchmark_suite.png` |
| `gpu_bandwidth_vs_size.png` | MERGE | Wchodzi jako lewy panel w figurze GPU 1x3. | `gpu_microbenchmark_suite.png` |
| `gpu_compute_family.png` | MERGE | Wchodzi jako środkowy panel. | `gpu_microbenchmark_suite.png` |
| `gpu_microbenchmark_overview.png` | REMOVE | Zbyt ogólne i częściowo dublujące. | `gpu_microbenchmark_suite.png` |
| `gpu_pointer_latency.png` | MERGE | Wchodzi jako prawy panel. | `gpu_microbenchmark_suite.png` |
| `gpu_pointer_latency_metal.png` | REMOVE | Styl/back-end split bez wystarczającej niezależnej treści. | `gpu_microbenchmark_suite.png` |
| `real_kernels_compute_detail.png` | REMOVE | Exploratory breakdown bez dominującej historii. | `real_kernels_model_validation.png` |
| `real_kernels_fem_detail.png` | REMOVE | Za wąskie i lokalne wobec centralnej mapy interpretacyjnej. | `real_kernels_filip_contrast_map.png` |
| `real_kernels_filip_contrast.png` | REDESIGN | Pomysł dobry, wykonanie zbyt lokalne. | `real_kernels_filip_contrast_map.png` |
| `real_kernels_filip_contrast_map.png` | KEEP / REDESIGN | Naukowo wartościowy rdzeń, ale wymagał lepszych osi i czyszczenia clutteru. | `real_kernels_filip_contrast_map.png` |
| `real_kernels_memory_detail.png` | REMOVE | Za bardzo exploratory. | `real_kernels_model_validation.png` |
| `real_kernels_overview.png` | REMOVE | Mieszał kilka metryk i nie prowadził historii badawczej. | `real_kernels_model_validation.png` |
| `roofline_cpu.png` | MERGE | Osobne roofline CPU/GPU nie są potrzebne jako domyślne pliki. | `platform_roofline_measured.png` |
| `roofline_gpu.png` | MERGE | Jak wyżej. | `platform_roofline_measured.png` |
| `roofline_session_overview.png` | REDESIGN | Koncepcyjnie ważny, ale poprzednio oparty częściowo o syntetyczny punkt. | `platform_roofline_measured.png` |

### B. Figury Filipa

| Figura | Decyzja | Uzasadnienie | Następca |
|---|---|---|---|
| `article_all_option_times_qss.png` | REMOVE | Wersja exploratory, dubluje nową figurę wariantową. | `filip_variant_qss.png` |
| `article_all_option_times_sqs.png` | REMOVE | Jak wyżej. | `filip_variant_sqs.png` |
| `article_all_option_times_ssq.png` | REMOVE | Jak wyżej. | `filip_variant_ssq.png` |
| `article_autotuning_overview.png` | REMOVE | Nadmiar wobec trace + summary. | `filip_autotuning_trace.png`, `filip_best_summary.png` |
| `article_autotuning_settings_heatmap.png` | REMOVE | Ciekawy diagnostycznie, ale zbyt appendixowy jak na core set. | opcjonalny appendix w przyszłości |
| `article_autotuning_trace_with_settings.png` | KEEP / REDESIGN | Wysoka wartość naukowa, ale przeniesiona do krótszej, docelowej rodziny nazw. | `filip_autotuning_trace.png` |
| `article_best_configuration_card.png` | KEEP (appendix) | Dobra figura wspierająca, ale nie centralna dla narracji głównej. | `filip_best_configuration_card.png` |
| `article_best_summary.png` | KEEP / REDESIGN | Dobra figura syntetyczna, zostaje jako core. | `filip_best_summary.png` |
| `article_filip_execution_time_by_option.png` | REMOVE | Zbyt zbliżony informacyjnie do nowych figur wariantowych. | `filip_variant_qss/sqs/ssq.png` |
| `article_memory_compute_breakdown.png` | KEEP / REDESIGN | Wartościowa interpretacja compute-vs-memory. | `filip_memory_compute_breakdown.png` |
| `article_operator_laplace_variants.png` | REMOVE | Exploratory split, zbyt granularny na core set. | brak domyślnego następcy |
| `article_operator_test_variants.png` | REMOVE | Jak wyżej. | brak domyślnego następcy |
| `article_option_times_qss.png` | REDESIGN | Poprzednio miała clipping i niespójną semantykę. | `filip_variant_qss.png` |
| `article_option_times_sqs.png` | REDESIGN | Jak wyżej. | `filip_variant_sqs.png` |
| `article_option_times_ssq.png` | REDESIGN | Jak wyżej. | `filip_variant_ssq.png` |
| `article_paper_option_times.png` | REMOVE | Zbyt pośredni wobec lepszych figur wariantowych. | brak osobnego następcy |
| `article_variant_option_times.png` | REMOVE | Eksploracyjny grid o mniejszej czytelności. | `filip_variant_qss/sqs/ssq.png` |
| `article_variant_option_times_full.png` | REMOVE | Duplikat / nadmiar. | brak osobnego następcy |

---

## Nowa struktura katalogów

### Globalne figury
- `analysis/figures/thesis_core/`
- `analysis/figures/appendix/`
- `analysis/figures/manifests/`

### Figury kampanii Filipa
- `<optimization_run>/figures/thesis_core/`
- `<optimization_run>/figures/appendix/`
- `<optimization_run>/figures/manifests/`

### Artefakty ZIP
- `<campaign_dir>/artifacts/plots_bundle__...zip`

Ta struktura rozdziela:
- **rdzeń figur do tekstu**,
- **appendix**,
- **metadane/manifests**,
- **paczki archiwalne**.

---

## Naming convention

### Zasada
Nazwa pliku ma mówić:
1. **o jakim obszarze mówimy**,
2. **jaką historię badawczą opowiada figura**,
3. a nie jak powstała technicznie.

### Wzorzec
- `<domain>_<story>.png`

### Przykłady
- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `platform_roofline_measured.png`
- `real_kernels_model_validation.png`
- `filip_variant_qss.png`
- `filip_memory_compute_breakdown.png`

### Czego unikamy
- `*_best`
- `*_per_thread`
- `*_speedup`
- `*_publication`
- `*_overview`
- `*_family`

chyba że nazwa jest rzeczywiście nośnikiem niezależnej historii naukowej.

---

## Reusable plotting utilities
Wspólna warstwa narzędziowa:
- `analysis/publication_style.py`

### Obecne utility
- `ensure_figure_dirs()`
- `clear_pngs(directory)`
- `setup_publication_theme(plt)`
- `backend_color(backend)`
- `algorithm_color(name)`
- `operator_style(name)`
- `apply_axis_style(ax)`
- `add_platform_badge(fig, label)`
- `save_figure(fig, out, dpi, platform_label)`
- `figure_entry(...)`
- `finite(values)`
- `padded_ylim(values, ...)`

### Rola tej warstwy
- wspólny wygląd wszystkich figur,
- jedna semantyka kolorów,
- jedna semantyka podpisów platformy,
- jeden mechanizm eksportu figur i manifestów.

---

## Wspólny plotting API

### Globalny generator
- `analysis/generate_plots.py`
- punkt wejścia: `generate_thesis_core_figures()`

Produkt:
- `analysis/figures/thesis_core/*.png`
- `analysis/figures/manifests/thesis_core_manifest.json`

### Generator Filipa
- `analysis/filip_article_plots.py`
- punkt wejścia: `generate_article_plots(optimization_dir)`

Produkt:
- `<run>/figures/thesis_core/*.png`
- `<run>/figures/appendix/*.png`
- `<run>/figures/manifests/filip_figures_manifest.json`

### ZIP
- `analysis/build_plot_zip.py`

Produkt:
- paczka z:
  - `thesis_core/global/`
  - `thesis_core/filip/`
  - opcjonalnie `appendix/...`
  - manifestami
  - metadanymi kampanii

---

## Niespójności metodologiczne wykryte w audycie

### Naprawione
1. **Duplikacja publication vs zwykłe**
2. **Redundancja scaling best/per-thread/speedup**
3. **Clipping outlierów w osi Y dla Filipa**
4. **Syntetyczny punkt w centralnym roofline**
5. **Mieszanie dumpów exploratory z figurami finalnymi**

### Ograniczenia jawnie rozpoznane
1. **`gemm/cpu` korzysta z `numpy @` / vendor BLAS**
   - może wyjść poza generic CPU peak microbenchmark,
   - dlatego nie powinien być bezrefleksyjnie mieszany z centralnym modelem generic roofline.

2. **Nie wszystkie wykresy Filipa pokazują pełne `2^N` kombinacji**
   - pokazują przestrzeń rzeczywiście ocenioną / dopuszczoną przez kampanię,
   - to jest poprawne, ale trzeba to rozumieć jako przestrzeń badanej kampanii, nie pełny abstrakcyjny hiperkub bitów.

3. **Roofline i real kernels nadal zależą od jakości definicji AI dla poszczególnych kerneli**
   - szczególnie uproszczone estymaty AI dla stencil / reduction należy traktować jako interpretacyjne, nie absolutne.

---

## Zestaw publication variants
Wszystkie warianty publikacyjne mają bazować na tym samym rdzeniu figur.

### `thesis`
- pełny zestaw 14 figur
- podpisy platformy i pełne opisy osi

### `ieee`
- skrócony podzbiór 6–8 figur
- mniej tekstu pomocniczego
- ciaśniejszy layout

### `slides`
- ten sam rdzeń danych
- większe fonty
- krótsze tytuły
- redukcja detalu w tick labels

Nie oznacza to trzech osobnych pipeline'ów danych.
To są trzy warianty eksportu z jednego rdzenia.

---

## Co zostało wdrożone w kodzie
1. nowy globalny generator thesis-core,
2. nowy generator figur Filipa do `figures/thesis_core` i `figures/appendix`,
3. nowy manifest figure-setów,
4. nowy ZIP oparty na `thesis_core`,
5. przepięcie panelu WWW na nowe nazwy figur,
6. przepięcie preferencji GUI na nowy rdzeń figur,
7. odcięcie centralnych figur od starych wariantów debug/exploratory.

---

## Co dalej byłoby sensowne
1. Dodać eksport wariantów layoutu `ieee` i `slides` z tego samego manifestu.
2. Dodać krótki tekstowy `caption pack`, czyli automatyczny szkic podpisów do figur.
3. Rozważyć osobny appendix-only generator dla bardziej eksploracyjnych figur, ale bez mieszania go z core setem.

---

## Podsumowanie końcowe
Projekt przeszedł z modelu:
- "generuj wszystko, co da się narysować"

do modelu:
- **"generuj tylko figury, które wzmacniają argument naukowy"**.

To jest właściwy kierunek pod doktorat i publikacje.
