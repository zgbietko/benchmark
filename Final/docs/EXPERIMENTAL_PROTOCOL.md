# Protokol eksperymentalny dla v3

Ten dokument definiuje protokol eksperymentalny dla `v3`. Jego celem jest:
- zapewnienie porownywalnosci wynikow,
- ograniczenie dryfu eksperymentalnego,
- przygotowanie danych do publikacji i rozprawy.

## 1. Zasady ogolne

### 1.1. Rozdziel correctness od performance

Wszystkie eksperymenty musza byc klasyfikowane jako jedna z warstw:

1. `microbenchmarks`
2. `fem_option_validation`
3. `reference_exact`
4. `correctness_replay`
5. `native_performance_campaign`
6. `profiler_correlation`

Nie wolno mieszac ich roli interpretacyjnej.

### 1.2. Nie porownuj backendow tylko po czasie

Przed porownaniem wydajnosci:
- dla reprezentatywnych przypadkow nalezy przeprowadzic correctness replay,
- albo jasno oznaczyc, ze dana kampania nie jest replayem 1:1.

### 1.3. Wszystko archiwizuj jako artefakt

Kazda kampania powinna pozostawic:
- `summary.json`
- pliki CSV / JSONL / MD
- wykresy finalne
- provenance i `summary_hash`

## 2. Projekt eksperymentu

## 2.1. Zmienne niezalezne

### Warstwa architektoniczna
- backend
- architektura sprzetowa
- model urzadzenia
- wersja sterownika / runtime
- system operacyjny

### Warstwa kernela
- operator
- element_type
- wariant `qss/sqs/ssq`
- `coal_read`
- `coal_write`
- `compute_all_shape_fun_der`
- `workspace_*`
- `padding`
- `workgroup_size`
- `n_qp`
- `n_elements`

## 2.2. Zmienne zalezne

- `elapsed_s`
- `throughput_gbps`
- `gflops`
- `ns_per_unit`
- energia i moc, jesli dostepne
- `max_abs_diff`
- `rms_diff`
- `records_within_tolerance`
- wyniki profilerow

## 3. Warstwa microbenchmarks

## 3.1. Cel

Zmierzyc:
- peak bandwidth,
- pointer latency,
- TLB / page-walk latency,
- peak compute,
- sensitivity na rozmiar i wzorzec pracy.

## 3.2. Minimalna kampania

Na kazdej platformie:
- CPU microbenchmarks
- GPU microbenchmarks dla aktywnego backendu

## 3.3. Rekomendowane powtorzenia

- smoke test: `1`
- exploratory: `3`
- paper-ready: `>= 5`

## 3.4. Artefakty

- CSV benchmarkow
- roofline
- session summary
- wygenerowane wykresy

## 4. Warstwa FEM option validation

## 4.1. Cel

Sprawdzic, jak backend reaguje na kontrolowane probe'y bliskie opcjom realistycznego kernela FEM.

## 4.2. Parametry startowe

Dla smoke test:
- `repeats = 1`
- `n_elements = 64`
- `n_qp = 2`
- `workgroup_size = 32`

Dla kampanii publikacyjnej:
- `repeats = 3..5`
- `n_elements` i `n_qp` zgodne z kampania docelowa
- `workgroup_size` dobrany do architektury lub utrzymany jako stale pole porownawcze

## 4.3. Artefakty

- `fem_option_validation.csv`
- `records.jsonl`
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`
- `summary.json`

## 4.4. Kryteria interpretacji

- `mean_delta_ratio < 1.0` oznacza, ze stan toggled jest korzystniejszy od baseline
- `recommended_state` musi byc interpretowane w kontekscie probe'u
- probe'y z kategorii `profile` nie sa pojedynczymi kontrolami i nie nalezy ich interpretowac jak pojedynczych toggle'i

## 5. Warstwa reference exact

## 5.1. Cel

Uruchomic referencyjna kampanie legacy FEM jako zrodlo:
- frozen inputs,
- expected outputs,
- metadata launchu,
- exact timings.

## 5.2. Srodowisko

- Linux
- OpenCL / Intel OpenCL exact
- przygotowany `mod_2022`

## 5.3. Artefakty

- `summary.json`
- `combined_csv`
- `launch_dumps/`
- `numerical_outputs/`
- `replay_inputs_bundle/`
- `canonical_replay_bundles/`

## 6. Warstwa correctness replay

## 6.1. Cel

Sprawdzic, czy inny backend liczy to samo na tych samych frozen inputs.

## 6.2. Minimalny protokol

1. Wygeneruj `reference_exact`
2. Wyeksportuj compact replay bundle
3. Uruchom replay na backendzie docelowym
4. Sprawdz:
   - `records_checked`
   - `records_within_tolerance`
   - `records_out_of_tolerance`
   - `worst_max_abs_diff`
   - `worst_rms_diff`

## 6.3. Kryterium akceptacji

Minimalne:
- `records_out_of_tolerance = 0`

Jesli nie jest spelnione:
- replay nie moze byc uznany za potwierdzenie correctness
- trzeba zbadac rozbieznosc numeryczna

## 7. Warstwa native performance campaign

## 7.1. Cel

Zbadac zachowanie natywnej implementacji projektu na wielu backendach.

## 7.2. Ograniczenie interpretacyjne

Wyniki natywnej kampanii:
- sa bardzo cenne wydajnosciowo,
- ale nie stanowia same w sobie dowodu 1:1 correctness wobec legacy exact.

Kazda tabela i wykres z tej warstwy powinny to jasno rozrozniac.

## 8. Profiler correlation

## 8.1. Cel

Scalic:
- microbench peaks,
- probe deltas,
- best exact/native configuration,
- raporty profilerowe.

## 8.2. Minimalne wejscia

Wymagane:
- katalog optimization / exact-native runu
- katalog `fem_option_validation`

Opcjonalne:
- eksporty profilerow

## 8.3. Rekomendowane profilery

### Intel
- VTune
- Advisor / roofline, jesli dostepne

### Apple
- Xcode GPU tools
- Metal System Trace / GPU capture

### NVIDIA
- Nsight Compute
- Nsight Systems

### AMD
- rocprof / rocprofv2

## 8.4. Minimalny plan licznikow

Na tyle, na ile pozwala platforma, zbieraj:
- achieved bandwidth
- achieved FLOPS / EU utilization / occupancy
- stall reasons
- kernel time
- memory transactions / cache behavior

## 9. Powtorzenia i statystyka

## 9.1. Minimalne zasady

- smoke test: `1`
- exploratory runs: `3`
- finalne wyniki do rozprawy: `>= 5`

## 9.2. Raportowane statystyki

Dla kazdej glownej metryki raportuj:
- mean
- sigma / std
- CV
- liczbe runow

Jesli to mozliwe, dodaj:
- 95% confidence interval

## 9.3. Ranking opcji

Przy porownywaniu opcji:
- raportuj nie tylko najlepszy punkt,
- ale tez stabilnosc rankingu miedzy runami / seriami.

## 10. Warmup, cache i stabilizacja

Przed kampania:
- wykonaj smoke test,
- odnotuj, czy pierwsze uruchomienie ma dodatkowy narzut kompilacji kernela,
- nie mieszaj warmup runow z wynikami finalnymi.

Dla GPU:
- zaznacz, czy pierwszy run zawiera JIT / kernel compilation overhead.

## 11. Zasady archiwizacji

Dla kazdej kampanii trzymaj:
- katalog wynikowy bez nadpisywania,
- `summary.json`,
- finalne CSV,
- finalne Markdown reporty,
- najwazniejsze PNG,
- bundle, jesli dotyczy.

Rekomendowany uklad:
- `artifacts/microbench/<backend>/...`
- `artifacts/fem_option_validation/<backend>/...`
- `artifacts/reference_exact/<backend>/...`
- `artifacts/correctness_replay/<backend>/...`
- `artifacts/profiler_correlation/<backend>/...`

## 12. Freeze eksperymentalny

Przed finalnym zbiorem danych:
- nie zmieniaj kodu eksperymentalnego,
- nie zmieniaj nazw artefaktow,
- zamroz:
  - wersje backendow,
  - wersje sterownikow,
  - wersje systemow,
  - ustawienia kampanii.

W praktyce:
- oznacz jeden snapshot `v3` jako finalny do danych rozprawowych.

## 13. Kryteria "paper-ready"

Wynik uznaj za gotowy do rozprawy, jesli:
- ma `summary.json`,
- ma `summary_hash`,
- ma provenance,
- ma stabilne powtorzenia,
- ma sensowna interpretacje,
- jest przypisany do jednej klasy eksperymentu,
- nie miesza correctness i performance.

## 14. Co raportowac w kazdym rozdziale eksperymentalnym

Minimalnie:
- konfiguracja sprzetowa,
- backend,
- workflow,
- parametry,
- liczba powtorzen,
- glowne metryki,
- najwazniejsze ograniczenia interpretacyjne.

## 15. Gotowa checklista przed finalnymi pomiarami

- [ ] wybrano finalne platformy
- [ ] ustalono profile `quick/exploratory/paper`
- [ ] sprawdzono exact replay na reprezentatywnych przypadkach
- [ ] przygotowano profiler export plan
- [ ] potwierdzono, ze wszystkie workflowy zapisują `summary.json`
- [ ] potwierdzono, ze wszystkie workflowy zapisują artefakty raportowe
- [ ] ustalono katalog archiwizacji finalnych wynikow
