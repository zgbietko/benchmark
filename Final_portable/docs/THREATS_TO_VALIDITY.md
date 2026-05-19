# Zagrozenia dla trafnosci w v3

Ten dokument porzadkuje zagrozenia dla trafnosci wynikow i sposob ich ograniczania.

## 1. Trafnosc konstruktu

Pytanie:
- czy mierzymy to, co twierdzimy, ze mierzymy?

### Zagrozenie 1. Mylenie czasu z poprawnoscia

Ryzyko:
- podobny czas wykonania nie oznacza automatycznie poprawnego wyniku obliczen.

Mitigacja:
- correctness replay na frozen inputs,
- porownanie outputow przez `max_abs_diff` i `rms_diff`.

### Zagrozenie 2. Mylenie `native_performance_campaign` z exact replay

Ryzyko:
- natywna kampania projektu moze byc przedstawiana jako strict 1:1 wobec legacy exact.

Mitigacja:
- jawne rozroznienie klas eksperymentow,
- osobne nazewnictwo i osobne sekcje w rozprawie.

### Zagrozenie 3. Mylenie high-level input z rzeczywistym wejściem kernela

Ryzyko:
- samo `mesh + problem file` moze byc uznane za identyczne wejscie.

Mitigacja:
- frozen runtime buffers:
  - `execution_parameters.bin`
  - `gauss_dat.bin`
  - `shape_fun_ref.bin`
  - `el_data_in.bin`

## 2. Trafnosc wewnetrzna

Pytanie:
- czy obserwowane roznice sa skutkiem badanych czynnikow, a nie ukrytych zaklocen?

### Zagrozenie 1. JIT / first-run overhead

Ryzyko:
- pierwszy run moze zawierac koszt kompilacji kernela.

Mitigacja:
- smoke test / warmup,
- jawne odnotowanie pierwszego runu,
- nie mieszanie warmup z wynikami finalnymi.

### Zagrozenie 2. Niekontrolowane obciazenie systemu

Ryzyko:
- inne procesy zaburzaja CPU/GPU timings.

Mitigacja:
- odseparowane sesje pomiarowe,
- ograniczenie innych zadan,
- zapisywanie provenance i kontekstu uruchomienia.

### Zagrozenie 3. Dynamiczne zarzadzanie energia i taktowaniem

Ryzyko:
- turbo, power limit, thermal throttling, DVFS.

Mitigacja:
- notatki o stanie systemu,
- powtorzenia,
- interpretacja z profilerem i energetyka tylko tam, gdzie ma to sens.

### Zagrozenie 4. Roznice w sterownikach i runtime

Ryzyko:
- backendy sa uruchamiane na roznych wersjach runtime i kompilatorow.

Mitigacja:
- provenance,
- hashowanie,
- zapisywanie wersji systemu i srodowiska,
- freeze eksperymentalny przed zbiorem finalnym.

## 3. Trafnosc wnioskowania

Pytanie:
- czy wnioski statystyczne sa uzasadnione?

### Zagrozenie 1. Za malo powtorzen

Ryzyko:
- losowy szum jest interpretowany jako efekt architektoniczny.

Mitigacja:
- `>= 5` powtorzen dla finalnych wynikow,
- raportowanie mean, sigma, CV,
- dodatkowe confidence intervals, jesli to mozliwe.

### Zagrozenie 2. Nadmierna interpretacja jednego najlepszego wyniku

Ryzyko:
- najlepsza konfiguracja jest niestabilna, ale przedstawiana jako reprezentatywna.

Mitigacja:
- analiza rankingu,
- raportowanie top-k,
- porownanie stabilnosci miedzy seriami.

### Zagrozenie 3. Zbyt slabe kryterium correctness

Ryzyko:
- tolerancja jest ustawiona zbyt luzno.

Mitigacja:
- jawne raportowanie:
  - `records_checked`
  - `records_within_tolerance`
  - `records_out_of_tolerance`
  - `worst_max_abs_diff`
  - `worst_rms_diff`

## 4. External validity

Pytanie:
- na ile wyniki uogolniaja sie poza testowany zestaw?

### Zagrozenie 1. Ograniczona liczba architektur

Ryzyko:
- wnioski beda zbyt mocno zalezne od jednej platformy.

Mitigacja:
- przynajmniej:
  - CPU,
  - Apple GPU / Metal,
  - Intel GPU / OpenCL,
- opcjonalnie NVIDIA i AMD.

### Zagrozenie 2. Ograniczony zestaw przypadkow FEM

Ryzyko:
- wyniki sa silne tylko dla `test_prism` i `laplace_prism`.

Mitigacja:
- uczciwe ograniczenie zakresu twierdzen,
- rozwoj dodatkowych cases w przyszlosci.

### Zagrozenie 3. Dopasowanie probe'ow do jednego kernela

Ryzyko:
- `fem_option_validation` zbyt mocno odzwierciedla tylko jeden styl kernela.

Mitigacja:
- jawne opisanie, ze probe'y sa warstwa interpretacyjna dla tej klasy realistycznego kernela FEM,
- nie przedstawianie ich jako uniwersalnych benchmarkow dla calego HPC.

## 5. Tool validity

Pytanie:
- czy same narzedzia nie wprowadzaja bledow?

### Zagrozenie 1. Bledy parserow raportow

Ryzyko:
- zle sparsowany profiler daje mylne wnioski.

Mitigacja:
- przechowywanie raw exports,
- zachowanie plikow zrodlowych obok raportow korelacyjnych,
- parsowanie do CSV/JSON bez nadpisywania raw danych.

### Zagrozenie 2. Dekoder `el_data_out`

Ryzyko:
- bledna interpretacja plaskiego bufora.

Mitigacja:
- jawne ograniczenie obecnej wersji dekodera do wspieranych przypadkow,
- testy regresyjne na canonical bundles.

### Zagrozenie 3. Translator replay

Ryzyko:
- translator `OpenCL -> Metal` nie jest ogolnym kompilatorem i moze miec ograniczony zakres poprawnosci.

Mitigacja:
- utrzymywanie canonical replay bundles,
- walidacja tylko w zakresie wspieranych przypadkow,
- jawne opisanie ograniczen.

## 6. Ograniczenia, ktore trzeba powiedziec wprost w rozprawie

Nalezy jawnie napisac, ze:
- `native_performance_campaign` nie jest strict replayem 1:1,
- correctness replay ma zakres ograniczony do wspieranych przypadkow i bundli,
- profiler correlation jest warstwa interpretacyjna, nie dowodem przyczynowosci samym w sobie,
- benchmark referencyjny nie jest glownym wkladem pracy.

## 7. Najwazniejsze dzialania ograniczajace ryzyko

Jesli trzeba wskazac 5 najwazniejszych mitigacji, to sa nimi:

1. frozen-input correctness replay,
2. provenance i `summary_hash`,
3. rozdzielenie correctness od performance,
4. profiler-assisted correlation,
5. jawne raportowanie ograniczen i klasy eksperymentu.

## 8. Minimalna checklista do sekcji "Threats to Validity"

- [ ] rozrozniono construct/internal/external/conclusion validity
- [ ] opisano ograniczenia replayu
- [ ] opisano ograniczenia native campaigns
- [ ] opisano ograniczenia profilerow
- [ ] opisano ograniczenia dekodera outputu
- [ ] wskazano, jak provenance i bundling ograniczaja ryzyko
