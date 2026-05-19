# Figury Filipa po uporzadkowaniu publication-grade

## Cel
Generator figur Filipa nie ma juz produkowac duzej liczby eksploracyjnych wariantow, tylko maly, spójny rdzen figur do:
- rozprawy,
- artykulow,
- slajdow,
- paczek archiwalnych.

## Gdzie sie zapisuja
Dla runu w `data/optimization/<run>/` figury trafiaja teraz do:

```text
data/optimization/<run>/figures/thesis_core/
data/optimization/<run>/figures/appendix/
data/optimization/<run>/figures/manifests/
```

## Zestaw thesis-core
Do glównego tekstu generowane sa:

```text
filip_variant_qss.png
filip_variant_sqs.png
filip_variant_ssq.png
filip_autotuning_trace.png
filip_best_summary.png
filip_memory_compute_breakdown.png
```

## Appendix
Do appendixu trafia obecnie:

```text
filip_best_configuration_card.png
```

## Znaczenie figur

### `filip_variant_qss.png`, `filip_variant_sqs.png`, `filip_variant_ssq.png`
To sa trzy glowne wykresy krajobrazu konfiguracji.

Kazdy z nich:
- dotyczy jednego wariantu organizacji kernela,
- pokazuje wszystkie kombinacje ocenione w kampanii,
- porownuje operatorow na jednej osi opcji,
- nie przycina wolniejszych outlierow w osi Y.

To sa figury odpowiadajace na pytanie:
- jak dany wariant reaguje na ustawienia autotuningu?

### `filip_autotuning_trace.png`
Pokazuje przebieg strojenia w czasie.

Odpowiada na pytania:
- jak szybko znajdowany jest dobry punkt,
- czy poprawa jest stabilna,
- czy najlepszy wynik pojawia sie wcześnie czy dopiero pod koniec.

### `filip_best_summary.png`
To syntetyczne podsumowanie najlepszych wynikow dla wybranych operatorow.

Nadaje sie do glownego tekstu, bo streszcza wynik bez zalewania czytelnika pelnym krajobrazem opcji.

### `filip_memory_compute_breakdown.png`
Pokazuje interpretacje obciazenia w jezyku:
- read,
- compute,
- write.

To jest figura, ktora pozwala polaczyc wyniki aplikacyjne z wnioskami z roofline i mikrobenchmarkow.

### `filip_best_configuration_card.png`
To karta najlepszej konfiguracji.

Jest przydatna jako:
- appendix,
- material roboczy,
- szybka karta identyfikacyjna runu.

Nie jest traktowana jako centralna figura narracji.

## Czego juz domyslnie nie generujemy
W podstawowym publication pipeline nie eksponujemy juz jako glównych produktow:
- `article_filip_execution_time_by_option.png`
- `article_paper_option_times.png`
- `article_variant_option_times.png`
- `article_variant_option_times_full.png`
- `article_all_option_times_qss.png`
- `article_all_option_times_sqs.png`
- `article_all_option_times_ssq.png`
- `article_operator_<operator>_variants.png`
- `article_autotuning_settings_heatmap.png`
- `article_autotuning_overview.png`

Te widoki byly przydatne eksploracyjnie, ale generowaly nadmiar i niespójnosc.

## Manifest
Generator zapisuje tez manifest:

```text
data/optimization/<run>/figures/manifests/filip_figures_manifest.json
```

Manifest zawiera:
- katalogi figur,
- liste wygenerowanych plikow,
- wybranych operatorow,
- tryb kampanii,
- podstawowe metadane zestawu figur.

## Uruchomienie reczne

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python3 analysis/filip_article_plots.py --optimization-dir data/optimization/<run>
```

## Zasada interpretacyjna
Te figury nie maja "pokazac wszystkiego".
Maja pokazac:
- krajobraz konfiguracji,
- przebieg strojenia,
- najlepszy wynik,
- oraz charakter obciazenia compute-vs-memory.
