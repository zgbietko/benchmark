# Final: wieloplatformowosc i zasady uruchamiania

Ten dokument streszcza, co w wersji `Final` jest rzeczywiscie wieloplatformowe,
a co pozostaje warunkowe z powodow sprzetowych albo systemowych.

## Co jest wspolne miedzy platformami

Warstwa wspolna projektu:
- `run_workflow.py`
- profile kampanii (`standard`, `extended`, `full_cross_platform`)
- kontrakty artefaktow
- analiza i generowanie figur
- workflowy CPU
- workflowy FEM / Filip portable sweep / autotuning / Firefly
- web GUI
- desktop GUI (`run_desktop_gui.py`)
- builder portable (`scripts/build_portable_bundle.py`)

To oznacza, ze **rdzen pipeline'u jest multiplatformowy na poziomie kodu i orkiestracji**.

## Co zalezy od architektury / hosta

### Apple Silicon
- GPU path: `metal`
- exact: `exact_reference_metal_port` lokalnie
- replay 1:1: wymaga dumpow OpenCL wygenerowanych w kampanii Linux/OpenCL
- AI path: `metal` + probe `Core ML`

### NVIDIA
- GPU path: `cuda` (fallback `opencl`)
- CUDA microbenchmarki wymagaja biblioteki zbudowanej przez `nvcc`
- AI accel korzysta najlepiej z `cupy` / runtime CUDA
- `exact_reference` nie idzie przez CUDA, tylko przez Linux/OpenCL + oneAPI + assets `mod_2022`

### AMD
- GPU path: `hip` (fallback `opencl`)
- HIP microbenchmarki wymagaja biblioteki zbudowanej przez `hipcc`
- `exact_reference` pozostaje sciezka OpenCL/oneAPI, nie HIP

### Intel Arc / Intel iGPU
- GPU path: `opencl`
- `exact_reference` naturalnie pasuje do Linux/OpenCL + oneAPI
- AI accel pozostaje glownie OpenCL / CPU fallback

## Exact reference: co jest uniwersalne, a co nie

`exact_reference` nie jest identyczne na wszystkich hostach:
- macOS/Apple: dziala lokalny `metal exact-style port`
- macOS/Apple replay 1:1: wymaga dumpow OpenCL
- Linux/OpenCL: wymaga `pyopencl`, runtime OpenCL, `icx`/oneAPI, MKL oraz assets `mod_2022`

Wniosek:
- **workflow istnieje wszedzie w tej samej strukturze**,
- ale **prerekwizyty runtime nie sa identyczne**.

## Energia i moc

Warstwa energii jest tylko `best-effort`:
- macOS: `powermetrics`, zwykle z `sudo`
- Linux + NVIDIA: `pynvml` / `nvidia-smi`
- Linux + CPU Intel: `RAPL` / `powercap`
- AMD / inne hosty: zalezne od tego, czy host udostepnia liczniki

To oznacza, ze sam pipeline jest wspolny, ale jakosc danych energetycznych zalezy od hosta.

## Final_portable

`Final_portable` ma byc funkcjonalnie rownowazny z `Final` na Linuxie w tym sensie, ze:
- przenosi wszystkie workflowy,
- ma te same launchery i te same skrypty analityczne,
- moze uruchomic dowolny workflow przez `--workflow`.

Nie znaczy to jednak, ze pendrive przenosi sterowniki i toolchainy.
Host nadal musi miec lokalnie:
- sterowniki GPU,
- runtime CUDA / ROCm / OpenCL,
- opcjonalnie oneAPI + `mod_2022` dla Linux `exact_reference`.

## Jak sprawdzic host przed eksperymentem

```bash
python3 scripts/platform_matrix_audit.py \
  --md-out reports/platform_matrix_audit.md \
  --json-out reports/platform_matrix_audit.json
```

Dla bundle portable:

```bash
python3 scripts/portable_compat_report.py \
  --md-out portable/host_compat.md \
  --json-out portable/host_compat.json
```

To powinno byc uruchamiane na kazdym nowym hoscie przed benchmarkami do rozprawy.

Pelna checklista odbioru hosta:
- `docs/HOST_ACCEPTANCE_CHECKLIST.md`
