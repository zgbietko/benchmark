# Rozdzial 2. Obliczenia numeryczne i metoda elementow skonczonych

## 2.1. Obliczenia numeryczne jako przyblizanie rozwiazan

W naukach stosowanych wiele problemow opisuje sie rownaniami rozniczkowymi. Rzeczywiste geometrie, warunki brzegowe oraz wspolczynniki materialowe sprawiaja jednak, ze dla duzej klasy takich problemow nie istnieje wygodna postac analityczna rozwiazania. Wtedy stosuje sie metody numeryczne.

Ich istota polega na tym, ze komputer nie znajduje idealnego wzoru na rozwiazanie, lecz buduje jego przyblizenie przez dyskretyzacje problemu i wykonanie duzej liczby obliczen elementarnych.

## 2.2. Intuicja metody elementow skonczonych

Metoda elementow skonczonych (FEM) nalezy do najwazniejszych technik dyskretyzacji problemow ciaglych. Jej idea polega na podziale badanego obszaru na skonczona liczbe malych elementow. Zamiast rozwiazywac problem globalnie i jednorazowo, rozwaza sie wiele malych fragmentow, dla ktorych mozna sformulowac lokalne zaleznosci matematyczne.

W ten sposob:
- rzeczywisty obszar zastepuje sie siatka elementow,
- pole rozwiazania aproksymuje sie funkcjami ksztaltu,
- a lokalne uklady sklada sie potem w uklad globalny.

## 2.3. Element, wezel i funkcja ksztaltu

Podstawowymi obiektami FEM sa:
- **element** - maly fragment geometrii,
- **wezel** - charakterystyczny punkt elementu,
- **funkcja ksztaltu** - funkcja laczaca wartosci w wezlach z wartoscia wewnatrz elementu.

W praktyce to oznacza, ze rozwiazanie nie jest przechowywane wszedzie, lecz w wybranych punktach, a wartosci pomiedzy nimi sa rekonstruowane przez aproksymacje.

## 2.4. Calkowanie numeryczne i punkty Gaussa

W wielu formulacjach FEM trzeba policzyc calki po objetosci lub powierzchni elementu. W praktyce wykonuje sie to przez calkowanie numeryczne, zwykle z uzyciem punktow Gaussa. Dla kazdego punktu obliczane sa odpowiednie skladniki funkcji ksztaltu, pochodnych, geometrii i wspolczynnikow rownania.

Z punktu widzenia wydajnosci jest to wazne, bo:
- liczba punktow całkowania bezposrednio wplywa na liczbe operacji,
- dane potrzebne w tych punktach generuja charakterystyczne wzorce odczytu i zapisu,
- a organizacja ich obslugi jest jednym z glownych tematow strojenia kernela.

## 2.5. Lokalna macierz i lokalny wektor

Dla pojedynczego elementu FEM buduje sie zwykle lokalna macierz i lokalny wektor prawej strony. Lokalna macierz opisuje relacje pomiedzy stopniami swobody elementu, natomiast lokalny wektor odpowiada za skladniki wymuszajace lub zrodlowe.

To wlasnie na poziomie budowy tych lokalnych obiektow wykonuje sie duza czesc kosztownych obliczen w realistycznym kernelu.

## 2.6. Dlaczego kernel elementowy jest dobrym obiektem badan wydajnosciowych

Kernel elementowy ma kilka cech, ktore czynia go bardzo dobrym obiektem badan:
- jest obliczeniowo intensywny,
- intensywnie korzysta z pamieci,
- daje sie wykonac bardzo wiele razy dla wielu elementow,
- a jego organizacja moze byc zmieniana bez zmiany celu matematycznego obliczenia.

To oznacza, ze nawet drobne roznice w organizacji pracy przekladaja sie na mierzalne roznice czasowe i energetyczne.

## 2.7. Co tak naprawde porownujemy w warstwie FEM

W analizie `v3` nie chodzi o spor miedzy dwiema rozna fizyka. Chodzi o porownanie roznych sposobow wykonania tego samego typu obliczenia elementowego.

Zmieniane moga byc m.in.:
- wzorce odczytu i zapisu,
- decyzja o ponownym liczeniu lub przechowywaniu danych,
- wykorzystanie pamieci roboczej,
- rozklad pracy miedzy watki,
- uporzadkowanie danych w pamieci.

Kazda z tych decyzji nie musi zmieniac tego, *co* liczymy, ale moze zmieniac to, *jak drogo* sprzetowo wykonywane jest obliczenie.

## 2.8. Znaczenie tego rozdzialu dla metodologii

Ten rozdzial uzasadnia, dlaczego realistyczny kernel FEM nie jest jedynie dodatkowym benchmarkiem. Jest on reprezentantem rzeczywistego obciazenia obliczeniowego, na ktorym mozna sprawdzac, czy obserwacje z poziomu architektury i mikrobenchmarkow pozostaja prawdziwe w problemie o znaczeniu praktycznym.
