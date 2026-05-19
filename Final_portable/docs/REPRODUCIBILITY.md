## Reproducibility guide (legacy base, adapted in v3)

Uwaga:
- ten dokument pochodzi z warstwy `v2` i nadal zawiera przydatne wskazowki ogolne,
- dla aktualnego workflowu `v3` najpierw czytaj:
  - `docs/V3_DOCUMENTATION_INDEX.md`
  - `docs/EXPERIMENTAL_PROTOCOL.md`
  - `docs/END_TO_END_TESTING_AND_WRITING.md`

Ten dokument opisuje, jak **powtarzalnie** uruchamiać benchmarki w
`apple_microbench_variant2_streamfix` tak, aby wyniki nadawały się do
wykorzystania w pracy doktorskiej.

### 1. Środowisko i zależności

- Wymagania ogólne:
  - Python 3.10+.
  - Zestaw standardowych bibliotek naukowych (`numpy`, `matplotlib`, itp.).
  - Dla GPU:
    - CUDA toolkit + sterowniki (dla backendu `cuda`),
    - ROCm (dla backendu `hip`),
    - sterownik z OpenCL (dla backendu `opencl`),
    - macOS 13+ z Metal (dla backendu `metal`).
- Dodatkowo dla energii:
  - Linux/x86: dostęp do `/sys/class/powercap/*rapl*/energy_uj` (RAPL).
  - macOS: narzędzie `powermetrics` (zwykle wymaga `sudo`, szczególnie dla GPU).
  - NVIDIA: `pynvml` lub `nvidia-smi` w PATH.

### 2. Budowanie bibliotek

CPU:

```bash
cd v2
cd cpu/lib
./build_linux.sh   # na Linux
./build_mac.sh     # na macOS
```

GPU:

- CUDA:

```bash
cd v2
cd gpu/cuda/lib
./build_cuda.sh
```

- HIP:

```bash
cd v2
cd gpu/hip/lib
./build_hip.sh
```

Metal i OpenCL nie wymagają budowania bibliotek natywnych (kod kerneli jest
kompilowany przez sterownik w czasie uruchomienia).

### 3. Podstawowe profile uruchomieniowe

Repozytorium korzysta z profili eksperymentów zdefiniowanych w:

- `configs/experiment_profiles.json`
- `configs/platform_profiles.json`

Główne profile:

- **quick** – minimalny zestaw rozmiarów i liczby runów (smoke / sanity).
- **paper** – domyślny profil do zbierania danych pod publikację.
- **full** – rozszerzony zestaw (większa rozdzielczość rozmiarów / więcej runów).

Przykład uruchomienia profilu `paper`:

```bash
cd v2
python3 run_all_backends.py --profile paper
```

Wygeneruje to katalog sesji w `data/runs/` oraz manifest (`manifest.json`).

### 4. Zalecenia dot. ustawień CPU i GPU

CPU:

- Jeśli to możliwe:
  - ustaw governor na `performance` (Linux),
  - wyłącz dynamiczne skalowanie częstotliwości (lub przynajmniej zanotuj jego stan),
  - nie uruchamiaj innych ciężkich zadań równolegle.
- W raportach zawsze zapisuj:
  - model CPU,
  - liczbę rdzeni / wątków,
  - informację o turbo / governorze (chociażby w formie tekstowej notatki).

GPU:

- Upewnij się, że GPU nie jest współdzielone z innymi ciężkimi zadaniami.
- Dla NVIDIA:
  - jeśli ustawiasz limit mocy, zanotuj go (np. `nvidia-smi -pl 120`).
- W raportach zapisuj:
  - model GPU,
  - wersję sterownika,
  - tryb pracy (domyślny / z limitem mocy).

### 5. Energia i jakość pomiaru

Pomiar energii realizowany jest przez:

- `energy.py` – głównie CPU (RAPL, powermetrics).
- `energy_utils.EnergyLogger` – CPU i GPU (NVML, `nvidia-smi`, sysfs, powermetrics GPU).

Rekomendacje:

- Dla **CPU/Linux**: uruchamiaj benchmarki jako zwykły użytkownik – RAPL nie wymaga sudo.
- Dla **CPU/GPU na macOS**: pomiary powermetrics zwykle wymagają `sudo`. Jeśli nie
  możesz używać sudo, traktuj energię jako **niedostępną** (fallback 0 J / 0 W
  nie powinien być interpretowany jako „prawdziwy” pomiar).
- W analizach:
  - korzystaj z pól `energy_source`, `energy_supported`, `energy_confidence`,
  - filtruj wiersze o niskim `energy_confidence` lub z `energy_source` zawierającym
    `fallback` / `unsupported`, jeśli chcesz rysować wykresy efektywności energetycznej.

### 6. Typowy pipeline „paper”

1. **Uruchom kampanię** (np. na maszynie z NVIDIA GPU):

   ```bash
   cd v2
   python3 run_all_backends.py --profile paper --platform-profile nvidia
   ```

2. **Sprawdź jakość danych**:

   ```bash
   cd v2
   python3 analysis/data_quality.py --scope session --strict
   ```

   Jeśli zwróci kod wyjścia 2 – sprawdź komunikaty, popraw błędy (np. dziwne CSV,
   brakujące pola) i powtórz pomiary.

3. **Wygeneruj raport**:

   ```bash
   python3 analysis/report.py --mode latest --roofline-target gpu --roofline-backend cuda --with-plots
   ```

   Raport pojawi się w `reports/report_YYYYMMDD_HHMMSS.md`.

4. **Zarchiwizuj artefakty**:

   - katalog sesji w `data/runs/`,
   - raport z `reports/`,
   - wersje sterowników (np. output `nvidia-smi`, `uname -a`, itp. – jako osobne pliki tekstowe).

### 7. Minimalne kryteria „paper-ready”

Przed użyciem wyników w publikacji:

- [ ] `analysis/data_quality.py --scope session --strict` przechodzi bez błędów.
- [ ] `analysis/gpu_summary.py --mode latest --strict` przechodzi bez błędów.
- [ ] Dla każdego kluczowego wyniku:
  - liczba runów ≥ 3,
  - raportujesz mean ± sigma i współczynnik zmienności (CV),
  - opisujesz źródło pomiaru energii.
- [ ] Istnieje zapisany manifest sesji oraz notatka o konfiguracji systemu (CPU, GPU, sterowniki).
