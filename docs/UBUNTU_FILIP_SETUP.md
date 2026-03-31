## Ubuntu setup for `Filip_original`

Ten dokument przygotowuje projekt do uruchomienia na maszynie Ubuntu z:

- CPU Intel
- GPU NVIDIA (`CUDA`)
- iGPU Intel (`OpenCL`)

Cel:

- uruchomić `Filip_original` możliwie najbliżej workflowu Filipa
- zebrać wyniki dla `CPU`, `CUDA` i `Intel OpenCL`
- wygenerować wykresy i pliki do porównania z opublikowanymi artykułami

### 1. Co zostało przygotowane w repo

Dodane pliki:

- [requirements-ubuntu.txt](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/requirements-ubuntu.txt)
- [scripts/setup_ubuntu_filip.sh](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/scripts/setup_ubuntu_filip.sh)
- [scripts/run_ubuntu_filip_validation.py](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/scripts/run_ubuntu_filip_validation.py)

Istotna poprawka:

- `intel` i `opencl` na maszynach mieszanych (`NVIDIA + Intel`) automatycznie wybierają właściwy OpenCL device Intela, zamiast ślepo brać `device-index 0`

### 2. Założenia systemowe

Na Ubuntu musisz mieć:

- działający sterownik NVIDIA
- działający `nvidia-smi`
- OpenCL runtime dla iGPU Intela
- dostęp do GUI, jeśli chcesz używać [run_autotune_gui.py](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/run_autotune_gui.py)

Skrypt setupu instaluje:

- pakiety systemowe potrzebne do Pythona, `tkinter`, `clinfo` i nagłówków OpenCL
- `tcsh` dla oryginalnego `Makefile_explicit` Filipa
- virtualenv
- `numpy`
- `matplotlib`
- `pyopencl`
- `pynvml`
- `cupy-cuda11x` albo `cupy-cuda12x`, jeśli wykryje zgodną wersję CUDA
- automatycznie buduje `cpu/lib/libmicrobench.so`
- automatycznie buduje `gpu/cuda/lib/libgpubench_cuda.so`, jeśli `nvcc` jest dostępne

### 3. Setup środowiska

W katalogu projektu uruchom:

```bash
chmod +x scripts/setup_ubuntu_filip.sh
./scripts/setup_ubuntu_filip.sh
```

Jeśli nie chcesz GUI:

```bash
./scripts/setup_ubuntu_filip.sh --no-gui
```

Jeśli chcesz też spróbować zbudować opcjonalne biblioteki mikrobenchmarków:

```bash
./scripts/setup_ubuntu_filip.sh --build-optional-libs
```

Po setupie aktywuj środowisko:

```bash
source .venv/bin/activate
```

### 4. Preflight

Sprawdzenie backendów:

```bash
python run_fem_parametric_preflight.py --backend cpu,cuda,intel --platform-profile auto
```

To pokaże:

- czy `CPU`, `CUDA` i `Intel OpenCL` są dostępne
- które urządzenia OpenCL są widoczne
- jaki `device_index` został faktycznie wybrany

Szybka lista wszystkich urządzeń:

```bash
python run_device_discovery.py --backends auto
```

Na maszynie z NVIDIĄ możesz wtedy osobno wybrać:

- `cuda`
- `opencl`

o ile runtime OpenCL dla GPU NVIDII jest zainstalowany i urządzenie pojawia się na liście.

### 5. Walidacja `Filip_original`

Pełna walidacja pod porównanie z artykułami:

```bash
python scripts/run_ubuntu_filip_validation.py --profile paper
```

Szybszy smoke test:

```bash
python scripts/run_ubuntu_filip_validation.py --profile quick --limit-option-rows 8
```

Domyślnie skrypt odpala:

- `CPU`
- `CUDA`
- `Intel OpenCL`

Domyślna metodyka:

- `Filip_original`
- `QSS/SQS/SSQ`
- `diffusion`
- `diffusion_convection_mass`
- `tet4`
- `float32`
- pełny constrained sweep opcji Filipa, chyba że podasz `--limit-option-rows`

### 6. Gdzie trafiają wyniki

Walidacja zapisuje wszystko do:

- `data/validation/<timestamp>__ubuntu_filip_validation`

W tym katalogu dostaniesz:

- `manifest.json`
- `validation_report.md`
- `logs/`

Każdy backend zapisuje swój właściwy run w:

- `data/optimization/<timestamp>__filip_original__backend-cpu`
- `data/optimization/<timestamp>__filip_original__backend-cuda`
- `data/optimization/<timestamp>__filip_original__backend-intel`

W środku są:

- `summary.json`
- `best.json`
- `evaluations.jsonl`
- `iterations.jsonl`
- CSV w stylu Filipa
- wykresy artykułowe

### 7. Co porównywać z artykułami

Najważniejsze pola:

- `ns / (element * qp)`
- przebieg dla wszystkich kombinacji opcji
- wykresy `QSS`, `SQS`, `SSQ`
- najlepsze konfiguracje per wariant

Najbardziej zbliżony do artykułów Intelowych jest backend:

- `intel`

`CUDA` traktuj jako rozszerzenie przenośne, nie 1:1 replika środowiska z publikacji Intel/OpenCL.

### 8. GUI na Ubuntu

Po setupie GUI powinno działać, jeśli system ma pakiet `python3-tk` i środowisko graficzne.

Uruchomienie:

```bash
python run_autotune_gui.py
```

Jeśli chcesz pomiary energii i masz odpowiednie uprawnienia/systemowe źródła:

```bash
sudo -E .venv/bin/python run_autotune_gui.py
```

W zakładce `Workflows`:

- wybierasz `Backend`
- klikasz `Refresh Devices`
- z listy `Detected device` wybierasz konkretny wpis

Po wyborze GUI ustawia właściwy `backend` i `device-index`.

Na Linuxie GUI nie jest wymagane do walidacji `Filip_original`. Cała walidacja działa także z CLI.

### 9. Exact reference mode `1:1` z kodem Filipa

Jeśli chcesz sprawdzić zgodność z dawnymi plikami referencyjnymi na tym samym procesorze, użyj trybu:

- `filip_mode = exact_reference`

Ten tryb:

- buduje oryginalny `mod_2022`
- uruchamia natywny binarny tor OpenCL Filipa
- używa oryginalnych `options.txt`
- czyta natywne pole `internal` z oryginalnego CSV

To jest jedyny tryb w tym repo, który jest sensownie zbliżony do testu `100%` względem dawnych referencji.

Wymagania dodatkowe:

- działający OpenCL Intela
- narzędzia kompilacyjne zgodne z plikami `make.arc_laplace` i `make.arc_test`
- `csh/tcsh`, bo oryginalny `Makefile_explicit` używa `SHELL = /bin/csh`
- w praktyce: `icx` + Intel oneAPI MKL/OpenMP albo ręcznie dostosowany plik `make.*`

Na aktualnym bootstrapie Ubuntu exact runner sam:

- dobiera zgodny root `oneAPI compiler/latest`
- ustawia runtime `PATH` i `LD_LIBRARY_PATH` dla `icx` oraz `MKL`
- rozpakowuje `mesh_prism.dmp.zip` do workspace exact
- omija problematyczne `deep_clean` ze starego `Makefile_explicit`

Dzięki temu exact mode powinien uruchamiać się z GUI i z `run_workflow.py` bez ręcznych `export`.

Uruchomienie:

```bash
python run_workflow.py \
  --workflow filip_original \
  --backend intel \
  --filip-mode exact_reference \
  --filip-case prism_pair
```

Tylko `laplace_prism`:

```bash
python run_workflow.py \
  --workflow filip_original \
  --backend intel \
  --filip-mode exact_reference \
  --filip-case laplace_prism
```

Tylko `test_prism`:

```bash
python run_workflow.py \
  --workflow filip_original \
  --backend intel \
  --filip-mode exact_reference \
  --filip-case test_prism
```

Bez przebudowy, jeśli binaria są już gotowe:

```bash
python run_filip_reference_exact.py \
  --backend intel \
  --benchmark-case prism_pair \
  --skip-build
```

Wyniki exact mode trafią do `data/optimization/...__filip_original__backend-opencl__exact` i będą zawierały:

- `summary.json`
- `evaluations.jsonl`
- `iterations.jsonl`
- `csv/result_filip_original__opencl.csv`
- `plots/article_paper_option_times.png`

To ten katalog ładujesz potem do porównania z dawnymi `xlsx/csv`.
