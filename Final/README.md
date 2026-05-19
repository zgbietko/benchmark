# apple_microbench Final

`Final` scala stabilne `v6` z audytem wieloplatformowosci, pelnym launcherem portable
oraz natywnym desktop GUI podobnym do panelu WWW.

Szczegoly nowego workloadu:
- `docs/V6_AUTHOR_WORKLOAD.md`
- `docs/V6_AI_ACCELERATION.md`

Poprzednie `v4`, `v5` i `v6` pozostaja wersjami historycznymi / roboczymi.

Glowny cel projektu:
- mierzyc rzeczywiste limity CPU i GPU,
- przejsc od mikrobenchmarkow do realistycznych kernels,
- zbadac kod Filipa jako kampanie aplikacyjna,
- zwalidowac poprawnosc i zbudowac material publikacyjny do rozprawy i artykulow.

## Aktualny porzadek metodologiczny

Projekt jest uporzadkowany w 6 etapach badawczych:
1. `Charakterystyka platformy`
2. `Jadra uproszczone i real kernels`
3. `Most interpretacyjny FEM`
4. `Kampania aplikacyjna: kod Filipa`
5. `Walidacja poprawnosci obliczen`
6. `Synteza i interpretacja`

Najwazniejsze doprecyzowanie:
- `L1 / L2 / L3 / TLB / page-walk` to **czesc mikrobenchmarkow**, a nie osobny etap.

Dokument porzadkujacy caly pipeline:
- `docs/PIPELINE_BADAWCZY.md`

## Najwazniejsze workflowy

- `cpu_benchmark`
- `gpu_benchmark`
- `cpu_real_kernels`
- `gpu_real_kernels`
- `ai_accel`
- `fem_option_validation`
- `filip_original`
- `filip_autotune`
- `filip_firefly`
- `filip_exact_reference`
- `profiler_correlation`
- `full_thesis_pipeline`

## Dwa tryby benchmarkow

- `standard` = glowna sciezka porownawcza miedzy platformami
- `extended` = diagnostyka architektur-specyficzna dla Apple / NVIDIA / AMD / Intel

Zasada metodologiczna:
- `standard` sluzy do porownan miedzyarchitektonicznych,
- `extended` sluzy do interpretacji zachowania konkretnej platformy.

## Finalny publication-grade pipeline figur

Domyslny pipeline figur nie produkuje juz duzej liczby exploratory wykresow.
Generuje zwarty zestaw `thesis_core`.

### Globalny thesis-core
Katalog:
- `analysis/figures/thesis_core/`

Pliki:
- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `cpu_peak_compute_scaling.png`
- `cpu_memory_latency_hierarchy.png`
- `gpu_microbenchmark_suite.png`
- `platform_roofline_measured.png`
- `real_kernels_model_validation.png`
- `real_kernels_filip_contrast_map.png`
- `ai_accel_overview.png`
- `ai_accel_break_even.png`
- `ai_precision_scaling.png`

### Thesis-core dla kodu Filipa
Katalog:
- `data/optimization/<run>/figures/thesis_core/`

Pliki:
- `filip_variant_qss.png`
- `filip_variant_sqs.png`
- `filip_variant_ssq.png`
- `filip_autotuning_trace.png`
- `filip_best_summary.png`
- `filip_memory_compute_breakdown.png`

### Appendix dla kodu Filipa
Katalog:
- `data/optimization/<run>/figures/appendix/`

Pliki:
- `filip_best_configuration_card.png`

### Manifesty figur
- `analysis/figures/manifests/thesis_core_manifest.json`
- `data/optimization/<run>/figures/manifests/filip_figures_manifest.json`

Pelny audyt publikacyjny:
- `docs/PLOT_PUBLICATION_AUDIT.md`

## Panel WWW

Panel graficzny jest uproszczony do trzech glownych akcji:
- `Uruchom benchmarki`
- `Uruchom real kernels`
- `Uruchom test Filipa`

W pakiecie `Uruchom real kernels` znajduje sie tez krok `AI acceleration paths`.

Panel ma:
- paski postepu etapow,
- zwijane sekcje,
- podglad finalnych wykresow `thesis_core`,
- wybor aktualnie dostepnych backendow i urzadzen,
- budowe ZIP-a ze wszystkimi finalnymi figurami.

Dokumentacja:
- `docs/GRAPHICAL_PIPELINE.md`

Uruchomienie WWW:
```bash
cd /sciezka/do/Final
./scripts/run_graphical_pipeline.sh
```

Uruchomienie desktop GUI:
```bash
cd /sciezka/do/Final
./scripts/run_desktop_pipeline.sh
```

## Szybki start

### Pelna kampania
```bash
cd /sciezka/do/Final
python3 run_workflow.py --workflow full_thesis_pipeline --platform-profile auto --backend auto --device-index 0
```

### Wykresy globalne thesis-core
```bash
cd /sciezka/do/Final
python3 analysis/generate_plots.py
```

### Testy AI acceleration paths
```bash
cd /sciezka/do/Final
python3 run_workflow.py --workflow ai_accel --profile quick --backend auto --benchmark-mode standard --real-runs 3
```

### Wykresy Filipa
```bash
cd /sciezka/do/Final
latest_run="$(find data/optimization -maxdepth 1 -type d -name '*__filip_original__backend-*' | sort | tail -n 1)"
python3 analysis/filip_article_plots.py --optimization-dir "$latest_run"
```

### ZIP wszystkich finalnych figur
```bash
cd /sciezka/do/Final
python3 analysis/build_plot_zip.py --campaign-dir data/thesis_full/<campaign>
```

### Raport wieloplatformowosci hosta
```bash
cd /sciezka/do/Final
python3 scripts/platform_matrix_audit.py --md-out reports/platform_matrix_audit.md --json-out reports/platform_matrix_audit.json
```

### Checklista odbioru nowego hosta
- `docs/HOST_ACCEPTANCE_CHECKLIST.md`

## Najwazniejsze dokumenty

### Metodologia i rozprawa
- `docs/PIPELINE_BADAWCZY.md`
- `docs/THESIS_RESEARCH_PLAN.md`
- `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
- `docs/EXPERIMENTAL_PROTOCOL.md`
- `docs/THREATS_TO_VALIDITY.md`
- `docs/PLOT_PUBLICATION_AUDIT.md`

### Dokumentacja techniczna
- `docs/GRAPHICAL_PIPELINE.md`
- `docs/HOST_ACCEPTANCE_CHECKLIST.md`
- `docs/V4_FULL_PIPELINE.md`
- `docs/SYSTEM_REFERENCE.md`
- `docs/NOTEBOOKLM_SUMMARY.md`
- `docs/FILIP_TIMING_PLOTS.md`

## Portable Linux bundle

Dodatkowo obok glownego `Final` mozna zbudowac osobna wersje `Final_portable` na pendrive.

Budowa bundle:
```bash
cd /sciezka/do/Final
python3 scripts/build_portable_bundle.py --force
```

Domyslny wynik:
- `../Final_portable`

Na hoście Linux uruchamiasz:
```bash
cd /media/<user>/<pendrive>/Final_portable
bash ./LAUNCH_PORTABLE.sh --package full
```

Dowolny workflow portable:
```bash
bash ./LAUNCH_PORTABLE.sh --workflow ai_accel
bash ./LAUNCH_PORTABLE.sh --workflow filip_original --filip-mode exact_reference --modfem-dir /sciezka/do/mod_2022
```

## Automatyczna synchronizacja wyników do Google Drive

`Final` potrafi po kazdym runie automatycznie kopiowac artefakty do Google Drive.

Obslugiwane sa dwa tryby:
- `folder` - kopiowanie do lokalnego katalogu synchronizowanego przez Google Drive,
- `rclone` - wysylka przez skonfigurowany remote `rclone`.

Najprostszy wariant:
```bash
python3 run_workflow.py \
  --workflow cpu_benchmark \
  --profile quick \
  --google-drive-sync folder \
  --google-drive-dir "/sciezka/do/Google Drive" \
  --google-drive-subdir "doktorat_benchmarki"
```

Pelna kampania:
```bash
python3 run_workflow.py \
  --workflow full_thesis_pipeline \
  --profile full \
  --google-drive-sync folder \
  --google-drive-dir "/sciezka/do/Google Drive" \
  --google-drive-subdir "doktorat_benchmarki"
```

Mozna to ustawic tez raz przez zmienne srodowiskowe:
```bash
export FINAL_GOOGLE_DRIVE_SYNC=folder
export FINAL_GOOGLE_DRIVE_DIR="/sciezka/do/Google Drive"
export FINAL_GOOGLE_DRIVE_SUBDIR="doktorat_benchmarki"
```

Legacy `V5_GOOGLE_DRIVE_*` pozostaja obslugiwane jako fallback dla zgodnosci wstecznej.

Wtedy nawet runy odpalane z panelu WWW beda dzialaly z ta sama konfiguracja, o ile serwer zostanie uruchomiony w tym samym shellu.

Dokumentacja:
- `docs/PORTABLE_LINUX_BUNDLE.md`

## Najkrotsze podsumowanie

`Final` to uporzadkowana platforma doktorancka, w ktorej:
- mikrobenchmarki buduja model sprzetu,
- roofline zamienia ten model w narzedzie interpretacyjne,
- real kernels waliduja model,
- `fem_option_validation` buduje most do FEM,
- kod Filipa stanowi glowna kampanie aplikacyjna,
- `exact/replay` pilnuje poprawnosci,
- `profiler_correlation` spina wyniki w narracje naukowa.
