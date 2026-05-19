# Final portable Linux bundle

Ten tryb jest dodatkowy wobec glownego `Final`.
Nie rusza glownej wersji roboczej i sluzy do przygotowania lekkiej paczki na pendrive.

## Cel

Scenariusz docelowy:
- masz pendrive z `Final_portable`,
- podlaczasz go do komputera z Linuxem,
- odpalasz jeden launcher,
- bundle tworzy lokalne srodowisko Pythona wewnatrz paczki,
- wykrywa CPU/GPU/backendy,
- uruchamia wybrany pakiet testow,
- zapisuje raport zgodnosci i artefakty.

## Ograniczenia, ktorych nie da sie uczciwie ukryc

Portable nie znaczy "magicznie niezalezne od hosta".
Bundle:
- przenosi kod i skrypty,
- tworzy lokalne `.portable_env`,
- potrafi lokalnie zbudowac CPU / CUDA / HIP microbench libs,
- ale **nie przenosi sterownikow GPU, CUDA runtime, ROCm ani systemowego OpenCL ICD**.

Czyli host Linux musi miec co najmniej:
- `python3`,
- kompilator `gcc` lub `clang` dla CPU,
- sterowniki / runtime GPU, jesli chcesz sciezke GPU.

## Co wchodzi do bundle

Bundle zawiera:
- benchmarki CPU/GPU,
- real kernels,
- portable case dla kodu Filipa,
- pipeline figur i ZIP,
- panel WWW,
- desktop GUI,
- dokumentacje potrzebna do uruchamiania.

Bundle nie zawiera:
- historycznych danych z `data/`,
- `legacy/`,
- ciezkiego `Kod Filipa/mod_2022`,
- exact bundle jako domyslnej sciezki przenosnej.

## Budowa bundle z Final

Z repo `Final`:

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/Final
python3 scripts/build_portable_bundle.py --force
```

Domyslny wynik:
- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/Final_portable`

## Uruchomienie na hoście Linux

Na komputerze docelowym:

```bash
cd /media/<user>/<pendrive>/Final_portable
bash ./LAUNCH_PORTABLE.sh --package full
```

Przy pierwszym uruchomieniu bundle sam zrobi bootstrap lokalnego `.portable_env`.

## Dostepne pakiety

- `benchmarks`
  - `cpu_benchmark`
  - `gpu_benchmark` jesli GPU jest dostepne
- `real-kernels`
  - `cpu_real_kernels`
  - `gpu_real_kernels` jesli GPU jest dostepne
- `filip`
  - `fem_option_validation`
  - `filip_original` w trybie `portable_sweep`
- `full`
  - `full_thesis_pipeline` z `filip-case portable`

## Przyklady

Tylko benchmarki:

```bash
bash ./LAUNCH_PORTABLE.sh --package benchmarks --mode standard
```

Real kernels na wykrytym GPU:

```bash
bash ./LAUNCH_PORTABLE.sh --package real-kernels --backend auto --mode extended
```

Portable test Filipa:

```bash
bash ./LAUNCH_PORTABLE.sh --package filip --backend auto --filip-case portable
```

Pelna kampania z limitem watkow CPU:

```bash
bash ./LAUNCH_PORTABLE.sh --package full --cpu-threads 8 --mode extended
```

Desktop GUI:

```bash
bash ./LAUNCH_DESKTOP_GUI.sh
```

## Raport zgodnosci

Po bootstrapie i po kazdym uruchomieniu zapisuje sie:
- `portable/host_compat.json`
- `portable/host_compat.md`

Raport pokazuje:
- system,
- CPU i topologie,
- dostepne komendy (`gcc`, `nvcc`, `hipcc`, `nvidia-smi`, `clinfo`),
- moduly Pythona,
- dostepne backendy GPU,
- rekomendowany backend,
- ktore pakiety realnie pojda na danym hoście.

## Figury i ZIP

Launcher domyslnie po runie:
- regeneruje `thesis_core` figury,
- regeneruje figury Filipa dla najnowszego `filip_original`,
- przy pakiecie `full` buduje tez ZIP finalnych figur.

## Exact reference

Portable bundle nie traktuje `exact_reference` jako domyslnej sciezki, ale workflow jest nadal dostepny.
Powod:
- exact zalezy od duzo ciezszego toolchainu,
- i jest bardziej stanowiskowo-laboratoryjny niz pendrive-friendly.

Jesli chcesz exact na konkretnym Linuxie:
- uruchom raport zgodnosci hosta,
- zapewnij `OpenCL + oneAPI + MKL + mod_2022`,
- odpal `bash ./LAUNCH_PORTABLE.sh --workflow filip_original --filip-mode exact_reference --modfem-dir /sciezka/do/mod_2022`.
