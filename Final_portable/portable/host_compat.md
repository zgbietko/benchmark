# Raport kompatybilnosci i wieloplatformowosci

- Host: `MacBook-Pro-Mateusz.local`
- System: `Darwin 25.3.0 arm64`
- Python: `3.13.7`
- CPU: `Apple M2 Pro`
- Watki logiczne: `12` | fizyczne: `12`
- Podzial rdzeni: `8P + 4E`

## Backendy GPU

- Zalecany backend GPU: `metal`
- Zalecany backend FEM: `metal`
- `cuda`: `not available`
- `hip`: `not available`
- `metal`: `available`
- `opencl`: `not available`

## Workflowy

- `ai_accel`: `supported` - CPU fallback jest dostepny wszedzie; sciezki akcelerowane zalezne od backendu i runtime vendorowego.
- `cpu_benchmark`: `supported` - CPU benchmarki moga budowac biblioteki natywne.
- `cpu_real_kernels`: `supported` - CPU real kernels korzystaja z tego samego hosta i numpy/C backendu.
- `desktop_gui`: `supported` - tkinter + TkAgg sa dostepne.
- `fem_option_validation`: `supported` - Walidacja FEM ma resolved backend: metal.
- `filip_autotune`: `supported` - Autotuning korzysta z tego samego problemu FEM-like / Filip co portable sweep.
- `filip_exact_reference`: `supported` - Metal exact-style port jest dostepny. Replay 1:1 wymaga dumpow OpenCL z kampanii Linux/OpenCL.
- `filip_firefly`: `supported` - Firefly korzysta z tej samej warstwy wykonawczej co autotuning.
- `filip_original`: `supported` - Portable sweep dziala na CPU i wspieranych backendach GPU.
- `full_thesis_pipeline`: `supported` - Pelny pipeline jest dostepny; exact_reference moze byc lokalny albo warunkowy.
- `gpu_benchmark`: `supported` - Dostepne backendy GPU: metal.
- `gpu_real_kernels`: `supported` - GPU real kernels pojda przez: metal.
- `profiler_correlation`: `supported` - Analiza korelacyjna jest lokalna, zalezy od obecnosci artefaktow kampanii.
- `web_gui`: `supported` - Statyczny frontend i lokalny serwer HTTP sa dostepne.

## Exact reference

- Status: `supported`
- Tryb: `exact_reference_metal_port`
- Opis: Metal exact-style port jest dostepny. Replay 1:1 wymaga dumpow OpenCL z kampanii Linux/OpenCL.

## Energia i moc

- `powermetrics`: `conditional` | scope=`cpu+gpu` | admin=`True` | Na macOS zwykle wymaga sudo; bez tego pomiar przechodzi w best-effort / unsupported.

## Portable bundle

- build_supported: `True`
- run_supported_on_host: `False`
- reason: Launcher portable jest linuxowy; ten host nie jest Linuxem.

## Zadeklarowane profile platform

- `apple`: Apple Silicon + Apple GPU | backends=`metal,opencl` | exact=`metal exact-style port; replay 1:1 po dostarczeniu dumpow OpenCL` | ai=`metal + Core ML probe`
- `nvidia`: x86 + NVIDIA GPU | backends=`cuda,opencl` | exact=`exact_reference przez OpenCL/oneAPI, nie przez CUDA` | ai=`cuda / cupy; INT8 i FP16 najlepsze przy vendor runtime`
- `amd`: x86 + AMD GPU | backends=`hip,opencl` | exact=`exact_reference przez OpenCL/oneAPI, nie przez HIP` | ai=`HIP/OpenCL w tej wersji glownie sciezki diagnostyczne / proxy`
- `intel_arc`: x86 + Intel Arc (OpenCL baseline) | backends=`opencl` | exact=`OpenCL + oneAPI jest naturalna sciezka exact` | ai=`OpenCL / CPU fallback; vendor-native AI nie jest glowna sciezka tej wersji`
- `intel_igpu`: x86 + Intel iGPU (OpenCL baseline) | backends=`opencl` | exact=`OpenCL + oneAPI` | ai=`OpenCL / CPU fallback`

## Ostrzezenia

- Portable launcher jest przygotowany na host Linux; na tym hoście mozna go zbudowac, ale nie uruchomic w trybie pendrive-run.
- Na macOS energia/moc wymaga uruchomienia z uprawnieniami administratora dla wiarygodnych danych.
