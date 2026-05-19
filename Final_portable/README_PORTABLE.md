# Final portable Linux bundle

Ta paczka jest dodatkowa wobec glownej wersji `Final`.
Ma sluzyc do przeniesienia projektu na pendrive i uruchamiania testow na obcym Linuxie
z zachowaniem tej samej logiki workflowow co w glownej wersji.

Najwazniejsze ograniczenia:
- bundle przenosi kod, skrypty i lokalne srodowisko Pythona tworzone na hoscie,
- ale nie przenosi sterownikow GPU, CUDA, ROCm ani systemowego OpenCL runtime,
- `exact_reference` na Linux/OpenCL nadal wymaga oneAPI + assets `mod_2022`,
- jesli historyczny `filip_exact_bundle` jest uszkodzony albo nieczytelny, portable exact uruchamiaj z zewnetrznym `--modfem-dir`,
- na Apple exact-style Metal port dziala lokalnie w glownej wersji `Final`, a nie przez ten linuxowy launcher.

Szybki start:
```bash
cd /sciezka/do/Final_portable
bash ./LAUNCH_PORTABLE.sh --package full
```

Jesli host nie ma jeszcze lokalnego env, launcher sam uruchomi bootstrap.

Glowne pakiety:
- `benchmarks`
- `real-kernels`
- `filip`
- `full`

Mozna tez uruchamiac dowolny workflow bezposrednio:
```bash
bash ./LAUNCH_PORTABLE.sh --workflow ai_accel
bash ./LAUNCH_PORTABLE.sh --workflow filip_original --filip-mode exact_reference --modfem-dir /sciezka/do/mod_2022
```

Desktop GUI:
```bash
bash ./LAUNCH_DESKTOP_GUI.sh
```

Dokumentacja:
- `docs/PORTABLE_LINUX_BUNDLE.md`
- `docs/HOST_ACCEPTANCE_CHECKLIST.md`
