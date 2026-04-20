# OpenMP sprawodzdanie

## Autorzy: Jakub Rękas, Ernest Szlamczyk

## Spis treści

1. [Wstęp](#1-wstęp)
2. [Tworzenie losowych liczb](#2-tworzenie-losowych-liczb)
3. [Poszczególne warianty implementacji algorytmu](#3-poszczególne-warianty-implementacji-algorytmu)
    - [Wariant 2](#wariant-2)
    - [Wariant 3](#wariant-3)
4. [Uruchamianie na Aresie](#4-uruchamianie-na-aresie)
5. [Wyniki](#5-wniki)


## 1. Wstęp

Celem tego sprawozdania jest przedstawienie wykorzystania `OpenMP` - bazowanego na dyrektywach API stworzeonego do tworzenia równoległych programów na współdzielonych architekturach pamięciowych. W tym sprawozdaniu będzie badany wpływ różnych funkcji `OpenMP` na performance poszczególnych części algorytmu **sortowania kubełkowego** (*ang. bucket sort*). 

### Sortowanie kubełkowe

Sortowanie kubełkowe (*ang. bucket sort*) jest algorytmem sortowania działającym w czasie liniowo-oczekiwanym dla danych o jednostajnym rozkładzie. Dane wejściowe dzielone są na `k` kubełków, z których każdy odpowiada pewnemu przedziałowi wartości. Algorytm składa się z czterech faz:

1. **(b) Rozdział** — każdy element tablicy trafia do kubełka odpowiadającego jego wartości.
2. **(c) Sortowanie kubełków** — każdy kubełek sortowany jest niezależnie dowolnym algorytmem porównawczym (np. quicksort).
3. **(d) Wyznaczenie pozycji zapisu** — dla każdego kubełka obliczana jest pozycja startowa w tablicy wynikowej (prefix sum rozmiarów kubełków).
4. **(d) Zapis** — posortowane kubełki przepisywane są kolejno do tablicy wyjściowej.

**Złożoność** przy `k` kubełkach i `n` elementach:

| Przypadek | Złożoność |
|---|---|
| Oczekiwana (rozkład jednostajny, `k = n`) | O(n) |
| Pesymistyczna (wszystkie elementy w jednym kubełku) | O(n log n) |

**Potencjalne miejsca zrównoleglenia:**

| Faza | Niezależność iteracji | Możliwość zrównoleglenia | Uwaga |
|---|---|---|---|
| (b) Rozdział | Nie — wiele wątków może pisać do tego samego kubełka | Tak, ale wymaga synchronizacji | Konieczne blokady lub eliminacja współdzielenia |
| (c) Sortowanie kubełków | Tak — kubełki są od siebie niezależne | Tak | Warto zastosować harmonogramowanie dynamiczne ze względu na nierówne rozmiary kubełków |
| (d) Prefix sum | Nie — każdy termin zależy od poprzedniego | Zasadniczo sekwencyjne | O(k) — koszt pomijalny wobec pozostałych faz |
| (d) Zapis | Tak — każdy kubełek trafia do rozłącznego fragmentu tablicy | Tak | Brak wyścigu przy zapisie |

Wąskim gardłem jest **faza rozdziału** — jedyna wymagająca synchronizacji przy równoległym wykonaniu. Dalsze sekcje opisują dwie strategie jej zrównoleglenia: ochronę kubełków blokadami (wariant 2) oraz eliminację współdzielenia przez prywatne kubełki per wątek (wariant 3).

## 2. Tworzenie losowych liczb

// todo @jbrs

## 3. Poszczególne warianty implementacji algorytmu

// todo - opisać boilerplate @jbrs

### Wariant 2

// todo - jbrs

### Wariant 3

// todo - eszlamczyk

## 4. Uruchamianie na Aresie

// todo - opisać odpalanie na aresie + skrypty

## 5. Wniki

Sekcja wyniki została podzielona na trzy części:
- skrypty wynikowe
- prezentacja wyników poszczególnych wariantów
- porównanie wariantów między sobą

### Skrypty wynikowe

#### Format CSV

Każde uruchomienie programu dopisuje jeden wiersz do pliku CSV (bez nagłówka):

| Kolumna | Typ | Opis |
|---|---|---|
| `algorithm` | string | nazwa wariantu (`bs2` lub `bs3`) |
| `n_threads` | int | liczba wątków OpenMP |
| `n_elems` | int | rozmiar tablicy wejściowej |
| `t_random` | float | czas losowania tablicy [s] |
| `t_distribute` | float | czas rozdziału elementów do kubełków [s] |
| `t_merge` | float | czas scalania prywatnych kubełków [s] (0 dla bs2) |
| `t_sort` | float | czas sortowania kubełków [s] |
| `t_writeback` | float | czas prefix-sum + przepisania do tablicy [s] |
| `t_total` | float | całkowity czas sortowania (mierzony niezależnie) [s] |
| `sorted` | int | weryfikacja poprawności (`1` = posortowane, `0` = błąd) |

Przykładowy wiersz:
```
bs2,8,1000000,0.012481,0.043210,0.000000,0.031540,0.005320,0.080100,1
```

#### `utils/bs_common.py`

Współdzielony moduł używany przez oba skrypty rysujące. Definiuje nazwy kolumn (`COLS`), listę mierzonych faz (`COMPONENTS`: Distribute, Merge, Sort, Write-back), funkcję `load()` wczytującą CSV do ramki pandas oraz `fmt_n()` formatującą rozmiary czytelnie (`1000000` → `1M`).

#### `utils/plot_heatmap.py`

Generuje mapy cieplne w układzie **liczba wątków × N** dla każdego wariantu:

```bash
python utils/plot_heatmap.py <plik.csv> [katalog_wyjściowy]
```

| Plik wyjściowy | Zawartość |
|---|---|
| `<algo>_total_heatmap.png` | heatmapa `t_total` |
| `<algo>_components_heatmap.png` | siatka 2×2 heatmap dla czterech faz |

#### `utils/plot_slice.py`

Generuje wykresy liniowe dla ustalonego N lub ustalonej liczby wątków:

```bash
python utils/plot_slice.py <plik.csv> <wariant> <N|threads> <wartość>
# np.: python utils/plot_slice.py results/results.csv bs2 N 1000000
```

| Plik wyjściowy | Zawartość |
|---|---|
| `<algo>_<typ><wartość>_total.png` | `t_total` w funkcji wolnej zmiennej |
| `<algo>_<typ><wartość>_components.png` | siatka 2×2 dla czterech faz |

### Prezentacja wyników poszczególnych wariantów

// todo - kiedy bedą skrypty do odpalania

#### Wariant 2

#### Wariant 3

### Porównanie wariantów między sobą


