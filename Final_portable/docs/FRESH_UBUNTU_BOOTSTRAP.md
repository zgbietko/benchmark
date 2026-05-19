## Fresh Ubuntu bootstrap

Ten bootstrap jest przygotowany pod świeży Ubuntu na laptopie podobnym do Twojego:

- Intel CPU
- Intel iGPU
- NVIDIA dGPU
- projekt odpalany z klonu tego repo

Główny skrypt:

- [bootstrap_fresh_ubuntu_benchmark.sh](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/scripts/bootstrap_fresh_ubuntu_benchmark.sh)

### Założenia

- repo jest już sklonowane
- masz internet
- możesz użyć `sudo`
- system to Ubuntu

### Domyślny tryb

Skrypt domyślnie:

- instaluje pakiety systemowe pod Python/OpenCL/build
- instaluje Intel OpenCL runtime, jeśli pakiety są dostępne
- instaluje minimalny Intel oneAPI potrzebny do `exact_reference`
- instaluje NVIDIA driver, jeśli nie jest obecny
- instaluje CUDA toolkit
- konfiguruje `git user.name`, `git user.email` i klucz SSH do GitHuba
- tworzy `.venv`
- instaluje zależności Python
- buduje CPU lib i próbuje zbudować CUDA lib, jeśli `nvcc` jest dostępne

### Zalecane uruchomienie po świeżej instalacji

```bash
cd /sciezka/do/repo
chmod +x scripts/bootstrap_fresh_ubuntu_benchmark.sh
./scripts/bootstrap_fresh_ubuntu_benchmark.sh
```

### Warianty

Bez CUDA toolkit:

```bash
./scripts/bootstrap_fresh_ubuntu_benchmark.sh --skip-cuda-toolkit
```

Bez dotykania sterownika NVIDIA:

```bash
./scripts/bootstrap_fresh_ubuntu_benchmark.sh --skip-nvidia-driver
```

Bez exact oneAPI:

```bash
./scripts/bootstrap_fresh_ubuntu_benchmark.sh --skip-oneapi-exact
```

### GitHub / push / pull

Bootstrap:

- ustawia `git user.name` i `git user.email`
- generuje klucz `~/.ssh/id_ed25519`, jeśli go nie ma
- zapisuje publiczny klucz do:
  - `scripts/generated/github_id_ed25519.pub`
- jeśli `origin` jest `https://github.com/...`, przełącza go na `git@github.com:...`

Po bootstrapie musisz tylko wkleić publiczny klucz do GitHuba:

- [GitHub SSH keys](https://github.com/settings/keys)

### Aktywacja środowiska po restarcie/logowaniu

Skrypt tworzy helper:

- [activate_benchmark_env.sh](/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v2/scripts/generated/activate_benchmark_env.sh)

Uruchom:

```bash
source scripts/generated/activate_benchmark_env.sh
```

### Exact Filip

Po bootstrapie:

```bash
source scripts/generated/activate_benchmark_env.sh
python run_workflow.py --workflow filip_original --backend intel --filip-mode exact_reference --filip-case laplace_prism
```

### Uwaga o grupach

Jeśli skrypt dodał użytkownika do grup:

- `render`
- `video`

to przed testami Intel OpenCL wyloguj się i zaloguj ponownie.
